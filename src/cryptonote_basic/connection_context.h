// Copyright (c) 2014-2024, The Monero Project
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
// Parts of this file are originally copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net

#pragma once
#include <unordered_set>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional/optional_fwd.hpp>
#include "crypto/hash.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net/net_utils_base.h"
#include "p2p/p2p_protocol_defs.h"
#include "syncobj.h"

namespace cryptonote
{
  //! \return Maximum number of bytes permissible for `command`.
  inline std::size_t get_command_max_bytes(const int command) noexcept
  {
    switch (command)
    {
    case nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_PING::ID:
      return 4096;
    case nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::ID:
      return 4096;
    case cryptonote::NOTIFY_NEW_BLOCK::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_NEW_TRANSACTIONS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_GET_OBJECTS::ID:
      return 1024 * 1024 * 2; // 2 MB
    case cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_CHAIN::ID:
      return 512 * 1024; // 512 kB
    case cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::ID:
      return 1024 * 1024 * 4; // 4 MB
    case cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though, fluffy blocks can be full)
    case cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::ID:
      return 1024 * 1024; // 1 MB
    case cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::ID:
      return 1024 * 1024 * 4; // 4 MB
    case cryptonote::NOTIFY_TX_POOL_HASH::ID:
      return 1024 * 1024 * 2; // 2 MB
    case cryptonote::NOTIFY_REQUEST_TX_POOL_TXS::ID:
      return 1024 * 1024 * 2; // 2 MB
    default:
      break;
    };
    return std::numeric_limits<size_t>::max();
  }

  struct cryptonote_connection_context: public epee::net_utils::connection_context_base
  {
    cryptonote_connection_context(): m_state(state_before_handshake), m_remote_blockchain_height(0), m_last_response_height(0),
        m_expected_heights_start(0), m_last_request_time(boost::date_time::not_a_date_time), m_callback_request_count(0),
        m_last_known_hash(crypto::null_hash), m_pruning_seed(0), m_rpc_port(0), m_rpc_credits_per_hash(0), m_anchor(false), m_score(0),
        m_expect_response(0), m_expect_height(0), m_num_requested(0) {}

    enum state
    {
      state_before_handshake = 0, //default state
      state_synchronizing,
      state_standby,
      state_normal
    };

    /*
      This class was originally from the EPEE module. It is identical in function to std::atomic<uint32_t> except
      that it has copy-construction and copy-assignment defined, which means that earliers devs didn't have to write
      custom copy-contructors and copy-assingment operators for the outer class, cryptonote_connection_context.
      cryptonote_connection_context should probably be refactored because it is both trying to be POD-like while
      also (very loosely) controlling access to its atomic members.
    */
    class copyable_atomic: public std::atomic<uint32_t>
    {
    public:
      copyable_atomic()
      {};
      copyable_atomic(uint32_t value)
      { store(value); }
      copyable_atomic(const copyable_atomic& a):std::atomic<uint32_t>(a.load())
      {}
      copyable_atomic& operator= (const copyable_atomic& a)
      {
        store(a.load());
        return *this;
      }
      uint32_t operator++()
      {
        return std::atomic<uint32_t>::operator++();
      }
      uint32_t operator++(int fake)
      {
        return std::atomic<uint32_t>::operator++(fake);
      }
    };

    static constexpr int handshake_command() noexcept { return 1001; }
    bool handshake_complete() const noexcept { return m_state != state_before_handshake; }

    //! Use this instead of `m_state = state_normal`.
    void set_state_normal();

    boost::optional<crypto::hash> get_expected_hash(uint64_t height) const;

    state m_state;
    std::vector<std::pair<crypto::hash, uint64_t>> m_needed_objects;
    std::vector<crypto::hash> m_expected_heights;
    std::unordered_set<crypto::hash> m_requested_objects;
    uint64_t m_remote_blockchain_height;
    uint64_t m_last_response_height;
    uint64_t m_expected_heights_start;
    boost::posix_time::ptime m_last_request_time;
    copyable_atomic m_callback_request_count; //in debug purpose: problem with double callback rise
    crypto::hash m_last_known_hash;
    uint32_t m_pruning_seed;
    uint16_t m_rpc_port;
    uint32_t m_rpc_credits_per_hash;
    bool m_anchor;
    int32_t m_score;
    int m_expect_response;
    uint64_t m_expect_height;
    size_t m_num_requested;
    copyable_atomic m_new_stripe_notification{0};
    copyable_atomic m_idle_peer_notification{0};
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
    case cryptonote_connection_context::state_normal:
      return 'n';
    default:
      return 'u';
    }
  }

}
