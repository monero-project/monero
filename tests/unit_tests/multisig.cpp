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

#include "crypto/crypto.h"
#include "multisig/multisig_account.h"
#include "multisig/multisig_kex_msg.h"
#include "ringct/rctOps.h"
#include "wallet/wallet2.h"

#include "gtest/gtest.h"

#include <cstdint>

static const struct
{
  const char *address;
  const char *spendkey;
} test_addresses[] =
{
  {
    "9uvjbU54ZJb8j7Dcq1h3F1DnBRkxXdYUX4pbJ7mE3ghM8uF3fKzqRKRNAKYZXcNLqMg7MxjVVD2wKC2PALUwEveGSC3YSWD",
    "2dd6e34a234c3e8b5d29a371789e4601e96dee4ea6f7ef79224d1a2d91164c01"
  },
  {
    "9ywDBAyDbb6QKFiZxDJ4hHZqZEQXXCR5EaYNcndUpqPDeE7rEgs6neQdZnhcDrWbURYK8xUjhuG2mVjJdmknrZbcG7NnbaB",
    "fac47aecc948ce9d3531aa042abb18235b1df632087c55a361b632ffdd6ede0c"
  },
  {
    "9t6Hn946u3eah5cuncH1hB5hGzsTUoevtf4SY7MHN5NgJZh2SFWsyVt3vUhuHyRKyrCQvr71Lfc1AevG3BXE11PQFoXDtD8",
    "bbd3175ef9fd9f5eefdc43035f882f74ad14c4cf1799d8b6f9001bc197175d02"
  },
  {
    "9zmAWoNyNPbgnYSm3nJNpAKHm6fCcs3MR94gBWxp9MCDUiMUhyYFfyQETUDLPF7DP6ZsmNo6LRxwPP9VmhHNxKrER9oGigT",
    "f2efae45bef1917a7430cda8fcffc4ee010e3178761aa41d4628e23b1fe2d501"
  },
  {
    "9ue8NJMg3WzKxTtmjeXzWYF5KmU6dC7LHEt9wvYdPn2qMmoFUa8hJJHhSHvJ46UEwpDyy5jSboNMRaDBKwU54NT42YcNUp5",
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
    wallet.init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
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

static std::vector<std::string> exchange_round(std::vector<tools::wallet2>& wallets, const std::vector<std::string>& infos)
{
  std::vector<std::string> new_infos;
  new_infos.reserve(infos.size());

  for (size_t i = 0; i < wallets.size(); ++i)
    new_infos.push_back(wallets[i].exchange_multisig_keys("", infos));

  return new_infos;
}

static std::vector<std::string> exchange_round_force_update(std::vector<tools::wallet2>& wallets,
  const std::vector<std::string>& infos,
  const std::size_t round_in_progress)
{
  EXPECT_TRUE(wallets.size() == infos.size());
  std::vector<std::string> new_infos;
  std::vector<std::string> temp_force_update_infos;
  new_infos.reserve(infos.size());

  // when force-updating, we only need at most 'num_signers - 1 - (round - 1)' messages from other signers
  size_t num_other_messages_required{wallets.size() - 1 - (round_in_progress - 1)};
  if (num_other_messages_required > wallets.size())
    num_other_messages_required = 0;  //overflow case for post-kex verification round of 1-of-N

  for (size_t i = 0; i < wallets.size(); ++i)
  {
    temp_force_update_infos.clear();
    temp_force_update_infos.reserve(num_other_messages_required + 1);
    temp_force_update_infos.push_back(infos[i]);  //always include the local signer's message for this round

    size_t infos_collected{0};
    for (size_t wallet_index = 0; wallet_index < wallets.size(); ++wallet_index)
    {
      // skip the local signer's message
      if (wallet_index == i)
        continue;

      temp_force_update_infos.push_back(infos[wallet_index]);
      ++infos_collected;

      if (infos_collected == num_other_messages_required)
        break;
    }

    new_infos.push_back(wallets[i].exchange_multisig_keys("", temp_force_update_infos, true));
  }

  return new_infos;
}

static void check_results(const std::vector<std::string> &intermediate_infos,
  std::vector<tools::wallet2>& wallets,
  const std::uint32_t M)
{
  // check results
  std::unordered_set<crypto::secret_key> unique_privkeys;
  rct::key composite_pubkey = rct::identity();

  ASSERT_TRUE(wallets.size() > 0);
  wallets[0].decrypt_keys("");
  crypto::public_key spend_pubkey = wallets[0].get_account().get_keys().m_account_address.m_spend_public_key;
  crypto::secret_key view_privkey = wallets[0].get_account().get_keys().m_view_secret_key;
  crypto::public_key view_pubkey;
  EXPECT_TRUE(crypto::secret_key_to_public_key(view_privkey, view_pubkey));
  wallets[0].encrypt_keys("");

  // at the end of multisig kex, all wallets should emit a post-kex message with the same two pubkeys
  std::vector<crypto::public_key> post_kex_msg_pubkeys;
  ASSERT_TRUE(intermediate_infos.size() == wallets.size());
  for (const std::string &intermediate_info : intermediate_infos)
  {
    multisig::multisig_kex_msg post_kex_msg;
    EXPECT_TRUE(!intermediate_info.empty());
    EXPECT_NO_THROW(post_kex_msg = intermediate_info);

    if (post_kex_msg_pubkeys.size() != 0)
      EXPECT_TRUE(post_kex_msg_pubkeys == post_kex_msg.get_msg_pubkeys());  //assumes sorting is always the same
    else
      post_kex_msg_pubkeys = post_kex_msg.get_msg_pubkeys();

    EXPECT_TRUE(post_kex_msg_pubkeys.size() == 2);
  }

  // the post-kex pubkeys should equal the account's public view and spend keys
  EXPECT_TRUE(std::find(post_kex_msg_pubkeys.begin(), post_kex_msg_pubkeys.end(), spend_pubkey) != post_kex_msg_pubkeys.end());
  EXPECT_TRUE(std::find(post_kex_msg_pubkeys.begin(), post_kex_msg_pubkeys.end(), view_pubkey) != post_kex_msg_pubkeys.end());

  // each wallet should have the same state (private view key, public spend key), and the public spend key should be
  //   reproducible from the private spend keys found in each account
  for (tools::wallet2 &wallet : wallets)
  {
    wallet.decrypt_keys("");
    const multisig::multisig_account_status ms_status{wallet.get_multisig_status()};
    EXPECT_TRUE(ms_status.multisig_is_active);
    EXPECT_TRUE(ms_status.kex_is_done);
    EXPECT_TRUE(ms_status.is_ready);
    EXPECT_TRUE(ms_status.threshold == M);
    EXPECT_TRUE(ms_status.total == wallets.size());

    EXPECT_TRUE(wallets[0].get_account().get_public_address_str(cryptonote::TESTNET) ==
      wallet.get_account().get_public_address_str(cryptonote::TESTNET));
    
    EXPECT_EQ(spend_pubkey, wallet.get_account().get_keys().m_account_address.m_spend_public_key);
    EXPECT_EQ(view_privkey, wallet.get_account().get_keys().m_view_secret_key);
    EXPECT_EQ(view_pubkey, wallet.get_account().get_keys().m_account_address.m_view_public_key);

    // sum together unique multisig keys
    for (const auto &privkey : wallet.get_account().get_keys().m_multisig_keys)
    {
      EXPECT_NE(privkey, crypto::null_skey);

      if (unique_privkeys.find(privkey) == unique_privkeys.end())
      {
        unique_privkeys.insert(privkey);
        crypto::public_key pubkey;
        EXPECT_TRUE(crypto::secret_key_to_public_key(privkey, pubkey));
        EXPECT_NE(privkey, crypto::null_skey);
        EXPECT_NE(pubkey, crypto::null_pkey);
        EXPECT_NE(pubkey, rct::rct2pk(rct::identity()));
        rct::addKeys(composite_pubkey, composite_pubkey, rct::pk2rct(pubkey));
      }
    }
    wallet.encrypt_keys("");
  }

  // final key via sum of privkeys should equal the wallets' public spend key
  wallets[0].decrypt_keys("");
  EXPECT_EQ(wallets[0].get_account().get_keys().m_account_address.m_spend_public_key, rct::rct2pk(composite_pubkey));
  wallets[0].encrypt_keys("");
}

static void make_wallets(const unsigned int M, const unsigned int N, const bool force_update)
{
  std::vector<tools::wallet2> wallets(N);
  ASSERT_TRUE(wallets.size() > 1 && wallets.size() <= KEYS_COUNT);
  ASSERT_TRUE(M <= wallets.size());
  std::uint32_t total_rounds_required = multisig::multisig_setup_rounds_required(wallets.size(), M);
  std::uint32_t rounds_complete{0};

  // initialize wallets, get first round multisig kex msgs
  std::vector<std::string> initial_infos(wallets.size());

  for (size_t i = 0; i < wallets.size(); ++i)
  {
    make_wallet(i, wallets[i]);

    wallets[i].decrypt_keys("");
    initial_infos[i] = wallets[i].get_multisig_first_kex_msg();
    wallets[i].encrypt_keys("");
  }

  // wallets should not be multisig yet
  for (const auto& wallet: wallets)
    ASSERT_FALSE(wallet.get_multisig_status().multisig_is_active);

  // make wallets multisig, get second round kex messages (if appropriate)
  std::vector<std::string> intermediate_infos(wallets.size());

  for (size_t i = 0; i < wallets.size(); ++i)
  {
    intermediate_infos[i] = wallets[i].make_multisig("", initial_infos, M);
  }

  ++rounds_complete;

  // perform kex rounds until kex is complete
  multisig::multisig_account_status ms_status{wallets[0].get_multisig_status()};
  while (!ms_status.is_ready)
  {
    if (force_update)
      intermediate_infos = exchange_round_force_update(wallets, intermediate_infos, rounds_complete + 1);
    else
      intermediate_infos = exchange_round(wallets, intermediate_infos);

    ms_status = wallets[0].get_multisig_status();
    ++rounds_complete;
  }

  EXPECT_EQ(total_rounds_required, rounds_complete);

  check_results(intermediate_infos, wallets, M);
}

static void make_wallets_boosting(std::vector<tools::wallet2>& wallets, unsigned int M)
{
  ASSERT_TRUE(wallets.size() > 1 && wallets.size() <= KEYS_COUNT);
  ASSERT_TRUE(M <= wallets.size());
  std::uint32_t kex_rounds_required = multisig::multisig_kex_rounds_required(wallets.size(), M);
  std::uint32_t rounds_required = multisig::multisig_setup_rounds_required(wallets.size(), M);
  std::uint32_t rounds_complete{0};

  // initialize wallets, get first round multisig kex msgs
  std::vector<std::string> initial_infos(wallets.size());

  for (size_t i = 0; i < wallets.size(); ++i)
  {
    make_wallet(i, wallets[i]);

    wallets[i].decrypt_keys("");
    initial_infos[i] = wallets[i].get_multisig_first_kex_msg();
    wallets[i].encrypt_keys("");
  }

  // wallets should not be multisig yet
  for (const auto &wallet: wallets)
  {
    const multisig::multisig_account_status ms_status{wallet.get_multisig_status()};
    ASSERT_FALSE(ms_status.multisig_is_active);
  }

  // get round 2 booster messages for wallet0 (if appropriate)
  auto initial_infos_truncated = initial_infos;
  initial_infos_truncated.erase(initial_infos_truncated.begin());

  std::vector<std::string> wallet0_booster_infos;
  wallet0_booster_infos.reserve(wallets.size() - 1);

  if (rounds_complete + 1 < kex_rounds_required)
  {
    for (size_t i = 1; i < wallets.size(); ++i)
    {
      wallet0_booster_infos.push_back(
          wallets[i].get_multisig_key_exchange_booster("", initial_infos_truncated, M, wallets.size())
        );
    }
  }

  // make wallets multisig
  std::vector<std::string> intermediate_infos(wallets.size());

  for (size_t i = 0; i < wallets.size(); ++i)
    intermediate_infos[i] = wallets[i].make_multisig("", initial_infos, M);

  ++rounds_complete;

  // perform all kex rounds
  // boost wallet0 each round, so wallet0 is always 1 round ahead
  std::string wallet0_intermediate_info;
  std::vector<std::string> new_infos(intermediate_infos.size());
  multisig::multisig_account_status ms_status{wallets[0].get_multisig_status()};
  while (!ms_status.is_ready)
  {
    // use booster infos to update wallet0 'early'
    if (rounds_complete < kex_rounds_required)
      new_infos[0] = wallets[0].exchange_multisig_keys("", wallet0_booster_infos);
    else
    {
      // force update the post-kex round with wallet0's post-kex message since wallet0 is 'ahead' of the other wallets
      wallet0_booster_infos = {wallets[0].exchange_multisig_keys("", {})};
      new_infos[0] = wallets[0].exchange_multisig_keys("", wallet0_booster_infos, true);
    }

    // get wallet0 booster infos for next round
    if (rounds_complete + 1 < kex_rounds_required)
    {
      // remove wallet0 info for this round (so boosters have incomplete kex message set)
      auto intermediate_infos_truncated = intermediate_infos;
      intermediate_infos_truncated.erase(intermediate_infos_truncated.begin());

      // obtain booster messages from all other wallets
      for (size_t i = 1; i < wallets.size(); ++i)
      {
        wallet0_booster_infos[i-1] =
          wallets[i].get_multisig_key_exchange_booster("", intermediate_infos_truncated, M, wallets.size());
      }
    }

    // update other wallets
    for (size_t i = 1; i < wallets.size(); ++i)
        new_infos[i] = wallets[i].exchange_multisig_keys("", intermediate_infos);

    intermediate_infos = new_infos;
    ++rounds_complete;
    ms_status = wallets[0].get_multisig_status();
  }

  EXPECT_EQ(rounds_required, rounds_complete);

  check_results(intermediate_infos, wallets, M);
}

TEST(multisig, make_1_2)
{
  make_wallets(1, 2, false);
  make_wallets(1, 2, true);
}

TEST(multisig, make_1_3)
{
  make_wallets(1, 3, false);
  make_wallets(1, 3, true);
}

TEST(multisig, make_2_2)
{
  make_wallets(2, 2, false);
  make_wallets(2, 2, true);
}

TEST(multisig, make_3_3)
{
  make_wallets(3, 3, false);
  make_wallets(3, 3, true);
}

TEST(multisig, make_2_3)
{
  make_wallets(2, 3, false);
  make_wallets(2, 3, true);
}

TEST(multisig, make_2_4)
{
  make_wallets(2, 4, false);
  make_wallets(2, 4, true);
}

TEST(multisig, make_2_4_boosting)
{
  std::vector<tools::wallet2> wallets(4);
  make_wallets_boosting(wallets, 2);
}

TEST(multisig, multisig_kex_msg)
{
  using namespace multisig;

  crypto::public_key pubkey1;
  crypto::public_key pubkey2;
  crypto::public_key pubkey3;
  crypto::secret_key_to_public_key(rct::rct2sk(rct::skGen()), pubkey1);
  crypto::secret_key_to_public_key(rct::rct2sk(rct::skGen()), pubkey2);
  crypto::secret_key_to_public_key(rct::rct2sk(rct::skGen()), pubkey3);

  crypto::secret_key signing_skey = rct::rct2sk(rct::skGen());
  crypto::public_key signing_pubkey;
  while(!crypto::secret_key_to_public_key(signing_skey, signing_pubkey))
  {
    signing_skey = rct::rct2sk(rct::skGen());
  }

  const crypto::secret_key ancillary_skey{rct::rct2sk(rct::skGen())};

  // misc. edge cases
  EXPECT_NO_THROW((multisig_kex_msg{}));
  EXPECT_ANY_THROW((multisig_kex_msg{multisig_kex_msg{}.get_msg()}));
  EXPECT_ANY_THROW((multisig_kex_msg{"abc"}));
  EXPECT_ANY_THROW((multisig_kex_msg{0, crypto::null_skey, std::vector<crypto::public_key>{}, crypto::null_skey}));
  EXPECT_ANY_THROW((multisig_kex_msg{1, crypto::null_skey, std::vector<crypto::public_key>{}, crypto::null_skey}));
  EXPECT_ANY_THROW((multisig_kex_msg{1, signing_skey, std::vector<crypto::public_key>{}, crypto::null_skey}));
  EXPECT_ANY_THROW((multisig_kex_msg{1, crypto::null_skey, std::vector<crypto::public_key>{}, ancillary_skey}));

  // test that messages are both constructible and reversible

  // round 1
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{1, signing_skey, std::vector<crypto::public_key>{}, ancillary_skey}.get_msg()
    }));
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{1, signing_skey, std::vector<crypto::public_key>{pubkey1}, ancillary_skey}.get_msg()
    }));

  // round 2
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{2, signing_skey, std::vector<crypto::public_key>{pubkey1}, ancillary_skey}.get_msg()
    }));
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{2, signing_skey, std::vector<crypto::public_key>{pubkey1}, crypto::null_skey}.get_msg()
    }));
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{2, signing_skey, std::vector<crypto::public_key>{pubkey1, pubkey2}, ancillary_skey}.get_msg()
    }));
  EXPECT_NO_THROW((multisig_kex_msg{
      multisig_kex_msg{2, signing_skey, std::vector<crypto::public_key>{pubkey1, pubkey2, pubkey3}, crypto::null_skey}.get_msg()
    }));

  // test that keys can be recovered if stored in a message and the message's reverse

  // round 1
  const multisig_kex_msg msg_rnd1{1, signing_skey, std::vector<crypto::public_key>{pubkey1}, ancillary_skey};
  const multisig_kex_msg msg_rnd1_reverse{msg_rnd1.get_msg()};
  EXPECT_EQ(msg_rnd1.get_round(), 1);
  EXPECT_EQ(msg_rnd1.get_round(), msg_rnd1_reverse.get_round());
  EXPECT_EQ(msg_rnd1.get_signing_pubkey(), signing_pubkey);
  EXPECT_EQ(msg_rnd1.get_signing_pubkey(), msg_rnd1_reverse.get_signing_pubkey());
  EXPECT_EQ(msg_rnd1.get_msg_pubkeys().size(), 0);
  EXPECT_EQ(msg_rnd1.get_msg_pubkeys().size(), msg_rnd1_reverse.get_msg_pubkeys().size());
  EXPECT_EQ(msg_rnd1.get_msg_privkey(), ancillary_skey);
  EXPECT_EQ(msg_rnd1.get_msg_privkey(), msg_rnd1_reverse.get_msg_privkey());

  // round 2
  const multisig_kex_msg msg_rnd2{2, signing_skey, std::vector<crypto::public_key>{pubkey1, pubkey2}, ancillary_skey};
  const multisig_kex_msg msg_rnd2_reverse{msg_rnd2.get_msg()};
  EXPECT_EQ(msg_rnd2.get_round(), 2);
  EXPECT_EQ(msg_rnd2.get_round(), msg_rnd2_reverse.get_round());
  EXPECT_EQ(msg_rnd2.get_signing_pubkey(), signing_pubkey);
  EXPECT_EQ(msg_rnd2.get_signing_pubkey(), msg_rnd2_reverse.get_signing_pubkey());
  ASSERT_EQ(msg_rnd2.get_msg_pubkeys().size(), 2);
  ASSERT_EQ(msg_rnd2.get_msg_pubkeys().size(), msg_rnd2_reverse.get_msg_pubkeys().size());
  EXPECT_EQ(msg_rnd2.get_msg_pubkeys()[0], pubkey1);
  EXPECT_EQ(msg_rnd2.get_msg_pubkeys()[1], pubkey2);
  EXPECT_EQ(msg_rnd2.get_msg_pubkeys()[0], msg_rnd2_reverse.get_msg_pubkeys()[0]);
  EXPECT_EQ(msg_rnd2.get_msg_pubkeys()[1], msg_rnd2_reverse.get_msg_pubkeys()[1]);
  EXPECT_EQ(msg_rnd2.get_msg_privkey(), crypto::null_skey);
  EXPECT_EQ(msg_rnd2.get_msg_privkey(), msg_rnd2_reverse.get_msg_privkey());
}
