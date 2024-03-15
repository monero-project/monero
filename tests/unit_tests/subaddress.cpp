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
