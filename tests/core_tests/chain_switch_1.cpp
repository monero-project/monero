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

#include "chaingen.h"
#include "chain_switch_1.h"

using namespace epee;
using namespace cryptonote;


gen_chain_switch_1::gen_chain_switch_1()
{
  REGISTER_CALLBACK("check_split_not_switched", gen_chain_switch_1::check_split_not_switched);
  REGISTER_CALLBACK("check_split_switched", gen_chain_switch_1::check_split_switched);
}


//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  /*
  (0 )-(1 )-(2 ) -(3 )-(4 )                  <- main chain, until 7 isn't connected
              \ |-(5 )-(6 )-(7 )|            <- alt chain, until 7 isn't connected

  transactions ([n] - tx amount, (m) - block):
  (1)     : miner -[ 5]-> account_1 ( +5 in main chain,  +5 in alt chain)
  (3)     : miner -[ 7]-> account_2 ( +7 in main chain,  +0 in alt chain), tx will be in tx pool after switch
  (4), (6): miner -[11]-> account_3 (+11 in main chain, +11 in alt chain)
  (5)     : miner -[13]-> account_4 ( +0 in main chain, +13 in alt chain), tx will be in tx pool before switch

  transactions orders ([n] - tx amount, (m) - block):
  miner -[1], [2]-> account_1: in main chain (3), (3), in alt chain (5), (6)
  miner -[1], [2]-> account_2: in main chain (3), (4), in alt chain (5), (5)
  miner -[1], [2]-> account_3: in main chain (3), (4), in alt chain (6), (5)
  miner -[1], [2]-> account_4: in main chain (4), (3), in alt chain (5), (6)
  */

  GENERATE_ACCOUNT(miner_account);

  //                                                                                              events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                     //  0
  MAKE_ACCOUNT(events, recipient_account_1);                                                      //  1
  MAKE_ACCOUNT(events, recipient_account_2);                                                      //  2
  MAKE_ACCOUNT(events, recipient_account_3);                                                      //  3
  MAKE_ACCOUNT(events, recipient_account_4);                                                      //  4
  REWIND_BLOCKS(events, blk_0r0, blk_0, miner_account)                                            // <N blocks>
  REWIND_BLOCKS(events, blk_0r, blk_0r0, miner_account)                                           // <N blocks>
  MAKE_TX(events, tx_00, miner_account, recipient_account_1, MK_COINS(5), blk_0r0);               //  5 + 2N
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_00);                               //  6 + 2N
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                           //  7 + 2N
  REWIND_BLOCKS(events, blk_2r, blk_2, miner_account)                                             // <N blocks>

  // Transactions to test account balances after switch
  MAKE_TX_LIST_START(events, txs_blk_3, miner_account, recipient_account_2, MK_COINS(7), blk_2);  //  8 + 3N
  MAKE_TX_LIST_START(events, txs_blk_4, miner_account, recipient_account_3, MK_COINS(11), blk_2); //  9 + 3N
  MAKE_TX_LIST_START(events, txs_blk_5, miner_account, recipient_account_4, MK_COINS(13), blk_2); // 10 + 3N
  std::list<transaction> txs_blk_6;
  txs_blk_6.push_back(txs_blk_4.front());

  // Transactions, that has different order in alt block chains
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_1, MK_COINS(1), blk_2);        // 11 + 3N
  txs_blk_5.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_1, MK_COINS(2), blk_2);        // 12 + 3N
  txs_blk_6.push_back(txs_blk_3.back());

  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_2, MK_COINS(1), blk_2);        // 13 + 3N
  txs_blk_5.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_2, MK_COINS(2), blk_2);        // 14 + 3N
  txs_blk_5.push_back(txs_blk_4.back());

  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_3, MK_COINS(1), blk_2);        // 15 + 3N
  txs_blk_6.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_3, MK_COINS(2), blk_2);        // 16 + 3N
  txs_blk_5.push_back(txs_blk_4.back());

  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_4, MK_COINS(1), blk_2);        // 17 + 3N
  txs_blk_5.push_back(txs_blk_4.back());
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_4, MK_COINS(2), blk_2);        // 18 + 3N
  txs_blk_6.push_back(txs_blk_3.back());

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2r, miner_account, txs_blk_3);                       // 19 + 3N
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3, miner_account, txs_blk_4);                        // 20 + 3N
  //split
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_2r, miner_account, txs_blk_5);                       // 22 + 3N
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5, miner_account, txs_blk_6);                        // 23 + 3N
  DO_CALLBACK(events, "check_split_not_switched");                                                // 21 + 3N
  MAKE_NEXT_BLOCK(events, blk_7, blk_6, miner_account);                                           // 24 + 3N
  DO_CALLBACK(events, "check_split_switched");                                                    // 25 + 3N

  return true;
}


static uint64_t transferred_in_tx(const cryptonote::account_base& account, const cryptonote::transaction& tx) {

  uint64_t total_amount = 0;

  for (auto i = 0u; i < tx.vout.size(); ++i) {

    if(is_out_to_acc(account.get_keys(), boost::get<txout_to_key>(tx.vout[i].target), get_tx_pub_key_from_extra(tx), get_additional_tx_pub_keys_from_extra(tx), i)) {
      total_amount += get_amount(account, tx, i);
    }
  }

  return total_amount;
}

//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::check_split_not_switched(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_chain_switch_1::check_split_not_switched");

  m_recipient_account_1 = boost::get<account_base>(events[1]);
  m_recipient_account_2 = boost::get<account_base>(events[2]);
  m_recipient_account_3 = boost::get<account_base>(events[3]);
  m_recipient_account_4 = boost::get<account_base>(events[4]);

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 10000, blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(5 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks.size());
  CHECK_TEST_CONDITION(blocks.back() == boost::get<block>(events[20 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW]));  // blk_4

  CHECK_EQ(2, c.get_alternative_blocks_count());

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(8),  get_balance(m_recipient_account_1, chain, mtx));
  CHECK_EQ(MK_COINS(10), get_balance(m_recipient_account_2, chain, mtx));
  CHECK_EQ(MK_COINS(14), get_balance(m_recipient_account_3, chain, mtx));
  CHECK_EQ(MK_COINS(3),  get_balance(m_recipient_account_4, chain, mtx));

  std::vector<transaction> tx_pool;
  r = c.get_pool_transactions(tx_pool);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(1, tx_pool.size());

  std::vector<size_t> tx_outs;

  const auto transferred = transferred_in_tx(m_recipient_account_4, tx_pool.front());
  CHECK_EQ(MK_COINS(13), transferred);

  m_chain_1.swap(blocks);
  m_tx_pool.swap(tx_pool);

  return true;
}

//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::check_split_switched(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_chain_switch_1::check_split_switched");

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 10000, blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(6 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks.size());
  auto it = blocks.end();
  --it; --it; --it;
  CHECK_TEST_CONDITION(std::equal(blocks.begin(), it, m_chain_1.begin()));
  CHECK_TEST_CONDITION(blocks.back() == boost::get<block>(events[24 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW]));  // blk_7

  std::vector<block> alt_blocks;
  r = c.get_alternative_blocks(alt_blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(2, c.get_alternative_blocks_count());

  // Some blocks that were in main chain are in alt chain now
  BOOST_FOREACH(block b, alt_blocks)
  {
    CHECK_TEST_CONDITION(m_chain_1.end() != std::find(m_chain_1.begin(), m_chain_1.end(), b));
  }

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(8),  get_balance(m_recipient_account_1, chain, mtx));
  CHECK_EQ(MK_COINS(3),  get_balance(m_recipient_account_2, chain, mtx));
  CHECK_EQ(MK_COINS(14), get_balance(m_recipient_account_3, chain, mtx));
  CHECK_EQ(MK_COINS(16), get_balance(m_recipient_account_4, chain, mtx));

  std::vector<transaction> tx_pool;
  r = c.get_pool_transactions(tx_pool);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(1, tx_pool.size());
  CHECK_TEST_CONDITION(!(tx_pool.front() == m_tx_pool.front()));

  const auto transferred = transferred_in_tx(m_recipient_account_2, tx_pool.front());
  CHECK_EQ(MK_COINS(7), transferred);

  return true;
}