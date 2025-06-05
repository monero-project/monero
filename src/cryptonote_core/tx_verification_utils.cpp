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
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain"

#define VER_ASSERT(cond, msgexpr) CHECK_AND_ASSERT_MES(cond, false, msgexpr)

using namespace cryptonote;

// Sanity checks on expanded pre-FCMP tx
static bool check_pre_fcmp_expanded_tx(const transaction& tx, const rct::ctkeyM& mix_ring)
{
    const rct::rctSig& rv = tx.rct_signatures;
    VER_ASSERT(!rct::is_rct_fcmp(rv.type), "Unexpected RCT type in pre-FCMP tx expansion");

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

// Sanity checks on expanded post-FCMP tx
static bool check_post_fcmp_expanded_tx(const transaction& tx)
{
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
    VER_ASSERT(n_inputs == tx.vin.size(), "Mismatched pseudo outs to inputs after expanding FCMP tx");
    VER_ASSERT(n_inputs == rv.p.fcmp_ver_helper_data.key_images.size(), "Mismatched key images to inputs after expanding FCMP tx");

    // For each input, check that the key images were copied into the expanded RCT sig correctly
    for (size_t n = 0; n < n_inputs; ++n)
    {
        const crypto::key_image& nth_vin_image = boost::get<txin_to_key>(tx.vin[n]).k_image;
        const bool ki_match = 0 == memcmp(&nth_vin_image, &rv.p.fcmp_ver_helper_data.key_images[n], 32);
        VER_ASSERT(ki_match, "Failed to check ringct signatures: mismatched FCMP key image");
    }

    return true;
}

// Do pre FCMP++ RCT expansion, then do post-expansion sanity checks.
static bool expand_pre_fcmp_tx(transaction& tx, const crypto::hash& tx_prefix_hash, const rct::ctkeyM& mix_ring)
{
    // Expand mixRing, tx inputs, tx key images, prefix hash message, etc into the RCT sig
    const bool exp_res = Blockchain::expand_transaction_2(tx, tx_prefix_hash, mix_ring, nullptr/*tree_root*/);
    VER_ASSERT(exp_res, "Failed to expand rct signatures!");

    // Do sanity checks after expansion
    return check_pre_fcmp_expanded_tx(tx, mix_ring);
}

// Do post FCMP++ RCT expansion, then do post-expansion sanity checks.
static bool expand_post_fcmp_tx(transaction& tx, const crypto::hash& tx_prefix_hash, const crypto::ec_point& tree_root)
{
    // Expand the tree root
    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    const auto root = curve_trees->get_tree_root_from_bytes(tx.rct_signatures.p.n_tree_layers, tree_root);
    VER_ASSERT(root != nullptr, "Failed to decompress root");

    // Expand tree_root, tx inputs, tx key images, prefix hash message, etc into the RCT sig
    const bool exp_res = Blockchain::expand_transaction_2(tx, tx_prefix_hash, {}/*mixRing*/, root);
    VER_ASSERT(exp_res, "Failed to expand rct signatures!");

    // Do sanity checks after expansion
    return check_post_fcmp_expanded_tx(tx);
}

static bool expand_fcmp_pp_tx(cryptonote::transaction& tx, const fcmp_pp::TreeRootShared &tree_root)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass verification");
    const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);

    // Expand tree_root, tx inputs, tx key images, prefix hash message, etc into the RCT sig
    const bool exp_res = Blockchain::expand_transaction_2(tx, tx_prefix_hash, {}/*mixRing*/, tree_root);
    VER_ASSERT(exp_res, "Failed to expand rct signatures!");

    // Do sanity checks after expansion
    return check_post_fcmp_expanded_tx(tx);
}

// Create a unique identifier for pair of tx blob + mix ring
static crypto::hash calc_tx_mixring_hash(const transaction& tx, const rct::ctkeyM& mix_ring)
{
    std::stringstream ss;

    // Start with domain seperation
    ss << config::HASH_KEY_TXHASH_AND_MIXRING;

    // Then add TX hash
    const crypto::hash tx_hash = get_transaction_hash(tx);
    ss.write(tx_hash.data, sizeof(crypto::hash));

    // Then serialize mix ring
    binary_archive<true> ar(ss);
    ::do_serialize(ar, const_cast<rct::ctkeyM&>(mix_ring));

    // Calculate hash of TX hash and mix ring blob
    crypto::hash tx_and_mixring_hash;
    get_blob_hash(ss.str(), tx_and_mixring_hash);

    return tx_and_mixring_hash;
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
        if (cryptonote::tx_outs_checked_for_torsion(tx)
            && !collect_pubkeys_and_commitments(tx, transparent_amount_commitments, pubkeys_and_commitments))
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

// Create a unique identifier for pair of tx blob + tree root
static crypto::hash calc_tx_tree_root_hash(const transaction& tx, const crypto::ec_point& tree_root)
{
    std::stringstream ss;

    // Start with domain seperation
    ss << config::HASH_KEY_TXHASH_AND_TREE_ROOT;

    // Then add TX hash
    const crypto::hash tx_hash = get_transaction_hash(tx);
    ss.write(tx_hash.data, sizeof(crypto::hash));

    // Then serialize tree root
    binary_archive<true> ar(ss);
    ::do_serialize(ar, const_cast<crypto::ec_point&>(tree_root));

    // Calculate hash of TX hash and tree root
    crypto::hash tx_and_tree_root_hash;
    get_blob_hash(ss.str(), tx_and_tree_root_hash);

    return tx_and_tree_root_hash;
}

// Expand the RCT tx then do post-expansion semantics AND non-semantics verification.
static bool expand_tx_and_ver_rct_non_sem(cryptonote::transaction& tx_inout,
    const rct::ctkeyM& mix_ring,
    const crypto::ec_point& tree_root)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx_inout.pruned, "Pruned transaction will not pass verRctNonSemanticsSimple");
    const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx_inout);

    const bool expanded = rct::is_rct_fcmp(tx_inout.rct_signatures.type)
        ? expand_post_fcmp_tx(tx_inout, tx_prefix_hash, tree_root)
        : expand_pre_fcmp_tx(tx_inout, tx_prefix_hash, mix_ring);
    VER_ASSERT(expanded, "Failed to expand RCT tx");

    return rct::verRctNonSemanticsSimple(tx_inout.rct_signatures);
}

// Create a unique identifer for a tx and its referenced anon set
static crypto::hash calc_tx_anon_set_hash(const cryptonote::transaction& tx,
    const rct::ctkeyM& mix_ring,
    const crypto::ec_point& tree_root)
{
    return rct::is_rct_fcmp(tx.rct_signatures.type)
        ? calc_tx_tree_root_hash(tx, tree_root)
        : calc_tx_mixring_hash(tx, mix_ring);
}

static bool collect_fcmp_pp_tx_verify_inputs(cryptonote::transaction &tx,
    rct_ver_cache_t& cache,
    const std::unordered_map<uint64_t, std::pair<crypto::ec_point, uint8_t>>& tree_root_by_block_index,
    std::unordered_map<uint64_t, fcmp_pp::TreeRootShared> &decompressed_tree_roots_by_block_index,
    std::vector<fcmp_pp::FcmpPpVerifyInput> &fcmp_pp_verify_inputs,
    std::vector<crypto::hash> &new_cache_hashes)
{
    VER_ASSERT(tx.rct_signatures.type == rct::RCTTypeFcmpPlusPlus, "expected FCMP++ RCT Type");
    VER_ASSERT(!tx.pruned, "expected unpruned FCMP++ tx");

    // Make sure tree metadata is correct
    const uint64_t ref_block_index = tx.rct_signatures.p.reference_block;
    const uint8_t n_tree_layers = tx.rct_signatures.p.n_tree_layers;

    VER_ASSERT(ref_block_index > 0, "tx reference_block must be > 0");
    VER_ASSERT(n_tree_layers > 0, "tx n_tree_layers must be > 0");

    const auto tree_root_it = tree_root_by_block_index.find(tx.rct_signatures.p.reference_block);

    VER_ASSERT(tree_root_it != tree_root_by_block_index.end(), "Did not find tree root by ref block");
    VER_ASSERT(n_tree_layers == tree_root_it->second.second, "Unexpected tx n tree layers");

    const crypto::ec_point &tree_root = tree_root_it->second.first;

    // Generate unique hash for tx+anon set identifier
    const crypto::hash cache_hash = calc_tx_anon_set_hash(tx, {}, tree_root);

    // Search cache for successful verification of same TX + mix set hash combination
    if (cache.has(cache_hash))
    {
        MDEBUG("RCT cache: FCMP++ tx " << get_transaction_hash(tx) << " hit");
        return true;
    }

    // We had a cache miss, so now we prepare the FCMP++ tx for batch verification
    MDEBUG("RCT cache: FCMP++ tx " << get_transaction_hash(tx) << " missed");

    // De-compress the tree root
    const auto tree_root_decompressed_it = decompressed_tree_roots_by_block_index.find(ref_block_index);
    if (tree_root_decompressed_it == decompressed_tree_roots_by_block_index.end())
    {
        const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
        auto decompressed_root = curve_trees->get_tree_root_from_bytes(n_tree_layers, tree_root);
        VER_ASSERT(decompressed_root != nullptr, "Failed to decompress root");

        decompressed_tree_roots_by_block_index[ref_block_index] = std::move(decompressed_root);
    }

    // Get decompressed tree root from map
    const auto &decompressed_tree_root = decompressed_tree_roots_by_block_index[ref_block_index];

    if (!expand_fcmp_pp_tx(tx, decompressed_tree_root))
    {
        MERROR("Failed to expand FCMP++ tx");
        return false;
    }

    // Now instantiate the FCMP++ verify input
    const auto &rv = tx.rct_signatures;
    const rct::key signable_tx_hash = rct::get_pre_mlsag_hash(rv, hw::get_device("default"));

    // Type conversion on pseudo outs
    const auto &pseudoOuts = rv.p.pseudoOuts;
    std::vector<crypto::ec_point> pseudo_outs;
    pseudo_outs.reserve(pseudoOuts.size());
    for (const auto &po : pseudoOuts)
        pseudo_outs.emplace_back(rct::rct2pt(po));

    auto fcmp_pp_verify_input = fcmp_pp::fcmp_pp_verify_input_new(
            rct::rct2hash(signable_tx_hash),
            rv.p.fcmp_pp,
            n_tree_layers,
            rv.p.fcmp_ver_helper_data.tree_root,
            pseudo_outs,
            rv.p.fcmp_ver_helper_data.key_images
        );

    fcmp_pp_verify_inputs.emplace_back(std::move(fcmp_pp_verify_input));
    new_cache_hashes.emplace_back(cache_hash);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace cryptonote
{

bool collect_pubkeys_and_commitments(const transaction& tx,
    const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments,
    std::vector<rct::key> &pubkeys_and_commitments_inout)
{
    for (std::size_t i = 0; i < tx.vout.size(); ++i)
    {
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
    // from v8, limit a tx to 50% of the minimum block weight
    if (hf_version >= HF_VERSION_PER_BYTE_FEE)
        return get_min_block_weight(hf_version) / 2 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    else
        return get_min_block_weight(hf_version) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
}

bool are_transaction_output_pubkeys_sorted(const transaction_prefix &tx_prefix)
{
    crypto::public_key last_output_pubkey = crypto::null_pkey;
    for (const tx_out &o : tx_prefix.vout) {
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

bool ver_rct_non_semantics_simple_cached
(
    transaction& tx,
    const rct::ctkeyM& mix_ring,
    const crypto::ec_point& tree_root,
    rct_ver_cache_t& cache,
    const std::uint8_t rct_type_to_cache
)
{
    // Hello future Monero dev! If you got this assert, read the following carefully:
    //
    // For this version of RCT, the way we guaranteed that verification caches do not generate false
    // positives (and thus possibly enabling double spends) is we take a hash of two things. One,
    // we use get_transaction_hash() which gives us a (cryptographically secure) unique
    // representation of all "knobs" controlled by the possibly malicious constructor of the
    // transaction. Two, we take a hash of all *previously validated* blockchain data referenced by
    // this transaction which is required to validate the membership proof. In our case, this is
    // either the mixring (from the ring signature era) or the tree root (from the FCMP era).
    // Future versions of the protocol may differ in this regard, but if this assumptions
    // holds true in the future, enable the verification hash by modifying the `untested_tx`
    // condition below.
    const bool untested_tx = tx.version > 2 || tx.rct_signatures.type > rct::RCTTypeFcmpPlusPlus;
    VER_ASSERT(!untested_tx, "Unknown TX type. Make sure RCT cache works correctly with this type and then enable it in the code here.");

    // Don't cache older (or newer) rctSig types
    // This cache only makes sense when it caches data from mempool first,
    // so only "current fork version-enabled" RCT types need to be cached
    if (tx.rct_signatures.type != rct_type_to_cache)
    {
        MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " skipped");
        return expand_tx_and_ver_rct_non_sem(tx, mix_ring, tree_root);
    }

    // Generate unique hash for tx+anon set identifier
    const crypto::hash cache_hash = calc_tx_anon_set_hash(tx, mix_ring, tree_root);

    // Search cache for successful verification of same TX + mix set hash combination
    if (cache.has(cache_hash))
    {
        MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " hit");
        return true;
    }

    // We had a cache miss, so now we must expand the mix ring and do full verification
    MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " missed");
    if (!expand_tx_and_ver_rct_non_sem(tx, mix_ring, tree_root))
    {
        return false;
    }

    // At this point, the TX RCT verified successfully, so add it to the cache and return true
    cache.add(cache_hash);

    return true;
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
    rct_ver_cache_t& cache,
    const std::uint8_t rct_type_to_cache
)
{
    if (ps.txs_by_txid.empty())
    {
        return true;
    }

    // We expect to be caching FCMP++ txs. If not, then we need to re-work
    // verification to batch verify FCMP++ txs AND whatever future txs we're
    // trying to batch verify.
    const bool caching_fcmp_pp_txs = rct_type_to_cache == rct::RCTTypeFcmpPlusPlus;
    VER_ASSERT(caching_fcmp_pp_txs, "Make sure batch verification works correctly with this type and then enable it in the code here.");

    // Collect unverified FCMP++ txs for batch verfication
    std::unordered_map<uint64_t, fcmp_pp::TreeRootShared> decompressed_tree_roots_by_block_index;
    std::vector<fcmp_pp::FcmpPpVerifyInput> fcmp_pp_verify_inputs;
    std::vector<crypto::hash> new_cache_hashes;

    fcmp_pp_verify_inputs.reserve(ps.txs_by_txid.size());
    new_cache_hashes.reserve(ps.txs_by_txid.size());

    for (auto &tx_entry : ps.txs_by_txid)
    {
        cryptonote::transaction &tx = tx_entry.second.first;
        if (tx.pruned || tx.rct_signatures.type != rct_type_to_cache)
        {
            MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " skipped");
            continue;
        }

        const bool r = collect_fcmp_pp_tx_verify_inputs(tx,
            cache,
            tree_root_by_block_index,
            decompressed_tree_roots_by_block_index,
            fcmp_pp_verify_inputs,
            new_cache_hashes);

        if (!r)
        {
            return false;
        }
    }

    if (fcmp_pp_verify_inputs.empty())
    {
        return true;
    }

    // Ok, we're ready to batch verify all FCMP++ txs now
    MDEBUG("Batch verifying " << fcmp_pp_verify_inputs.size() << " FCMP++ txs");
    if (!fcmp_pp::verify(fcmp_pp_verify_inputs))
    {
        return false;
    }
    MDEBUG("Successfully batch verified " << fcmp_pp_verify_inputs.size() << " FCMP++ txs");

    // At this point, the FCMP++ txs all verified successfully. Add them to the cache
    for (const auto &cache_hash : new_cache_hashes)
    {
        cache.add(cache_hash);
    }

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
