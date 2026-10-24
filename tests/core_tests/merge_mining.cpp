// Copyright (c) 2026, The Monero Project
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

#include "merge_mining.h"

#include "cryptonote_basic/merge_mining.h"
#include "cryptonote_basic/tx_extra.h"

using namespace cryptonote;

gen_merge_mining_block_template::gen_merge_mining_block_template()
{
  REGISTER_CALLBACK_METHOD(gen_merge_mining_block_template, check_template);
}

bool gen_merge_mining_block_template::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_merge_mining_block_template::generate");
  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, genesis, miner_account, 1338224400);
  events.push_back(miner_account);
  REWIND_BLOCKS(events, head, genesis, miner_account);
  GENERATE_ACCOUNT(receiver);

  // Two ~11 kB v1 transactions exceed the 20 kB median. Fees cover the penalty.
  tx_destination_entry destination{};
  destination.addr = receiver.get_keys().m_account_address;
  destination.amount = 1000;
  const std::vector<tx_destination_entry> destinations(300, destination);
  for (size_t i = 0; i < 2; ++i)
  {
    transaction tx;
    CHECK_TEST_CONDITION(construct_tx_to_key(events, tx, head, miner_account,
      destinations, 100 * TESTS_DEFAULT_FEE, 0));
    events.push_back(tx);
  }
  DO_CALLBACK(events, "check_template");
  return true;
}

bool gen_merge_mining_block_template::check_template(core& c, size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_merge_mining_block_template::check_template");
  const account_base& miner_account = boost::get<account_base>(events.at(1));
  const transaction& first_tx = boost::get<transaction>(events.at(ev_index - 2));
  const transaction& second_tx = boost::get<transaction>(events.at(ev_index - 1));
  const size_t txs_weight = get_transaction_weight(first_tx) + get_transaction_weight(second_tx);
  const uint64_t fees = get_tx_fee(first_tx) + get_tx_fee(second_tx);
  Blockchain& blockchain = c.get_blockchain_storage();
  const uint64_t median = blockchain.get_current_cumulative_block_weight_median();
  const uint64_t generated_coins = blockchain.get_db().get_block_already_generated_coins(
    c.get_current_blockchain_height() - 1);
  const blobdata extra_nonce(80, '\x5a');
  difficulty_type difficulty;
  uint64_t height = 0, expected_reward = 0, cumulative_weight = 0, seed_height = 0;
  crypto::hash seed_hash;
  block b;

  const auto get_template = [&](bool merge_mining)
  {
    b = {};
    return blockchain.create_block_template(b, nullptr, miner_account.get_keys().m_account_address,
      difficulty, height, expected_reward, cumulative_weight, extra_nonce, seed_height,
      seed_hash, true, merge_mining);
  };

  crypto::hash previous_miner_tx = crypto::null_hash;
  for (const bool merge_mining : {false, true, false, true})
  {
    CHECK_TEST_CONDITION(get_template(merge_mining));
    const crypto::hash miner_tx_hash = get_transaction_hash(b.miner_tx);
    CHECK_TEST_CONDITION(miner_tx_hash != previous_miner_tx);
    previous_miner_tx = miner_tx_hash;
    CHECK_EQ(2, b.tx_hashes.size());
    CHECK_TEST_CONDITION(cumulative_weight > median);
    CHECK_EQ(txs_weight + get_transaction_weight(b.miner_tx), cumulative_weight);
    uint64_t reward;
    CHECK_TEST_CONDITION(get_block_reward(median, cumulative_weight, generated_coins, reward, b.major_version));
    CHECK_EQ(reward + fees, get_outs_money_amount(b.miner_tx));

    std::vector<tx_extra_field> fields;
    CHECK_TEST_CONDITION(parse_tx_extra(b.miner_tx.extra, fields));
    tx_extra_padding padding{};
    const bool has_padding = find_tx_extra_field_by_type(fields, padding);
    // Ordinary templates can also have substantial reward-convergence padding.
    if (merge_mining)
      CHECK_TEST_CONDITION(has_padding && padding.size >= TX_EXTRA_MERGE_MINING_TAG_MAX_SIZE);
    tx_extra_nonce nonce;
    CHECK_TEST_CONDITION(find_tx_extra_field_by_type(fields, nonce));
    CHECK_EQ(extra_nonce, nonce.nonce);
  }
  const crypto::hash cached_miner_tx = get_transaction_hash(b.miner_tx);
  CHECK_TEST_CONDITION(get_template(true));
  CHECK_EQ(cached_miner_tx, get_transaction_hash(b.miner_tx));

  const size_t miner_weight = get_transaction_weight(b.miner_tx);
  const crypto::hash merkle_root = crypto::cn_fast_hash(extra_nonce.data(), extra_nonce.size());
  CHECK_TEST_CONDITION(update_mm_merkle_root_in_tx_extra(b.miner_tx.extra, merkle_root, encode_mm_depth(1, 0)));
  b.miner_tx.invalidate_hashes();
  b.invalidate_hashes();
  CHECK_EQ(miner_weight, get_transaction_weight(b.miner_tx));

  // A 35-byte append would overpay the reward for this block.
  uint64_t oversized_reward;
  CHECK_TEST_CONDITION(get_block_reward(median, cumulative_weight + 35, generated_coins,
    oversized_reward, b.major_version));
  CHECK_TEST_CONDITION(oversized_reward + fees < get_outs_money_amount(b.miner_tx));

  CHECK_TEST_CONDITION(miner::find_nonce_for_given_block(
    [&blockchain](const block& blk, uint64_t block_height, const crypto::hash* seed, unsigned int threads, crypto::hash& hash)
    {
      return get_block_longhash(&blockchain, blk, hash, block_height, seed, threads);
    }, b, difficulty, height, &seed_hash));
  b.invalidate_hashes();
  block_verification_context bvc{};
  pool_supplement supplement;
  CHECK_TEST_CONDITION(c.handle_single_incoming_block(block_to_blob(b), &b, bvc, supplement, false));
  CHECK_TEST_CONDITION(!bvc.m_verifivation_failed);
  CHECK_TEST_CONDITION(bvc.m_added_to_main_chain);
  CHECK_EQ(height + 1, c.get_current_blockchain_height());
  CHECK_EQ(0, c.get_pool_transactions_count());
  return true;
}
