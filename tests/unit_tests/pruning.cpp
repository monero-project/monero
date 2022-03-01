// Copyright (c) 2018-2022, The Monero Project

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

#include "misc_log_ex.h"
#include "cryptonote_config.h"
#include "common/pruning.h"

#define ASSERT_EX(x) do { bool ex = false; try { x; } catch(...) { ex = true; }  ASSERT_TRUE(ex); } while(0)

TEST(pruning, parts)
{
  ASSERT_EQ(tools::get_pruning_stripe(tools::make_pruning_seed(3, 2)), 3);
  ASSERT_EQ(tools::get_pruning_stripe(tools::make_pruning_seed(1, 2)), 1);
  ASSERT_EQ(tools::get_pruning_stripe(tools::make_pruning_seed(7, 7)), 7);

  ASSERT_EQ(tools::get_pruning_log_stripes(tools::make_pruning_seed(1, 2)), 2);
  ASSERT_EQ(tools::get_pruning_log_stripes(tools::make_pruning_seed(1, 0)), 0);
  ASSERT_EQ(tools::get_pruning_log_stripes(tools::make_pruning_seed(1, 7)), 7);
  ASSERT_EQ(tools::get_pruning_log_stripes(tools::make_pruning_seed(7, 7)), 7);

  for (uint32_t log_stripes = 1; log_stripes <= tools::PRUNING_SEED_LOG_STRIPES_MASK; ++log_stripes)
  {
    for (uint32_t stripe = 1; stripe <= (1 << log_stripes); ++stripe)
    {
      ASSERT_EQ(tools::get_pruning_log_stripes(tools::make_pruning_seed(stripe, log_stripes)), log_stripes);
      ASSERT_EQ(tools::get_pruning_stripe(tools::make_pruning_seed(stripe, log_stripes)), stripe);
    }
  }
}

TEST(pruning, out_of_range)
{
  ASSERT_EX(tools::make_pruning_seed(5, 2));
  ASSERT_EX(tools::make_pruning_seed(0, 2));
}

TEST(pruning, fits)
{
  const uint32_t log_stripes = 3;
  const uint32_t num_stripes = 1 << log_stripes;
  for (uint32_t stripe = 1; stripe <= num_stripes; ++stripe)
  {
    const uint32_t seed = tools::make_pruning_seed(stripe, log_stripes);
    ASSERT_NE(seed, 0);
    ASSERT_EQ(tools::get_pruning_log_stripes(seed), log_stripes);
    ASSERT_EQ(tools::get_pruning_stripe(seed), stripe);
  }
}

TEST(pruning, tip)
{
  static constexpr uint64_t H = CRYPTONOTE_PRUNING_TIP_BLOCKS + 1000;
  static_assert(H >= CRYPTONOTE_PRUNING_TIP_BLOCKS, "H must be >= CRYPTONOTE_PRUNING_TIP_BLOCKS");
  for (uint64_t h = H - CRYPTONOTE_PRUNING_TIP_BLOCKS; h < H; ++h)
  {
    uint32_t pruning_seed = tools::get_pruning_seed(h, H, CRYPTONOTE_PRUNING_LOG_STRIPES);
    ASSERT_EQ(pruning_seed, 0);
    for (pruning_seed = 0; pruning_seed <= (1 << CRYPTONOTE_PRUNING_LOG_STRIPES); ++pruning_seed)
      ASSERT_TRUE(tools::has_unpruned_block(h, H, pruning_seed));
  }
}

TEST(pruning, seed)
{
  const uint64_t SS = CRYPTONOTE_PRUNING_STRIPE_SIZE;
  const uint64_t NS = 1 << CRYPTONOTE_PRUNING_LOG_STRIPES;
  const uint64_t TB = NS * SS;

  for (uint64_t cycle = 0; cycle < 10; ++cycle)
  {
    const uint64_t O = TB * cycle;
    ASSERT_EQ(tools::get_pruning_stripe(O + 0,       10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 1);
    ASSERT_EQ(tools::get_pruning_stripe(O + 1,       10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 1);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS-1,    10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 1);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS,      10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 2);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS*2-1,  10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 2);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS*2,    10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 3);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS*NS-1, 10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), NS);
    ASSERT_EQ(tools::get_pruning_stripe(O + SS*NS,   10000000, CRYPTONOTE_PRUNING_LOG_STRIPES), 1);
  }
}

TEST(pruning, match)
{
  static constexpr uint64_t H = CRYPTONOTE_PRUNING_TIP_BLOCKS + 1000;
  static_assert(H >= CRYPTONOTE_PRUNING_TIP_BLOCKS, "H must be >= CRYPTONOTE_PRUNING_TIP_BLOCKS");
  for (uint64_t h = 0; h < H - CRYPTONOTE_PRUNING_TIP_BLOCKS; ++h)
  {
    uint32_t pruning_seed = tools::get_pruning_seed(h, H, CRYPTONOTE_PRUNING_LOG_STRIPES);
    uint32_t pruning_stripe = tools::get_pruning_stripe(pruning_seed);
    ASSERT_TRUE(pruning_stripe > 0 && pruning_stripe <= (1 << CRYPTONOTE_PRUNING_LOG_STRIPES));
    for (uint32_t other_pruning_stripe = 1; other_pruning_stripe <= (1 << CRYPTONOTE_PRUNING_LOG_STRIPES); ++other_pruning_stripe)
    {
      uint32_t other_pruning_seed = tools::make_pruning_seed(other_pruning_stripe, CRYPTONOTE_PRUNING_LOG_STRIPES);
      ASSERT_TRUE(tools::has_unpruned_block(h, H, other_pruning_seed) == (other_pruning_seed == pruning_seed));
    }
  }
}

TEST(pruning, stripe_size)
{
  static constexpr uint64_t H = CRYPTONOTE_PRUNING_TIP_BLOCKS + CRYPTONOTE_PRUNING_STRIPE_SIZE * (1 << CRYPTONOTE_PRUNING_LOG_STRIPES) + 1000;
  static_assert(H >= CRYPTONOTE_PRUNING_TIP_BLOCKS + CRYPTONOTE_PRUNING_STRIPE_SIZE * (1 << CRYPTONOTE_PRUNING_LOG_STRIPES), "H must be >= that stuff in front");
  for (uint32_t pruning_stripe = 1; pruning_stripe <= (1 << CRYPTONOTE_PRUNING_LOG_STRIPES); ++pruning_stripe)
  {
    uint32_t pruning_seed = tools::make_pruning_seed(pruning_stripe, CRYPTONOTE_PRUNING_LOG_STRIPES);
    unsigned int current_run = 0, best_run = 0;
    for (uint64_t h = 0; h < H - CRYPTONOTE_PRUNING_TIP_BLOCKS; ++h)
    {
      if (tools::has_unpruned_block(h, H, pruning_seed))
      {
        ++current_run;
      }
      else if (current_run)
      {
        ASSERT_EQ(current_run, CRYPTONOTE_PRUNING_STRIPE_SIZE);
        best_run = std::max(best_run, current_run);
        current_run = 0;
      }
    }
    ASSERT_EQ(best_run, CRYPTONOTE_PRUNING_STRIPE_SIZE);
  }
}

TEST(pruning, next_unpruned)
{
  static_assert((1 << CRYPTONOTE_PRUNING_LOG_STRIPES) >= 4, "CRYPTONOTE_PRUNING_LOG_STRIPES too low");

  const uint64_t SS = CRYPTONOTE_PRUNING_STRIPE_SIZE;
  const uint64_t NS = 1 << CRYPTONOTE_PRUNING_LOG_STRIPES;
  const uint64_t TB = NS * SS;

  for (uint64_t h = 0; h < 100; ++h)
    ASSERT_EQ(tools::get_next_unpruned_block_height(h, 10000000, 0), h);

  const uint32_t seed1 = tools::make_pruning_seed(1, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seed2 = tools::make_pruning_seed(2, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seed3 = tools::make_pruning_seed(3, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seed4 = tools::make_pruning_seed(4, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seedNS = tools::make_pruning_seed(NS, CRYPTONOTE_PRUNING_LOG_STRIPES);

  ASSERT_EQ(tools::get_next_unpruned_block_height(0,      10000000, seed1), 0);
  ASSERT_EQ(tools::get_next_unpruned_block_height(1,      10000000, seed1), 1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(SS-1,   10000000, seed1), SS-1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(SS,     10000000, seed1), TB);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB,     10000000, seed1), TB);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB-1,   10000000, seed1), TB);

  ASSERT_EQ(tools::get_next_unpruned_block_height(0,      10000000, seed2), SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(1,      10000000, seed2), SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(SS-1,   10000000, seed2), SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(SS,     10000000, seed2), SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(2*SS-1, 10000000, seed2), 2*SS-1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(2*SS,   10000000, seed2), TB+SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB+2*SS,10000000, seed2), TB*2+SS);

  ASSERT_EQ(tools::get_next_unpruned_block_height(0,      10000000, seed3), SS*2);
  ASSERT_EQ(tools::get_next_unpruned_block_height(SS,     10000000, seed3), SS*2);
  ASSERT_EQ(tools::get_next_unpruned_block_height(2*SS,   10000000, seed3), SS*2);
  ASSERT_EQ(tools::get_next_unpruned_block_height(3*SS-1, 10000000, seed3), SS*3-1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(3*SS,   10000000, seed3), TB+SS*2);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB+3*SS,10000000, seed3), TB*2+SS*2);

  ASSERT_EQ(tools::get_next_unpruned_block_height(SS,     10000000, seed4), 3*SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(4*SS-1, 10000000, seed4), 4*SS-1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(4*SS,   10000000, seed4), TB+3*SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB+4*SS,10000000, seed4), TB*2+3*SS);

  ASSERT_EQ(tools::get_next_unpruned_block_height(SS,     10000000, seedNS), (NS-1)*SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(NS*SS-1,10000000, seedNS), NS*SS-1);
  ASSERT_EQ(tools::get_next_unpruned_block_height(NS*SS,  10000000, seedNS), TB+(NS-1)*SS);
  ASSERT_EQ(tools::get_next_unpruned_block_height(TB+NS*SS,  10000000, seedNS), TB*2+(NS-1)*SS);
}

TEST(pruning, next_pruned)
{
  static_assert((1 << CRYPTONOTE_PRUNING_LOG_STRIPES) >= 4, "CRYPTONOTE_PRUNING_LOG_STRIPES too low");

  const uint64_t SS = CRYPTONOTE_PRUNING_STRIPE_SIZE;
  const uint64_t NS = 1 << CRYPTONOTE_PRUNING_LOG_STRIPES;
  const uint64_t TB = NS * SS;

  const uint32_t seed1 = tools::make_pruning_seed(1, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seed2 = tools::make_pruning_seed(2, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seedNS_1 = tools::make_pruning_seed(NS-1, CRYPTONOTE_PRUNING_LOG_STRIPES);
  const uint32_t seedNS = tools::make_pruning_seed(NS, CRYPTONOTE_PRUNING_LOG_STRIPES);

  for (uint64_t h = 0; h < 100; ++h)
    ASSERT_EQ(tools::get_next_pruned_block_height(h, 10000000, 0), 10000000);
  for (uint64_t h = 10000000 - 1 - CRYPTONOTE_PRUNING_TIP_BLOCKS; h < 10000000; ++h)
    ASSERT_EQ(tools::get_next_pruned_block_height(h, 10000000, 0), 10000000);

  ASSERT_EQ(tools::get_next_pruned_block_height(1,    10000000, seed1), SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS-1, 10000000, seed1), SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS,   10000000, seed1), SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(TB-1, 10000000, seed1), TB-1);
  ASSERT_EQ(tools::get_next_pruned_block_height(TB,   10000000, seed1), TB+SS);

  ASSERT_EQ(tools::get_next_pruned_block_height(1,    10000000, seed2), 1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS-1, 10000000, seed2), SS-1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS,   10000000, seed2), 2*SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(TB-1, 10000000, seed2), TB-1);

  ASSERT_EQ(tools::get_next_pruned_block_height(1,    10000000, seedNS_1), 1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS-1, 10000000, seedNS_1), SS-1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS,   10000000, seedNS_1), SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(TB-1, 10000000, seedNS_1), TB-1);

  ASSERT_EQ(tools::get_next_pruned_block_height(1,    10000000, seedNS), 1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS-1, 10000000, seedNS), SS-1);
  ASSERT_EQ(tools::get_next_pruned_block_height(SS,   10000000, seedNS), SS);
  ASSERT_EQ(tools::get_next_pruned_block_height(TB-1, 10000000, seedNS), TB);
}
