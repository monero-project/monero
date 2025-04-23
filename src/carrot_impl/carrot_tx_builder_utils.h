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

#pragma once

//local headers
#include "carrot_tx_builder_types.h"
#include "cryptonote_basic/cryptonote_basic.h"

//third party headers

//standard headers
#include <cstddef>

//forward declarations

namespace carrot
{

static inline std::size_t get_carrot_default_tx_extra_size(const std::size_t num_outputs)
{
    // @TODO: actually implement
    return num_outputs * (32 + 1) + (8 + 2);
}

std::size_t get_carrot_coinbase_default_tx_extra_size(const std::size_t num_outputs);

static inline std::size_t get_fcmppp_tx_weight(const std::size_t num_inputs,
    const std::size_t num_outputs,
    const std::size_t tx_extra_size)
{
    // @TODO: actually implement
    return 200 + num_inputs * 1000 + num_outputs * 100 + tx_extra_size;
}

std::size_t get_fcmppp_coinbase_tx_weight(const std::size_t num_outputs,
    const std::size_t tx_extra_size);

static inline bool compare_input_key_images(const crypto::key_image &lhs, const crypto::key_image &rhs)
{
    return memcmp(lhs.data, rhs.data, sizeof(crypto::key_image)) > 0;
}

void make_carrot_transaction_proposal_v1(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    carve_fees_and_balance_func_t &&carve_fees_and_balance,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out);

void make_carrot_transaction_proposal_v1_transfer_subtractable(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    const std::set<std::size_t> &subtractable_normal_payment_proposals,
    const std::set<std::size_t> &subtractable_selfsend_payment_proposals,
    CarrotTransactionProposalV1 &tx_proposal_out);

void make_carrot_transaction_proposal_v1_transfer(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out);

void make_carrot_transaction_proposal_v1_sweep(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<CarrotSelectedInput> &&selected_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out);

void make_pruned_transaction_from_carrot_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    cryptonote::transaction &pruned_tx_out);

} //namespace carrot
