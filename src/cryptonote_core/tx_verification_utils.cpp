// Copyright (c) 2023, The Monero Project
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

#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "hardforks/hardforks.h"
#include "ringct/rctSigs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain"

#define VER_ASSERT(cond, msgexpr) CHECK_AND_ASSERT_MES(cond, false, msgexpr)

using namespace cryptonote;

// Do RCT expansion, then do post-expansion sanity checks, then do full non-semantics verification.
static bool expand_tx_and_ver_rct_non_sem(transaction& tx, const rct::ctkeyM& mix_ring)
{
    // Pruned transactions can not be expanded and verified because they are missing RCT data
    VER_ASSERT(!tx.pruned, "Pruned transaction will not pass verRctNonSemanticsSimple");

    // Calculate prefix hash
    const crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);

    // Expand mixring, tx inputs, tx key images, prefix hash message, etc into the RCT sig
    const bool exp_res = Blockchain::expand_transaction_2(tx, tx_prefix_hash, mix_ring);
    VER_ASSERT(exp_res, "Failed to expand rct signatures!");

    const rct::rctSig& rv = tx.rct_signatures;

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
    return rct::verRctNonSemanticsSimple(rv);
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

template <class TxForwardIt>
static bool ver_non_input_consensus_templated(TxForwardIt tx_begin, TxForwardIt tx_end,
        tx_verification_context& tvc, std::uint8_t hf_version)
{
    std::vector<const rct::rctSig*> rvv;
    rvv.reserve(static_cast<size_t>(std::distance(tx_begin, tx_end)));

    const size_t max_tx_version = hf_version < HF_VERSION_DYNAMIC_FEE ? 1 : 2;

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

        // Rule 2 & 3
        if (tx.version == 0 || tx.version > max_tx_version)
        {
            tvc.m_verifivation_failed = true;
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
    }

    // Rule 7
    if (!ver_mixed_rct_semantics(std::move(rvv)))
    {
        tvc.m_verifivation_failed = true;
        tvc.m_invalid_input = true;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace cryptonote
{

uint64_t get_transaction_weight_limit(const uint8_t hf_version)
{
    // from v8, limit a tx to 50% of the minimum block weight
    if (hf_version >= HF_VERSION_PER_BYTE_FEE)
        return get_min_block_weight(hf_version) / 2 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    else
        return get_min_block_weight(hf_version) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
}

bool ver_rct_non_semantics_simple_cached
(
    transaction& tx,
    const rct::ctkeyM& mix_ring,
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
    // this transaction which is required to validate the ring signature. In our case, this is the
    // mixring. Future versions of the protocol may differ in this regard, but if this assumptions
    // holds true in the future, enable the verification hash by modifying the `untested_tx`
    // condition below.
    const bool untested_tx = tx.version > 2 || tx.rct_signatures.type > rct::RCTTypeBulletproofPlus;
    VER_ASSERT(!untested_tx, "Unknown TX type. Make sure RCT cache works correctly with this type and then enable it in the code here.");

    // Don't cache older (or newer) rctSig types
    // This cache only makes sense when it caches data from mempool first,
    // so only "current fork version-enabled" RCT types need to be cached
    if (tx.rct_signatures.type != rct_type_to_cache)
    {
        MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " skipped");
        return expand_tx_and_ver_rct_non_sem(tx, mix_ring);
    }

    // Generate unique hash for tx+mix_ring pair
    const crypto::hash tx_mixring_hash = calc_tx_mixring_hash(tx, mix_ring);

    // Search cache for successful verification of same TX + mix ring combination
    if (cache.has(tx_mixring_hash))
    {
        MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " hit");
        return true;
    }

    // We had a cache miss, so now we must expand the mix ring and do full verification
    MDEBUG("RCT cache: tx " << get_transaction_hash(tx) << " missed");
    if (!expand_tx_and_ver_rct_non_sem(tx, mix_ring))
    {
        return false;
    }

    // At this point, the TX RCT verified successfully, so add it to the cache and return true
    cache.add(tx_mixring_hash);

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

bool ver_non_input_consensus(const transaction& tx, tx_verification_context& tvc,
    std::uint8_t hf_version)
{
    return ver_non_input_consensus_templated(&tx, &tx + 1, tvc, hf_version);
}

bool ver_non_input_consensus(const pool_supplement& ps, tx_verification_context& tvc,
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
    const bool verified = ver_non_input_consensus_templated(tx_begin, tx_end, tvc, hf_version);

    // Cache the hard fork version on success
    if (verified)
        ps.nic_verified_hf_version = hf_version;

    return verified;
}

} // namespace cryptonote
