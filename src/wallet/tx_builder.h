// Copyright (c) 2025, The Monero Project
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

//local headers
#include "carrot_impl/address_device.h"
#include "carrot_impl/input_selection.h"
#include "fee_priority.h"
#include "fcmp_pp/tree_cache.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers

//standard headers
#include <unordered_map>

//forward declarations

namespace tools
{
namespace wallet
{
struct multisig_sig
{
    rct::rctSig sigs;
    std::unordered_set<crypto::public_key> ignore;
    std::unordered_set<rct::key> used_L;
    std::unordered_set<crypto::public_key> signing_keys;
    rct::multisig_out msout;

    rct::keyM total_alpha_G;
    rct::keyM total_alpha_H;
    rct::keyV c_0;
    rct::keyV s;
};

struct PreCarrotTransactionProposal
{
    std::vector<cryptonote::tx_source_entry> sources;
    cryptonote::tx_destination_entry change_dts;
    std::vector<cryptonote::tx_destination_entry> splitted_dsts; // split, includes change
    std::vector<size_t> selected_transfers;
    std::vector<uint8_t> extra;
    uint64_t unlock_time;
    bool use_rct;
    rct::RCTConfig rct_config;
    bool use_view_tags;
    std::vector<cryptonote::tx_destination_entry> dests; // original setup, does not include change
    uint32_t subaddr_account;   // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> subaddr_indices;  // set of address indices used as inputs in this transfer

    enum construction_flags_ : uint8_t
    {
        _use_rct          = 1 << 0, // 00000001
        _use_view_tags    = 1 << 1  // 00000010
        // next flag      = 1 << 2  // 00000100
        // ...
        // final flag     = 1 << 7  // 10000000
    };
    uint8_t construction_flags;
};

/**
 * brief: data to reconstruct any Monero transaction's "signable transaction hash" or "pre-MLSAG hash"
 */
using tx_reconstruct_variant_t = std::variant<
        PreCarrotTransactionProposal,
        carrot::CarrotTransactionProposalV1
    >;
/// destinations for finalized enote (AKA split and w/ change) [requires view-incoming key]
std::vector<cryptonote::tx_destination_entry> finalized_destinations_ref(const tx_reconstruct_variant_t&,
    const carrot::view_incoming_key_device &k_view_dev);
/// change destination [requires view-incoming key]
cryptonote::tx_destination_entry change_destination_ref(const tx_reconstruct_variant_t&,
    const carrot::view_incoming_key_device &k_view_dev);
/// fee
rct::xmr_amount fee_ref(const tx_reconstruct_variant_t&);
/// short payment ID (8 bytes, pre-encryption)
std::optional<crypto::hash8> short_payment_id_ref(const tx_reconstruct_variant_t&);
/// long payment ID (32 bytes)
std::optional<crypto::hash> long_payment_id_ref(const tx_reconstruct_variant_t&);
/// "true-spend" one-time addresses in inputs (in proposal order, not final tx order)
std::vector<crypto::public_key> spent_onetime_addresses_ref(const tx_reconstruct_variant_t&);
/// sum total of input amounts
boost::multiprecision::uint128_t input_amount_total_ref(const tx_reconstruct_variant_t&);
/// ring sizes (in proposal order, not final tx order)
std::vector<std::uint64_t> ring_sizes_ref(const tx_reconstruct_variant_t&);
/// unlock time
std::uint64_t unlock_time_ref(const tx_reconstruct_variant_t&);
/// extra tx fields (includes PIDs and enote ephemeral pubkeys in pre-Carrot ONLY)
const std::vector<std::uint8_t> &extra_ref(const tx_reconstruct_variant_t&);

// The convention for destinations is:
// dests does not include change
// splitted_dsts (in construction_data) does
struct pending_tx
{
    cryptonote::transaction tx;
    uint64_t dust, fee;
    bool dust_added_to_fee;
    cryptonote::tx_destination_entry change_dts;
    std::vector<size_t> selected_transfers;
    std::string key_images;
    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    std::vector<cryptonote::tx_destination_entry> dests;
    std::vector<multisig_sig> multisig_sigs;
    crypto::secret_key multisig_tx_key_entropy;
    uint32_t subaddr_account;            // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> subaddr_indices;  // set of address indices used as inputs in this transfer

    tx_reconstruct_variant_t construction_data;
};

/**
 * brief: index transfers by OTA, including a burning bug filter
 * param: transfers -
 */
std::unordered_map<crypto::public_key, size_t> collect_non_burned_transfers_by_onetime_address(
    const wallet2_basic::transfer_container &transfers);
/**
 * brief: filter and convert wallet2 transfer contain into carrot input candidates
 * param: transfers - wallet2 incoming transfers list
 * param: from_subaddr_account - ignore transfers not addressed to this major account index
 * param: from_subaddr_indices - ignore transfers not addressed to one of these minor account indices, empty means all
 * param: ignore_above - ignore transfers with an amount greater than this limit
 * param: ignore_below - ignore transfers with an amount less than this limit
 * param: top_block_index - block index of current top known block in the chain
 * return: filtered carrot input candidate list derived from transfer container
 */
std::vector<carrot::InputCandidate> collect_carrot_input_candidate_list(
    const wallet2_basic::transfer_container &transfers,
    const std::uint32_t from_subaddr_account,
    const std::set<std::uint32_t> &from_subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index);
/**
 * brief: create "transfer" style Carrot/FCMP++ transaction proposals
 * param: transfers - transfers list to perform input selection from
 * param: subaddress_map -
 * param: dsts - list of (address, amount) payment outlays to fulfill
 * param: fee_per_weight - ratio of pXMR / vB to set fee at
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: subaddr_account - the only account (AKA major) index for which input selection should pull inputs from
 * param: subaddr_indices - if non-empty, the only minor indices for which input selection should pull inputs from
 * param: ignore_above - if the enote's amount is greater than this amount, exclude it from input selection
 * param: ignore_below - if the enote's amount is less than this amount, exclude it from input selection
 * param: subtract_fee_from_outputs - indices into `dsts` which are "fee-subtractable"
 * param: top_block_index - the block index of the current top block inside the blockchain
 * return: list of carrot transaction proposals
 *
 * Transfer-style means that transactions are added until all payment outlays are fulfilled.
 */
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const rct::xmr_amount fee_per_weight,
    std::vector<uint8_t> extra,
    const uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    std::set<std::uint32_t> subtract_fee_from_outputs,
    const std::uint64_t top_block_index);
/**
 * brief: create "sweep-multiple" style Carrot/FCMP++ transaction proposals
 * param: transfers - transfers list to perform input selection from
 * param: subaddress_map -
 * param: input_key_images - key images of inputs to spend
 * param: address - public address for all destinations in txs
 * param: is_subaddress - true iff `address` refers to a subaddress
 * param: n_dests_per_tx - the min num of outputs to make per tx (if `address` isn't ours, a change output is included)
 * param: fee_per_weight - ratio of pXMR / vB to set fee at
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: top_block_index - the block index of the current top block inside the blockchain
 * return: list of carrot transaction proposals
 *
 * Sweep-multiple-style means that transactions are added until all inputs specified by index are spent.
 */
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const rct::xmr_amount fee_per_weight,
    std::vector<uint8_t> extra,
    const std::uint64_t top_block_index);
/**
 * brief: create "sweep-all" style Carrot/FCMP++ transaction proposals
 * param: transfers - transfers list to perform input selection from
 * param: subaddress_map -
 * param: only_below - if the enote's amount is greater than this amount, exclude it from input selection (unless 0)
 * param: address - public address for all destinations in txs
 * param: is_subaddress - true iff `address` refers to a subaddress
 * param: n_dests_per_tx - the min num of outputs to make per tx (if `address` isn't ours, a change output is included)
 * param: fee_per_weight - ratio of pXMR / vB to set fee at
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: subaddr_account - the only account (AKA major) index for which input selection should pull inputs from
 * param: subaddr_indices - if non-empty, the only minor indices for which input selection should pull inputs from
 * param: top_block_index - the block index of the current top block inside the blockchain
 * return: list of carrot transaction proposals
 *
 * Sweep-all-style means that transactions are added until all inputs <= amount `only_below` are spent.
 */
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep_all(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const rct::xmr_amount only_below,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const rct::xmr_amount fee_per_weight,
    std::vector<uint8_t> extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const std::uint64_t top_block_index);
/**
 * brief: convert a `wallet2_basic::transfer_details` into a output opening hint
 * param: td -
 * return: output opening hint
 */
carrot::OutputOpeningHintVariant make_sal_opening_hint_from_transfer_details(const wallet2_basic::transfer_details &td);
/**
 * brief: get index into transfers list of spent enotes in a potential transaction
 * param: tx_construction_data -
 * param: transfers -
 * return: list of spent input enotes indices in construction-specified order, not necessarily final transaction order
 *
 * WARNING: The indices returned assumes the best case scenario for burning bugs for pre-Carrot
 * transactions. If the transfers list contains multiple pre-Carrot enotes with the same onetime
 * address, this function returns the index of the best of the duplicates. If you do not trust the
 * originating source of this construction data, then this is not safe. Validating the amounts
 * actually line up with the best case, instead of trusting, would require tx_reconstruct_variant_t
 * to store openings for the pseudo output amount commitments. This isn't an issue w/ spending
 * Carrot enotes since Carrot mitigates the burning bug statelessly.
 */
std::vector<std::size_t> collect_selected_transfer_indices(const tx_reconstruct_variant_t &tx_construction_data,
    const wallet2_basic::transfer_container &transfers);
/**
 * brief: sign all Carrot transaction proposal inputs given rerandomized outputs
 * param: tx_proposal -
 * param: rerandomized_outputs - rerandomized outputs in order of input proposals in `tx_proposal`
 * param: addr_dev -
 * param: k_spend - k_s
 * return: valid SA/L proofs by key image
 */
std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sign_carrot_transaction_proposal(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const carrot::cryptonote_hierarchy_address_device &addr_dev,
    const crypto::secret_key &k_spend);
/**
 * brief: finalize FCMPs and BP+ range proofs for output amounts for Carrot/FCMP++ txs
 * param: sorted_input_key_images - key images in input order
 * param: sorted_rerandomized_outputs - rerandomized outputs in key image order
 * param: sorted_sal_proofs - SA/L proofs in key image order
 * param: encrypted_payment_id - pid_enc
 * param: fee -
 * param: tree_cache - FCMP tree cache to draw enote paths from
 * param: curve_trees -
 */
cryptonote::transaction finalize_fcmps_and_range_proofs(
    const std::vector<crypto::key_image> &sorted_input_key_images,
    const std::vector<FcmpRerandomizedOutputCompressed> &sorted_rerandomized_outputs,
    const std::vector<fcmp_pp::FcmpPpSalProof> &sorted_sal_proofs,
    const std::vector<carrot::RCTOutputEnoteProposal> &output_enote_proposals,
    const carrot::encrypted_payment_id_t &encrypted_payment_id,
    const rct::xmr_amount fee,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees);
/**
 * brief: finalize FCMPs, BP+ range proofs for outputs amounts, and SA/L proofs for Carrot/FCMP++ txs
 * param: tx_proposal -
 * param: tree_cache - FCMP tree cache to draw enote paths from
 * param: curve_trees -
 * param: acc_keys -
 * return: a fully proved FCMP++ transaction corresponding to the transaction proposal
 */
cryptonote::transaction finalize_all_fcmp_pp_proofs(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys);
/**
 * brief: fill out a `pending_tx` from a Carrot transaction proposal, excluding the transaction itself
 * param: tx_proposal -
 * param: sorted_input_key_images - key images in input order
 * param: k_view_incoming_dev - k_v
 * return: `pending_tx` representing the Carrot transaction proposal 
 */
pending_tx make_pending_carrot_tx(const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<crypto::key_image> &sorted_input_key_images,
    const carrot::view_incoming_key_device &k_view_incoming_dev);
/**
 * brief: finalize FCMPs, BP+ range proofs, and SA/Ls, proofs for Carrot/FCMP++ txs into a `pending_tx`
 * param: tx_proposal -
 * param: tree_cache - FCMP tree cache to draw enote paths from
 * param: curve_trees -
 * param: acc_keys -
 * return: `pending_tx` representing the Carrot transaction proposal, including a fully proved FCMP++ transaction
 */
pending_tx finalize_all_fcmp_pp_proofs_as_pending_tx(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys);

} //namespace wallet
} //namespace tools
