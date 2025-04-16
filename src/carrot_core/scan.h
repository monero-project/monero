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

//! @file Utilities for scanning carrot enotes

/**
 * The "scan process" mentioned in this file is described in Section 8.1 "Enote Scan" of the Carrot specification:
 *   https://github.com/jeffro256/carrot/blob/master/carrot.md#81-enote-scan
 */

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "device.h"
#include "span.h"

//third party headers

//standard headers
#include <optional>

//forward declarations
namespace carrot { struct CarrotDestinationV1; }


namespace carrot
{
/**
 * brief: make_carrot_uncontextualized_shared_key_receiver - perform the receiver-side ECDH exchange for Carrot enotes
 *   s_sr = k_v D_e
 * param: k_view_dev -
 * param: enote_ephemeral_pubkey - D_e
 * outparam: s_sender_receiver_unctx_out - s_sr
 * return: true if successful, false if a failure occurred in point decompression
 */
bool make_carrot_uncontextualized_shared_key_receiver(
    const view_incoming_key_device &k_view_dev,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out);
/**
 * brief: try_scan_carrot_coinbase_enote_[sender/receiver] - attempt scan process on coinbase enote
 * param: enote -
 * param: destination - (is_subaddress, K^j_s, K^j_v, pid)
 * param: anchor_norm - anchor_norm
 * param: enote_ephemeral_privkey - d_e
 * param: s_sender_receiver_unctx - s_sr
 * param: main_address_spend_pubkeys - {K^0_s, ...}
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: main_address_spend_pubkey_out - K^0_s, which will be one of main_address_spend_pubkeys
 * return: true iff the scan process succeeded
 */
bool try_scan_carrot_coinbase_enote_sender(
    const CarrotCoinbaseEnoteV1 &enote,
    const CarrotDestinationV1 &destination,
    const janus_anchor_t &anchor_norm,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out);
bool try_scan_carrot_coinbase_enote_sender(
    const CarrotCoinbaseEnoteV1 &enote,
    const CarrotDestinationV1 &destination,
    const crypto::secret_key &enote_ephemeral_privkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out);
bool try_scan_carrot_coinbase_enote_receiver(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &main_address_spend_pubkey_out);
bool try_scan_carrot_coinbase_enote_receiver(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const crypto::public_key &main_address_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out);
/**
 * brief: try_scan_carrot_enote_external_[sender/receiver] - attempt scan process on external enote
 * param: enote -
 * param: encrypted_payment_id - pid_enc
 * param: destination - (is_subaddress, K^j_s, K^j_v, pid)
 * param: anchor_norm - anchor_norm
 * param: enote_ephemeral_privkey - d_e
 * param: s_sender_receiver_unctx - s_sr
 * param: main_address_spend_pubkeys - {K^0_s, ...}
 * param: k_view_dev -
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: address_spend_pubkey_out - K^j_s
 * outparam: amount_out - a
 * outparam: amount_blinding_factor_out - k_a
 * outparam: payment_id_out - pid
 * outparam: enote_type_out - enote_type
 * return: true iff the scan process succeeded
 *
 * warning: the _sender overloads cannot check for Janus protection on special enotes
 */
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const janus_anchor_t &anchor_norm,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid = true);
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const crypto::secret_key &enote_ephemeral_privkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid = true);
bool try_scan_carrot_enote_external_sender(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const CarrotDestinationV1 &destination,
    const mx25519_pubkey &s_sender_receiver_unctx,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    const bool check_pid = true);
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
    CarrotEnoteType &enote_type_out);
/**
 * brief: try_scan_carrot_enote_internal_receiver - attempt scan process on internal enote
 * param: enote -
 * param: s_view_balance_dev -
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: address_spend_pubkey_out - K^j_s
 * outparam: amount_out - a
 * outparam: amount_blinding_factor_out - k_a
 * outparam: enote_type_out - enote_type
 * outparam: internal_message_out - anchor'
 * return: true iff the scan process succeeded
 */
bool try_scan_carrot_enote_internal_receiver(const CarrotEnoteV1 &enote,
    const view_balance_secret_device &s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &internal_message_out);
//! @TODO: try_scan_carrot_enote_internal_sender(): can't validate burning w/o passing s_sr = s_vb

} //namespace carrot
