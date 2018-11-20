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
#include "ring_signature_1.h"

using namespace epee;
using namespace cryptonote;


////////
// class gen_ring_signature_1;

gen_ring_signature_1::gen_ring_signature_1()
{
  REGISTER_CALLBACK("check_balances_1", gen_ring_signature_1::check_balances_1);
  REGISTER_CALLBACK("check_balances_2", gen_ring_signature_1::check_balances_2);
}

namespace
{
  // To be sure that miner tx outputs don't match any bob_account and some_accounts inputs
  const uint64_t rnd_11 = 475921;
  const uint64_t rnd_20 = 360934;
  const uint64_t rnd_29 = 799665;
}

bool gen_ring_signature_1::generate(std::vector<test_event_entry>& events) const
{

  linear_chain_generator gen(events);
  gen.create_genesis_block();

  const auto miner = gen.first_miner();
  const auto bob = gen.create_account(); /// event 1
  const auto alice = gen.create_account(); /// event 2
  const auto some_account_1 = gen.create_account();
  const auto some_account_2 = gen.create_account();

  /// give the miner some outputs to spend and ulock them
  gen.rewind_blocks_n(20);
  gen.rewind_blocks();

  std::vector<cryptonote::transaction> txs;
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(1)) );
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(11) + rnd_11) );

  txs.push_back( gen.create_tx(miner, bob, MK_COINS(11) + rnd_11) );
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(20) + rnd_20) );
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(29) + rnd_29) );
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(29) + rnd_29) );
  txs.push_back( gen.create_tx(miner, bob, MK_COINS(29) + rnd_29) );
  txs.push_back( gen.create_tx(miner, some_account_1, MK_COINS(11) + rnd_11) );
  txs.push_back( gen.create_tx(miner, some_account_1, MK_COINS(11) + rnd_11) );
  txs.push_back( gen.create_tx(miner, some_account_1, MK_COINS(11) + rnd_11) );
  txs.push_back( gen.create_tx(miner, some_account_1, MK_COINS(11) + rnd_11) );
  txs.push_back( gen.create_tx(miner, some_account_1, MK_COINS(20) + rnd_20) );
  txs.push_back( gen.create_tx(miner, some_account_2, MK_COINS(20) + rnd_20) );

  gen.create_block(txs);

  gen.rewind_blocks();

  DO_CALLBACK(events, "check_balances_1");

  auto tx = gen.create_tx(bob, alice, MK_COINS(129) + 2 * rnd_11 + rnd_20 + 3 * rnd_29 - TESTS_DEFAULT_FEE);
  gen.create_block({tx});

  DO_CALLBACK(events, "check_balances_2");

  return true;
}

bool gen_ring_signature_1::check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_1::check_balances_1");

  m_bob_account = boost::get<account_base>(events[1]);
  m_alice_account = boost::get<account_base>(events[2]);

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 1000, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(130) + 2 * rnd_11 + rnd_20 + 3 * rnd_29, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(0, get_balance(m_alice_account, chain, mtx));

  return true;
}

bool gen_ring_signature_1::check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_1::check_balances_2");

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 1000, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(1), get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(MK_COINS(129) + 2 * rnd_11 + rnd_20 + 3 * rnd_29 - TESTS_DEFAULT_FEE, get_balance(m_alice_account, chain, mtx));

  return true;
}


////////
// class gen_ring_signature_2;

gen_ring_signature_2::gen_ring_signature_2()
{
  REGISTER_CALLBACK("check_balances_1", gen_ring_signature_2::check_balances_1);
  REGISTER_CALLBACK("check_balances_2", gen_ring_signature_2::check_balances_2);
}

/**
 * Bob has 4 inputs by 13 coins. He sends 4 * 13 coins to Alice, using ring signature with nmix = 9. Each Bob's input
 * is used as mix for 9 others.
 */
bool gen_ring_signature_2::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);

  //                                                                                                    events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                           //  0
  MAKE_ACCOUNT(events, bob_account);                                                                    //  1
  MAKE_ACCOUNT(events, alice_account);                                                                  //  2
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);                                                 //  3
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                                 //  4
  REWIND_BLOCKS(events, blk_2b, blk_2, miner_account);                                                  // <N blocks>
  MAKE_NEXT_BLOCK(events, blk_3, blk_2b, miner_account);                                                //  5 + N
  REWIND_BLOCKS(events, blk_3r, blk_3, miner_account);                                                  // <N blocks>
  MAKE_TX_LIST_START(events, txs_blk_4, miner_account, bob_account, MK_COINS(13), blk_3);               //  6 + 2N
  MAKE_TX_LIST(events, txs_blk_4, miner_account, bob_account, MK_COINS(13), blk_3);                     //  7 + 2N
  MAKE_TX_LIST(events, txs_blk_4, miner_account, bob_account, MK_COINS(13), blk_3);                     //  8 + 2N
  MAKE_TX_LIST(events, txs_blk_4, miner_account, bob_account, MK_COINS(13), blk_3);                     //  9 + 2N
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_blk_4);                             // 10 + 2N
  DO_CALLBACK(events, "check_balances_1");                                                              // 11 + 2N
  REWIND_BLOCKS(events, blk_4r, blk_4, miner_account);                                                  // <N blocks>
  MAKE_TX_MIX(events, tx_0, bob_account, alice_account, MK_COINS(52) - TESTS_DEFAULT_FEE, 9, blk_4);    // 12 + 3N
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4r, miner_account, tx_0);                                      // 13 + 3N
  DO_CALLBACK(events, "check_balances_2");                                                              // 14 + 3N

  return true;
}

bool gen_ring_signature_2::check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_2::check_balances_1");

  m_bob_account = boost::get<account_base>(events[1]);
  m_alice_account = boost::get<account_base>(events[2]);

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 100 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(52), get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(0, get_balance(m_alice_account, chain, mtx));

  return true;
}

bool gen_ring_signature_2::check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_2::check_balances_2");

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 100 + 3 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(0, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(MK_COINS(52) - TESTS_DEFAULT_FEE, get_balance(m_alice_account, chain, mtx));

  return true;
}


////////
// class gen_ring_signature_big;

gen_ring_signature_big::gen_ring_signature_big()
  : m_test_size(100)
  , m_tx_amount(MK_COINS(29))
{
  REGISTER_CALLBACK("check_balances_1", gen_ring_signature_big::check_balances_1);
  REGISTER_CALLBACK("check_balances_2", gen_ring_signature_big::check_balances_2);
}

/**
 * Check ring signature with m_test_size-1 sources.
 * - Create 100 accounts.
 * - Create 100 blocks, each block contains transaction from the miner to account[i].
 * - Create transaction with ring signature from account[99] to Alice with nmix = 99.
 * - Check balances.
 */
bool gen_ring_signature_big::generate(std::vector<test_event_entry>& events) const
{
  std::vector<account_base> accounts(m_test_size);
  std::vector<block> blocks;
  blocks.reserve(m_test_size + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  for (size_t i = 0; i < m_test_size; ++i)
  {
    MAKE_ACCOUNT(events, an_account);
    accounts[i] = an_account;
  }
  MAKE_ACCOUNT(events, alice_account);

  size_t blk_0r_idx = events.size();
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);
  blocks.push_back(blk_0);
  for (size_t i = blk_0r_idx; i < events.size(); ++i)
  {
    blocks.push_back(boost::get<block>(events[i]));
  }

  for (size_t i = 0; i < m_test_size; ++i)
  {
    block blk_with_unlocked_out = blocks[blocks.size() - 1 - CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW];
    MAKE_TX_LIST_START(events, txs_blk_i, miner_account, accounts[i], m_tx_amount, blk_with_unlocked_out);
    for (size_t j = 0; j <= i; ++j)
    {
      MAKE_TX_LIST(events, txs_blk_i, miner_account, accounts[i], TESTS_DEFAULT_FEE, blk_with_unlocked_out);
    }
    MAKE_NEXT_BLOCK_TX_LIST(events, blk_i, blocks.back(), miner_account, txs_blk_i);
    blocks.push_back(blk_i);

    std::vector<cryptonote::block> chain;
    map_hash2tx_t mtx;
    bool r = find_block_chain(events, chain, mtx, get_block_hash(blk_i));
    CHECK_AND_NO_ASSERT_MES(r, false, "failed to call find_block_chain");
    std::cout << i << ": " << get_balance(accounts[i], chain, mtx) << std::endl;
  }

  DO_CALLBACK(events, "check_balances_1");
  MAKE_TX_MIX(events, tx_0, accounts[0], alice_account, m_tx_amount, m_test_size - 1, blocks.back());
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blocks.back(), miner_account, tx_0);
  DO_CALLBACK(events, "check_balances_2");

  return true;
}

bool gen_ring_signature_big::check_balances_1(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_big::check_balances_1");

  m_bob_account = boost::get<account_base>(events[1]);
  m_alice_account = boost::get<account_base>(events[1 + m_test_size]);

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 2 * m_test_size + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(m_tx_amount + TESTS_DEFAULT_FEE, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(0, get_balance(m_alice_account, chain, mtx));

  for (size_t i = 2; i < 1 + m_test_size; ++i)
  {
    const account_base& an_account = boost::get<account_base>(events[i]);
    uint64_t balance = m_tx_amount + TESTS_DEFAULT_FEE * i;
    CHECK_EQ(balance, get_balance(an_account, chain, mtx));
  }

  return true;
}

bool gen_ring_signature_big::check_balances_2(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_ring_signature_big::check_balances_2");

  std::vector<block> blocks;
  bool r = c.get_blocks(0, 2 * m_test_size + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(0, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(m_tx_amount, get_balance(m_alice_account, chain, mtx));

  for (size_t i = 2; i < 1 + m_test_size; ++i)
  {
    const account_base& an_account = boost::get<account_base>(events[i]);
    uint64_t balance = m_tx_amount + TESTS_DEFAULT_FEE * i;
    CHECK_EQ(balance, get_balance(an_account, chain, mtx));
  }

  std::vector<size_t> tx_outs;
  uint64_t transfered;
  const transaction& tx = boost::get<transaction>(events[events.size() - 3]);
  lookup_acc_outs(m_alice_account.get_keys(), boost::get<transaction>(events[events.size() - 3]), get_tx_pub_key_from_extra(tx), get_additional_tx_pub_keys_from_extra(tx), tx_outs, transfered);
  CHECK_EQ(m_tx_amount, transfered);

  return true;
}
