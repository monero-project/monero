//#define DBG
// Copyright (c) 2016-2024, Monero Research Labs
//
// Author: Shen Noether <shen.noether@gmx.com>
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

#pragma once

#ifndef RCTOPS_H
#define RCTOPS_H

#include <cstddef>
#include <tuple>

#include "crypto/generic-ops.h"

extern "C" {
#include "crypto/random.h"
#include "crypto/keccak.h"
#include "rctCryptoOps.h"
}
#include "crypto/crypto.h"

#include "rctTypes.h"

//Define this flag when debugging to get additional info on the console
#ifdef DBG
#define DP(x) dp(x)
#else
#define DP(x)
#endif

namespace rct {

    //Various key initialization functions

    static const key Z = { {0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
    static const key I = { {0x01, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
    static const key L = { {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 } };
    static const key G = { {0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 } };
    static const key EIGHT = { {0x08, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
    static const key INV_EIGHT = { { 0x79, 0x2f, 0xdc, 0xe2, 0x29, 0xe5, 0x06, 0x61, 0xd0, 0xda, 0x1c, 0x7d, 0xb3, 0x9d, 0xd3, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06 } };

    //Creates a zero scalar
    inline key zero() { return Z; }
    inline void zero(key &z) { memset(&z, 0, 32); }
    //Creates a zero elliptic curve point
    inline key identity() { return I; }
    inline void identity(key &Id) { memcpy(&Id, &I, 32); }
    //Creates a key equal to the curve order
    inline key curveOrder() { return L; }
    inline void curveOrder(key &l) { l = L; }
    //copies a scalar or point
    inline void copy(key &AA, const key &A) { memcpy(&AA, &A, 32); }
    inline key copy(const key & A) { key AA; memcpy(&AA, &A, 32); return AA; }

    //initializes a key matrix;
    //first parameter is rows, 
    //second is columns
    keyM keyMInit(size_t rows, size_t cols);

    //Various key generation functions        
    bool toPointCheckOrder(ge_p3 *P, const unsigned char *data);

    //generates a random scalar which can be used as a secret key or mask
    key skGen();
    void skGen(key &);
    
    //generates a vector of secret keys of size "int"
    keyV skvGen(size_t rows );
    
    //generates a random curve point (for testing)
    key pkGen();
    //generates a random secret and corresponding public key
    void skpkGen(key &sk, key &pk);
    std::tuple<key, key> skpkGen();
    //generates a <secret , public> / Pedersen commitment to the amount
    std::tuple<ctkey, ctkey> ctskpkGen(xmr_amount amount);
    //generates C =aG + bH from b, a is random
    void genC(key & C, const key & a, xmr_amount amount);
    //this one is mainly for testing, can take arbitrary amounts..
    std::tuple<ctkey, ctkey> ctskpkGen(const key &bH);
    // make a pedersen commitment with given key
    key commit(xmr_amount amount, const key &mask);
    // make a pedersen commitment with zero key
    key zeroCommit(xmr_amount amount);
    //generates a random uint long long
    xmr_amount randXmrAmount(xmr_amount upperlimit);

    //Scalar multiplications of curve points        

    //does a * G where a is a scalar and G is the curve basepoint
    void scalarmultBase(key & aG, const key &a);
    key scalarmultBase(const key & a);
    //does a * P where a is a scalar and P is an arbitrary point
    void scalarmultKey(key &aP, const key &P, const key &a);
    key scalarmultKey(const key &P, const key &a);
    //Computes aH where H= toPoint(cn_fast_hash(G)), G the basepoint
    key scalarmultH(const key & a);
    // multiplies a point by 8
    key scalarmult8(const key & P);
    void scalarmult8(ge_p3 &res, const key & P);
    // checks a is in the main subgroup (ie, not a small one)
    bool isInMainSubgroup(const key & a);

    //Curve addition / subtractions

    //for curve points: AB = A + B
    void addKeys(key &AB, const key &A, const key &B);
    rct::key addKeys(const key &A, const key &B);
    rct::key addKeys(const keyV &A);
    //aGB = aG + B where a is a scalar, G is the basepoint, and B is a point
    void addKeys1(key &aGB, const key &a, const key & B);
    //aGbB = aG + bB where a, b are scalars, G is the basepoint and B is a point
    void addKeys2(key &aGbB, const key &a, const key &b, const key &B);
    //Does some precomputation to make addKeys3 more efficient
    // input B a curve point and output a ge_dsmp which has precomputation applied
    void precomp(ge_dsmp rv, const key &B);
    //aAbB = a*A + b*B where a, b are scalars, A, B are curve points
    //B must be input after applying "precomp"
    void addKeys3(key &aAbB, const key &a, const key &A, const key &b, const ge_dsmp B);
    void addKeys3(key &aAbB, const key &a, const ge_dsmp A, const key &b, const ge_dsmp B);

    void addKeys_aGbBcC(key &aGbBcC, const key &a, const key &b, const ge_dsmp B, const key &c, const ge_dsmp C);
    void addKeys_aAbBcC(key &aAbBcC, const key &a, const ge_dsmp A, const key &b, const ge_dsmp B, const key &c, const ge_dsmp C);

    //AB = A - B where A, B are curve points
    void subKeys(key &AB, const key &A, const  key &B);
    //checks if A, B are equal as curve points
    bool equalKeys(const key & A, const key & B);

    //Hashing - cn_fast_hash
    //be careful these are also in crypto namespace
    //cn_fast_hash for arbitrary l multiples of 32 bytes 
    void cn_fast_hash(key &hash, const void * data, const size_t l);
    void hash_to_scalar(key &hash, const void * data, const size_t l);
    //cn_fast_hash for a 32 byte key
    void cn_fast_hash(key &hash, const key &in);
    void hash_to_scalar(key &hash, const key &in);
    //cn_fast_hash for a 32 byte key
    key cn_fast_hash(const key &in);
    key hash_to_scalar(const key &in);
    //for mg sigs
    key cn_fast_hash128(const void * in);
    key hash_to_scalar128(const void * in);
    key cn_fast_hash(const ctkeyV &PC);
    key hash_to_scalar(const ctkeyV &PC);
    //for mg sigs 
    key cn_fast_hash(const keyV &keys);
    key hash_to_scalar(const keyV &keys);
    //for ANSL
    key cn_fast_hash(const key64 keys);
    key hash_to_scalar(const key64 keys);

    void hash_to_p3(ge_p3 &hash8_p3, const key &k);

    //sums a vector of curve points (for scalars use sc_add)
    void sumKeys(key & Csum, const key &Cis);

    //Elliptic Curve Diffie Helman: encodes and decodes the amount b and mask a
    // where C= aG + bH
    key genAmountEncodingFactor(const key &k);
    key genCommitmentMask(const key &sk);
    void ecdhEncode(ecdhTuple & unmasked, const key & sharedSec, bool v2);
    void ecdhDecode(ecdhTuple & masked, const key & sharedSec, bool v2);
}
#endif  /* RCTOPS_H */
