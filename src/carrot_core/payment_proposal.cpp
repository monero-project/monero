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

//paired header
#include "payment_proposal.h"

//local headers
#include "int-util.h"
#include "enote_utils.h"
#include "misc_language.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static const janus_anchor_t null_anchor{{0}};
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T>
static auto auto_wiper(T &obj)
{
    static_assert(std::is_trivially_copyable<T>());
    return epee::misc_utils::create_scope_leave_handler([&]{ memwipe(&obj, sizeof(T)); });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static crypto::secret_key get_enote_ephemeral_privkey(const janus_anchor_t randomness,
    const CarrotDestinationV1 &destination,
    const input_context_t &input_context)
{
    // d_e = H_n(anchor_norm, input_context, K^j_s, pid))
    crypto::secret_key enote_ephemeral_privkey;
    make_carrot_enote_ephemeral_privkey(randomness,
        input_context,
        destination.address_spend_pubkey,
        destination.payment_id,
        enote_ephemeral_privkey);

    return enote_ephemeral_privkey;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static mx25519_pubkey get_enote_ephemeral_pubkey(const janus_anchor_t randomness,
    const CarrotDestinationV1 &destination,
    const input_context_t &input_context)
{
    // d_e = H_n(anchor_norm, input_context, K^j_s, pid))
    const crypto::secret_key enote_ephemeral_privkey{get_enote_ephemeral_privkey(randomness,
        destination,
        input_context)};

    mx25519_pubkey enote_ephemeral_pubkey;
    if (destination.is_subaddress)
        // D_e = d_e ConvertPointE(K^j_s)
        make_carrot_enote_ephemeral_pubkey_subaddress(enote_ephemeral_privkey,
            destination.address_spend_pubkey,
            enote_ephemeral_pubkey);
    else
        // D_e = d_e B
        make_carrot_enote_ephemeral_pubkey_cryptonote(enote_ephemeral_privkey,
            enote_ephemeral_pubkey);

    return enote_ephemeral_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_normal_proposal_ecdh_parts(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context,
    mx25519_pubkey &enote_ephemeral_pubkey_out,
    mx25519_pubkey &s_sender_receiver_unctx_out)
{
    // 1. d_e = H_n(anchor_norm, input_context, K^j_s, pid))
    const crypto::secret_key enote_ephemeral_privkey = get_enote_ephemeral_privkey(proposal.randomness,
        proposal.destination,
        input_context);

    // 2. make D_e
    enote_ephemeral_pubkey_out = get_enote_ephemeral_pubkey(proposal, input_context);

    // 3. s_sr = d_e ConvertPointE(K^j_v)
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        proposal.destination.address_view_pubkey,
        s_sender_receiver_unctx_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_output_proposal_parts(const crypto::hash &s_sender_receiver,
    const crypto::public_key &destination_spend_pubkey,
    const payment_id_t payment_id,
    const rct::xmr_amount amount,
    const CarrotEnoteType enote_type,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const bool coinbase_amount_commitment,
    crypto::secret_key &amount_blinding_factor_out,
    rct::key &amount_commitment_out,
    crypto::public_key &onetime_address_out,
    encrypted_amount_t &encrypted_amount_out,
    encrypted_payment_id_t &encrypted_payment_id_out)
{
    // 1. k_a = H_n(s^ctx_sr, a, K^j_s, enote_type) if !coinbase, else 1
    if (coinbase_amount_commitment)
        sc_1(to_bytes(amount_blinding_factor_out));
    else
        make_carrot_amount_blinding_factor(s_sender_receiver,
            amount,
            destination_spend_pubkey,
            enote_type,
            amount_blinding_factor_out);

    // 2. C_a = k_a G + a H
    amount_commitment_out = rct::commit(amount, rct::sk2rct(amount_blinding_factor_out));

    // 3. Ko = K^j_s + K^o_ext = K^j_s + (k^o_g G + k^o_t T)
    make_carrot_onetime_address(destination_spend_pubkey,
        s_sender_receiver,
        amount_commitment_out,
        onetime_address_out);
    
    // 4. a_enc = a XOR m_a
    encrypted_amount_out = encrypt_carrot_amount(amount,
        s_sender_receiver,
        onetime_address_out);
    
    // 5. pid_enc = pid XOR m_pid
    encrypted_payment_id_out = encrypt_legacy_payment_id(payment_id, s_sender_receiver, onetime_address_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void get_external_output_proposal_parts(const mx25519_pubkey &s_sender_receiver_unctx,
    const crypto::public_key &destination_spend_pubkey,
    const payment_id_t payment_id,
    const rct::xmr_amount amount,
    const CarrotEnoteType enote_type,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const bool coinbase_amount_commitment,
    crypto::hash &s_sender_receiver_out,
    crypto::secret_key &amount_blinding_factor_out,
    rct::key &amount_commitment_out,
    crypto::public_key &onetime_address_out,
    encrypted_amount_t &encrypted_amount_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    view_tag_t &view_tag_out)
{
    // 1. s^ctx_sr = H_32(s_sr, D_e, input_context)
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_out);

    // 2. get other parts: k_a, C_a, Ko, a_enc, pid_enc
    get_output_proposal_parts(s_sender_receiver_out,
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

    // 3. vt = H_3(s_sr || input_context || Ko)
    make_carrot_view_tag(s_sender_receiver_unctx.data, input_context, onetime_address_out, view_tag_out);
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
           a.amount                           == b.amount &&
           a.enote_type                       == b.enote_type &&
           a.internal_message                 == b.internal_message &&
           0 == memcmp(&a.enote_ephemeral_pubkey, &b.enote_ephemeral_pubkey, sizeof(mx25519_pubkey));
}
//-------------------------------------------------------------------------------------------------------------------
crypto::secret_key get_enote_ephemeral_privkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context)
{
    return get_enote_ephemeral_privkey(proposal.randomness, proposal.destination, input_context);
}
//-------------------------------------------------------------------------------------------------------------------
mx25519_pubkey get_enote_ephemeral_pubkey(const CarrotPaymentProposalV1 &proposal,
    const input_context_t &input_context)
{
    return get_enote_ephemeral_pubkey(proposal.randomness, proposal.destination, input_context);
}
//-------------------------------------------------------------------------------------------------------------------
void get_coinbase_output_proposal_v1(const CarrotPaymentProposalV1 &proposal,
    const std::uint64_t block_index,
    CarrotCoinbaseEnoteV1 &output_enote_out)
{
    // 1. sanity checks
    CHECK_AND_ASSERT_THROW_MES(proposal.randomness != null_anchor,
        "get coinbase output proposal v1: invalid randomness for janus anchor (zero).");
    CHECK_AND_ASSERT_THROW_MES(!proposal.destination.is_subaddress,
        "get coinbase output proposal v1: subaddresses aren't allowed as destinations of coinbase outputs");
    CHECK_AND_ASSERT_THROW_MES(proposal.destination.payment_id == null_payment_id,
        "get coinbase output proposal v1: integrated addresses aren't allowed as destinations of coinbase outputs");

    // 2. coinbase input context
    const input_context_t input_context= make_carrot_input_context_coinbase(block_index);

    // 3. make D_e and do external ECDH
    mx25519_pubkey s_sender_receiver_unctx; auto dhe_wiper = auto_wiper(s_sender_receiver_unctx);
    get_normal_proposal_ecdh_parts(proposal,
        input_context,
        output_enote_out.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    // 4. build the output enote address pieces
    crypto::hash s_sender_receiver; auto q_wiper = auto_wiper(s_sender_receiver);
    crypto::secret_key dummy_amount_blinding_factor;
    rct::key dummy_amount_commitment;
    encrypted_amount_t dummy_encrypted_amount;
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_external_output_proposal_parts(s_sender_receiver_unctx,
        proposal.destination.address_spend_pubkey,
        null_payment_id,
        proposal.amount,
        CarrotEnoteType::PAYMENT,
        output_enote_out.enote_ephemeral_pubkey,
        input_context,
        true, // coinbase_amount_commitment
        s_sender_receiver,
        dummy_amount_blinding_factor,
        dummy_amount_commitment,
        output_enote_out.onetime_address,
        dummy_encrypted_amount,
        dummy_encrypted_payment_id,
        output_enote_out.view_tag);
    
    // 5. anchor_enc = anchor XOR m_anchor
    output_enote_out.anchor_enc = encrypt_carrot_anchor(proposal.randomness,
        s_sender_receiver,
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
    CHECK_AND_ASSERT_THROW_MES(proposal.randomness != null_anchor,
        "jamtis payment proposal: invalid randomness for janus anchor (zero).");

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. make D_e and do external ECDH
    mx25519_pubkey s_sender_receiver_unctx; auto dhe_wiper = auto_wiper(s_sender_receiver_unctx);
    get_normal_proposal_ecdh_parts(proposal,
        input_context,
        output_enote_out.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    // 4. build the output enote address pieces
    crypto::hash s_sender_receiver; auto q_wiper = auto_wiper(s_sender_receiver);
    get_external_output_proposal_parts(s_sender_receiver_unctx,
        proposal.destination.address_spend_pubkey,
        proposal.destination.payment_id,
        proposal.amount,
        CarrotEnoteType::PAYMENT,
        output_enote_out.enote.enote_ephemeral_pubkey,
        input_context,
        false, // coinbase_amount_commitment
        s_sender_receiver,
        output_enote_out.amount_blinding_factor,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.amount_enc,
        encrypted_payment_id_out,
        output_enote_out.enote.view_tag);
    
    // 5. anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(proposal.randomness,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 6. save the amount and first key image
    output_enote_out.amount                   = proposal.amount;
    output_enote_out.enote.tx_first_key_image = tx_first_key_image;
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_special_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_incoming_key_device &k_view_dev,
    const crypto::key_image &tx_first_key_image,
    const std::optional<mx25519_pubkey> &other_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out)
{
    // 1. sanity checks
    CHECK_AND_ASSERT_THROW_MES(!proposal.internal_message,
        "get output proposal special v1: internal messages are only for internal selfsends, not special selfsends");

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. D_e
    const bool missing_enote_ephemeral_pubkeys = !proposal.enote_ephemeral_pubkey && !other_enote_ephemeral_pubkey;
    const bool mismatched_enote_ephemeral_pubkeys = proposal.enote_ephemeral_pubkey &&
        other_enote_ephemeral_pubkey &&
        memcmp(&*proposal.enote_ephemeral_pubkey, &*other_enote_ephemeral_pubkey, sizeof(mx25519_pubkey));
    CHECK_AND_ASSERT_THROW_MES(!missing_enote_ephemeral_pubkeys,
        "get output proposal special v1: no enote ephemeral pubkey provided");
    CHECK_AND_ASSERT_THROW_MES(!mismatched_enote_ephemeral_pubkeys,
        "get output proposal special v1: mismatched enote ephemeral pubkeys provided");
    const mx25519_pubkey enote_ephemeral_pubkey = proposal.enote_ephemeral_pubkey.value_or(
        other_enote_ephemeral_pubkey.value_or(mx25519_pubkey{}));

    // 4. s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx; auto ecdh_wiper = auto_wiper(s_sender_receiver_unctx);
    CHECK_AND_ASSERT_THROW_MES(k_view_dev.view_key_scalar_mult_x25519(enote_ephemeral_pubkey,
        s_sender_receiver_unctx),
        "get output proposal special v1: HW device failed to perform ECDH with ephemeral pubkey");

    // 5. build the output enote address pieces
    crypto::hash s_sender_receiver; auto q_wiper = auto_wiper(s_sender_receiver);
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_external_output_proposal_parts(s_sender_receiver_unctx,
        proposal.destination_address_spend_pubkey,
        null_payment_id,
        proposal.amount,
        proposal.enote_type,
        enote_ephemeral_pubkey,
        input_context,
        false, // coinbase_amount_commitment
        s_sender_receiver,
        output_enote_out.amount_blinding_factor,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.amount_enc,
        dummy_encrypted_payment_id,
        output_enote_out.enote.view_tag);

    // 6. make special janus anchor: anchor_sp = H_16(D_e, input_context, Ko, k_v)
    janus_anchor_t janus_anchor_special;
    k_view_dev.make_janus_anchor_special(enote_ephemeral_pubkey,
        input_context,
        output_enote_out.enote.onetime_address,
        janus_anchor_special);

    // 7. encrypt special anchor: anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(janus_anchor_special,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 8. save the enote ephemeral pubkey, first tx key image, and amount
    output_enote_out.enote.enote_ephemeral_pubkey = enote_ephemeral_pubkey;
    output_enote_out.enote.tx_first_key_image     = tx_first_key_image;
    output_enote_out.amount                       = proposal.amount;
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_proposal_internal_v1(const CarrotPaymentProposalSelfSendV1 &proposal,
    const view_balance_secret_device &s_view_balance_dev,
    const crypto::key_image &tx_first_key_image,
    const std::optional<mx25519_pubkey> &other_enote_ephemeral_pubkey,
    RCTOutputEnoteProposal &output_enote_out)
{
    // 1. sanity checks
    // @TODO

    // 2. input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. D_e
    const bool missing_enote_ephemeral_pubkeys = !proposal.enote_ephemeral_pubkey && !other_enote_ephemeral_pubkey;
    const bool mismatched_enote_ephemeral_pubkeys = proposal.enote_ephemeral_pubkey &&
        other_enote_ephemeral_pubkey &&
        memcmp(&*proposal.enote_ephemeral_pubkey, &*other_enote_ephemeral_pubkey, sizeof(mx25519_pubkey));
    CHECK_AND_ASSERT_THROW_MES(!missing_enote_ephemeral_pubkeys,
        "get output proposal internal v1: no enote ephemeral pubkey provided");
    CHECK_AND_ASSERT_THROW_MES(!mismatched_enote_ephemeral_pubkeys,
        "get output proposal internal v1: mismatched enote ephemeral pubkeys provided");
    const mx25519_pubkey enote_ephemeral_pubkey = proposal.enote_ephemeral_pubkey.value_or(
        other_enote_ephemeral_pubkey.value_or(mx25519_pubkey{}));

    // 4. s^ctx_sr = H_32(s_vb, D_e, input_context)
    crypto::hash s_sender_receiver; auto q_wiper = auto_wiper(s_sender_receiver);
    s_view_balance_dev.make_internal_sender_receiver_secret(enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // 5. build the output enote address pieces
    encrypted_payment_id_t dummy_encrypted_payment_id;
    get_output_proposal_parts(s_sender_receiver,
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

    // 6. vt = H_3(s_vb || input_context || Ko)
    s_view_balance_dev.make_internal_view_tag(input_context,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.view_tag);

    // 7. anchor = given message OR 0s, if not available
    const janus_anchor_t anchor = proposal.internal_message.value_or(janus_anchor_t{});

    // 8. encrypt anchor: anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(anchor,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 9. save the enote ephemeral pubkey, first tx key image, and amount
    output_enote_out.enote.enote_ephemeral_pubkey = enote_ephemeral_pubkey;
    output_enote_out.enote.tx_first_key_image     = tx_first_key_image;
    output_enote_out.amount                       = proposal.amount;
}
//-------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalV1 gen_carrot_payment_proposal_v1(const bool is_subaddress,
    const bool has_payment_id,
    const rct::xmr_amount amount,
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
