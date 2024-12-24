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

//paired header
#include "x25519.h"

//local headers
#include "crypto/crypto.h"
extern "C"
{
#include "mx25519.h"
}

//third party headers

//standard headers
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "crypto"

namespace crypto
{

/// File-scope data
static const x25519_scalar X25519_EIGHT{ mx25519_privkey{ .data = { 8 } } };

//-------------------------------------------------------------------------------------------------------------------
x25519_scalar x25519_eight()
{
    return X25519_EIGHT;
}
//-------------------------------------------------------------------------------------------------------------------
x25519_secret_key x25519_secret_key_gen()
{
    x25519_secret_key privkey;
    do
    {
        crypto::rand(32, privkey.data);
        privkey.data[0] &= 255 - 7;
        privkey.data[31] &= 127;
    } while (privkey == x25519_secret_key{});

    return privkey;
}
//-------------------------------------------------------------------------------------------------------------------
x25519_pubkey x25519_pubkey_gen()
{
    const x25519_secret_key privkey{x25519_secret_key_gen()};
    x25519_pubkey pubkey;
    x25519_scmul_base(privkey, pubkey);

    return pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
bool x25519_scalar_is_canonical(const x25519_scalar &test_scalar)
{
    //todo: is this constant time?
    return (test_scalar.data[0] & 7) == 0 &&
        (test_scalar.data[31] & 128) == 0;
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_scmul_base(const x25519_scalar &scalar, x25519_pubkey &result_out)
{
    static const mx25519_impl *impl{mx25519_select_impl(mx25519_type::MX25519_TYPE_AUTO)};
    mx25519_scmul_base(impl, &result_out, &scalar);
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_scmul_key(const x25519_scalar &scalar, const x25519_pubkey &pubkey, x25519_pubkey &result_out)
{
    static const mx25519_impl *impl{mx25519_select_impl(mx25519_type::MX25519_TYPE_AUTO)};
    mx25519_scmul_key(impl, &result_out, &scalar, &pubkey);
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_invmul_key(std::vector<x25519_secret_key> privkeys_to_invert,
    const x25519_pubkey &initial_pubkey,
    x25519_pubkey &result_out)
{
    // 1. (1/({privkey1 * privkey2 * ...}))
    // note: mx25519_invkey() will error if the resulting X25519 scalar is >= 2^255, so we 'search' for a valid solution
    x25519_secret_key inverted_xkey;
    result_out = initial_pubkey;

    while (mx25519_invkey(&inverted_xkey, privkeys_to_invert.data(), privkeys_to_invert.size()) != 0)
    {
        privkeys_to_invert.emplace_back(X25519_EIGHT);  //add 8 to keys to invert
        x25519_scmul_key(X25519_EIGHT, result_out, result_out);  //xK = 8 * xK
    }

    // 2. (1/([8*8*...*8] * {privkey1 * privkey2 * ...})) * [8*8*...*8] * xK
    x25519_scmul_key(inverted_xkey, result_out, result_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace crypto
