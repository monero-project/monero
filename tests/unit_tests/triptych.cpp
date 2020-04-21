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
        std::vector<TriptychProof *> proofs;
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

