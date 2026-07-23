// Copyright (c) 2024-2026, The Monero Project
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

/// @file Utilities for converting payment proposals into Carrot enotes
/// A 'payment proposal' is a proposal to make an enote sending funds to a Carrot address.

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "destination.h"
#include "device.h"

//third party headers

//standard headers
#include <optional>

//forward declarations


namespace carrot
{


/**
 * @brief Proposal for a normal payment enote to an destination for a specific amount
 */
struct CarrotPaymentProposalV1 final
{
    /// user address
    CarrotDestinationV1 destination;
    /// b
    xmr_amount amount;
    /// anchor_norm: secret randomness for Janus anchor
    janus_anchor_t randomness;
};

/**
 * @brief Proposal for a self-send enote to own address spend pubkey for a specific amount
 */
struct CarrotPaymentProposalSelfSendV1 final
{
    /// one of our own address spend pubkeys: K^j_s
    crypto::public_key destination_address_spend_pubkey;
    /// whether K^j_s belongs to a subaddress
    bool is_subaddress;
    /// a
    xmr_amount amount;

    /// enote_type
    CarrotEnoteType enote_type;
    /// enote ephemeral privkey: d_e
    std::optional<crypto::secret_key> enote_ephemeral_privkey;
    /// anchor: arbitrary, pre-encrypted message for _internal_ selfsends
    std::optional<janus_anchor_t> internal_message;
};

/**
 * @brief Finalized rerandomizable RingCT enote, and data required to prove range proofs on it
 */
struct RCTOutputEnoteProposal final
{
    CarrotEnoteV1 enote;

    // we need this opening information to make amount range proofs
    xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
};

/// equality operators
bool operator==(const CarrotPaymentProposalV1 &a, const CarrotPaymentProposalV1 &b);
/// equality operators
bool operator==(const CarrotPaymentProposalSelfSendV1 &a, const CarrotPaymentProposalSelfSendV1 &b);

/**
 * @brief Get normal proposal's enote ephemeral privkey d_e
 * @param proposal -
 * @param input_context -
 * @return d_e
 */
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context);
/**
 * @brief Get self-send proposal's enote ephemeral privkey d_e
 * @param proposal -
 * @param other_normal_payment_proposal other normal payment proposal in a 2-out tx, if applicable
 * @param other_self_send_proposal other self-send payment proposal in a 2-out tx, if applicable
 * @param tx_first_key_image -
 * @return d_e
 */
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalSelfSendV1 &proposal,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    const crypto::key_image &tx_first_key_image);
/**
 * @brief Get normal proposal's enote ephemeral pubkey D_e
 * @param proposal -
 * @param input_context -
 * @return D_e
 */
mx25519_pubkey get_enote_ephemeral_pubkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context);
/**
 * @brief Get self-send proposal's enote ephemeral pubkey D_e
 * @param proposal -
 * @param other_normal_payment_proposal other normal payment proposal in a 2-out tx, if applicable
 * @param other_self_send_proposal other self-send payment proposal in a 2-out tx, if applicable
 * @param tx_first_key_image -
 * @return D_e
 */
mx25519_pubkey get_enote_ephemeral_pubkey(const CarrotPaymentProposalSelfSendV1 &proposal,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    const crypto::key_image &tx_first_key_image);
/**
 * @brief Convert the carrot proposal to a coinbase output enote
 * @param proposal -
 * @param block_index index of the coinbase tx's block
 * @param[out] output_enote_out -
 */
void get_coinbase_enote_v1(const CarrotPaymentProposalV1 &proposal,
    const std::uint64_t block_index,
    CarrotCoinbaseEnoteV1 &output_enote_out);
/**
 * @brief Convert the carrot proposal to an output proposal
 * @param proposal -
 * @param tx_first_key_image -
 * @param[out] output_enote_out -
 * @param[out] encrypted_payment_id_out pid_enc
 */
void get_output_proposal_normal_v1(const CarrotPaymentProposalV1 &proposal,
    const crypto::key_image &tx_first_key_image,
    RCTOutputEnoteProposal &output_enote_out,
    encrypted_payment_id_t &encrypted_payment_id_out);
/**
 * @brief Convert the carrot proposal to an output proposal (external selfsend)
 * @param proposal -
 * @param k_view_dev -
 * @param tx_first_key_image -
 * @param other_enote_ephemeral_pubkey -
 * @param other_normal_payment_proposal other normal payment proposal in a 2-out tx, if applicable
 * @param other_self_send_proposal other self-send payment proposal in a 2-out tx, if applicable
 * @param explicit_enote_ephemeral_pubkey explicit D_e to use for enote, ignores d_e value in proposal
 * @param[out] output_enote_out -
 */
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    RCTOutputEnoteProposal &output_enote_out);
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const mx25519_pubkey &explicit_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out);
/**
 * @brief Convert the carrot proposal to an output proposal (internal)
 * @param proposal -
 * @param s_view_balance_dev -
 * @param tx_first_key_image -
 * @param other_normal_payment_proposal other normal payment proposal in a 2-out tx, if applicable
 * @param other_self_send_proposal other self-send payment proposal in a 2-out tx, if applicable
 * @param[out] output_enote_out -
 */
void get_output_proposal_internal_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_balance_secret_device &s_view_balance_dev,
    const crypto::key_image &tx_first_key_image,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    RCTOutputEnoteProposal &output_enote_out);
/**
 * @brief Generate a random payment proposal
 * @param is_subaddress whether to generate a proposal to subaddress
 * @param has_payment_id true to generate non-zero payment ID, false for null payment ID
 * @param amount -
 * @return a random proposal
 */
CarrotPaymentProposalV1 gen_carrot_payment_proposal_v1(const bool is_subaddress,
    const bool has_payment_id,
    const xmr_amount amount);

} //namespace carrot
