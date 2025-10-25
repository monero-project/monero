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
#include "enote_utils.h"

//local headers
#include "config.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "crypto/generators.h"
#include "crypto/wallet/crypto.h"
#include "hash_functions.h"
#include "int-util.h"
#include "misc_language.h"
#include "ringct/rctOps.h"
#include "transcript_fixed.h"

//third party headers

//standard headers
#include <mutex>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static const mx25519_impl* get_mx25519_impl()
{
    static std::once_flag of;
    static const mx25519_impl *impl;
    std::call_once(of, [&](){ impl = mx25519_select_impl(MX25519_TYPE_AUTO); });
    if (impl == nullptr)
        throw std::runtime_error("failed to obtain a mx25519 implementation");
    return impl;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static encrypted_amount_t enc_amount(const rct::xmr_amount amount, const encrypted_amount_t &mask)
{
    static_assert(sizeof(rct::xmr_amount) == sizeof(encrypted_amount_t), "");

    // little_endian(amount) XOR H_8(q, Ko)
    encrypted_amount_t amount_LE;
    memcpy_swap64le(amount_LE.bytes, &amount, 1);
    return amount_LE ^ mask;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static rct::xmr_amount dec_amount(const encrypted_amount_t &encrypted_amount, const encrypted_amount_t &mask)
{
    static_assert(sizeof(rct::xmr_amount) == sizeof(encrypted_amount_t), "");

    // system_endian(encrypted_amount XOR H_8(q, Ko))
    const encrypted_amount_t decryptd_amount{encrypted_amount ^ mask};
    rct::xmr_amount amount;
    memcpy_swap64le(&amount, &decryptd_amount, 1);
    return amount;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename Pid,
    typename OtherPid = std::conditional_t<std::is_same_v<Pid, payment_id_t>, encrypted_payment_id_t, payment_id_t>>
static OtherPid convert_payment_id(const Pid &v)
{
    static_assert(sizeof(Pid) == PAYMENT_ID_BYTES);
    OtherPid conv;
    memcpy(&conv, &v, PAYMENT_ID_BYTES);
    return conv;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_enote_ephemeral_privkey(const janus_anchor_t &anchor_norm,
    const input_context_t &input_context,
    const crypto::public_key &address_spend_pubkey,
    const payment_id_t payment_id,
    crypto::secret_key &enote_ephemeral_privkey_out)
{
    // k_e = (H_64(anchor_norm, input_context, K^j_s, pid)) mod l
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_EPHEMERAL_PRIVKEY>(
        anchor_norm, input_context, address_spend_pubkey, payment_id);
    derive_scalar(transcript.data(), transcript.size(), nullptr, &enote_ephemeral_privkey_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_enote_ephemeral_pubkey_cryptonote(const crypto::secret_key &enote_ephemeral_privkey,
    mx25519_pubkey &enote_ephemeral_pubkey_out)
{
    // D_e = d_e B
    mx25519_scmul_base(get_mx25519_impl(),
        &enote_ephemeral_pubkey_out,
        reinterpret_cast<const mx25519_privkey*>(&enote_ephemeral_privkey));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_enote_ephemeral_pubkey_subaddress(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_spend_pubkey,
    mx25519_pubkey &enote_ephemeral_pubkey_out)
{
    // deserialize K^j_s
    ge_p3 address_spend_pubkey_p3;
    ge_frombytes_vartime(&address_spend_pubkey_p3, to_bytes(address_spend_pubkey));

    // K_e = d_e K^j_s
    ge_p3 D_e_in_ed25519;
    ge_scalarmult_p3(&D_e_in_ed25519, to_bytes(enote_ephemeral_privkey), &address_spend_pubkey_p3);

    // D_e = ConvertPointE(K_e)
    ge_p3_to_x25519(enote_ephemeral_pubkey_out.data, &D_e_in_ed25519);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_enote_ephemeral_pubkey(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_spend_pubkey,
    const bool is_subaddress,
    mx25519_pubkey &enote_ephemeral_pubkey_out)
{
    if (is_subaddress)
    {
        // D_e = d_e ConvertPointE(K^j_s)
        make_carrot_enote_ephemeral_pubkey_subaddress(enote_ephemeral_privkey,
            address_spend_pubkey,
            enote_ephemeral_pubkey_out);
    }
    else // !is_subaddress
    {
        // D_e = d_e B
        make_carrot_enote_ephemeral_pubkey_cryptonote(enote_ephemeral_privkey, enote_ephemeral_pubkey_out);
    }
}
//-------------------------------------------------------------------------------------------------------------------
bool make_carrot_uncontextualized_shared_key_receiver(const crypto::secret_key &k_view,
    const mx25519_pubkey &enote_ephemeral_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out)
{
    // s_sr = k_v D_e
    mx25519_scmul_key(get_mx25519_impl(),
        &s_sender_receiver_unctx_out,
        reinterpret_cast<const mx25519_privkey*>(&k_view),
        &enote_ephemeral_pubkey);

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool make_carrot_uncontextualized_shared_key_sender(const crypto::secret_key &enote_ephemeral_privkey,
    const crypto::public_key &address_view_pubkey,
    mx25519_pubkey &s_sender_receiver_unctx_out)
{
    // if K^j_v not in prime order subgroup, then FAIL
    ge_p3 address_view_pubkey_p3;
    if (!rct::toPointCheckOrder(&address_view_pubkey_p3, to_bytes(address_view_pubkey)))
        return false;

    // D^j_v = ConvertPointE(K^j_v)
    mx25519_pubkey address_view_pubkey_x25519;
    ge_p3_to_x25519(address_view_pubkey_x25519.data, &address_view_pubkey_p3);

    // s_sr = d_e D^j_v
    mx25519_scmul_key(get_mx25519_impl(),
        &s_sender_receiver_unctx_out,
        reinterpret_cast<const mx25519_privkey*>(&enote_ephemeral_privkey),
        &address_view_pubkey_x25519);

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_view_tag(const unsigned char s_sender_receiver_unctx[32],
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    view_tag_t &view_tag_out)
{
    // vt = H_3(s_sr || input_context || Ko)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_VIEW_TAG>(input_context, onetime_address);
    derive_bytes_3(transcript.data(), transcript.size(), s_sender_receiver_unctx, &view_tag_out);
}
//-------------------------------------------------------------------------------------------------------------------
input_context_t make_carrot_input_context_coinbase(const std::uint64_t block_index)
{
    // input_context = "C" || IntToBytes256(block_index)
    input_context_t input_context{};
    input_context.bytes[0] = CARROT_DOMAIN_SEP_INPUT_CONTEXT_COINBASE;
    memcpy_swap64le(input_context.bytes + 1, &block_index, 1);
    return input_context;
}
//-------------------------------------------------------------------------------------------------------------------
input_context_t make_carrot_input_context(const crypto::key_image &first_rct_key_image)
{
    // input_context = "R" || KI_1
    input_context_t input_context{};
    input_context.bytes[0] = CARROT_DOMAIN_SEP_INPUT_CONTEXT_RINGCT;
    memcpy(input_context.bytes + 1, first_rct_key_image.data, sizeof(crypto::key_image));
    return input_context;
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_sender_receiver_secret(const unsigned char s_sender_receiver_unctx[32],
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    crypto::hash &s_sender_receiver_out)
{
    // s^ctx_sr = H_32(s_sr, D_e, input_context)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_SENDER_RECEIVER_SECRET>(
        enote_ephemeral_pubkey, input_context);
    derive_bytes_32(transcript.data(), transcript.size(), s_sender_receiver_unctx, &s_sender_receiver_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_onetime_address_extension_g(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::secret_key &sender_extension_out)
{
    // k^o_g = H_n("..g..", s^ctx_sr, C_a)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ONETIME_EXTENSION_G>(amount_commitment);
    derive_scalar(transcript.data(), transcript.size(), &s_sender_receiver, &sender_extension_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_onetime_address_extension_t(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::secret_key &sender_extension_out)
{
    // k^o_t = H_n("..t..", s^ctx_sr, C_a)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ONETIME_EXTENSION_T>(amount_commitment);
    derive_scalar(transcript.data(), transcript.size(), &s_sender_receiver, &sender_extension_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_onetime_address_extension_pubkey(const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &sender_extension_pubkey_out)
{
    // k^o_g = H_n("..g..", s^ctx_sr, C_a)
    crypto::secret_key sender_extension_g;
    make_carrot_onetime_address_extension_g(s_sender_receiver, amount_commitment, sender_extension_g);

    // k^o_t = H_n("..t..", s^ctx_sr, C_a)
    crypto::secret_key sender_extension_t;
    make_carrot_onetime_address_extension_t(s_sender_receiver, amount_commitment, sender_extension_t);

    // K^o_ext = k^o_g G + k^o_t T
    rct::key sender_extension_pubkey_tmp;
    rct::addKeys2(sender_extension_pubkey_tmp,
        rct::sk2rct(sender_extension_g),
        rct::sk2rct(sender_extension_t),
        rct::pk2rct(crypto::get_T()));

    sender_extension_pubkey_out = rct::rct2pk(sender_extension_pubkey_tmp);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_onetime_address(const crypto::public_key &address_spend_pubkey,
    const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &onetime_address_out)
{
    // K^o_ext = k^o_g G + k^o_t T
    crypto::public_key sender_extension_pubkey;
    make_carrot_onetime_address_extension_pubkey(s_sender_receiver, amount_commitment, sender_extension_pubkey);

    // Ko = K^j_s + K^o_ext
    onetime_address_out = rct::rct2pk(rct::addKeys(
        rct::pk2rct(address_spend_pubkey), rct::pk2rct(sender_extension_pubkey)));
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_amount_blinding_factor(const crypto::hash &s_sender_receiver,
    const rct::xmr_amount amount,
    const crypto::public_key &address_spend_pubkey,
    const CarrotEnoteType enote_type,
    crypto::secret_key &amount_blinding_factor_out)
{
    // k_a = H_n(s^ctx_sr, a, K^j_s, enote_type)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_AMOUNT_BLINDING_FACTOR>(
        amount, address_spend_pubkey, static_cast<unsigned char>(enote_type));
    derive_scalar(transcript.data(), transcript.size(), &s_sender_receiver, &amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_anchor_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_janus_anchor_t &anchor_encryption_mask_out)
{
    // m_anchor = H_16(s^ctx_sr, Ko)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ENCRYPTION_MASK_ANCHOR>(onetime_address);
    derive_bytes_16(transcript.data(), transcript.size(), &s_sender_receiver, &anchor_encryption_mask_out);
}
//-------------------------------------------------------------------------------------------------------------------
encrypted_janus_anchor_t encrypt_carrot_anchor(const janus_anchor_t &anchor,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_anchor = H_16(s^ctx_sr, Ko)
    encrypted_janus_anchor_t mask;
    make_carrot_anchor_encryption_mask(s_sender_receiver, onetime_address, mask);

    // anchor_enc = anchor XOR m_anchor
    return anchor ^ mask;
}
//-------------------------------------------------------------------------------------------------------------------
janus_anchor_t decrypt_carrot_anchor(const encrypted_janus_anchor_t &encrypted_anchor,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_anchor = H_16(s^ctx_sr, Ko)
    encrypted_janus_anchor_t mask;
    make_carrot_anchor_encryption_mask(s_sender_receiver, onetime_address, mask);

    // anchor = anchor_enc XOR m_anchor
    return encrypted_anchor ^ mask;
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_amount_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_amount_t &amount_encryption_mask_out)
{
    // m_a = H_8(s^ctx_sr, Ko)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ENCRYPTION_MASK_AMOUNT>(onetime_address);
    derive_bytes_8(transcript.data(), transcript.size(), &s_sender_receiver, &amount_encryption_mask_out);
}
//-------------------------------------------------------------------------------------------------------------------
encrypted_amount_t encrypt_carrot_amount(const rct::xmr_amount amount,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_a = H_8(s^ctx_sr, Ko)
    encrypted_amount_t mask;
    make_carrot_amount_encryption_mask(s_sender_receiver, onetime_address, mask);

    // a_enc = a XOR m_a  [paying attention to system endianness]
    return enc_amount(amount, mask);
}
//-------------------------------------------------------------------------------------------------------------------
rct::xmr_amount decrypt_carrot_amount(const encrypted_amount_t encrypted_amount,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_a = H_8(s^ctx_sr, Ko)
    encrypted_amount_t mask;
    make_carrot_amount_encryption_mask(s_sender_receiver, onetime_address, mask);

    // a = a_enc XOR m_a  [paying attention to system endianness]
    return dec_amount(encrypted_amount, mask);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_payment_id_encryption_mask(const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address,
    encrypted_payment_id_t &payment_id_encryption_mask_out)
{
    // m_pid = H_8(s^ctx_sr, Ko)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ENCRYPTION_MASK_PAYMENT_ID>(onetime_address);
    derive_bytes_8(transcript.data(), transcript.size(), &s_sender_receiver, &payment_id_encryption_mask_out);
}
//-------------------------------------------------------------------------------------------------------------------
encrypted_payment_id_t encrypt_legacy_payment_id(const payment_id_t payment_id,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_pid = H_8(s^ctx_sr, Ko)
    encrypted_payment_id_t mask;
    make_carrot_payment_id_encryption_mask(s_sender_receiver, onetime_address, mask);

    // pid_enc = pid XOR m_pid
    return convert_payment_id(payment_id) ^ mask;
}
//-------------------------------------------------------------------------------------------------------------------
payment_id_t decrypt_legacy_payment_id(const encrypted_payment_id_t encrypted_payment_id,
    const crypto::hash &s_sender_receiver,
    const crypto::public_key &onetime_address)
{
    // m_pid = H_8(s^ctx_sr, Ko)
    encrypted_payment_id_t mask;
    make_carrot_payment_id_encryption_mask(s_sender_receiver, onetime_address, mask);

    // pid = pid_enc XOR m_pid
    return convert_payment_id(encrypted_payment_id ^ mask);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_janus_anchor_special(const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    const crypto::secret_key &k_view,
    janus_anchor_t &anchor_special_out)
{
    // anchor_sp = H_16(D_e, input_context, Ko, k_v)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_JANUS_ANCHOR_SPECIAL>(
        enote_ephemeral_pubkey, input_context, onetime_address);
    derive_bytes_16(transcript.data(), transcript.size(), &k_view, &anchor_special_out);
}
//-------------------------------------------------------------------------------------------------------------------
void recover_address_spend_pubkey(const crypto::public_key &onetime_address,
    const crypto::hash &s_sender_receiver,
    const rct::key &amount_commitment,
    crypto::public_key &address_spend_key_out)
{
    // K^o_ext = k^o_g G + k^o_t T
    crypto::public_key sender_extension_pubkey;
    make_carrot_onetime_address_extension_pubkey(s_sender_receiver, amount_commitment, sender_extension_pubkey);

    // K^j_s = Ko - K^o_ext
    rct::key res_tmp;
    rct::subKeys(res_tmp, rct::pk2rct(onetime_address), rct::pk2rct(sender_extension_pubkey));
    address_spend_key_out = rct::rct2pk(res_tmp);
}
//-------------------------------------------------------------------------------------------------------------------
bool test_carrot_view_tag(const unsigned char s_sender_receiver_unctx[32],
    const input_context_t input_context,
    const crypto::public_key &onetime_address,
    const view_tag_t view_tag)
{
    // vt' = H_3(s_sr || input_context || Ko)
    view_tag_t nominal_view_tag;
    make_carrot_view_tag(s_sender_receiver_unctx, input_context, onetime_address, nominal_view_tag);

    // vt' ?= vt
    return nominal_view_tag == view_tag;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_recompute_carrot_amount_commitment(const crypto::hash &s_sender_receiver,
    const rct::xmr_amount nominal_amount,
    const crypto::public_key &nominal_address_spend_pubkey,
    const CarrotEnoteType nominal_enote_type,
    const rct::key &amount_commitment,
    crypto::secret_key &amount_blinding_factor_out)
{
    // k_a' = H_n(s^ctx_sr, a', K^j_s', enote_type')
    make_carrot_amount_blinding_factor(s_sender_receiver,
        nominal_amount,
        nominal_address_spend_pubkey,
        nominal_enote_type,
        amount_blinding_factor_out);

    // C_a' = k_a' G + a' H
    const rct::key nominal_amount_commitment = rct::commit(nominal_amount, rct::sk2rct(amount_blinding_factor_out));

    // C_a' ?= C_a
    return nominal_amount_commitment == amount_commitment;
}
//-------------------------------------------------------------------------------------------------------------------
bool try_get_carrot_amount(const crypto::hash &s_sender_receiver,
    const encrypted_amount_t &encrypted_amount,
    const crypto::public_key &onetime_address,
    const crypto::public_key &address_spend_pubkey,
    const rct::key &amount_commitment,
    CarrotEnoteType &enote_type_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out)
{
    // a' = a_enc XOR m_a
    amount_out = decrypt_carrot_amount(encrypted_amount, s_sender_receiver, onetime_address);

    // set enote_type <- "payment" 
    enote_type_out = CarrotEnoteType::PAYMENT;

    // if C_a ?= k_a' G + a' H, then PASS
    if (try_recompute_carrot_amount_commitment(s_sender_receiver,
            amount_out,
            address_spend_pubkey,
            enote_type_out,
            amount_commitment,
            amount_blinding_factor_out))
        return true;

    // set enote_type <- "change" 
    enote_type_out = CarrotEnoteType::CHANGE;

    // if C_a ?= k_a' G + a' H, then PASS
    if (try_recompute_carrot_amount_commitment(s_sender_receiver,
            amount_out,
            address_spend_pubkey,
            enote_type_out,
            amount_commitment,
            amount_blinding_factor_out))
        return true;

    // neither attempt at recomputing passed: so FAIL
    return false;
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_carrot_normal_janus_protection(const janus_anchor_t &nominal_anchor,
    const input_context_t &input_context,
    const crypto::public_key &nominal_address_spend_pubkey,
    const bool is_subaddress,
    const payment_id_t nominal_payment_id,
    const mx25519_pubkey &enote_ephemeral_pubkey)
{
    // d_e' = H_n(anchor_norm, input_context, K^j_s, pid))
    crypto::secret_key nominal_enote_ephemeral_privkey;
    make_carrot_enote_ephemeral_privkey(nominal_anchor,
        input_context,
        nominal_address_spend_pubkey,
        nominal_payment_id,
        nominal_enote_ephemeral_privkey);

    // recompute D_e' for d_e' and address type
    mx25519_pubkey nominal_enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey(nominal_enote_ephemeral_privkey,
        nominal_address_spend_pubkey,
        is_subaddress,
        nominal_enote_ephemeral_pubkey);

    // D_e' ?= D_e
    return 0 == memcmp(&nominal_enote_ephemeral_pubkey, &enote_ephemeral_pubkey, sizeof(mx25519_pubkey));
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
