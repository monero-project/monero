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
#include <boost/filesystem.hpp>
#include <boost/optional/optional_io.hpp>
#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "wallet/wallet2.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "wallet/api/subaddress.h"

class WalletSubaddress : public ::testing::Test 
{
  protected:
    virtual void SetUp() 
    {
      try
      {
        w1.generate("", password, recovery_key, true, false);
      }
      catch (const std::exception& e)
      {
        LOG_ERROR("failed to generate wallet: " << e.what());
        throw;
      }

      w1.add_subaddress_account(test_label);
      w1.set_subaddress_label(subaddress_index, test_label);
    }

    virtual void TearDown()
    {
    }

    tools::wallet2 w1;
    const std::string password = "testpass";
    crypto::secret_key recovery_key = crypto::secret_key();
    const std::string test_label = "subaddress test label";

    uint32_t major_index = 0;
    uint32_t minor_index = 0;
    const cryptonote::subaddress_index subaddress_index = {major_index, minor_index};
};

TEST_F(WalletSubaddress, GetSubaddressLabel)
{
  EXPECT_EQ(test_label, w1.get_subaddress_label(subaddress_index));
}

TEST_F(WalletSubaddress, AddSubaddress)
{
  std::string label = "test adding subaddress";
  w1.add_subaddress(0, label);
  EXPECT_EQ(label, w1.get_subaddress_label({0, 1}));
}

TEST_F(WalletSubaddress, OutOfBoundsIndexes)
{
  try 
  {
    w1.get_subaddress_label({1,0});
  } 
  catch(const std::exception& e)
  {
    EXPECT_STREQ("index_major is out of bound", e.what());  
  }   
  try 
  {
    w1.get_subaddress_label({0,2});
  } 
  catch(const std::exception& e)
  {
    EXPECT_STREQ("index.minor is out of bound", e.what());  
  }   
}

// Helper function to check max subaddrs allocated
static void check_expected_max(const tools::wallet2 &w1, const cryptonote::subaddress_index exp_max)
{
  for (uint32_t i = 0; i <= exp_max.minor; ++i)
  {
    auto subaddr = w1.get_subaddress({exp_max.major, i});
    EXPECT_NE(boost::none, w1.get_subaddress_index(subaddr));
  }
  auto subaddr = w1.get_subaddress({exp_max.major, exp_max.minor + 1});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(subaddr));
};

static void expect_default_wallet_state(const tools::wallet2 &w1)
{
  // these tests assume we are starting with the default setup state
  EXPECT_EQ(2, w1.get_num_subaddress_accounts());
  EXPECT_EQ(50, w1.get_subaddress_lookahead().first);
  EXPECT_EQ(200, w1.get_subaddress_lookahead().second);

  // We assume we start with subaddrs for minor indexes 0 to 199
  check_expected_max(w1, {0,199});
  check_expected_max(w1, {1,199});
  check_expected_max(w1, {49,199});
  check_expected_max(w1, {50,199}); // 50 because the test starts with accounts 0 and 1 already allocated
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({51,0})));
}

TEST_F(WalletSubaddress, SetLookahead)
{
  expect_default_wallet_state(w1);
  // get_subaddress_index looks up keys in the private m_subaddresses dictionary so we will use it to test if a key is properly being scanned for
  cryptonote::subaddress_index test_idx = {50, 199};
  auto subaddr = w1.get_subaddress(test_idx);
  EXPECT_NE(boost::none, w1.get_subaddress_index(subaddr));
  // fist test expanding the major lookahead
  w1.set_subaddress_lookahead(100, 200);
  EXPECT_EQ(100, w1.get_subaddress_lookahead().first);
  EXPECT_EQ(200, w1.get_subaddress_lookahead().second);
  check_expected_max(w1, {100, 199});
  // next test expanding the minor lookahead
  w1.set_subaddress_lookahead(100, 300);
  EXPECT_EQ(100, w1.get_subaddress_lookahead().first);
  EXPECT_EQ(300, w1.get_subaddress_lookahead().second);
  check_expected_max(w1, {100, 299});
}

TEST_F(WalletSubaddress, ExpandThenSetMinorIncreaseOnly)
{
  expect_default_wallet_state(w1);

  // Mock receive to {0,150}, so expand from there
  w1.expand_subaddresses({0,150});
  // We should now have subaddresses for minor indexes 0 to 349
  check_expected_max(w1, {0,349});
  check_expected_max(w1, {1,199});
  check_expected_max(w1, {49,199});
  check_expected_max(w1, {50,199});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({51,0})));

  // Now set the minor lookahead 100 higher
  w1.set_subaddress_lookahead(50, 200+100);
  // We should have subaddresses for minor indexes 0 to 449
  check_expected_max(w1, {0,449});
  check_expected_max(w1, {1,299});
  check_expected_max(w1, {49,299});
  check_expected_max(w1, {50,299});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({51,0})));
}

TEST_F(WalletSubaddress, ExpandThenSetMajorIncreaseOnly)
{
  expect_default_wallet_state(w1);

  // Mock receive to {40,0}, so expand from there
  w1.expand_subaddresses({40,0});
  check_expected_max(w1, {0,199});
  check_expected_max(w1, {1,199});
  check_expected_max(w1, {40,199});
  check_expected_max(w1, {89,199});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({90,0})));

  // Now set the major lookahead 10 higher
  w1.set_subaddress_lookahead(50+10, 200);
  check_expected_max(w1, {0,199});
  check_expected_max(w1, {1,199});
  check_expected_max(w1, {40,199});
  check_expected_max(w1, {99,199});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({100,0})));
}

TEST_F(WalletSubaddress, ExpandThenSetIncreaseBoth)
{
  expect_default_wallet_state(w1);

  // Mock receive to {40,150}, so expand from there
  w1.expand_subaddresses({40,150});
  check_expected_max(w1, {0,199});
  check_expected_max(w1, {1,199});
  check_expected_max(w1, {40,349});
  check_expected_max(w1, {89,199});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({90,0})));

  // Now set the major lookahead 10 higher and minor 100 higher
  w1.set_subaddress_lookahead(50+10, 200+100);
  check_expected_max(w1, {0,299});
  check_expected_max(w1, {1,299});
  check_expected_max(w1, {40,449});
  check_expected_max(w1, {99,299});
  EXPECT_EQ(boost::none, w1.get_subaddress_index(w1.get_subaddress({100,0})));
}
