// Copyright (c) 2017-2022, The Monero Project
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
