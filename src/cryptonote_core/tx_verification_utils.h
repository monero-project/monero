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

#pragma once

#include <unordered_map>

#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/verification_context.h"

namespace cryptonote
{
/**
 * @brief Add the tx's output pub keys and commitments to the collection
 *
 * @param tx
 * @param transparent_amount_commitments pre-calculated transparent amount commitments
 * @param pubkeys_and_commitments_inout the collection by ref
 * @return successful insert
 */
bool collect_points_for_torsion_check(const transaction& tx,
    const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments,
    std::vector<rct::key> &pubkeys_and_commitments_inout);

void collect_transparent_amount_commitments(
    const std::vector<std::reference_wrapper<const transaction>> &txs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout);

std::vector<std::reference_wrapper<const transaction>> collect_transparent_amount_commitments(
    const transaction &miner_tx,
    const std::vector<std::pair<transaction, blobdata>> &tx_pairs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout);

std::vector<std::reference_wrapper<const transaction>> collect_transparent_amount_commitments(
    const transaction &miner_tx,
    const std::vector<transaction> &txs,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout);

void collect_transparent_amount_commitments(
    const std::unordered_map<crypto::hash, std::pair<transaction, blobdata>> &txs_by_txid,
    std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments_inout);

/**
 * @brief Get the maximum transaction weight for a given hardfork
 *
 * @param hf_version hard fork version
 * @return the maximum unconditional transaction weight
 */
uint64_t get_transaction_weight_limit(uint8_t hf_version);

/**
 * @brief Check whether transaction's output pubkeys are sorted in strictly increasing lexicographical order
 */
bool are_transaction_output_pubkeys_sorted(const std::vector<tx_out> &vout);

/**
 * @brief Check whether transaction's output pubkeys are sorted, iff required by fork rule
 * @param tx_prefix
 * @param hf_version hard fork version
 * @see are_transaction_output_pubkeys_sorted()
 *
 * Output pubkeys must be sorted after FCMP++ grace period or if a Carrot tx during FCMP++ grace period.
 */
bool check_transaction_output_pubkeys_order(const transaction_prefix &tx_prefix, std::uint8_t hf_version);

/**
 * @brief Get the minimum allowed transaction version
 *
 * An "unmixable" ring is a ring appearing in block index BI spending a pre-RingCT enote
 * (i.e. referencible amount != 0) where the minimum required ring size is greater than the total
 * number of pre-RingCT enotes on-chain at block indices < BI with that same amount.
 *
 * @param hf_version hard fork version
 * @param has_unmixable_ring true iff at least one of the rings in the transaction is "unmixable"
 * @return the minimum allowed transaction version
 */
size_t get_minimum_transaction_version(uint8_t hf_version, bool has_unmixable_ring);

/**
 * @brief Get the maximum allowed transaction version
 *
 * @param hf_version hard fork version
 * @return the maximum allowed transaction version
 */
size_t get_maximum_transaction_version(uint8_t hf_version);

/**
 * @brief Tx-safe version of crypto::check_ring_signature() / rct::verRct(_, false) / rct::verRctNonSemanticsSimple()
 *
 * This function will not affect how the transaction is serialized and it will never modify the
 * transaction prefix.
 *
 * The reference to tx is mutable since the transaction's ring signatures will be expanded by
 * Blockchain::expand_transaction_2. This means that the caller does not need to call
 * expand_transaction_2 on this transaction before passing it; the transaction will not successfully
 * verify with "old" mixring / misc RCT data if the transaction has been otherwise modified since
 * the last verification.
 *
 * @param tx transaction which contains RCT signature to verify
 * @param dereferenced_mixring mixring referenced by this tx. THIS DATA MUST BE PREVIOUSLY VALIDATED
 * @return true when verRctNonSemanticsSimple() w/ expanded tx.rct_signatures would return true
 * @return false when verRctNonSemanticsSimple() w/ expanded tx.rct_signatures would return false
 */
bool ver_input_proofs_rings(transaction& tx, const rct::ctkeyM &dereferenced_mix_ring);

/**
 * @brief Tx-safe version of rct::verRctNonSemanticsSimple() for FCMP++ txs
 *
 * This function will not affect how the transaction is serialized and it will never modify the
 * transaction prefix.
 *
 * The reference to tx is mutable since the transaction's ring signatures will be expanded by
 * Blockchain::expand_transaction_2. This means that the caller does not need to call
 * expand_transaction_2 on this transaction before passing it; the transaction will not successfully
 * verify with "old" FCMP tree / misc RCT data if the transaction has been otherwise modified since
 * the last verification.
 *
 * @param tx transaction which contains RCT signature to verify
 * @param dereferenced_fcmp_root FCMP tree root referenced by this tx. THIS DATA MUST BE PREVIOUSLY VALIDATED
 * @return true when verRctNonSemanticsSimple() w/ expanded tx.rct_signatures would return true
 * @return false when verRctNonSemanticsSimple() w/ expanded tx.rct_signatures would return false
 */
bool ver_input_proofs_fcmps(transaction& tx, const crypto::ec_point &dereferenced_fcmp_root);

/**
 * @brief Make an ID for the parameters to ver_input_proofs_rings() for a transaction and its dereferenced chain data
 *
 * For any two calls to ver_input_proofs_rings() in any order, if the input verification ID for the
 * (transaction, mixring) pair match, the result of ver_input_proofs_rings() will also match.
 *
 * If this transaction hash is "stale", this function is not guaranteed to work. So make sure that
 * the cryptonote::transaction passed has not had modifications to it since the last time its hash
 * was fetched without properly invalidating the hashes.
 */
crypto::hash make_input_verification_id(const crypto::hash &tx_hash, const rct::ctkeyM &dereferenced_mix_ring);
crypto::hash make_input_verification_id(const crypto::hash &tx_hash, const crypto::ec_point &dereferenced_fcmp_root);
crypto::hash make_input_verification_id(const transaction &tx,
    const rct::ctkeyM &dereferenced_mix_ring,
    const crypto::ec_point &dereferenced_fcmp_root);

/**
 * @brief Verify the semantics of a group of RingCT signatures as a batch (if applicable)
 *
 * Coinbase txs or other transaction with a RingCT type of RCTTypeNull will fail to verify.
 *
 * @param rvv list of signatures to verify
 * @return true if all signatures verified semantics successfully, false otherwise
 */
bool ver_mixed_rct_semantics(std::vector<const rct::rctSig*> rvv);

/**
 * @brief Used to provide transaction info that skips the mempool to block handling code 
 */
struct pool_supplement
{
    // Map of supplemental tx info that we might need to validate a block
    // Maps TXID -> transaction and blob
    std::unordered_map<crypto::hash, std::pair<transaction, blobdata>> txs_by_txid;
    // If non-zero, then consider all the txs' non-input consensus (NIC) rules verified for this
    // hard fork. User: If you add an unverified transaction to txs_by_txid, set this field to zero!
    mutable std::uint8_t nic_verified_hf_version = 0;
};

// TODO: document the function
bool batch_ver_fcmp_pp_consensus
(
    pool_supplement& ps,
    const std::unordered_map<uint64_t, std::pair<crypto::ec_point, uint8_t>>& tree_root_by_block_index,
    std::unordered_map<crypto::hash, crypto::hash> &valid_input_verification_id_by_txid_out
);

/**
 * @brief Verify every non-input consensus rule for a group of non-coinbase transactions
 *
 * List of checks that we do for each transaction:
 *     1. Check tx blob size < get_max_tx_size()
 *     2. Check tx version >= get_minimum_transaction_version()
 *     3. Check tx version <= get_maximum_transaction_version()
 *     4. Check tx weight < get_transaction_weight_limit()
 *     5. Passes core::check_tx_semantic()
 *     6. Passes Blockchain::check_tx_outputs()
 *     7. Passes ver_mixed_rct_semantics() [Uses batch RingCT verification when applicable]
 *     8. Check unlock time is 0 from hardfork v17
 *     9. Check extra size <= MAX_TX_EXTRA_SIZE from hardfork v17
 *     10. Passes check_transaction_output_pubkeys_order()
 *
 * For pool_supplement input:
 * We assume the structure of the pool supplement is already correct: for each value entry, the
 * cryptonote::transaction matches its corresponding blobdata and the TXID map key is correctly
 * calculated for that transaction. We use the .nic_verified_hf_version field to skip verification
 * for the pool supplement if hf_version matches, and we cache that version on success.
 *
 * @param tx single transaction to verify
 * @param pool_supplement pool supplement to verify
 * @param tvc relevant flags will be set for if/why verification failed
 * @param hf_version Hard fork version to run rules against
 * @return true if all relevant transactions verify, false otherwise
 */
bool ver_non_input_consensus(const transaction& tx, tx_verification_context& tvc,
    std::uint8_t hf_version);

bool ver_non_input_consensus(const pool_supplement& ps,
    const std::unordered_map<uint64_t, rct::key>& transparent_amount_commitments,
    tx_verification_context& tvc,
    std::uint8_t hf_version);

} // namespace cryptonote
