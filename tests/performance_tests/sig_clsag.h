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

#include "single_tx_test_base.h"

template<size_t ring_size, bool ver, size_t index>
class test_sig_clsag : public single_tx_test_base
{
public:
  static const size_t n = ring_size;
  static const size_t loop_count = 1000;
  static const size_t l = index;

  bool init()
  {
    if (!single_tx_test_base::init())
      return false;

    p = rct::skGen();
    z = rct::skGen();
    P = rct::skvGen(n);
    C = rct::skvGen(n);
    for (size_t i = 0 ; i < n; i++)
    {
        P[i] = rct::scalarmultBase(P[i]);
        C[i] = rct::scalarmultBase(C[i]);
    }
    P[l] = rct::scalarmultBase(p);
    C[l] = rct::scalarmultBase(z);
    
    sig = CLSAG_Gen(rct::identity(),P,p,C,z,l);

    return true;
  }

  bool test()
  {
    if (ver)
      return CLSAG_Ver(rct::identity(),P,C,sig);
    else
      CLSAG_Gen(rct::identity(),P,p,C,z,l);
    return true;
  }

private:
  rct::key p;
  rct::key z;
  rct::keyV P;
  rct::keyV C;
  rct::clsag sig;
};
