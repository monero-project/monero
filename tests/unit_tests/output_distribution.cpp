// Copyright (c) 2018, The Monero Project
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
#include "rpc/rpc_handler.h"
#include "blockchain_db/blockchain_db.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/blockchain.h"
#include "testdb.h"

static const uint64_t test_distribution[32] = {
  0, 0, 0, 0, 0, 1, 5, 1, 4, 0, 0, 1, 0, 1, 2, 3, 1, 0, 2, 0, 1, 3, 8, 1, 3, 5, 7, 1, 5, 0, 2, 3
};
static const size_t test_distribution_size = sizeof(test_distribution) / sizeof(test_distribution[0]);

namespace
{

class TestDB: public BaseTestDB
{
public:
  TestDB(size_t bc_height = test_distribution_size): blockchain_height(bc_height) { m_open = true; }
  virtual uint64_t height() const override { return blockchain_height; }

  std::vector<uint64_t> get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const override
  {
    std::vector<uint64_t> d;
    for (uint64_t h: heights)
    {
      uint64_t c = 0;
      for (uint64_t i = 0; i <= h; ++i)
        c += test_distribution[i];
      d.push_back(c);
    }
    return d;
  }

  uint64_t blockchain_height;
};

}

bool get_output_distribution(uint64_t amount, uint64_t from, uint64_t to, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base)
{
  std::unique_ptr<cryptonote::Blockchain> bc;
  cryptonote::tx_memory_pool txpool(*bc);
  bc.reset(new cryptonote::Blockchain(txpool));
  struct get_test_options {
    const std::pair<uint8_t, uint64_t> hard_forks[2];
    const cryptonote::test_options test_options = {
      hard_forks
    };
    get_test_options():hard_forks{std::make_pair((uint8_t)1, (uint64_t)0), std::make_pair((uint8_t)0, (uint64_t)0)}{}
  } opts;
  cryptonote::Blockchain *blockchain = bc.get();
  bool r = blockchain->init(new TestDB(test_distribution_size), cryptonote::FAKECHAIN, true, &opts.test_options, 0, NULL);
  return r && bc->get_output_distribution(amount, from, to, start_height, distribution, base);
}

TEST(output_distribution, extend)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 29, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 2);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 29, true);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 2);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 30, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 3);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0, 2}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 30, true);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 3);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55, 57}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 31, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 4);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0, 2, 3}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 31, true);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 4);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55, 57, 60}));
}

TEST(output_distribution, one)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 0, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 1);
  ASSERT_EQ(res->distribution.back(), 0);
}

TEST(output_distribution, full_cumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 31, true);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 32);
  ASSERT_EQ(res->distribution.back(), 60);
}

TEST(output_distribution, full_noncumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 31, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 32);
  for (size_t i = 0; i < 32; ++i)
    ASSERT_EQ(res->distribution[i], test_distribution[i]);
}

TEST(output_distribution, part_cumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 4, 8, true);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 5);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({0, 1, 6, 7, 11}));
}

TEST(output_distribution, part_noncumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 4, 8, false);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 5);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({0, 1, 5, 1, 4}));
}
