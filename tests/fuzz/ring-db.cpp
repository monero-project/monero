// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Fuzzes the wallet's ring database (ringdb.cpp, LMDB-backed): set_ring/get_ring/
// add_rings/remove_rings driven by attacker-controlled key images and ring vectors.
// The database persists across iterations so operations accumulate into deeper states.

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "wallet/ringdb.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

namespace
{
  std::unique_ptr<tools::ringdb> &db()
  {
    static std::unique_ptr<tools::ringdb> instance = nullptr;
    if (instance == nullptr)
    {
      try { instance = std::make_unique<tools::ringdb>("/tmp/ringdb-fuzz.lmdb", "genesis"); }
      catch (...) { instance = nullptr; }
    }
    return instance;
  }

  crypto::key_image fuzz_key_image(FuzzedDataProvider &fdp)
  {
    crypto::key_image ki;
    memset(ki.data, 0, sizeof(ki.data));
    const std::string bytes = fdp.ConsumeBytesAsString(sizeof(ki.data));
    memcpy(ki.data, bytes.data(), bytes.size() < sizeof(ki.data) ? bytes.size() : sizeof(ki.data));
    return ki;
  }

  crypto::chacha_key fixed_chacha_key()
  {
    crypto::chacha_key key;
    memset(key.data(), 0x42, CHACHA_KEY_SIZE);
    return key;
  }
} // anonymous namespace

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);
  std::unique_ptr<tools::ringdb> &rdb = db();
  if (rdb == nullptr)
    return 0;
  const crypto::chacha_key key = fixed_chacha_key();

  // Configuration from the back.
  const std::size_t n_ops = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 7);
  const std::size_t n_ring = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 7);

  for (std::size_t i = 0; i < n_ops && fdp.remaining_bytes() > 0; ++i)
  {
    const unsigned op = fdp.ConsumeIntegralInRange<unsigned>(0, 3);
    const crypto::key_image ki = fuzz_key_image(fdp);
    try
    {
      switch (op)
      {
      case 0:
      {
        std::vector<uint64_t> outs(n_ring);
        for (std::size_t j = 0; j < n_ring; ++j)
          outs[j] = 1 + j * 8 + fdp.ConsumeIntegralInRange<uint64_t>(0, 8);
        std::sort(outs.begin(), outs.end());
        rdb->set_ring(key, ki, outs, (i & 1) != 0);
        break;
      }
      case 1:
      {
        std::vector<uint64_t> outs;
        rdb->get_ring(key, ki, outs);
        break;
      }
      case 2:
      {
        // add_rings derives one ring per tx input from the tx's key offsets.
        cryptonote::transaction_prefix tx{};
        tx.version = 2;
        for (std::size_t j = 0; j < n_ring; ++j)
        {
          cryptonote::txin_to_key in{};
          in.amount = 0;
          in.key_offsets.push_back(1 + j * 8 + fdp.ConsumeIntegralInRange<uint32_t>(0, 8));
          in.k_image = fuzz_key_image(fdp);
          tx.vin.push_back(std::move(in));
        }
        rdb->add_rings(key, tx);
        break;
      }
      default:
      {
        std::vector<crypto::key_image> kis;
        for (std::size_t j = 0; j < n_ring; ++j)
          kis.push_back(fuzz_key_image(fdp));
        rdb->remove_rings(key, kis);
        break;
      }
      }
    }
    catch (...) {}
  }
END_SIMPLE_FUZZER()
