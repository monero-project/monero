// Copyright (c) 2017-2024, The Monero Project
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

#include <boost/uuid/uuid.hpp>
#include "gtest/gtest.h"
#include "crypto/crypto.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_protocol/block_queue.h"

static const boost::uuids::uuid &uuid1()
{
  static const boost::uuids::uuid uuid = crypto::rand<boost::uuids::uuid>();
  return uuid;
}

static const boost::uuids::uuid &uuid2()
{
  static const boost::uuids::uuid uuid = crypto::rand<boost::uuids::uuid>();
  return uuid;
}

TEST(block_queue, empty)
{
  cryptonote::block_queue bq;
  ASSERT_EQ(bq.get_max_block_height(), 0);
}

TEST(block_queue, add_stepwise)
{
  epee::net_utils::network_address na;
  cryptonote::block_queue bq;
  bq.add_blocks(0, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.add_blocks(401, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 600);
  bq.add_blocks(400, 10, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 600);
}

TEST(block_queue, flush_uuid)
{
  cryptonote::block_queue bq;
  epee::net_utils::network_address na;

  bq.add_blocks(0, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid2(), na);
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.flush_spans(uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.flush_spans(uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 0);

  bq.add_blocks(0, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid2(), na);
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.flush_spans(uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.add_blocks(0, 200, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_height(), 399);
}

TEST(block_queue, reserve_does_not_overlap_later_span)
{
  cryptonote::block_queue bq;
  epee::net_utils::network_address na;
  std::vector<std::pair<crypto::hash, uint64_t>> hashes;
  hashes.reserve(100);
  for (size_t i = 0; i < 100; ++i)
    hashes.emplace_back(crypto::rand<crypto::hash>(), 0);

  ASSERT_EQ(bq.reserve_span(1, 100, 28, uuid1(), na, false, 0, 0, 101, hashes), std::make_pair(uint64_t{1}, uint64_t{28}));
  ASSERT_EQ(bq.reserve_span(1, 100, 28, uuid1(), na, false, 0, 0, 101, hashes), std::make_pair(uint64_t{29}, uint64_t{28}));
  ASSERT_EQ(bq.reserve_span(1, 100, 28, uuid1(), na, false, 0, 0, 101, hashes), std::make_pair(uint64_t{57}, uint64_t{28}));

  ASSERT_TRUE(bq.remove_span(29));

  // A recalculated, larger sync size must fill only the gap, without
  // overlapping the span which is already reserved at height 57.
  ASSERT_EQ(bq.reserve_span(1, 100, 36, uuid2(), na, false, 0, 0, 101, hashes), std::make_pair(uint64_t{29}, uint64_t{28}));
}

TEST(block_queue, reserve_span_same_height_different_hashes)
{
  const std::vector<std::pair<crypto::hash, uint64_t>> first_hashes{
    {crypto::rand<crypto::hash>(), 0}, {crypto::rand<crypto::hash>(), 0}};
  const std::vector<std::pair<crypto::hash, uint64_t>> conflicting_hashes{
    {crypto::rand<crypto::hash>(), 0}, {crypto::rand<crypto::hash>(), 0}};
  epee::net_utils::network_address na;

  for (const auto &connection_id: {uuid1(), uuid2()})
  {
    cryptonote::block_queue bq;
    ASSERT_EQ(bq.reserve_span(100, 101, 2, uuid1(), na, false, 0, 0, 1000, first_hashes),
        std::make_pair(uint64_t(100), uint64_t(2)));
    ASSERT_EQ(bq.reserve_span(100, 101, 2, connection_id, na, false, 0, 0, 1000, conflicting_hashes),
        std::make_pair(uint64_t(0), uint64_t(0)));

    const auto scheduled = bq.get_next_span_if_scheduled(100);
    ASSERT_TRUE(scheduled);
    ASSERT_EQ(scheduled->start_block_height, 100u);
    ASSERT_EQ(scheduled->nblocks, 2u);
    ASSERT_EQ(scheduled->connection_id, uuid1());
    ASSERT_TRUE(bq.requested(first_hashes[0].first, 100));
    ASSERT_TRUE(bq.requested(first_hashes[1].first, 101));
    for (const auto &entry: conflicting_hashes)
      ASSERT_FALSE(bq.requested(entry.first));

    bq.flush_spans(uuid1());
    ASSERT_EQ(bq.reserve_span(100, 101, 2, connection_id, na, false, 0, 0, 1000, conflicting_hashes),
        std::make_pair(uint64_t(100), uint64_t(2)));
    for (const auto &entry: first_hashes)
      ASSERT_FALSE(bq.requested(entry.first));
    for (const auto &entry: conflicting_hashes)
      ASSERT_TRUE(bq.requested(entry.first));
  }
}

TEST(block_queue, reserve_span_skips_requested_prefix)
{
  const std::vector<std::pair<crypto::hash, uint64_t>> hashes{
    {crypto::rand<crypto::hash>(), 0}, {crypto::rand<crypto::hash>(), 0},
    {crypto::rand<crypto::hash>(), 0}, {crypto::rand<crypto::hash>(), 0}};
  cryptonote::block_queue bq;
  epee::net_utils::network_address na;

  ASSERT_EQ(bq.reserve_span(100, 103, 2, uuid1(), na, false, 0, 0, 1000, hashes),
      std::make_pair(uint64_t(100), uint64_t(2)));
  ASSERT_EQ(bq.reserve_span(100, 103, 2, uuid2(), na, false, 0, 0, 1000, hashes),
      std::make_pair(uint64_t(102), uint64_t(2)));
  bq.flush_spans(uuid1());
  ASSERT_FALSE(bq.requested(hashes[0].first));
  ASSERT_FALSE(bq.requested(hashes[1].first));
  ASSERT_TRUE(bq.requested(hashes[2].first));
  ASSERT_TRUE(bq.requested(hashes[3].first));
}

TEST(block_queue, reservation_limit)
{
  using limit = cryptonote::block_queue::download_limit;
  cryptonote::block_queue queue;
  epee::net_utils::network_address address;
  const auto now = boost::posix_time::microsec_clock::universal_time();
  const std::vector<std::pair<crypto::hash, uint64_t>> hashes{
    {crypto::rand<crypto::hash>(), 0}, {crypto::rand<crypto::hash>(), 0}};
  ASSERT_EQ(queue.reserve_span(10, 11, 1, uuid1(), address, false, 0, 0, 20, hashes, now, limit{10, false}).second, 1u);
  EXPECT_EQ(queue.reserve_span(10, 11, 1, uuid2(), address, false, 0, 0, 20, hashes, now, limit{10, true}).second, 0u);

  const std::vector<std::pair<crypto::hash, uint64_t>> fork{{crypto::rand<crypto::hash>(), 0}};
  EXPECT_EQ(queue.reserve_span(4, 4, 1, uuid2(), address, false, 0, 0, 20, fork, now, limit{10, false}).second, 0u);
  EXPECT_EQ(queue.reserve_span(4, 4, 1, uuid2(), address, false, 0, 0, 20, fork, now, limit{10, true}).second, 1u);
  const std::vector<std::pair<crypto::hash, uint64_t>> another{{crypto::rand<crypto::hash>(), 0}};
  EXPECT_EQ(queue.reserve_span(6, 6, 1, uuid1(), address, false, 0, 0, 20, another, now, limit{10, true}).second, 0u);
}

TEST(block_queue, overlapping_span_hashes)
{
  cryptonote::block_queue queue;
  epee::net_utils::network_address address;
  const auto a = crypto::rand<crypto::hash>();
  const auto b = crypto::rand<crypto::hash>();
  ASSERT_EQ(queue.reserve_span(10, 11, 2, uuid1(), address, false, 0, 0, 20, {{a, 0}, {b, 0}}).second, 2u);
  queue.add_blocks(11, std::vector<cryptonote::block_complete_entry>(1), {b}, uuid2(), address, 0, 0, 0, a);
  queue.add_blocks(10, std::vector<cryptonote::block_complete_entry>(2), {a, b}, uuid1(), address, 0, 0, 0, crypto::null_hash);
  EXPECT_EQ(queue.have_height(a), 10u);
  ASSERT_TRUE(queue.remove_span(10));
  EXPECT_FALSE(queue.requested(a));
  EXPECT_TRUE(queue.requested(b));
  EXPECT_EQ(queue.have_height(b), 11u);
  ASSERT_TRUE(queue.remove_span(11));
  EXPECT_FALSE(queue.requested(b));
  EXPECT_FALSE(queue.have(b));
}

TEST(block_queue, same_start_replies_preserve_reserved_tail)
{
  cryptonote::block_queue queue;
  epee::net_utils::network_address address;
  const auto a = crypto::rand<crypto::hash>();
  const auto b = crypto::rand<crypto::hash>();
  const auto old = boost::posix_time::microsec_clock::universal_time() - boost::posix_time::minutes(1);
  ASSERT_EQ(queue.reserve_span(10, 11, 2, uuid1(), address, false, 0, 0, 20, {{a, 0}, {b, 0}}, old).second, 2u);
  const auto scheduled = queue.get_next_span_if_scheduled(10);
  ASSERT_TRUE(scheduled);
  ASSERT_TRUE(queue.claim_span(*scheduled, 10, {a}, crypto::null_hash, uuid2(), address));
  const auto third_peer = crypto::rand<boost::uuids::uuid>();
  EXPECT_FALSE(queue.claim_span(*scheduled, 10, {a}, crypto::null_hash, third_peer, address));

  queue.add_blocks(10, std::vector<cryptonote::block_complete_entry>(1), {a}, uuid2(), address, 0, 0, 0, crypto::null_hash);
  EXPECT_EQ(queue.have_height(a), 10u);
  EXPECT_FALSE(queue.have(b));
  EXPECT_TRUE(queue.requested(b, 11));

  queue.add_blocks(10, std::vector<cryptonote::block_complete_entry>(2), {a, b}, uuid1(), address, 0, 0, 0, crypto::null_hash);
  queue.remove_span(10, uuid2(), a);
  EXPECT_EQ(queue.have_height(a), 10u);
  EXPECT_EQ(queue.have_height(b), 11u);
  queue.remove_span(10, uuid1(), b);
  EXPECT_FALSE(queue.requested(b));
}

TEST(block_queue, same_start_fork_replies)
{
  cryptonote::block_queue queue;
  epee::net_utils::network_address address;
  const auto a = crypto::rand<crypto::hash>();
  const auto b = crypto::rand<crypto::hash>();
  const auto fork_parent = crypto::rand<crypto::hash>();
  const auto old = boost::posix_time::microsec_clock::universal_time() - boost::posix_time::minutes(1);
  ASSERT_EQ(queue.reserve_span(10, 10, 1, uuid1(), address, false, 0, 0, 20, {{a, 0}}, old).second, 1u);
  const auto scheduled = queue.get_next_span_if_scheduled(10);
  ASSERT_TRUE(scheduled);
  ASSERT_TRUE(queue.claim_span(*scheduled, 10, {b}, fork_parent, uuid2(), address));

  const std::vector<cryptonote::block_complete_entry> block(1);
  queue.add_blocks(10, block, {b}, uuid2(), address, 0, 0, 0, fork_parent);
  EXPECT_TRUE(queue.requested(a, 10));
  queue.add_blocks(10, block, {a}, uuid1(), address, 0, 0, 0, crypto::null_hash);
  const auto ready = queue.get_next_span({fork_parent});
  ASSERT_TRUE(ready);
  EXPECT_EQ(ready->connection_id, uuid1());
  queue.remove_span(10, uuid1(), a);
  EXPECT_FALSE(queue.requested(a));
  EXPECT_EQ(queue.have_height(b), 10u);
}

TEST(block_queue, retry_fork_parent_below_tip)
{
  cryptonote::block_queue queue;
  epee::net_utils::network_address address;
  const auto unrelated = crypto::rand<crypto::hash>();
  const auto ancestor = crypto::rand<crypto::hash>();
  const auto middle = crypto::rand<crypto::hash>();
  const auto parent = crypto::rand<crypto::hash>();
  const auto child = crypto::rand<crypto::hash>();
  const auto next = crypto::rand<crypto::hash>();
  const auto tip = crypto::rand<crypto::hash>();
  const auto now = boost::posix_time::microsec_clock::universal_time();
  ASSERT_EQ(queue.reserve_span(90, 90, 1, uuid2(), address, false, 0, 0, 110, {{unrelated, 0}}).second, 1u);
  ASSERT_EQ(queue.reserve_span(98, 98, 1, uuid1(), address, false, 0, 0, 110, {{ancestor, 0}}).second, 1u);
  ASSERT_EQ(queue.reserve_span(98, 99, 1, uuid2(), address, false, 0, 0, 110, {{ancestor, 0}, {middle, 0}}).first, 99u);
  ASSERT_EQ(queue.reserve_span(100, 100, 1, uuid1(), address, false, 0, 0, 110, {{parent, 0}}, now, boost::none, middle).second, 1u);
  const std::vector<cryptonote::block_complete_entry> block(1);
  queue.add_blocks(101, block, {child}, uuid2(), address, 0, 0, 0, parent);
  ASSERT_EQ(queue.reserve_span(102, 102, 1, uuid2(), address, false, 0, 0, 110, {{next, 0}}).second, 1u);
  queue.add_blocks(103, block, {tip}, uuid2(), address, 0, 0, 0, next);

  // The adder is waiting on the parent of 101, even though 103 covers the tip.
  const auto scheduled = queue.get_next_span_if_scheduled(103);
  ASSERT_TRUE(scheduled);
  EXPECT_EQ(scheduled->start_block_height, 98u);
}

TEST(block_queue, count_filled_blocks)
{
  cryptonote::block_queue bq;
  epee::net_utils::network_address na;

  bq.add_blocks(0, std::vector<cryptonote::block_complete_entry>(3), std::vector<crypto::hash>(3), uuid1(), na, 0.0f, 0, 0, crypto::null_hash);
  bq.add_blocks(3, 2, uuid2(), na);
  bq.add_blocks(5, std::vector<cryptonote::block_complete_entry>(4), std::vector<crypto::hash>(4), uuid2(), na, 0.0f, 0, 0, crypto::null_hash);

  ASSERT_EQ(bq.get_num_filled_spans(), 2);
  ASSERT_EQ(bq.get_num_filled_blocks(), 7);
}

TEST(block_queue, cached_sync_size_tracks_span_changes)
{
  cryptonote::block_queue bq;
  epee::net_utils::network_address na;

  bq.add_blocks(0, 2, uuid1(), na);
  ASSERT_EQ(bq.get_max_block_size_average(), 0);
  bq.add_blocks(0, std::vector<cryptonote::block_complete_entry>(2), std::vector<crypto::hash>(2), uuid1(), na, 0.0f, 100, 1000, crypto::null_hash);
  bq.add_blocks(2, std::vector<cryptonote::block_complete_entry>(1), std::vector<crypto::hash>(1), uuid2(), na, 0.0f, 200, 900, crypto::null_hash);
  ASSERT_EQ(bq.get_max_block_size_average(), 900);
  ASSERT_EQ(bq.get_data_size(), 300);

  ASSERT_TRUE(bq.remove_span(2));
  bq.add_blocks(2, std::vector<cryptonote::block_complete_entry>(1), std::vector<crypto::hash>(1), uuid2(), na, 0.0f, 150, 300, crypto::null_hash);
  ASSERT_EQ(bq.get_max_block_size_average(), 500);
  ASSERT_EQ(bq.get_data_size(), 250);

  bq.flush_spans(uuid1(), true);
  ASSERT_EQ(bq.get_max_block_size_average(), 300);
  ASSERT_TRUE(bq.remove_span(2));
  ASSERT_EQ(bq.get_max_block_size_average(), 0);
  ASSERT_EQ(bq.get_data_size(), 0);
}
