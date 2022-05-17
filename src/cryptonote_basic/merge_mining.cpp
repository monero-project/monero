// Copyright (c) 2020-2022, The Monero Project

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

#include <string.h>
#include "misc_log_ex.h"
#include "int-util.h"
#include "crypto/crypto.h"
#include "common/util.h"
#include "merge_mining.h"

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn.mm"

using namespace crypto;

namespace cryptonote
{

//---------------------------------------------------------------
uint32_t get_aux_slot(const crypto::hash &id, uint32_t nonce, uint32_t n_aux_chains)
{
  CHECK_AND_ASSERT_THROW_MES(n_aux_chains > 0, "n_aux_chains is 0");

  uint8_t buf[HASH_SIZE + sizeof(uint32_t) + 1];
  memcpy(buf, &id, HASH_SIZE);
  uint32_t v = SWAP32LE(nonce);
  memcpy(buf + HASH_SIZE, &v, sizeof(uint32_t));
  buf[HASH_SIZE + sizeof(uint32_t)] = config::HASH_KEY_MM_SLOT;

  crypto::hash res;
  tools::sha256sum(buf, sizeof(buf), res);
  v = *((const uint32_t*)&res);
  return SWAP32LE(v) % n_aux_chains;
}
//---------------------------------------------------------------
uint32_t get_path_from_aux_slot(uint32_t slot, uint32_t n_aux_chains)
{
  CHECK_AND_ASSERT_THROW_MES(n_aux_chains > 0, "n_aux_chains is 0");
  CHECK_AND_ASSERT_THROW_MES(slot < n_aux_chains, "slot >= n_aux_chains");

  uint32_t path = 0;
  CHECK_AND_ASSERT_THROW_MES(tree_path(n_aux_chains, slot, &path), "Failed to get path from aux slot");
  return path;
}
//---------------------------------------------------------------
uint32_t encode_mm_depth(uint32_t n_aux_chains, uint32_t nonce)
{
  CHECK_AND_ASSERT_THROW_MES(n_aux_chains > 0, "n_aux_chains is 0");

  // how many bits to we need to representing n_aux_chains - 1
  uint32_t n_bits = 1;
  while ((1u << n_bits) < n_aux_chains && n_bits < 16)
    ++n_bits;
  CHECK_AND_ASSERT_THROW_MES(n_bits <= 16, "Way too many bits required");

  const uint32_t depth = (n_bits - 1) | ((n_aux_chains - 1) << 3) | (nonce << (3 + n_bits));
  return depth;
}
//---------------------------------------------------------------
bool decode_mm_depth(uint32_t depth, uint32_t &n_aux_chains, uint32_t &nonce)
{
  const uint32_t n_bits = 1 + (depth & 7);
  n_aux_chains = 1 + (depth >> 3 & ((1 << n_bits) - 1));
  nonce = depth >> (3 + n_bits);
  return true;
}
//---------------------------------------------------------------
}
