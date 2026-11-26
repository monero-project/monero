// Copyright (c) 2019-2024, The Monero Project
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

#define IN_UNIT_TESTS
#include "block_pow_cache.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include "scope_guard.h"

namespace
{
  // Exercise the retained cache without batch PoW precomputation.
  bool submit_block(cryptonote::core& c, const cryptonote::block& block,
    cryptonote::block_verification_context& bvc)
  {
    cryptonote::pool_supplement supplement;
    return c.handle_single_incoming_block(cryptonote::block_to_blob(block), &block, bvc, supplement);
  }
}

block_pow_cache::block_pow_cache()
{
  REGISTER_CALLBACK_METHOD(block_pow_cache, check_reuse);
  REGISTER_CALLBACK_METHOD(block_pow_cache, check_admission);
  REGISTER_CALLBACK_METHOD(block_pow_cache, check_eviction);
  REGISTER_CALLBACK_METHOD(block_pow_cache, check_new_tip);
}

bool block_pow_cache::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(bob_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, 1338224400);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);
  // Each callback submits the following block with its transaction removed from the pool.
  MAKE_TX(events, tx_1, miner_account, bob_account, 1000, blk_0r);
  DO_CALLBACK(events, "check_reuse");
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_1);
  MAKE_TX(events, tx_2, miner_account, bob_account, 1000, blk_1);
  DO_CALLBACK(events, "check_new_tip");
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_account, tx_2);
  MAKE_TX(events, tx_3, miner_account, bob_account, 1000, blk_2);
  DO_CALLBACK(events, "check_admission");
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_account, tx_3);
  MAKE_TX(events, tx_4, miner_account, bob_account, 1000, blk_3);
  DO_CALLBACK(events, "check_eviction");
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_account, tx_4);
  return true;
}

bool block_pow_cache::check_reuse(cryptonote::core& c, size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  const std::string perr_context = "block_pow_cache check_reuse";
  auto& blockchain = c.get_blockchain_storage();
  auto& cache = blockchain.m_missing_tx_pow_cache;
  const auto& tx = boost::get<cryptonote::transaction>(events.at(ev_index - 1));
  const auto& block = boost::get<cryptonote::block>(events.at(ev_index + 1));
  const auto hash = cryptonote::get_block_hash(block);
  const auto height = cryptonote::get_block_height(block);
  CHECK_TEST_CONDITION(blockchain.flush_txes_from_pool({cryptonote::get_transaction_hash(tx)}));

  cryptonote::block_verification_context bvc{};
  CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
  CHECK_TEST_CONDITION(cache.size() == 1 && cache.front().first == hash);
  const crypto::hash pow = cryptonote::get_block_longhash(&blockchain, block, height);
  CHECK_TEST_CONDITION(cache.front().second == pow);
  CHECK_TEST_CONDITION(blockchain.m_blocks_longhash_table.empty());
  CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
  CHECK_TEST_CONDITION(cache.size() == 1);

  // Override the cached digest to verify reuse and difficulty checking.
  // At difficulty two, a zero digest passes and an all-ones digest fails.
  {
    auto saved_cache = cache;
    const auto difficulty = blockchain.get_difficulty_for_next_block();
    const epee::scope_guard restore([&]() {
      cache.swap(saved_cache);
      blockchain.m_difficulty_for_next_block = difficulty;
    });
    blockchain.m_difficulty_for_next_block = 2;
    cache.front().second = crypto::null_hash;
    CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
    CHECK_TEST_CONDITION(cache.size() == 1 && cache.front().first == hash);
    std::memset(cache.front().second.data, 0xff, sizeof(crypto::hash));
    CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_bad_pow && !bvc.m_missing_txs);
  }

  cryptonote::tx_verification_context tvc{};
  CHECK_TEST_CONDITION(c.handle_incoming_tx(cryptonote::tx_to_blob(tx), tvc, cryptonote::relay_method::fluff, false));
  CHECK_TEST_CONDITION(!tvc.m_verifivation_failed);
  return true;
}

bool block_pow_cache::check_admission(cryptonote::core& c, size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  const std::string perr_context = "block_pow_cache check_admission";
  auto& blockchain = c.get_blockchain_storage();
  auto& cache = blockchain.m_missing_tx_pow_cache;
  const auto& tx = boost::get<cryptonote::transaction>(events.at(ev_index - 1));
  const auto& block = boost::get<cryptonote::block>(events.at(ev_index + 1));
  CHECK_TEST_CONDITION(blockchain.flush_txes_from_pool({cryptonote::get_transaction_hash(tx)}));

  cryptonote::block_verification_context bvc{};
  CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
  const auto saved_cache = cache;
  // A failing PoW must leave existing entries intact.
  cryptonote::block sibling = block;
  ++sibling.nonce;
  sibling.invalidate_hashes();
  {
    const auto difficulty = blockchain.get_difficulty_for_next_block();
    const epee::scope_guard restore_difficulty([&]() {
      blockchain.m_difficulty_for_next_block = difficulty;
    });
    blockchain.m_difficulty_for_next_block = std::numeric_limits<cryptonote::difficulty_type>::max();
    CHECK_TEST_CONDITION(submit_block(c, sibling, bvc) && bvc.m_bad_pow);
    CHECK_TEST_CONDITION(cache == saved_cache);
  }

#ifdef PER_BLOCK_CHECKPOINT
  {
    const auto height = cryptonote::get_block_height(block);
    // A checkpoint can skip PoW calculation; its placeholder must not enter the cache.
    auto checkpoints = std::move(blockchain.m_blocks_hash_check);
    const epee::scope_guard restore_checkpoints([&]() {
      blockchain.m_blocks_hash_check = std::move(checkpoints);
    });
    blockchain.m_blocks_hash_check.resize(height + 1);
    blockchain.m_blocks_hash_check[height].first = cryptonote::get_block_hash(sibling);
    CHECK_TEST_CONDITION(submit_block(c, sibling, bvc) && bvc.m_missing_txs);
    CHECK_TEST_CONDITION(cache == saved_cache);
  }
#endif

  cryptonote::tx_verification_context tvc{};
  CHECK_TEST_CONDITION(c.handle_incoming_tx(cryptonote::tx_to_blob(tx), tvc, cryptonote::relay_method::fluff, false));
  CHECK_TEST_CONDITION(!tvc.m_verifivation_failed);
  return true;
}

bool block_pow_cache::check_eviction(cryptonote::core& c, size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  const std::string perr_context = "block_pow_cache check_eviction";
  auto& blockchain = c.get_blockchain_storage();
  auto& cache = blockchain.m_missing_tx_pow_cache;
  const auto& tx = boost::get<cryptonote::transaction>(events.at(ev_index - 1));
  const auto& block = boost::get<cryptonote::block>(events.at(ev_index + 1));
  const auto hash = cryptonote::get_block_hash(block);
  CHECK_TEST_CONDITION(blockchain.flush_txes_from_pool({cryptonote::get_transaction_hash(tx)}));

  cryptonote::block_verification_context bvc{};
  CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
  // Test-chain difficulty is one, so these distinct siblings all pass PoW.
  CHECK_TEST_CONDITION(blockchain.get_difficulty_for_next_block() == 1);
  for (uint32_t i = 1; i <= 8; ++i)
  {
    cryptonote::block sibling = block;
    sibling.nonce += i;
    sibling.invalidate_hashes();
    CHECK_TEST_CONDITION(submit_block(c, sibling, bvc) && bvc.m_missing_txs);
    CHECK_TEST_CONDITION(cache.size() <= 8);
  }
  CHECK_TEST_CONDITION(cache.size() == 8);
  CHECK_TEST_CONDITION(std::none_of(cache.begin(), cache.end(),
    [&hash](const auto& entry) { return entry.first == hash; }));

  CHECK_TEST_CONDITION(submit_block(c, block, bvc) && bvc.m_missing_txs);
  cryptonote::tx_verification_context tvc{};
  CHECK_TEST_CONDITION(c.handle_incoming_tx(cryptonote::tx_to_blob(tx), tvc, cryptonote::relay_method::fluff, false));
  CHECK_TEST_CONDITION(!tvc.m_verifivation_failed);
  return true;
}

bool block_pow_cache::check_new_tip(cryptonote::core& c, size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  const std::string perr_context = "block_pow_cache new tip";
  auto& blockchain = c.get_blockchain_storage();
  const auto& tx = boost::get<cryptonote::transaction>(events.at(ev_index - 1));
  const auto& block = boost::get<cryptonote::block>(events.at(ev_index + 1));
  CHECK_TEST_CONDITION(c.get_tail_id() == block.prev_id);
  CHECK_TEST_CONDITION(blockchain.flush_txes_from_pool({cryptonote::get_transaction_hash(tx)}));
  cryptonote::block_verification_context bvc{};
  CHECK_TEST_CONDITION(submit_block(c, block, bvc));
  CHECK_TEST_CONDITION(bvc.m_missing_txs);
  CHECK_TEST_CONDITION(blockchain.m_missing_tx_pow_cache_tip == block.prev_id);
  CHECK_TEST_CONDITION(blockchain.m_missing_tx_pow_cache.size() == 1);
  CHECK_TEST_CONDITION(blockchain.m_missing_tx_pow_cache.front().first == cryptonote::get_block_hash(block));
  cryptonote::tx_verification_context tvc{};
  CHECK_TEST_CONDITION(c.handle_incoming_tx(cryptonote::tx_to_blob(tx), tvc, cryptonote::relay_method::fluff, false));
  CHECK_TEST_CONDITION(!tvc.m_verifivation_failed);
  return true;
}
