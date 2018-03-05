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

#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

#include "single_tx_test_base.h"

class test_is_out_to_acc : public single_tx_test_base
{
public:
  static const size_t loop_count = 1000;

  bool test()
  {
    const cryptonote::txout_to_key& tx_out = boost::get<cryptonote::txout_to_key>(m_tx.vout[0].target);
    return cryptonote::is_out_to_acc(m_bob.get_keys(), tx_out, m_tx_pub_key, m_additional_tx_pub_keys, 0);
  }
};

class test_is_out_to_acc_precomp : public single_tx_test_base
{
public:
  static const size_t loop_count = 1000;

  bool init()
  {
    if (!single_tx_test_base::init())
      return false;
    crypto::generate_key_derivation(m_tx_pub_key, m_bob.get_keys().m_view_secret_key, m_derivation);
    return true;
  }
  bool test()
  {
    const cryptonote::txout_to_key& tx_out = boost::get<cryptonote::txout_to_key>(m_tx.vout[0].target);
    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[m_bob.get_keys().m_account_address.m_spend_public_key] = {0,0};
    std::vector<crypto::key_derivation> additional_derivations;
    boost::optional<cryptonote::subaddress_receive_info> info = cryptonote::is_out_to_acc_precomp(subaddresses, tx_out.key, m_derivation, additional_derivations, 0, hw::get_device("default"));
    return (bool)info;
  }

private:
  crypto::key_derivation m_derivation;
};
