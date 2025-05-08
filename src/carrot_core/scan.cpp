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

// Utilities for scanning carrot enotes

//paired header
#include "scan.h"

//local headers
#include "crypto/generators.h"
#include "enote_utils.h"
#include "lazy_amount_commitment.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers


namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool is_main_address_spend_pubkey(const crypto::public_key &address_spend_pubkey,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys)
{
    for (const crypto::public_key &main_address_spend_pubkey : main_address_spend_pubkeys)
        if (address_spend_pubkey == main_address_spend_pubkey)
            return true;
    return false;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void scan_carrot_dest_info(const crypto::public_key &onetime_address,
    const rct::key &amount_commitment,
    const encrypted_janus_anchor_t &encrypted_janus_anchor,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const crypto::hash &s_sender_receiver,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    payment_id_t &payment_id_out,
    janus_anchor_t &nominal_janus_anchor_out)
{
    // k^o_g = H_n("..g..", s^ctx_sr, C_a)
    make_carrot_onetime_address_extension_g(s_sender_receiver,
        amount_commitment,
        sender_extension_g_out);

    // k^o_t = H_n("..t..", s^ctx_sr, C_a)
    make_carrot_onetime_address_extension_t(s_sender_receiver,
        amount_commitment,
        sender_extension_t_out);

    // K^j_s = Ko - K^o_ext = Ko - (k^o_g G + k^o_t T)
    recover_address_spend_pubkey(onetime_address,
        s_sender_receiver,
        amount_commitment,
        address_spend_pubkey_out);

    // pid = pid_enc XOR m_pid, if applicable
    if (encrypted_payment_id)
        payment_id_out = decrypt_legacy_payment_id(*encrypted_payment_id, s_sender_receiver, onetime_address);
    else
        payment_id_out = null_payment_id;

    // anchor = anchor_enc XOR m_anchor
    nominal_janus_anchor_out = decrypt_carrot_anchor(encrypted_janus_anchor,
        s_sender_receiver,
        onetime_address);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool try_scan_carrot_external_noamount(const crypto::public_key &onetime_address,
    const lazy_amount_commitment_t &lazy_amount_commitment,
    const encrypted_janus_anchor_t &encrypted_janus_anchor,
    const view_tag_t view_tag,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const input_context_t &input_context,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::hash &s_sender_receiver_out,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    payment_id_t &payment_id_out)
{
    // if vt' != vt, then FAIL
    if (!test_carrot_view_tag(s_sender_receiver_unctx.data, input_context, onetime_address, view_tag))
        return false;

    // s^ctx_sr = H_32(s_sr, D_e, input_context)
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_out);

    // get C_a
    const rct::key amount_commitment = calculate_amount_commitment(lazy_amount_commitment);

    // k^g_o, k^t_o, K^j_s', pid', anchor'
    janus_anchor_t nominal_janus_anchor;
    scan_carrot_dest_info(onetime_address,
        amount_commitment,
        encrypted_janus_anchor,
        encrypted_payment_id,
        s_sender_receiver_out,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        payment_id_out,
        nominal_janus_anchor);

    return verify_carrot_janus_protection(input_context,
        onetime_address,
        k_view_dev,
        address_spend_pubkey_out,
        is_main_address_spend_pubkey(address_spend_pubkey_out, main_address_spend_pubkeys),
        enote_ephemeral_pubkey,
        nominal_janus_anchor,
        payment_id_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool verify_carrot_janus_protection(const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &nominal_address_spend_pubkey,
    const bool is_main_address_spend_pubkey,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const janus_anchor_t &nominal_anchor,
    payment_id_t &nominal_payment_id_inout)
{
    // make K^j_v'
    crypto::public_key nominal_address_view_pubkey;
    if (is_main_address_spend_pubkey)
    {
        // K^j_v' = k_v G
        if (!k_view_dev.view_key_scalar_mult_ed25519(crypto::get_G(), nominal_address_view_pubkey))
            return false;
    }
    else // subaddress
    {
        // K^j_v' = k_v K^j_s'
        if (!k_view_dev.view_key_scalar_mult_ed25519(nominal_address_spend_pubkey, nominal_address_view_pubkey))
            return false;
    }

    // if can recompute D_e with pid', then PASS
    if (verify_carrot_external_janus_protection(nominal_anchor,
            input_context,
            nominal_address_spend_pubkey, 
            nominal_address_view_pubkey,
            !is_main_address_spend_pubkey,
            nominal_payment_id_inout,
            enote_ephemeral_pubkey))
        return true;

    // if can recompute D_e with null pid, then PASS
    nominal_payment_id_inout = null_payment_id;
    if (verify_carrot_external_janus_protection(nominal_anchor,
            input_context,
            nominal_address_spend_pubkey, 
            nominal_address_view_pubkey,
            !is_main_address_spend_pubkey,
            null_payment_id,
            enote_ephemeral_pubkey))
        return true;

    // anchor_sp = H_16(D_e, input_context, Ko, k_v)
    janus_anchor_t expected_special_anchor;
    k_view_dev.make_janus_anchor_special(enote_ephemeral_pubkey,
        input_context,
        onetime_address,
        expected_special_anchor);

    // attempt special janus check: anchor_sp ?= anchor'
    return expected_special_anchor == nominal_anchor;
}
//-------------------------------------------------------------------------------------------------------------------
bool make_carrot_uncontextualized_shared_key_receiver(
    const view_incoming_key_device &k_view_dev,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out)
{
    return k_view_dev.view_key_scalar_mult_x25519(enote_ephemeral_pubkey, s_sender_receiver_unctx_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_ecdh_and_scan_carrot_coinbase_enote(const CarrotCoinbaseEnoteV1 &enote,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    return try_ecdh_and_scan_carrot_coinbase_enote(enote,
        k_view_dev,
        {&main_address_spend_pubkey, 1},
        sender_extension_g_out,
        sender_extension_t_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_ecdh_and_scan_carrot_coinbase_enote(
    const CarrotCoinbaseEnoteV1 &enote,
    const view_incoming_key_device &k_view_dev,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    if (!make_carrot_uncontextualized_shared_key_receiver(k_view_dev,
            enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx))
        return false;

    // input_context
    input_context_t input_context;
    make_carrot_input_context_coinbase(enote.block_index, input_context);

    // s^ctx_sr, k^g_o, k^g_t, K^j_s, pid, and Janus verification
    crypto::public_key nominal_address_spend_pubkey;
    crypto::hash dummy_s_sender_receiver;
    payment_id_t dummy_payment_id;
    if (!try_scan_carrot_external_noamount(enote.onetime_address,
            enote.amount,
            enote.anchor_enc,
            enote.view_tag,
            enote.enote_ephemeral_pubkey,
            std::nullopt,
            input_context,
            s_sender_receiver_unctx,
            k_view_dev,
            main_address_spend_pubkeys,
            dummy_s_sender_receiver,
            sender_extension_g_out,
            sender_extension_t_out,
            nominal_address_spend_pubkey,
            dummy_payment_id))
        return false;

    // if K^j_s' != K_s, then FAIL
    // - We have no "hard target" in the amount commitment, so if we want deterministic enote
    //   scanning without a subaddress table, we reject all non-main addresses in coinbase enotes
    return is_main_address_spend_pubkey(nominal_address_spend_pubkey, main_address_spend_pubkeys);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out)
{
    return try_scan_carrot_enote_external(enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        k_view_dev,
        {&main_address_spend_pubkey, 1},
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        amount_out,
        amount_blinding_factor_out,
        payment_id_out,
        enote_type_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const epee::span<const crypto::public_key> &main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out)
{
    // input_context
    input_context_t input_context;
    make_carrot_input_context(enote.tx_first_key_image, input_context);

    // s^ctx_sr, k^g_o, k^g_t, K^j_s, pid, and Janus verification
    crypto::hash s_sender_receiver;
    if (!try_scan_carrot_external_noamount(enote.onetime_address,
            enote.amount_commitment,
            enote.anchor_enc,
            enote.view_tag,
            enote.enote_ephemeral_pubkey,
            encrypted_payment_id,
            input_context,
            s_sender_receiver_unctx,
            k_view_dev,
            main_address_spend_pubkeys,
            s_sender_receiver,
            sender_extension_g_out,
            sender_extension_t_out,
            address_spend_pubkey_out,
            payment_id_out))
        return false;

    // enote_type, a, z
    return try_get_carrot_amount(s_sender_receiver,
        enote.amount_enc,
        enote.onetime_address,
        address_spend_pubkey_out,
        enote.amount_commitment,
        enote_type_out,
        amount_out,
        amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_internal(const CarrotEnoteV1 &enote,
    const view_balance_secret_device &s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &internal_message_out)
{
    // input_context
    input_context_t input_context;
    make_carrot_input_context(enote.tx_first_key_image, input_context);

    // vt = H_3(s_sr || input_context || Ko)
    view_tag_t nominal_view_tag;
    s_view_balance_dev.make_internal_view_tag(input_context, enote.onetime_address, nominal_view_tag);

    // test view tag
    if (nominal_view_tag != enote.view_tag)
        return false;

    // s^ctx_sr = H_32(s_vb, D_e, input_context)
    crypto::hash s_sender_receiver;
    s_view_balance_dev.make_internal_sender_receiver_secret(enote.enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // k^g_o, k^t_o, K^j_s', pid', anchor'
    payment_id_t dummy_payment_id;
    scan_carrot_dest_info(enote.onetime_address,
        enote.amount_commitment,
        enote.anchor_enc,
        std::nullopt,
        s_sender_receiver,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        dummy_payment_id,
        internal_message_out);

    // janus protection checks are not needed for internal scans

    // enote_type, a, z
    return try_get_carrot_amount(s_sender_receiver,
        enote.amount_enc,
        enote.onetime_address,
        address_spend_pubkey_out,
        enote.amount_commitment,
        enote_type_out,
        amount_out,
        amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_ecdh_and_scan_carrot_enote_external(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out)
{
    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    if (!k_view_dev.view_key_scalar_mult_x25519(enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx))
        return false;

    return try_scan_carrot_enote_external(enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        k_view_dev,
        main_address_spend_pubkey,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        amount_out,
        amount_blinding_factor_out,
        payment_id_out,
        enote_type_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_destination_only(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    payment_id_t &payment_id_out)
{
    // input_context
    input_context_t input_context;
    make_carrot_input_context(enote.tx_first_key_image, input_context);

    // s^ctx_sr, k^g_o, k^g_t, K^j_s, pid, and Janus verification
    crypto::hash s_sender_receiver;
    return try_scan_carrot_external_noamount(enote.onetime_address,
        enote.amount_commitment,
        enote.anchor_enc,
        enote.view_tag,
        enote.enote_ephemeral_pubkey,
        encrypted_payment_id,
        input_context,
        s_sender_receiver_unctx,
        k_view_dev,
        {&main_address_spend_pubkey, 1},
        s_sender_receiver,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        payment_id_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
