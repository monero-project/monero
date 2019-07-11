// Copyright (c) 2014-2018, The Monero Project
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

#include <memory>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "checkpoints/checkpoints.cpp"
#include "blockchain_db/testdb.h"


using namespace cryptonote;

struct TestDB: public BaseTestDB
{
  TestDB() { m_open = true; }

  virtual void update_block_checkpoint(checkpoint_t const &checkpoint) override
  {
    auto it = std::find_if(checkpoints.begin(), checkpoints.end(), [&checkpoint](checkpoint_t const &entry) {
        bool result = checkpoint.height == entry.height;
        return result;
    });

    if (it == checkpoints.end()) checkpoints.push_back(checkpoint);
    else                         *it = checkpoint;
  }

  virtual bool get_block_checkpoint(uint64_t height, checkpoint_t &checkpoint) const override
  {
    auto it = std::find_if(checkpoints.begin(), checkpoints.end(), [height](checkpoint_t const &entry) {
        bool result = height == entry.height;
        return result;
    });

    if (it == checkpoints.end())
      return false;

    checkpoint = *it;
    return true;
  }

  virtual bool get_top_checkpoint(checkpoint_t &checkpoint) const override
  {
    if (checkpoints.empty())
      return false;

    checkpoint = checkpoints.back();
    return true;
  }

  virtual std::vector<checkpoint_t> get_checkpoints_range(uint64_t start, uint64_t end, size_t num_desired_checkpoints) const override
  {
    std::vector<checkpoint_t> result;

    checkpoint_t top_checkpoint = {};
    if (get_top_checkpoint(top_checkpoint))
    {
      if (top_checkpoint.height < std::min(start, end))
        return result;
    }

    if (num_desired_checkpoints == 0)
      num_desired_checkpoints = (size_t)-1;
    else
      result.reserve(num_desired_checkpoints);

    for (uint64_t height = start;
         height != end && result.size() < num_desired_checkpoints;
         )
    {
      checkpoint_t checkpoint;
      if (get_block_checkpoint(height, checkpoint))
        result.push_back(checkpoint);

      if (end >= start) height++;
      else height--;
    }

    if (result.size() < num_desired_checkpoints)
    {
      // NOTE: Inclusive of end height
      checkpoint_t checkpoint;
      if (get_block_checkpoint(end, checkpoint))
        result.push_back(checkpoint);
    }

    return result;
  }

private:
  std::vector<checkpoint_t> checkpoints;
};

TEST(checkpoints_is_alternative_block_allowed, handles_empty_checkpoints)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  ASSERT_FALSE(cp.is_alternative_block_allowed(0, 0));
  ASSERT_TRUE(cp.is_alternative_block_allowed(1, 1));
  ASSERT_TRUE(cp.is_alternative_block_allowed(1, 9));
  ASSERT_TRUE(cp.is_alternative_block_allowed(9, 1));
}

TEST(checkpoints_is_alternative_block_allowed, handles_one_checkpoint)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  ASSERT_TRUE(cp.add_checkpoint(5, "0000000000000000000000000000000000000000000000000000000000000000"));

  ASSERT_FALSE(cp.is_alternative_block_allowed(0, 0));

  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 1));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 4));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 9));

  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 1));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 4));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 9));

  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 9));

  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 9));

  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(9, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(9, 9));
}

TEST(checkpoints_is_alternative_block_allowed, handles_two_and_more_checkpoints)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  ASSERT_TRUE(cp.add_checkpoint(5, "0000000000000000000000000000000000000000000000000000000000000000"));
  ASSERT_TRUE(cp.add_checkpoint(9, "0000000000000000000000000000000000000000000000000000000000000000"));

  ASSERT_FALSE(cp.is_alternative_block_allowed(0, 0));

  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 1));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 4));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 8));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(1, 11));

  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 1));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 4));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 8));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(4, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(5, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 8));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(5, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 8));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(6, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(8, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(8, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(8, 5));
  ASSERT_TRUE (cp.is_alternative_block_allowed(8, 6));
  ASSERT_TRUE (cp.is_alternative_block_allowed(8, 8));
  ASSERT_TRUE (cp.is_alternative_block_allowed(8, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(8, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(8, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 5));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 6));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 8));
  ASSERT_FALSE(cp.is_alternative_block_allowed(9, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(9, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(9, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 5));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 6));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 8));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(10, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(10, 11));

  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 1));
  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 5));
  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 6));
  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 8));
  ASSERT_FALSE(cp.is_alternative_block_allowed(11, 9));
  ASSERT_TRUE (cp.is_alternative_block_allowed(11, 10));
  ASSERT_TRUE (cp.is_alternative_block_allowed(11, 11));
}

TEST(checkpoints_is_alternative_block_allowed, override_1_sn_checkpoint)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  checkpoint_t checkpoint = {};
  checkpoint.type         = checkpoint_type::service_node;
  checkpoint.height       = 5;
  test_db->update_block_checkpoint(checkpoint);

  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 6));

  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 6));

  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 6));
}

TEST(checkpoints_is_alternative_block_allowed, cant_override_2nd_oldest_sn_checkpoint)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  checkpoint_t checkpoint = {};
  checkpoint.type         = checkpoint_type::service_node;
  checkpoint.height       = 5;
  test_db->update_block_checkpoint(checkpoint);

  checkpoint.height       = 10;
  test_db->update_block_checkpoint(checkpoint);


  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(4, 6));

  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(5, 6));

  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 4));
  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 6));

  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(10, 5));
  ASSERT_TRUE(cp.is_alternative_block_allowed(10, 6));
}

TEST(checkpoints_is_alternative_block_allowed, hardcoded_checkpoint_overrides_sn_checkpoint)
{
  std::unique_ptr<TestDB> test_db(new TestDB());
  checkpoints cp = {}; cp.init(cryptonote::FAKECHAIN, test_db.get());

  checkpoint_t checkpoint = {};
  checkpoint.type         = checkpoint_type::service_node;
  checkpoint.height       = 5;
  test_db->update_block_checkpoint(checkpoint);

  checkpoint.height       = 10;
  test_db->update_block_checkpoint(checkpoint);

  ASSERT_TRUE(cp.add_checkpoint(6, "0000000000000000000000000000000000000000000000000000000000000000"));

  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 4));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 5));
  ASSERT_FALSE(cp.is_alternative_block_allowed(6, 6));
  ASSERT_TRUE(cp.is_alternative_block_allowed(6, 7));

  ASSERT_TRUE(cp.is_alternative_block_allowed(7, 7));
  ASSERT_TRUE(cp.is_alternative_block_allowed(7, 8));
  ASSERT_TRUE(cp.is_alternative_block_allowed(8, 9));
}
