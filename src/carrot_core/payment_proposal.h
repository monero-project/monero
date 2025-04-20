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

// A 'payment proposal' is a proposal to make an enote sending funds to a Carrot address.
// Carrot: Cryptonote Address For Rerandomizable-RingCT-Output Transactions

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "destination.h"
#include "device.h"
#include "ringct/rctTypes.h"
#include "common/variant.h"

//third party headers

//standard headers
#include <optional>

//forward declarations


namespace carrot
{

////
// CarrotPaymentProposalV1
// - for creating an output proposal to send an amount to someone
///
struct CarrotPaymentProposalV1 final
{
    /// user address
    CarrotDestinationV1 destination;
    /// b
    rct::xmr_amount amount;
    /// anchor_norm: secret 16-byte randomness for Janus anchor
    janus_anchor_t randomness;
};

////
// CarrotPaymentProposalSelfSendV1
// - for creating an output proposal to send an change to yourself
///
struct CarrotPaymentProposalSelfSendV1 final
{
    /// one of our own address spend pubkeys: K^j_s
    crypto::public_key destination_address_spend_pubkey;
    /// a
    rct::xmr_amount amount;

    /// enote_type
    CarrotEnoteType enote_type;
    /// enote ephemeral pubkey: D_e
    std::optional<mx25519_pubkey> enote_ephemeral_pubkey;
    /// anchor: arbitrary, pre-encrypted message for _internal_ selfsends
    std::optional<janus_anchor_t> internal_message;
};

struct RCTOutputEnoteProposal
{
    CarrotEnoteV1 enote;

    // we need this opening information to make amount range proofs
    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
};

/// equality operators
bool operator==(const CarrotPaymentProposalV1 &a, const CarrotPaymentProposalV1 &b);
/// equality operators
bool operator==(const CarrotPaymentProposalSelfSendV1 &a, const CarrotPaymentProposalSelfSendV1 &b);

/**
* brief: get_enote_ephemeral_privkey - get the proposal's enote ephemeral privkey d_e
* param: proposal -
* param: input_context -
* return: d_e
*/
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context);
/**
* brief: get_enote_ephemeral_pubkey - get the proposal's enote ephemeral pubkey D_e
* param: proposal -
* param: input_context -
* return: D_e
*/
mx25519_pubkey get_enote_ephemeral_pubkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context);
/**
* brief: get_coinbase_output_proposal_v1 - convert the carrot proposal to a coinbase output proposal
* param: proposal -
* param: block_index - index of the coinbase tx's block
* outparam: output_enote_out -
* outparam: partial_memo_out -
*/
void get_coinbase_output_proposal_v1(const CarrotPaymentProposalV1 &proposal,
    const std::uint64_t block_index,
    CarrotCoinbaseEnoteV1 &output_enote_out);
/**
* brief: get_output_proposal_normal_v1 - convert the carrot proposal to an output proposal
* param: proposal -
* param: tx_first_key_image -
* outparam: output_enote_out -
* outparam: encrypted_payment_id_out - pid_enc
*/
void get_output_proposal_normal_v1(const CarrotPaymentProposalV1 &proposal,
    const crypto::key_image &tx_first_key_image,
    RCTOutputEnoteProposal &output_enote_out,
    encrypted_payment_id_t &encrypted_payment_id_out);
/**
* brief: get_output_proposal_v1 - convert the carrot proposal to an output proposal (external selfsend)
* param: proposal -
* param: k_view_dev -
* param: tx_first_key_image -
* param: other_enote_ephemeral_pubkey -
* outparam: output_enote_out -
*/
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const std::optional<mx25519_pubkey> &other_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out);
/**
* brief: get_output_proposal_internal_v1 - convert the carrot proposal to an output proposal (internal)
* param: proposal -
* param: s_view_balance_dev -
* param: account_spend_pubkey -
* param: tx_first_key_image -
* param: other_enote_ephemeral_pubkey -
* outparam: output_enote_out -
*/
void get_output_proposal_internal_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_balance_secret_device &s_view_balance_dev,
    const crypto::key_image &tx_first_key_image,
    const std::optional<mx25519_pubkey> &other_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out);
/**
* brief: gen_jamtis_payment_proposal_v1 - generate a random proposal
* param: is_subaddress - whether to generate a proposal to subaddress
* param: has_payment_id - true to generate non-zero payment ID, false for null payment ID
* param: amount -
* return: a random proposal
*/
CarrotPaymentProposalV1 gen_carrot_payment_proposal_v1(const bool is_subaddress,
    const bool has_payment_id,
    const rct::xmr_amount amount);

} //namespace carrot
