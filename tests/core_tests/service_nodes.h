// Copyright (c) 2014-2018, The Monero Project
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
#include "chaingen.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_service_nodes : public test_chain_unit_base
{
public:
  gen_service_nodes();
  bool generate(std::vector<test_event_entry> &events) const;
  bool check_registered(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool check_expired(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
  cryptonote::keypair m_alice_service_node_keys;
};

template<>
struct get_test_options<gen_service_nodes> {
  const std::pair<uint8_t, uint64_t> hard_forks[3] = {std::make_pair(7, 0), std::make_pair(8, 1), std::make_pair(9, 2)};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};

class test_prefer_deregisters : public test_chain_unit_base
{
public:
  test_prefer_deregisters();
  bool generate(std::vector<test_event_entry> &events);
  bool check_prefer_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};

template<>
struct get_test_options<test_prefer_deregisters> {
  const std::pair<uint8_t, uint64_t> hard_forks[3] = {std::make_pair(7, 0), std::make_pair(8, 1), std::make_pair(9, 2)};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};

class test_zero_fee_deregister : public test_chain_unit_base
{
  size_t m_invalid_tx_index;

public:
  test_zero_fee_deregister();
  bool generate(std::vector<test_event_entry> &events);

  bool mark_invalid_tx(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_tx_index = ev_index + 1;
    return true;
  }
  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc,
                                     bool tx_added,
                                     size_t event_idx,
                                     const cryptonote::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
      return tvc.m_verifivation_failed;
    else
      return !tvc.m_verifivation_failed && tx_added;
  }
};

template<>
struct get_test_options<test_zero_fee_deregister> {
  const std::pair<uint8_t, uint64_t> hard_forks[3] = {std::make_pair(7, 0), std::make_pair(8, 1), std::make_pair(9, 2)};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};

class test_deregister_safety_buffer : public test_chain_unit_base
{
  size_t m_invalid_tx_index;

public:

  test_deregister_safety_buffer();
  bool generate(std::vector<test_event_entry>& events);

  bool mark_invalid_tx(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_tx_index = ev_index + 1;
    return true;
  }
  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc,
                                     bool tx_added,
                                     size_t event_idx,
                                     const cryptonote::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
      return tvc.m_verifivation_failed;
    else
      return !tvc.m_verifivation_failed && tx_added;
  }
};

template<>
struct get_test_options<test_deregister_safety_buffer> {
  const std::pair<uint8_t, uint64_t> hard_forks[3] = {std::make_pair(7, 0), std::make_pair(8, 1), std::make_pair(9, 2)};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};