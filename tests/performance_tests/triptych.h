// Copyright (c) 2014-2020, The Monero Project
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

#include "ringct/triptych.h"
#include "ringct/rctTypes.h"

template<bool a_verify>
class test_triptych
{
    public:
        static const size_t loop_count = 1;
        static const bool verify = a_verify;
        static const size_t m = 7;
        static const size_t n = 2;
        static const size_t N = pow(n,m);

        bool init()
        {
            M.reserve(N);
            P.reserve(N);
            l = 1;

            rct::key temp;
            for (size_t k = 0; k < N; k++)
            {
                if (k == l)
                {
                    rct::skpkGen(r,M[k]);
                    rct::skpkGen(s,P[k]);
                }
                else
                {
                    rct::skpkGen(temp,M[k]);
                    rct::skpkGen(temp,P[k]);
                }
            }

            proof = rct::triptych_prove(M,P,l,r,s);

            return true;
        }

        bool test()
        {
            if (!verify)
            {
                proof = rct::triptych_prove(M,P,l,r,s);
                return true;
            }

            return rct::triptych_verify(M,P,proof);
        }

    private:
        rct::keyV M;
        rct::keyV P;
        size_t l;
        rct::key r;
        rct::key s;
        rct::TriptychProof proof;
};
