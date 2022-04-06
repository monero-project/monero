// Copyright (c) 2019-2022, The Monero Project
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

#include "gtest/gtest.h"
#include "int-util.h"
#include "cryptonote_basic/difficulty.h"

static cryptonote::difficulty_type MKDIFF(uint64_t high, uint64_t low)
{
  cryptonote::difficulty_type d = high;
  d = (d << 64) | low;
  return d;
}

static crypto::hash MKHASH(uint64_t high, uint64_t low)
{
  cryptonote::difficulty_type hash_target = high;
  hash_target = (hash_target << 64) | low;
  boost::multiprecision::uint256_t hash_value = std::numeric_limits<boost::multiprecision::uint256_t>::max() / hash_target;
  crypto::hash h;
  uint64_t val;
  val = (hash_value & 0xffffffffffffffff).convert_to<uint64_t>();
  ((uint64_t*)&h)[0] = SWAP64LE(val);
  hash_value >>= 64;
  val = (hash_value & 0xffffffffffffffff).convert_to<uint64_t>();
  ((uint64_t*)&h)[1] = SWAP64LE(val);
  hash_value >>= 64;
  val = (hash_value & 0xffffffffffffffff).convert_to<uint64_t>();
  ((uint64_t*)&h)[2] = SWAP64LE(val);
  hash_value >>= 64;
  val = (hash_value & 0xffffffffffffffff).convert_to<uint64_t>();
  ((uint64_t*)&h)[3] = SWAP64LE(val);
  return h;
}

TEST(difficulty, check_hash)
{
  ASSERT_TRUE(cryptonote::check_hash(MKHASH(0, 1), MKDIFF(0, 1)));
  ASSERT_FALSE(cryptonote::check_hash(MKHASH(0, 1), MKDIFF(0, 2)));

  ASSERT_TRUE(cryptonote::check_hash(MKHASH(0, 0xffffffffffffffff), MKDIFF(0, 0xffffffffffffffff)));
  ASSERT_FALSE(cryptonote::check_hash(MKHASH(0, 0xffffffffffffffff), MKDIFF(1, 0)));

  ASSERT_TRUE(cryptonote::check_hash(MKHASH(1, 1), MKDIFF(1, 1)));
  ASSERT_FALSE(cryptonote::check_hash(MKHASH(1, 1), MKDIFF(1, 2)));

  ASSERT_TRUE(cryptonote::check_hash(MKHASH(0xffffffffffffffff, 1), MKDIFF(0xffffffffffffffff, 1)));
  ASSERT_FALSE(cryptonote::check_hash(MKHASH(0xffffffffffffffff, 1), MKDIFF(0xffffffffffffffff, 2)));
}
