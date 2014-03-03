// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/crypto.h"
#include "cryptonote_core/cryptonote_basic.h"

#include "single_tx_test_base.h"

class test_generate_key_image : public single_tx_test_base
{
public:
  static const size_t loop_count = 1000;

  bool init()
  {
    using namespace cryptonote;

    if (!single_tx_test_base::init())
      return false;

    account_keys bob_keys = m_bob.get_keys();

    crypto::key_derivation recv_derivation;
    crypto::generate_key_derivation(m_tx_pub_key, bob_keys.m_view_secret_key, recv_derivation);

    crypto::derive_public_key(recv_derivation, 0, bob_keys.m_account_address.m_spend_public_key, m_in_ephemeral.pub);
    crypto::derive_secret_key(recv_derivation, 0, bob_keys.m_spend_secret_key, m_in_ephemeral.sec);

    return true;
  }

  bool test()
  {
    crypto::key_image ki;
    crypto::generate_key_image(m_in_ephemeral.pub, m_in_ephemeral.sec, ki);
    return true;
  }

private:
  cryptonote::keypair m_in_ephemeral;
};
