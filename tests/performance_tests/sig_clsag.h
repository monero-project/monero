// Copyright (c) 2014-2022, The Monero Project
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

#pragma once

#include "ringct/rctSigs.h"
#include "ringct/rctTypes.h"
#include "device/device.hpp"

using namespace rct;

template<size_t a_N, size_t a_T, size_t a_w>
class test_sig_clsag
{
    public:
        static const size_t loop_count = 1000;
        static const size_t N = a_N;
        static const size_t T = a_T;
        static const size_t w = a_w;

        bool init()
        {
            pubs.reserve(N);
            pubs.resize(N);

            r = keyV(w); // M[l[u]] = Com(0,r[u])

            a = keyV(w); // P[l[u]] = Com(a[u],s[u])
            s = keyV(w);

            Q = keyV(T); // Q[j] = Com(b[j],t[j])
            b = keyV(T);
            t = keyV(T);

            // Random keys
            key temp;
            for (size_t k = 0; k < N; k++)
            {
                skpkGen(temp,pubs[k].dest);
                skpkGen(temp,pubs[k].mask);
            }

            // Signing and commitment keys (assumes fixed signing indices 0,1,...,w-1 for this test)
            // TODO: random signing indices
            C_offsets = keyV(w); // P[l[u]] - C_offsets[u] = Com(0,s[u]-s1[u])
            s1 = keyV(w);
            key a_sum = zero();
            key s1_sum = zero();
            messages = keyV(w);
            for (size_t u = 0; u < w; u++)
            {
                skpkGen(r[u],pubs[u].dest); // M[u] = Com(0,r[u])

                a[u] = skGen(); // P[u] = Com(a[u],s[u])
                s[u] = skGen();
                addKeys2(pubs[u].mask,s[u],a[u],H);

                s1[u] = skGen(); // C_offsets[u] = Com(a[u],s1[u])
                addKeys2(C_offsets[u],s1[u],a[u],H);

                sc_add(a_sum.bytes,a_sum.bytes,a[u].bytes);
                sc_add(s1_sum.bytes,s1_sum.bytes,s1[u].bytes);

                messages[u] = skGen();
            }

            // Outputs
            key b_sum = zero();
            key t_sum = zero();
            for (size_t j = 0; j < T-1; j++)
            {
                b[j] = skGen(); // Q[j] = Com(b[j],t[j])
                t[j] = skGen();
                addKeys2(Q[j],t[j],b[j],H);

                sc_add(b_sum.bytes,b_sum.bytes,b[j].bytes);
                sc_add(t_sum.bytes,t_sum.bytes,t[j].bytes);
            }
            // Value/mask balance for Q[T-1]
            sc_sub(b[T-1].bytes,a_sum.bytes,b_sum.bytes);
            sc_sub(t[T-1].bytes,s1_sum.bytes,t_sum.bytes);
            addKeys2(Q[T-1],t[T-1],b[T-1],H);

            // Build proofs
            sigs.reserve(w);
            sigs.resize(0);
            ctkey sk;
            for (size_t u = 0; u < w; u++)
            {
                sk.dest = r[u];
                sk.mask = s[u];

                sigs.push_back(proveRctCLSAGSimple(messages[u],pubs,sk,s1[u],C_offsets[u],u,hw::get_device("default")));
            }

            return true;
        }

        bool test()
        {
            for (size_t u = 0; u < w; u++)
            {
                if (!verRctCLSAGSimple(messages[u],sigs[u],pubs,C_offsets[u]))
                {
                    return false;
                }
            }

            // Check balanace
            std::vector<MultiexpData> balance;
            balance.reserve(w + T);
            balance.resize(0);
            key ZERO = zero();
            key ONE = identity();
            key MINUS_ONE;
            sc_sub(MINUS_ONE.bytes,ZERO.bytes,ONE.bytes);
            for (size_t u = 0; u < w; u++)
            {
                balance.push_back({ONE,C_offsets[u]});
            }
            for (size_t j = 0; j < T; j++)
            {
                balance.push_back({MINUS_ONE,Q[j]});
            }
            if (!(straus(balance) == ONE)) // group identity
            {
                return false;
            }

            return true;
        }

    private:
        ctkeyV pubs;
        keyV Q;
        keyV r;
        keyV s;
        keyV s1;
        keyV t;
        keyV a;
        keyV b;
        keyV C_offsets;
        keyV messages;
        std::vector<clsag> sigs;
};
