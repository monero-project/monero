// Copyright (c) 2014-2024, The Monero Project
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

#include <algorithm>
#include "gtest/gtest.h"

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/hardfork.h"
#include "blockchain_db/testdb.h"
#include "fcmp_pp/curve_trees.h"

using namespace cryptonote;

#define BLOCKS_PER_YEAR 525960
#define SECONDS_PER_YEAR 31557600

namespace
{

class TestDB: public cryptonote::BaseTestDB {
public:
  virtual uint64_t height() const override { return blocks.size(); }
  virtual void add_block( const block& blk
                        , size_t block_weight
                        , uint64_t long_term_block_weight
                        , const difficulty_type& cumulative_difficulty
                        , const uint64_t& coins_generated
                        , uint64_t num_rct_outs
                        , const crypto::hash& blk_hash
                        ) override {
    blocks.push_back(blk);
  }
  virtual void remove_block() override { blocks.pop_back(); }
  virtual block get_block_from_height(const uint64_t& height) const override {
    return blocks.at(height);
  }
  virtual void set_hard_fork_version(uint64_t height, uint8_t version) override {
    if (versions.size() <= height) 
      versions.resize(height+1); 
    versions[height] = version;
  }
  virtual uint8_t get_hard_fork_version(uint64_t height) const override {
    return versions.at(height);
  }

private:
  std::vector<block> blocks;
  std::deque<uint8_t> versions;
};

}

static cryptonote::block mkblock(uint8_t version, uint8_t vote)
{
  cryptonote::block b;
  b.major_version = version;
  b.minor_version = vote;
  return b;
}

static cryptonote::block mkblock(const HardFork &hf, uint64_t height, uint8_t vote)
{
  cryptonote::block b;
  b.major_version = hf.get(height);
  b.minor_version = vote;
  return b;
}

TEST(major, Only)
{
  TestDB db;
  HardFork hf(db, 1, 0, 0, 0, 1, 0); // no voting

  //                      v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 2, 1));
  hf.init();

  // block height 0, only version 1 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 0));
  ASSERT_FALSE(hf.add(mkblock(2, 2), 0));
  ASSERT_TRUE(hf.add(mkblock(1, 2), 0));
  db.add_block(mkblock(1, 1), 0, 0, 0, 0, 0, crypto::hash());

  // block height 1, only version 1 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 1));
  ASSERT_FALSE(hf.add(mkblock(2, 2), 1));
  ASSERT_TRUE(hf.add(mkblock(1, 2), 1));
  db.add_block(mkblock(1, 1), 0, 0, 0, 0, 0, crypto::hash());

  // block height 2, only version 2 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 2));
  ASSERT_FALSE(hf.add(mkblock(1, 2), 2));
  ASSERT_FALSE(hf.add(mkblock(3, 2), 2));
  ASSERT_TRUE(hf.add(mkblock(2, 2), 2));
  db.add_block(mkblock(2, 1), 0, 0, 0, 0, 0, crypto::hash());
}

TEST(empty_hardforks, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  hf.init();
  ASSERT_TRUE(hf.get_state(time(NULL)) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(time(NULL) + 3600*24*400) == HardFork::Ready);

  for (uint64_t h = 0; h <= 10; ++h) {
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
  ASSERT_EQ(hf.get(0), 1);
  ASSERT_EQ(hf.get(1), 1);
  ASSERT_EQ(hf.get(10), 1);
}

TEST(ordering, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(2, 2, 1));
  ASSERT_FALSE(hf.add_fork(3, 3, 1));
  ASSERT_FALSE(hf.add_fork(3, 2, 2));
  ASSERT_FALSE(hf.add_fork(2, 3, 2));
  ASSERT_TRUE(hf.add_fork(3, 10, 2));
  ASSERT_TRUE(hf.add_fork(4, 20, 3));
  ASSERT_FALSE(hf.add_fork(5, 5, 4));
}

TEST(check_for_height, Success)
{
  TestDB db;
  HardFork hf(db, 1, 0, 0, 0, 1, 0); // no voting

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 5, 1));
  hf.init();

  for (uint64_t h = 0; h <= 4; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 1), h));
    ASSERT_FALSE(hf.check_for_height(mkblock(2, 2), h));  // block version is too high
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 5; h <= 10; ++h) {
    ASSERT_FALSE(hf.check_for_height(mkblock(1, 1), h));  // block version is too low
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), h));
    db.add_block(mkblock(hf, h, 2), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(get, next_version)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 5, 1));
  ASSERT_TRUE(hf.add_fork(4, 10, 2));
  hf.init();

  for (uint64_t h = 0; h <= 4; ++h) {
    ASSERT_EQ(2, hf.get_next_version());
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 5; h <= 9; ++h) {
    ASSERT_EQ(4, hf.get_next_version());
    db.add_block(mkblock(hf, h, 2), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 10; h <= 15; ++h) {
    ASSERT_EQ(4, hf.get_next_version());
    db.add_block(mkblock(hf, h, 4), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(states, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, BLOCKS_PER_YEAR, SECONDS_PER_YEAR));

  ASSERT_TRUE(hf.get_state(0) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_UPDATE_TIME / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + (HardFork::DEFAULT_UPDATE_TIME + HardFork::DEFAULT_FORKED_TIME) / 2) == HardFork::UpdateNeeded);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_FORKED_TIME * 2) == HardFork::LikelyForked);

  ASSERT_TRUE(hf.add_fork(3, BLOCKS_PER_YEAR * 5, SECONDS_PER_YEAR * 5));

  ASSERT_TRUE(hf.get_state(0) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_UPDATE_TIME / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + (HardFork::DEFAULT_UPDATE_TIME + HardFork::DEFAULT_FORKED_TIME) / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_FORKED_TIME * 2) == HardFork::Ready);
}

TEST(steps_asap, Success)
{
  TestDB db;
  HardFork hf(db, 1,0,1,1,1);

  //                 v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(4, 2, 1));
  ASSERT_TRUE(hf.add_fork(7, 4, 2));
  ASSERT_TRUE(hf.add_fork(9, 6, 3));
  hf.init();

  for (uint64_t h = 0; h < 10; ++h) {
    db.add_block(mkblock(hf, h, 9), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  ASSERT_EQ(hf.get(0), 1);
  ASSERT_EQ(hf.get(1), 1);
  ASSERT_EQ(hf.get(2), 4);
  ASSERT_EQ(hf.get(3), 4);
  ASSERT_EQ(hf.get(4), 7);
  ASSERT_EQ(hf.get(5), 7);
  ASSERT_EQ(hf.get(6), 9);
  ASSERT_EQ(hf.get(7), 9);
  ASSERT_EQ(hf.get(8), 9);
  ASSERT_EQ(hf.get(9), 9);
}

TEST(steps_1, Success)
{
  TestDB db;
  HardFork hf(db, 1,0,1,1,1);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  for (int n = 1 ; n < 10; ++n)
    ASSERT_TRUE(hf.add_fork(n+1, n, n));
  hf.init();

  for (uint64_t h = 0 ; h < 10; ++h) {
    db.add_block(mkblock(hf, h, h+1), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 0; h < 10; ++h) {
    ASSERT_EQ(hf.get(h), std::max(1,(int)h));
  }
}

TEST(reorganize, Same)
{
  for (int history = 1; history <= 12; ++history) {
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, history, 100);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(4, 2, 1));
    ASSERT_TRUE(hf.add_fork(7, 4, 2));
    ASSERT_TRUE(hf.add_fork(9, 6, 3));
    hf.init();

    //                                 index  0  1  2  3  4  5  6  7  8  9
    static const uint8_t block_versions[] = { 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
    for (uint64_t h = 0; h < 20; ++h) {
      db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, 0, crypto::hash());
      ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
    }

    for (uint64_t rh = 0; rh < 20; ++rh) {
      hf.reorganize_from_block_height(rh);
      for (int hh = 0; hh < 20; ++hh) {
        uint8_t version = hh >= history ? block_versions[hh - history] : 1;
        ASSERT_EQ(hf.get(hh), version);
      }
    }
  }
}

TEST(reorganize, Changed)
{
  TestDB db;
  HardFork hf(db, 1, 0, 1, 1, 4, 100);

  //                 v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(4, 2, 1));
  ASSERT_TRUE(hf.add_fork(7, 4, 2));
  ASSERT_TRUE(hf.add_fork(9, 6, 3));
  hf.init();

  //                                    fork         4     7     9
  //                                    index  0  1  2  3  4  5  6  7  8  9
  static const uint8_t block_versions[] =    { 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
  static const uint8_t expected_versions[] = { 1, 1, 1, 1, 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9 };
  for (uint64_t h = 0; h < 16; ++h) {
    db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE (hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t rh = 0; rh < 16; ++rh) {
    hf.reorganize_from_block_height(rh);
    for (int hh = 0; hh < 16; ++hh) {
      ASSERT_EQ(hf.get(hh), expected_versions[hh]);
    }
  }

  // delay a bit for 9, and go back to 1 to check it stays at 9
  static const uint8_t block_versions_new[] =    { 1, 1, 4, 4, 7, 7, 4, 7, 7, 7, 9, 9, 9, 9, 9, 1 };
  static const uint8_t expected_versions_new[] = { 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 7, 7, 7, 9, 9 };
  for (uint64_t h = 3; h < 16; ++h) {
    db.remove_block();
  }
  ASSERT_EQ(db.height(), 3);
  hf.reorganize_from_block_height(2);
  for (uint64_t h = 3; h < 16; ++h) {
    db.add_block(mkblock(hf, h, block_versions_new[h]), 0, 0, 0, 0, 0, crypto::hash());
    bool ret = hf.add(db.get_block_from_height(h), h);
    ASSERT_EQ (ret, h < 15);
  }
  db.remove_block(); // last block added to the blockchain, but not hf
  ASSERT_EQ(db.height(), 15);
  for (int hh = 0; hh < 15; ++hh) {
    ASSERT_EQ(hf.get(hh), expected_versions_new[hh]);
  }
}

TEST(voting, threshold)
{
  for (int threshold = 87; threshold <= 88; ++threshold) {
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 8, threshold);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    hf.init();

    for (uint64_t h = 0; h <= 8; ++h) {
      uint8_t v = 1 + !!(h % 8);
      db.add_block(mkblock(hf, h, v), 0, 0, 0, 0, 0, crypto::hash());
      bool ret = hf.add(db.get_block_from_height(h), h);
      if (h >= 8 && threshold == 87) {
        // for threshold 87, we reach the threshold at height 7, so from height 8, hard fork to version 2, but 8 tries to add 1
        ASSERT_FALSE(ret);
      }
      else {
        // for threshold 88, we never reach the threshold
        ASSERT_TRUE(ret);
        uint8_t expected = threshold == 88 ? 1 : h < 8 ? 1 : 2;
        ASSERT_EQ(hf.get(h), expected);
      }
    }
  }
}

TEST(voting, different_thresholds)
{
  for (int threshold = 87; threshold <= 88; ++threshold) {
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50); // window size 4

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 5, 0, 1)); // asap
    ASSERT_TRUE(hf.add_fork(3, 10, 100, 2)); // all votes
    ASSERT_TRUE(hf.add_fork(4, 15, 3)); // default 50% votes
    hf.init();

    //                                           0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9
    static const uint8_t block_versions[] =    { 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4 };
    static const uint8_t expected_versions[] = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };

    for (uint64_t h = 0; h < sizeof(block_versions) / sizeof(block_versions[0]); ++h) {
      db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, 0, crypto::hash());
      bool ret = hf.add(db.get_block_from_height(h), h);
      ASSERT_EQ(ret, true);
    }
    for (uint64_t h = 0; h < sizeof(expected_versions) / sizeof(expected_versions[0]); ++h) {
      ASSERT_EQ(hf.get(h), expected_versions[h]);
    }
  }
}

TEST(voting, info)
{
  TestDB db;
  HardFork hf(db, 1, 0, 1, 1, 4, 50); // window size 4, default threshold 50%

  //                      v  h  ts
  ASSERT_TRUE(hf.add_fork(1, 0,  0));
  //                      v  h   thr  ts
  ASSERT_TRUE(hf.add_fork(2, 5,    0,  1)); // asap
  ASSERT_TRUE(hf.add_fork(3, 10, 100,  2)); // all votes
  //                      v   h  ts
  ASSERT_TRUE(hf.add_fork(4, 15,  3)); // default 50% votes
  hf.init();

  //                                             0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9
  static const uint8_t block_versions[]      = { 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4 };
  static const uint8_t expected_thresholds[] = { 0, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 2, 2, 2, 2 };

  for (uint64_t h = 0; h < sizeof(block_versions) / sizeof(block_versions[0]); ++h) {
    uint32_t window, votes, threshold;
    uint64_t earliest_height;
    uint8_t voting;

    ASSERT_TRUE(hf.get_voting_info(1, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h, 4), votes);
    ASSERT_EQ(0, earliest_height);

    ASSERT_EQ(hf.get_current_version() >= 2, hf.get_voting_info(2, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 3 ? 0 : h - 3, 4), votes);
    ASSERT_EQ(5, earliest_height);

    ASSERT_EQ(hf.get_current_version() >= 3, hf.get_voting_info(3, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 8 ? 0 : h - 8, 4), votes);
    ASSERT_EQ(10, earliest_height);

    ASSERT_EQ(hf.get_current_version() == 4, hf.get_voting_info(4, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 14 ? 0 : h - 14, 4), votes);
    ASSERT_EQ(15, earliest_height);

    ASSERT_EQ(std::min<uint64_t>(h, 4), window);
    ASSERT_EQ(expected_thresholds[h], threshold);
    ASSERT_EQ(4, voting);

    db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(new_blocks, denied)
{
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    hf.init();

    ASSERT_TRUE(hf.add(mkblock(1, 1), 0));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 1));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 2));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 3));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 4));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 5));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 6));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 7));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 8)); // we reach 50% of the last 4
    ASSERT_FALSE(hf.add(mkblock(2, 1), 9)); // so this one can't get added
    ASSERT_TRUE(hf.add(mkblock(2, 2), 9));
}

TEST(new_version, early)
{
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 4, 1));
    hf.init();

    ASSERT_TRUE(hf.add(mkblock(1, 2), 0));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 1)); // we have enough votes already
    ASSERT_TRUE(hf.add(mkblock(1, 2), 2));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 3)); // we accept a previous version because we did not switch, even with all the votes
    ASSERT_TRUE(hf.add(mkblock(2, 2), 4)); // but have to wait for the declared height anyway
    ASSERT_TRUE(hf.add(mkblock(2, 2), 5));
    ASSERT_FALSE(hf.add(mkblock(2, 1), 6)); // we don't accept 1 anymore
    ASSERT_TRUE(hf.add(mkblock(2, 2), 7)); // but we do accept 2
}

TEST(reorganize, changed)
{
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(3, 5, 2));
    ASSERT_TRUE(hf.add_fork(4, 555, 222));
    hf.init();

#define ADD(v, h, a) \
  do { \
    cryptonote::block b = mkblock(hf, h, v); \
    db.add_block(b, 0, 0, 0, 0, 0, crypto::hash()); \
    ASSERT_##a(hf.add(b, h)); \
  } while(0)
#define ADD_TRUE(v, h) ADD(v, h, TRUE)
#define ADD_FALSE(v, h) ADD(v, h, FALSE)

    ADD_TRUE(1, 0);
    ADD_TRUE(1, 1);
    ADD_TRUE(2, 2);
    ADD_TRUE(2, 3); // switch to 2 here
    ADD_TRUE(2, 4);
    ADD_TRUE(2, 5);
    ADD_TRUE(2, 6);
    ASSERT_EQ(hf.get_current_version(), 2);
    ADD_TRUE(3, 7);
    ADD_TRUE(4, 8);
    ADD_TRUE(4, 9);
    ASSERT_EQ(hf.get_current_version(), 3);

    // pop a few blocks and check current version goes back down
    db.remove_block();
    hf.reorganize_from_block_height(8);
    ASSERT_EQ(hf.get_current_version(), 3);
    db.remove_block();
    hf.reorganize_from_block_height(7);
    ASSERT_EQ(hf.get_current_version(), 2);
    db.remove_block();
    ASSERT_EQ(hf.get_current_version(), 2);

    // add blocks again, but remaining at 2
    ADD_TRUE(2, 7);
    ADD_TRUE(2, 8);
    ADD_TRUE(2, 9);
    ASSERT_EQ(hf.get_current_version(), 2); // we did not bump to 3 this time
}

TEST(get, higher)
{
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(3, 5, 2));
    hf.init();

    ASSERT_EQ(hf.get_ideal_version(0), 1);
    ASSERT_EQ(hf.get_ideal_version(1), 1);
    ASSERT_EQ(hf.get_ideal_version(2), 2);
    ASSERT_EQ(hf.get_ideal_version(3), 2);
    ASSERT_EQ(hf.get_ideal_version(4), 2);
    ASSERT_EQ(hf.get_ideal_version(5), 3);
    ASSERT_EQ(hf.get_ideal_version(6), 3);
    ASSERT_EQ(hf.get_ideal_version(7), 3);
}

TEST(get, earliest_ideal_height)
{
    TestDB db;
    HardFork hf(db, 1, 0, 1, 1, 4, 50);

    //                      v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(5, 5, 2));
    ASSERT_TRUE(hf.add_fork(6, 10, 3));
    ASSERT_TRUE(hf.add_fork(9, 15, 4));
    hf.init();

    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(1), 0);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(2), 2);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(3), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(4), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(5), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(6), 10);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(7), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(8), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(9), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(10), std::numeric_limits<uint64_t>::max());
}

