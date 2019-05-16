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

#include "ringct/rctSigs.h"
#include "ringct/bulletproofs.h"
#include "chaingen.h"
#include "bulletproofs.h"
#include "device/device.hpp"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_bp_tx_validation_base::generate_with(std::vector<test_event_entry>& events,
    size_t n_txes, const uint64_t *amounts_paid, bool valid, const rct::RCTConfig *rct_config,
    const std::function<bool(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations, size_t tx_idx)> &pre_tx,
    const std::function<bool(transaction &tx, size_t tx_idx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  // create 12 miner accounts, and have them mine the next 48 blocks
  int const NUM_MINERS = 12;

  cryptonote::account_base miner_accounts[NUM_MINERS];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[NUM_UNLOCKED_BLOCKS + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW];

  for (size_t i = 0; i < NUM_MINERS; ++i)
    miner_accounts[i].generate();

  generator.m_hf_version = 8;
  for (size_t n = 0; n < NUM_UNLOCKED_BLOCKS; ++n) {
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, miner_accounts[n % NUM_MINERS],
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        8, 8, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
    LOG_PRINT_L0("Initial miner tx " << n << ": " << obj_to_json_str(blocks[n].miner_tx));
  }

  // rewind
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[NUM_UNLOCKED_BLOCKS - 1];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[NUM_UNLOCKED_BLOCKS + i], blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          8, 8, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
          false, "Failed to generate block");
      events.push_back(blocks[NUM_UNLOCKED_BLOCKS+i]);
      blk_last = blocks[NUM_UNLOCKED_BLOCKS+i];
    }
    blk_r = blk_last;
  }

  // NOTE(loki): Submit one more block. On the fork height, we allow exactly the
  // forking block to contain borromean TX's, due to some clients constructing
  // old style TX's on the fork height. So make sure we create one block so that
  // the block containing bulletproofs txes, which is 1 block after the fork
  // height, is tested with BP logic
  generator.m_hf_version = cryptonote::network_version_11_infinite_staking;
  {
    block blk;
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        generator.m_hf_version, generator.m_hf_version, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
        false, "Failed to generate block");
    events.push_back(blk);
    blk_last = blk;
  }

  std::vector<transaction> rct_txes;
  cryptonote::block blk_txes;
  std::vector<crypto::hash> starting_rct_tx_hashes;

  for (size_t n = 0, block_index = 0; n < n_txes; ++n)
  {
    std::vector<tx_source_entry> sources;
    std::vector<tx_destination_entry> destinations;

    cryptonote::account_base const &from = miner_accounts[n];
    cryptonote::account_base const &to   = miner_accounts[n+1];
    assert(n + 1 < NUM_MINERS);

    // NOTE: Monero tests use multiple null terminated entries in their arrays
    int amounts_paid_len = 0;
    for(int i = 0; amounts_paid[i] != (uint64_t)-1; ++i) ++amounts_paid_len;

    uint64_t change_amount;
    fill_tx_sources_and_multi_destinations(events,
                                           blk_last,
                                           from,
                                           to.get_keys().m_account_address,
                                           amounts_paid,
                                           amounts_paid_len,
                                           TESTS_DEFAULT_FEE,
                                           CRYPTONOTE_DEFAULT_TX_MIXIN,
                                           sources,
                                           destinations,
                                           true,
                                           &change_amount);

    tx_destination_entry change_addr{change_amount, from.get_keys().m_account_address, false /* is subaddr */ };

    // NOTE(loki): Monero tests presume the generated TX doesn't have change so remove it from our output.
    for (auto it = destinations.begin(); it != destinations.end(); ++it)
    {
      if (it->amount != change_amount) continue;
      destinations.erase(it);
      break;
    }

    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[from.get_keys().m_account_address.m_spend_public_key] = {0,0};

    std::vector<crypto::secret_key> additional_tx_keys;
    cryptonote::transaction tx;
    crypto::secret_key private_tx_key;

    if (pre_tx && !pre_tx(sources, destinations, n))
    {
      MDEBUG("pre_tx returned failure");
      return false;
    }

    loki_construct_tx_params tx_params(generator.m_hf_version);
    if (!cryptonote::construct_tx_and_get_tx_key(
        from.get_keys(),
        subaddresses,
        sources,
        destinations,
        change_addr,
        {} /*tx_extra*/,
        tx,
        0 /*unlock_time*/,
        private_tx_key,
        additional_tx_keys,
        rct_config[n],
        nullptr, /*multisig_out*/
        tx_params))
    {
      MDEBUG("construct_tx_and_get_tx_key failure");
      return false;
    }

    rct_txes.push_back(tx);
    if (post_tx && !post_tx(rct_txes.back(), n))
    {
      MDEBUG("post_tx returned failure");
      return false;
    }

    starting_rct_tx_hashes.push_back(get_transaction_hash(rct_txes.back()));
    LOG_PRINT_L0("Test tx: " << obj_to_json_str(rct_txes.back()));

    uint64_t total_amount_encoded = 0;
    for (int o = 0; amounts_paid[o] != (uint64_t)-1; ++o)
    {
      crypto::key_derivation derivation;
      bool r = crypto::generate_key_derivation(destinations[o].addr.m_view_public_key, private_tx_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      crypto::secret_key amount_key;
      crypto::derivation_to_scalar(derivation, o, amount_key);
      rct::key rct_tx_mask;

      uint64_t amount = 0;
      const uint8_t type = rct_txes.back().rct_signatures.type;
      if (type == rct::RCTTypeSimple || type == rct::RCTTypeBulletproof || type == rct::RCTTypeBulletproof2)
        amount = rct::decodeRctSimple(rct_txes.back().rct_signatures, rct::sk2rct(amount_key), o, rct_tx_mask, hw::get_device("default"));
      else
        amount = rct::decodeRct(rct_txes.back().rct_signatures, rct::sk2rct(amount_key), o, rct_tx_mask, hw::get_device("default"));

      total_amount_encoded += amount;
    }

    uint64_t expected_amount_encoded = 0;
    while (amounts_paid[0] != (size_t)-1)
      expected_amount_encoded += *amounts_paid++;
    ++amounts_paid;

    CHECK_AND_ASSERT_MES(expected_amount_encoded == total_amount_encoded, false, "Decoded rct did not match amount to pay");
  }

  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(rct_txes);

  CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk_txes, blk_last, miner_account,
      test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version,
      generator.m_hf_version, generator.m_hf_version, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
      crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0),
      false, "Failed to generate block");
  if (!valid)
    DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_txes);
  blk_last = blk_txes;

  return true;
}

bool gen_bp_tx_validation_base::check_bp(const cryptonote::transaction &tx, size_t tx_idx, const size_t *sizes, const char *context) const
{
  DEFINE_TESTS_ERROR_CONTEXT(context);
  CHECK_TEST_CONDITION(tx.version >= 2);
  CHECK_TEST_CONDITION(rct::is_rct_bulletproof(tx.rct_signatures.type));
  size_t n_sizes = 0, n_amounts = 0;
  for (size_t n = 0; n < tx_idx; ++n)
  {
    while (sizes[0] != (size_t)-1)
      ++sizes;
    ++sizes;
  }
  while (sizes[n_sizes] != (size_t)-1)
    n_amounts += sizes[n_sizes++];
  CHECK_TEST_CONDITION(tx.rct_signatures.p.bulletproofs.size() == n_sizes);
  CHECK_TEST_CONDITION(rct::n_bulletproof_max_amounts(tx.rct_signatures.p.bulletproofs) == n_amounts);
  for (size_t n = 0; n < n_sizes; ++n)
    CHECK_TEST_CONDITION(rct::n_bulletproof_max_amounts(tx.rct_signatures.p.bulletproofs[n]) == sizes[n]);
  return true;
}

// TODO(doyle): Revisit this. Is there some rule prohibiting a tx fee greater
// than the block reward? Monero is unaffected because they have multiple
// outputs of varying sizes in their miner tx, so the tx fee (inputs-outputs)
// (because they don't use a change addr) doesn't eclipse the reward and doesn't
// trigger the "base reward calculation bug" assert, whereas we do since we only
// have 1 output. So my fix is to make it so we don't generate a tx that makes
// too high of a fee from the change amount.

// Further addendum. In Loki hardfork 10, we also introduce batching governance
// payments- so most block heights will remove the governance output from the
// reward. So if we send less than the governance amount (~6ish loki from the
// start of the chain), then we'll eclipse the reward again and overflow, so
// most of these tests have again been modified to ensure that we use atleast
// 6 loki from the block reward.
//  - 2018/10/29

bool gen_bp_tx_valid_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {MK_COINS(10), (uint64_t)-1};
  const size_t bp_sizes[] = {1, (size_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofPaddedBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx){ return check_bp(tx, tx_idx, bp_sizes, "gen_bp_tx_valid_1"); });
}

bool gen_bp_tx_invalid_1_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {5, 5, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBulletproof , 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, NULL);
}

bool gen_bp_tx_valid_2::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {MK_COINS(5), MK_COINS(5), (uint64_t)-1};
  const size_t bp_sizes[] = {2, (size_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofPaddedBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx){ return check_bp(tx, tx_idx, bp_sizes, "gen_bp_tx_valid_2"); });
}

bool gen_bp_tx_valid_3::generate(std::vector<test_event_entry>& events) const
{
  // const uint64_t amounts_paid[] = {50, 50, 50, (uint64_t)-1};
  const uint64_t amounts_paid[] = {MK_COINS(28), MK_COINS(28), MK_COINS(28), (uint64_t)-1};
  const size_t bp_sizes[] = {4, (size_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofPaddedBulletproof , 0 } };
  return generate_with(events, 1, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx){ return check_bp(tx, tx_idx, bp_sizes, "gen_bp_tx_valid_3"); });
}

bool gen_bp_tx_valid_16::generate(std::vector<test_event_entry>& events) const
{
  // const uint64_t amounts_paid[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, (uint64_t)-1};
  const uint64_t amounts_paid[] = {MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), MK_COINS(1), (uint64_t)-1};
  const size_t bp_sizes[] = {16, (size_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofPaddedBulletproof , 0 } };
  return generate_with(events, 1, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx){ return check_bp(tx, tx_idx, bp_sizes, "gen_bp_tx_valid_16"); });
}

bool gen_bp_tx_invalid_4_2_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {1, 1, 1, 1, 1, 1, 1, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofMultiOutputBulletproof , 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, NULL);
}

bool gen_bp_tx_invalid_16_16::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofMultiOutputBulletproof , 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, NULL);
}

bool gen_bp_txs_valid_2_and_2::generate(std::vector<test_event_entry>& events) const
{
  //const uint64_t amounts_paid[] = {1000, 1000, (size_t)-1, 1000, 1000, (uint64_t)-1};
  const uint64_t amounts_paid[] = {MK_COINS(50), MK_COINS(50), (size_t)-1, MK_COINS(50), MK_COINS(50), (uint64_t)-1};

  const size_t bp_sizes[] = {2, (size_t)-1, 2, (size_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofPaddedBulletproof, 0 }, {rct::RangeProofPaddedBulletproof, 0 } };
  return generate_with(events, 2, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx){ return check_bp(tx, tx_idx, bp_sizes, "gen_bp_txs_valid_2_and_2"); });
}

bool gen_bp_txs_invalid_2_and_8_2_and_16_16_1::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {1, 1, (uint64_t)-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, (uint64_t)-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = {{rct::RangeProofMultiOutputBulletproof, 0}, {rct::RangeProofMultiOutputBulletproof, 0}, {rct::RangeProofMultiOutputBulletproof, 0}};
  return generate_with(events, 3, amounts_paid, false, rct_config, NULL, NULL);
}

bool gen_bp_txs_valid_2_and_3_and_2_and_4::generate(std::vector<test_event_entry>& events) const
{
  // TODO(doyle): See valid_2_and_2 comment
  // const uint64_t amounts_paid[] = {11111115000, 11111115000, (uint64_t)-1, 11111115000, 11111115000, 11111115001, (uint64_t)-1, 11111115000, 11111115002, (uint64_t)-1, 11111115000, 11111115000, 11111115000, 11111115003, (uint64_t)-1};
  const uint64_t amounts_paid[] = {MK_COINS(50), MK_COINS(50), (uint64_t)-1, MK_COINS(5), MK_COINS(50), MK_COINS(51), (uint64_t)-1, MK_COINS(50), MK_COINS(52), (uint64_t)-1, MK_COINS(5), MK_COINS(50), MK_COINS(53), MK_COINS(6), (uint64_t)-1};

  const rct::RCTConfig rct_config[] = {{rct::RangeProofPaddedBulletproof, 0}, {rct::RangeProofPaddedBulletproof, 0}, {rct::RangeProofPaddedBulletproof, 0}, {rct::RangeProofPaddedBulletproof, 0}};
  const size_t bp_sizes[] = {2, (size_t)-1, 4, (size_t)-1, 2, (size_t)-1, 4, (size_t)-1};
  return generate_with(events, 4, amounts_paid, true, rct_config, NULL, [&](const cryptonote::transaction &tx, size_t tx_idx) { return check_bp(tx, tx_idx, bp_sizes, "gen_bp_txs_valid_2_and_3_and_2_and_4"); });
}

bool gen_bp_tx_invalid_not_enough_proofs::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_bp_tx_invalid_not_enough_proofs");
  const uint64_t amounts_paid[] = {5, 5, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, [&](cryptonote::transaction &tx, size_t idx){
    CHECK_TEST_CONDITION(tx.rct_signatures.type == rct::RCTTypeBulletproof || tx.rct_signatures.type == rct::RCTTypeBulletproof2);
    CHECK_TEST_CONDITION(!tx.rct_signatures.p.bulletproofs.empty());
    tx.rct_signatures.p.bulletproofs.pop_back();
    CHECK_TEST_CONDITION(!tx.rct_signatures.p.bulletproofs.empty());
    return true;
  });
}

bool gen_bp_tx_invalid_empty_proofs::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_bp_tx_invalid_empty_proofs");
  const uint64_t amounts_paid[] = {50, 50, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, [&](cryptonote::transaction &tx, size_t idx){
    CHECK_TEST_CONDITION(tx.rct_signatures.type == rct::RCTTypeBulletproof || tx.rct_signatures.type == rct::RCTTypeBulletproof2);
    tx.rct_signatures.p.bulletproofs.clear();
    return true;
  });
}

bool gen_bp_tx_invalid_too_many_proofs::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_bp_tx_invalid_too_many_proofs");
  const uint64_t amounts_paid[] = {10000, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, [&](cryptonote::transaction &tx, size_t idx){
    CHECK_TEST_CONDITION(tx.rct_signatures.type == rct::RCTTypeBulletproof || tx.rct_signatures.type == rct::RCTTypeBulletproof2);
    CHECK_TEST_CONDITION(!tx.rct_signatures.p.bulletproofs.empty());
    tx.rct_signatures.p.bulletproofs.push_back(tx.rct_signatures.p.bulletproofs.back());
    return true;
  });
}

bool gen_bp_tx_invalid_wrong_amount::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_bp_tx_invalid_wrong_amount");
  const uint64_t amounts_paid[] = {10, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBulletproof, 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, [&](cryptonote::transaction &tx, size_t idx){
    CHECK_TEST_CONDITION(tx.rct_signatures.type == rct::RCTTypeBulletproof || tx.rct_signatures.type == rct::RCTTypeBulletproof2);
    CHECK_TEST_CONDITION(!tx.rct_signatures.p.bulletproofs.empty());
    tx.rct_signatures.p.bulletproofs.back() = rct::bulletproof_PROVE(1000, rct::skGen());
    return true;
  });
}

bool gen_bp_tx_invalid_borromean_type::generate(std::vector<test_event_entry>& events) const
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_bp_tx_invalid_borromean_type");
  const uint64_t amounts_paid[] = {5, 5, (uint64_t)-1};
  const rct::RCTConfig rct_config[] = { { rct::RangeProofBorromean, 0 } };
  return generate_with(events, 1, amounts_paid, false, rct_config, NULL, [&](cryptonote::transaction &tx, size_t tx_idx){
    return true;
  });
}
