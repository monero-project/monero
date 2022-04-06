// Copyright (c) 2014-2022, The Monero Project
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

const size_t invalid_index_value = std::numeric_limits<size_t>::max();
const uint64_t FIRST_BLOCK_REWARD = 17592186044415;


template<class concrete_test>
class gen_double_spend_base : public test_chain_unit_base
{
public:
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;

  gen_double_spend_base();

  bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& tx);
  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& block);

  bool mark_last_valid_block(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool mark_invalid_tx(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool mark_invalid_block(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_double_spend(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  cryptonote::block m_last_valid_block;
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};


template<bool txs_keeped_by_block>
struct gen_double_spend_in_tx : public gen_double_spend_base< gen_double_spend_in_tx<txs_keeped_by_block> >
{
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const bool has_invalid_tx = true;
  static const size_t expected_pool_txs_count = 0;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_keeped_by_block>
struct gen_double_spend_in_the_same_block : public gen_double_spend_base< gen_double_spend_in_the_same_block<txs_keeped_by_block> >
{
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const bool has_invalid_tx = !txs_keeped_by_block;
  static const size_t expected_pool_txs_count = has_invalid_tx ? 1 : 2;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_keeped_by_block>
struct gen_double_spend_in_different_blocks : public gen_double_spend_base< gen_double_spend_in_different_blocks<txs_keeped_by_block> >
{
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const bool has_invalid_tx = !txs_keeped_by_block;
  static const size_t expected_pool_txs_count = has_invalid_tx ? 0 : 1;
  static const uint64_t expected_bob_balance = 0;
  static const uint64_t expected_alice_balance = send_amount - TESTS_DEFAULT_FEE;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_keeped_by_block>
struct gen_double_spend_in_alt_chain_in_the_same_block : public gen_double_spend_base< gen_double_spend_in_alt_chain_in_the_same_block<txs_keeped_by_block> >
{
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const bool has_invalid_tx = !txs_keeped_by_block;
  static const size_t expected_pool_txs_count = has_invalid_tx ? 1 : 2;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_keeped_by_block>
struct gen_double_spend_in_alt_chain_in_different_blocks : public gen_double_spend_base< gen_double_spend_in_alt_chain_in_different_blocks<txs_keeped_by_block> >
{
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const bool has_invalid_tx = !txs_keeped_by_block;
  static const size_t expected_pool_txs_count = has_invalid_tx ? 1 : 2;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


class gen_double_spend_in_different_chains : public test_chain_unit_base
{
public:
  static const uint64_t send_amount = FIRST_BLOCK_REWARD - TESTS_DEFAULT_FEE;
  static const size_t expected_blockchain_height = 4 + 2 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;

  gen_double_spend_in_different_chains();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_double_spend(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


#define INIT_DOUBLE_SPEND_TEST()                                           \
  uint64_t ts_start = 1338224400;                                          \
  GENERATE_ACCOUNT(miner_account);                                         \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);              \
  MAKE_ACCOUNT(events, bob_account);                                       \
  MAKE_ACCOUNT(events, alice_account);                                     \
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);                     \
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0);   \
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_0);         \
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);


#include "double_spend.inl"
