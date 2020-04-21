// Copyright (c) 2014-2019, The Monero Project
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
#include "cryptonote_basic/cryptonote_basic.h"
#include "device/device.hpp"

#include "single_tx_test_base.h"

using namespace rct;

template<size_t ring_size, size_t index>
class test_sig_clsag : public single_tx_test_base
{
public:
  static const size_t N = ring_size;
  static const size_t loop_count = 1000;
  static const size_t l = index;

  bool init()
  {
    if (!single_tx_test_base::init())
      return false;

    message = skGen();

    // Random signing/commitment keys
    pubs.reserve(N);
    for (size_t i = 0; i < N; i++)
    {
        key sk;
        ctkey tmp;

        skpkGen(sk, tmp.dest);
        skpkGen(sk, tmp.mask);

        pubs.push_back(tmp);
    }

    // Signing key
    key p;
    skpkGen(p,pubs[l].dest);
    
    // Commitment key
    key t,u;
    t = skGen();
    u = skGen();
    addKeys2(pubs[l].mask,t,u,H);

    // Offset
    key t2;
    t2 = skGen();
    addKeys2(C_offset,t2,u,H);

    // Final signing keys
    ctkey insk;
    insk.dest = p;
    insk.mask = t;

    sig = proveRctCLSAGSimple(message,pubs,insk,t2,C_offset,NULL,NULL,NULL,l,hw::get_device("default"));

    return true;
  }

  bool test()
  {
    return verRctCLSAGSimple(message,sig,pubs,C_offset);
  }

private:
  ctkeyV pubs;
  key C_offset;
  clsag sig;
  key message;
};
