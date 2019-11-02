// Copyright (c) 2019, The Monero Project
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

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "chaingen.h"
#include "crypto/crypto.h"

enum class relay_test
{
  no_change = 0, //!< No expected changes to the txpool
  broadcasted,   //!< A new block or fluff/flood tx is expected in txpool
  hidden,        //!< A new stem or local tx is expected in txpool
  no_relay       //!< A new no relay is expected in txpool
};

class txpool_base : public test_chain_unit_base
{
  size_t m_broadcasted_tx_count;
  size_t m_all_tx_count;

public:
  txpool_base();

  bool increase_broadcasted_tx_count(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
  bool increase_all_tx_count(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
  bool check_txpool_spent_keys(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events);
};

struct txpool_spend_key_public : txpool_base
{
  txpool_spend_key_public() : txpool_base()
  {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct txpool_spend_key_all : txpool_base
{
  txpool_spend_key_all() : txpool_base()
  {}

  bool generate(std::vector<test_event_entry>& events);
};

class txpool_double_spend_base : public txpool_base
{
  std::unordered_set<crypto::hash> m_broadcasted_hashes;
  std::unordered_set<crypto::hash> m_no_relay_hashes;
  std::unordered_map<crypto::hash, uint64_t> m_all_hashes;
  size_t m_no_new_index;
  size_t m_new_timestamp_index;
  crypto::hash m_last_tx;

  bool check_changed(cryptonote::core& c, size_t ev_index, relay_test condition);

public:
  txpool_double_spend_base();

  bool mark_no_new(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool mark_timestamp_change(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  //! Pause for 1 second, so that `receive_time` for tx meta changes (tx hidden from public rpc being updated)
  bool timestamp_change_pause(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  bool check_unchanged(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_new_broadcasted(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_new_hidden(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_new_no_relay(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& /*tx*/);
};

struct txpool_double_spend_norelay : txpool_double_spend_base
{
  txpool_double_spend_norelay()
    : txpool_double_spend_base()
  {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct txpool_double_spend_local : txpool_double_spend_base
{
  txpool_double_spend_local()
    : txpool_double_spend_base()
  {}

  bool generate(std::vector<test_event_entry>& events) const;
};
