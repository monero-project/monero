// Copyright (c) 2014-2018, The Monero Project
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

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

using namespace cryptonote;

namespace
{
  //--------------------------------------------------------------------------------------------------------------------
  class block_reward_and_already_generated_coins : public ::testing::Test
  {
  protected:
    static const size_t current_block_weight = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 / 2;

    union
    {
      bool m_block_not_too_big;
      bool m_block_reward_calc_success;
    };
    uint64_t m_block_reward;
  };

  #define TEST_ALREADY_GENERATED_COINS(already_generated_coins, expected_reward)                                \
    m_block_not_too_big = get_base_block_reward(0, current_block_weight, already_generated_coins, m_block_reward,7,0); \
    ASSERT_TRUE(m_block_not_too_big);                                                                           \
    ASSERT_EQ(m_block_reward, expected_reward);

  #define TEST_ALREADY_GENERATED_COINS_V2(already_generated_coins, expected_reward, h)                          \
    m_block_not_too_big = get_base_block_reward(0, current_block_weight, already_generated_coins, m_block_reward,8,h); \
    ASSERT_TRUE(m_block_not_too_big);                                                                           \
    ASSERT_EQ(m_block_reward, expected_reward);

  TEST_F(block_reward_and_already_generated_coins, handles_first_values)
  {
    TEST_ALREADY_GENERATED_COINS(0, UINT64_C(22500000000000000));
    TEST_ALREADY_GENERATED_COINS(m_block_reward, UINT64_C(122740188075));
    TEST_ALREADY_GENERATED_COINS(UINT64_C(2756434948434199641), 0);
  }

  TEST_F(block_reward_and_already_generated_coins, correctly_does_new_emissions_curve)
  {
    uint64_t supply = UINT64_C(22500000000000000)+116*720*90;
    TEST_ALREADY_GENERATED_COINS_V2(supply, UINT64_C(78255231055), 64324);
    TEST_ALREADY_GENERATED_COINS_V2(supply, UINT64_C(53000000000), 129600);
    TEST_ALREADY_GENERATED_COINS_V2(supply, UINT64_C(28390625000), 518400);
    TEST_ALREADY_GENERATED_COINS_V2(supply, UINT64_C(28000095367), 1296000);
  }

  TEST_F(block_reward_and_already_generated_coins, correctly_steps_from_2_to_1)
  {
    TEST_ALREADY_GENERATED_COINS(((uint64_t)(1) << 58)-1, 0);
    TEST_ALREADY_GENERATED_COINS(((uint64_t)(1) << 58)  , 0);
    TEST_ALREADY_GENERATED_COINS(((uint64_t)(1) << 58)+1, 0);
  }

  TEST_F(block_reward_and_already_generated_coins, handles_max)
  {
    TEST_ALREADY_GENERATED_COINS(MONEY_SUPPLY - ((1 << 20) + 1), UINT64_C(0));
    TEST_ALREADY_GENERATED_COINS(MONEY_SUPPLY -  (1 << 20)     , UINT64_C(0));
    TEST_ALREADY_GENERATED_COINS(MONEY_SUPPLY - ((1 << 20) - 1), UINT64_C(0));
  }

  TEST_F(block_reward_and_already_generated_coins, reward_parity_between_orig_and_loki_algo)
  {
    uint64_t already_generated_coins = UINT64_C(22500000000000000)+116*720*90;
    m_block_reward_calc_success      = true;

    cryptonote::block_reward_parts        reward_parts_v7   = {};
    cryptonote::loki_block_reward_context reward_context_v7 = {};
    m_block_reward_calc_success &= get_loki_block_reward(0, current_block_weight, already_generated_coins, 7, reward_parts_v7, reward_context_v7);

    cryptonote::block_reward_parts        reward_parts_v8   = {};
    cryptonote::loki_block_reward_context reward_context_v8 = {};
    m_block_reward_calc_success &= get_loki_block_reward(0, current_block_weight, already_generated_coins, 8, reward_parts_v8, reward_context_v8);

    cryptonote::block_reward_parts        reward_parts_v9   = {};
    cryptonote::loki_block_reward_context reward_context_v9 = {};
    m_block_reward_calc_success &= get_loki_block_reward(0, current_block_weight, already_generated_coins, 9, reward_parts_v9, reward_context_v9);

    uint64_t reward_v7_orig = 0;
    m_block_reward_calc_success &= cryptonote::get_base_block_reward(0, current_block_weight, already_generated_coins, reward_v7_orig, 7, 0);

    uint64_t reward_v8_orig = 0;
    m_block_reward_calc_success &= cryptonote::get_base_block_reward(0, current_block_weight, already_generated_coins, reward_v8_orig, 8, 0);

    uint64_t reward_v9_orig = 0;
    m_block_reward_calc_success &= cryptonote::get_base_block_reward(0, current_block_weight, already_generated_coins, reward_v9_orig, 9, 0);

    ASSERT_TRUE(m_block_reward_calc_success);                                                                           \
    ASSERT_EQ(reward_parts_v7.adjusted_base_reward, reward_v7_orig);
    ASSERT_EQ(reward_parts_v8.adjusted_base_reward, reward_v8_orig);
    ASSERT_EQ(reward_parts_v9.adjusted_base_reward, reward_v9_orig);
  }

  //--------------------------------------------------------------------------------------------------------------------
  // TODO(doyle): The following tests, test using CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 which is not relevant to us.
}
