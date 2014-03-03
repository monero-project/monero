// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "crypto/crypto.h"

template<size_t a_ring_size>
class multi_tx_test_base
{
  static_assert(0 < a_ring_size, "ring_size must be greater than 0");

public:
  static const size_t ring_size = a_ring_size;
  static const size_t real_source_idx = ring_size / 2;

  bool init()
  {
    using namespace cryptonote;

    std::vector<tx_source_entry::output_entry> output_entries;
    for (size_t i = 0; i < ring_size; ++i)
    {
      m_miners[i].generate();

      std::vector<size_t> block_sizes;
      if (!construct_miner_tx(0, 0, m_miners[i].get_keys().m_account_address, m_miner_txs[i], 0, block_sizes, 2))
        return false;

      txout_to_key tx_out = boost::get<txout_to_key>(m_miner_txs[i].vout[0].target);
      output_entries.push_back(std::make_pair(i, tx_out.key));
      m_public_keys[i] = tx_out.key;
      m_public_key_ptrs[i] = &m_public_keys[i];
    }

    m_source_amount = m_miner_txs[0].vout[0].amount;

    tx_source_entry source_entry;
    source_entry.amount = m_source_amount;
    source_entry.real_out_tx_key = get_tx_pub_key_from_extra(m_miner_txs[real_source_idx]);
    source_entry.real_output_in_tx_index = 0;
    source_entry.outputs.swap(output_entries);
    source_entry.real_output = real_source_idx;

    m_sources.push_back(source_entry);

    return true;
  }

protected:
  cryptonote::account_base m_miners[ring_size];
  cryptonote::transaction m_miner_txs[ring_size];
  uint64_t m_source_amount;

  std::vector<cryptonote::tx_source_entry> m_sources;
  crypto::public_key m_public_keys[ring_size];
  const crypto::public_key* m_public_key_ptrs[ring_size];
};
