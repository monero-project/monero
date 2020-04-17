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

#include "gtest/gtest.h"
#include "ringct/rctOps.h"
#include "ringct/triptych.h"

using namespace rct;

// Test random proofs
TEST(triptych, random)
{
    const size_t n = 2;

    // Ring sizes
    for (size_t m = 1; m <= 6; m++)
    {
        const size_t N = pow(n,m);

        // Signing indices
        for (size_t l = 0; l < N; l++)
        {
            keyV M = keyV(N);
            keyV P = keyV(N);
            key r,s; // secret keys
            const key message = skGen(); // random message
        
            // Build key vectors
            key temp;
            for (size_t k = 0; k < N; k++)
            {
                if (k == l)
                {
                    skpkGen(r,M[k]);
                    skpkGen(s,P[k]);
                }
                else
                {
                    skpkGen(temp,M[k]);
                    skpkGen(temp,P[k]);
                }
            }

            // Prove and verify
            if (m == 1)
            {
                ASSERT_ANY_THROW(TriptychProof proof = triptych_prove(M,P,l,r,s,n,m,message));
            }
            else
            {
                TriptychProof proof = triptych_prove(M,P,l,r,s,n,m,message);
                ASSERT_TRUE(triptych_verify(M,P,proof,n,m,message));
            }
        }
    }
}

