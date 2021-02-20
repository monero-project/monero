// Copyright (c) 2017-2020, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <stdlib.h>
#include "gtest/gtest.h"
#include "string_tools.h"
#include "ringct/rctOps.h"
#include "ringct/triptych.h"

using namespace rct;

// Test random proofs in batches
//
// Fixed: n (size base)
// Variable: m (size exponent), l (signing index)
TEST(triptych, random)
{
    const size_t n = 2; // size base: N = n**m
    const size_t N_proofs = 2; // number of proofs with common keys to verify in a batch

    // Ring sizes: N = n**m
    for (size_t m = 2; m <= 6; m++)
    {
        const size_t N = pow(n,m); // anonymity set size
        std::vector<TriptychProof> p;
        p.reserve(N_proofs);
        p.resize(0);
        std::vector<const TriptychProof *> proofs;
        proofs.reserve(N_proofs);
        proofs.resize(0);

        // Build key vectors
        keyV M = keyV(N);
        keyV P = keyV(N);
        keyV r = keyV(N_proofs);
        keyV s = keyV(N_proofs);
        keyV messages = keyV(N_proofs);
        keyV C_offsets = keyV(N_proofs);

        // Random keys
        key temp;
        for (size_t k = 0; k < N; k++)
        {
            skpkGen(temp,M[k]);
            skpkGen(temp,P[k]);
        }

        // Signing keys, messages, and commitment offsets
        key s1,s2;
        for (size_t i = 0; i < N_proofs; i++)
        {
            skpkGen(r[i],M[i]);
            skpkGen(s1,P[i]);
            messages[i] = skGen();
            skpkGen(s2,C_offsets[i]);
            sc_sub(s[i].bytes,s1.bytes,s2.bytes);
        }

        // Build proofs
        for (size_t i = 0; i < N_proofs; i++)
        {
            p.push_back(triptych_prove(M,P,C_offsets[i],i,r[i],s[i],n,m,messages[i]));
        }
        for (TriptychProof &proof: p)
        {
            proofs.push_back(&proof);
        }

        // Verify batch
        ASSERT_TRUE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
    }
}

static const char * const torsion_elements[] =
{
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa",
  "0000000000000000000000000000000000000000000000000000000000000000",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85",
  "ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05",
  "0000000000000000000000000000000000000000000000000000000000000080",
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a",
};

TEST(triptych, torsion)
{
    const size_t n = 2; // size base: N = n**m
    const size_t m = 2;
    const size_t N = pow(n,m); // anonymity set size
    TriptychProof p;
    std::vector<const TriptychProof *> proofs;
    proofs.reserve(1);
    proofs.resize(0);

    // Build key vectors
    keyV M = keyV(N);
    keyV P = keyV(N);
    keyV r = keyV(1);
    keyV s = keyV(1);
    keyV messages = keyV(1);
    keyV C_offsets = keyV(1);

    // Random keys
    key temp;
    for (size_t k = 0; k < N; k++)
    {
        skpkGen(temp,M[k]);
        skpkGen(temp,P[k]);
    }

    // Signing keys, messages, and commitment offsets
    key s1,s2;
    skpkGen(r[0],M[0]);
    skpkGen(s1,P[0]);
    messages[0] = skGen();
    skpkGen(s2,C_offsets[0]);
    sc_sub(s[0].bytes,s1.bytes,s2.bytes);

    // Build proof
    p = triptych_prove(M,P,C_offsets[0],0,r[0],s[0],n,m,messages[0]);
    proofs.push_back(&p);

    // Verify proof
    ASSERT_TRUE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));

    for (const auto &xs: torsion_elements)
    {
      rct::key x;
      ASSERT_TRUE(epee::string_tools::hex_to_pod(xs, x));
      ASSERT_FALSE(rct::isInMainSubgroup(x));

      rct::addKeys(p.J, p.J, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.J, p.J, x);

      rct::addKeys(p.K, p.K, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.K, p.K, x);

      rct::addKeys(p.A, p.A, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.A, p.A, x);

      rct::addKeys(p.B, p.B, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.B, p.B, x);

      rct::addKeys(p.C, p.C, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.C, p.C, x);

      rct::addKeys(p.D, p.D, x);
      ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
      rct::subKeys(p.D, p.D, x);

      for (rct::key &e: p.X)
      {
        rct::addKeys(e, e, x);
        ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
        rct::subKeys(e, e, x);
      }

      for (rct::key &e: p.Y)
      {
        rct::addKeys(e, e, x);
        ASSERT_FALSE(triptych_verify(M,P,C_offsets,proofs,n,m,messages));
        rct::subKeys(e, e, x);
      }
    }
}
