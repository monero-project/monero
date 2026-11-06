// Copyright (c) 2026, The Monero Project
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
#include "mx25519.h"

//third party headers

//standard headers
#include <cassert>
#include <cstdint>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "crypto"

#if __cplusplus >= 202002L
#include <type_traits>
static_assert(std::is_layout_compatible_v<mx25519_pubkey, crypto::x25519_pubkey>);
static_assert(std::is_layout_compatible_v<mx25519_privkey, crypto::x25519_scalar>);
static_assert(std::is_layout_compatible_v<mx25519_privkey, crypto::x25519_secret_key>);
#endif
static_assert(sizeof(mx25519_pubkey) == sizeof(crypto::x25519_pubkey));
static_assert(sizeof(mx25519_privkey) == sizeof(crypto::x25519_scalar));
static_assert(sizeof(mx25519_privkey) == sizeof(crypto::x25519_secret_key));

namespace
{
const mx25519_impl *get_impl()
{
    static const mx25519_impl *impl = mx25519_select_impl(MX25519_TYPE_AUTO);
    return impl;
}

// https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.c#L17
// In different order
constexpr mx25519_pubkey x25519_small_order_points[7] = {
    /* 0 (order 4) */
    {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* 1 (order 1) */
    {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* 39382357235489614581723060781553021112529911719440698176882885853963445705823
        (order 8) */
    {{ 0x5f, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24, 0xb1, 0xd0, 0xb1,
        0x55, 0x9c, 0x83, 0xef, 0x5b, 0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c,
        0x8e, 0x86, 0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0x57 }},
    /* 325606250916557431795983626356110631294008115727848805560023387167927233504
        (order 8) */
    {{ 0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3,
        0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32,
        0xb1, 0xfd, 0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x00 }},
    /* p-1 (order 2) */
    {{ 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }},
    /* p (=0, order 4) */
    {{ 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }},
    /* p+1 (=1, order 1) */
    {{ 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }}
};
} //anonymous namespace

namespace crypto
{
//-------------------------------------------------------------------------------------------------------------------
x25519_secret_key x25519_secret_key_gen(bool scalar)
{
    x25519_secret_key privkey;
    do
    {
        rand(sizeof(privkey.data), privkey.data);
        if (scalar)
            sc_reduce32(privkey.data);
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
    mx25519_scmul_base_unclamped(get_impl(),
        reinterpret_cast<mx25519_pubkey*>(&result_out),
        reinterpret_cast<const mx25519_privkey*>(&scalar));
}
//-------------------------------------------------------------------------------------------------------------------
void x25519_scmul_key_unclamped(const x25519_scalar &scalar, const x25519_pubkey &pubkey, x25519_pubkey &result_out)
{
    mx25519_scmul_key_unclamped(get_impl(),
        reinterpret_cast<mx25519_pubkey*>(&result_out),
        reinterpret_cast<const mx25519_privkey*>(&scalar),
        reinterpret_cast<const mx25519_pubkey*>(&pubkey));
}
//-------------------------------------------------------------------------------------------------------------------
bool x25519_is_small_order_point(const x25519_pubkey &point)
{
    size_t h = ((static_cast<std::uint32_t>(point.data[0]) * 703) >> 9) & 7;
    h -= (h + 1) >> 3; // map 7 -> 6
    assert(h < sizeof(x25519_small_order_points)/sizeof(x25519_small_order_points[0]));
    // match small point at hash index, insensitive of the MSB
    if (memcmp(point.data, x25519_small_order_points[h].data, sizeof(point) - 1))
        return false;
    return 0 == ((point.data[31] ^ x25519_small_order_points[h].data[31]) & 0x7f);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace crypto
