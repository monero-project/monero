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

// Seraphis core types.

#pragma once

//local headers
#include "core_types.h"
#include "mx25519.h"
#include "ringct/rctTypes.h"

//third party headers

//standard headers

//forward declarations


namespace carrot
{

////
// CarrotEnoteV1
// - onetime address
// - amount commitment
// - encrypted amount
// - encrypted janus anchor
// - view tag
// - ephemeral pubkey
// - tx first key image
///
struct CarrotEnoteV1 final
{
    /// K_o
    crypto::public_key onetime_address;
    /// C_a
    rct::key amount_commitment;
    /// a_enc
    encrypted_amount_t amount_enc;
    /// anchor_enc
    encrypted_janus_anchor_t anchor_enc;
    /// view_tag
    view_tag_t view_tag;
    /// D_e
    mx25519_pubkey enote_ephemeral_pubkey;
    /// L_0
    crypto::key_image tx_first_key_image;
};

/// equality operators
bool operator==(const CarrotEnoteV1 &a, const CarrotEnoteV1 &b);
static inline bool operator!=(const CarrotEnoteV1 &a, const CarrotEnoteV1 &b) { return !(a == b); }

////
// CarrotCoinbaseEnoteV1
// - onetime address
// - cleartext amount
// - encrypted janus anchor
// - view tag
// - ephemeral pubkey
// - block index
///
struct CarrotCoinbaseEnoteV1 final
{
    /// K_o
    crypto::public_key onetime_address;
    /// a
    rct::xmr_amount amount;
    /// anchor_enc
    encrypted_janus_anchor_t anchor_enc;
    /// view_tag
    view_tag_t view_tag;
    /// D_e
    mx25519_pubkey enote_ephemeral_pubkey;
    /// block_index
    std::uint64_t block_index;
};

/// equality operators
bool operator==(const CarrotCoinbaseEnoteV1 &a, const CarrotCoinbaseEnoteV1 &b);
static inline bool operator!=(const CarrotCoinbaseEnoteV1 &a, const CarrotCoinbaseEnoteV1 &b) { return !(a == b); }

} //namespace carrot
