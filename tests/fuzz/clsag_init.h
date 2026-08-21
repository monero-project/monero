// Copyright (c) 2017-2026, The Monero Project
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

#include "cryptonote_basic/cryptonote_basic.h"
#include "misc_log_ex.h"
#include "ringct/rctSigs.h"

using namespace crypto;
using namespace rct;

static key message;
static ctkeyV pubs;
static key Cout;
static clsag clsag_s;

inline static void clsag_init() {
  message = identity();

  const size_t N = 11;
  const size_t idx = 5;

  pubs.clear();
  pubs.reserve(N);

  for (size_t i = 0; i < N; ++i)
  {
    key sk;
    ctkey tmp;

    skpkGen(sk, tmp.dest);
    skpkGen(sk, tmp.mask);

    pubs.push_back(tmp);
  }

  // Set P[idx]
  key p;
  skpkGen(p, pubs[idx].dest);

  // Set C[idx]
  key t = skGen();
  key u = skGen();
  addKeys2(pubs[idx].mask,t,u,H);

  // Set commitment offset
  key t2 = skGen();
  addKeys2(Cout,t2,u,H);

  // Prepare generation inputs
  ctkey insk;
  insk.dest = p;
  insk.mask = t;

  clsag_s = proveRctCLSAGSimple(message, pubs, insk, t2, Cout, idx, hw::get_device("default"));
  CHECK_AND_ASSERT_THROW_MES(verRctCLSAGSimple(message, clsag_s, pubs, Cout), "Generated CLSAG does not verify");
}
