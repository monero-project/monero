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
#include "scan_unsafe.h"

//local headers
#include "enote_utils.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers


namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool scan_non_coinbase_info(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const crypto::hash &s_sender_receiver,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    payment_id_t &nominal_payment_id_out,
    janus_anchor_t &nominal_janus_anchor_out,
    CarrotEnoteType &enote_type_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out)
{
    // k^o_g = H_n("..g..", s^ctx_sr, C_a)
    make_carrot_sender_extension_g(s_sender_receiver,
        enote.amount_commitment,
        sender_extension_g_out);

    // k^o_t = H_n("..t..", s^ctx_sr, C_a)
    make_carrot_sender_extension_t(s_sender_receiver,
        enote.amount_commitment,
        sender_extension_t_out);

    // K^j_s = Ko - K^o_ext = Ko - (k^o_g G + k^o_t T)
    recover_address_spend_pubkey(enote.onetime_address,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out);

    // pid = pid_enc XOR m_pid, if applicable
    if (encrypted_payment_id)
        nominal_payment_id_out = decrypt_legacy_payment_id(*encrypted_payment_id,
            s_sender_receiver,
            enote.onetime_address);
    else
        nominal_payment_id_out = null_payment_id;

    // anchor = anchor_enc XOR m_anchor
    nominal_janus_anchor_out = decrypt_carrot_anchor(enote.anchor_enc,
        s_sender_receiver,
        enote.onetime_address);

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
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_coinbase_enote_no_janus(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &nominal_address_spend_pubkey_out,
    janus_anchor_t &nominal_janus_anchor_out)
{
    // input_context
    const input_context_t input_context = make_carrot_input_context_coinbase(enote.block_index);

    // if vt' != vt, then FAIL
    if (!test_carrot_view_tag(s_sender_receiver_unctx.data, input_context, enote.onetime_address, enote.view_tag))
        return false;

    // s^ctx_sr = H_32(s_sr, D_e, input_context)
    crypto::hash s_sender_receiver;
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote.enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    bool recovered_main_pubkey = false;
    for (const crypto::public_key &main_address_spend_pubkey : main_address_spend_pubkeys)
    {
        // k^o_g = H_n("..g..", s^ctx_sr, C_a)
        make_carrot_sender_extension_g_coinbase(s_sender_receiver,
            enote.amount,
            main_address_spend_pubkey,
            sender_extension_g_out);

        // k^o_t = H_n("..t..", s^ctx_sr, C_a)
        make_carrot_sender_extension_t_coinbase(s_sender_receiver,
            enote.amount,
            main_address_spend_pubkey,
            sender_extension_t_out);

        // K^j_s = Ko - K^o_ext = Ko - (k^o_g G + k^o_t T)
        recover_address_spend_pubkey(enote.onetime_address,
            sender_extension_g_out,
            sender_extension_t_out,
            nominal_address_spend_pubkey_out);

        if (nominal_address_spend_pubkey_out == main_address_spend_pubkey)
        {
            recovered_main_pubkey = true;
            break;
        }
    }

    if (!recovered_main_pubkey)
        return false;

    // anchor = anchor_enc XOR m_anchor
    nominal_janus_anchor_out = decrypt_carrot_anchor(enote.anchor_enc,
        s_sender_receiver,
        enote.onetime_address);

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_no_janus(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &nominal_payment_id_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &nominal_janus_anchor_out)
{
    // input_context
    const input_context_t input_context = make_carrot_input_context(enote.tx_first_key_image);

    // if vt' != vt, then FAIL
    if (!test_carrot_view_tag(s_sender_receiver_unctx.data, input_context, enote.onetime_address, enote.view_tag))
        return false;

    // s^ctx_sr = H_32(s_sr, D_e, input_context)
    crypto::hash s_sender_receiver;
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote.enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // k^g_o, k^g_t, K^j_s, pid, a, z
    return scan_non_coinbase_info(enote,
        encrypted_payment_id,
        s_sender_receiver,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        nominal_payment_id_out,
        nominal_janus_anchor_out,
        enote_type_out,
        amount_out,
        amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_internal_burnt(const CarrotEnoteV1 &enote,
    const crypto::hash &s_sender_receiver,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &internal_message_out)
{
    // k^g_o, k^t_o, K^j_s', pid', anchor', a, z
    payment_id_t dummy_payment_id;
    return scan_non_coinbase_info(enote,
        std::nullopt,
        s_sender_receiver,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        dummy_payment_id,
        internal_message_out,
        enote_type_out,
        amount_out,
        amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_carrot_normal_janus_protection(const input_context_t &input_context,
    const crypto::public_key &nominal_address_spend_pubkey,
    const bool is_subaddress,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const janus_anchor_t &nominal_janus_anchor,
    payment_id_t &nominal_payment_id_inout)
{
    // if can recompute D_e with pid', then PASS
    if (verify_carrot_normal_janus_protection(nominal_janus_anchor,
            input_context,
            nominal_address_spend_pubkey,
            is_subaddress,
            nominal_payment_id_inout,
            enote_ephemeral_pubkey))
        return true;

    // if can recompute D_e with null pid, then PASS
    nominal_payment_id_inout = null_payment_id;
    return verify_carrot_normal_janus_protection(nominal_janus_anchor,
        input_context,
        nominal_address_spend_pubkey,
        is_subaddress,
        nominal_payment_id_inout,
        enote_ephemeral_pubkey);
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_carrot_special_janus_protection(const crypto::key_image &tx_first_key_image,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const crypto::public_key &onetime_address,
    const view_incoming_key_device &k_view_dev,
    const janus_anchor_t &nominal_janus_anchor)
{
    // input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // anchor_sp = H_16(D_e, input_context, Ko, k_v)
    janus_anchor_t expected_special_anchor;
    k_view_dev.make_janus_anchor_special(enote_ephemeral_pubkey,
        input_context,
        onetime_address,
        expected_special_anchor);

    // attempt special janus check: anchor_sp ?= anchor'
    return expected_special_anchor == nominal_janus_anchor;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
