// Copyright (c) 2017, The Monero Project
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
  cryptonote::block_queue bq;
  bq.add_blocks(0, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.add_blocks(401, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 600);
  bq.add_blocks(400, 10, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 600);
}

TEST(block_queue, flush_uuid)
{
  cryptonote::block_queue bq;

  bq.add_blocks(0, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.flush_spans(uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.flush_spans(uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 0);

  bq.add_blocks(0, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 199);
  bq.add_blocks(200, 200, uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.flush_spans(uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.add_blocks(0, 200, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 399);
}

TEST(block_queue, reserve_overlaps_both)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(50, 250, 250, uuid2());
  ASSERT_EQ(span.first, 100);
  ASSERT_EQ(span.second, 100);
}

TEST(block_queue, reserve_overlaps_none)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(120, 180, 250, uuid2());
  ASSERT_EQ(span.first, 120);
  ASSERT_EQ(span.second, 61);
}

TEST(block_queue, reserve_overlaps_none_max_hit)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(120, 500, 50, uuid2());
  ASSERT_EQ(span.first, 120);
  ASSERT_EQ(span.second, 50);
}

TEST(block_queue, reserve_overlaps_start)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(50, 150, 250, uuid2());
  ASSERT_EQ(span.first, 100);
  ASSERT_EQ(span.second, 51);
}

TEST(block_queue, reserve_overlaps_start_max_hit)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(50, 300, 75, uuid2());
  ASSERT_EQ(span.first, 100);
  ASSERT_EQ(span.second, 75);
}

TEST(block_queue, reserve_overlaps_stop)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(0, 100, uuid1());
  bq.add_blocks(200, 100, uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 299);

  span = bq.reserve_span(150, 300, 250, uuid2());
  ASSERT_EQ(span.first, 150);
  ASSERT_EQ(span.second, 50);
}

TEST(block_queue, reserve_start_is_empty_after)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(100, 100, uuid1());
  span = bq.reserve_span(150, 250, 100, uuid1());
  ASSERT_EQ(span.first, 200);
  ASSERT_EQ(span.second, 51);
}

TEST(block_queue, reserve_start_is_empty_start_fits)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(100, 100, uuid1());
  span = bq.reserve_span(0, 250, 50, uuid1());
  ASSERT_EQ(span.first, 0);
  ASSERT_EQ(span.second, 50);
}

TEST(block_queue, reserve_start_is_empty_start_overflows)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(100, 100, uuid1());
  span = bq.reserve_span(0, 250, 150, uuid1());
  ASSERT_EQ(span.first, 0);
  ASSERT_EQ(span.second, 100);
}

TEST(block_queue, flush_spans)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;

  bq.add_blocks(100, 100, uuid2());
  bq.add_blocks(200, 100, uuid1());
  bq.add_blocks(300, 100, uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 399);
  bq.flush_spans(uuid2());
  ASSERT_EQ(bq.get_max_block_height(), 299);
  span = bq.reserve_span(0, 500, 500, uuid1());
  ASSERT_EQ(span.first, 0);
  ASSERT_EQ(span.second, 200);
  bq.flush_spans(uuid1());
  ASSERT_EQ(bq.get_max_block_height(), 0);
}

TEST(block_queue, get_next_span)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;
  uint64_t height;
  std::list<cryptonote::block_complete_entry> blocks;
  boost::uuids::uuid uuid;

  bq.add_blocks(100, std::list<cryptonote::block_complete_entry>(100), uuid2(), 0, 0);
  bq.add_blocks(200, std::list<cryptonote::block_complete_entry>(101), uuid1(), 0, 0);
  bq.add_blocks(300, std::list<cryptonote::block_complete_entry>(102), uuid2(), 0, 0);

  ASSERT_TRUE(bq.get_next_span(height, blocks, uuid));
  ASSERT_EQ(height, 100);
  ASSERT_EQ(blocks.size(), 100);
  ASSERT_EQ(uuid, uuid2());
  bq.remove_span(height);

  ASSERT_TRUE(bq.get_next_span(height, blocks, uuid));
  ASSERT_EQ(height, 200);
  ASSERT_EQ(blocks.size(), 101);
  ASSERT_EQ(uuid, uuid1());
  bq.remove_span(height);

  ASSERT_TRUE(bq.get_next_span(height, blocks, uuid));
  ASSERT_EQ(height, 300);
  ASSERT_EQ(blocks.size(), 102);
  ASSERT_EQ(uuid, uuid2());
  bq.remove_span(height);

  ASSERT_FALSE(bq.get_next_span(height, blocks, uuid));
}

TEST(block_queue, get_next_span_if_scheduled)
{
  cryptonote::block_queue bq;
  std::pair<uint64_t, uint64_t> span;
  uint64_t height;
  std::list<cryptonote::block_complete_entry> blocks;
  boost::uuids::uuid uuid;
  std::list<crypto::hash> hashes;
  boost::posix_time::ptime time;

  bq.reserve_span(0, 100, 100, uuid1());
  span = bq.get_next_span_if_scheduled(hashes, uuid, time);
  ASSERT_EQ(span.first, 0);
  ASSERT_EQ(span.second, 100);
  ASSERT_EQ(uuid, uuid1());
  bq.add_blocks(0, std::list<cryptonote::block_complete_entry>(100), uuid1(), 0, 0);
  span = bq.get_next_span_if_scheduled(hashes, uuid, time);
  ASSERT_EQ(span.second, 0);
}
