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

//! @file Supporting types for Carrot (anchor, view tag, etc.).

#pragma once

//local headers
#include "mx25519.h"

//third party headers

//standard headers
#include <cstdint>
#include <cstddef>

//forward declarations

namespace carrot
{

constexpr std::size_t JANUS_ANCHOR_BYTES{16};

/// either encodes randomness the private key of, or an HMAC of, the ephemeral pubkey 
struct janus_anchor_t final
{
    unsigned char bytes[JANUS_ANCHOR_BYTES];
};

/// carrot janus anchor XORd with a user-defined secret
using encrypted_janus_anchor_t = janus_anchor_t;

/// carrot enote types
enum class CarrotEnoteType : unsigned char
{
    PAYMENT = 0,
    CHANGE = 1
};

/// carrot encrypted amount
constexpr std::size_t ENCRYPTED_AMOUNT_BYTES{8};
struct encrypted_amount_t final
{
    unsigned char bytes[ENCRYPTED_AMOUNT_BYTES];
};

/// legacy payment ID
constexpr std::size_t PAYMENT_ID_BYTES{8};
struct payment_id_t final
{
    unsigned char bytes[PAYMENT_ID_BYTES];
};
static constexpr payment_id_t null_payment_id{{0}};

/// legacy encrypted payment ID
using encrypted_payment_id_t = payment_id_t;

/// carrot view tags
constexpr std::size_t VIEW_TAG_BYTES{3};
struct view_tag_t final
{
    unsigned char bytes[VIEW_TAG_BYTES];
};

static_assert(sizeof(view_tag_t) < 32, "uint8_t cannot index all view tag bits");

/// carrot input context
constexpr std::size_t INPUT_CONTEXT_BYTES{1 + 32};
struct input_context_t final
{
    unsigned char bytes[INPUT_CONTEXT_BYTES];
};

/// overloaded operators: address tag
bool operator==(const janus_anchor_t &a, const janus_anchor_t &b);
static inline bool operator!=(const janus_anchor_t &a, const janus_anchor_t &b) { return !(a == b); }
janus_anchor_t operator^(const janus_anchor_t &a, const janus_anchor_t &b);

/// overloaded operators: encrypted amount
bool operator==(const encrypted_amount_t &a, const encrypted_amount_t &b);
static inline bool operator!=(const encrypted_amount_t &a, const encrypted_amount_t &b) { return !(a == b); }
encrypted_amount_t operator^(const encrypted_amount_t &a, const encrypted_amount_t &b);

/// overloaded operators: payment ID
bool operator==(const payment_id_t &a, const payment_id_t &b);
static inline bool operator!=(const payment_id_t &a, const payment_id_t &b) { return !(a == b); }
payment_id_t operator^(const payment_id_t &a, const payment_id_t &b);

/// overloaded operators: input context
bool operator==(const input_context_t &a, const input_context_t &b);
static inline bool operator!=(const input_context_t &a, const input_context_t &b) { return !(a == b); }

/// overloaded operators: view tag
bool operator==(const view_tag_t &a, const view_tag_t &b);
static inline bool operator!=(const view_tag_t &a, const view_tag_t &b) { return !(a == b); }

/// generate a random janus anchor
janus_anchor_t gen_janus_anchor();
/// generate a random (non-zero) payment ID
payment_id_t gen_payment_id();
/// generate a random view tag
view_tag_t gen_view_tag();
/// generate a random input context
input_context_t gen_input_context();
/// generate a random X25519 pubkey (unclamped)
mx25519_pubkey gen_x25519_pubkey();

} //namespace carrot
