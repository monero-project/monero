// Copyright (c) 2014-2026, The Monero Project
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

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/hardfork.h"

using namespace cryptonote;

#define BLOCKS_PER_YEAR 525960
#define SECONDS_PER_YEAR 31557600

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
  b.major_version = hf.get_ideal_version(height);
  b.minor_version = vote;
  return b;
}

TEST(major, Only)
{
  HardFork hf(1, 0, 0, 0); // no voting

  //                      v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 2, 1));
  hf.init();

  // block height 0, only version 1 is accepted
  ASSERT_FALSE(hf.check_for_height(mkblock(0, 2), 0));
  ASSERT_FALSE(hf.check_for_height(mkblock(2, 2), 0));
  ASSERT_TRUE(hf.check_for_height(mkblock(1, 2), 0));

  // block height 1, only version 1 is accepted
  ASSERT_FALSE(hf.check_for_height(mkblock(0, 2), 1));
  ASSERT_FALSE(hf.check_for_height(mkblock(2, 2), 1));
  ASSERT_TRUE(hf.check_for_height(mkblock(1, 2), 1));

  // block height 2, only version 2 is accepted
  ASSERT_FALSE(hf.check_for_height(mkblock(0, 2), 2));
  ASSERT_FALSE(hf.check_for_height(mkblock(1, 2), 2));
  ASSERT_FALSE(hf.check_for_height(mkblock(3, 2), 2));
  ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), 2));
}

TEST(empty_hardforks, Success)
{
  HardFork hf;

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  hf.init();
  ASSERT_TRUE(hf.get_state(time(NULL)) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(time(NULL) + 3600*24*400) == HardFork::Ready);

  for (uint64_t h = 0; h <= 10; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 1), h));
  }
  ASSERT_EQ(hf.get_ideal_version(0), 1);
  ASSERT_EQ(hf.get_ideal_version(1), 1);
  ASSERT_EQ(hf.get_ideal_version(10), 1);
}

TEST(ordering, Success)
{
  HardFork hf;

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
  HardFork hf(1, 0, 0, 0); // no voting

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 5, 1));
  hf.init();

  for (uint64_t h = 0; h <= 4; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 1), h));
    ASSERT_FALSE(hf.check_for_height(mkblock(2, 2), h));  // block version is too high
    ASSERT_TRUE(hf.check_for_height(mkblock(hf, h, 1), h));
  }

  for (uint64_t h = 5; h <= 10; ++h) {
    ASSERT_FALSE(hf.check_for_height(mkblock(1, 1), h));  // block version is too low
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), h));
    ASSERT_TRUE(hf.check_for_height(mkblock(hf, h, 2), h));
  }
}

TEST(states, Success)
{
  HardFork hf;

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
  HardFork hf(1,0,1,1);

  //                 v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(4, 2, 1));
  ASSERT_TRUE(hf.add_fork(7, 4, 2));
  ASSERT_TRUE(hf.add_fork(9, 6, 3));
  hf.init();

  for (uint64_t h = 0; h < 10; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(hf, h, 9), h));
  }

  ASSERT_EQ(hf.get_ideal_version(0), 1);
  ASSERT_EQ(hf.get_ideal_version(1), 1);
  ASSERT_EQ(hf.get_ideal_version(2), 4);
  ASSERT_EQ(hf.get_ideal_version(3), 4);
  ASSERT_EQ(hf.get_ideal_version(4), 7);
  ASSERT_EQ(hf.get_ideal_version(5), 7);
  ASSERT_EQ(hf.get_ideal_version(6), 9);
  ASSERT_EQ(hf.get_ideal_version(7), 9);
  ASSERT_EQ(hf.get_ideal_version(8), 9);
  ASSERT_EQ(hf.get_ideal_version(9), 9);
}

TEST(steps_1, Success)
{
  HardFork hf(1,0,1,1);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  for (int n = 1 ; n < 10; ++n)
    ASSERT_TRUE(hf.add_fork(n+1, n, n));
  hf.init();

  for (uint64_t h = 0 ; h < 10; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(hf, h, h+1), h));
  }
}

TEST(new_version, early)
{
    HardFork hf(1, 0, 1, 1);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 4, 1));
    hf.init();

    ASSERT_TRUE(hf.check_for_height(mkblock(1, 2), 0));
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 2), 1)); // we have enough votes already
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 2), 2));
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 1), 3)); // we accept a previous version because we did not switch, even with all the votes
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), 4)); // but have to wait for the declared height anyway
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), 5));
    ASSERT_FALSE(hf.check_for_height(mkblock(2, 1), 6)); // we don't accept 1 anymore
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), 7)); // but we do accept 2
}

TEST(get, higher)
{
    HardFork hf(1, 0, 1, 1);

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
    HardFork hf(1, 0, 1, 1);

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

