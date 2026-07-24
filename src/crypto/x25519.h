// Copyright (c) 2022-2026, The Monero Project
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

//! @file Interface for an x25519 implementation (mx25519).

#pragma once

//local headers
#include "generic-ops.h"
#include "memwipe.h"
#include "mlocker.h"

//third party headers

//standard headers

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
    x25519_pubkey(const mx25519_pubkey &other): mx25519_pubkey(other) {}
    x25519_pubkey& operator=(const mx25519_pubkey &other)
    { static_cast<mx25519_pubkey&>(*this) = other ; return *this; }
};
struct x25519_scalar : public mx25519_privkey
{
    x25519_scalar() = default;
    x25519_scalar(const mx25519_privkey &other): mx25519_privkey(other) {}
    x25519_scalar& operator=(const mx25519_privkey &other)
    { static_cast<mx25519_privkey&>(*this) = other ; return *this; }
};
struct x25519_secret_key : public epee::mlocked<tools::scrubbed<x25519_scalar>>
{
    x25519_secret_key() = default;
    x25519_secret_key(const x25519_scalar &other) { *this = other; }
    x25519_secret_key& operator=(const x25519_scalar &other)
    { unwrap(unwrap(*this)) = other; return *this; }
};

/**
 * @brief Generate a random X25519 privkey
 * @param clamp true to clamp uniform key (see RFC 7748), false to sample element of Field25519
 * @return random X25519 privkey
 */
x25519_secret_key x25519_secret_key_gen(bool clamp = true);
/**
 * @brief Generate a random X25519 pubkey
 * @return random X25519 pubkey
 */
x25519_pubkey x25519_pubkey_gen();
/**
 * @brief Compute scalar * xG [Unclamped, does not conform to RFC 7748]
 * @param scalar scalar to multiply
 * @param[out] result_out scalar * xG
 */
void x25519_scmul_base_unclamped(const x25519_scalar &scalar, x25519_pubkey &result_out);
/**
 * @brief Compute scalar * pubkey [Unclamped, does not conform to RFC 7748]
 * @param scalar scalar to multiply
 * @param pubkey public key to multiply against
 * @param[out] result_out scalar * pubkey
 */
void x25519_scmul_key_unclamped(const x25519_scalar &scalar, const x25519_pubkey &pubkey, x25519_pubkey &result_out);

} //namespace crypto

/// upgrade x25519 keys
CRYPTO_MAKE_HASHABLE(x25519_pubkey)
CRYPTO_MAKE_HASHABLE_CONSTANT_TIME(x25519_scalar)
CRYPTO_MAKE_HASHABLE_CONSTANT_TIME(x25519_secret_key)
