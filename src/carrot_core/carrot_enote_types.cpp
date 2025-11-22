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

// Utilities for scanning carrot enotes

//paired header
#include "carrot_enote_types.h"

//local headers

//third party headers

//standard headers

/*
 onetime address
// - amount commitment
// - encrypted amount
// - encrypted janus anchor
// - view tag
// - ephemeral pubkey
// - tx first key image*/

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotEnoteV1 &a, const CarrotEnoteV1 &b)
{
    return a.onetime_address    == b.onetime_address    &&
           a.amount_commitment  == b.amount_commitment  &&
           a.amount_enc         == b.amount_enc         &&
           a.anchor_enc         == b.anchor_enc         &&
           a.view_tag           == b.view_tag           &&
           a.tx_first_key_image == b.tx_first_key_image &&
           memcmp(a.enote_ephemeral_pubkey.data, b.enote_ephemeral_pubkey.data, sizeof(mx25519_pubkey)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotCoinbaseEnoteV1 &a, const CarrotCoinbaseEnoteV1 &b)
{
    return a.onetime_address == b.onetime_address &&
           a.amount          == b.amount          &&
           a.anchor_enc      == b.anchor_enc      &&
           a.view_tag        == b.view_tag        &&
           a.block_index     == b.block_index     &&
           memcmp(a.enote_ephemeral_pubkey.data, b.enote_ephemeral_pubkey.data, sizeof(mx25519_pubkey)) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot