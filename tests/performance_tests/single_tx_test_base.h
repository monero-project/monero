// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"

class single_tx_test_base
{
public:
  bool init()
  {
    using namespace cryptonote;

    m_bob.generate();

    if (!construct_miner_tx(0, 0, 0, 2, 0, m_bob.get_keys().m_account_address, m_tx))
      return false;

    m_tx_pub_key = get_tx_pub_key_from_extra(m_tx);
    return true;
  }

protected:
  cryptonote::account_base m_bob;
  cryptonote::transaction m_tx;
  crypto::public_key m_tx_pub_key;
};
