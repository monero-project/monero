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

#pragma once
#include "chaingen.h"

struct gen_fcmp_pp_tx_validation_base : public test_chain_unit_base
{
  gen_fcmp_pp_tx_validation_base()
    : m_invalid_tx_index(0)
    , m_invalid_block_index(0)
  {
    REGISTER_CALLBACK_METHOD(gen_fcmp_pp_tx_validation_base, mark_invalid_tx);
    REGISTER_CALLBACK_METHOD(gen_fcmp_pp_tx_validation_base, mark_invalid_block);
  }

  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
      return tvc.m_verifivation_failed;
    else
      return !tvc.m_verifivation_failed && tx_added;
  }

  bool check_tx_verification_context_array(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t event_idx, const std::vector<cryptonote::transaction>& /*txs*/)
  {
    size_t failed = 0;
    for (const cryptonote::tx_verification_context &tvc: tvcs)
      if (tvc.m_verifivation_failed)
        ++failed;
    if (m_invalid_tx_index == event_idx)
      return failed > 0;
    else
      return failed == 0 && tx_added == tvcs.size();
  }

  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*block*/)
  {
    if (m_invalid_block_index == event_idx)
      return bvc.m_verifivation_failed;
    else
      return !bvc.m_verifivation_failed;
  }

  bool mark_invalid_block(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_block_index = ev_index + 1;
    return true;
  }

  bool mark_invalid_tx(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_tx_index = ev_index + 1;
    return true;
  }

  bool generate_with(std::vector<test_event_entry>& events,
      size_t n_txes, const uint64_t *amounts_paid, bool valid, const rct::RCTConfig &rct_config, uint8_t hf_version,
      const std::function<bool(std::vector<cryptonote::tx_source_entry> &sources, std::vector<cryptonote::tx_destination_entry> &destinations, size_t)> &pre_tx,
      const std::function<bool(cryptonote::transaction &tx, size_t)> &post_tx) const;

  bool check_fcmp_pp(const cryptonote::transaction &tx, size_t tx_idx, const size_t *sizes, const char *context) const;

private:
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};

template<>
struct get_test_options<gen_fcmp_pp_tx_validation_base> {
  const std::pair<uint8_t, uint64_t> hard_forks[4] = {std::make_pair(1, 0), std::make_pair(2, 1), std::make_pair(HF_VERSION_FCMP_PLUS_PLUS, 73), std::make_pair(0, 0)};
  const cryptonote::test_options test_options = {
    hard_forks, 0
  };
};

template<uint8_t test_version = 1>
struct get_fcmp_pp_versioned_test_options: public get_test_options<gen_fcmp_pp_tx_validation_base> {
  const std::pair<uint8_t, uint64_t> hard_forks[4] = {std::make_pair(1, 0), std::make_pair(2, 1), std::make_pair(test_version, 73), std::make_pair(0, 0)};
  const cryptonote::test_options test_options = {
    hard_forks, 0
  };
};

struct gen_fcmp_pp_tx_valid_at_fork : public gen_fcmp_pp_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_fcmp_pp_tx_valid_at_fork>: public get_fcmp_pp_versioned_test_options<HF_VERSION_FCMP_PLUS_PLUS> {};
