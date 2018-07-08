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

#include "gtest/gtest.h"

#include "cryptonote_basic/account.h"

TEST(account, encrypt_keys)
{
  cryptonote::keypair recovery_key = cryptonote::keypair::generate(hw::get_device("default"));
  cryptonote::account_base account;
  crypto::secret_key key = account.generate(recovery_key.sec);
  const cryptonote::account_keys keys = account.get_keys();

  ASSERT_EQ(account.get_keys().m_account_address, keys.m_account_address);
  ASSERT_EQ(account.get_keys().m_spend_secret_key, keys.m_spend_secret_key);
  ASSERT_EQ(account.get_keys().m_view_secret_key, keys.m_view_secret_key);
  ASSERT_EQ(account.get_keys().m_multisig_keys, keys.m_multisig_keys);

  crypto::chacha_key chacha_key;
  crypto::generate_chacha_key(&recovery_key, sizeof(recovery_key), chacha_key, 1);

  account.encrypt_keys(chacha_key);

  ASSERT_EQ(account.get_keys().m_account_address, keys.m_account_address);
  ASSERT_NE(account.get_keys().m_spend_secret_key, keys.m_spend_secret_key);
  ASSERT_NE(account.get_keys().m_view_secret_key, keys.m_view_secret_key);

  account.decrypt_viewkey(chacha_key);

  ASSERT_EQ(account.get_keys().m_account_address, keys.m_account_address);
  ASSERT_NE(account.get_keys().m_spend_secret_key, keys.m_spend_secret_key);
  ASSERT_EQ(account.get_keys().m_view_secret_key, keys.m_view_secret_key);

  account.encrypt_viewkey(chacha_key);

  ASSERT_EQ(account.get_keys().m_account_address, keys.m_account_address);
  ASSERT_NE(account.get_keys().m_spend_secret_key, keys.m_spend_secret_key);
  ASSERT_NE(account.get_keys().m_view_secret_key, keys.m_view_secret_key);

  account.decrypt_keys(chacha_key);

  ASSERT_EQ(account.get_keys().m_account_address, keys.m_account_address);
  ASSERT_EQ(account.get_keys().m_spend_secret_key, keys.m_spend_secret_key);
  ASSERT_EQ(account.get_keys().m_view_secret_key, keys.m_view_secret_key);
}
