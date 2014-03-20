// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_set>
#include <atomic>
#include "net/net_utils_base.h"
#include "copyable_atomic.h"

namespace cryptonote
{

  struct cryptonote_connection_context: public epee::net_utils::connection_context_base
  {

    enum state
    {
      state_befor_handshake = 0, //default state
      state_synchronizing,
      state_idle,
      state_normal
    };

    state m_state;
    std::list<crypto::hash> m_needed_objects;
    std::unordered_set<crypto::hash> m_requested_objects;
    uint64_t m_remote_blockchain_height;
    uint64_t m_last_response_height;
    epee::copyable_atomic m_callback_request_count; //in debug purpose: problem with double callback rise
    //size_t m_score;  TODO: add score calculations
  };

  inline std::string get_protocol_state_string(cryptonote_connection_context::state s)
  {
    switch (s)
    {
    case cryptonote_connection_context::state_befor_handshake:
      return "state_befor_handshake";
    case cryptonote_connection_context::state_synchronizing:
      return "state_synchronizing";
    case cryptonote_connection_context::state_idle:
      return "state_idle";
    case cryptonote_connection_context::state_normal:
      return "state_normal";
    default:
      return "unknown";
    }    
  }

}
