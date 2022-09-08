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

#include <boost/foreach.hpp>

#include "chaingen.h"
#include "block_reward.h"

using namespace epee;
using namespace cryptonote;

namespace
{
  bool construct_miner_tx_by_weight(transaction& miner_tx, uint64_t height, uint64_t already_generated_coins,
    const account_public_address& miner_address, std::vector<size_t>& block_weights, size_t target_tx_weight,
    size_t target_block_weight, uint64_t fee = 0)
  {
    if (!construct_miner_tx(height, misc_utils::median(block_weights), already_generated_coins, target_block_weight, fee, miner_address, miner_tx))
      return false;

    size_t current_weight = get_transaction_weight(miner_tx);
    size_t try_count = 0;
    while (target_tx_weight != current_weight)
    {
      ++try_count;
      if (10 < try_count)
        return false;

      if (target_tx_weight < current_weight)
      {
        size_t diff = current_weight - target_tx_weight;
        if (diff <= miner_tx.extra.size())
          miner_tx.extra.resize(miner_tx.extra.size() - diff);
        else
          return false;
      }
      else
      {
        size_t diff = target_tx_weight - current_weight;
        miner_tx.extra.resize(miner_tx.extra.size() + diff);
      }

      current_weight = get_transaction_weight(miner_tx);
    }

    return true;
  }

  bool construct_max_weight_block(test_generator& generator, block& blk, const block& blk_prev, const account_base& miner_account,
    size_t median_block_count = CRYPTONOTE_REWARD_BLOCKS_WINDOW)
  {
    std::vector<size_t> block_weights;
    generator.get_last_n_block_weights(block_weights, get_block_hash(blk_prev), median_block_count);

    size_t median = misc_utils::median(block_weights);
    median = std::max(median, static_cast<size_t>(CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1));

    transaction miner_tx;
    bool r = construct_miner_tx_by_weight(miner_tx, get_block_height(blk_prev) + 1, generator.get_already_generated_coins(blk_prev),
      miner_account.get_keys().m_account_address, block_weights, 2 * median, 2 * median);
    if (!r)
      return false;

    return generator.construct_block_manually(blk, blk_prev, miner_account, test_generator::bf_miner_tx, 0, 0, 0,
      crypto::hash(), 0, miner_tx);
  }

  bool rewind_blocks(std::vector<test_event_entry>& events, test_generator& generator, block& blk, const block& blk_prev,
    const account_base& miner_account, size_t block_count)
  {
    blk = blk_prev;
    for (size_t i = 0; i < block_count; ++i)
    {
      block blk_i;
      if (!construct_max_weight_block(generator, blk_i, blk, miner_account))
        return false;

      events.push_back(blk_i);
      blk = blk_i;
    }

    return true;
  }

  uint64_t get_tx_out_amount(const transaction& tx)
  {
    uint64_t amount = 0;
    BOOST_FOREACH(auto& o, tx.vout)
      amount += o.amount;
    return amount;
  }
}

gen_block_reward::gen_block_reward()
  : m_invalid_block_index(0)
{
  REGISTER_CALLBACK_METHOD(gen_block_reward, mark_invalid_block);
  REGISTER_CALLBACK_METHOD(gen_block_reward, mark_checked_block);
  REGISTER_CALLBACK_METHOD(gen_block_reward, check_block_rewards);
}

bool gen_block_reward::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  DO_CALLBACK(events, "mark_checked_block");
  MAKE_ACCOUNT(events, bob_account);

  // Test: miner transactions without outputs (block reward == 0)
  block blk_0r;
  if (!rewind_blocks(events, generator, blk_0r, blk_0, miner_account, CRYPTONOTE_REWARD_BLOCKS_WINDOW))
    return false;

  // Test: block reward is calculated using median of the latest CRYPTONOTE_REWARD_BLOCKS_WINDOW blocks
  DO_CALLBACK(events, "mark_invalid_block");
  block blk_1_bad_1;
  if (!construct_max_weight_block(generator, blk_1_bad_1, blk_0r, miner_account, CRYPTONOTE_REWARD_BLOCKS_WINDOW + 1))
    return false;
  events.push_back(blk_1_bad_1);

  DO_CALLBACK(events, "mark_invalid_block");
  block blk_1_bad_2;
  if (!construct_max_weight_block(generator, blk_1_bad_2, blk_0r, miner_account, CRYPTONOTE_REWARD_BLOCKS_WINDOW - 1))
    return false;
  events.push_back(blk_1_bad_2);

  block blk_1;
  if (!construct_max_weight_block(generator, blk_1, blk_0r, miner_account))
    return false;
  events.push_back(blk_1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);
  DO_CALLBACK(events, "mark_checked_block");
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);
  DO_CALLBACK(events, "mark_checked_block");
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);
  DO_CALLBACK(events, "mark_checked_block");
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_account);
  DO_CALLBACK(events, "mark_checked_block");

  block blk_5r;
  if (!rewind_blocks(events, generator, blk_5r, blk_5, miner_account, CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW))
    return false;

  // Test: fee increases block reward
  transaction tx_0(construct_tx_with_fee(events, blk_5, miner_account, bob_account, MK_COINS(1), 3 * TESTS_DEFAULT_FEE));
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5r, miner_account, tx_0);
  DO_CALLBACK(events, "mark_checked_block");

  // Test: fee from all block transactions increase block reward
  std::list<transaction> txs_0;
  txs_0.push_back(construct_tx_with_fee(events, blk_5, miner_account, bob_account, MK_COINS(1), 5 * TESTS_DEFAULT_FEE));
  txs_0.push_back(construct_tx_with_fee(events, blk_5, miner_account, bob_account, MK_COINS(1), 7 * TESTS_DEFAULT_FEE));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_7, blk_6, miner_account, txs_0);
  DO_CALLBACK(events, "mark_checked_block");

  // Test: block reward == transactions fee
  {
    transaction tx_1 = construct_tx_with_fee(events, blk_5, miner_account, bob_account, MK_COINS(1), 11 * TESTS_DEFAULT_FEE);
    transaction tx_2 = construct_tx_with_fee(events, blk_5, miner_account, bob_account, MK_COINS(1), 13 * TESTS_DEFAULT_FEE);
    size_t txs_1_weight = get_transaction_weight(tx_1) + get_transaction_weight(tx_2);
    uint64_t txs_fee = get_tx_fee(tx_1) + get_tx_fee(tx_2);

    std::vector<size_t> block_weights;
    generator.get_last_n_block_weights(block_weights, get_block_hash(blk_7), CRYPTONOTE_REWARD_BLOCKS_WINDOW);
    size_t median = misc_utils::median(block_weights);

    transaction miner_tx;
    bool r = construct_miner_tx_by_weight(miner_tx, get_block_height(blk_7) + 1, generator.get_already_generated_coins(blk_7),
      miner_account.get_keys().m_account_address, block_weights, 2 * median - txs_1_weight, 2 * median, txs_fee);
    if (!r)
      return false;

    std::vector<crypto::hash> txs_1_hashes;
    txs_1_hashes.push_back(get_transaction_hash(tx_1));
    txs_1_hashes.push_back(get_transaction_hash(tx_2));

    block blk_8;
    generator.construct_block_manually(blk_8, blk_7, miner_account, test_generator::bf_miner_tx | test_generator::bf_tx_hashes,
      0, 0, 0, crypto::hash(), 0, miner_tx, txs_1_hashes, txs_1_weight);

    events.push_back(blk_8);
    DO_CALLBACK(events, "mark_checked_block");
  }

  DO_CALLBACK(events, "check_block_rewards");

  return true;
}

bool gen_block_reward::check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/)
{
  if (m_invalid_block_index == event_idx)
  {
    m_invalid_block_index = 0;
    return bvc.m_verifivation_failed;
  }
  else
  {
    return !bvc.m_verifivation_failed;
  }
}

bool gen_block_reward::mark_invalid_block(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_invalid_block_index = ev_index + 1;
  return true;
}

bool gen_block_reward::mark_checked_block(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_checked_blocks_indices.push_back(ev_index - 1);
  return true;
}

bool gen_block_reward::check_block_rewards(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_block_reward_without_txs::check_block_rewards");

  std::array<uint64_t, 7> blk_rewards;
  blk_rewards[0] = MONEY_SUPPLY >> EMISSION_SPEED_FACTOR_PER_MINUTE;
  uint64_t cumulative_reward = blk_rewards[0];
  for (size_t i = 1; i < blk_rewards.size(); ++i)
  {
    blk_rewards[i] = (MONEY_SUPPLY - cumulative_reward) >> EMISSION_SPEED_FACTOR_PER_MINUTE;
    cumulative_reward += blk_rewards[i];
  }

  for (size_t i = 0; i < 5; ++i)
  {
    block blk_i = boost::get<block>(events[m_checked_blocks_indices[i]]);
    CHECK_EQ(blk_rewards[i], get_tx_out_amount(blk_i.miner_tx));
  }

  block blk_n1 = boost::get<block>(events[m_checked_blocks_indices[5]]);
  CHECK_EQ(blk_rewards[5] + 3 * TESTS_DEFAULT_FEE, get_tx_out_amount(blk_n1.miner_tx));

  block blk_n2 = boost::get<block>(events[m_checked_blocks_indices[6]]);
  CHECK_EQ(blk_rewards[6] + (5 + 7) * TESTS_DEFAULT_FEE, get_tx_out_amount(blk_n2.miner_tx));

  block blk_n3 = boost::get<block>(events[m_checked_blocks_indices[7]]);
  CHECK_EQ((11 + 13) * TESTS_DEFAULT_FEE, get_tx_out_amount(blk_n3.miner_tx));

  return true;
}
