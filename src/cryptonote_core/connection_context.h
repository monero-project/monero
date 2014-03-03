// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_set>
#include <atomic>
#include "net/net_utils_base.h"


namespace cryptonote
{
  class my_atomic: public std::atomic<uint32_t>
  {
  public:
    my_atomic()
    {};
    my_atomic(const my_atomic& a):std::atomic<uint32_t>(a.load())
    {}
    my_atomic& operator= (const my_atomic& a)
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


  struct cryptonote_connection_context: public epee::net_utils::connection_context_base
  {

    enum state
    {
      state_befor_handshake = 0, //default state
      state_synchronizing,
      state_normal
    };

    state m_state;
    std::list<crypto::hash> m_needed_objects;
    std::unordered_set<crypto::hash> m_requested_objects;
    uint64_t m_remote_blockchain_height;
    uint64_t m_last_response_height;
    my_atomic m_callback_request_count; //in debug purpose: problem with double callback rise
    //size_t m_score;  TODO: add score calculations
  };
}
