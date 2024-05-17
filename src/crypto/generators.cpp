// Copyright (c) 2022-2024, The Monero Project
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

#include "generators.h"

#include "crypto.h"
extern "C"
{
#include "crypto-ops.h"
}
#include "hash.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

namespace crypto
{

/// constexpr assert for old gcc bug: https://stackoverflow.com/questions/34280729/throw-in-constexpr-function
/// - this function won't compile in a constexpr context if b == false
constexpr void constexpr_assert(const bool b) { b ? 0 : throw std::runtime_error("constexpr assert failed"); };

/// constexpr paste bytes into an array-of-bytes type
template<typename T>
constexpr T bytes_to(const std::initializer_list<unsigned char> bytes)
{
    T out{}; // zero-initialize trailing bytes

    auto current = std::begin(out.data);
    constexpr_assert(static_cast<long>(bytes.size()) <= std::end(out.data) - current);

    for (const unsigned char byte : bytes)
        *current++ = byte;
    return out;
}

// generators
//standard ed25519 generator G: {x, 4/5} (positive x when decompressing y = 4/5)
constexpr public_key G = bytes_to<public_key>({ 0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 });
//pedersen commitment generator H: toPoint(cn_fast_hash(G))
constexpr public_key H = bytes_to<public_key>({ 0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1,
    0xad, 0xd0, 0xea, 0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94 });
//FCMP++ generator T: keccak_to_pt(keccak("Monero Generator T"))
constexpr public_key T = bytes_to<public_key>({ 0x96, 0x6f, 0xc6, 0x6b, 0x82, 0xcd, 0x56, 0xcf, 0x85, 0xea, 0xec, 0x80, 0x1c,
    0x42, 0x84, 0x5f, 0x5f, 0x40, 0x88, 0x78, 0xd1, 0x56, 0x1e, 0x00, 0xd3, 0xd7, 0xde, 0xd2, 0x79, 0x4d, 0x09, 0x4f });
//FCMP++ generator U: keccak_to_pt(keccak("Monero FCMP++ Generator U"))
constexpr public_key U = bytes_to<public_key>({ 0x09, 0x75, 0x9c, 0x17, 0xc9, 0x07, 0xf7, 0x16, 0xa2, 0x0b, 0x1a, 0xec, 0x5c,
    0xc3, 0xaf, 0xfd, 0xe7, 0xf3, 0xa1, 0xb9, 0x14, 0x6b, 0x5a, 0xf2, 0x8c, 0xb7, 0xaf, 0x0a, 0xf4, 0x7a, 0x00, 0x66 });
//FCMP++ generator V: keccak_to_pt(keccak("Monero FCMP++ Generator V"))
constexpr public_key V = bytes_to<public_key>({ 0x32, 0xb4, 0xd2, 0x9f, 0x2a, 0x80, 0x55, 0x69, 0xd9, 0x59, 0xd2, 0x44, 0x96,
    0xed, 0x41, 0x1e, 0x87, 0x91, 0x26, 0xd8, 0xf5, 0x2c, 0x1e, 0xcd, 0x86, 0x4d, 0xb9, 0x02, 0xb5, 0x81, 0x33, 0xe0 });
static ge_p3 G_p3;
static ge_p3 H_p3;
static ge_p3 T_p3;
static ge_p3 U_p3;
static ge_p3 V_p3;
static ge_cached G_cached;
static ge_cached H_cached;
static ge_cached T_cached;
static ge_cached U_cached;
static ge_cached V_cached;

// misc
static std::once_flag init_gens_once_flag;

//-------------------------------------------------------------------------------------------------------------------
// hash-to-point: H_p(x) = 8*point_from_bytes(keccak(x))
//-------------------------------------------------------------------------------------------------------------------
static void hash_to_point(const hash &x, crypto::ec_point &point_out)
{
    hash h;
    ge_p3 temp_p3;
    ge_p2 temp_p2;
    ge_p1p1 temp_p1p1;

    cn_fast_hash(reinterpret_cast<const unsigned char*>(&x), sizeof(hash), h);
    ge_fromfe_frombytes_vartime(&temp_p2, reinterpret_cast<const unsigned char*>(&h));
    ge_mul8(&temp_p1p1, &temp_p2);
    ge_p1p1_to_p3(&temp_p3, &temp_p1p1);
    ge_p3_tobytes(to_bytes(point_out), &temp_p3);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static public_key reproduce_generator_G()
{
    // G = {x, 4/5 mod q}
    fe four, five, inv_five, y;
    fe_0(four);
    fe_0(five);
    four[0] = 4;
    five[0] = 5;
    fe_invert(inv_five, five);
    fe_mul(y, four, inv_five);

    public_key reproduced_G;
    fe_tobytes(to_bytes(reproduced_G), y);

    return reproduced_G;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static public_key reproduce_generator_H()
{
    // H = 8*to_point(keccak(G))
    // note: this does not use the point_from_bytes() function found in H_p(), instead directly interpreting the
    //       input bytes as a compressed point (this can fail, so should not be used generically)
    // note2: to_point(keccak(G)) is known to succeed for the canonical value of G (it will fail 7/8ths of the time
    //        normally)
    ge_p3 temp_p3;
    ge_p2 temp_p2;
    ge_p1p1 temp_p1p1;

    hash H_temp_hash{cn_fast_hash(to_bytes(G), sizeof(ec_point))};
    (void)H_temp_hash;  //suppress unused warning
    assert(ge_frombytes_vartime(&temp_p3, reinterpret_cast<const unsigned char*>(&H_temp_hash)) == 0);
    ge_p3_to_p2(&temp_p2, &temp_p3);
    ge_mul8(&temp_p1p1, &temp_p2);
    ge_p1p1_to_p3(&temp_p3, &temp_p1p1);

    public_key reproduced_H;
    ge_p3_tobytes(to_bytes(reproduced_H), &temp_p3);

    return reproduced_H;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static public_key reproduce_generator_T()
{
    // T = H_p(keccak("Monero Generator T"))
    const std::string_view T_seed{"Monero Generator T"};
    const hash T_temp_hash{cn_fast_hash(T_seed.data(), T_seed.size())};
    public_key reproduced_T;
    hash_to_point(T_temp_hash, reproduced_T);

    return reproduced_T;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static public_key reproduce_generator_U()
{
    // U = H_p(keccak("Monero FCMP++ Generator U"))
    const std::string_view U_seed{"Monero FCMP++ Generator U"};
    const hash U_temp_hash{cn_fast_hash(U_seed.data(), U_seed.size())};
    public_key reproduced_U;
    hash_to_point(U_temp_hash, reproduced_U);

    return reproduced_U;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static public_key reproduce_generator_V()
{
    // V = H_p(keccak("Monero FCMP++ Generator V"))
    const std::string_view V_seed{"Monero FCMP++ Generator V"};
    const hash V_temp_hash{cn_fast_hash(V_seed.data(), V_seed.size())};
    public_key reproduced_V;
    hash_to_point(V_temp_hash, reproduced_V);

    return reproduced_V;
}
//-------------------------------------------------------------------------------------------------------------------
// Make generators, but only once
//-------------------------------------------------------------------------------------------------------------------
static void init_gens()
{
    std::call_once(init_gens_once_flag,
        [&](){

        // sanity check the generators
        static_assert(static_cast<unsigned char>(G.data[0]) == 0x58, "compile-time constant sanity check");
        static_assert(static_cast<unsigned char>(H.data[0]) == 0x8b, "compile-time constant sanity check");
        static_assert(static_cast<unsigned char>(T.data[0]) == 0x96, "compile-time constant sanity check");
        static_assert(static_cast<unsigned char>(U.data[0]) == 0x09, "compile-time constant sanity check");
        static_assert(static_cast<unsigned char>(V.data[0]) == 0x32, "compile-time constant sanity check");

        // build ge_p3 representations of generators
        const int G_deserialize = ge_frombytes_vartime(&G_p3, to_bytes(G));
        const int H_deserialize = ge_frombytes_vartime(&H_p3, to_bytes(H));
        const int T_deserialize = ge_frombytes_vartime(&T_p3, to_bytes(T));
        const int U_deserialize = ge_frombytes_vartime(&U_p3, to_bytes(U));
        const int V_deserialize = ge_frombytes_vartime(&V_p3, to_bytes(V));

        (void)G_deserialize; assert(G_deserialize == 0);
        (void)H_deserialize; assert(H_deserialize == 0);
        (void)T_deserialize; assert(T_deserialize == 0);
        (void)U_deserialize; assert(U_deserialize == 0);
        (void)V_deserialize; assert(V_deserialize == 0);

        // get cached versions
        ge_p3_to_cached(&G_cached, &G_p3);
        ge_p3_to_cached(&H_cached, &H_p3);
        ge_p3_to_cached(&T_cached, &T_p3);
        ge_p3_to_cached(&U_cached, &U_p3);
        ge_p3_to_cached(&V_cached, &V_p3);

        // in debug mode, check that generators are reproducible
        (void)reproduce_generator_G; assert(reproduce_generator_G() == G);
        (void)reproduce_generator_H; assert(reproduce_generator_H() == H);
        (void)reproduce_generator_T; assert(reproduce_generator_T() == T);
        (void)reproduce_generator_U; assert(reproduce_generator_U() == U);
        (void)reproduce_generator_V; assert(reproduce_generator_V() == V);

    });
}
//-------------------------------------------------------------------------------------------------------------------
public_key get_G()
{
    return G;
}
//-------------------------------------------------------------------------------------------------------------------
public_key get_H()
{
    return H;
}
//-------------------------------------------------------------------------------------------------------------------
public_key get_T()
{
    return T;
}
//-------------------------------------------------------------------------------------------------------------------
public_key get_U()
{
    return U;
}
//-------------------------------------------------------------------------------------------------------------------
public_key get_V()
{
    return V;
}
//-------------------------------------------------------------------------------------------------------------------
ge_p3 get_G_p3()
{
    init_gens();
    return G_p3;
}
//-------------------------------------------------------------------------------------------------------------------
ge_p3 get_H_p3()
{
    init_gens();
    return H_p3;
}
//-------------------------------------------------------------------------------------------------------------------
ge_p3 get_T_p3()
{
    init_gens();
    return T_p3;
}
//-------------------------------------------------------------------------------------------------------------------
ge_p3 get_U_p3()
{
    init_gens();
    return U_p3;
}
//-------------------------------------------------------------------------------------------------------------------
ge_p3 get_V_p3()
{
    init_gens();
    return V_p3;
}
//-------------------------------------------------------------------------------------------------------------------
ge_cached get_G_cached()
{
    init_gens();
    return G_cached;
}
//-------------------------------------------------------------------------------------------------------------------
ge_cached get_H_cached()
{
    init_gens();
    return H_cached;
}
//-------------------------------------------------------------------------------------------------------------------
ge_cached get_T_cached()
{
    init_gens();
    return T_cached;
}
//-------------------------------------------------------------------------------------------------------------------
ge_cached get_U_cached()
{
    init_gens();
    return U_cached;
}
//-------------------------------------------------------------------------------------------------------------------
ge_cached get_V_cached()
{
    init_gens();
    return V_cached;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace crypto
