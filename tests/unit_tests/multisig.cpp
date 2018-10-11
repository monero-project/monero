// Copyright (c) 2017-2018, The Monero Project
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

#include <cstdint>

#include "wallet/wallet2.h"

static const struct
{
  const char *address;
  const char *spendkey;
} test_addresses[] =
{
  {
    "T6SkvQ3GyyNRnY5UAw3sMZLTExBSCLvpCAP8sdPuPhJ2BjfsUoBes3eGUJx2fZf7S8Fsa7urJ7LamRTt4uucAtqk1wiLrE6zj",
    "2dd6e34a234c3e8b5d29a371789e4601e96dee4ea6f7ef79224d1a2d91164c01"
  },
  {
    "T6TfbGiuGXA1Fz3USjnSYJcb771AkkNmqgmoAiHAu5713pewPFyAA9wcmemSiYxWtqcvARA3r4L6QUD7VXodxC7n2WQS5M3q1",
    "fac47aecc948ce9d3531aa042abb18235b1df632087c55a361b632ffdd6ede0c"
  },
  {
    "T6SLoeEpwLA6emvHeethY1JkPzMHbAAHjQ4hH3zpo5JZBwZX8UxmjPjQG8tyg5HJaKVK6eWHENbPA4fKPw9eEkG71Jgj85miT",
    "bbd3175ef9fd9f5eefdc43035f882f74ad14c4cf1799d8b6f9001bc197175d02"
  },
  {
    "T6TrTRFMVcKUFcmoSBScYXdhie6sp2b4zaV5VLHFFJv488jRD62zgYcYzzJWeSz2MP1J9g6kDTDNr674CVuWFy1o2JcQW9Zxq",
    "f2efae45bef1917a7430cda8fcffc4ee010e3178761aa41d4628e23b1fe2d501"
  },
  {
    "T6ShAAdn81gb2eeTyHSuiuLdEedYwnGMK3PxqRekb2Nj3RXNzB7mjxKGt5aucMN3BRJHAstbGdqsi1oL6Es5tTju362tAWn3N",
    "a4cef54ed3fd61cd78a2ceb82ecf85a903ad2db9a86fb77ff56c35c56016280a"
  }
};

static const size_t KEYS_COUNT = 5;

static void make_wallet(unsigned int idx, tools::wallet2 &wallet)
{
  ASSERT_TRUE(idx < sizeof(test_addresses) / sizeof(test_addresses[0]));

  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(test_addresses[idx].spendkey, spendkey);

  try
  {
    wallet.init("");
    wallet.set_subaddress_lookahead(1, 1);
    wallet.generate("", "", spendkey, true, false);
    ASSERT_TRUE(test_addresses[idx].address == wallet.get_account().get_public_address_str(cryptonote::TESTNET));
    wallet.decrypt_keys("");
    ASSERT_TRUE(test_addresses[idx].spendkey == epee::string_tools::pod_to_hex(wallet.get_account().get_keys().m_spend_secret_key));
    wallet.encrypt_keys("");
  }
  catch (const std::exception &e)
  {
    MFATAL("Error creating test wallet: " << e.what());
    ASSERT_TRUE(0);
  }
}

static std::vector<std::string> exchange_round(std::vector<tools::wallet2>& wallets, const std::vector<std::string>& mis)
{
  std::vector<std::string> new_infos;
  for (size_t i = 0; i < wallets.size(); ++i) {
      new_infos.push_back(wallets[i].exchange_multisig_keys("", mis));
  }

  return new_infos;
}

static void make_wallets(std::vector<tools::wallet2>& wallets, unsigned int M)
{
  ASSERT_TRUE(wallets.size() > 1 && wallets.size() <= KEYS_COUNT);
  ASSERT_TRUE(M <= wallets.size());

  std::vector<std::string> mis(wallets.size());

  for (size_t i = 0; i < wallets.size(); ++i) {
    make_wallet(i, wallets[i]);

    wallets[i].decrypt_keys("");
    mis[i] = wallets[i].get_multisig_info();
    wallets[i].encrypt_keys("");
  }

  for (auto& wallet: wallets) {
    ASSERT_FALSE(wallet.multisig() || wallet.multisig() || wallet.multisig());
  }

  std::vector<std::string> mxis;
  for (size_t i = 0; i < wallets.size(); ++i) {
    // it's ok to put all of multisig keys in this function. it throws in case of error
    mxis.push_back(wallets[i].make_multisig("", mis, M));
  }

  while (!mxis[0].empty()) {
    mxis = exchange_round(wallets, mxis);
  }

  for (size_t i = 0; i < wallets.size(); ++i) {
    ASSERT_TRUE(mxis[i].empty());
    bool ready;
    uint32_t threshold, total;
    ASSERT_TRUE(wallets[i].multisig(&ready, &threshold, &total));
    ASSERT_TRUE(ready);
    ASSERT_TRUE(threshold == M);
    ASSERT_TRUE(total == wallets.size());

    if (i != 0) {
      // "equals" is transitive relation so we need only to compare first wallet's address to each others' addresses. no need to compare 0's address with itself.
      ASSERT_TRUE(wallets[0].get_account().get_public_address_str(cryptonote::TESTNET) == wallets[i].get_account().get_public_address_str(cryptonote::TESTNET));
    }
  }
}

TEST(multisig, make_2_2)
{
  std::vector<tools::wallet2> wallets(2);
  make_wallets(wallets, 2);
}

TEST(multisig, make_3_3)
{
  std::vector<tools::wallet2> wallets(3);
  make_wallets(wallets, 3);
}

TEST(multisig, make_2_3)
{
  std::vector<tools::wallet2> wallets(3);
  make_wallets(wallets, 2);
}

TEST(multisig, make_2_4)
{
  std::vector<tools::wallet2> wallets(4);
  make_wallets(wallets, 2);
}

TEST(multisig, make_2_5)
{
  std::vector<tools::wallet2> wallets(5);
  make_wallets(wallets, 2);
}
