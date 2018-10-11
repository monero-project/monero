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

#include <boost/lexical_cast.hpp>
#include "misc_log_ex.h"
#include "rctOps.h"
using namespace crypto;
using namespace std;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "ringct"

#define CHECK_AND_ASSERT_THROW_MES_L1(expr, message) {if(!(expr)) {MWARNING(message); throw std::runtime_error(message);}}

namespace rct {

    //Various key initialization functions

    //initializes a key matrix;
    //first parameter is rows,
    //second is columns
    keyM keyMInit(size_t rows, size_t cols) {
        keyM rv(cols);
        size_t i = 0;
        for (i = 0 ; i < cols ; i++) {
            rv[i] = keyV(rows);
        }
        return rv;
    }




    //Various key generation functions

    bool toPointCheckOrder(ge_p3 *P, const unsigned char *data)
    {
        if (ge_frombytes_vartime(P, data))
            return false;
        ge_p2 R;
        ge_scalarmult(&R, curveOrder().bytes, P);
        key tmp;
        ge_tobytes(tmp.bytes, &R);
        return tmp == identity();
    }

    //generates a random scalar which can be used as a secret key or mask
    void skGen(key &sk) {
        random32_unbiased(sk.bytes);
    }

    //generates a random scalar which can be used as a secret key or mask
    key skGen() {
        key sk;
        skGen(sk);
        return sk;
    }

    //Generates a vector of secret key
    //Mainly used in testing
    keyV skvGen(size_t rows ) {
        CHECK_AND_ASSERT_THROW_MES(rows > 0, "0 keys requested");
        keyV rv(rows);
        size_t i = 0;
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

    //generates C =aG + bH from b, a is given..
    void genC(key & C, const key & a, xmr_amount amount) {
        key bH = scalarmultH(d2h(amount));
        addKeys1(C, a, bH);
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
    tuple<ctkey, ctkey> ctskpkGen(const key &bH) {
        ctkey sk, pk;
        skpkGen(sk.dest, pk.dest);
        skpkGen(sk.mask, pk.mask);
        addKeys(pk.mask, pk.mask, bH);
        return make_tuple(sk, pk);
    }
    
    key zeroCommit(xmr_amount amount) {
        key am = d2h(amount);
        key bH = scalarmultH(am);
        return addKeys(G, bH);
    }

    key commit(xmr_amount amount, const key &mask) {
        key c = scalarmultBase(mask);
        key am = d2h(amount);
        key bH = scalarmultH(am);
        addKeys(c, c, bH);
        return c;
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
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A, P.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_scalarmult(&R, a.bytes, &A);
        ge_tobytes(aP.bytes, &R);
    }

    //does a * P where a is a scalar and P is an arbitrary point
    key scalarmultKey(const key & P, const key & a) {
        ge_p3 A;
        ge_p2 R;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A, P.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_scalarmult(&R, a.bytes, &A);
        key aP;
        ge_tobytes(aP.bytes, &R);
        return aP;
    }


    //Computes aH where H= toPoint(cn_fast_hash(G)), G the basepoint
    key scalarmultH(const key & a) {
        ge_p2 R;
        ge_scalarmult(&R, a.bytes, &ge_p3_H);
        key aP;
        ge_tobytes(aP.bytes, &R);
        return aP;
    }

    //Computes 8P
    key scalarmult8(const key & P) {
        ge_p3 p3;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&p3, P.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_p2 p2;
        ge_p3_to_p2(&p2, &p3);
        ge_p1p1 p1;
        ge_mul8(&p1, &p2);
        ge_p1p1_to_p2(&p2, &p1);
        rct::key res;
        ge_tobytes(res.bytes, &p2);
        return res;
    }

    //Computes aL where L is the curve order
    bool isInMainSubgroup(const key & a) {
        ge_p3 p3;
        return toPointCheckOrder(&p3, a.bytes);
    }

    //Curve addition / subtractions

    //for curve points: AB = A + B
    void addKeys(key &AB, const key &A, const key &B) {
        ge_p3 B2, A2;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&B2, B.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A2, A.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_cached tmp2;
        ge_p3_to_cached(&tmp2, &B2);
        ge_p1p1 tmp3;
        ge_add(&tmp3, &A2, &tmp2);
        ge_p1p1_to_p3(&A2, &tmp3);
        ge_p3_tobytes(AB.bytes, &A2);
    }

    rct::key addKeys(const key &A, const key &B) {
      key k;
      addKeys(k, A, B);
      return k;
    }

    rct::key addKeys(const keyV &A) {
      if (A.empty())
        return rct::identity();
      ge_p3 p3, tmp;
      CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&p3, A[0].bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
      for (size_t i = 1; i < A.size(); ++i)
      {
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&tmp, A[i].bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_cached p2;
        ge_p3_to_cached(&p2, &tmp);
        ge_p1p1 p1;
        ge_add(&p1, &p3, &p2);
        ge_p1p1_to_p3(&p3, &p1);
      }
      rct::key res;
      ge_p3_tobytes(res.bytes, &p3);
      return res;
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
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&B2, B.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_double_scalarmult_base_vartime(&rv, b.bytes, &B2, a.bytes);
        ge_tobytes(aGbB.bytes, &rv);
    }

    //Does some precomputation to make addKeys3 more efficient
    // input B a curve point and output a ge_dsmp which has precomputation applied
    void precomp(ge_dsmp rv, const key & B) {
        ge_p3 B2;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&B2, B.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_dsm_precomp(rv, &B2);
    }

    //addKeys3
    //aAbB = a*A + b*B where a, b are scalars, A, B are curve points
    //B must be input after applying "precomp"
    void addKeys3(key &aAbB, const key &a, const key &A, const key &b, const ge_dsmp B) {
        ge_p2 rv;
        ge_p3 A2;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A2, A.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_double_scalarmult_precomp_vartime(&rv, a.bytes, &A2, b.bytes, B);
        ge_tobytes(aAbB.bytes, &rv);
    }

    //addKeys3
    //aAbB = a*A + b*B where a, b are scalars, A, B are curve points
    //A and B must be input after applying "precomp"
    void addKeys3(key &aAbB, const key &a, const ge_dsmp A, const key &b, const ge_dsmp B) {
        ge_p2 rv;
        ge_double_scalarmult_precomp_vartime2(&rv, a.bytes, A, b.bytes, B);
        ge_tobytes(aAbB.bytes, &rv);
    }


    //subtract Keys (subtracts curve points)
    //AB = A - B where A, B are curve points
    void subKeys(key & AB, const key &A, const key &B) {
        ge_p3 B2, A2;
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&B2, B.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&A2, A.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
        ge_cached tmp2;
        ge_p3_to_cached(&tmp2, &B2);
        ge_p1p1 tmp3;
        ge_sub(&tmp3, &A2, &tmp2);
        ge_p1p1_to_p3(&A2, &tmp3);
        ge_p3_tobytes(AB.bytes, &A2);
    }

    //checks if A, B are equal in terms of bytes (may say no if one is a non-reduced scalar)
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
        keccak((const uint8_t *)data, l, hash.bytes, 32);
    }
    
    void hash_to_scalar(key &hash, const void * data, const std::size_t l) {
        cn_fast_hash(hash, data, l);
        sc_reduce32(hash.bytes);
    }

    //cn_fast_hash for a 32 byte key
    void cn_fast_hash(key & hash, const key & in) {
        keccak((const uint8_t *)in.bytes, 32, hash.bytes, 32);
    }
    
    void hash_to_scalar(key & hash, const key & in) {
        cn_fast_hash(hash, in);
        sc_reduce32(hash.bytes);
    }

    //cn_fast_hash for a 32 byte key
    key cn_fast_hash(const key & in) {
        key hash;
        keccak((const uint8_t *)in.bytes, 32, hash.bytes, 32);
        return hash;
    }
    
     key hash_to_scalar(const key & in) {
        key hash = cn_fast_hash(in);
        sc_reduce32(hash.bytes);
        return hash;
     }
    
    //cn_fast_hash for a 128 byte unsigned char
    key cn_fast_hash128(const void * in) {
        key hash;
        keccak((const uint8_t *)in, 128, hash.bytes, 32);
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
    key cn_fast_hash(const ctkeyV &PC) {
        if (PC.empty()) return rct::hash2rct(crypto::cn_fast_hash("", 0));
        key rv;
        cn_fast_hash(rv, &PC[0], 64*PC.size());
        return rv;
    }
    
    key hash_to_scalar(const ctkeyV &PC) {
        key rv = cn_fast_hash(PC);
        sc_reduce32(rv.bytes);
        return rv;
    }
    
   //cn_fast_hash for a key-vector of arbitrary length
   //this is useful since you take a number of keys
   //put them in the key vector and it concatenates them
   //and then hashes them
   key cn_fast_hash(const keyV &keys) {
       if (keys.empty()) return rct::hash2rct(crypto::cn_fast_hash("", 0));
       key rv;
       cn_fast_hash(rv, &keys[0], keys.size() * sizeof(keys[0]));
       //dp(rv);
       return rv;
   }
   
   key hash_to_scalar(const keyV &keys) {
       key rv = cn_fast_hash(keys);
       sc_reduce32(rv.bytes);
       return rv;
   }

   key cn_fast_hash(const key64 keys) {
      key rv;
      cn_fast_hash(rv, &keys[0], 64 * sizeof(keys[0]));
      //dp(rv);
      return rv;
   }

   key hash_to_scalar(const key64 keys) {
       key rv = cn_fast_hash(keys);
       sc_reduce32(rv.bytes);
       return rv;
   }

    key hashToPointSimple(const key & hh) {
        key pointk;
        ge_p1p1 point2;
        ge_p2 point;
        ge_p3 res;
        key h = cn_fast_hash(hh); 
        CHECK_AND_ASSERT_THROW_MES_L1(ge_frombytes_vartime(&res, h.bytes) == 0, "ge_frombytes_vartime failed at "+boost::lexical_cast<std::string>(__LINE__));
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
    void ecdhEncode(ecdhTuple & unmasked, const key & sharedSec) {
        key sharedSec1 = hash_to_scalar(sharedSec);
        key sharedSec2 = hash_to_scalar(sharedSec1);
        //encode
        sc_add(unmasked.mask.bytes, unmasked.mask.bytes, sharedSec1.bytes);
        sc_add(unmasked.amount.bytes, unmasked.amount.bytes, sharedSec2.bytes);
    }
    void ecdhDecode(ecdhTuple & masked, const key & sharedSec) {
        key sharedSec1 = hash_to_scalar(sharedSec);
        key sharedSec2 = hash_to_scalar(sharedSec1);
        //decode
        sc_sub(masked.mask.bytes, masked.mask.bytes, sharedSec1.bytes);
        sc_sub(masked.amount.bytes, masked.amount.bytes, sharedSec2.bytes);
    }
}
