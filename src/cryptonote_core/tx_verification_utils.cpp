// Copyright (c) 2023-2024, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <boost/iterator/transform_iterator.hpp>

#include "carrot_impl/format_utils.h"
#include "common/threadpool.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "hardforks/hardforks.h"
#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/proof_len.h"
#include "fcmp_pp/prove.h"
#include "ringct/rctSigs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "verify"

#define VER_ASSERT(cond, msgexpr) CHECK_AND_ASSERT_MES(cond, false, msgexpr)
#define VER_ASSERT_EQ(a, b, msgexpr) VER_ASSERT(a == b, msgexpr << " (" << a << " != " << b << ")");

using namespace cryptonote;

// Sanity checks on expanded FCMP++ tx
static bool check_fcmp_pp_expanded_tx(const transaction& tx)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass verRctNonSemanticsSimple");

    const rct::rctSig& rv = tx.rct_signatures;
    VER_ASSERT(rct::is_rct_fcmp(rv.type), "Unexpected RCT type in post-FCMP tx expansion");

    VER_ASSERT(rv.mixRing.empty(),        "Non-empty mixRing after expanding FCMP tx");
    VER_ASSERT(rv.p.CLSAGs.empty(),       "Non-empty CLSAGs after expanding FCMP tx");
    VER_ASSERT(rv.p.MGs.empty(),          "Non-empty MGs after expanding FCMP tx");
    VER_ASSERT(rv.p.rangeSigs.empty(),    "Non-empty range sigs after expanding FCMP tx");
    VER_ASSERT(rv.p.bulletproofs.empty(), "Non-empty bulletproofs after expanding FCMP tx");
    VER_ASSERT(rv.pseudoOuts.empty(),     "Non-empty old pseudo outs after expanding FCMP tx");

    // Make sure the tree root is set
    VER_ASSERT(rv.p.fcmp_ver_helper_data.tree_root != nullptr, "tree_root is not set");

    // Check pseudoOuts size against transaction inputs
    const size_t n_inputs = rv.p.pseudoOuts.size();
    VER_ASSERT_EQ(n_inputs, tx.vin.size(), "Mismatched pseudo outs to inputs after expanding FCMP tx");
    VER_ASSERT_EQ(n_inputs, rv.p.fcmp_ver_helper_data.key_images.size(), "Mismatched key images to inputs after expanding FCMP tx");

    // For each input, check that the key images were copied into the expanded RCT sig correctly
    for (size_t n = 0; n < n_inputs; ++n)
    {
        const crypto::key_image& nth_vin_image = boost::get<txin_to_key>(tx.vin[n]).k_image;
        const bool ki_match = 0 == memcmp(&nth_vin_image, &rv.p.fcmp_ver_helper_data.key_images[n], 32);
        VER_ASSERT(ki_match, "Failed to check ringct signatures: mismatched FCMP key image");
    }

    return true;
}

// Sanity checks on expanded pre-FCMP, simple tx
static bool check_simple_pre_fcmp_expanded_tx(const transaction& tx, const rct::ctkeyM& mix_ring)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass verRctNonSemanticsSimple");

    const rct::rctSig& rv = tx.rct_signatures;
    VER_ASSERT(rct::is_rct_simple(rv.type) && !rct::is_rct_fcmp(rv.type),
        "Unexpected RCT type in pre-FCMP simple tx expansion");

    // Check that expanded RCT mixring == input mixring
    VER_ASSERT(rv.mixRing == mix_ring, "Failed to check ringct signatures: mismatched pubkeys/mixRing");

    // Check CLSAG/MLSAG size against transaction input
    const size_t n_sigs = rct::is_rct_clsag(rv.type) ? rv.p.CLSAGs.size() : rv.p.MGs.size();
    VER_ASSERT(n_sigs == tx.vin.size(), "Failed to check ringct signatures: mismatched input sigs/vin sizes");

    // For each input, check that the key images were copied into the expanded RCT sig correctly
    for (size_t n = 0; n < n_sigs; ++n)
    {
        const crypto::key_image& nth_vin_image = boost::get<txin_to_key>(tx.vin[n]).k_image;

        if (rct::is_rct_clsag(rv.type))
        {
            const bool ki_match = 0 == memcmp(&nth_vin_image, &rv.p.CLSAGs[n].I, 32);
            VER_ASSERT(ki_match, "Failed to check ringct signatures: mismatched CLSAG key image");
        }
        else
        {
            const bool mg_nonempty = !rv.p.MGs[n].II.empty();
            VER_ASSERT(mg_nonempty, "Failed to check ringct signatures: missing MLSAG key image");
            const bool ki_match = 0 == memcmp(&nth_vin_image, &rv.p.MGs[n].II[0], 32);
            VER_ASSERT(ki_match, "Failed to check ringct signatures: mismatched MLSAG key image");
        }
    }

    // Mix ring data is now known to be correctly incorporated into the RCT sig inside tx.
    return true;
}

// Sanity checks on expanded pre-FCMP, full tx
static bool check_full_pre_fcmp_expanded_tx(transaction& tx, const rct::ctkeyM& mix_ring)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass verRctNonSemanticsSimple");
    VER_ASSERT(tx.rct_signatures.type == rct::RCTTypeFull, "Unexpected RCT type in pre-FCMP full tx expansion");

    const rct::rctSig& rv = tx.rct_signatures;

    // check all this, either reconstructed (so should really pass), or not
    bool size_matches = true;
    for (size_t i = 0; i < mix_ring.size(); ++i)
        size_matches &= mix_ring[i].size() == rv.mixRing.size();
    for (size_t i = 0; i < rv.mixRing.size(); ++i)
        size_matches &= mix_ring.size() == rv.mixRing[i].size();
    if (!size_matches)
    {
        MERROR("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
        return false;
    }

    for (size_t n = 0; n < mix_ring.size(); ++n)
    {
        for (size_t m = 0; m < mix_ring[n].size(); ++m)
        {
            if (mix_ring[n][m].dest != rct::rct2pk(rv.mixRing[m][n].dest))
            {
                MERROR("Failed to check ringct signatures: mismatched pubkey at vin " << n << ", index " << m);
                return false;
            }
            if (mix_ring[n][m].mask != rct::rct2pk(rv.mixRing[m][n].mask))
            {
                MERROR("Failed to check ringct signatures: mismatched commitment at vin " << n << ", index " << m);
                return false;
            }
        }
    }

    if (rv.p.MGs.size() != 1)
    {
        MERROR("Failed to check ringct signatures: Bad MGs size");
        return false;
    }
    if (rv.p.MGs.empty() || rv.p.MGs[0].II.size() != tx.vin.size())
    {
        MERROR("Failed to check ringct signatures: mismatched II/vin sizes");
        return false;
    }
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
        if (memcmp(&boost::get<txin_to_key>(tx.vin[n]).k_image, &rv.p.MGs[0].II[n], 32))
        {
            MERROR("Failed to check ringct signatures: mismatched II/vin sizes");
            return false;
        }
    }

    return true;
}

static bool expand_fcmp_pp_tx(transaction &tx, const fcmp_pp::TreeRootShared &decompressed_tree_root)
{
    const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);
    if (!Blockchain::expand_transaction_2(tx, tx_prefix_hash, /*pubkeys=*/{}, decompressed_tree_root))
    {
        MERROR("Failed to expand FCMP++ tx");
        return false;
    }
    else if (!check_fcmp_pp_expanded_tx(tx))
    {
        MERROR("Failed post-expansion FCMP++ checks");
        return false;
    }
    return true;
}

static bool tx_ver_legacy_ring_sigs(transaction& tx, const rct::ctkeyM& mix_ring)
{
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass crypto::check_ring_signature");
    VER_ASSERT(tx.version == 1, "RingCT transaction will not pass crypto::check_ring_signature");

    VER_ASSERT(tx.signatures.size() == mix_ring.size(), "Wrong number of v1 mix rings");

    // This shape checks should be implied as part of serialization, but we re-check them here anyways
    VER_ASSERT(tx.signatures.size() == tx.vin.size(), "Wrong number of v1 ring signatures");

    // Calculate prefix hash
    const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);

    // Define job to run one call of crypto::check_ring_signature()
    std::atomic_flag fail_occurred{};
    const auto check_ring_signature_job = [&fail_occurred, &tx, &mix_ring, &tx_prefix_hash](const std::size_t input_idx)
    {
        const txin_to_key *pin = boost::get<txin_to_key>(&tx.vin[input_idx]);
        if (pin == nullptr || pin->key_offsets.size() != tx.signatures[input_idx].size())
        {
            MERROR("Transaction input is wrong type or ring member count mismatch");
            fail_occurred.test_and_set();
            return;
        }

        std::vector<const crypto::public_key *> p_output_keys;
        p_output_keys.reserve(mix_ring.at(input_idx).size());
        for (const rct::ctkey &key : mix_ring.at(input_idx))
        {
            // rct::key and crypto::public_key have the same structure, avoid object ctor/memcpy
            p_output_keys.push_back(reinterpret_cast<const crypto::public_key*>(&key.dest));
        }

        const bool ver = crypto::check_ring_signature(tx_prefix_hash,
            pin->k_image,
            p_output_keys,
            tx.signatures.at(input_idx).data());
        if (!ver)
        {
            MERROR("Failed to check ring signature for tx " << get_transaction_hash(tx) << "  vin key with k_image: "
                << pin->k_image << "  sig_index: " << input_idx);
            fail_occurred.test_and_set();
        }
    };

    // Multi-thread calls to check_ring_signature_job() for each input if available, else iterate on ths thread
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    const int threads = tpool.get_max_concurrency();
    const bool multi_threaded = threads > 1;
    std::unique_ptr<tools::threadpool::waiter> waiter(multi_threaded ? new tools::threadpool::waiter(tpool) : nullptr);
    for (std::size_t input_idx = 0; input_idx < tx.signatures.size(); ++input_idx)
    {
        if (waiter)
            tpool.submit(waiter.get(), [&, input_idx](){ check_ring_signature_job(input_idx); }, true);
        else
            check_ring_signature_job(input_idx);
    }
    if (waiter && !waiter->wait())
        return false;

    return !fail_occurred.test_and_set(); // test_and_set() returns previously held value
}

static bool is_canonical_bulletproof_layout(const std::vector<rct::Bulletproof> &proofs)
{
    if (proofs.size() != 1)
        return false;
    const size_t sz = proofs[0].V.size();
    if (sz == 0 || sz > BULLETPROOF_MAX_OUTPUTS)
        return false;
    return true;
}

static bool is_canonical_bulletproof_plus_layout(const std::vector<rct::BulletproofPlus> &proofs)
{
    if (proofs.size() != 1)
        return false;
    const size_t sz = proofs[0].V.size();
    if (sz == 0 || sz > BULLETPROOF_PLUS_MAX_OUTPUTS)
        return false;
    return true;
}

static bool is_canonical_fcmp_plus_plus_layout(const uint64_t reference_block, const uint8_t n_tree_layers, const std::size_t n_inputs, const std::size_t n_outputs, const fcmp_pp::FcmpPpProof &proof)
{
    // Must have non-0 reference block since tree does not have elems at genesis
    if (reference_block == 0)
        return false;
    // Tree must have layers if FCMP++ is included
    if (n_tree_layers == 0 || n_tree_layers > FCMP_PLUS_PLUS_MAX_LAYERS)
        return false;
    if (n_inputs == 0 || n_inputs > FCMP_PLUS_PLUS_MAX_INPUTS)
        return false;
    if (n_outputs == 0 || n_outputs > FCMP_PLUS_PLUS_MAX_OUTPUTS)
        return false;
    if (proof.empty())
        return false;
    const std::size_t act_sz = proof.size();
    if (act_sz == 0)
        return false;
    const std::size_t exp_sz = fcmp_pp::fcmp_pp_proof_len(n_inputs, n_tree_layers);
    if (act_sz != exp_sz)
        return false;
    return true;
}

template <class TxForwardIt>
static bool ver_non_input_consensus_templated(TxForwardIt tx_begin,
    TxForwardIt tx_end,
    const std::unordered_map<uint64_t, rct::key>& transparent_amount_commitments,
    tx_verification_context& tvc,
    std::uint8_t hf_version)
{
    std::vector<const rct::rctSig*> rvv;
    std::vector<rct::key> pubkeys_and_commitments;
    rvv.reserve(static_cast<size_t>(std::distance(tx_begin, tx_end)));
    pubkeys_and_commitments.reserve(static_cast<size_t>(std::distance(tx_begin, tx_end)) * 2);

    // We assume transactions have an unmixable ring since it's more permissive. The version is
    // checked again in Blockchain::check_tx_inputs() with `has_unmixable_ring` actually resolved.
    const size_t min_tx_version = get_minimum_transaction_version(hf_version, /*has_unmixable_ring=*/true);
    const size_t max_tx_version = get_maximum_transaction_version(hf_version);

    const size_t tx_weight_limit = get_transaction_weight_limit(hf_version);

    for (; tx_begin != tx_end; ++tx_begin)
    {
        const transaction& tx = *tx_begin;
        const uint64_t blob_size = get_transaction_blob_size(tx);

        // Rule 1
        if (blob_size > get_max_tx_size())
        {
            tvc.m_verifivation_failed = true;
            tvc.m_too_big = true;
            return false;
        }

        // Rule 2 and Rule 3
        if (tx.version < min_tx_version || tx.version > max_tx_version)
        {
            tvc.m_verifivation_failed = true;
            return false;
        }

        // Rule 8
        if (hf_version >= HF_VERSION_REJECT_UNLOCK_TIME && tx.unlock_time != 0)
        {
            tvc.m_verifivation_failed = true;
            tvc.m_nonzero_unlock_time = true;
            return false;
        }

        // Rule 9
        if (hf_version >= HF_VERSION_REJECT_LARGE_EXTRA && tx.extra.size() > MAX_TX_EXTRA_SIZE)
        {
            tvc.m_verifivation_failed = true;
            tvc.m_tx_extra_too_big = true;
            return false;
        }

        // Rule 4
        const size_t tx_weight = get_transaction_weight(tx, blob_size);
        if (hf_version >= HF_VERSION_PER_BYTE_FEE && tx_weight > tx_weight_limit)
        {
            tvc.m_verifivation_failed = true;
            tvc.m_too_big = true;
            return false;
        }

        // Rule 10
        if (!check_transaction_output_pubkeys_order(tx, hf_version))
        {
            tvc.m_verifivation_failed = true;
            tvc.m_invalid_output = true;
            return false;
        }

        // Rule 5
        if (!core::check_tx_semantic(tx, tvc, hf_version))
            return false;

        // Rule 6
        if (!Blockchain::check_tx_outputs(tx, tvc, hf_version) || tvc.m_verifivation_failed)
            return false;

        // We only want to check RingCT semantics if this is actually a RingCT transaction
        if (tx.version >= 2)
            rvv.push_back(&tx.rct_signatures);

        // Collect pubkeys and commitments for torsion check
        if (!collect_points_for_torsion_check(tx, transparent_amount_commitments, pubkeys_and_commitments))
        {
            tvc.m_verifivation_failed = true;
            return false;
        }
    }

    // Rule 7
    if (!ver_mixed_rct_semantics(std::move(rvv)))
    {
        tvc.m_verifivation_failed = true;
        tvc.m_invalid_input = true;
        return false;
    }

    // Rule 8
    // Note: technically this could be threaded with ver_mixed_rct_semantics
    if (!rct::verPointsForTorsion(pubkeys_and_commitments))
    {
        tvc.m_verifivation_failed = true;
        tvc.m_invalid_output = true;
        return false;
    }

    return true;
}

static bool collect_fcmp_pp_tx_verify_input(cryptonote::transaction &tx,
    const std::pair<crypto::ec_point, uint8_t> &tree_root_for_block_index,
    fcmp_pp::TreeRootShared &decompressed_tree_root_for_block_index_inout,
    fcmp_pp::FcmpPpVerifyInput &fcmp_pp_verify_input_out)
{
    VER_ASSERT(tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus, "expected FCMP++ RCT Type");
    VER_ASSERT(!tx.pruned, "expected unpruned FCMP++ tx");

    // Make sure tree metadata is correct
    const uint64_t ref_block_index = tx.rct_signatures.p.reference_block;
    const uint8_t n_tree_layers = tx.rct_signatures.p.n_tree_layers;

    VER_ASSERT(ref_block_index > 0, "tx reference_block must be > 0");
    VER_ASSERT(n_tree_layers > 0, "tx n_tree_layers must be > 0");

    VER_ASSERT(n_tree_layers == tree_root_for_block_index.second, "Unexpected tx n tree layers");

    const crypto::ec_point &tree_root = tree_root_for_block_index.first;

    // De-compress the tree root
    if (!decompressed_tree_root_for_block_index_inout)
    {
        const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
        decompressed_tree_root_for_block_index_inout = curve_trees->get_tree_root_from_bytes(n_tree_layers, tree_root);
        VER_ASSERT(decompressed_tree_root_for_block_index_inout != nullptr, "Failed to decompress root");
    }

    // Expand transaction
    if (!expand_fcmp_pp_tx(tx, decompressed_tree_root_for_block_index_inout))
        return false;

    // Now instantiate the FCMP++ verify input
    const auto &rv = tx.rct_signatures;
    const rct::key signable_tx_hash = rct::get_pre_mlsag_hash(rv, hw::get_device("default"));

    // Type conversion on pseudo outs
    const auto &pseudoOuts = rv.p.pseudoOuts;
    std::vector<crypto::ec_point> pseudo_outs;
    pseudo_outs.reserve(pseudoOuts.size());
    for (const auto &po : pseudoOuts)
        pseudo_outs.emplace_back(rct::rct2pt(po));

    fcmp_pp_verify_input_out = fcmp_pp::fcmp_pp_verify_input_new(
            rct::rct2hash(signable_tx_hash),
            rv.p.fcmp_pp,
            n_tree_layers,
            rv.p.fcmp_ver_helper_data.tree_root,
            pseudo_outs,
            rv.p.fcmp_ver_helper_data.key_images
        );

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace cryptonote
{

bool collect_points_for_torsion_check(const transaction& tx,
    const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments,
    std::vector<rct::key> &pubkeys_and_commitments_inout)
{
    for (std::size_t i = 0; i < tx.vout.size(); ++i)
    {
        // Don't need to collect points if we're not checking the tx outs for torsion
        if (!cryptonote::output_checked_for_torsion(tx.vout[i].target))
            continue;

        crypto::public_key output_pubkey;
        if (!cryptonote::get_output_public_key(tx.vout[i], output_pubkey))
            return false;
        rct::key pubkey = rct::pk2rct(output_pubkey);

        rct::key commitment;
        if (!cryptonote::get_commitment(tx, i, transparent_amount_commitments, commitment))
            return false;

        pubkeys_and_commitments_inout.emplace_back(pubkey);
        pubkeys_and_commitments_inout.emplace_back(commitment);
    }

    return true;
}

void collect_transparent_amount_commitments(
    const std::vector<std::reference_wrapper<const transaction>> &txs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout)
{
    // Note: we do not clear transparent_amount_commitments_inout because it may be a rolling cache

    for (const auto &tx_ref : txs)
    {
        const auto &tx = tx_ref.get();

        // We only need commitments for transparent amounts, which are tx version 1 || coinbase txs
        if (tx.version > 1 && !cryptonote::is_coinbase(tx))
            continue;
        for (const auto &tx_out : tx.vout)
        {
            const uint64_t amount = tx_out.amount;
            if (transparent_amount_commitments_inout.find(amount) == transparent_amount_commitments_inout.end())
                transparent_amount_commitments_inout[amount] = rct::zeroCommitVartime(amount);
        }
    }
}

std::vector<std::reference_wrapper<const transaction>> collect_transparent_amount_commitments(
    const transaction &miner_tx,
    const std::vector<std::pair<transaction, blobdata>> &tx_pairs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout)
{
    std::vector<std::reference_wrapper<const transaction>> tx_refs;
    tx_refs.reserve(1 + tx_pairs.size());
    tx_refs.push_back(std::cref(miner_tx));
    for (const auto &tx : tx_pairs)
        tx_refs.push_back(std::cref(tx.first));
    collect_transparent_amount_commitments(tx_refs, transparent_amount_commitments_inout);
    return tx_refs;
}

std::vector<std::reference_wrapper<const transaction>> collect_transparent_amount_commitments(
    const transaction &miner_tx,
    const std::vector<transaction> &txs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout)
{
    std::vector<std::reference_wrapper<const transaction>> tx_refs;
    tx_refs.reserve(1 + txs.size());
    tx_refs.push_back(std::cref(miner_tx));
    for (const auto &tx : txs)
        tx_refs.push_back(std::cref(tx));
    collect_transparent_amount_commitments(tx_refs, transparent_amount_commitments_inout);
    return tx_refs;
}

void collect_transparent_amount_commitments(
    const std::unordered_map<crypto::hash, std::pair<transaction, blobdata>> &txs_by_txid,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout)
{
    std::vector<std::reference_wrapper<const transaction>> tx_refs;
    tx_refs.reserve(txs_by_txid.size());
    for (const auto &tx_pair : txs_by_txid)
      tx_refs.push_back(std::cref(tx_pair.second.first));
    collect_transparent_amount_commitments(tx_refs, transparent_amount_commitments_inout);
}

uint64_t get_transaction_weight_limit(const uint8_t hf_version)
{
    // FIXME: get_transaction_weight_limit for FCMP++
    if (hf_version >= HF_VERSION_FCMP_PLUS_PLUS)
    {
        static bool print_once = true;
        if (print_once)
            MERROR("FIXME: get_transaction_weight_limit for FCMP++");
        print_once = false;
        return 1000000;
    }
    // from v8, limit a tx to 50% of the minimum block weight
    else if (hf_version >= HF_VERSION_PER_BYTE_FEE)
        return get_min_block_weight(hf_version) / 2 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    else
        return get_min_block_weight(hf_version) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
}

bool are_transaction_output_pubkeys_sorted(const std::vector<tx_out> &vout)
{
    crypto::public_key last_output_pubkey = crypto::null_pkey;
    for (const tx_out &o : vout) {
      crypto::public_key output_pubkey;
      if (!get_output_public_key(o, output_pubkey)) {
        return false;
      }
      else if (!(output_pubkey > last_output_pubkey)) {
        return false;
      }
      last_output_pubkey = output_pubkey;
    }

    return true;
}

bool check_transaction_output_pubkeys_order(const transaction_prefix &tx_prefix, const std::uint8_t hf_version)
{
    if (hf_version > HF_VERSION_FCMP_PLUS_PLUS || carrot::is_carrot_transaction_v1(tx_prefix))
        return are_transaction_output_pubkeys_sorted(tx_prefix.vout);
    return true;
}

size_t get_minimum_transaction_version(uint8_t hf_version, bool has_unmixable_ring)
{
    if (hf_version >= HF_VERSION_REJECT_UNMIXABLE_V1)
    {
        return 2;
    }
    else if (hf_version < HF_VERSION_ENFORCE_RCT)
    {
        return 1;
    }
    else // HF_VERSION_ENFORCE_RCT <= hf_version < HF_VERSION_REJECT_UNMIXABLE_V1
    {
        return has_unmixable_ring ? 1 : 2;
    }
}

size_t get_maximum_transaction_version(uint8_t hf_version)
{
    if (hf_version >= HF_VERSION_DYNAMIC_FEE)
    {
        return 2;
    }
    else // hf_version < HF_VERSION_DYNAMIC_FEE
    {
        return 1;
    }
}

bool ver_input_proofs_rings(transaction& tx, const rct::ctkeyM &dereferenced_mix_ring)
{
    // Hello future Monero dev! If you got this assert, read the following carefully:
    //
    // For this version of RCT, the way we guaranteed that verification caches do not generate false
    // positives (and thus possibly enabling double spends) is we take a hash of two things. One,
    // we use get_transaction_hash() which gives us a (cryptographically secure) unique
    // representation of all "knobs" controlled by the possibly malicious constructor of the
    // transaction. Two, we take a hash of all *previously validated* blockchain data referenced by
    // this transaction which is required to validate the ring signature. In our case, this is the
    // mix_ring. Future versions of the protocol may differ in this regard, but if this assumptions
    // holds true in the future, enable the verification hash by modifying the `untested_tx`
    // condition below.
    const bool untested_tx = tx.version > 2 || tx.rct_signatures.type > rct::RCTTypeBulletproofPlus;
    VER_ASSERT(!untested_tx, "Unknown TX type. Make sure RCT cache works correctly with this type and then enable it in the code here.");

    if (tx.version == 1)
    {
        return tx_ver_legacy_ring_sigs(tx, dereferenced_mix_ring);
    }
    else if (tx.version == 2)
    {
        const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);
        const bool exp_res = Blockchain::expand_transaction_2(tx, tx_prefix_hash, dereferenced_mix_ring, nullptr/*tree_root*/);
        VER_ASSERT(exp_res, "Failed to expand pre-FCMP++ RingCT signatures!");

        switch (tx.rct_signatures.type)
        {
        case rct::RCTTypeNull:
            MERROR("Null RingCT does not have input proofs to verify");
            return false;
        case rct::RCTTypeFull:
            VER_ASSERT(check_full_pre_fcmp_expanded_tx(tx, dereferenced_mix_ring),
                "Failed post-expansion checks on full RingCT tx");
            return rct::verRct(tx.rct_signatures, /*semantics=*/false);
        case rct::RCTTypeSimple:
        case rct::RCTTypeBulletproof:
        case rct::RCTTypeBulletproof2:
        case rct::RCTTypeCLSAG:
        case rct::RCTTypeBulletproofPlus:
            VER_ASSERT(check_simple_pre_fcmp_expanded_tx(tx, dereferenced_mix_ring),
                "Failed post-expansion checks on simple, pre-FCMP++ RingCT tx");
            return rct::verRctNonSemanticsSimple(tx.rct_signatures);
        default:
            MERROR("Unrecognized RingCT type: " << tx.rct_signatures.type);
            return false;
        }
    }
    else
    {
        MERROR("Unrecognized transaction version: " << tx.version);
        return false;
    }
}

bool ver_input_proofs_fcmps(transaction& tx, const crypto::ec_point &dereferenced_fcmp_root)
{
    // Hello future Monero dev! If you got this assert, read the following carefully:
    //
    // For this version of RCT, the way we guaranteed that verification caches do not generate false
    // positives (and thus possibly enabling double spends) is we take a hash of two things. One,
    // we use get_transaction_hash() which gives us a (cryptographically secure) unique
    // representation of all "knobs" controlled by the possibly malicious constructor of the
    // transaction. Two, we take a hash of all *previously validated* blockchain data referenced by
    // this transaction which is required to validate the ring signature. In our case, this is the
    // FCMP tree root. Future versions of the protocol may differ in this regard, but if this
    // assumption holds true in the future, enable the verification hash by modifying the
    // `untested_tx` condition below.
    const bool untested_tx = !(tx.version == 2 && tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus);
    VER_ASSERT(!untested_tx,
        "Unknown TX type. Make sure FCMP cache works correctly with this type and then enable it in the code here.");
    VER_ASSERT(!tx.pruned, "Expected unpruned transaction");

    fcmp_pp::TreeRootShared decompressed_root;
    std::vector<fcmp_pp::FcmpPpVerifyInput> fcmp_pp_verify_inputs(1); // @TODO: make non-vector verify() overload
    if (!collect_fcmp_pp_tx_verify_input(tx, {dereferenced_fcmp_root, tx.rct_signatures.p.n_tree_layers},
        decompressed_root, fcmp_pp_verify_inputs.back()))
    {
        return false;
    }

    return fcmp_pp::verify(fcmp_pp_verify_inputs); 
}

crypto::hash make_input_verification_id(const crypto::hash &tx_hash, const rct::ctkeyM &dereferenced_mix_ring)
{
    std::stringstream ss;

    // Start with domain seperation
    ss << config::HASH_KEY_TXHASH_AND_MIXRING;

    // Then add TX hash
    ss.write(tx_hash.data, sizeof(crypto::hash));

    // Then serialize mix ring
    binary_archive<true> ar(ss);
    ::do_serialize(ar, const_cast<rct::ctkeyM&>(dereferenced_mix_ring));

    // Calculate hash of TX hash and mix ring blob
    crypto::hash input_verification_id;
    get_blob_hash(ss.str(), input_verification_id);
    return input_verification_id;
}

crypto::hash make_input_verification_id(const crypto::hash &tx_hash, const crypto::ec_point &dereferenced_fcmp_root)
{
    std::stringstream ss;

    // Start with domain seperation
    ss << config::HASH_KEY_TXHASH_AND_TREE_ROOT;

    // Then add TX hash
    ss.write(tx_hash.data, sizeof(crypto::hash));

    // Then serialize FCMP tree root
    ss.write(dereferenced_fcmp_root.data, sizeof(dereferenced_fcmp_root));

    // Calculate hash of TX hash and FCMP tree root blob
    crypto::hash input_verification_id;
    get_blob_hash(ss.str(), input_verification_id);
    return input_verification_id;
}

crypto::hash make_input_verification_id(const transaction &tx,
    const rct::ctkeyM &dereferenced_mix_ring,
    const crypto::ec_point &dereferenced_fcmp_root)
{
    if (rct::is_rct_fcmp(tx.rct_signatures.type))
        return make_input_verification_id(get_transaction_hash(tx), dereferenced_fcmp_root);
    else
        return make_input_verification_id(get_transaction_hash(tx), dereferenced_mix_ring);
}

bool ver_mixed_rct_semantics(std::vector<const rct::rctSig*> rvv)
{
    size_t batch_rv_size = 0; // this acts as an "end" iterator to the last simple batchable sig ptr
    for (size_t i = 0; i < rvv.size(); ++i)
    {
        const rct::rctSig& rv = *rvv[i];

        bool is_batchable_rv = false;

        switch (rv.type)
        {
        case rct::RCTTypeNull:
            // coinbase should not come here, so we reject for all other types
            MERROR("Unexpected Null rctSig type");
            return false;
            break;
        case rct::RCTTypeSimple:
            if (!rct::verRctSemanticsSimple(rv))
            {
                MERROR("rct signature semantics check failed: type simple");
                return false;
            }
            break;
        case rct::RCTTypeFull:
            if (!rct::verRct(rv, /*semantics=*/true))
            {
                MERROR("rct signature semantics check failed: type full");
                return false;
            }
            break;
        case rct::RCTTypeBulletproof:
        case rct::RCTTypeBulletproof2:
        case rct::RCTTypeCLSAG:
            if (!is_canonical_bulletproof_layout(rv.p.bulletproofs))
            {
                MERROR("Bulletproof does not have canonical form");
                return false;
            }
            is_batchable_rv = true;
            break;
        case rct::RCTTypeBulletproofPlus:
            if (!is_canonical_bulletproof_plus_layout(rv.p.bulletproofs_plus))
            {
                MERROR("Bulletproof_plus does not have canonical form");
                return false;
            }
            is_batchable_rv = true;
            break;
        case rct::RCTTypeFcmpPlusPlus:
            if (!is_canonical_bulletproof_plus_layout(rv.p.bulletproofs_plus) ||
                !is_canonical_fcmp_plus_plus_layout(rv.p.reference_block,
                    rv.p.n_tree_layers,
                    rv.p.pseudoOuts.size(), // number of tx inputs
                    rv.outPk.size(),        // number of tx outputs
                    rv.p.fcmp_pp))
            {
                MERROR("fcmp_plus_plus does not have canonical form");
                return false;
            }
            is_batchable_rv = true;
            break;
        default:
            MERROR("Unknown rct type: " << rv.type);
            return false;
            break;
        }

        // Save this ring sig for later, as we will attempt simple RCT semantics batch verification
        if (is_batchable_rv)
            rvv[batch_rv_size++] = rvv[i];
    }

    if (batch_rv_size) // if any simple, batchable ring sigs...
    {
        rvv.resize(batch_rv_size);
        if (!rct::verRctSemanticsSimple(rvv))
        {
            MERROR("rct signature semantics check failed: simple-style batch verification failed");
            return false;
        }
    }

    return true;
}

bool batch_ver_fcmp_pp_consensus
(
    pool_supplement& ps,
    const std::unordered_map<uint64_t, std::pair<crypto::ec_point, uint8_t>>& tree_root_by_block_index,
    std::unordered_map<crypto::hash, crypto::hash> &valid_input_verification_id_by_txid_out
)
{
    valid_input_verification_id_by_txid_out.clear();
    if (ps.txs_by_txid.empty())
    {
        return true;
    }

    // Collect unverified FCMP++ txs for batch verification
    std::unordered_map<uint64_t, fcmp_pp::TreeRootShared> decompressed_tree_roots_by_block_index;
    std::vector<fcmp_pp::FcmpPpVerifyInput> fcmp_pp_verify_inputs;
    fcmp_pp_verify_inputs.reserve(ps.txs_by_txid.size());

    // Prepare input verification ID's for FCMP++'s we are verifying
    std::unordered_map<crypto::hash, crypto::hash> input_verification_id_by_txid;
    input_verification_id_by_txid.reserve(ps.txs_by_txid.size());

    for (auto &tx_entry : ps.txs_by_txid)
    {
        const crypto::hash &txid = tx_entry.first;
        cryptonote::transaction &tx = tx_entry.second.first;
        if (tx.pruned || tx.version != 2 || tx.rct_signatures.type != rct::RCTTypeFcmpPlusPlus)
        {
            continue;
        }

        MDEBUG("Preparing FCMP++ tx " << txid << " for batch verification " << "(" << tx.vin.size() << " inputs)");

        const uint64_t reference_block = tx.rct_signatures.p.reference_block;
        const bool r = collect_fcmp_pp_tx_verify_input(tx,
            tree_root_by_block_index.at(reference_block),
            decompressed_tree_roots_by_block_index[reference_block],
            fcmp_pp_verify_inputs.emplace_back());

        if (!r)
        {
            return false;
        }

        input_verification_id_by_txid[txid] = make_input_verification_id(txid,
            tree_root_by_block_index.at(reference_block).first);
    }

    if (fcmp_pp_verify_inputs.empty())
    {
        return true;
    }

    // Ok, we're ready to batch verify all FCMP++ txs now
    const std::size_t n_proofs = fcmp_pp_verify_inputs.size();
    MDEBUG("Batch verifying " << n_proofs << " FCMP++ txs");
    if (!rct::batchVerifyFcmpPpProofs(std::move(fcmp_pp_verify_inputs)))
    {
        return false;
    }
    MDEBUG("Successfully batch verified " << n_proofs << " FCMP++ txs");

    // All FCMP++'s have been verified, so set the valid input verification ID's
    valid_input_verification_id_by_txid_out = std::move(input_verification_id_by_txid);

    return true;
}

bool ver_non_input_consensus(const transaction& tx, tx_verification_context& tvc,
    std::uint8_t hf_version)
{
    // Get tx's transparent amount commitments
    std::unordered_map<uint64_t, rct::key> transparent_amount_commitments;
    collect_transparent_amount_commitments({std::cref(tx)}, transparent_amount_commitments);

    return ver_non_input_consensus_templated(&tx, &tx + 1, transparent_amount_commitments, tvc, hf_version);
}

bool ver_non_input_consensus(const pool_supplement& ps,
    const std::unordered_map<uint64_t, rct::key>& transparent_amount_commitments,
    tx_verification_context& tvc,
    const std::uint8_t hf_version)
{
    // We already verified the pool supplement for this hard fork version! Yippee!
    if (ps.nic_verified_hf_version == hf_version)
        return true;

    const auto it_transform = [] (const decltype(ps.txs_by_txid)::value_type& in)
        -> const transaction& { return in.second.first; };
    const auto tx_begin = boost::make_transform_iterator(ps.txs_by_txid.cbegin(), it_transform);
    const auto tx_end = boost::make_transform_iterator(ps.txs_by_txid.cend(), it_transform);

    // Perform the checks...
    const bool verified = ver_non_input_consensus_templated(tx_begin,
        tx_end,
        transparent_amount_commitments,
        tvc,
        hf_version);

    // Cache the hard fork version on success
    if (verified)
        ps.nic_verified_hf_version = hf_version;

    return verified;
}

} // namespace cryptonote
