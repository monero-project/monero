// Copyright (c) 2016, Monero Research Labs
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

#include "rctOps.h"
using namespace crypto;
using namespace std;

namespace rct {

    //Various key initialization functions

    //Creates a zero scalar
    void zero(key &zero) {
        int i = 0;
        for (i = 0; i < 32; i++) {
            zero[i] = (unsigned char)(0x00);
        }
    }

    //Creates a zero scalar
    key zero() {
        return{ {0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
    }

    //Creates a zero elliptic curve point
    void identity(key &Id) {
        int i = 0;
        Id[0] = (unsigned char)(0x01);
        for (i = 1; i < 32; i++) {
            Id[i] = (unsigned char)(0x00);
        }
    }

    //Creates a zero elliptic curve point
    key identity() {
        key Id;
        int i = 0;
        Id[0] = (unsigned char)(0x01);
        for (i = 1; i < 32; i++) {
            Id[i] = (unsigned char)(0x00);
        }
        return Id;
    }

    //copies a scalar or point
    void copy(key &AA, const key &A) {
        int i = 0;
        for (i = 0; i < 32; i++) {
            AA[i] = A.bytes[i];
        }
    }

    //copies a scalar or point
    key copy(const key &A) {
        int i = 0;
        key AA;
        for (i = 0; i < 32; i++) {
            AA[i] = A.bytes[i];
        }
        return AA;
    }


    //initializes a key matrix;
    //first parameter is rows,
    //second is columns
    keyM keyMInit(int rows, int cols) {
        keyM rv(cols);
        int i = 0;
        for (i = 0 ; i < cols ; i++) {
            rv[i] = keyV(rows);
        }
        return rv;
    }




    //Various key generation functions

    //generates a random scalar which can be used as a secret key or mask
    void skGen(key &sk) {
        unsigned char tmp[64];
        rand(64, tmp);
        memcpy(sk.bytes, tmp, 32);
        sc_reduce32(sk.bytes);
    }

    //generates a random scalar which can be used as a secret key or mask
    key skGen() {
        unsigned char tmp[64];
        rand(64, tmp);
        key sk;
        memcpy(sk.bytes, tmp, 32);
        sc_reduce32(sk.bytes);
        return sk;
    }

    //Generates a vector of secret key
    //Mainly used in testing
    keyV skvGen(int rows ) {
        keyV rv(rows);
        int i = 0;
        for (i = 0 ; i < rows ; i++) {
            skGen(rv[i]);
        }
        return rv;
    }

    //generates a random curve point (for testing)
    key  pkGen() {
        key sk = skGen();
        key pk = scalarmultBase(sk);
        return pk;
    }

    //generates a random secret and corresponding public key
    void skpkGen(key &sk, key &pk) {
        skGen(sk);
        scalarmultBase(pk, sk);
    }

    //generates a random secret and corresponding public key
    tuple<key, key>  skpkGen() {
        key sk = skGen();
        key pk = scalarmultBase(sk);
        return make_tuple(sk, pk);
    }

    //generates a <secret , public> / Pedersen commitment to the amount
    tuple<ctkey, ctkey> ctskpkGen(xmr_amount amount) {
        ctkey sk, pk;
        skpkGen(sk.dest, pk.dest);
        skpkGen(sk.mask, pk.mask);
        key am = d2h(amount);
        key bH = scalarmultH(am);
        addKeys(pk.mask, pk.mask, bH);
        return make_tuple(sk, pk);
    }
    
    
    //generates a <secret , public> / Pedersen commitment but takes bH as input 
    tuple<ctkey, ctkey> ctskpkGen(key bH) {
        ctkey sk, pk;
        skpkGen(sk.dest, pk.dest);
        skpkGen(sk.mask, pk.mask);
        addKeys(pk.mask, pk.mask, bH);
        return make_tuple(sk, pk);
    }
    
    //generates a random uint long long (for testing)
    xmr_amount randXmrAmount(xmr_amount upperlimit) {
        return h2d(skGen()) % (upperlimit);
    }

    //Scalar multiplications of curve points

    //does a * G where a is a scalar and G is the curve basepoint
    void scalarmultBase(key &aG,const key &a) {
        ge_p3 point;
        sc_reduce32copy(aG.bytes, a.bytes); //do this beforehand!
        ge_scalarmult_base(&point, aG.bytes);
        ge_p3_tobytes(aG.bytes, &point);
    }

    //does a * G where a is a scalar and G is the curve basepoint
    key scalarmultBase(const key & a) {
        ge_p3 point;
        key aG;
        sc_reduce32copy(aG.bytes, a.bytes); //do this beforehand
        ge_scalarmult_base(&point, aG.bytes);
        ge_p3_tobytes(aG.bytes, &point);
        return aG;
    }

    //does a * P where a is a scalar and P is an arbitrary point
    void scalarmultKey(key & aP, const key &P, const key &a) {
        ge_p3 A;
        ge_p2 R;
        ge_frombytes_vartime(&A, P.bytes);
        ge_scalarmult(&R, a.bytes, &A);
        ge_tobytes(aP.bytes, &R);
    }

    //does a * P where a is a scalar and P is an arbitrary point
    key scalarmultKey(const key & P, const key & a) {
        ge_p3 A;
        ge_p2 R;
        ge_frombytes_vartime(&A, P.bytes);
        ge_scalarmult(&R, a.bytes, &A);
        key aP;
        ge_tobytes(aP.bytes, &R);
        return aP;
    }


    //Computes aH where H= toPoint(cn_fast_hash(G)), G the basepoint
    key scalarmultH(const key & a) {
        ge_p3 A;
        ge_p2 R;
        key Htmp = { {0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1, 0xad, 0xd0, 0xea, 0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94} };
        ge_frombytes_vartime(&A, Htmp.bytes);
        ge_scalarmult(&R, a.bytes, &A);
        key aP;
        ge_tobytes(aP.bytes, &R);
        return aP;
    }

    //Curve addition / subtractions

    //for curve points: AB = A + B
    void addKeys(key &AB, const key &A, const key &B) {
        ge_p3 B2, A2;
        ge_frombytes_vartime(&B2, B.bytes);
        ge_frombytes_vartime(&A2, A.bytes);
        ge_cached tmp2;
        ge_p3_to_cached(&tmp2, &B2);
        ge_p1p1 tmp3;
        ge_add(&tmp3, &A2, &tmp2);
        ge_p1p1_to_p3(&A2, &tmp3);
        ge_p3_tobytes(AB.bytes, &A2);
    }


    //addKeys1
    //aGB = aG + B where a is a scalar, G is the basepoint, and B is a point
    void addKeys1(key &aGB, const key &a, const key & B) {
        key aG = scalarmultBase(a);
        addKeys(aGB, aG, B);
    }

    //addKeys2
    //aGbB = aG + bB where a, b are scalars, G is the basepoint and B is a point
    void addKeys2(key &aGbB, const key &a, const key &b, const key & B) {
        ge_p2 rv;
        ge_p3 B2;
        ge_frombytes_vartime(&B2, B.bytes);
        ge_double_scalarmult_base_vartime(&rv, b.bytes, &B2, a.bytes);
        ge_tobytes(aGbB.bytes, &rv);
    }

    //Does some precomputation to make addKeys3 more efficient
    // input B a curve point and output a ge_dsmp which has precomputation applied
    void precomp(ge_dsmp rv, const key & B) {
        ge_p3 B2;
        ge_frombytes_vartime(&B2, B.bytes);
        ge_dsm_precomp(rv, &B2);
    }

    //addKeys3
    //aAbB = a*A + b*B where a, b are scalars, A, B are curve points
    //B must be input after applying "precomp"
    void addKeys3(key &aAbB, const key &a, const key &A, const key &b, const ge_dsmp B) {
        ge_p2 rv;
        ge_p3 A2;
        ge_frombytes_vartime(&A2, A.bytes);
        ge_double_scalarmult_precomp_vartime(&rv, a.bytes, &A2, b.bytes, B);
        ge_tobytes(aAbB.bytes, &rv);
    }


    //subtract Keys (subtracts curve points)
    //AB = A - B where A, B are curve points
    void subKeys(key & AB, const key &A, const key &B) {
        ge_p3 B2, A2;
        ge_frombytes_vartime(&B2, B.bytes);
        ge_frombytes_vartime(&A2, A.bytes);
        ge_cached tmp2;
        ge_p3_to_cached(&tmp2, &B2);
        ge_p1p1 tmp3;
        ge_sub(&tmp3, &A2, &tmp2);
        ge_p1p1_to_p3(&A2, &tmp3);
        ge_p3_tobytes(AB.bytes, &A2);
    }

    //checks if A, B are equal as curve points
    //without doing curve operations
    bool equalKeys(const key & a, const key & b) {
        bool rv = true;
        for (int i = 0; i < 32; ++i) {
          if (a.bytes[i] != b.bytes[i]) {
            rv = false;
          }
        }
        return rv;
    }

    //Hashing - cn_fast_hash
    //be careful these are also in crypto namespace
    //cn_fast_hash for arbitrary multiples of 32 bytes
    void cn_fast_hash(key &hash, const void * data, const std::size_t l) {
        uint8_t md2[32];
        int j = 0;
        keccak((uint8_t *)data, l, md2, 32);
        for (j = 0; j < 32; j++) {
            hash[j] = (unsigned char)md2[j];
        }
    }
    
    void hash_to_scalar(key &hash, const void * data, const std::size_t l) {
        cn_fast_hash(hash, data, l);
        sc_reduce32(hash.bytes);
    }

    //cn_fast_hash for a 32 byte key
    void cn_fast_hash(key & hash, const key & in) {
        uint8_t md2[32];
        int j = 0;
        keccak((uint8_t *)in.bytes, 32, md2, 32);
        for (j = 0; j < 32; j++) {
            hash[j] = (unsigned char)md2[j];
        }
    }
    
    void hash_to_scalar(key & hash, const key & in) {
        cn_fast_hash(hash, in);
        sc_reduce32(hash.bytes);
    }

    //cn_fast_hash for a 32 byte key
    key cn_fast_hash(const key & in) {
        uint8_t md2[32];
        int j = 0;
        key hash;
        keccak((uint8_t *)in.bytes, 32, md2, 32);
        for (j = 0; j < 32; j++) {
            hash[j] = (unsigned char)md2[j];
        }
        return hash;
    }
    
     key hash_to_scalar(const key & in) {
        key hash = cn_fast_hash(in);
        sc_reduce32(hash.bytes);
        return hash;
     }
    
    //cn_fast_hash for a 128 byte unsigned char
    key cn_fast_hash128(const void * in) {
        uint8_t md2[32];
        int j = 0;
        key hash;
        keccak((uint8_t *)in, 128, md2, 32);
        for (j = 0; j < 32; j++) {
            hash[j] = (unsigned char)md2[j];
        }
        return hash;
    }
    
    key hash_to_scalar128(const void * in) {
        key hash = cn_fast_hash128(in);
        sc_reduce32(hash.bytes);
        return hash;
    }
    
    //cn_fast_hash for multisig purpose
    //This takes the outputs and commitments
    //and hashes them into a 32 byte sized key
    key cn_fast_hash(ctkeyV PC) {
        key rv = identity();
        std::size_t l = (std::size_t)PC.size();
        size_t i = 0, j = 0;
        vector<char> m(l * 64);
        for (i = 0 ; i < l ; i++) {
            for (j = 0 ; j < 32 ; j++) {
                m[i * 64 + j] = PC[i].dest[j];
                m[i * 64 + 32 + j] = PC[i].mask[j];
            }
        }
        cn_fast_hash(rv, &m[0], 2*l);
        return rv;
    }
    
    key hash_to_scalar(ctkeyV PC) {
        key rv = cn_fast_hash(PC);
        sc_reduce32(rv.bytes);
        return rv;
    }
    
    key hashToPointSimple(const key & hh) {
        key pointk;
        ge_p1p1 point2;
        ge_p2 point;
        ge_p3 res;
        key h = cn_fast_hash(hh); 
        ge_frombytes_vartime(&res, h.bytes);
        ge_p3_to_p2(&point, &res);
        ge_mul8(&point2, &point);
        ge_p1p1_to_p3(&res, &point2);
        ge_p3_tobytes(pointk.bytes, &res);
        return pointk;
    }    
    
    key hashToPoint(const key & hh) {
        key pointk;
        ge_p2 point;
        ge_p1p1 point2;
        ge_p3 res;
        key h = cn_fast_hash(hh); 
        ge_fromfe_frombytes_vartime(&point, h.bytes);
        ge_mul8(&point2, &point);
        ge_p1p1_to_p3(&res, &point2);        
        ge_p3_tobytes(pointk.bytes, &res);
        return pointk;
    }

void fe_mul(fe h,const fe f,const fe g)
{
    int32_t f0 = f[0];
    int32_t f1 = f[1];
    int32_t f2 = f[2];
    int32_t f3 = f[3];
    int32_t f4 = f[4];
    int32_t f5 = f[5];
    int32_t f6 = f[6];
    int32_t f7 = f[7];
    int32_t f8 = f[8];
    int32_t f9 = f[9];
    int32_t g0 = g[0];
    int32_t g1 = g[1];
    int32_t g2 = g[2];
    int32_t g3 = g[3];
    int32_t g4 = g[4];
    int32_t g5 = g[5];
    int32_t g6 = g[6];
    int32_t g7 = g[7];
    int32_t g8 = g[8];
    int32_t g9 = g[9];
    int32_t g1_19 = 19 * g1; /* 1.959375*2^29 */
    int32_t g2_19 = 19 * g2; /* 1.959375*2^30; still ok */
    int32_t g3_19 = 19 * g3;
    int32_t g4_19 = 19 * g4;
    int32_t g5_19 = 19 * g5;
    int32_t g6_19 = 19 * g6;
    int32_t g7_19 = 19 * g7;
    int32_t g8_19 = 19 * g8;
    int32_t g9_19 = 19 * g9;
    int32_t f1_2 = 2 * f1;
    int32_t f3_2 = 2 * f3;
    int32_t f5_2 = 2 * f5;
    int32_t f7_2 = 2 * f7;
    int32_t f9_2 = 2 * f9;
    int64_t f0g0    = f0   * (int64_t) g0;
    int64_t f0g1    = f0   * (int64_t) g1;
    int64_t f0g2    = f0   * (int64_t) g2;
    int64_t f0g3    = f0   * (int64_t) g3;
    int64_t f0g4    = f0   * (int64_t) g4;
    int64_t f0g5    = f0   * (int64_t) g5;
    int64_t f0g6    = f0   * (int64_t) g6;
    int64_t f0g7    = f0   * (int64_t) g7;
    int64_t f0g8    = f0   * (int64_t) g8;
    int64_t f0g9    = f0   * (int64_t) g9;
    int64_t f1g0    = f1   * (int64_t) g0;
    int64_t f1g1_2  = f1_2 * (int64_t) g1;
    int64_t f1g2    = f1   * (int64_t) g2;
    int64_t f1g3_2  = f1_2 * (int64_t) g3;
    int64_t f1g4    = f1   * (int64_t) g4;
    int64_t f1g5_2  = f1_2 * (int64_t) g5;
    int64_t f1g6    = f1   * (int64_t) g6;
    int64_t f1g7_2  = f1_2 * (int64_t) g7;
    int64_t f1g8    = f1   * (int64_t) g8;
    int64_t f1g9_38 = f1_2 * (int64_t) g9_19;
    int64_t f2g0    = f2   * (int64_t) g0;
    int64_t f2g1    = f2   * (int64_t) g1;
    int64_t f2g2    = f2   * (int64_t) g2;
    int64_t f2g3    = f2   * (int64_t) g3;
    int64_t f2g4    = f2   * (int64_t) g4;
    int64_t f2g5    = f2   * (int64_t) g5;
    int64_t f2g6    = f2   * (int64_t) g6;
    int64_t f2g7    = f2   * (int64_t) g7;
    int64_t f2g8_19 = f2   * (int64_t) g8_19;
    int64_t f2g9_19 = f2   * (int64_t) g9_19;
    int64_t f3g0    = f3   * (int64_t) g0;
    int64_t f3g1_2  = f3_2 * (int64_t) g1;
    int64_t f3g2    = f3   * (int64_t) g2;
    int64_t f3g3_2  = f3_2 * (int64_t) g3;
    int64_t f3g4    = f3   * (int64_t) g4;
    int64_t f3g5_2  = f3_2 * (int64_t) g5;
    int64_t f3g6    = f3   * (int64_t) g6;
    int64_t f3g7_38 = f3_2 * (int64_t) g7_19;
    int64_t f3g8_19 = f3   * (int64_t) g8_19;
    int64_t f3g9_38 = f3_2 * (int64_t) g9_19;
    int64_t f4g0    = f4   * (int64_t) g0;
    int64_t f4g1    = f4   * (int64_t) g1;
    int64_t f4g2    = f4   * (int64_t) g2;
    int64_t f4g3    = f4   * (int64_t) g3;
    int64_t f4g4    = f4   * (int64_t) g4;
    int64_t f4g5    = f4   * (int64_t) g5;
    int64_t f4g6_19 = f4   * (int64_t) g6_19;
    int64_t f4g7_19 = f4   * (int64_t) g7_19;
    int64_t f4g8_19 = f4   * (int64_t) g8_19;
    int64_t f4g9_19 = f4   * (int64_t) g9_19;
    int64_t f5g0    = f5   * (int64_t) g0;
    int64_t f5g1_2  = f5_2 * (int64_t) g1;
    int64_t f5g2    = f5   * (int64_t) g2;
    int64_t f5g3_2  = f5_2 * (int64_t) g3;
    int64_t f5g4    = f5   * (int64_t) g4;
    int64_t f5g5_38 = f5_2 * (int64_t) g5_19;
    int64_t f5g6_19 = f5   * (int64_t) g6_19;
    int64_t f5g7_38 = f5_2 * (int64_t) g7_19;
    int64_t f5g8_19 = f5   * (int64_t) g8_19;
    int64_t f5g9_38 = f5_2 * (int64_t) g9_19;
    int64_t f6g0    = f6   * (int64_t) g0;
    int64_t f6g1    = f6   * (int64_t) g1;
    int64_t f6g2    = f6   * (int64_t) g2;
    int64_t f6g3    = f6   * (int64_t) g3;
    int64_t f6g4_19 = f6   * (int64_t) g4_19;
    int64_t f6g5_19 = f6   * (int64_t) g5_19;
    int64_t f6g6_19 = f6   * (int64_t) g6_19;
    int64_t f6g7_19 = f6   * (int64_t) g7_19;
    int64_t f6g8_19 = f6   * (int64_t) g8_19;
    int64_t f6g9_19 = f6   * (int64_t) g9_19;
    int64_t f7g0    = f7   * (int64_t) g0;
    int64_t f7g1_2  = f7_2 * (int64_t) g1;
    int64_t f7g2    = f7   * (int64_t) g2;
    int64_t f7g3_38 = f7_2 * (int64_t) g3_19;
    int64_t f7g4_19 = f7   * (int64_t) g4_19;
    int64_t f7g5_38 = f7_2 * (int64_t) g5_19;
    int64_t f7g6_19 = f7   * (int64_t) g6_19;
    int64_t f7g7_38 = f7_2 * (int64_t) g7_19;
    int64_t f7g8_19 = f7   * (int64_t) g8_19;
    int64_t f7g9_38 = f7_2 * (int64_t) g9_19;
    int64_t f8g0    = f8   * (int64_t) g0;
    int64_t f8g1    = f8   * (int64_t) g1;
    int64_t f8g2_19 = f8   * (int64_t) g2_19;
    int64_t f8g3_19 = f8   * (int64_t) g3_19;
    int64_t f8g4_19 = f8   * (int64_t) g4_19;
    int64_t f8g5_19 = f8   * (int64_t) g5_19;
    int64_t f8g6_19 = f8   * (int64_t) g6_19;
    int64_t f8g7_19 = f8   * (int64_t) g7_19;
    int64_t f8g8_19 = f8   * (int64_t) g8_19;
    int64_t f8g9_19 = f8   * (int64_t) g9_19;
    int64_t f9g0    = f9   * (int64_t) g0;
    int64_t f9g1_38 = f9_2 * (int64_t) g1_19;
    int64_t f9g2_19 = f9   * (int64_t) g2_19;
    int64_t f9g3_38 = f9_2 * (int64_t) g3_19;
    int64_t f9g4_19 = f9   * (int64_t) g4_19;
    int64_t f9g5_38 = f9_2 * (int64_t) g5_19;
    int64_t f9g6_19 = f9   * (int64_t) g6_19;
    int64_t f9g7_38 = f9_2 * (int64_t) g7_19;
    int64_t f9g8_19 = f9   * (int64_t) g8_19;
    int64_t f9g9_38 = f9_2 * (int64_t) g9_19;
    int64_t h0 = f0g0+f1g9_38+f2g8_19+f3g7_38+f4g6_19+f5g5_38+f6g4_19+f7g3_38+f8g2_19+f9g1_38;
    int64_t h1 = f0g1+f1g0   +f2g9_19+f3g8_19+f4g7_19+f5g6_19+f6g5_19+f7g4_19+f8g3_19+f9g2_19;
    int64_t h2 = f0g2+f1g1_2 +f2g0   +f3g9_38+f4g8_19+f5g7_38+f6g6_19+f7g5_38+f8g4_19+f9g3_38;
    int64_t h3 = f0g3+f1g2   +f2g1   +f3g0   +f4g9_19+f5g8_19+f6g7_19+f7g6_19+f8g5_19+f9g4_19;
    int64_t h4 = f0g4+f1g3_2 +f2g2   +f3g1_2 +f4g0   +f5g9_38+f6g8_19+f7g7_38+f8g6_19+f9g5_38;
    int64_t h5 = f0g5+f1g4   +f2g3   +f3g2   +f4g1   +f5g0   +f6g9_19+f7g8_19+f8g7_19+f9g6_19;
    int64_t h6 = f0g6+f1g5_2 +f2g4   +f3g3_2 +f4g2   +f5g1_2 +f6g0   +f7g9_38+f8g8_19+f9g7_38;
    int64_t h7 = f0g7+f1g6   +f2g5   +f3g4   +f4g3   +f5g2   +f6g1   +f7g0   +f8g9_19+f9g8_19;
    int64_t h8 = f0g8+f1g7_2 +f2g6   +f3g5_2 +f4g4   +f5g3_2 +f6g2   +f7g1_2 +f8g0   +f9g9_38;
    int64_t h9 = f0g9+f1g8   +f2g7   +f3g6   +f4g5   +f5g4   +f6g3   +f7g2   +f8g1   +f9g0   ;
    int64_t carry0;
    int64_t carry1;
    int64_t carry2;
    int64_t carry3;
    int64_t carry4;
    int64_t carry5;
    int64_t carry6;
    int64_t carry7;
    int64_t carry8;
    int64_t carry9;

    /*
    |h0| <= (1.65*1.65*2^52*(1+19+19+19+19)+1.65*1.65*2^50*(38+38+38+38+38))
      i.e. |h0| <= 1.4*2^60; narrower ranges for h2, h4, h6, h8
    |h1| <= (1.65*1.65*2^51*(1+1+19+19+19+19+19+19+19+19))
      i.e. |h1| <= 1.7*2^59; narrower ranges for h3, h5, h7, h9
    */

    carry0 = (h0 + (int64_t) (1<<25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    carry4 = (h4 + (int64_t) (1<<25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    /* |h0| <= 2^25 */
    /* |h4| <= 2^25 */
    /* |h1| <= 1.71*2^59 */
    /* |h5| <= 1.71*2^59 */

    carry1 = (h1 + (int64_t) (1<<24)) >> 25;
    h2 += carry1;
    h1 -= carry1 << 25;
    carry5 = (h5 + (int64_t) (1<<24)) >> 25;
    h6 += carry5;
    h5 -= carry5 << 25;
    /* |h1| <= 2^24; from now on fits into int32 */
    /* |h5| <= 2^24; from now on fits into int32 */
    /* |h2| <= 1.41*2^60 */
    /* |h6| <= 1.41*2^60 */

    carry2 = (h2 + (int64_t) (1<<25)) >> 26;
    h3 += carry2;
    h2 -= carry2 << 26;
    carry6 = (h6 + (int64_t) (1<<25)) >> 26;
    h7 += carry6;
    h6 -= carry6 << 26;
    /* |h2| <= 2^25; from now on fits into int32 unchanged */
    /* |h6| <= 2^25; from now on fits into int32 unchanged */
    /* |h3| <= 1.71*2^59 */
    /* |h7| <= 1.71*2^59 */

    carry3 = (h3 + (int64_t) (1<<24)) >> 25;
    h4 += carry3;
    h3 -= carry3 << 25;
    carry7 = (h7 + (int64_t) (1<<24)) >> 25;
    h8 += carry7;
    h7 -= carry7 << 25;
    /* |h3| <= 2^24; from now on fits into int32 unchanged */
    /* |h7| <= 2^24; from now on fits into int32 unchanged */
    /* |h4| <= 1.72*2^34 */
    /* |h8| <= 1.41*2^60 */

    carry4 = (h4 + (int64_t) (1<<25)) >> 26;
    h5 += carry4;
    h4 -= carry4 << 26;
    carry8 = (h8 + (int64_t) (1<<25)) >> 26;
    h9 += carry8;
    h8 -= carry8 << 26;
    /* |h4| <= 2^25; from now on fits into int32 unchanged */
    /* |h8| <= 2^25; from now on fits into int32 unchanged */
    /* |h5| <= 1.01*2^24 */
    /* |h9| <= 1.71*2^59 */

    carry9 = (h9 + (int64_t) (1<<24)) >> 25;
    h0 += carry9 * 19;
    h9 -= carry9 << 25;
    /* |h9| <= 2^24; from now on fits into int32 unchanged */
    /* |h0| <= 1.1*2^39 */

    carry0 = (h0 + (int64_t) (1<<25)) >> 26;
    h1 += carry0;
    h0 -= carry0 << 26;
    /* |h0| <= 2^25; from now on fits into int32 unchanged */
    /* |h1| <= 1.01*2^24 */

    h[0] = h0;
    h[1] = h1;
    h[2] = h2;
    h[3] = h3;
    h[4] = h4;
    h[5] = h5;
    h[6] = h6;
    h[7] = h7;
    h[8] = h8;
    h[9] = h9;
}



    void hashToPoint(key & pointk, const key & hh) {
        ge_p2 point;
        ge_p1p1 point2;
        ge_p3 res;
        key h = cn_fast_hash(hh); 
        ge_fromfe_frombytes_vartime(&point, h.bytes);
        ge_mul8(&point2, &point);
        ge_p1p1_to_p3(&res, &point2);        
        ge_p3_tobytes(pointk.bytes, &res);
    }    

    //sums a vector of curve points (for scalars use sc_add)
    void sumKeys(key & Csum, const keyV &  Cis) {
        identity(Csum);
        size_t i = 0;
        for (i = 0; i < Cis.size(); i++) {
            addKeys(Csum, Csum, Cis[i]);
        }
    }

    //Elliptic Curve Diffie Helman: encodes and decodes the amount b and mask a
    // where C= aG + bH
    void ecdhEncode(ecdhTuple & unmasked, const key & receiverPk) {
        key esk;
        //compute shared secret
        skpkGen(esk, unmasked.senderPk);
        key sharedSec1 = hash_to_scalar(scalarmultKey(receiverPk, esk));
        key sharedSec2 = hash_to_scalar(sharedSec1);
        //encode
        sc_add(unmasked.mask.bytes, unmasked.mask.bytes, sharedSec1.bytes);
        sc_add(unmasked.amount.bytes, unmasked.amount.bytes, sharedSec2.bytes);
    }
    void ecdhDecode(ecdhTuple & masked, const key & receiverSk) {
        //compute shared secret
        key sharedSec1 = hash_to_scalar(scalarmultKey(masked.senderPk, receiverSk));
        key sharedSec2 = hash_to_scalar(sharedSec1);
        //encode
        sc_sub(masked.mask.bytes, masked.mask.bytes, sharedSec1.bytes);
        sc_sub(masked.amount.bytes, masked.amount.bytes, sharedSec2.bytes);
    }
}
