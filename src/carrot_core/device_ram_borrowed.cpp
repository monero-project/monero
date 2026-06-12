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

//pair header
#include "device_ram_borrowed.h"

//local headers
#include "address_utils.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "crypto/wallet/crypto.h"
#include "enote_utils.h"

//third party headers

//standard headers


namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_ram_borrowed_device::view_key_scalar_mult_ed25519(const crypto::public_key &P,
    crypto::public_key &kvP) const
{
    ge_p3 P_p3;
    if (0 != ge_frombytes_vartime(&P_p3, to_bytes(P)))
        return false;
    ge_scalarmult_p3(&P_p3, to_bytes(this->m_k_view_incoming), &P_p3);
    ge_p3_tobytes(to_bytes(kvP), &P_p3);
    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_ram_borrowed_device::view_key_scalar_mult8_ed25519(const crypto::public_key &P,
    crypto::public_key &kv8P) const
{
    tools::scrubbed<crypto::key_derivation> kd;
    if (!crypto::wallet::generate_key_derivation(P, this->m_k_view_incoming, kd))
        return false;
    memcpy(&kv8P, &kd, sizeof(crypto::public_key));
    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_ram_borrowed_device::view_key_scalar_mult_x25519(const mx25519_pubkey &D,
    mx25519_pubkey &kvD) const
{
    return try_make_carrot_shared_key_receiver(m_k_view_incoming, D, kvD);
}
//-------------------------------------------------------------------------------------------------------------------
void view_incoming_key_ram_borrowed_device::make_janus_anchor_special(
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    janus_anchor_t &anchor_special_out) const
{
    return make_carrot_janus_anchor_special(enote_ephemeral_pubkey,
        input_context,
        onetime_address,
        m_k_view_incoming,
        anchor_special_out);
}
//-------------------------------------------------------------------------------------------------------------------
void view_balance_secret_ram_borrowed_device::make_internal_view_tag(const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    view_tag_t &view_tag_out) const
{
    make_carrot_view_tag(to_bytes(m_s_view_balance), input_context, onetime_address, view_tag_out);
}
//-------------------------------------------------------------------------------------------------------------------
void view_balance_secret_ram_borrowed_device::make_internal_sender_receiver_secret(
    const mx25519_pubkey &enote_ephemeral_pubkey,
    const input_context_t &input_context,
    crypto::hash &s_sender_receiver_ctx_out) const
{
    make_carrot_contextualized_sender_receiver_secret(to_bytes(m_s_view_balance),
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_ctx_out);
}
//-------------------------------------------------------------------------------------------------------------------
void generate_address_secret_ram_borrowed_device::make_address_index_preimage_1(
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &address_index_preimage_1_out) const
{
    make_carrot_address_index_preimage_1(m_s_generate_address, major_index, minor_index, address_index_preimage_1_out);
}
//-------------------------------------------------------------------------------------------------------------------
crypto::ec_point generate_image_key_ram_borrowed_device::generate_image_scalar_mult_hash_to_point(
    const crypto::public_key &onetime_address) const
{
    // I = Hp(K_o)
    crypto::ec_point P;
    crypto::derive_key_image_generator(onetime_address, P);

    // L_partial = k_gi I
    ge_p3 P_p3;
    [[maybe_unused]] const int r = ge_frombytes_vartime(&P_p3, to_bytes(P));
    assert(0 == r);
    ge_scalarmult_p3(&P_p3, to_bytes(this->m_k_generate_image), &P_p3);
    ge_p3_tobytes(to_bytes(P), &P_p3);
    return P;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
