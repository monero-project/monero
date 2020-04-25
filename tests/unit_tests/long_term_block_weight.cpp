// Copyright (c) 2019, The Monero Project
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

#define IN_UNIT_TESTS

#include "gtest/gtest.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "blockchain_db/testdb.h"

#define TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW 5000

namespace
{

class TestDB: public cryptonote::BaseTestDB
{
private:
  struct block_t
  {
    size_t weight;
    uint64_t long_term_weight;
  };

public:
  TestDB() { m_open = true; }

  virtual void add_block( const cryptonote::block& blk
                        , size_t block_weight
                        , uint64_t long_term_block_weight
                        , const cryptonote::difficulty_type& cumulative_difficulty
                        , const uint64_t& coins_generated
                        , uint64_t num_rct_outs
                        , const crypto::hash& blk_hash
                        ) override {
    blocks.push_back({block_weight, long_term_block_weight});
  }
  virtual uint64_t height() const override { return blocks.size(); }
  virtual size_t get_block_weight(const uint64_t &h) const override { return blocks[h].weight; }
  virtual uint64_t get_block_long_term_weight(const uint64_t &h) const override { return blocks[h].long_term_weight; }
  virtual std::vector<uint64_t> get_block_weights(uint64_t start_height, size_t count) const override {
    std::vector<uint64_t> ret;
    ret.reserve(count);
    while (count-- && start_height < blocks.size()) ret.push_back(blocks[start_height++].weight);
    return ret;
  }
  virtual std::vector<uint64_t> get_long_term_block_weights(uint64_t start_height, size_t count) const override {
    std::vector<uint64_t> ret;
    ret.reserve(count);
    while (count-- && start_height < blocks.size()) ret.push_back(blocks[start_height++].long_term_weight);
    return ret;
  }
  virtual crypto::hash get_block_hash_from_height(const uint64_t &height) const override {
    crypto::hash hash = crypto::null_hash;
    *(uint64_t*)&hash = height;
    return hash;
  }
  virtual crypto::hash top_block_hash() const override {
    uint64_t h = height();
    crypto::hash top = crypto::null_hash;
    if (h)
      *(uint64_t*)&top = h - 1;
    return top;
  }
  virtual void pop_block(cryptonote::block &blk, std::vector<cryptonote::transaction> &txs) override { blocks.pop_back(); }

private:
  std::vector<block_t> blocks;
};

static uint32_t lcg_seed = 0;

static uint32_t lcg()
{
  lcg_seed = (lcg_seed * 0x100000001b3 + 0xcbf29ce484222325) & 0xffffffff;
  return lcg_seed;
}

}

#define PREFIX_WINDOW(hf_version,window) \
  std::unique_ptr<cryptonote::Blockchain> bc; \
  cryptonote::tx_memory_pool txpool(*bc); \
  bc.reset(new cryptonote::Blockchain(txpool)); \
  struct get_test_options { \
    const cryptonote::test_options::hard_fork_t hard_forks[3]; \
    const cryptonote::test_options test_options = { \
      hard_forks, \
      window, \
    }; \
    get_test_options(): hard_forks{{1, (uint64_t)0, 0}, {(uint8_t)hf_version, (uint64_t)1, 0}, {(uint8_t)0, (uint64_t)0,}} {} \
  } opts; \
  cryptonote::Blockchain *blockchain = bc.get(); \
  bool r = blockchain->init(new TestDB(), cryptonote::FAKECHAIN, true, &opts.test_options, 0); \
  ASSERT_TRUE(r)

#define PREFIX(hf_version) PREFIX_WINDOW(hf_version, TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW)

TEST(long_term_block_weight, empty_short)
{
  PREFIX(7);

  ASSERT_TRUE(bc->update_next_cumulative_weight_limit());

  ASSERT_EQ(bc->get_current_cumulative_block_weight_median(), CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1);
  ASSERT_EQ(bc->get_current_cumulative_block_weight_limit(), CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 * 2);
}

TEST(long_term_block_weight, identical_before_fork)
{
  PREFIX(7);

  for (uint64_t h = 1; h < 10 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    size_t w = h < CRYPTONOTE_REWARD_BLOCKS_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }
  for (uint64_t h = 0; h < 10 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    ASSERT_EQ(bc->get_db().get_block_long_term_weight(h), bc->get_db().get_block_weight(h));
  }
}

TEST(long_term_block_weight, identical_after_fork_before_long_term_window)
{
  PREFIX(8);

  for (uint64_t h = 1; h <= TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    size_t w = h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }
  for (uint64_t h = 0; h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    ASSERT_EQ(bc->get_db().get_block_long_term_weight(h), bc->get_db().get_block_weight(h));
  }
}

TEST(long_term_block_weight, ceiling_at_800000)
{
  PREFIX(8);

  for (uint64_t h = 0; h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW + TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 2 - 1; ++h)
  {
    size_t w = h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }
  ASSERT_EQ(bc->get_current_cumulative_block_weight_median(), 400000);
  ASSERT_EQ(bc->get_current_cumulative_block_weight_limit(), 800000);
}

TEST(long_term_block_weight, multi_pop)
{
  PREFIX(8);

  for (uint64_t h = 1; h <= TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW + 20; ++h)
  {
    size_t w = h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }

  const uint64_t effective_median = bc->get_current_cumulative_block_weight_median();
  const uint64_t effective_limit = bc->get_current_cumulative_block_weight_limit();

  const uint64_t num_pop = 4;
  for (uint64_t h = 0; h < num_pop; ++h)
  {
    size_t w = bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }

  cryptonote::block b;
  std::vector<cryptonote::transaction> txs;
  for (uint64_t h = 0; h < num_pop; ++h)
    bc->get_db().pop_block(b, txs);
  ASSERT_TRUE(bc->update_next_cumulative_weight_limit());

  ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
  ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
}

TEST(long_term_block_weight, multiple_updates)
{
  PREFIX(8);

  for (uint64_t h = 1; h <= 3 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    size_t w = h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    const uint64_t effective_median = bc->get_current_cumulative_block_weight_median();
    const uint64_t effective_limit = bc->get_current_cumulative_block_weight_limit();
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
    ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
    ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
    ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
  }
}

TEST(long_term_block_weight, pop_invariant_max)
{
  PREFIX(8);

  for (uint64_t h = 1; h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW - 10; ++h)
  {
    size_t w = bc->get_db().height() < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }

  for (int n = 0; n < 1000; ++n)
  {
    // pop some blocks, then add some more
    int remove = 1 + (n * 17) % 8;
    int add = (n * 23) % 12;

    // save long term block weights we're about to remove
    uint64_t old_ltbw[16], h0 = bc->get_db().height() - remove - 1;
    for (int i = -2; i < remove; ++i)
    {
      old_ltbw[i + 2] = bc->get_db().get_block_long_term_weight(h0 + i);
    }

    for (int i = 0; i < remove; ++i)
    {
      cryptonote::block b;
      std::vector<cryptonote::transaction> txs;
      bc->get_db().pop_block(b, txs);
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    }
    for (int i = 0; i < add; ++i)
    {
      size_t w = bc->get_db().height() < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : bc->get_current_cumulative_block_weight_limit();
      uint64_t ltw = bc->get_next_long_term_block_weight(w);
      bc->get_db().add_block(cryptonote::block(), w, ltw, bc->get_db().height(), bc->get_db().height(), {});
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
    }

    // check the new values are the same as the old ones
    for (int i = -2; i < std::min(add, remove); ++i)
    {
      ASSERT_EQ(bc->get_db().get_block_long_term_weight(h0 + i), old_ltbw[i + 2]);
    }
  }
}

TEST(long_term_block_weight, pop_invariant_random)
{
  PREFIX(8);

  for (uint64_t h = 1; h < 2 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW - 10; ++h)
  {
    lcg_seed = bc->get_db().height();
    uint32_t r = lcg();
    size_t w = bc->get_db().height() < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : (r % bc->get_current_cumulative_block_weight_limit());
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
  }

  for (int n = 0; n < 1000; ++n)
  {
    // pop some blocks, then add some more
    int remove = 1 + (n * 17) % 8;
    int add = (n * 23) % 123;

    // save long term block weights we're about to remove
    uint64_t old_ltbw[16], h0 = bc->get_db().height() - remove - 1;
    for (int i = -2; i < remove; ++i)
    {
      old_ltbw[i + 2] = bc->get_db().get_block_long_term_weight(h0 + i);
    }

    for (int i = 0; i < remove; ++i)
    {
      cryptonote::block b;
      std::vector<cryptonote::transaction> txs;
      bc->get_db().pop_block(b, txs);
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
      const uint64_t effective_median = bc->get_current_cumulative_block_weight_median();
      const uint64_t effective_limit = bc->get_current_cumulative_block_weight_limit();
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
      ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
      ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
    }
    for (int i = 0; i < add; ++i)
    {
      lcg_seed = bc->get_db().height();
      uint32_t r = lcg();
      size_t w = bc->get_db().height() < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW ? CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 : (r % bc->get_current_cumulative_block_weight_limit());
      uint64_t ltw = bc->get_next_long_term_block_weight(w);
      bc->get_db().add_block(cryptonote::block(), w, ltw, bc->get_db().height(), bc->get_db().height(), {});
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
      const uint64_t effective_median = bc->get_current_cumulative_block_weight_median();
      const uint64_t effective_limit = bc->get_current_cumulative_block_weight_limit();
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit());
      ASSERT_EQ(effective_median, bc->get_current_cumulative_block_weight_median());
      ASSERT_EQ(effective_limit, bc->get_current_cumulative_block_weight_limit());
    }

    // check the new values are the same as the old ones
    for (int i = -2; i < std::min(add, remove); ++i)
    {
      ASSERT_EQ(bc->get_db().get_block_long_term_weight(h0 + i), old_ltbw[i + 2]);
    }
  }
}

TEST(long_term_block_weight, long_growth_spike_and_drop)
{
  PREFIX(8);

  uint64_t long_term_effective_median_block_weight;

  // constant init
  for (uint64_t h = 0; h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    size_t w = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
  }
  ASSERT_EQ(long_term_effective_median_block_weight, 20000);

  // slow 10% yearly for a year (scaled down by 100000 / TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW) -> 6.19% change
  for (uint64_t h = 0; h < 365 * 360 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 100000; ++h)
  {
    //size_t w = bc->get_current_cumulative_block_weight_median() * rate;
    float t = h / float(365 * 360 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 100000);
    size_t w = 20000 + t * 2000;
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
  }
  ASSERT_GT(long_term_effective_median_block_weight, 20000 * 1.061);
  ASSERT_LT(long_term_effective_median_block_weight, 20000 * 1.062);

  // spike over three weeks - does not move much
  for (uint64_t h = 0; h < 21 * 360 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 100000; ++h)
  {
    size_t w = bc->get_current_cumulative_block_weight_limit();
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
  }
  ASSERT_GT(long_term_effective_median_block_weight, 20000 * 1.067);
  ASSERT_LT(long_term_effective_median_block_weight, 20000 * 1.068);

  // drop - does not move much
  for (uint64_t h = 0; h < 21 * 360 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 100000; ++h)
  {
    size_t w = bc->get_current_cumulative_block_weight_median() * .25;
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
  }
  ASSERT_GT(long_term_effective_median_block_weight, 20000 * 1.069);
  ASSERT_LT(long_term_effective_median_block_weight, 20000 * 1.07);
}

TEST(long_term_block_weight, long_growth_max)
{
  PREFIX(8);

  uint64_t long_term_effective_median_block_weight;

  // constant init
  for (uint64_t h = 0; h < TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    size_t w = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
    ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
  }
  ASSERT_EQ(long_term_effective_median_block_weight, 20000);

  const uint64_t expected_ltembs[10] = { 21600, 24000, 25600, 28000, 30400, 32000, 34400, 36800, 38400, 40800 };
  for (int k = 0; k < 10; ++k)
  {
    // max growth over a year
    for (uint64_t h = 0; h < 365 * 360 * TEST_LONG_TERM_BLOCK_WEIGHT_WINDOW / 100000; ++h)
    {
      size_t w = bc->get_current_cumulative_block_weight_limit();
      uint64_t ltw = bc->get_next_long_term_block_weight(w);
      bc->get_db().add_block(cryptonote::block(), w, ltw, h, h, {});
      ASSERT_TRUE(bc->update_next_cumulative_weight_limit(&long_term_effective_median_block_weight));
    }
    ASSERT_EQ(long_term_effective_median_block_weight, expected_ltembs[k]);
  }
}
