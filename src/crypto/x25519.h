// Copyright (c) 2022, The Monero Project
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

// Interface for an x25519 implementation (mx25519).

#pragma once

//local headers
#include "crypto.h"
#include "generic-ops.h"
#include "memwipe.h"
#include "mlocker.h"

//third party headers

//standard headers
#include <vector>

//forward declarations


namespace crypto
{

extern "C"
{
#include "mx25519.h"
}

struct x25519_pubkey : public mx25519_pubkey
{
    x25519_pubkey() = default;
    x25519_pubkey(const mx25519_pubkey &other) { *this = other; }
    x25519_pubkey& operator=(const mx25519_pubkey &other) { memcpy(data, other.data, 32); return *this; }
};
struct x25519_scalar : public mx25519_privkey
{
    x25519_scalar() = default;
    x25519_scalar(const mx25519_privkey &other) { *this = other; }
    x25519_scalar& operator=(const mx25519_privkey &other) { memcpy(data, other.data, 32); return *this; }
};
struct x25519_secret_key : public epee::mlocked<tools::scrubbed<x25519_scalar>>
{
    x25519_secret_key() = default;
    x25519_secret_key(const x25519_scalar &other) { *this = other; }
    x25519_secret_key& operator=(const x25519_scalar &other) { memcpy(data, other.data, 32); return *this; }
};

/**
* brief: x25519_eight - scalar 8
* return: scalar 8
*/
x25519_scalar x25519_eight();
/**
* brief: x25519_secret_key_gen - generate a random x25519 privkey
* return: random canonical x25519 privkey
*/
x25519_secret_key x25519_secret_key_gen();
/**
* brief: x25519_pubkey_gen - generate a random x25519 pubkey
* return: random x25519 pubkey
*/
x25519_pubkey x25519_pubkey_gen();
/**
* brief: x25519_scalar_is_canonical - check that an X25519 scalar is canonical
*   - expect: 2^255 > scalar >= 8 (i.e. last bit and first three bits not set)
* result: true if input scalar is canonical
*/
bool x25519_scalar_is_canonical(const x25519_scalar &test_scalar);
/**
* brief: x25519_scmul_base - compute scalar * xG
* param: scalar - scalar to multiply
* result: scalar * xG
*/
void x25519_scmul_base(const x25519_scalar &scalar, x25519_pubkey &result_out);
/**
* brief: x25519_scmul_key - compute scalar * pubkey
* param: scalar - scalar to multiply
* param: pubkey - public key to multiple against
* result: scalar * pubkey
*/
void x25519_scmul_key(const x25519_scalar &scalar, const x25519_pubkey &pubkey, x25519_pubkey &result_out);
/**
* brief: x25519_invmul_key - compute (1/({privkey1 * privkey2 * ...})) * initial_pubkey
* param: privkeys_to_invert - {privkey1, privkey2, ...}
* param: initial_pubkey - base key for inversion
* result: (1/({privkey1 * privkey2 * ...})) * initial_pubkey
*/
void x25519_invmul_key(std::vector<x25519_secret_key> privkeys_to_invert,
    const x25519_pubkey &initial_pubkey,
    x25519_pubkey &result_out);

} //namespace crypto

inline const unsigned char* to_bytes(const crypto::x25519_pubkey &point) { return &reinterpret_cast<const unsigned char&>(point); }
inline const unsigned char* to_bytes(const crypto::x25519_scalar &scalar) { return &reinterpret_cast<const unsigned char&>(scalar); }
inline const unsigned char* to_bytes(const crypto::x25519_secret_key &skey) { return &reinterpret_cast<const unsigned char&>(skey); }

/// upgrade x25519 keys
CRYPTO_MAKE_HASHABLE(x25519_pubkey)
CRYPTO_MAKE_HASHABLE_CONSTANT_TIME(x25519_scalar)
CRYPTO_MAKE_HASHABLE_CONSTANT_TIME(x25519_secret_key)
