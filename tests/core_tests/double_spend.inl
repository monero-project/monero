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

//======================================================================================================================

template<class concrete_test>
gen_double_spend_base<concrete_test>::gen_double_spend_base()
  : m_invalid_tx_index(invalid_index_value)
  , m_invalid_block_index(invalid_index_value)
{
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, mark_last_valid_block);
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, mark_invalid_tx);
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, mark_invalid_block);
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, check_double_spend);
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& /*tx*/)
{
  if (m_invalid_tx_index == event_idx)
    return tvc.m_verifivation_failed;
  else
    return !tvc.m_verifivation_failed && tx_added;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*block*/)
{
  if (m_invalid_block_index == event_idx)
    return bvc.m_verifivation_failed;
  else
    return !bvc.m_verifivation_failed;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::mark_last_valid_block(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  std::vector<cryptonote::block> block_list;
  bool r = c.get_blocks(c.get_current_blockchain_height() - 1, 1, block_list);
  CHECK_AND_ASSERT_MES(r, false, "core::get_blocks failed");
  m_last_valid_block = block_list.back();
  return true;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::mark_invalid_tx(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_invalid_tx_index = ev_index + 1;
  return true;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::mark_invalid_block(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_invalid_block_index = ev_index + 1;
  return true;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::check_double_spend(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_double_spend_base::check_double_spend");

  if (concrete_test::has_invalid_tx)
  {
    CHECK_NOT_EQ(invalid_index_value, m_invalid_tx_index);
  }
  CHECK_NOT_EQ(invalid_index_value, m_invalid_block_index);

  std::vector<cryptonote::block> block_list;
  bool r = c.get_blocks(0, 100 + 2 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, block_list);
  CHECK_TEST_CONDITION(r);
  CHECK_TEST_CONDITION(m_last_valid_block == block_list.back());

  CHECK_EQ(concrete_test::expected_pool_txs_count, c.get_pool_transactions_count());

  cryptonote::account_base bob_account = boost::get<cryptonote::account_base>(events[1]);
  cryptonote::account_base alice_account = boost::get<cryptonote::account_base>(events[2]);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  std::vector<cryptonote::block> blocks(block_list.begin(), block_list.end());
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(concrete_test::expected_bob_balance, get_balance(bob_account, blocks, mtx));
  CHECK_EQ(concrete_test::expected_alice_balance, get_balance(alice_account, blocks, mtx));

  return true;
}

//======================================================================================================================

template<bool txs_keeped_by_block>
bool gen_double_spend_in_tx<txs_keeped_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();
  DO_CALLBACK(events, "mark_last_valid_block");

  std::vector<cryptonote::tx_source_entry> sources;
  cryptonote::tx_source_entry se;
  se.amount = tx_0.vout[0].amount;
  se.push_output(0, boost::get<cryptonote::txout_to_key>(tx_0.vout[0].target).key, se.amount);
  se.real_output = 0;
  se.rct = false;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_0);
  se.real_output_in_tx_index = 0;
  sources.push_back(se);
  // Double spend!
  sources.push_back(se);

  cryptonote::tx_destination_entry de;
  de.addr = alice_account.get_keys().m_account_address;
  de.amount = 2 * se.amount - TESTS_DEFAULT_FEE;
  std::vector<cryptonote::tx_destination_entry> destinations;
  destinations.push_back(de);

  cryptonote::transaction tx_1;
  if (!construct_tx(bob_account.get_keys(), sources, destinations, boost::none, std::vector<uint8_t>(), tx_1, 0))
    return false;

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, txs_keeped_by_block);
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_1);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_account, tx_1);
  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_keeped_by_block>
bool gen_double_spend_in_the_same_block<txs_keeped_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  DO_CALLBACK(events, "mark_last_valid_block");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, txs_keeped_by_block);

  MAKE_TX_LIST_START(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  cryptonote::transaction tx_1 = txs_1.front();
  auto tx_1_idx = events.size() - 1;
  // Remove tx_1, it is being inserted back a little later
  events.pop_back();

  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  MAKE_TX_LIST(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.insert(events.begin() + tx_1_idx, tx_1);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_1);
  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_keeped_by_block>
bool gen_double_spend_in_different_blocks<txs_keeped_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  DO_CALLBACK(events, "mark_last_valid_block");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, txs_keeped_by_block);

  // Create two identical transactions, but don't push it to events list
  MAKE_TX(events, tx_blk_2, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();
  MAKE_TX(events, tx_blk_3, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();

  events.push_back(tx_blk_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_account, tx_blk_2);
  DO_CALLBACK(events, "mark_last_valid_block");

  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  events.push_back(tx_blk_3);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_account, tx_blk_3);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_keeped_by_block>
bool gen_double_spend_in_alt_chain_in_the_same_block<txs_keeped_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, txs_keeped_by_block);

  // Main chain
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_account);
  DO_CALLBACK(events, "mark_last_valid_block");

  // Alt chain
  MAKE_TX_LIST_START(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  cryptonote::transaction tx_1 = txs_1.front();
  auto tx_1_idx = events.size() - 1;
  // Remove tx_1, it is being inserted back a little later
  events.pop_back();

  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  MAKE_TX_LIST(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.insert(events.begin() + tx_1_idx, tx_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_1r, miner_account, txs_1);

  // Try to switch to alternative chain
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_keeped_by_block>
bool gen_double_spend_in_alt_chain_in_different_blocks<txs_keeped_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, txs_keeped_by_block);

  // Main chain
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_account);
  DO_CALLBACK(events, "mark_last_valid_block");

  // Alternative chain
  MAKE_TX(events, tx_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();
  MAKE_TX(events, tx_2, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();

  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_1r, miner_account, tx_1);

  // Try to switch to alternative chain
  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  events.push_back(tx_2);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_account, tx_2);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}
