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
    size_t n_txes, const uint64_t *amounts_paid, bool valid, uint8_t hf_version,
    const FcmpPpFromTxType spend_tx_type,
    const std::function<bool(transaction &tx, size_t tx_idx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(genesis_account);
  MAKE_GENESIS_BLOCK(events, blk_0, genesis_account, ts_start);
  cryptonote::block blk_last = blk_0;

  // mine the next 16 blocks to a new miner
  cryptonote::account_base miner_account;
  miner_account.generate();
  const std::size_t n_manual_blocks = 16 + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW + 2 + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
  cryptonote::block blocks[n_manual_blocks];
  std::size_t cur_block_idx = 0;
  for (size_t n = 0; n < 16; ++n) {
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[cur_block_idx], blk_last, miner_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
        false, "Failed to generate block");
    events.push_back(blocks[cur_block_idx]);
    blk_last = blocks[cur_block_idx];
    ++cur_block_idx;
  }

  // mine CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW blocks so the above is spendable
  {
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[cur_block_idx], blk_last, genesis_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
          false, "Failed to generate block");
      events.push_back(blocks[cur_block_idx]);
      blk_last = blocks[cur_block_idx];
      ++cur_block_idx;
    }
  }

  // Create a regular pre-RCT tx to spend from
  cryptonote::transaction pre_rct_tx;
  const uint64_t pre_rct_tx_src_amount = 30000000000000;
  const uint64_t pre_rct_tx_dest_amount = 5000000000000;
  {
    std::vector<tx_source_entry> sources;

    sources.resize(1);
    tx_source_entry& src = sources.back();

    const size_t index_in_tx = blocks[1].miner_tx.vout.size() - 1;
    src.amount = pre_rct_tx_src_amount;
    for (int m = 1; m < 4; ++m) {
      crypto::public_key output_public_key = crypto::null_pkey;
      for (const auto &out : blocks[m].miner_tx.vout)
      {
        if (out.amount != src.amount)
          continue;
        cryptonote::get_output_public_key(out, output_public_key);
        break;
      }
      CHECK_AND_ASSERT_MES(output_public_key != crypto::null_pkey, false, "no out with expected amount");
      src.push_output(m, output_public_key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[1].miner_tx);
    src.real_output = 0;
    src.real_output_in_tx_index = index_in_tx;
    src.mask = rct::identity();
    src.rct = false;

    //fill outputs entry
    tx_destination_entry td;
    td.addr = miner_account.get_keys().m_account_address;
    td.amount = pre_rct_tx_dest_amount;
    std::vector<tx_destination_entry> destinations;
    destinations.push_back(td);
    destinations.push_back(td);

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddrs{{ miner_account.get_keys().m_account_address.m_spend_public_key, {0,0} }};
    bool r = construct_tx_and_get_tx_key(miner_account.get_keys(), subaddrs, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), pre_rct_tx, tx_key, additional_tx_keys, false/*rct*/, {});
    CHECK_AND_ASSERT_MES(r, false, "failed to construct pre-RCT transaction");
    events.push_back(pre_rct_tx);

    // Mine the tx
    uint64_t fee = 0;
    get_tx_fee(pre_rct_tx, fee);

    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[cur_block_idx], blk_last, genesis_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
        2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), {get_transaction_hash(pre_rct_tx)}, 0, 6, 2, fee),
        false, "Failed to generate block");
    events.push_back(blocks[cur_block_idx]);
    blk_last = blocks[cur_block_idx];
    ++cur_block_idx;
  }
  const std::size_t pre_rct_tx_block_idx = cur_block_idx;

  // Create a BP+ tx to spend from
  cryptonote::transaction bpp_tx;
  rct::key bpp_tx_masks[2];
  const uint64_t bpp_tx_src_amount = 5000000000000;
  const uint64_t bpp_tx_dest_amount = bpp_tx_src_amount / 3;
  {
    std::vector<tx_source_entry> sources;

    sources.resize(1);
    tx_source_entry& src = sources.back();

    const size_t index_in_tx = 4;
    src.amount = bpp_tx_src_amount;
    for (int m = 0; m < 16; ++m) {
      crypto::public_key output_public_key = crypto::null_pkey;
      for (const auto &out : blocks[m].miner_tx.vout)
      {
        if (out.amount != src.amount)
          continue;
        cryptonote::get_output_public_key(out, output_public_key);
        break;
      }
      CHECK_AND_ASSERT_MES(output_public_key != crypto::null_pkey, false, "no out with expected amount");
      src.push_output(m, output_public_key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[0].miner_tx);
    src.real_output = 0;
    src.real_output_in_tx_index = index_in_tx;
    src.mask = rct::identity();
    src.rct = false;

    //fill outputs entry
    tx_destination_entry td;
    td.addr = miner_account.get_keys().m_account_address;
    td.amount = bpp_tx_dest_amount;
    std::vector<tx_destination_entry> destinations;
    destinations.push_back(td);
    destinations.push_back(td);

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddrs{{ miner_account.get_keys().m_account_address.m_spend_public_key, {0,0} }};
    const rct::RCTConfig rct_config{ rct::RangeProofPaddedBulletproof, 4 };
    bool r = construct_tx_and_get_tx_key(miner_account.get_keys(), subaddrs, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), bpp_tx, tx_key, additional_tx_keys, true, rct_config);
    CHECK_AND_ASSERT_MES(r, false, "failed to construct bp+ transaction");
    events.push_back(bpp_tx);

    // Collect output mask from BP+ tx
    for (size_t o = 0; o < 2; ++o)
    {
      crypto::key_derivation derivation;
      bool r = crypto::generate_key_derivation(destinations[o].addr.m_view_public_key, tx_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      crypto::secret_key amount_key;
      crypto::derivation_to_scalar(derivation, o, amount_key);
      rct::decodeRctSimple(bpp_tx.rct_signatures, rct::sk2rct(amount_key), o, bpp_tx_masks[o], hw::get_device("default"));
    }

    // Mine the tx
    uint64_t fee = 0;
    get_tx_fee(bpp_tx, fee);

    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[cur_block_idx], blk_last, genesis_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
        HF_VERSION_BULLETPROOF_PLUS, HF_VERSION_BULLETPROOF_PLUS, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), {get_transaction_hash(bpp_tx)}, 0, 6, HF_VERSION_BULLETPROOF_PLUS, fee),
        false, "Failed to generate block");
    events.push_back(blocks[cur_block_idx]);
    blk_last = blocks[cur_block_idx];
    ++cur_block_idx;
  }
  const std::size_t bpp_block_idx = cur_block_idx;

  // mine CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW blocks so the above txs are spendable
  {
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[cur_block_idx], blk_last, genesis_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          HF_VERSION_BULLETPROOF_PLUS, HF_VERSION_BULLETPROOF_PLUS, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, HF_VERSION_BULLETPROOF_PLUS),
          false, "Failed to generate block");
      events.push_back(blocks[cur_block_idx]);
      blk_last = blocks[cur_block_idx];
      ++cur_block_idx;
    }
  }

  // Select the "input" tx we're going to spend and other necessary tx construction params
  cryptonote::transaction input_tx;
  uint64_t internal_o_idx;
  uint64_t global_o_idx;
  uint64_t spend_amount;
  rct::key mask;
  switch (spend_tx_type)
  {
    case FcmpPpFromTxType::PreRctCoinbase:
    {
      input_tx = blocks[0].miner_tx;
      CHECK_AND_ASSERT_MES(input_tx.version == 1, false, "expected tx version 1");
      CHECK_AND_ASSERT_MES(cryptonote::is_coinbase(input_tx), false, "expected coinbase");

      internal_o_idx = blocks[0].miner_tx.vout.size() - 1;
      global_o_idx = internal_o_idx;
      spend_amount = input_tx.vout.at(internal_o_idx).amount;
      mask = rct::identity();

      break;
    }
    case FcmpPpFromTxType::PreRctRegular:
    {
      input_tx = pre_rct_tx;
      CHECK_AND_ASSERT_MES(input_tx.version == 1, false, "expected tx version 1");
      CHECK_AND_ASSERT_MES(!cryptonote::is_coinbase(input_tx), false, "expected regular (non-coinbase) tx");

      internal_o_idx = 0;
      global_o_idx = 0; // ok to be incorrect
      spend_amount = pre_rct_tx_dest_amount;
      mask = rct::identity();

      break;
    }
    case FcmpPpFromTxType::BulletproofPlus:
    {
      input_tx = bpp_tx;
      CHECK_AND_ASSERT_MES(input_tx.rct_signatures.type == rct::RCTTypeBulletproofPlus, false, "expected BP+ tx type");

      internal_o_idx = 0;
      global_o_idx = 0; // ok to be incorrect
      spend_amount = bpp_tx_dest_amount;
      mask = bpp_tx_masks[internal_o_idx];

      break;
    }
    default:
    {
      MERROR("unexpected spend tx type");
      return false;
    }
  }

  // Register the output we're spending with the TreeCache to know its location in the tree
  TreeCacheV1 tree_cache(fcmp_pp::curve_trees::curve_trees_v1());
  const auto &spending_out = input_tx.vout.at(internal_o_idx);
  const auto &output_pubkey = boost::get<txout_to_key>(spending_out.target).key;
  const rct::key C = (input_tx.version >= 2 && !cryptonote::is_coinbase(input_tx))
    ? input_tx.rct_signatures.outPk.at(internal_o_idx).mask
    : rct::zeroCommitVartime(spending_out.amount);
  CHECK_AND_ASSERT_MES(C == rct::commit(spend_amount, mask), false, "commitment mismatch");
  const fcmp_pp::curve_trees::OutputPair output_pair{.output_pubkey = output_pubkey, .commitment = C};
  tree_cache.register_output(output_pair);

  // Prepare to build the FCMP++ curve tree
  std::vector<crypto::hash> new_block_hashes;
  std::unordered_map<uint64_t, rct::key> transparent_amount_commitments;
  std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> outs_by_last_locked_blocks;
  uint64_t first_output_id = 0;
  for (uint64_t blk_idx = 0; blk_idx < (1 + n_manual_blocks); ++blk_idx)
  {
    const auto &blk = blk_idx == 0 ? blk_0 : blocks[blk_idx - 1];
    new_block_hashes.push_back(blk.hash);
    const auto txs =
      blk_idx == pre_rct_tx_block_idx ? std::vector<transaction>{pre_rct_tx}
      : blk_idx == bpp_block_idx ? std::vector<transaction>{bpp_tx}
      : std::vector<transaction>{};
    const auto tx_refs = cryptonote::collect_transparent_amount_commitments(blk.miner_tx, txs, transparent_amount_commitments);
    auto outs_meta = cryptonote::get_outs_by_last_locked_block(tx_refs, transparent_amount_commitments, first_output_id, blk_idx);
    outs_by_last_locked_blocks.emplace_back(std::move(outs_meta.outs_by_last_locked_block));
    first_output_id = outs_meta.next_output_id;
  }

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
  std::vector<crypto::hash> starting_rct_tx_hashes;
  uint64_t fees = 0;
  rct_txes.resize(1);

  // Generate key image
  const crypto::public_key in_tx_pub_key = cryptonote::get_tx_pub_key_from_extra(input_tx);
  const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddrs = {{ miner_account.get_keys().m_account_address.m_spend_public_key, {0,0} }};
  cryptonote::keypair in_ephemeral;
  crypto::key_image key_image;
  bool r = cryptonote::generate_key_image_helper(
      miner_account.get_keys(),
      subaddrs,
      output_pubkey,
      in_tx_pub_key,
      {},
      internal_o_idx,
      in_ephemeral,
      key_image,
      hw::get_device("default"));
  CHECK_AND_ASSERT_MES(r, false, "Failed to generate key image");

  // Source
  const wallet2_basic::transfer_details wallet2_td{
      .m_block_height = 0,
      .m_tx = input_tx,
      .m_txid = cryptonote::get_transaction_hash(input_tx),
      .m_internal_output_index = internal_o_idx,
      .m_global_output_index = global_o_idx,
      .m_spent = false,
      .m_frozen = false,
      .m_spent_height = 0,
      .m_key_image = key_image,
      .m_mask = mask,
      .m_amount = spend_amount,
      .m_rct = input_tx.version >= 2,
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
  td.addr = miner_account.get_keys().m_account_address;
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

  rct_txes.back() = tools::wallet::finalize_all_fcmp_pp_proofs(tx_proposals.front(),
      tree_cache,
      *fcmp_pp::curve_trees::curve_trees_v1(),
      miner_account.get_keys());

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

  cryptonote::block tx_block;
  CHECK_AND_ASSERT_MES(generator.construct_block_manually(tx_block, blk_last, genesis_account,
      test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
      hf_version, hf_version, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
      crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0, 6, hf_version, fees, fcmp_pp_n_tree_layers, fcmp_pp_tree_root),
      false, "Failed to generate block");
  if (!valid)
    DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(tx_block);
  blk_last = tx_block;

  return true;
}

bool gen_fcmp_pp_tx_from_pre_rct_coinbase::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {10000, (uint64_t)-1};
  return generate_with(events, 1/*n_txes*/, amounts_paid, true/*valid*/, HF_VERSION_FCMP_PLUS_PLUS, FcmpPpFromTxType::PreRctCoinbase, NULL);
}

bool gen_fcmp_pp_tx_from_pre_rct_regular::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {10000, (uint64_t)-1};
  return generate_with(events, 1/*n_txes*/, amounts_paid, true/*valid*/, HF_VERSION_FCMP_PLUS_PLUS, FcmpPpFromTxType::PreRctRegular, NULL);
}

bool gen_fcmp_pp_tx_from_bpp::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  return generate_with(events, 1/*n_txes*/, amounts_paid, true/*valid*/, HF_VERSION_FCMP_PLUS_PLUS, FcmpPpFromTxType::BulletproofPlus, NULL);
}
