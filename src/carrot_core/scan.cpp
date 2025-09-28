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
#include "destination.h"
#include "enote_utils.h"
#include "ringct/rctOps.h"
#include "scan_unsafe.h"

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
static crypto::secret_key make_enote_ephemeral_privkey_sender(const janus_anchor_t &anchor_norm,
    const CarrotDestinationV1 &destination,
    const input_context_t &input_context)
{
    // d_e = H_n(anchor_norm, input_context, K^j_s, pid))
    crypto::secret_key enote_ephemeral_privkey;
    make_carrot_enote_ephemeral_privkey(anchor_norm,
        input_context,
        destination.address_spend_pubkey,
        destination.payment_id,
        enote_ephemeral_privkey);
    return enote_ephemeral_privkey;
}
//-------------------------------------------------------------------------------------------------------------------
static bool try_scan_carrot_coinbase_enote_checked(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_addresss_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out)
{
    // s^ctx_sr, k^g_o, k^g_t, K^j_s, pid, anchor
    janus_anchor_t nominal_janus_anchor;
    if (!try_scan_carrot_coinbase_enote_no_janus(enote,
            s_sender_receiver_unctx,
            sender_extension_g_out,
            sender_extension_t_out,
            address_spend_pubkey_out,
            nominal_janus_anchor))
        return false;

    if (!is_main_address_spend_pubkey(address_spend_pubkey_out, main_addresss_spend_pubkeys))
        return false;

    return verify_carrot_normal_janus_protection(nominal_janus_anchor,
        make_carrot_input_context_coinbase(enote.block_index),
        address_spend_pubkey_out,
        /*is_subaddress=*/false,
        null_payment_id,
        enote.enote_ephemeral_pubkey);
}
//-------------------------------------------------------------------------------------------------------------------
static bool try_scan_carrot_enote_external_normal_checked(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &nominal_janus_anchor_out,
    bool &verified_normal_janus)
{
    if (!try_scan_carrot_enote_external_no_janus(enote,
            encrypted_payment_id,
            s_sender_receiver_unctx,
            sender_extension_g_out,
            sender_extension_t_out,
            address_spend_pubkey_out,
            amount_out,
            amount_blinding_factor_out,
            payment_id_out,
            enote_type_out,
            nominal_janus_anchor_out))
        return false;

    verified_normal_janus = verify_carrot_normal_janus_protection(
        make_carrot_input_context(enote.tx_first_key_image),
        address_spend_pubkey_out,
        !is_main_address_spend_pubkey(address_spend_pubkey_out, main_address_spend_pubkeys),
        enote.enote_ephemeral_pubkey,
        nominal_janus_anchor_out,
        payment_id_out);

    return true;
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
bool try_scan_carrot_coinbase_enote_sender(
    const CarrotCoinbaseEnoteV1 &enote,
    const CarrotDestinationV1 &destination,
    const janus_anchor_t &anchor_norm,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    const crypto::secret_key enote_ephemeral_privkey = make_enote_ephemeral_privkey_sender(anchor_norm,
        destination,
        make_carrot_input_context_coinbase(enote.block_index));

    return try_scan_carrot_coinbase_enote_sender(enote,
        destination,
        enote_ephemeral_privkey,
        sender_extension_g_out,
        sender_extension_t_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_coinbase_enote_sender(
    const CarrotCoinbaseEnoteV1 &enote,
    const CarrotDestinationV1 &destination,
    const crypto::secret_key &enote_ephemeral_privkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    // s_sr = d_e ConvertPointE(K^j_v)
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        destination.address_view_pubkey,
        s_sender_receiver_unctx);

    crypto::public_key dummy_main_address_spend_pubkey;
    if (!try_scan_carrot_coinbase_enote_checked(enote,
            s_sender_receiver_unctx,
            {&destination.address_spend_pubkey, 1},
            sender_extension_g_out,
            sender_extension_t_out,
            dummy_main_address_spend_pubkey))
        return false;

    // this should've already been checked, but just for good measure...
    return dummy_main_address_spend_pubkey == destination.address_spend_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_coinbase_enote_receiver(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &main_address_spend_pubkey_out)
{
    return try_scan_carrot_coinbase_enote_checked(enote,
        s_sender_receiver_unctx,
        main_address_spend_pubkeys,
        sender_extension_g_out,
        sender_extension_t_out,
        main_address_spend_pubkey_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_coinbase_enote_receiver(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    crypto::public_key dummy_main_address_spend_pubkey;
    return try_scan_carrot_coinbase_enote_receiver(
        enote,
        s_sender_receiver_unctx,
        {&main_address_spend_pubkey, 1},
        sender_extension_g_out,
        sender_extension_t_out,
        dummy_main_address_spend_pubkey);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const janus_anchor_t &anchor_norm,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid)
{
    const crypto::secret_key enote_ephemeral_privkey = make_enote_ephemeral_privkey_sender(anchor_norm,
        destination,
        make_carrot_input_context(enote.tx_first_key_image));

    return try_scan_carrot_enote_external_sender(enote,
        encrypted_payment_id,
        destination,
        enote_ephemeral_privkey,
        sender_extension_g_out,
        sender_extension_t_out,
        amount_out,
        amount_blinding_factor_out,
        enote_type_out,
        check_pid);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const crypto::secret_key &enote_ephemeral_privkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid)
{
    // s_sr = d_e ConvertPointE(K^j_v)
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        destination.address_view_pubkey,
        s_sender_receiver_unctx);

    return try_scan_carrot_enote_external_sender(enote,
        encrypted_payment_id,
        destination,
        s_sender_receiver_unctx,
        sender_extension_g_out,
        sender_extension_t_out,
        amount_out,
        amount_blinding_factor_out,
        enote_type_out,
        check_pid);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const mx25519_pubkey &s_sender_receiver_unctx,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid)
{
    crypto::public_key recovered_address_spend_pubkey;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    janus_anchor_t dummy_janus_anchor;
    bool verified_normal_janus = false;
    if (!try_scan_carrot_enote_external_normal_checked(enote,
            encrypted_payment_id,
            s_sender_receiver_unctx,
            {&destination.address_spend_pubkey, 1},
            sender_extension_g_out,
            sender_extension_t_out,
            recovered_address_spend_pubkey,
            amount_out,
            amount_blinding_factor_out,
            recovered_payment_id,
            recovered_enote_type,
            dummy_janus_anchor,
            verified_normal_janus))
        return false;
    else if (!verified_normal_janus)
        return false;
    else if (recovered_address_spend_pubkey != destination.address_spend_pubkey)
        return false;
    else if (check_pid && recovered_payment_id != destination.payment_id)
        return false;
    else if (recovered_enote_type != CarrotEnoteType::PAYMENT)
        return false;

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_external_receiver(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device &k_view_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out)
{
    janus_anchor_t nominal_janus_anchor;
    bool verified_normal_janus = false;
    if (!try_scan_carrot_enote_external_normal_checked(enote,
            encrypted_payment_id,
            s_sender_receiver_unctx,
            main_address_spend_pubkeys,
            sender_extension_g_out,
            sender_extension_t_out,
            address_spend_pubkey_out,
            amount_out,
            amount_blinding_factor_out,
            payment_id_out,
            enote_type_out,
            nominal_janus_anchor,
            verified_normal_janus))
        return false;

    if (!verified_normal_janus && !verify_carrot_special_janus_protection(enote.tx_first_key_image,
            enote.enote_ephemeral_pubkey,
            enote.onetime_address,
            k_view_dev,
            nominal_janus_anchor))
        return false;

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_carrot_enote_internal_receiver(const CarrotEnoteV1 &enote,
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
    const input_context_t input_context = make_carrot_input_context(enote.tx_first_key_image);

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

    return try_scan_carrot_enote_internal_burnt(enote,
        s_sender_receiver,
        sender_extension_g_out,
        sender_extension_t_out,
        address_spend_pubkey_out,
        amount_out,
        amount_blinding_factor_out,
        enote_type_out,
        internal_message_out);

    // janus protection checks are not needed for internal scans
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
