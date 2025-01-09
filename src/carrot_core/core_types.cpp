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
#include "core_types.h"

//local headers
#include "crypto/crypto.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}

//third party headers
#include <cstring>

//standard headers

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <std::size_t Sz>
static void xor_bytes(const unsigned char(&a)[Sz], const unsigned char(&b)[Sz], unsigned char(&c_out)[Sz])
{
    for (std::size_t i{0}; i < Sz; ++i)
        c_out[i] = a[i] ^ b[i];
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T>
static T xor_bytes(const T &a, const T &b)
{
    T temp;
    xor_bytes(a.bytes, b.bytes, temp.bytes);
    return temp;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const janus_anchor_t &a, const janus_anchor_t &b)
{
    return memcmp(&a, &b, sizeof(janus_anchor_t)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
janus_anchor_t operator^(const janus_anchor_t &a, const janus_anchor_t &b)
{
    return xor_bytes(a, b);
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const encrypted_amount_t &a, const encrypted_amount_t &b)
{
    return memcmp(&a, &b, sizeof(encrypted_amount_t)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
encrypted_amount_t operator^(const encrypted_amount_t &a, const encrypted_amount_t &b)
{
    return xor_bytes(a, b);
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const payment_id_t &a, const payment_id_t &b)
{
    return memcmp(&a, &b, sizeof(payment_id_t)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
payment_id_t operator^(const payment_id_t &a, const payment_id_t &b)
{
    return xor_bytes(a, b);
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const input_context_t &a, const input_context_t &b)
{
    return memcmp(&a, &b, sizeof(input_context_t)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const view_tag_t &a, const view_tag_t &b)
{
    return memcmp(&a, &b, sizeof(view_tag_t)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
janus_anchor_t gen_janus_anchor()
{
    return crypto::rand<janus_anchor_t>();
}
//-------------------------------------------------------------------------------------------------------------------
payment_id_t gen_payment_id()
{
    return crypto::rand<payment_id_t>();
}
//-------------------------------------------------------------------------------------------------------------------
view_tag_t gen_view_tag()
{
    return crypto::rand<view_tag_t>();
}
//-------------------------------------------------------------------------------------------------------------------
input_context_t gen_input_context()
{
    return crypto::rand<input_context_t>();
}
//-------------------------------------------------------------------------------------------------------------------
mx25519_pubkey gen_x25519_pubkey()
{
    unsigned char sc64[64];
    crypto::rand(sizeof(sc64), sc64);
    sc_reduce(sc64);
    ge_p3 P;
    ge_scalarmult_base(&P, sc64);
    mx25519_pubkey P_x25519;
    ge_p3_to_x25519(P_x25519.data, &P);
    return P_x25519;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
