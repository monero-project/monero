// Copyright (c) 2018-2023, The Monero Project

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
#include "blockchain_db/testdb.h"

static const uint64_t test_distribution[32] = {
  0, 0, 0, 0, 0, 1, 5, 1, 4, 0, 0, 1, 0, 1, 2, 3, 1, 0, 2, 0, 1, 3, 8, 1, 3, 5, 7, 1, 5, 0, 2, 3
};
static const size_t test_distribution_size = sizeof(test_distribution) / sizeof(test_distribution[0]);

namespace
{

class TestDB: public cryptonote::BaseTestDB
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

  std::vector<uint64_t> get_block_weights(uint64_t start_offset, size_t count) const override
  {
    std::vector<uint64_t> weights;
    while (count--) weights.push_back(1);
    return weights;
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

crypto::hash get_block_hash(uint64_t height)
{
  crypto::hash hash = crypto::null_hash;
  *((uint64_t*)&hash) = height;
  return hash;
}

TEST(output_distribution, extend)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 29, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 2);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 29, ::get_block_hash, true, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 2);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 30, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 3);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0, 2}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 30, ::get_block_hash, true, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 3);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55, 57}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 31, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 4);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({5, 0, 2, 3}));

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 28, 31, ::get_block_hash, true, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 4);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({55, 55, 57, 60}));
}

TEST(output_distribution, one)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 0, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 1);
  ASSERT_EQ(res->distribution.back(), 0);
}

TEST(output_distribution, full_cumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 31, ::get_block_hash, true, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 32);
  ASSERT_EQ(res->distribution.back(), 60);
}

TEST(output_distribution, full_noncumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 0, 31, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 32);
  for (size_t i = 0; i < 32; ++i)
    ASSERT_EQ(res->distribution[i], test_distribution[i]);
}

TEST(output_distribution, part_cumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 4, 8, ::get_block_hash, true, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 5);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({0, 1, 6, 7, 11}));
}

TEST(output_distribution, part_noncumulative)
{
  boost::optional<cryptonote::rpc::output_distribution_data> res;

  res = cryptonote::rpc::RpcHandler::get_output_distribution(::get_output_distribution, 0, 4, 8, ::get_block_hash, false, test_distribution_size);
  ASSERT_TRUE(res != boost::none);
  ASSERT_EQ(res->distribution.size(), 5);
  ASSERT_EQ(res->distribution, std::vector<uint64_t>({0, 1, 5, 1, 4}));
}

TEST(CoinbaseOutputDistributionCache, is_output_coinbase)
{
  // Actual mainnet output data from [2853000, 2853999)
  std::vector<uint64_t> coinbase_output_dist =
    {1,1,31,1,1,1,1,1,1,1,1,1,1,1,36,1,49,1,32,1,1,1,1,1,1,1,1,1,26,1,1,1,25,1,1,1,1,30,1,31,1,1,1,
    1,1,33,1,1,1,1,1,1,30,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,26,1,1,1,1,1,1,1,24,1,1,26,26,1,
    1,449,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,27,1,1,1,28,1,1,1,1,26,1,1,25,1,1,1,1,1,1,1,1,50,1,1,1,
    1,1,1,1,1,1,31,1,1,31,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,48,1,1,1,1,1,1,1,48,48,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,28,1,1,1,1,1,1,1,1,1,1,1,1,50,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,28,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,49,1,1,1,1,
    29,1,1,1,1,1,1,1,1,1,1,1,1,24,1,1,1,1,1,1,1,1,1,1,1,1,1,437,1,1,1,1,1,1,1,1,1,1,29,1,1,1,25,1,1,
    1,1,23,23,1,1,1,1,1,1,23,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,27,27,1,
    470,28,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,23,1,1,1,1,1,1,1,1,1,1,1,24,1,1,1,1,1,1,1,
    1,1,1,1,32,1,1,1,1,1,1,1,1,1,1,1,31,1,1,1,1,1,1,1,1,1,1,51,1,1,1,1,1,1,27,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,49,1,1,1,1,1,1,1,32,1,1,31,1,1,1,1,1,1,1,1,1,1,1,48,1,1,1,1,1,1,1,1,1,28,28,1,1,1,
    52,1,1,1,1,1,1,1,1,1,26,26,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,126,1,1,1,1,34,1,1,1,1,1,1,1,1,1,1,
    1,1,1,32,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,36,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,37,1,
    1,1,1,1,1,1,1,1,1,1,1,39,1,1,1,1,1,1,1,1,1,1,1,47,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,31,1,46,31,1,1,
    1,1,1,1,1,1,1,32,1,1,1,1,1,33,1,1,33,1,1,1,1,47,1,1,1,36,1,478,1,1,1,1,46,1,1,1,1,1,1,1,1,1,1,1,
    1,463,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,33,1,1,1,1,1,32,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,33,1,1,1,34,1,1,1,1,1,1,36,1,1,1,1,1,1,53,1,1,1,1,1,
    1,33,1,1,1,1,53,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,55,1,1,1,54,1,33,1,1,1,1,1,1,1,1,1,1,1,1,1,1,56,
    1,1,1,32,1,1,1,35,1,1,1,1,36,1,1,1,1,1,1,1,1,1,54,1,1,31,32,32,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,28,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,54,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,34,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,30,1,1,1,52,1,30,1,1,1,1,29,1,1,1,26,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,33,1,1,
    1,1,1,1,1,35,1,1,1,1,34,1,1,1,1,1,1,34,1,1,33,1,1,1,1,1,1,1,1,35,1,34,1,1,1,1,1,1,35,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1};

  // Actual mainnet output data from [2853000, 2853999)
  std::vector<uint64_t> total_output_dist =
    {129,3,41,129,3,83,3,55,131,82,21,15,1,40,62,51,49,94,127,82,1,157,82,68,1,29,58,1,40,14,15,18,
    105,129,5,44,53,58,64,77,5,84,50,1,15,66,34,222,27,100,73,63,72,119,17,84,21,1,1,43,74,11,7,1,
    77, 33,58,1,64,7,69,17,65,255,26,129,268,136,7,29,5,13,66,64,141,138,26,134,15,468,125,105,17,
    113,37,73,3,1,70,9,63,17,52,224,35,35,39,27,67,7,161,40,17,102,15,90,72,155,53,53,63,23,116,39,
    1,138,7,37,157,31,58,5,156,1,73,3,55,14,61,91,83,67,95,46,82,57,7,11,1,72,52,98,9,15,43,54,29,
    5,225,16,13,39,1,3,94,21,86,13,19,11,7,61,234,110,88,118,9,1,319,5,59,7,25,9,88,26,135,93,21,87,
    68,58,21,5,193,35,89,5,51,13,39,28,102,127,5,49,62,1,178,118,7,39,28,7,11,183,32,13,7,9,1,29,5,
    65,38,115,9,29,38,40,56,17,1,57,250,7,90,22,7,3,3,155,101,3,72,25,66,281,317,48,1,17,142,56,59,
    96,11,7,29,150,21,3,180,146,1,227,11,173,181,98,214,19,91,1,60,77,1,84,236,184,83,12,22,136,21,
    3,226,209,87,47,1,1,87,15,83,67,27,473,219,146,70,156,71,223,190,208,124,33,202,125,13,207,208,
    158,154,60,153,101,27,183,227,104,9,59,11,55,87,201,23,3,29,21,118,206,152,30,7,160,110,161,11,
    165,167,181,183,105,27,7,41,220,113,11,152,69,164,17,27,53,27,567,121,84,55,78,61,15,54,3,7,180,
    230,25,183,197,169,152,51,132,63,3,149,3,133,130,25,5,118,46,46,167,171,31,1,123,39,122,50,27,
    51,108,65,40,39,131,74,9,78,71,38,214,24,7,40,120,116,93,122,145,99,162,298,188,60,78,13,151,1,
    41,152,74,49,53,209,107,19,39,5,155,188,216,148,185,41,83,5,129,43,73,111,122,198,230,76,15,69,
    7,19,208,174,172,151,86,47,1,7,155,77,13,232,108,135,110,105,17,19,86,1,51,78,5,156,35,3,77,149,
    159,80,150,121,148,194,212,9,130,123,64,96,150,217,89,31,1,109,13,55,68,98,114,45,151,7,29,155,
    7,117,53,3,57,35,108,7,181,71,155,239,73,31,53,90,187,23,27,52,5,9,192,219,43,61,27,96,1,152,73,
    183,83,7,3,203,113,17,5,101,137,17,197,181,156,173,107,90,126,7,76,195,36,133,11,1,3,83,11,167,
    136,164,78,170,19,1,86,6,5,180,27,205,117,11,72,140,11,124,9,99,209,40,163,7,91,96,134,198,149,
    195,138,51,1,165,85,112,200,17,61,15,145,64,129,5,59,149,62,13,137,45,15,27,119,63,82,308,78,
    140,59,8,1,63,1,102,93,153,129,63,7,147,78,49,129,158,39,5,67,3,36,219,55,5,69,78,3,589,67,180,
    170,103,62,53,48,177,68,4,47,3,11,73,108,23,1,583,21,17,136,159,7,25,114,85,63,15,3,13,1,51,1,
    123,42,108,85,123,75,108,7,47,92,5,114,15,1,111,22,165,7,41,97,35,23,29,11,125,119,5,108,92,79,
    1,67,72,166,29,66,42,54,71,50,97,88,1,138,11,5,53,3,1,236,146,153,21,20,11,46,189,19,9,21,1,82,
    85,39,15,1,81,1,15,78,49,80,1,23,13,4,64,19,9,166,9,99,15,119,47,1,23,59,4,5,1,79,1,10,42,17,51,
    19,55,1,74,22,54,3,86,5,7,1,94,7,63,5,36,103,25,195,108,53,15,111,25,22,38,123,116,3,11,106,26,
    29,9,11,76,9,38,9,301,99,210,1,7,7,188,7,73,65,72,167,120,5,79,131,47,54,9,54,101,111,78,81,38,
    34,17,90,17,19,30,148,76,5,204,170,179,119,133,3,62,3,29,69,76,20,158,171,49,54,3,134,20,11,93,
    30,1,1,3,79,24,54,11,69,86,44,175,63,16,59,1,62,3,47,15,50,19,55,13,34,157,79,4,117,35,200,3,52,
    11,78,109,109,87,36,87,171,223,156,110,3,186,251,123,26,7,47,3,199,109,128,54,13,143,181,129,
    133,201,56,1,209,114,18,77,228,166,157,40,42,1,62,238,119,27,20,3,96,70,13,27,117,57,61,212,141,
    41,103,56,3,95,5,91,119,3,215,282,165,40,192,18,11,55,63,13,45,55,48,163,148,12,63,113,19};

  ASSERT_EQ(1000, coinbase_output_dist.size());
  ASSERT_EQ(1000, total_output_dist.size());

  // Make these distributions cumulative
  for (size_t i = 1; i < coinbase_output_dist.size(); ++i)
  {
    coinbase_output_dist[i] += coinbase_output_dist[i - 1];
  }
  for (size_t i = 1; i < total_output_dist.size(); ++i)
  {
    total_output_dist[i] += total_output_dist[i - 1];
  }

  auto is_output_coinbase = [&](const uint64_t o) -> bool
  {
    auto rct_it = std::upper_bound(total_output_dist.cbegin(), total_output_dist.cend(), o);
    CHECK_AND_ASSERT_THROW_MES(rct_it != total_output_dist.cend(), "Can't determine whether this transfer is coinbase or not b/c the index is too high");
    const size_t block_offset = std::distance(total_output_dist.cbegin(), rct_it);
    const uint64_t offset_base = block_offset ? total_output_dist[block_offset - 1] : 0;
    const uint64_t index_inside_block = o - offset_base;
    const uint64_t first_non_coinbase_in_block = coinbase_output_dist[block_offset] -
      (block_offset ? coinbase_output_dist[block_offset - 1] : 0);
    const bool transfer_is_coinbase = index_inside_block < first_non_coinbase_in_block;
    return transfer_is_coinbase;
  };

  EXPECT_TRUE(is_output_coinbase(total_output_dist[10]));
}
