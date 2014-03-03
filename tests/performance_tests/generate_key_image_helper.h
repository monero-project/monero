// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"

#include "single_tx_test_base.h"

class test_generate_key_image_helper : public single_tx_test_base
{
public:
  static const size_t loop_count = 500;

  bool test()
  {
    cryptonote::keypair in_ephemeral;
    crypto::key_image ki;
    return cryptonote::generate_key_image_helper(m_bob.get_keys(), m_tx_pub_key, 0, in_ephemeral, ki);
  }
};
