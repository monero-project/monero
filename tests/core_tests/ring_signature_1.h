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

/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_ring_signature_1 : public test_chain_unit_base
{
public:
  gen_ring_signature_1();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  cryptonote::account_base m_bob_account;
  cryptonote::account_base m_alice_account;
};


/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_ring_signature_2 : public test_chain_unit_base
{
public:
  gen_ring_signature_2();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  cryptonote::account_base m_bob_account;
  cryptonote::account_base m_alice_account;
};


/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_ring_signature_big : public test_chain_unit_base
{
public:
  gen_ring_signature_big();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  size_t m_test_size;
  uint64_t m_tx_amount;

  cryptonote::account_base m_bob_account;
  cryptonote::account_base m_alice_account;
};
