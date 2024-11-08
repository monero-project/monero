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
#include "enote_utils.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers


namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_ram_borrowed_device::view_key_scalar_mult_ed25519(const crypto::public_key &P,
    crypto::public_key &kvP) const
{
    kvP = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(P), rct::sk2rct(m_k_view_incoming)));
    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_ram_borrowed_device::view_key_scalar_mult_x25519(const mx25519_pubkey &D,
    mx25519_pubkey &kvD) const
{
    return make_carrot_uncontextualized_shared_key_receiver(m_k_view_incoming, D, kvD);
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
    crypto::hash &s_sender_receiver_out) const
{
    make_carrot_sender_receiver_secret(to_bytes(m_s_view_balance),
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver_out);
}
//-------------------------------------------------------------------------------------------------------------------
void generate_address_secret_ram_borrowed_device::make_index_extension_generator(
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &address_generator_out) const
{
    make_carrot_index_extension_generator(m_s_generate_address, major_index, minor_index, address_generator_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
