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
#include "service_nodes.h"

using namespace std;

using namespace epee;
using namespace cryptonote;


gen_service_nodes::gen_service_nodes()
{
  m_alice_service_node_keys = keypair::generate(hw::get_device("default"));

  REGISTER_CALLBACK("check_stuff", gen_service_nodes::check_stuff);
}
//-----------------------------------------------------------------------------------------------------
bool gen_service_nodes::generate(std::vector<test_event_entry> &events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner);

  MAKE_GENESIS_BLOCK(events, blk_0, miner, ts_start);
  MAKE_ACCOUNT(events, alice);

  generator.set_hf_version(8);
  MAKE_NEXT_BLOCK(events, blk_a, blk_0, miner);

  generator.set_hf_version(9);
  MAKE_NEXT_BLOCK(events, blk_b, blk_a, miner);


  REWIND_BLOCKS_N(events, blk_c, blk_b, miner, 10);

  REWIND_BLOCKS(events, blk_1, blk_c, miner);

  MAKE_TX(events, tx_0, miner, alice, MK_COINS(101), blk_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner, tx_0);

  REWIND_BLOCKS(events, blk_3, blk_2, miner);

  cryptonote::transaction alice_registration =
    make_registration_tx(events, alice, m_alice_service_node_keys, 0, { alice.get_keys().m_account_address }, { STAKING_PORTIONS }, blk_3);

  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner, alice_registration);

  DO_CALLBACK(events, "check_stuff");

  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_service_nodes::check_stuff(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_service_nodes::check_stuff");

  cryptonote::account_base alice = boost::get<cryptonote::account_base>(events[1]);

  std::list<block> block_list;
  bool r = c.get_blocks(0, 1000, block_list);
  CHECK_TEST_CONDITION(r);
  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  std::vector<block> blocks(block_list.begin(), block_list.end());
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);

  const uint64_t staking_requirement = MK_COINS(100);

  CHECK_EQ(MK_COINS(101) - TESTS_DEFAULT_FEE - staking_requirement, get_unlocked_balance(alice, blocks, mtx));

  return true;
}
