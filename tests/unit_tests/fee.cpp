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

#include "cryptonote_core/blockchain.h"

using namespace cryptonote;

namespace
{
  //--------------------------------------------------------------------------------------------------------------------
  class fee : public ::testing::Test
  {
  };

  TEST_F(fee, 10xmr)
  {
    // CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 and lower are clamped
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2), 2000000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 2), 2000000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 100), 2000000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, 1), 2000000000);

    // higher is inverse proportional
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 2), 2000000000 / 2);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 10), 2000000000 / 10);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000), 2000000000 / 1000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000000ull), 2000000000 / 1000000);
  }

  TEST_F(fee, 1xmr)
  {
    // CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 and lower are clamped
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2), 200000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 2), 200000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 100), 200000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, 1), 200000000);

    // higher is inverse proportional
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 2), 200000000 / 2);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 10), 200000000 / 10);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000), 200000000 / 1000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000000ull), 200000000 / 1000000);
  }

  TEST_F(fee, dot3xmr)
  {
    // CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 and lower are clamped
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2), 60000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 2), 60000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 / 100), 60000000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, 1), 60000000);

    // higher is inverse proportional
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 2), 60000000 / 2);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 10), 60000000 / 10);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000), 60000000 / 1000);
    ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000000, CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000000ull), 60000000 / 1000000);
  }

  static bool is_more_or_less(double x, double y)
  {
    return fabs(y - x) < 0.001;
  }

  static const double MAX_MULTIPLIER = 166.f;

  TEST_F(fee, double_at_full)
  {
    static const uint64_t block_rewards[] = {
      20000000000000ull, // 20 monero
      13000000000000ull,
      1000000000000ull,
      600000000000ull, // .6 monero, minimum reward per block at 2min
      300000000000ull, // .3 monero, minimum reward per block at 1min
    };
    static const uint64_t median_block_sizes[] = {
      CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2,
      CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 2,
      CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 10,
      CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000,
      CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 1000000ull
    };

    for (uint64_t block_reward: block_rewards)
    {
      for (uint64_t median_block_size: median_block_sizes)
      {
        ASSERT_TRUE(is_more_or_less(Blockchain::get_dynamic_per_kb_fee(block_reward, median_block_size) * (median_block_size / 1024.) * MAX_MULTIPLIER / (double)block_reward, 1.992 * 1000 / 1024));
      }
    }
  }
}
