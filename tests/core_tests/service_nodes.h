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

class test_service_nodes_base : public test_chain_unit_base {};

template<>
struct get_test_options<test_service_nodes_base>
{
  const std::vector<std::pair<uint8_t, uint64_t>> hard_forks = { std::make_pair(7, 0),
                                                                 std::make_pair(8, 1),
                                                                 std::make_pair(9, 2) };
  const cryptonote::test_options test_options = { hard_forks };
};

class gen_service_nodes : public test_service_nodes_base
{
public:
  gen_service_nodes();
  bool generate(std::vector<test_event_entry> &events) const;
  bool check_registered(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool check_expired(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
  cryptonote::keypair m_alice_service_node_keys;
};

template<> struct get_test_options<gen_service_nodes>: public get_test_options<test_service_nodes_base> {};

class test_prefer_deregisters : public test_service_nodes_base
{
public:
  test_prefer_deregisters();
  bool generate(std::vector<test_event_entry> &events);
  bool check_prefer_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};
template<> struct get_test_options<test_prefer_deregisters>: public get_test_options<test_service_nodes_base> {};

class test_zero_fee_deregister : public test_service_nodes_base
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

template<> struct get_test_options<test_zero_fee_deregister>: public get_test_options<test_service_nodes_base> {};

class test_deregister_safety_buffer : public test_service_nodes_base
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

template<> struct get_test_options<test_deregister_safety_buffer>: public get_test_options<test_service_nodes_base> {};

class test_deregisters_on_split : public test_service_nodes_base
{

  size_t m_invalid_tx_index;

public:
  test_deregisters_on_split();
  bool generate(std::vector<test_event_entry>& events);
  bool test_on_split(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

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

template<> struct get_test_options<test_deregisters_on_split>: public get_test_options<test_service_nodes_base> {};

class deregister_too_old : public test_chain_unit_base
{
  size_t m_invalid_block_index;

public:
  deregister_too_old();
  bool generate(std::vector<test_event_entry>& events);

  bool mark_invalid_block(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_block_index = ev_index + 1;
    return true;
  }

  bool check_block_verification_context(const cryptonote::block_verification_context& bvc,
                                        size_t event_idx,
                                        const cryptonote::block& /*blk*/)
  {
    if (m_invalid_block_index == event_idx)
      return bvc.m_verifivation_failed;
    else
      return !bvc.m_verifivation_failed;
  }
};


template<> struct get_test_options<deregister_too_old>: public get_test_options<test_service_nodes_base> {};
//-------------------------------------------------------------------------------------------
class sn_test_rollback : public test_chain_unit_base
{

public:
  sn_test_rollback();
  bool generate(std::vector<test_event_entry>& events);
  bool test_registrations(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};

template<> struct get_test_options<sn_test_rollback>: public get_test_options<test_service_nodes_base> {};

//-------------------------------------------------------------------------------------------

class test_swarms_basic : public test_service_nodes_base
{
public:
  test_swarms_basic();
  bool generate(std::vector<test_event_entry>& events);

  bool test_initial_swarms(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool test_with_more_sn(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool test_after_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};

template<>
struct get_test_options<test_swarms_basic>
{
  const std::vector<std::pair<uint8_t, uint64_t>> hard_forks = { std::make_pair(7, 0),
                                                                 std::make_pair(8, 1),
                                                                 std::make_pair(9, 2),
                                                                 std::make_pair(10, 150) };
  const cryptonote::test_options test_options = { hard_forks };
};