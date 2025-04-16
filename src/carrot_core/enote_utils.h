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

// @file Utilities for making and handling enotes with carrot.

#pragma once

//local headers
#include "crypto/crypto.h"
#include "core_types.h"
#include "mx25519.h"
#include "ringct/rctTypes.h"

//third party headers

//standard headers

//forward declarations

namespace carrot
{

/**
 * brief: make_carrot_enote_ephemeral_privkey - enote ephemeral privkey k_e for Carrot enotes
 *   d_e = H_n(anchor_norm, input_context, K^j_s, pid))
 * param: anchor_norm - anchor_norm
 * param: input_context - input_context
 * param: address_spend_pubkey - K^j_s
 * param: payment_id - pid
 * outparam: enote_ephemeral_privkey_out - k_e
 */
void make_carrot_enote_ephemeral_privkey(const janus_anchor_t &anchor_norm,
    const input_context_t &input_context,
    const crypto::public_key &address_spend_pubkey,
    const payment_id_t payment_id,
    crypto::secret_key &enote_ephemeral_privkey_out);
/**
 * brief: make_carrot_enote_ephemeral_pubkey_main - make enote ephemeral pubkey D_e for a main address
 *   D_e = d_e B
 * param: enote_ephemeral_privkey - d_e
 * outparam: enote_ephemeral_pubkey_out - D_e
 */
void make_carrot_enote_ephemeral_pubkey_cryptonote(const crypto::secret_key &enote_ephemeral_privkey,
    mx25519_pubkey &enote_ephemeral_pubkey_out);
/**
 * brief: make_carrot_enote_ephemeral_pubkey_subaddress - make enote ephemeral pubkey D_e for a subaddress
 *   D_e = d_e ConvertPointE(K^j_s)
 * param: enote_ephemeral_privkey - d_e
 * param: address_spend_pubkey - K^j_s
 * outparam: enote_ephemeral_pubkey_out - D_e
 */
void make_carrot_enote_ephemeral_pubkey_subaddress(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_spend_pubkey,
    mx25519_pubkey &enote_ephemeral_pubkey_out);
/**
 * brief: make_carrot_enote_ephemeral_pubkey - make enote ephemeral pubkey D_e for either either address type
 *   [is_subaddress]: D_e = d_e ConvertPointE(K^j_s)
 *   [!is_subaddress]: D_e = d_e B
 * param: enote_ephemeral_privkey - d_e
 * param: address_spend_pubkey - K^j_s
 * param: is_subaddress -
 * outparam: enote_ephemeral_pubkey_out - D_e
 */
void make_carrot_enote_ephemeral_pubkey(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_spend_pubkey,
    const bool is_subaddress,
    mx25519_pubkey &enote_ephemeral_pubkey_out);
/**
 * brief: make_carrot_uncontextualized_shared_key_receiver - perform the receiver-side ECDH exchange for Carrot enotes
 *   s_sr = k_v D_e
 * param: k_view - k_v
 * param: enote_ephemeral_pubkey - D_e
 * outparam: s_sender_receiver_unctx_out - s_sr
 * return: true if successful, false if a failure occurred in point decompression
 */
bool make_carrot_uncontextualized_shared_key_receiver(const crypto::secret_key &k_view,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out);
/**
 * brief: make_carrot_uncontextualized_shared_key_sender - perform the sender-side ECDH exchange for Carrot enotes
 *   s_sr = d_e ConvertPointE(K^j_v)
 * param: enote_ephemeral_privkey - d_e
 * param: address_view_pubkey - K^j_v
 * outparam: s_sender_receiver_unctx_out - s_sr
 * return: true if successful, false if a failure occurred in point decompression
 */
bool make_carrot_uncontextualized_shared_key_sender(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_view_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out);
/**
* brief: make_carrot_view_tag - used for optimized identification of enotes
*    vt = H_3(s_sr || input_context || Ko)
* param: s_sender_receiver_unctx - s_sr
* param: input_context - input_context
* param: onetime_address - Ko
* outparam: view_tag_out - vt
*/
void make_carrot_view_tag(const unsigned char s_sender_receiver_unctx[32],
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    view_tag_t &view_tag_out);
/**
* brief: make_carrot_input_context_coinbase - input context for a sender-receiver secret (coinbase txs)
*    input_context = "C" || IntToBytes256(block_index)
* param: block_index - block index of the coinbase tx
* return: input_context
*/
input_context_t make_carrot_input_context_coinbase(const std::uint64_t block_index);
/**
* brief: make_carrot_input_context - input context for a sender-receiver secret (standard RingCT txs)
*    input_context = "R" || KI_1
* param: first_rct_key_image - KI_1, the first spent RingCT key image in a tx
* return: input_context
*/
input_context_t make_carrot_input_context(const crypto::key_image &first_rct_key_image);
/**
* brief: make_carrot_sender_receiver_secret - contextualized sender-receiver secret s^ctx_sr
*    s^ctx_sr = H_32(s_sr, D_e, input_context)
* param: s_sender_receiver_unctx - s_sr
* param: enote_ephemeral_pubkey - D_e
* param: input_context - [standard: KI_1] [coinbase: block index]
* outparam: s_sender_receiver_out - s^ctx_sr
*   - note: this is 'crypto::hash' instead of 'crypto::secret_key' for better performance in multithreaded environments
*/
void make_carrot_sender_receiver_secret(const unsigned char s_sender_receiver_unctx[32],
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    crypto::hash &s_sender_receiver_out);
/**
* brief: make_carrot_onetime_address_extension_g - extension for transforming a receiver's spendkey into an
*        enote one-time address
*    k^o_g = H_n("..g..", s^ctx_sr, C_a)
* param: s_sender_receiver - s^ctx_sr
* param: amount_commitment - C_a
* outparam: sender_extension_out - k^o_g
*/
void make_carrot_onetime_address_extension_g(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::secret_key &sender_extension_out);
/**
* brief: make_carrot_onetime_address_extension_t - extension for transforming a receiver's spendkey into an
*        enote one-time address
*    k^o_t = H_n("..t..", s^ctx_sr, C_a)
* param: s_sender_receiver - s^ctx_sr
* param: amount_commitment - C_a
* outparam: sender_extension_out - k^o_t
*/
void make_carrot_onetime_address_extension_t(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::secret_key &sender_extension_out);
/**
* brief: make_carrot_onetime_address_extension_pubkey - create a FCMP++ onetime address extension pubkey
*    K^o_ext = k^o_g G + k^o_t T
* param: s_sender_receiver - s^ctx_sr
* param: amount_commitment - C_a
* outparam: sender_extension_pubkey_out - K^o_ext
*/
void make_carrot_onetime_address_extension_pubkey(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &sender_extension_pubkey_out);
/**
* brief: make_carrot_onetime_address - create a FCMP++ onetime address
*    Ko = K^j_s + K^o_ext = K^j_s + (k^o_g G + k^o_t T)
* param: address_spend_pubkey - K^j_s
* param: s_sender_receiver - s^ctx_sr
* param: amount_commitment - C_a
* outparam: onetime_address_out - Ko
*/
void make_carrot_onetime_address(const crypto::public_key &address_spend_pubkey,
    const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &onetime_address_out);
/**
* brief: make_carrot_amount_blinding_factor - create blinding factor for enote's amount commitment C_a
*   k_a = H_n(s^ctx_sr, a, K^j_s, enote_type)
* param: s_sender_receiver - s^ctx_sr
* param: amount - a
* param: address_spend_pubkey - K^j_s
* param: enote_type - enote_type
* outparam: amount_blinding_factor_out - k_a
*/
void make_carrot_amount_blinding_factor(const crypto::hash &s_sender_receiver,
    const rct::xmr_amount amount,
    const crypto::public_key &address_spend_pubkey,
    const CarrotEnoteType enote_type,
    crypto::secret_key &amount_blinding_factor_out);
/**
* brief: make_carrot_anchor_encryption_mask - create XOR encryption mask for enote's anchor
*   m_anchor = H_16(s^ctx_sr, Ko)
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* outparam: anchor_encryption_mask_out - m_anchor
*/
void make_carrot_anchor_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_janus_anchor_t &anchor_encryption_mask_out);
/**
* brief: encrypt_carrot_anchor - encrypt a Janus anchor for an enote
*   anchor_enc = anchor XOR m_anchor
* param: anchor -
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: anchor_enc
*/
encrypted_janus_anchor_t encrypt_carrot_anchor(const janus_anchor_t &anchor,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
* brief: decrypt_carrot_address_tag - decrypt a Janus anchor from an enote
*   anchor = anchor_enc XOR m_anchor
* param: encrypted_anchor - anchor_enc
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: anchor
*/
janus_anchor_t decrypt_carrot_anchor(const encrypted_janus_anchor_t &encrypted_anchor,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
* brief: make_carrot_amount_encryption_mask - create XOR encryption mask for enote's amount
*   m_a = H_8(s^ctx_sr, Ko)
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* outparam: amount_encryption_mask_out - m_a
*/
void make_carrot_amount_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_amount_t &amount_encryption_mask_out);
/**
* brief: encrypt_carrot_amount - encrypt an amount for an enote
*   a_enc = a XOR m_a
* param: amount - a
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: a_enc
*/
encrypted_amount_t encrypt_carrot_amount(const rct::xmr_amount amount,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
* brief: decrypt_carrot_amount - decrypt an amount from an enote
*   a = a_enc XOR m_a
* param: encrypted_amount - a_enc
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: a
*/
rct::xmr_amount decrypt_carrot_amount(const encrypted_amount_t encrypted_amount,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
* brief: make_carrot_payment_id_encryption_mask - create XOR encryption mask for enote's payment ID
*   m_pid = H_8(s^ctx_sr, Ko)
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* outparam: payment_id_encryption_mask_out - m_pid
*/
void make_carrot_payment_id_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_payment_id_t &payment_id_encryption_mask_out);
/**
* brief: encrypt_legacy_payment_id - encrypt a payment ID from an enote
*   pid_enc = pid XOR m_pid
* param: payment_id - pid
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: pid_enc
*/
encrypted_payment_id_t encrypt_legacy_payment_id(const payment_id_t payment_id,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
* brief: decrypt_legacy_payment_id - decrypt a payment ID from an enote
*   pid = pid_enc XOR m_pid
* param: encrypted_payment_id - pid_enc
* param: s_sender_receiver - s^ctx_sr
* param: onetime_address - Ko
* return: pid
*/
payment_id_t decrypt_legacy_payment_id(const encrypted_payment_id_t encrypted_payment_id,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address);
/**
 * brief: make_carrot_janus_anchor_special - make a janus anchor for "special" enotes
 *   anchor_sp = H_16(D_e, input_context, Ko, k_v)
 * param: enote_ephemeral_pubkey - D_e
 * param: input_context -
 * param: onetime_address - Ko
 * param: k_view - k_v
 * outparam: anchor_special_out - anchor_sp
 */
void make_carrot_janus_anchor_special(const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    const crypto::secret_key &k_view,
    janus_anchor_t &anchor_special_out);
/**
* brief: recover_address_spend_pubkey - get the receiver's spend key for which this RingCT onetime address
*                                              can be reconstructed as 'owned' by
*   K^j_s = Ko - K^o_ext = Ko - (k^o_g G + k^o_t U)
* param: onetime_address - Ko
* param: s_sender_receiver - s^ctx_sr
* param: amount_commitment - C_a
* outparam: address_spend_key_out: - K^j_s
*/
void recover_address_spend_pubkey(const crypto::public_key &onetime_address,
    const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &address_spend_key_out);
/**
* brief: test_carrot_view_tag - test carrot view tag
* param: s_sender_receiver_unctx - s_sr
* param: input_context -
* param: onetime_address - Ko
* param: view_tag - vt
* return: true if successfully recomputed the view tag
*/
bool test_carrot_view_tag(const unsigned char s_sender_receiver_unctx[32],
    const input_context_t input_context,
    const crypto::public_key &onetime_address,
    const view_tag_t view_tag);
/**
* brief: try_recompute_carrot_amount_commitment - test recreating the amount commitment for given enote_type and amount
* param: s_sender_receiver - s^ctx_sr
* param: nominal_amount - a'
* param: nominal_address_spend_pubkey - K^j_s'
* param: nominal_enote_type - enote_type'
* param: amount_commitment - C_a
* outparam: amount_blinding_factor_out - k_a' = H_n(s^ctx_sr, enote_type')
* return: true if successfully recomputed the amount commitment (C_a ?= k_a' G + a' H)
*/
bool try_recompute_carrot_amount_commitment(const crypto::hash &s_sender_receiver,
    const rct::xmr_amount nominal_amount,
    const crypto::public_key &nominal_address_spend_pubkey,
    const CarrotEnoteType nominal_enote_type,
    const rct::key &amount_commitment,
    crypto::secret_key &amount_blinding_factor_out);
/**
* brief: try_get_amount - test decrypting the amount and recomputing the amount commitment
* param: s_sender_receiver - s^ctx_sr
* param: encrypted_amount - a_enc
* param: onetime_address - Ko
* param: address_spend_pubkey - K^j_s
* param: amount_commitment - C_a
* outparam: enote_type_out - enote_type'
* outparam: amount_out - a' = a_enc XOR m_a
* outparam: amount_blinding_factor_out - k_a' = H_n(s^ctx_sr, enote_type')
* return: true if successfully recomputed the amount commitment (C_a ?= k_a' G + a' H)
*/
bool try_get_carrot_amount(const crypto::hash &s_sender_receiver,
    const encrypted_amount_t &encrypted_amount,
    const crypto::public_key &onetime_address,
    const crypto::public_key &address_spend_pubkey,
    const rct::key &amount_commitment,
    CarrotEnoteType &enote_type_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out);
/**
 * brief: verify_carrot_normal_janus_protection - check normal external enote is Janus safe (i.e. can recompute D_e)
 * param: nominal_anchor - anchor'
 * param: input_context -
 * param: nominal_address_spend_pubkey - K^j_s'
 * param: is_subaddress -
 * param: nominal_payment_id - pid'
 * param: enote_ephemeral_pubkey - D_e
 * return: true if this normal external enote is safe from Janus attacks
 */
bool verify_carrot_normal_janus_protection(const janus_anchor_t &nominal_anchor,
    const input_context_t &input_context,
    const crypto::public_key &nominal_address_spend_pubkey,
    const bool is_subaddress,
    const payment_id_t nominal_payment_id,
    const mx25519_pubkey &enote_ephemeral_pubkey);
} //namespace carrot
