// Copyright (c) 2014-2016, The Monero Project
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

#include <cstdint>

#include "ringct/rctTypes.h"
#include "ringct/rctSigs.h"
#include "ringct/rctOps.h"

using namespace crypto;
using namespace rct;

TEST(ringct, SNL)
{
  key x, P1;
  skpkGen(x, P1);

  key P2 = pkGen();
  key P3 = pkGen();

  key L1, s1, s2;
  GenSchnorrNonLinkable(L1, s1, s2, x, P1, P2, 0);

  // a valid one
  // an invalid one
  ASSERT_TRUE(VerSchnorrNonLinkable(P1, P2, L1, s1, s2));
  ASSERT_FALSE(VerSchnorrNonLinkable(P1, P3, L1, s1, s2));
}

TEST(ringct, ASNL)
{
    int j = 0;

        //Tests for ASNL
        //#ASNL true one, false one, C != sum Ci, and one out of the range..
        int N = 64;
        key64 xv;
        key64 P1v;
        key64 P2v;
        bits indi;

        for (j = 0 ; j < N ; j++) {
            indi[j] = (int)randXmrAmount(2);

            xv[j] = skGen();
            if ( (int)indi[j] == 0 ) {
                P1v[j] = scalarmultBase(xv[j]);
                P2v[j] = pkGen();

            } else {

                P2v[j] = scalarmultBase(xv[j]);
                P1v[j] = pkGen();

            }
        }

        asnlSig L1s2s = GenASNL(xv, P1v, P2v, indi);
        //#true one
        ASSERT_TRUE(VerASNL(P1v, P2v, L1s2s));

        //#false one
        indi[3] = (indi[3] + 1) % 2;
        L1s2s = GenASNL(xv, P1v, P2v, indi);

        ASSERT_FALSE(VerASNL(P1v, P2v, L1s2s));
}

TEST(ringct, MG_sigs)
{
    int j = 0;
    int N = 0;

        //Tests for MG Sigs
        //#MG sig: true one
        N = 3;// #cols
        int   R = 3;// #rows
        keyV xtmp = skvGen(R);
        keyM xm = keyMInit(R, N);// = [[None]*N] #just used to generate test public keys
        keyV sk = skvGen(R);
        keyM P  = keyMInit(R, N);// = keyM[[None]*N] #stores the public keys;
        int ind = 2;
        int i = 0;
        for (j = 0 ; j < R ; j++) {
            for (i = 0 ; i < N ; i++)
            {
                xm[i][j] = skGen();
                P[i][j] = scalarmultBase(xm[i][j]);
            }
        }
        for (j = 0 ; j < R ; j++) {
            sk[j] = xm[ind][j];
        }
        key message = identity();
        mgSig IIccss = MLSAG_Gen(message, P, sk, ind);
        ASSERT_TRUE(MLSAG_Ver(message, P, IIccss));

        //#MG sig: false one
        N = 3;// #cols
        R = 3;// #rows
        xtmp = skvGen(R);
        keyM xx(N, xtmp);// = [[None]*N] #just used to generate test public keys
        sk = skvGen(R);
        //P (N, xtmp);// = keyM[[None]*N] #stores the public keys;

        ind = 2;
        for (j = 0 ; j < R ; j++) {
            for (i = 0 ; i < N ; i++)
            {
                xx[i][j] = skGen();
                P[i][j] = scalarmultBase(xx[i][j]);
            }
            sk[j] = xx[ind][j];
        }
        sk[2] = skGen();//asume we don't know one of the private keys..
        IIccss = MLSAG_Gen(message, P, sk, ind);
        ASSERT_FALSE(MLSAG_Ver(message, P, IIccss));
}

TEST(ringct, range_proofs)
{
        //Ring CT Stuff
        //ct range proofs
        ctkeyV sc, pc;
        ctkey sctmp, pctmp;
        //add fake input 5000
        tie(sctmp, pctmp) = ctskpkGen(6000);
        sc.push_back(sctmp);
        pc.push_back(pctmp);


        tie(sctmp, pctmp) = ctskpkGen(7000);
        sc.push_back(sctmp);
        pc.push_back(pctmp);
        vector<xmr_amount >amounts;


        //add output 500
        amounts.push_back(500);
        keyV destinations;
        key Sk, Pk;
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);


        //add output for 12500
        amounts.push_back(12500);
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        //compute rct data with mixin 500
        rctSig s = genRct(sc, pc, destinations, amounts, 3);

        //verify rct data
        ASSERT_TRUE(verRct(s));

        //decode received amount
        ASSERT_TRUE(decodeRct(s, Sk, 1));

        // Ring CT with failing MG sig part should not verify!
        // Since sum of inputs != outputs

        amounts[1] = 12501;
        skpkGen(Sk, Pk);
        destinations[1] = Pk;


        //compute rct data with mixin 500
        s = genRct(sc, pc, destinations, amounts, 3);

        //verify rct data
        ASSERT_FALSE(verRct(s));

        //decode received amount
        ASSERT_TRUE(decodeRct(s, Sk, 1));
}

