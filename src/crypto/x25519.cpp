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

//paired header
#include "x25519.h"

//local headers
#include "crypto.h"
extern "C"
{
#include "crypto-ops.h"
}

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "crypto"

namespace crypto
{
//-------------------------------------------------------------------------------------------------------------------
x25519_secret_key x25519_secret_key_gen(bool clamp)
{
    x25519_secret_key privkey;
    do
    {
        rand(sizeof(privkey.data), privkey.data);
        if (clamp)
        {
            privkey.data[0] &= 255 - 7;
            privkey.data[31] &= 127;
            privkey.data[31] |= 64;
        }
        else
        {
            sc_reduce32(privkey.data);
        }
    } while (privkey == x25519_secret_key{});

    return privkey;
}
//-------------------------------------------------------------------------------------------------------------------
x25519_pubkey x25519_pubkey_gen()
{
    const x25519_secret_key privkey{x25519_secret_key_gen()};
    x25519_pubkey pubkey;
    x25519_scmul_base_unclamped(privkey, pubkey);

    return pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_scmul_base_unclamped(const x25519_scalar &scalar, x25519_pubkey &result_out)
{
    static const mx25519_impl *impl{mx25519_select_impl(mx25519_type::MX25519_TYPE_AUTO)};
    mx25519_scmul_base_unclamped(impl, &result_out, &scalar, MX25519_UNCLAMP_ALL);
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_scmul_key_unclamped(const x25519_scalar &scalar, const x25519_pubkey &pubkey, x25519_pubkey &result_out)
{
    static const mx25519_impl *impl{mx25519_select_impl(mx25519_type::MX25519_TYPE_AUTO)};
    mx25519_scmul_key_unclamped(impl, &result_out, &scalar, &pubkey, MX25519_UNCLAMP_ALL);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace crypto
