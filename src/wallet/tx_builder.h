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
#include "carrot_impl/carrot_tx_builder_inputs.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "wallet2.h"

//third party headers

//standard headers
#include <unordered_map>

//forward declarations

namespace tools
{
namespace wallet
{
std::unordered_map<crypto::key_image, size_t> collect_non_burned_transfers_by_key_image(
    const wallet2::transfer_container &transfers);

carrot::select_inputs_func_t make_wallet2_single_transfer_input_selector(
    const wallet2::transfer_container &transfers,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> &from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index,
    const bool allow_carrot_external_inputs_in_normal_transfers,
    const bool allow_pre_carrot_inputs_in_normal_transfers,
    std::set<size_t> &selected_transfer_indices_out);

std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    const wallet2::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    std::vector<cryptonote::tx_destination_entry> dsts,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    wallet2::unique_index_container subtract_fee_from_outputs,
    const std::uint64_t top_block_index,
    const cryptonote::account_keys &acc_keys);
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    wallet2 &w,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const wallet2::unique_index_container &subtract_fee_from_outputs);

carrot::CarrotTransactionProposalV1 make_carrot_transaction_proposal_wallet2_sweep(
    const wallet2::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint64_t top_block_index,
    const cryptonote::account_keys &acc_keys);
carrot::CarrotTransactionProposalV1 make_carrot_transaction_proposal_wallet2_sweep(
    wallet2 &w,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra);

carrot::OutputOpeningHintVariant make_sal_opening_hint_from_transfer_details(
    const wallet2::transfer_details &td,
    const crypto::secret_key &k_view,
    hw::device &hwdev);

std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sign_carrot_transaction_proposal_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const wallet2::transfer_container &transfers,
    const cryptonote::account_keys &acc_keys);

//! @TODO: accept already-calculated rerandomized outputs and their blinds
cryptonote::transaction finalize_all_proofs_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2::transfer_container &transfers,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys);
} //namespace wallet
} //namespace tools
