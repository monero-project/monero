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

//paired header
#include "payment_proposal.h"

//local headers
#include "enote_utils.h"
#include "exceptions.h"
#include "misc_log_ex.h"

//third party headers

//standard headers


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot.pp"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static const janus_anchor_t null_anchor{{0}};
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_normal_proposal_ecdh_parts(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context,
    crypto::x25519_pubkey &enote_ephemeral_pubkey_out,
    crypto::x25519_pubkey &s_sender_receiver_out)
{
    // 1. d_e = H_n(anchor_norm, input_context, K^j_s, pid)
    const crypto::secret_key enote_ephemeral_privkey = get_enote_ephemeral_privkey(proposal, input_context);

    // 2. make D_e
    enote_ephemeral_pubkey_out = get_enote_ephemeral_pubkey(proposal, input_context);

    // 3. s_sr = d_e ConvertPointE(K^j_v)
    CARROT_CHECK_AND_THROW(
        try_make_carrot_shared_key_sender(enote_ephemeral_privkey,
            proposal.destination.address_view_pubkey,
            s_sender_receiver_out),
        invalid_point, "invalid address view pubkey point");
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_output_proposal_parts(const crypto::hash &s_sender_receiver_ctx,
    const crypto::public_key &destination_spend_pubkey,
    const payment_id_t payment_id,
    const xmr_amount amount,
    const CarrotEnoteType enote_type,
    const crypto::x25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const bool coinbase_amount_commitment,
    crypto::secret_key &amount_blinding_factor_out,
    amount_commitment_t &amount_commitment_out,
    crypto::public_key &onetime_address_out,
    encrypted_amount_t &encrypted_amount_out,
    encrypted_payment_id_t &encrypted_payment_id_out)
{
    // 1. k_a = H_n[s^ctx_sr](a, K^j_s, enote_type) if !coinbase, else 1
    memset(amount_blinding_factor_out.data, 0, sizeof(amount_blinding_factor_out));
    if (coinbase_amount_commitment)
        amount_blinding_factor_out.data[0] = 1;
    else
        make_carrot_amount_blinding_factor(s_sender_receiver_ctx,
            amount,
            destination_spend_pubkey,
            enote_type,
            amount_blinding_factor_out);

    // 2. C_a = k_a G + a H
    amount_commitment_out = commit_carrot_amount(amount, amount_blinding_factor_out);

    // 3. Ko = K^j_s + K^o_ext = K^j_s + (k^o_g G + k^o_t T)
    bool made_ota = false;
    if (coinbase_amount_commitment)
        made_ota = try_make_carrot_onetime_address_coinbase(destination_spend_pubkey,
            s_sender_receiver_ctx,
            amount,
            onetime_address_out);
    else
        made_ota = try_make_carrot_onetime_address(destination_spend_pubkey,
            s_sender_receiver_ctx,
            amount_commitment_out,
            onetime_address_out);
    CARROT_CHECK_AND_THROW(made_ota, invalid_point, "destination spend pubkey is invalid");

    // 4. a_enc = a XOR m_a
    encrypted_amount_out = encrypt_carrot_amount(amount,
        s_sender_receiver_ctx,
        onetime_address_out);

    // 5. pid_enc = pid XOR m_pid
    encrypted_payment_id_out = encrypt_legacy_payment_id(payment_id, s_sender_receiver_ctx, onetime_address_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_external_output_proposal_parts(const crypto::x25519_pubkey &s_sender_receiver,
    const crypto::public_key &destination_spend_pubkey,
    const payment_id_t payment_id,
    const xmr_amount amount,
    const CarrotEnoteType enote_type,
    const crypto::x25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const bool coinbase_amount_commitment,
    crypto::hash &s_sender_receiver_ctx_out,
    crypto::secret_key &amount_blinding_factor_out,
    amount_commitment_t &amount_commitment_out,
    crypto::public_key &onetime_address_out,
    encrypted_amount_t &encrypted_amount_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    view_tag_t &view_tag_out)
{
    // 1. s^ctx_sr = H_32[s_sr](D_e, input_context)
    make_carrot_contextualized_sender_receiver_secret(s_sender_receiver.data,
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_ctx_out);

    // 2. get other parts: k_a, C_a, Ko, a_enc, pid_enc
    get_output_proposal_parts(s_sender_receiver_ctx_out,
        destination_spend_pubkey,
        payment_id,
        amount,
        enote_type,
        enote_ephemeral_pubkey,
        input_context,
        coinbase_amount_commitment,
        amount_blinding_factor_out,
        amount_commitment_out,
        onetime_address_out,
        encrypted_amount_out,
        encrypted_payment_id_out);

    // 3. vt = H_3[s_sr](input_context || Ko)
    make_carrot_view_tag(s_sender_receiver.data, input_context, onetime_address_out, view_tag_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotPaymentProposalV1 &a, const CarrotPaymentProposalV1 &b)
{
    return a.destination == b.destination &&
           a.amount      == b.amount &&
           a.randomness  == b.randomness;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotPaymentProposalSelfSendV1 &a, const CarrotPaymentProposalSelfSendV1 &b)
{
    return a.destination_address_spend_pubkey == b.destination_address_spend_pubkey &&
           a.is_subaddress                    == b.is_subaddress &&
           a.amount                           == b.amount &&
           a.enote_type                       == b.enote_type &&
           a.enote_ephemeral_privkey          == b.enote_ephemeral_privkey &&
           a.internal_message                 == b.internal_message;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context)
{
    // d_e = H_n(anchor_norm, input_context, K^j_s, pid)
    crypto::secret_key enote_ephemeral_privkey;
    make_carrot_enote_ephemeral_privkey(proposal.randomness,
        input_context,
        proposal.destination.address_spend_pubkey,
        proposal.destination.payment_id,
        enote_ephemeral_privkey);

    return enote_ephemeral_privkey;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalSelfSendV1 &proposal,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    const crypto::key_image &tx_first_key_image)
{
    CARROT_CHECK_AND_THROW(!other_normal_payment_proposal || !other_self_send_proposal,
        std::logic_error, "Other payment proposal in a 2-out tx may not be both normal, and a self-send");
    if (other_normal_payment_proposal)
    {
        CARROT_CHECK_AND_THROW(!proposal.enote_ephemeral_privkey.has_value(),
            component_out_of_order, "Self-send in a 2-out tx with a normal may not have an enote ephemeral privkey");
        return get_enote_ephemeral_privkey(*other_normal_payment_proposal,
            make_carrot_input_context(tx_first_key_image));
    }

    // resolve d_e b/t selfsend proposals
    const crypto::secret_key *other_enote_ephemeral_privkey =
        (other_self_send_proposal && other_self_send_proposal->enote_ephemeral_privkey)
        ? &other_self_send_proposal->enote_ephemeral_privkey.value() : nullptr;
    const std::optional<crypto::secret_key> &this_enote_ephemeral_privkey = proposal.enote_ephemeral_privkey;
    const bool missing_enote_ephemeral_privkeys = !this_enote_ephemeral_privkey && !other_enote_ephemeral_privkey;
    const bool mismatched_enote_ephemeral_privkeys = this_enote_ephemeral_privkey &&
        other_enote_ephemeral_privkey &&
        this_enote_ephemeral_privkey.value() != *other_enote_ephemeral_privkey;
    CARROT_CHECK_AND_THROW(!missing_enote_ephemeral_privkeys,
        missing_components, "no enote ephemeral privkey provided");
    CARROT_CHECK_AND_THROW(!mismatched_enote_ephemeral_privkeys,
        component_out_of_order, "conflicting enote ephemeral privkeys provided");
    return other_enote_ephemeral_privkey
        ? *other_enote_ephemeral_privkey
        : this_enote_ephemeral_privkey.value();
}
//-------------------------------------------------------------------------------------------------------------------
crypto::x25519_pubkey get_enote_ephemeral_pubkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context)
{
    // d_e = H_n(anchor_norm, input_context, K^j_s, pid)
    const crypto::secret_key enote_ephemeral_privkey{get_enote_ephemeral_privkey(proposal, input_context)};

    // D_e = d_e * ...
    crypto::x25519_pubkey enote_ephemeral_pubkey;
    const bool r = try_make_carrot_enote_ephemeral_pubkey(enote_ephemeral_privkey,
        proposal.destination.address_spend_pubkey,
        proposal.destination.is_subaddress,
        enote_ephemeral_pubkey);
    CARROT_CHECK_AND_THROW(r, invalid_point, "could not get D_e: address spend pubkey is invalid");

    return enote_ephemeral_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::x25519_pubkey get_enote_ephemeral_pubkey(
    const CarrotPaymentProposalSelfSendV1 &proposal,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    const crypto::key_image &tx_first_key_image)
{
    // if there is an other normal, use that
    CARROT_CHECK_AND_THROW(!other_normal_payment_proposal || !other_self_send_proposal,
        std::logic_error, "Other payment proposal in a 2-out tx may not be both normal, and a self-send");
    if (other_normal_payment_proposal)
    {
        const input_context_t input_context = make_carrot_input_context(tx_first_key_image);
        CARROT_CHECK_AND_THROW(!proposal.enote_ephemeral_privkey.has_value() || proposal.enote_ephemeral_privkey.value()
                == get_enote_ephemeral_privkey(*other_normal_payment_proposal, input_context),
            component_out_of_order, "Self-send in a 2-out tx with a normal has conflicting enote ephemeral privkey");
        return get_enote_ephemeral_pubkey(*other_normal_payment_proposal, input_context);
    }

    // else, resolve base K^j_s, is_subaddress b/t selfsend proposals in a 2-out, preferring
    // selfsend proposal marked as "payment" for base K^j_s so that one can make an OutProof to that
    // enote (can't do both)
    crypto::public_key base_address_spend_pubkey;
    bool base_is_subaddress;
    if (!other_self_send_proposal)
    {
        base_address_spend_pubkey = proposal.destination_address_spend_pubkey;
        base_is_subaddress = proposal.is_subaddress;
    }
    else // other_self_send_proposal
    {
        if (other_self_send_proposal->enote_type == proposal.enote_type)
        {
            CARROT_THROW(component_out_of_order, "Selfsend proposals in a 2-out tx must have differing enote types");
        }
        else if (other_self_send_proposal->enote_type == CarrotEnoteType::PAYMENT)
        {
            base_address_spend_pubkey = other_self_send_proposal->destination_address_spend_pubkey;
            base_is_subaddress = other_self_send_proposal->is_subaddress;
        }
        else if (proposal.enote_type == CarrotEnoteType::PAYMENT)
        {
            base_address_spend_pubkey = proposal.destination_address_spend_pubkey;
            base_is_subaddress = proposal.is_subaddress;
        }
        else // neither proposal has type PAYMENT
        {
            CARROT_THROW(component_out_of_order, "One selfsend proposals in a 2-out tx must have enote type PAYMENT");
        }
    }

    // d_e
    const crypto::secret_key enote_ephemeral_privkey = get_enote_ephemeral_privkey(proposal,
        other_normal_payment_proposal, other_self_send_proposal, tx_first_key_image);

    // D_e = d_e * ...
    crypto::x25519_pubkey enote_ephemeral_pubkey;
    const bool r = try_make_carrot_enote_ephemeral_pubkey(enote_ephemeral_privkey,
        base_address_spend_pubkey,
        base_is_subaddress,
        enote_ephemeral_pubkey);
    CARROT_CHECK_AND_THROW(r, invalid_point, "could not get self-send D_e: address spend pubkey is invalid");

    return enote_ephemeral_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
void get_coinbase_enote_v1(const CarrotPaymentProposalV1 &proposal,
    const std::uint64_t block_index,
    CarrotCoinbaseEnoteV1 &output_enote_out)
{
    // 1. sanity checks
    CARROT_CHECK_AND_THROW(proposal.randomness != null_anchor,
        missing_randomness, "invalid randomness for janus anchor (zero).");
    CARROT_CHECK_AND_THROW(!proposal.destination.is_subaddress,
        bad_address_type, "subaddresses aren't allowed as destinations of coinbase outputs");
    CARROT_CHECK_AND_THROW(proposal.destination.payment_id == null_payment_id,
        bad_address_type, "integrated addresses aren't allowed as destinations of coinbase outputs");

    // 2. coinbase input context
    const input_context_t input_context = make_carrot_input_context_coinbase(block_index);

    // 3. make D_e and do external ECDH
    tools::scrubbed<crypto::x25519_pubkey> s_sender_receiver;
    get_normal_proposal_ecdh_parts(proposal,
        input_context,
        output_enote_out.enote_ephemeral_pubkey,
        s_sender_receiver);

    // 4. build the output enote address pieces
    tools::scrubbed<crypto::hash> s_sender_receiver_ctx;
    crypto::secret_key dummy_amount_blinding_factor;
    amount_commitment_t dummy_amount_commitment;
    encrypted_amount_t dummy_encrypted_amount;
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_external_output_proposal_parts(s_sender_receiver,
        proposal.destination.address_spend_pubkey,
        null_payment_id,
        proposal.amount,
        CarrotEnoteType::PAYMENT,
        output_enote_out.enote_ephemeral_pubkey,
        input_context,
        /*coinbase_amount_commitment=*/true,
        s_sender_receiver_ctx,
        dummy_amount_blinding_factor,
        dummy_amount_commitment,
        output_enote_out.onetime_address,
        dummy_encrypted_amount,
        dummy_encrypted_payment_id,
        output_enote_out.view_tag);

    // 5. anchor_enc = anchor XOR m_anchor
    output_enote_out.anchor_enc = encrypt_carrot_anchor(proposal.randomness,
        s_sender_receiver_ctx,
        output_enote_out.onetime_address);

    // 6. save the amount and block index
    output_enote_out.amount = proposal.amount;
    output_enote_out.block_index = block_index;
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_normal_v1(const CarrotPaymentProposalV1 &proposal,
    const crypto::key_image &tx_first_key_image,
    RCTOutputEnoteProposal &output_enote_out,
    encrypted_payment_id_t &encrypted_payment_id_out)
{
    // 1. sanity checks
    CARROT_CHECK_AND_THROW(proposal.randomness != null_anchor,
        missing_randomness, "invalid randomness for janus anchor (zero).");

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. make D_e and do external ECDH
    tools::scrubbed<crypto::x25519_pubkey> s_sender_receiver;
    get_normal_proposal_ecdh_parts(proposal,
        input_context,
        output_enote_out.enote.enote_ephemeral_pubkey,
        s_sender_receiver);

    // 4. build the output enote address pieces
    tools::scrubbed<crypto::hash> s_sender_receiver_ctx;
    get_external_output_proposal_parts(s_sender_receiver,
        proposal.destination.address_spend_pubkey,
        proposal.destination.payment_id,
        proposal.amount,
        CarrotEnoteType::PAYMENT,
        output_enote_out.enote.enote_ephemeral_pubkey,
        input_context,
        false, // coinbase_amount_commitment
        s_sender_receiver_ctx,
        output_enote_out.amount_blinding_factor,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.amount_enc,
        encrypted_payment_id_out,
        output_enote_out.enote.view_tag);

    // 5. anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(proposal.randomness,
        s_sender_receiver_ctx,
        output_enote_out.enote.onetime_address);

    // 6. save the amount and first key image
    output_enote_out.amount                   = proposal.amount;
    output_enote_out.enote.tx_first_key_image = tx_first_key_image;
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    RCTOutputEnoteProposal &output_enote_out)
{
    // 1. Resolve D_e according to d_e derivation and 2-out rules
    const crypto::x25519_pubkey explicit_enote_ephemeral_pubkey = get_enote_ephemeral_pubkey(proposal,
        other_normal_payment_proposal,
        other_self_send_proposal,
        tx_first_key_image);

    // 2. Call override w/ explicit D_e
    get_output_proposal_special_v1(proposal,
        k_view_dev,
        tx_first_key_image,
        explicit_enote_ephemeral_pubkey,
        output_enote_out);
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const crypto::x25519_pubkey &explicit_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out)
{
    // 1. sanity checks
    CARROT_CHECK_AND_THROW(!proposal.internal_message,
        component_out_of_order, "internal messages are only for internal selfsends, not special selfsends");

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. s_sr = k_v D_e
    tools::scrubbed<crypto::x25519_pubkey> s_sender_receiver;
    CARROT_CHECK_AND_THROW(k_view_dev.view_key_scalar_mult_x25519(explicit_enote_ephemeral_pubkey, s_sender_receiver),
        crypto_function_failed, "HW device failed to perform ECDH with ephemeral pubkey");

    // 4. build the output enote address pieces
    tools::scrubbed<crypto::hash> s_sender_receiver_ctx;
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_external_output_proposal_parts(s_sender_receiver,
        proposal.destination_address_spend_pubkey,
        null_payment_id,
        proposal.amount,
        proposal.enote_type,
        explicit_enote_ephemeral_pubkey,
        input_context,
        /*coinbase_amount_commitment=*/false,
        s_sender_receiver_ctx,
        output_enote_out.amount_blinding_factor,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.amount_enc,
        dummy_encrypted_payment_id,
        output_enote_out.enote.view_tag);

    // 5. make special janus anchor: anchor_sp = H_16[k_v](D_e, input_context, Ko)
    janus_anchor_t janus_anchor_special;
    k_view_dev.make_janus_anchor_special(explicit_enote_ephemeral_pubkey,
        input_context,
        output_enote_out.enote.onetime_address,
        janus_anchor_special);

    // 6. encrypt special anchor: anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(janus_anchor_special,
        s_sender_receiver_ctx,
        output_enote_out.enote.onetime_address);

    // 7. save the enote ephemeral pubkey, first tx key image, and amount
    output_enote_out.enote.enote_ephemeral_pubkey = explicit_enote_ephemeral_pubkey;
    output_enote_out.enote.tx_first_key_image     = tx_first_key_image;
    output_enote_out.amount                       = proposal.amount;
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_internal_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_balance_secret_device &s_view_balance_dev,
    const crypto::key_image &tx_first_key_image,
    const CarrotPaymentProposalV1 *other_normal_payment_proposal,
    const CarrotPaymentProposalSelfSendV1 *other_self_send_proposal,
    RCTOutputEnoteProposal &output_enote_out)
{
    // 1. sanity checks
    // @TODO

    // 2. input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. D_e
    const crypto::x25519_pubkey enote_ephemeral_pubkey = get_enote_ephemeral_pubkey(proposal,
        other_normal_payment_proposal,
        other_self_send_proposal,
        tx_first_key_image);

    // 4. s^ctx_sr = H_32[s_vb](D_e, input_context)
    tools::scrubbed<crypto::hash> s_sender_receiver_ctx;
    s_view_balance_dev.make_internal_sender_receiver_secret(enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_ctx);

    // 5. build the output enote address pieces
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_output_proposal_parts(s_sender_receiver_ctx,
        proposal.destination_address_spend_pubkey,
        null_payment_id,
        proposal.amount,
        proposal.enote_type,
        enote_ephemeral_pubkey,
        input_context,
        false, // coinbase_amount_commitment
        output_enote_out.amount_blinding_factor,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.amount_enc,
        dummy_encrypted_payment_id);

    // 6. vt = H_3[s_vb](input_context || Ko)
    s_view_balance_dev.make_internal_view_tag(input_context,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.view_tag);

    // 7. anchor = given message OR 0s, if not available
    const janus_anchor_t anchor = proposal.internal_message.value_or(janus_anchor_t{});

    // 8. encrypt anchor: anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(anchor,
        s_sender_receiver_ctx,
        output_enote_out.enote.onetime_address);

    // 9. save the enote ephemeral pubkey, first tx key image, and amount
    output_enote_out.enote.enote_ephemeral_pubkey = enote_ephemeral_pubkey;
    output_enote_out.enote.tx_first_key_image     = tx_first_key_image;
    output_enote_out.amount                       = proposal.amount;
}
//-------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalV1 gen_carrot_payment_proposal_v1(const bool is_subaddress,
    const bool has_payment_id,
    const xmr_amount amount,
    const std::size_t num_random_memo_elements)
{
    CarrotPaymentProposalV1 temp;

    if (is_subaddress)
        temp.destination = gen_carrot_subaddress_v1();
    else if (has_payment_id)
        temp.destination = gen_carrot_integrated_address_v1();
    else
        temp.destination = gen_carrot_main_address_v1();

    temp.amount     = amount;
    temp.randomness = gen_janus_anchor();

    return temp;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
