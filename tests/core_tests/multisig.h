// Copyright (c) 2017-2022, The Monero Project
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

struct gen_multisig_tx_validation_base : public test_chain_unit_base
{
  gen_multisig_tx_validation_base()
    : m_invalid_tx_index(0)
    , m_invalid_block_index(0)
  {
    REGISTER_CALLBACK_METHOD(gen_multisig_tx_validation_base, mark_invalid_tx);
    REGISTER_CALLBACK_METHOD(gen_multisig_tx_validation_base, mark_invalid_block);
  }

  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
      return tvc.m_verifivation_failed;
    else
      return !tvc.m_verifivation_failed && tx_added;
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

  bool generate_with(std::vector<test_event_entry>& events, size_t inputs, size_t mixin,
      uint64_t amount_paid, bool valid,
      size_t threshold, size_t total, size_t creator, std::vector<size_t> other_signers,
      const std::function<void(std::vector<cryptonote::tx_source_entry> &sources, std::vector<cryptonote::tx_destination_entry> &destinations)> &pre_tx,
      const std::function<void(cryptonote::transaction &tx)> &post_tx) const;

private:
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};

template<>
struct get_test_options<gen_multisig_tx_validation_base> {
  const std::pair<uint8_t, uint64_t> hard_forks[3] = {std::make_pair(1, 0), std::make_pair(HF_VERSION_BULLETPROOF_PLUS, 1), std::make_pair(0, 0)};
  const cryptonote::test_options test_options = {
    hard_forks, 0
  };
};

// valid
struct gen_multisig_tx_valid_22_1_2: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_22_1_2>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_22_1_2_many_inputs: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_22_1_2_many_inputs>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_22_2_1: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_22_2_1>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_33_1_23: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_33_1_23>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_33_3_21: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_33_3_21>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_23_1_2: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_23_1_2>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_23_1_3: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_23_1_3>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_23_2_1: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_23_2_1>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_23_2_3: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_23_2_3>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_45_1_234: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_45_1_234>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_45_4_135_many_inputs: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_45_4_135_many_inputs>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_89_3_1245789: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_89_3_1245789>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_24_1_2: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_24_1_2>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_24_1_2_many_inputs: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_24_1_2_many_inputs>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_25_1_2: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_25_1_2>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_25_1_2_many_inputs: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_25_1_2_many_inputs>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_48_1_234: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_48_1_234>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_valid_48_1_234_many_inputs: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_valid_48_1_234_many_inputs>: public get_test_options<gen_multisig_tx_validation_base> {};

// invalid
struct gen_multisig_tx_invalid_22_1__no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_22_1__no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_33_1__no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_33_1__no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_33_1_2_no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_33_1_2_no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_33_1_3_no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_33_1_3_no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_23_1__no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_23_1__no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_45_5_23_no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_45_5_23_no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_24_1_no_signers: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_24_1_no_signers>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_25_1_no_signers: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_25_1_no_signers>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_48_1_no_signers: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_48_1_no_signers>: public get_test_options<gen_multisig_tx_validation_base> {};

struct gen_multisig_tx_invalid_48_1_23_no_threshold: public gen_multisig_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_multisig_tx_invalid_48_1_23_no_threshold>: public get_test_options<gen_multisig_tx_validation_base> {};
