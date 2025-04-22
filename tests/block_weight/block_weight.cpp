// Copyright (c) 2019-2024, The Monero Project
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

#include <stdio.h>
#include <math.h>
#include "cryptonote_core/cryptonote_core.h"
#include "blockchain_db/testdb.h"
#include "fcmp_pp/curve_trees.h"

#define LONG_TERM_BLOCK_WEIGHT_WINDOW 5000

enum test_t
{
  test_max = 0,
  test_lcg = 1,
  test_min = 2,
};

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
                        , const fcmp_pp::curve_trees::OutsByLastLockedBlock& outs_by_last_locked_block
                        , const std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/>& timelocked_outputs
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
  virtual crypto::hash top_block_hash(uint64_t *block_height = NULL) const override {
    uint64_t h = height();
    crypto::hash top = crypto::null_hash;
    if (h)
      *(uint64_t*)&top = h - 1;
    if (block_height)
      *block_height = h - 1;
    return top;
  }
  virtual void pop_block(cryptonote::block &blk, std::vector<cryptonote::transaction> &txs) override { blocks.pop_back(); }
  virtual void set_hard_fork_version(uint64_t height, uint8_t version) override { if (height >= hf.size()) hf.resize(height + 1); hf[height] = version; }
  virtual uint8_t get_hard_fork_version(uint64_t height) const override { if (height >= hf.size()) return 255; return hf[height]; }

private:
  std::vector<block_t> blocks;
  std::vector<uint8_t> hf;
};

}

#define PREFIX_WINDOW(hf_version,window) \
  struct get_test_options { \
    const std::pair<uint8_t, uint64_t> hard_forks[3]; \
    const cryptonote::test_options test_options = { \
      hard_forks, \
      window, \
    }; \
    get_test_options(): hard_forks{std::make_pair(1, (uint64_t)0), std::make_pair((uint8_t)hf_version, (uint64_t)LONG_TERM_BLOCK_WEIGHT_WINDOW), std::make_pair((uint8_t)0, (uint64_t)0)} {} \
  } opts; \
  cryptonote::BlockchainAndPool bap; \
  cryptonote::Blockchain *blockchain = &bap.blockchain; \
  cryptonote::Blockchain *bc = blockchain; \
  bool r = blockchain->init(new TestDB(), cryptonote::FAKECHAIN, true, &opts.test_options, 0, NULL); \
  if (!r) \
  { \
    fprintf(stderr, "Failed to init blockchain\n"); \
    exit(1); \
  }

#define PREFIX(hf_version) PREFIX_WINDOW(hf_version, LONG_TERM_BLOCK_WEIGHT_WINDOW)

static uint32_t lcg_seed = 0;

static uint32_t lcg()
{
  lcg_seed = (lcg_seed * 0x100000001b3 + 0xcbf29ce484222325) & 0xffffffff;
  return lcg_seed;
}

static void test(test_t t, uint64_t blocks)
{
  PREFIX(HF_VERSION_2021_SCALING);

  for (uint64_t h = 0; h < LONG_TERM_BLOCK_WEIGHT_WINDOW; ++h)
  {
    cryptonote::block b;
    b.major_version = 1;
    b.minor_version = 1;
    bc->get_db().add_block(std::make_pair(b, ""), 300000, 300000, bc->get_db().height(), bc->get_db().height(), {});
    if (!bc->update_next_cumulative_weight_limit())
    {
      fprintf(stderr, "Failed to update cumulative weight limit 1\n");
      exit(1);
    }
  }

  for (uint64_t h = 0; h < blocks; ++h)
  {
    uint64_t w;
    uint64_t effective_block_weight_median = bc->get_current_cumulative_block_weight_median();
    switch (t)
    {
      case test_lcg:
      {
        uint32_t r = lcg();
        int64_t wi = 90 + r % 500000 + 250000 + sin(h / 200.) * 350000;
        w = wi < 90 ? 90 : wi;
        break;
      }
      case test_max:
        w = bc->get_current_cumulative_block_weight_limit();
        break;
      case test_min:
        w = 90;
        break;
      default:
        exit(1);
    }
    uint64_t ltw = bc->get_next_long_term_block_weight(w);
    cryptonote::block b;
    b.major_version = HF_VERSION_2021_SCALING;
    b.minor_version = HF_VERSION_2021_SCALING;
    bc->get_db().add_block(std::make_pair(std::move(b), ""), w, ltw, bc->get_db().height(), bc->get_db().height(), {});

    if (!bc->update_next_cumulative_weight_limit())
    {
      fprintf(stderr, "Failed to update cumulative weight limit\n");
      exit(1);
    }
    std::cout << "H " << h << ", BW " << w << ", EMBW " << effective_block_weight_median << ", LTBW " << ltw << std::endl;
  }
}

int main()
{
  TRY_ENTRY();
  test(test_max, 2 * LONG_TERM_BLOCK_WEIGHT_WINDOW);
  test(test_lcg, 9 * LONG_TERM_BLOCK_WEIGHT_WINDOW);
  test(test_min, 1 * LONG_TERM_BLOCK_WEIGHT_WINDOW);
  return 0;
  CATCH_ENTRY_L0("main", 1);
}
