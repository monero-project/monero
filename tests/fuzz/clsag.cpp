// Copyright (c) 2017-2019, The Monero Project
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

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctSigs.h"
#include "serialization/binary_utils.h"
#include "fuzzer.h"

using namespace crypto;
using namespace rct;

static key message;
static ctkeyV pubs;
static key p, t, t2, u;
static ctkey backup;
static key Cout;
static ctkey insk;
static clsag clsag_s;

static int fuzz_message(const std::string& s)
{
  // fuzz message
  memcpy(&message, (uint8_t*)s.c_str(), sizeof(message));
  auto valid = rct::verRctCLSAGSimple(message,clsag_s,pubs,Cout);

  return 0;
}

static int fuzz_cout(const std::string& s)
{
  // fuzz Cout
  memcpy(&Cout, (uint8_t*)s.c_str(), sizeof(Cout));
  rct::verRctCLSAGSimple(message,clsag_s,pubs,Cout);

  return 0;
}

static int fuzz_clsag(const std::string& s)
{
  // fuzz clsag
  size_t off = 0;

  for (auto& s_ : clsag_s.s)
  {
    memcpy(&s_, (uint8_t*)s.c_str() + off, sizeof(s_));
    off += sizeof(s_);
  }

  memcpy(&clsag_s.c1, (uint8_t*)s.c_str() + off, sizeof(clsag_s.c1)); off += sizeof(clsag_s.c1);
  memcpy(&clsag_s.I, (uint8_t*)s.c_str() + off, sizeof(clsag_s.I)); off += sizeof(clsag_s.I);
  memcpy(&clsag_s.D, (uint8_t*)s.c_str() + off, sizeof(clsag_s.D));

  rct::verRctCLSAGSimple(message,clsag_s,pubs,Cout);

  return 0;
}

static int fuzz_clsag_deserialize(const std::string& s)
{
  // fuzz deserialization
  serialization::parse_binary(s, clsag_s);

  rct::verRctCLSAGSimple(message,clsag_s,pubs,Cout);

  return 0;
}

BEGIN_INIT_SIMPLE_FUZZER()
  message = identity();

  const size_t N = 11;
  const size_t idx = 5;

  for (size_t i = 0; i < N; ++i)
  {
    key sk;
    ctkey tmp;

    skpkGen(sk, tmp.dest);
    skpkGen(sk, tmp.mask);

    pubs.push_back(tmp);
}

  // Set P[idx]
  skpkGen(p, pubs[idx].dest);

  // Set C[idx]
  t = skGen();
  u = skGen();
  addKeys2(pubs[idx].mask,t,u,H);

  // Set commitment offset
  t2 = skGen();
  addKeys2(Cout,t2,u,H);

  // Prepare generation inputs
  insk.dest = p;
  insk.mask = t;

  clsag_s = rct::proveRctCLSAGSimple(zero(),pubs,insk,t2,Cout,NULL,NULL,NULL,idx,hw::get_device("default"));
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  std::string s((const char*)buf, len);
  fuzz_message(s);
  fuzz_cout(s);
  fuzz_clsag(s);
  fuzz_clsag_deserialize(s);
END_SIMPLE_FUZZER()
