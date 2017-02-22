// Copyright (c) 2014-2017, The Monero Project
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

struct get_tx_validation_base : public test_chain_unit_base
{
  get_tx_validation_base()
    : m_invalid_tx_index(0)
    , m_invalid_block_index(0)
  {
    REGISTER_CALLBACK_METHOD(get_tx_validation_base, mark_invalid_tx);
    REGISTER_CALLBACK_METHOD(get_tx_validation_base, mark_invalid_block);
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

private:
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};

struct gen_tx_big_version : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_unlock_time : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_input_is_not_txin_to_key : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_no_inputs_no_outputs : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_no_inputs_has_outputs : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_has_inputs_no_outputs : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_invalid_input_amount : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_input_wo_key_offsets : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_offest_points_to_foreign_key : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_sender_key_offest_not_exist : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_mixed_key_offest_not_exist : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_image_not_derive_from_tx_key : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_image_is_invalid : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_check_input_unlock_time : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_txout_to_key_has_invalid_key : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_output_with_zero_amount : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_output_is_not_txout_to_key : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_signatures_are_invalid : public get_tx_validation_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
