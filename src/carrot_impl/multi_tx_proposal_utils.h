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
#include "input_selection.h"

//third party headers

//standard headers
#include <set>
#include <vector>

//forward declarations

namespace carrot
{
/**
 * brief: make_multiple_carrot_transaction_proposals_transfer - make multiple "transfer-style" Carrot tx proposals
 * param: normal_payment_proposals - normal payment proposals to be included *once* in the tx set
 * param: selfsend_payment_proposals - selfsend payment proposals to be included *once* in the tx set
 * param: fee_per_weight - concrete fee is calculated as transaction weight times this value
 * param: extra - truly "extra" fields to be included in *every* tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: input_candidates - list of potential input candidates to choose from
 * param: input_selection_policies - span of ISPs (see `make_single_transfer_input_selector` for more info)
 * param: input_selection_flags - flags passed to `make_single_transfer_input_selector`
 * param: change_address_spend_pubkey - address spend pubkey to send to for change selfsend enotes
 * param: change_address_index - subaddress index of change_address_spend_pubkey in your account
 * param: subtractable_normal_payment_proposals - indices of normal payment proposals which are "fee subtractable"
 * param: subtractable_selfsend_payment_proposals - indices of selfsend payment proposals which are "fee subtractable"
 * outparam: tx_proposals_out - set of fully formed Carrot transaction proposal which satisfies all payments
 *
 * Creates as many transactions as is necessary to fulfill all of the passed payment proposals.
 * Internally, this function uses a greedy loop, selecting inputs with
 * `make_single_transfer_input_selector` for input selection for a single transaction, marking them
 * as used, and then moves onto the next single transaction in isolation. This is technically not
 * optimal, but optimality in this case our problem is NP-complete, which means that achieving
 * optimality requires a non-polynomial runtime. Also, using `make_single_transfer_input_selector`
 * for pulling single transactions at a time means that it's easier to verify the input selection
 * rules are being upheld for each individual transaction in the set.
 */
void make_multiple_carrot_transaction_proposals_transfer(
    std::vector<CarrotPaymentProposalV1> &&normal_payment_proposals,
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &&selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<InputCandidate> &&input_candidates,
    const epee::span<const input_selection_policy_t> input_selection_policies,
    const std::uint32_t input_selection_flags,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    const std::set<std::size_t> &subtractable_normal_payment_proposals,
    const std::set<std::size_t> &subtractable_selfsend_payment_proposals,
    std::vector<CarrotTransactionProposalV1> &tx_proposals_out);
/**
 * brief: make_multiple_carrot_transaction_proposals_sweep - make multiple "sweep-style" Carrot transaction proposals
 * param: normal_payment_proposals - normal payment proposals to be included in *every* tx in the set
 * param: selfsend_payment_proposals - selfsend payment proposals to be included in *every* tx in the set
 * param: fee_per_weight - concrete fee is calculated as transaction weight times this value
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: selected_inputs - explicitly provided inputs
 * param: change_address_spend_pubkey - address spend pubkey to send to for change selfsend enotes
 * param: change_address_index - subaddress index of change_address_spend_pubkey in your account
 * outparam: tx_proposal_out - set of fully formed Carrot transaction proposal which spend all inputs
 *
 * Creates as many sweep transaction as is necessary to spend all of `selected_inputs`, including
 * the provided payment proposals in every transaction.
 */
void make_multiple_carrot_transaction_proposals_sweep(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<CarrotSelectedInput> &&selected_inputs,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    const bool ignore_dust,
    std::vector<CarrotTransactionProposalV1> &tx_proposals_out);
} //namespace carrot
