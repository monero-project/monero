// Copyright (c) 2014-2017, The Monero Project
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

#pragma once

#include <vector>

#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
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

      if (!construct_miner_tx(0, 0, 0, 2, 0, m_miners[i].get_keys().m_account_address, m_miner_txs[i]))
        return false;

      txout_to_key tx_out = boost::get<txout_to_key>(m_miner_txs[i].vout[0].target);
      output_entries.push_back(std::make_pair(i, rct::ctkey({rct::pk2rct(tx_out.key), rct::zeroCommit(m_miner_txs[i].vout[0].amount)})));
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
    source_entry.mask = rct::identity();
    source_entry.rct = false;

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
