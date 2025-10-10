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

//! @file Low-level helper utilities for scanning carrot enotes

/**
 * These functions should not be used directly unless you know what you're doing. Using these
 * functions incorrectly can result in Janus attacks, incorrect PIDs, and view tag skips. For
 * safer high-level scanning functions, see the scan.h header.
 */

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "device.h"
#include "lazy_amount_commitment.h"
#include "span.h"

//third party headers

//standard headers
#include <optional>

//forward declarations


namespace carrot
{
/**
 * brief: try_scan_carrot_coinbase_enote_no_janus - attempt scan process on coinbase enote w/o Janus protection
 * param: enote -
 * param: s_sender_receiver_unctx - s_sr
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: nominal_address_spend_pubkey - K^j_s'
 * outparam: nominal_janus_anchor_out - anchor'
 * return: true iff the scan process (w/o Janus checks) succeeded
 */
bool try_scan_carrot_coinbase_enote_no_janus(
    const CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &nominal_address_spend_pubkey_out,
    janus_anchor_t &nominal_janus_anchor_out);
/**
 * brief: try_scan_carrot_enote_external_no_janus - attempt scan process on external enote,
 *                                                  w/o Janus protection nor correct PID
 * param: enote -
 * param: encrypted_payment_id - pid_enc
 * param: s_sender_receiver_unctx - s_sr
 * param: k_view_dev -
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: address_spend_pubkey_out - K^j_s
 * outparam: amount_out - a
 * outparam: amount_blinding_factor_out - k_a
 * outparam: payment_id_out - pid
 * outparam: enote_type_out - enote_type
 * outparam: nominal_janus_anchor_out - anchor'
 * return: true iff the scan process (w/o Janus or PID checks) succeeded
 */
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
    janus_anchor_t &nominal_janus_anchor_out);
/**
 * brief: try_scan_carrot_enote_internal_burnt - attempt scan process on internal enote w/o
 *                                               regular burning bug check or view tag check
 * param: enote -
 * param: s_sender_receiver - s^ctx_sr
 * outparam: sender_extension_g_out - k^g_o
 * outparam: sender_extension_g_out - k^t_o
 * outparam: address_spend_pubkey_out - K^j_s
 * outparam: amount_out - a
 * outparam: amount_blinding_factor_out - k_a
 * outparam: enote_type_out - enote_type
 * outparam: internal_message_out - anchor'
 * return: true iff the scan process (w/o view tag check) succeeded
 */
bool try_scan_carrot_enote_internal_burnt(const CarrotEnoteV1 &enote,
    const crypto::hash &s_sender_receiver,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out,
    janus_anchor_t &internal_message_out);
/**
 * brief: verify_carrot_normal_janus_protection - verify that scanned normal enote is not attempting a
 *                                                Janus attack, and check pid
 * param: input_context - input_context
 * param: nominal_address_spend_pubkey - K^j_s'
 * param: is_subaddress - true iff K^j_s' corresponds to a subaddress
 * param: enote_ephemeral_pubkey - D_e
 * param: nominal_janus_anchor - anchor'
 * outparam: nominal_payment_id_inout - takes pid', sets to nullpid if not associated to this enote
 * return: true iff it is computationally intractable for a sender to succeed at a Janus attack
 */
bool verify_carrot_normal_janus_protection(const input_context_t &input_context,
    const crypto::public_key &nominal_address_spend_pubkey,
    const bool is_subaddress,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const janus_anchor_t &nominal_janus_anchor,
    payment_id_t &nominal_payment_id_inout);
/**
 * brief: verify_carrot_special_janus_protection - verify that scanned special enote is not attempting a Janus attack
 * param: tx_first_key_image - KI_1
 * param: enote_ephemeral_pubkey - D_e
 * param: onetime_address - K_o
 * param: k_view_dev -
 * param: nominal_janus_anchor - anchor'
 * return: true iff it is computationally intractable for a sender to succeed at a Janus attack
 */
bool verify_carrot_special_janus_protection(const crypto::key_image &tx_first_key_image,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const crypto::public_key &onetime_address,
    const view_incoming_key_device &k_view_dev,
    const janus_anchor_t &nominal_janus_anchor);

} //namespace carrot
