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

#include <vector>
#include <iostream>

#include "include_base_utils.h"

#include "console_handler.h"

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

#include "chaingen.h"
#include "chaingen_tests_list.h"

using namespace std;

using namespace epee;
using namespace cryptonote;

////////
// class one_block;

using eventV = std::vector<test_event_entry>;

one_block::one_block()
{
  REGISTER_CALLBACK("verify_1", one_block::verify_1);
}

bool one_block::generate(eventV &events)
{
    uint64_t ts_start = 1338224400;

    MAKE_GENESIS_BLOCK(events, blk_0, alice, ts_start);
    MAKE_ACCOUNT(events, alice);
    DO_CALLBACK(events, "verify_1");

    return true;
}

bool one_block::verify_1(cryptonote::core& c, size_t ev_index, const eventV &events)
{
    DEFINE_TESTS_ERROR_CONTEXT("one_block::verify_1");

    alice = boost::get<cryptonote::account_base>(events[1]);

    // check balances
    //std::vector<const cryptonote::block*> chain;
    //map_hash2tx_t mtx;
    //CHECK_TEST_CONDITION(find_block_chain(events, chain, mtx, get_block_hash(boost::get<cryptonote::block>(events[1]))));
    //CHECK_TEST_CONDITION(get_block_reward(0) == get_balance(alice, events, chain, mtx));

    // check height
    std::vector<cryptonote::block> blocks;
    std::list<crypto::public_key> outs;
    bool r = c.get_blocks(0, 100, blocks);
    //c.get_outs(100, outs);
    CHECK_TEST_CONDITION(r);
    CHECK_TEST_CONDITION(blocks.size() == 1);
    //CHECK_TEST_CONDITION(outs.size() == blocks.size());
    CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 1);
    CHECK_TEST_CONDITION(blocks.back() == boost::get<cryptonote::block>(events[0]));

    return true;
}


////////
// class gen_simple_chain_001;

gen_simple_chain_001::gen_simple_chain_001()
{
  REGISTER_CALLBACK("verify_callback_1", gen_simple_chain_001::verify_callback_1);
  REGISTER_CALLBACK("verify_callback_2", gen_simple_chain_001::verify_callback_2);
}

static void make_rct_tx(eventV& events,
                        std::vector<cryptonote::transaction>& txs,
                        const cryptonote::block& blk_head,
                        const cryptonote::account_base& from,
                        const cryptonote::account_base& to,
                        uint64_t amount)
{
    txs.emplace_back();

    bool success = TxBuilder(events, txs.back(), blk_head, from, to, amount, cryptonote::network_version_7).build();
    /// TODO: beter error message
    if (!success) throw std::exception();
    events.push_back(txs.back());
}

/// generate 30 more blocks to unlock outputs
static void rewind_blocks(test_generator& gen, eventV& events, std::vector<cryptonote::block>& chain, const cryptonote::account_base& miner)
{
    for (auto i = 0u; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i) {
        chain.emplace_back();
        const auto idx = chain.size() - 1;
        gen.construct_block(chain[idx], chain[idx - 1], miner);
        events.push_back(chain[idx]);
    }
}

void construct_block(test_generator& gen, eventV& events, std::vector<cryptonote::block>& chain, std::vector<cryptonote::transaction>& txs, const cryptonote::account_base& miner) {

    chain.emplace_back();
    const auto idx = chain.size() - 1;
        /// todo: change this to take a vector instead?
    gen.construct_block(chain[idx], chain[idx - 1], miner, { txs.begin(), txs.end() });
    events.push_back(chain.back());
}

bool gen_simple_chain_001::generate(eventV& events)
{

    uint64_t ts_start = 1338224400;
    GENERATE_ACCOUNT(miner);
    GENERATE_ACCOUNT(alice);

    test_generator generator;
    std::vector<cryptonote::block> chain;

    chain.resize(1);
    generator.construct_block(chain.back(), miner, ts_start);
    events.push_back(chain[0]);

    cryptonote::block blk_side;
    generator.construct_block(blk_side, chain.back(), miner);
    events.push_back(chain.back());

    /// Note: to create N RingCT transactions need at least 10 + N outputs
    while (chain.size() < 10) {
        chain.emplace_back();
        const auto idx = chain.size() - 1;
        generator.construct_block(chain[idx], chain[idx-1], miner);
        events.push_back(chain.back());
    }

    rewind_blocks(generator, events, chain, miner);

    std::vector<cryptonote::transaction> txs;

    make_rct_tx(events, txs, chain.back(), miner, alice, MK_COINS(1));
    make_rct_tx(events, txs, chain.back(), miner, alice, MK_COINS(2));
    make_rct_tx(events, txs, chain.back(), miner, alice, MK_COINS(4));

    construct_block(generator, events, chain, txs, miner);

    auto last_unlocked = chain.back();
    rewind_blocks(generator, events, chain, miner);
    txs.clear();
    make_rct_tx(events, txs, last_unlocked, miner, alice, MK_COINS(50));
    construct_block(generator, events, chain, txs, miner);

    last_unlocked = chain.back();
    rewind_blocks(generator, events, chain, miner);
    txs.clear();
    make_rct_tx(events, txs, last_unlocked, miner, alice, MK_COINS(50));
    construct_block(generator, events, chain, txs, miner);

    last_unlocked = chain.back();
    rewind_blocks(generator, events, chain, miner);
    txs.clear();
    make_rct_tx(events, txs, last_unlocked, miner, alice, MK_COINS(50));
    construct_block(generator, events, chain, txs, miner);

    DO_CALLBACK(events, "verify_callback_1");

    return true;
}

bool gen_simple_chain_001::verify_callback_1(cryptonote::core& c, size_t ev_index, const eventV &events)
{
  return true;
}

bool gen_simple_chain_001::verify_callback_2(cryptonote::core& c, size_t ev_index, const eventV &events)
{
  return true;
}
