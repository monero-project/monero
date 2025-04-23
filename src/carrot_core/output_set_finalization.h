// Copyright (c) 2024, The Monero Project
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

//! @file Utilities for constructing output proposal sets that adhere to Carrot rules

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "common/variant.h"
#include "config.h"
#include "payment_proposal.h"
#include "ringct/rctTypes.h"

//third party headers

//standard headers
#include <optional>
#include <vector>

//forward declarations


namespace carrot
{
enum class AdditionalOutputType
{
    PAYMENT_SHARED, // selfsend proposal with enote_type="payment" with a shared D_e
    CHANGE_SHARED,  // selfsend proposal with enote_type="change" with a shared D_e
    CHANGE_UNIQUE,  // selfsend proposal with enote_type="change" with a unique D_e
    DUMMY           // outgoing proposal to a random address
};

/**
 * brief: get_additional_output_type - get the type of the additional enote needed to finalize an output set
 * param: num_outgoing - number of outgoing transfers
 * param: num_selfsend - number of selfsend transfers
 * param: need_change_output - whether an additional change output needs to be included for balance
 * param: have_payment_type_selfsend - true if the enote set has a selfsend enote with enote_type="payment"
 * return: AdditionalOutputType if need an additional enote, else std::nullopt
 * throw: std::runtime_error if the output set is in a state where it cannot be finalized
 */
std::optional<AdditionalOutputType> get_additional_output_type(const size_t num_outgoing,
    const size_t num_selfsend,
    const bool need_change_output,
    const bool have_payment_type_selfsend);
/**
 * brief: get_additional_output_proposal - get an additional output proposal to complete an output set
 * param: num_outgoing - number of outgoing transfers
 * param: num_selfsend - number of selfsend transfers
 * param: needed_change_amount - the amount of leftover change needed to be included
 * param: have_payment_type_selfsend - true if the enote set has a selfsend enote with enote_type="payment"
 * param: change_address_spend_pubkey - K^j_s of our change address
 * return: an output proposal if need an additional enote, else none
 * throw: std::runtime_error if the output set is in a state where it cannot be finalized
 */
tools::optional_variant<CarrotPaymentProposalV1, CarrotPaymentProposalSelfSendV1> get_additional_output_proposal(
    const size_t num_outgoing,
    const size_t num_selfsend,
    const rct::xmr_amount needed_change_amount,
    const bool have_payment_type_selfsend,
    const crypto::public_key &change_address_spend_pubkey);
/**
 * brief: get_output_enote_proposals - convert a *finalized* set of payment proposals into output enote proposals
 * param: normal_payment_proposals -
 * param: selfsend_payment_proposals -
 * param: dummy_encrypted_payment_id - random pid_enc, required if no integrated addresses in normal payment proposals
 * param: s_view_balance_dev - pointer to view-balance device (OPTIONAL)
 * param: k_view_dev - pointer to view-incoming device (OPTIONAL)
 * param: tx_first_key_image - KI_1
 * outparam: output_enote_proposals_out -
 * outparam: encrypted_payment_id_out - pid_enc
 * outparam: payment_proposal_order_out - (is self-send, payment idx) pairs which specify order of proposals
 *                                        converted to output enotes (OPTIONAL)
 * throw: std::runtime_error if the payment proposals do not represent a valid tx output set, or if no devices
 * 
 * If s_view_balance_dev is not NULL, then the selfsend payments are converted into *internal* enotes.
 * Otherwise, if k_view_dev is not NULL, then the selfsend payments are converted into *external* enotes.
 */
void get_output_enote_proposals(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalSelfSendV1> &selfsend_payment_proposals,
    const std::optional<encrypted_payment_id_t> &dummy_encrypted_payment_id,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::key_image &tx_first_key_image,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    std::vector<std::pair<bool, std::size_t>> *payment_proposal_order_out = nullptr);
/**
 * brief: get_coinbase_output_enotes - convert a *finalized* set of payment proposals into coinbase output enotes
 * param: normal_payment_proposals -
 * param: block_index -
 * outparam: output_coinbase_enotes_out -
 * throw: std::runtime_error if the payment proposals do not represent a valid tx output set, or if no devices
 */
void get_coinbase_output_enotes(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const uint64_t block_index,
    std::vector<CarrotCoinbaseEnoteV1> &output_coinbase_enotes_out);

} //namespace carrot
