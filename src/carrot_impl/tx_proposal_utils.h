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

//! @file Utilities for creating Carrot transaction proposals

#pragma once

//local headers
#include "tx_proposal.h"

//third party headers
#include <boost/multiprecision/cpp_int.hpp>

//standard headers
#include <cstddef>

//forward declarations

namespace carrot
{
struct CarrotSelectedInput
{
    rct::xmr_amount amount;
    InputProposalV1 input;
};
static inline bool operator==(const CarrotSelectedInput &a, const CarrotSelectedInput &b)
{
    return a.amount == b.amount && onetime_address_ref(a.input) == onetime_address_ref(b.input);
}
static inline bool operator!=(const CarrotSelectedInput &a, const CarrotSelectedInput &b)
{
    return !(a == b);
}

using select_inputs_func_t = std::function<void(
        const boost::multiprecision::uint128_t&,       // nominal output sum, w/o fee
        const std::map<std::size_t, rct::xmr_amount>&, // absolute fee per input count
        const std::size_t,                             // number of normal payment proposals
        const std::size_t,                             // number of self-send payment proposals
        std::vector<CarrotSelectedInput>&              // selected inputs result
    )>;

using carve_fees_and_balance_func_t = std::function<void(
        const boost::multiprecision::uint128_t&,                // input sum amount
        const rct::xmr_amount,                                  // fee
        std::vector<CarrotPaymentProposalV1>&,                  // normal payment proposals [inout]
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1>& // selfsend payment proposals [inout]
    )>;

/**
 * brief: make_carrot_transaction_proposal_v1 - generic core function for forming single Carrot transaction proposals
 * param: normal_payment_proposals - normal payment proposals to be included in the tx
 * param: selfsend_payment_proposals - selfsend payment proposals to be included in the tx
 * param: fee_per_weight - concrete fee is calculated as transaction weight times this value
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: select_inputs - input selection callback (see more below)
 * param: carve_fees_and_balance - fee carving callback (see more below)
 * param: change_address_spend_pubkey - address spend pubkey to send to for change selfsend enotes
 * param: change_address_index - subaddress index of change_address_spend_pubkey in your account
 * outparam: tx_proposal_out - a fully formed Carrot transaction proposal
 *
 * This function will add a selfsend payment proposal if no other selfsend payment proposal is
 * passed in, so that all Carrot transactions contain at least 1 selfsend enote, conforming to the
 * "Mandatory self-send enote rule." This function also fills in all random fields where applicable
 * so that generating enotes and signable transaction hashes from this transaction proposal is
 * deterministic.
 *
 * `select_inputs`: takes in four arguments: 1) the "nominal output sum", which is the sum of the
 * amounts in the passed normal and selfsend payment proposals, excluding fee, 2) the concrete fee
 * for the transaction indexed by number of inputs, 3) the number of normal payment proposals, and
 * 4) the number of selfsend payment proposals, including additional ones added inside the body of
 * `make_carrot_transaction_proposal_v1`. `select_inputs` outputs a list of "selected inputs", in
 * no particular order, which are each an opening hint for a spent enote, and a corresponding
 * amount. Because the weight of a FCMP++ transaction is simply a function of number of inputs,
 * number of outputs, and tx_extra size, the exact concrete fee for each potential input count is
 * calculated and passed to the input selection callback for ease of algorithms.
 *
 * `carve_fees_and_balance`: takes in the sum of input amounts, the concrete fee, and the payment
 * proposals by reference. It should modify the *amounts* of the payment proposals such that
 * the sum of amounts of all payment proposals + fee = input amount sum. Any selfsend payment
 * proposal created in the body of this function will appear at the end of selfsend list.
 */
void make_carrot_transaction_proposal_v1(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    carve_fees_and_balance_func_t &&carve_fees_and_balance,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    CarrotTransactionProposalV1 &tx_proposal_out);
/**
 * brief: make_carrot_transaction_proposal_v1_transfer - make a "transfer-style" Carrot transaction proposal
 * param: normal_payment_proposals - normal payment proposals to be included in the tx
 * param: selfsend_payment_proposals - selfsend payment proposals to be included in the tx
 * param: fee_per_weight - concrete fee is calculated as transaction weight times this value
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: select_inputs - input selection callback (see make_carrot_transaction_proposal_v1 for info)
 * param: change_address_spend_pubkey - address spend pubkey to send to for change selfsend enotes
 * param: change_address_index - subaddress index of change_address_spend_pubkey in your account
 * param: subtractable_normal_payment_proposals - indices of normal payment proposals which are "fee subtractable"
 * param: subtractable_selfsend_payment_proposals - indices of selfsend payment proposals which are "fee subtractable"
 * outparam: tx_proposal_out - a fully formed Carrot transaction proposal
 *
 * This function *always* adds an additional selfsend enote not in `selfsend_payment_proposals`,
 * even if not strictly needed. Any leftover "change" XMR after fulfilling the passed payment
 * proposals is assigned to the aforementioned additional selfsend proposal.
 *
 * All passed payment proposals in `normal_payment_proposals` and `selfsend_payment_proposals` are
 * included in final transaction proposal as-is with their passed amount, *unless* they are marked
 * as "fee subtractable" using `subtractable_normal_payment_proposals` or
 * `subtractable_selfsend_payment_proposals`, respectively. If at least one payment proposal is
 * marked as fee subtractable, then the fee is split evenly amongst those payments are subtracted
 * from their respective amount. If no payment proposal is marked as fee subtractable, then the fee
 * is subtracted from the additional selfsend proposal.
 */
void make_carrot_transaction_proposal_v1_transfer(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    const std::set<std::size_t> &subtractable_normal_payment_proposals,
    const std::set<std::size_t> &subtractable_selfsend_payment_proposals,
    CarrotTransactionProposalV1 &tx_proposal_out);
/**
 * brief: make_carrot_transaction_proposal_v1_sweep - make a "sweep-style" Carrot transaction proposal
 * param: normal_payment_proposals - normal payment proposals to be included in the tx (with amount=0)
 * param: selfsend_payment_proposals - selfsend payment proposals to be included in the tx (with amount=0)
 * param: fee_per_weight - concrete fee is calculated as transaction weight times this value
 * param: extra - truly "extra" fields to be included in tx_extra, doesn't include ephemeral tx pubkeys or PIDs
 * param: selected_inputs - explicitly provided inputs
 * param: change_address_spend_pubkey - address spend pubkey to send to for change selfsend enotes
 * param: change_address_index - subaddress index of change_address_spend_pubkey in your account
 * outparam: tx_proposal_out - a fully formed Carrot transaction proposal
 *
 * Unlike with "transfer-style" transactions, this function does *not* add an additional selfsend
 * proposal if one is already passed in `selfsend_payment_proposals`. The fee and input amount sum
 * is divided equally amongst all payment proposals.
 */
void make_carrot_transaction_proposal_v1_sweep(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<CarrotSelectedInput> &&selected_inputs,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    CarrotTransactionProposalV1 &tx_proposal_out);

} //namespace carrot
