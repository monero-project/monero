// Copyright (c) 2014-2024, The Monero Project
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

#include "fcmp_pp.h"

#include "ringct/rctSigs.h"
#include "ringct/bulletproofs_plus.h"
#include "chaingen.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/tree_cache.h"
#include "device/device.hpp"
#include "wallet/tx_builder.h"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

using Selene = fcmp_pp::curve_trees::Selene;
using Helios = fcmp_pp::curve_trees::Helios;
using TreeCacheV1 = fcmp_pp::curve_trees::TreeCache<Selene, Helios>;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_fcmp_pp_tx_validation_base::generate_with(std::vector<test_event_entry>& events,
    size_t n_txes, const uint64_t *amounts_paid, bool valid, const rct::RCTConfig &rct_config, uint8_t hf_version,
    const std::function<bool(transaction &tx, size_t tx_idx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);


  // create 12 miner accounts, and have them mine the next 12 blocks
  cryptonote::account_base miner_accounts[12];
  const cryptonote::block *prev_block = &blk_0;
  const std::size_t n_manual_blocks = 12 + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
  cryptonote::block blocks[n_manual_blocks];
  for (size_t n = 0; n < 12; ++n) {
    miner_accounts[n].generate();
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, miner_accounts[n],
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        2, 2, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
  }

  // mine CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW blocks so the above is spendable
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[11];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[12+i], blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
          false, "Failed to generate block");
      events.push_back(blocks[12+i]);
      blk_last = blocks[12+i];
    }
    blk_r = blk_last;
  }

  // Build the FCMP++ curve tree
  TreeCacheV1 tree_cache(fcmp_pp::curve_trees::curve_trees_v1());
  std::vector<crypto::hash> new_block_hashes;
  std::unordered_map<uint64_t, rct::key> transparent_amount_commitments;
  std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> outs_by_last_locked_blocks;
  uint64_t first_output_id = 0;
  for (uint64_t blk_idx = 0; blk_idx < (1 + n_manual_blocks); ++blk_idx)
  {
    const auto &blk = blk_idx == 0 ? blk_0 : blocks[blk_idx - 1];
    new_block_hashes.push_back(blk.hash);
    const auto tx_refs = cryptonote::collect_transparent_amount_commitments(blk.miner_tx, std::vector<transaction>{}, transparent_amount_commitments);
    auto outs_meta = cryptonote::get_outs_by_last_locked_block(tx_refs, transparent_amount_commitments, first_output_id, blk_idx);
    outs_by_last_locked_blocks.emplace_back(std::move(outs_meta.outs_by_last_locked_block));
    first_output_id = outs_meta.next_output_id;
  }

  // We're going to spend the last output in the first block
  // Note: I'm using the last otuput because first output doesn't have enough to cover min fee without changes to fee code
  // TODO: change the fee checking code for FCMP++
  const auto &spending_out = blocks[0].miner_tx.vout.back();
  const auto o_idx = blocks[0].miner_tx.vout.size() - 1;

  // Register the output with the TreeCache to know its location in the tree
  const auto &output_pubkey = boost::get<txout_to_key>(spending_out.target).key;
  const rct::key C = rct::zeroCommitVartime(spending_out.amount);
  const fcmp_pp::curve_trees::OutputPair output_pair{.output_pubkey = output_pubkey, .commitment = C};
  tree_cache.register_output(output_pair, cryptonote::get_last_locked_block_index(blocks[0].miner_tx.unlock_time, 0));

  // Build the tree, keeping track of output's path in the tree
  fcmp_pp::curve_trees::TreeCacheV1::CacheStateChange tree_cache_state_change;
  tree_cache.prepare_to_grow_cache(0, {}, new_block_hashes, outs_by_last_locked_blocks, tree_cache_state_change);
  tree_cache.grow_cache(0, new_block_hashes, std::move(tree_cache_state_change));
  const uint64_t n_synced_blocks = tree_cache.n_synced_blocks();
  if (n_synced_blocks == 0)
  {
    MDEBUG("n_synced_blocks == 0");
    return false;
  }

  // create 1 tx in another block, spending from the first block
  std::vector<transaction> rct_txes;
  cryptonote::block blk_txes;
  std::vector<crypto::hash> starting_rct_tx_hashes;
  uint64_t fees = 0;
  rct_txes.resize(rct_txes.size() + 1);

  // Generate key image
  const crypto::public_key in_tx_pub_key = cryptonote::get_tx_pub_key_from_extra(blocks[0].miner_tx);
  const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddrs{{ miner_accounts[0].get_keys().m_account_address.m_spend_public_key, {0,0} }};
  cryptonote::keypair in_ephemeral;
  crypto::key_image key_image;
  bool r = cryptonote::generate_key_image_helper(
      miner_accounts[0].get_keys(),
      subaddrs,
      output_pubkey,
      in_tx_pub_key,
      {},
      o_idx,
      in_ephemeral,
      key_image,
      hw::get_device("default"));
  CHECK_AND_ASSERT_MES(r, false, "Failed to generate key image");

  // Source
  const wallet2_basic::transfer_details wallet2_td{
      .m_block_height = 0,
      .m_tx = blocks[0].miner_tx,
      .m_txid = blocks[0].hash,
      .m_internal_output_index = o_idx,
      .m_global_output_index = o_idx,
      .m_spent = false,
      .m_frozen = false,
      .m_spent_height = 0,
      .m_key_image = key_image,
      .m_mask = rct::identity(),
      .m_amount = spending_out.amount,
      .m_rct = false,
      .m_key_image_known = true,
      .m_key_image_request = false,
      .m_pk_index = 0,
      .m_subaddr_index = {},
      .m_key_image_partial = false,
      .m_multisig_k = {},
      .m_multisig_info = {},
      .m_uses = {},
    };

  //fill outputs entry
  tx_destination_entry td;
  td.addr = miner_accounts[0].get_keys().m_account_address;
  std::vector<tx_destination_entry> destinations;
  for (int o = 0; amounts_paid[o] != (uint64_t)-1; ++o)
  {
    td.amount = amounts_paid[o];
    destinations.push_back(td);
  }

  const auto tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_transfer(
      {wallet2_td},
      subaddrs,
      destinations,
      /*fee_per_weight=*/10000000, // This is just a mock value to pass the test
      /*extra=*/{},
      /*subaddr_account=*/0,
      /*subaddr_indices=*/{},
      /*ignore_above=*/MONEY_SUPPLY,
      /*ignore_below=*/0,
      {},
      n_synced_blocks - 1);
  CHECK_AND_ASSERT_MES(tx_proposals.size() == 1, false, "Expected 1 tx proposal");

  rct_txes.back() = tools::wallet::finalize_all_proofs_from_transfer_details(tx_proposals.front(),
      {wallet2_td},
      tree_cache,
      *fcmp_pp::curve_trees::curve_trees_v1(),
      miner_accounts[0].get_keys());

  if (post_tx && !post_tx(rct_txes.back(), 0))
  {
    MDEBUG("post_tx returned failure");
    return false;
  }

  //events.push_back(rct_txes.back());
  starting_rct_tx_hashes.push_back(get_transaction_hash(rct_txes.back()));
  LOG_PRINT_L0("Test tx: " << obj_to_json_str(rct_txes.back()));

  uint64_t fee = 0;
  get_tx_fee(rct_txes.back(), fee);
  fees += fee;

  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(rct_txes);

  // To calculate the tree root needed for the block header,
  // we sync empty blocks so all outputs staged for insertion in the cache
  // enter the tree. This assumes assumes we don't need the tree cache after
  // this. We can call pop_block after reading the root to reverse this
  // if we need the cache to be in the correct state after this op.
  const uint64_t new_block_idx = n_synced_blocks;
  tree_cache.sync_block(new_block_idx, crypto::hash{}, blocks[n_manual_blocks - 1].hash, {});
  uint64_t synced_blk_idx = new_block_idx;
  while (tree_cache.n_synced_blocks() < cryptonote::get_default_last_locked_block_index(new_block_idx))
    tree_cache.sync_block(++synced_blk_idx, crypto::hash{}, crypto::hash{}, {});

  crypto::ec_point fcmp_pp_tree_root;
  const uint8_t fcmp_pp_n_tree_layers = tree_cache.get_tree_root(fcmp_pp_tree_root);

  CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk_txes, blk_last, miner_account,
      test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
      hf_version, hf_version, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
      crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0, 6, hf_version, fees, fcmp_pp_n_tree_layers, fcmp_pp_tree_root),
      false, "Failed to generate block");
  if (!valid)
    DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_txes);
  blk_last = blk_txes;

  return true;
}

bool gen_fcmp_pp_tx_valid_at_fork::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with(events, 1, amounts_paid, true, rct_config, HF_VERSION_FCMP_PLUS_PLUS, NULL);
}

// TODO: verification
// TODO: spend from pre-RCT, post-RCT, and coinbase outputs
