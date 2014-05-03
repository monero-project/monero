// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"

#include "multi_tx_test_base.h"

template<size_t a_in_count, size_t a_out_count>
class test_construct_tx : private multi_tx_test_base<a_in_count>
{
  static_assert(0 < a_in_count, "in_count must be greater than 0");
  static_assert(0 < a_out_count, "out_count must be greater than 0");

public:
  static const size_t loop_count = (a_in_count + a_out_count < 100) ? 100 : 10;
  static const size_t in_count  = a_in_count;
  static const size_t out_count = a_out_count;

  typedef multi_tx_test_base<a_in_count> base_class;

  bool init()
  {
    using namespace cryptonote;

    if (!base_class::init())
      return false;

    m_alice.generate();

    for (size_t i = 0; i < out_count; ++i)
    {
      m_destinations.push_back(tx_destination_entry(this->m_source_amount / out_count, m_alice.get_keys().m_account_address));
    }

    return true;
  }

  bool test()
  {
    return cryptonote::construct_tx(this->m_miners[this->real_source_idx].get_keys(), this->m_sources, m_destinations, std::vector<uint8_t>(), m_tx, 0);
  }

private:
  cryptonote::account_base m_alice;
  std::vector<cryptonote::tx_destination_entry> m_destinations;
  cryptonote::transaction m_tx;
};
