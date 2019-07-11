// Copyright (c) 2018, The Loki Project
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
#include "cryptonote_core/service_node_swarm.h"
#include "cryptonote_basic/cryptonote_basic.h"

#include <functional>
#include <iterator>

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "sn_unit_tests"

using namespace service_nodes;

crypto::public_key newPubKey() {
  return cryptonote::keypair::generate(hw::get_device("default")).pub;
};

size_t calculateExcess(const swarm_snode_map_t& swarm_to_snodes) {
  return std::accumulate(swarm_to_snodes.begin(), swarm_to_snodes.end(), 0, [](size_t total, const swarm_snode_map_t::value_type& pair) {
    return total + std::max<int>(0, (int)pair.second.size() - MIN_SWARM_SIZE);
  });
}

size_t getExpectedNumSwarmsNoDereg(size_t num_snodes) {
  /// The number of swarm (y) should be a step function of the number of snodes (x).
  /// Assuming the ideal size is 7:
  /// 1 <= x < 14 : y = 1
  /// 14 <= x < 21: y = 2
  /// 21 <= x < 28: y = 3
  /// etc
  const ssize_t diff = num_snodes - IDEAL_SWARM_SIZE;
  return 1 + std::max(diff, ssize_t(0)) / IDEAL_SWARM_SIZE;
}

void validateSwarmsNoDereg(const swarm_snode_map_t& swarm_to_snodes, size_t num_snodes) {
  const size_t expected_num_swarms = getExpectedNumSwarmsNoDereg(num_snodes);
  ASSERT_EQ(expected_num_swarms, swarm_to_snodes.size()) << " Failed with num_snodes:" << num_snodes;

  /// Expected excess
  if (num_snodes >= IDEAL_SWARM_SIZE) {
    const size_t excess = calculateExcess(swarm_to_snodes);
    const size_t expected_excess = (expected_num_swarms * IDEAL_SWARM_MARGIN) + num_snodes % IDEAL_SWARM_SIZE;
    ASSERT_EQ(expected_excess, excess) << " Failed with num_snodes:" << num_snodes;
  }
}

void registerInitialSnodes(swarm_snode_map_t& swarm_to_snodes, size_t reg_per_block, size_t num_blocks) {
  std::vector<crypto::public_key> unassigned_snodes;
  size_t num_snodes = std::accumulate(swarm_to_snodes.begin(),
                                      swarm_to_snodes.end(),
                                      size_t(0),
                                      [](size_t acc, const swarm_snode_map_t::value_type& entry){
                                        return acc + entry.second.size();
                                      });
  for (int i = 0; i < num_blocks; ++i)
  {
    for (int j = 0; j < reg_per_block; ++j)
    {
      unassigned_snodes.push_back(newPubKey());
    }
    swarm_to_snodes[UNASSIGNED_SWARM_ID] = unassigned_snodes;
    calc_swarm_changes(swarm_to_snodes, i /* seed */);
    unassigned_snodes.clear();
    num_snodes += reg_per_block;
    LOG_PRINT_L2("num_snodes: " << num_snodes);
    validateSwarmsNoDereg(swarm_to_snodes, num_snodes);
  }
}

TEST(swarm_to_snodes, calc_excess)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t expected_excess = 52;
  const size_t num_swarms = 11;
  // create swarms with EXCESS_BASE snodes
  for (size_t i = 0; i < num_swarms; ++i) {
    for (size_t j = 0; j < EXCESS_BASE; j++){
      swarm_to_snodes[i].push_back(newPubKey());
    }
  }
  /// This should have no excess
  ASSERT_EQ(0, calc_excess(swarm_to_snodes));
  /// Any additional snodes will contribute to the excess
  for (size_t i = 0; i < expected_excess; i++) {
    const swarm_id_t id = i % num_swarms;
    swarm_to_snodes[id].push_back(newPubKey());
  }
  ASSERT_EQ(expected_excess, calc_excess(swarm_to_snodes));
}

TEST(swarm_to_snodes, calc_threshold)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t num_swarms = 11;
  const size_t num_snodes = num_swarms * EXCESS_BASE;
  // create swarms with EXCESS_BASE snodes
  for (size_t i = 0; i < num_snodes; i++) {
    const swarm_id_t id = i % num_swarms;
    swarm_to_snodes[id].push_back(newPubKey());
    ASSERT_EQ(swarm_to_snodes.size() * IDEAL_SWARM_MARGIN + NEW_SWARM_SIZE, calc_threshold(swarm_to_snodes));
  }
}

TEST(swarm_to_snodes, create_new_swarm_from_excess)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t num_swarms = 11;
  /// create swarms with EXCESS_BASE snodes
  for (size_t i = 0; i < num_swarms; ++i) {
    for (size_t j = 0; j < EXCESS_BASE; j++){
      swarm_to_snodes[i].push_back(newPubKey());
    }
  }
  std::mt19937_64 mt(42);
  /// should not create any new swarm since there is not enough excess
  create_new_swarm_from_excess(swarm_to_snodes, mt);
  ASSERT_EQ(num_swarms, swarm_to_snodes.size());
  /// Add some excess but just not enough
  const size_t excess_snodes = num_swarms * IDEAL_SWARM_MARGIN + NEW_SWARM_SIZE;
  for (size_t i = 0; i < excess_snodes - 1; i++) {
    const swarm_id_t id = i % num_swarms;
    swarm_to_snodes[id].push_back(newPubKey());
  }
  create_new_swarm_from_excess(swarm_to_snodes, mt);
  ASSERT_EQ(num_swarms, swarm_to_snodes.size());
  /// Add one final snode, which should trigger the creation of a new swarm
  swarm_to_snodes[1].push_back(newPubKey());
  create_new_swarm_from_excess(swarm_to_snodes, mt);
  ASSERT_EQ(num_swarms + 1, swarm_to_snodes.size());
}

TEST(swarm_to_snodes, calc_swarm_sizes)
{
  const size_t num_swarms = 42;
  swarm_snode_map_t swarm_to_snodes;
  for (size_t i = 0; i < num_swarms; ++i) {
    for (size_t j = 0; j < i + 1; j++){
      swarm_to_snodes[i].push_back(newPubKey());
    }
  }

  std::vector<swarm_size> sorted_swarm_sizes;
  calc_swarm_sizes(swarm_to_snodes, sorted_swarm_sizes);
  ASSERT_EQ(num_swarms, sorted_swarm_sizes.size());

  size_t previous_size = 0;
  for (size_t i = 0; i < sorted_swarm_sizes.size(); i++) {
      const auto& swarm_size = sorted_swarm_sizes[i];
      /// assert that it is sorted
      ASSERT_TRUE(swarm_size.size >= previous_size);
      ASSERT_EQ(i + 1, swarm_size.size);
      previous_size = swarm_size.size;
  }
}

TEST(swarm_to_snodes, assign_snodes)
{
  swarm_snode_map_t swarm_to_snodes;
  swarm_to_snodes[0] = {
    newPubKey(),
    newPubKey(),
    newPubKey(),
  };
  swarm_to_snodes[1] = {
    newPubKey(),
  };
  swarm_to_snodes[2] = {
    newPubKey(),
    newPubKey(),
    newPubKey(),
    newPubKey(),
    newPubKey(),
  };
  swarm_to_snodes[3] = {
    newPubKey(),
    newPubKey(),
  };
  std::mt19937_64 mt(42);
  /// should fill swarm 1 since it's the only one if the lowest 25 percentile
  assign_snodes({ newPubKey() }, swarm_to_snodes, mt, FILL_SWARM_LOWER_PERCENTILE);
  ASSERT_EQ(2, swarm_to_snodes[1].size());
  /// should fill swarm 1 and 3
  assign_snodes({ newPubKey() }, swarm_to_snodes, mt, FILL_SWARM_LOWER_PERCENTILE);
  assign_snodes({ newPubKey() }, swarm_to_snodes, mt, FILL_SWARM_LOWER_PERCENTILE);
  ASSERT_EQ(3, swarm_to_snodes[1].size());
  ASSERT_EQ(3, swarm_to_snodes[3].size());

}

TEST(swarm_to_snodes, register_snodes_one_by_one)
{
  const size_t expected_total_num_swarms = 40;
  const size_t total_num_snodes = expected_total_num_swarms * IDEAL_SWARM_SIZE;
  const size_t registration_per_block = 1;
  const size_t num_blocks = total_num_snodes / registration_per_block;

  swarm_snode_map_t swarm_to_snodes;
  registerInitialSnodes(swarm_to_snodes, registration_per_block, num_blocks);
}

TEST(swarm_to_snodes, register_snodes_in_bulk)
{
  swarm_snode_map_t swarm_to_snodes;

  /// Add snodes by bulk
  const auto bulk_increments = { 1, 3, 19, 4, 8, 13, 1, 22, 1, 7, 2, 1, 11, 18, 6, 10 };
  const size_t num_blocks = 1;
  for (const auto registration_per_block : bulk_increments)
  {
    registerInitialSnodes(swarm_to_snodes, registration_per_block, num_blocks);
  }
}


TEST(swarm_to_snodes, get_excess_pool)
{
  swarm_snode_map_t swarm_to_snodes;

  std::vector<excess_pool_snode> pool_snodes;
  size_t excess;
  /// First swarm up to MIN_SWARM_SIZE
  for (size_t i = 0; i < MIN_SWARM_SIZE; ++i)
  {
    swarm_to_snodes[0].push_back(newPubKey());
    get_excess_pool(MIN_SWARM_SIZE, swarm_to_snodes, pool_snodes, excess);
    ASSERT_EQ(0, excess);
    ASSERT_EQ(0, pool_snodes.size());
  }

  /// Second swarm up to MIN_SWARM_SIZE
  for (size_t i = 0; i < MIN_SWARM_SIZE; ++i)
  {
    swarm_to_snodes[1].push_back(newPubKey());
    get_excess_pool(MIN_SWARM_SIZE, swarm_to_snodes, pool_snodes, excess);
    ASSERT_EQ(0, excess);
    ASSERT_EQ(0, pool_snodes.size());
  }

  /// add excess in first swarm
  const size_t first_swarm_excess = 10;
  const size_t second_swarm_excess = 10;
  for (size_t i = 0; i < first_swarm_excess; ++i)
  {
    swarm_to_snodes[0].push_back(newPubKey());
    get_excess_pool(MIN_SWARM_SIZE, swarm_to_snodes, pool_snodes, excess);
    ASSERT_EQ(i + 1, excess);
    ASSERT_EQ(MIN_SWARM_SIZE + i + 1, pool_snodes.size());
  }
  /// add excess in second swarm
  for (size_t i = 0; i < second_swarm_excess; ++i)
  {
    swarm_to_snodes[1].push_back(newPubKey());
    get_excess_pool(MIN_SWARM_SIZE, swarm_to_snodes, pool_snodes, excess);
    ASSERT_EQ(first_swarm_excess + i + 1, excess);
    ASSERT_EQ(MIN_SWARM_SIZE * 2 + first_swarm_excess + i + 1, pool_snodes.size());
  }

  /// Third swarm up to MIN_SWARM_SIZE
  /// Check that none of the snodes in swarm 3 are in the pool
  for (size_t i = 0; i < MIN_SWARM_SIZE; ++i)
  {
    const auto pubKey = newPubKey();
    swarm_to_snodes[3].push_back(pubKey);
    get_excess_pool(MIN_SWARM_SIZE, swarm_to_snodes, pool_snodes, excess);
    const auto it = std::find_if(pool_snodes.begin(), pool_snodes.end(), [&](const excess_pool_snode& excess_snode) {
        return excess_snode.public_key == pubKey;
      });
    ASSERT_TRUE(it == pool_snodes.end());
    ASSERT_EQ(first_swarm_excess + second_swarm_excess, excess);
  }
}

TEST(swarm_to_snodes, pick_from_excess_pool)
{
  std::mt19937_64 mt(123456);

  const std::vector<excess_pool_snode> excess_pool = {
    { newPubKey(), 0},
    { newPubKey(), 0},
    { newPubKey(), 0},
    { newPubKey(), 0},
    { newPubKey(), 0},
    { newPubKey(), 0},
    { newPubKey(), 1},
    { newPubKey(), 1},
    { newPubKey(), 1},
    { newPubKey(), 1},
    { newPubKey(), 1}
  };
  /// (it is implicilty assumed that excess_pool is never empty)
  const auto& random_excess_snode = pick_from_excess_pool(excess_pool, mt);
  ASSERT_TRUE(std::find_if(excess_pool.begin(), excess_pool.end(), [random_excess_snode](const excess_pool_snode& snode){
    return snode.public_key == random_excess_snode.public_key;
  }) != excess_pool.end());
}

TEST(swarm_to_snodes, remove_excess_snode_from_swarm)
{
  swarm_snode_map_t swarm_to_snodes = {
    {0, {newPubKey(), newPubKey(), newPubKey(), newPubKey(), newPubKey(), newPubKey()}},
    {1, {newPubKey(), newPubKey(), newPubKey(), newPubKey(), newPubKey()}},
    {2, {newPubKey(), newPubKey(), newPubKey(), newPubKey(), newPubKey()}}
  };

  const auto& first_swarm = swarm_to_snodes[0];

  std::vector<excess_pool_snode> pool_snodes = {
    {first_swarm[0], 0},
    {first_swarm[1], 0},
    {first_swarm[2], 0},
    {first_swarm[3], 0},
    {first_swarm[4], 0},
    {first_swarm[5], 0}
  };

  /// when snode exists in swarm_to_snodes
  auto& excess_snode = pool_snodes[0];
  remove_excess_snode_from_swarm(excess_snode, swarm_to_snodes);
  /// should be removed from first swarm
  ASSERT_TRUE(std::find(first_swarm.begin(), first_swarm.end(), excess_snode.public_key) == first_swarm.end());
  /// first swarm has one snode less
  ASSERT_EQ(5, swarm_to_snodes[0].size());
  /// other swarms untouched
  ASSERT_EQ(5, swarm_to_snodes[1].size());
  ASSERT_EQ(5, swarm_to_snodes[2].size());

  /// when snode doesn't exist in swarm_to_snodes
  const excess_pool_snode new_snode = {newPubKey(), 2};
  remove_excess_snode_from_swarm(new_snode, swarm_to_snodes);
  /// other swarms untouched
  ASSERT_EQ(5, swarm_to_snodes[0].size());
  ASSERT_EQ(5, swarm_to_snodes[1].size());
  ASSERT_EQ(5, swarm_to_snodes[2].size());
}

TEST(swarm_to_snodes, swarm_above_min_size_unaffects_other_swarms)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t initial_num_snodes = 10 * IDEAL_SWARM_SIZE;

  registerInitialSnodes(swarm_to_snodes, initial_num_snodes, 1);

  const auto first_id = swarm_to_snodes.begin()->first;

  /// Reduce first swarm size to MIN_SWARM_SIZE, 1 snode at the time
  /// This should not affect the other swarms.
  int seed = initial_num_snodes;
  while (swarm_to_snodes[first_id].size() > MIN_SWARM_SIZE)
  {
    swarm_snode_map_t copy = swarm_to_snodes;
    swarm_to_snodes[first_id].pop_back();
    calc_swarm_changes(swarm_to_snodes, seed++);
    ASSERT_TRUE(swarm_to_snodes.size() == copy.size());
    /// Ensure the other swarms are unaffected
    for (const auto& entry : swarm_to_snodes) {
      if (entry.first != first_id) {
        ASSERT_TRUE(copy[entry.first] == entry.second);
      }
    }
  }

  /// Reduce second swarm size to MIN_SWARM_SIZE all at once
  /// This should not affect the other swarms.
  {
    const auto second_id = std::next(swarm_to_snodes.begin())->first;

    swarm_snode_map_t copy = swarm_to_snodes;
    while (swarm_to_snodes[second_id].size() > MIN_SWARM_SIZE)
    {
      swarm_to_snodes[second_id].pop_back();
    }
    calc_swarm_changes(swarm_to_snodes, seed++);
    ASSERT_TRUE(swarm_to_snodes.size() == copy.size());
    /// Ensure the other swarms are unaffected
    for (const auto& entry : swarm_to_snodes) {
      if (entry.first != second_id) {
        ASSERT_TRUE(copy[entry.first] == entry.second);
      }
    }
  }
}

TEST(swarm_to_snodes, register_snode_fills_low_swarms)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t initial_num_snodes = 10 * IDEAL_SWARM_SIZE;

  registerInitialSnodes(swarm_to_snodes, initial_num_snodes, 1);

  /// Select victims
  const auto first_id = swarm_to_snodes.begin()->first;
  const auto second_id = std::next(swarm_to_snodes.begin())->first;

  /// Reduce swarms size to MIN_SWARM_SIZE
  int seed = initial_num_snodes;
  while (swarm_to_snodes[first_id].size() > MIN_SWARM_SIZE)
  {
    swarm_to_snodes[first_id].pop_back();
    swarm_to_snodes[second_id].pop_back();
    calc_swarm_changes(swarm_to_snodes, seed++);
  }

  /// Drop the 2 first swarm below the minimum size and provide 3 registrations
  /// (3 registration should cover the lowest 25% of the 10 swarms)
  swarm_to_snodes[first_id].pop_back();
  swarm_to_snodes[second_id].pop_back();

  std::vector<crypto::public_key> unassigned_snodes;
  for (int i=0; i< 3; ++i)
  {
    unassigned_snodes.push_back(newPubKey());
  }
  swarm_to_snodes[UNASSIGNED_SWARM_ID] = unassigned_snodes;
  calc_swarm_changes(swarm_to_snodes, seed++);
  ASSERT_GT(swarm_to_snodes[first_id].size(), MIN_SWARM_SIZE - 1);
  ASSERT_GT(swarm_to_snodes[second_id].size(), MIN_SWARM_SIZE - 1);
}

TEST(swarm_to_snodes, stealing_from_rich_swarms)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t initial_num_snodes = 10 * IDEAL_SWARM_SIZE;

  registerInitialSnodes(swarm_to_snodes, initial_num_snodes, 1);

  /// Take a copy for later comparison
  swarm_snode_map_t copy = swarm_to_snodes;

  /// Select a victim
  auto first_id = swarm_to_snodes.begin()->first;

  /// Reduce swarm size to MIN_SWARM_SIZE - 1
  int seed = initial_num_snodes;
  while (swarm_to_snodes[first_id].size() >= MIN_SWARM_SIZE)
  {
    swarm_to_snodes[first_id].pop_back();
  }
  calc_swarm_changes(swarm_to_snodes, seed++);
  /// the victim swarm size should be >= MIN_SWARM_SIZE
  ASSERT_GT(swarm_to_snodes[first_id].size(), MIN_SWARM_SIZE - 1);

  /// Ensure only one other swarm was reduced in size due to a stolen snode
  size_t num_swarm_affected = 0;
  for (const auto& entry : swarm_to_snodes) {
    if (entry.first != first_id) {
      if (entry.second.size() < copy[entry.first].size()) {
        ASSERT_TRUE(entry.second.size() >= MIN_SWARM_SIZE);
        ASSERT_EQ(copy[entry.first].size() - 1, entry.second.size());
        num_swarm_affected++;
      }
    }
  }
  ASSERT_EQ(1, num_swarm_affected);
}

TEST(swarm_to_snodes, decommission)
{
  swarm_snode_map_t swarm_to_snodes;
  const size_t initial_num_snodes = 10 * IDEAL_SWARM_SIZE;

  registerInitialSnodes(swarm_to_snodes, initial_num_snodes, 1);
  const size_t initial_num_swarms = swarm_to_snodes.size();

  /// Reduce swarm size to MIN_SWARM_SIZE to all
  int seed = initial_num_snodes;
  for (auto& entry : swarm_to_snodes) {
    while (entry.second.size() > MIN_SWARM_SIZE)
    {
      entry.second.pop_back();
    }
  }
  /// Select a victim
  swarm_to_snodes.begin()->second.pop_back();

  calc_swarm_changes(swarm_to_snodes, seed++);
  /// 1 swarm should have been decommissioned
  ASSERT_EQ(initial_num_swarms - 1, swarm_to_snodes.size());
}
