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

struct gen_rct_tx_validation_base : public test_chain_unit_base
{
  gen_rct_tx_validation_base()
    : m_invalid_tx_index(0)
    , m_invalid_block_index(0)
  {
    REGISTER_CALLBACK_METHOD(gen_rct_tx_validation_base, mark_invalid_tx);
    REGISTER_CALLBACK_METHOD(gen_rct_tx_validation_base, mark_invalid_block);
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

  bool generate_with(std::vector<test_event_entry>& events, const int *out_idx, int mixin,
      uint64_t amount_paid, bool valid,
      const std::function<void(std::vector<cryptonote::tx_source_entry> &sources, std::vector<cryptonote::tx_destination_entry> &destinations)> &pre_tx,
      const std::function<void(cryptonote::transaction &tx)> &post_tx) const;

private:
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};

template<>
struct get_test_options<gen_rct_tx_validation_base> {
  const std::vector<std::pair<uint8_t, uint64_t>> hard_forks = {std::make_pair(1, 0), std::make_pair(2, 1), std::make_pair(4, 65)};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};

// valid
struct gen_rct_tx_valid_from_pre_rct : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_valid_from_pre_rct>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_valid_from_rct : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_valid_from_rct>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_valid_from_mixed : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_valid_from_mixed>: public get_test_options<gen_rct_tx_validation_base> {};

// altered commitment/dest
struct gen_rct_tx_pre_rct_bad_real_dest : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_bad_real_dest>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_bad_real_mask : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_bad_real_mask>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_bad_fake_dest : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_bad_fake_dest>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_bad_fake_mask : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_bad_fake_mask>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_bad_real_dest : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_bad_real_dest>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_bad_real_mask : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_bad_real_mask>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_bad_fake_dest : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_bad_fake_dest>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_bad_fake_mask : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_bad_fake_mask>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_spend_with_zero_commit : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_spend_with_zero_commit>: public get_test_options<gen_rct_tx_validation_base> {};

// altered amounts
struct gen_rct_tx_pre_rct_zero_vin_amount : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_zero_vin_amount>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_non_zero_vin_amount : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_non_zero_vin_amount>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_non_zero_vout_amount : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_non_zero_vout_amount>: public get_test_options<gen_rct_tx_validation_base> {};

// key image
struct gen_rct_tx_pre_rct_duplicate_key_image : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_duplicate_key_image>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_duplicate_key_image : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_duplicate_key_image>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_wrong_key_image : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_wrong_key_image>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_wrong_key_image : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_wrong_key_image>: public get_test_options<gen_rct_tx_validation_base> {};

// fee
struct gen_rct_tx_pre_rct_wrong_fee : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_wrong_fee>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_wrong_fee : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_wrong_fee>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_increase_vin_and_fee : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_increase_vin_and_fee>: public get_test_options<gen_rct_tx_validation_base> {};

// modify vin/vout
struct gen_rct_tx_pre_rct_remove_vin : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_remove_vin>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_remove_vin : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_remove_vin>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_pre_rct_add_vout : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_add_vout>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_add_vout : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_add_vout>: public get_test_options<gen_rct_tx_validation_base> {};

// extra
struct gen_rct_tx_pre_rct_altered_extra : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_pre_rct_altered_extra>: public get_test_options<gen_rct_tx_validation_base> {};

struct gen_rct_tx_rct_altered_extra : public gen_rct_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
template<> struct get_test_options<gen_rct_tx_rct_altered_extra>: public get_test_options<gen_rct_tx_validation_base> {};

