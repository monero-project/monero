// Copyright (c) 2014-2020, The Monero Project
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
#include <unordered_set>
#include <atomic>
#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "net/net_utils_base.h"
#include "copyable_atomic.h"
#include "crypto/hash.h"

namespace cryptonote
{

  struct cryptonote_connection_context: public epee::net_utils::connection_context_base
  {
    cryptonote_connection_context(): m_state(state_before_handshake), m_remote_blockchain_height(0), m_last_response_height(0),
        m_last_request_time(boost::date_time::not_a_date_time), m_callback_request_count(0),
        m_last_known_hash(crypto::null_hash), m_pruning_seed(0), m_rpc_port(0), m_rpc_credits_per_hash(0), m_anchor(false), m_score(0),
        m_expect_response(0), m_expect_height(0), m_num_requested(0) {}

    enum state
    {
      state_before_handshake = 0, //default state
      state_synchronizing,
      state_standby,
      state_idle,
      state_normal
    };

    bool handshake_complete() const noexcept { return m_state != state_before_handshake; }

    void set_max_bytes(int command, size_t bytes) {
      const auto i = std::lower_bound(m_max_bytes.begin(), m_max_bytes.end(), std::make_pair(command, bytes), [](const std::pair<int, size_t> &e0, const std::pair<int, size_t> &e1){
        return e0.first < e1.first;
      });
      if (i == m_max_bytes.end())
        m_max_bytes.push_back(std::make_pair(command, bytes));
      else if (i->first == command)
        i->second = bytes;
      else
        m_max_bytes.insert(i, std::make_pair(command, bytes));
    }
    size_t get_max_bytes(int command) const {
      const auto i = std::lower_bound(m_max_bytes.begin(), m_max_bytes.end(), std::make_pair(command, 0), [](const std::pair<int, size_t> &e0, const std::pair<int, size_t> &e1){
        return e0.first < e1.first;
      });
      if (i == m_max_bytes.end() || i->first != command)
        return std::numeric_limits<size_t>::max();
      else
        return i->second;
    }

    state m_state;
    std::vector<std::pair<crypto::hash, uint64_t>> m_needed_objects;
    std::unordered_set<crypto::hash> m_requested_objects;
    uint64_t m_remote_blockchain_height;
    uint64_t m_last_response_height;
    boost::posix_time::ptime m_last_request_time;
    epee::copyable_atomic m_callback_request_count; //in debug purpose: problem with double callback rise
    crypto::hash m_last_known_hash;
    uint32_t m_pruning_seed;
    uint16_t m_rpc_port;
    uint32_t m_rpc_credits_per_hash;
    bool m_anchor;
    int32_t m_score;
    int m_expect_response;
    uint64_t m_expect_height;
    size_t m_num_requested;
    std::vector<std::pair<int, size_t>> m_max_bytes;
  };

  inline std::string get_protocol_state_string(cryptonote_connection_context::state s)
  {
    switch (s)
    {
    case cryptonote_connection_context::state_before_handshake:
      return "before_handshake";
    case cryptonote_connection_context::state_synchronizing:
      return "synchronizing";
    case cryptonote_connection_context::state_standby:
      return "standby";
    case cryptonote_connection_context::state_idle:
      return "idle";
    case cryptonote_connection_context::state_normal:
      return "normal";
    default:
      return "unknown";
    }    
  }

  inline char get_protocol_state_char(cryptonote_connection_context::state s)
  {
    switch (s)
    {
    case cryptonote_connection_context::state_before_handshake:
      return 'h';
    case cryptonote_connection_context::state_synchronizing:
      return 's';
    case cryptonote_connection_context::state_standby:
      return 'w';
    case cryptonote_connection_context::state_idle:
      return 'i';
    case cryptonote_connection_context::state_normal:
      return 'n';
    default:
      return 'u';
    }
  }

}
