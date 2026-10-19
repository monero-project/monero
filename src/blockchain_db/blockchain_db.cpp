// Copyright (c) 2014-2026, The Monero Project
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

#include <boost/range/adaptor/reversed.hpp>

#include "string_tools.h"
#include "blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "fcmp_pp/curve_trees.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"

#include "lmdb/db_lmdb.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain.db"

using epee::string_tools::pod_to_hex;

namespace cryptonote
{

bool matches_category(relay_method method, relay_category category) noexcept
{
  switch (category)
  {
    default:
      return false;
    case relay_category::all:
      return true;
    case relay_category::relayable:
      return method != relay_method::none;
    case relay_category::broadcasted:
    case relay_category::legacy:
      break;
  }
  // check for "broadcasted" or "legacy" methods:
  switch (method)
  {
    default:
    case relay_method::local:
    case relay_method::forward:
    case relay_method::stem:
      return false;
    case relay_method::block:
    case relay_method::fluff:
      return true;
    case relay_method::none:
      break;
  }
  return category == relay_category::legacy;
}

void txpool_tx_meta_t::set_relay_method(relay_method method) noexcept
{
  kept_by_block = 0;
  do_not_relay = 0;
  is_local = 0;
  is_forwarding = 0;
  dandelionpp_stem = 0;

  switch (method)
  {
    case relay_method::none:
      do_not_relay = 1;
      break;
    case relay_method::local:
      is_local = 1;
      break;
    case relay_method::forward:
      is_forwarding = 1;
      break;
    case relay_method::stem:
      dandelionpp_stem = 1;
      break;
    case relay_method::block:
      kept_by_block = 1;
      break;
    default:
    case relay_method::fluff:
      break;
  }
}

relay_method txpool_tx_meta_t::get_relay_method() const noexcept
{
  const uint8_t state =
    uint8_t(kept_by_block) +
    (uint8_t(do_not_relay) << 1) +
    (uint8_t(is_local) << 2) +
    (uint8_t(is_forwarding) << 3) +
    (uint8_t(dandelionpp_stem) << 4);

  switch (state)
  {
    default: // error case
    case 0:
      break;
    case 1:
      return relay_method::block;
    case 2:
      return relay_method::none;
    case 4:
      return relay_method::local;
    case 8:
      return relay_method::forward;
    case 16:
      return relay_method::stem;
  };
  return relay_method::fluff;
}

bool txpool_tx_meta_t::upgrade_relay_method(relay_method method) noexcept
{
  static_assert(relay_method::none < relay_method::local, "bad relay_method value");
  static_assert(relay_method::local < relay_method::forward, "bad relay_method value");
  static_assert(relay_method::forward < relay_method::stem, "bad relay_method value");
  static_assert(relay_method::stem < relay_method::fluff, "bad relay_method value");
  static_assert(relay_method::fluff < relay_method::block, "bad relay_method value");

  if (get_relay_method() < method)
  {
    set_relay_method(method);
    return true;
  }
  return false;
}

const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
  "db-sync-mode"
, "Specify sync option, using format [safe|fast|fastest]:[sync|async]:[<nblocks_per_sync>[blocks]|<nbytes_per_sync>[bytes]]." 
, "fast:async:250000000bytes"
};
const command_line::arg_descriptor<bool> arg_db_salvage  = {
  "db-salvage"
, "Try to salvage a blockchain database if it seems corrupted"
, false
};

BlockchainDB *new_db()
{
  return new BlockchainLMDB();
}

void BlockchainDB::init_options(boost::program_options::options_description& desc)
{
  command_line::add_arg(desc, arg_db_sync_mode);
  command_line::add_arg(desc, arg_db_salvage);
}

void BlockchainDB::add_transaction(const crypto::hash& blk_hash, const transaction& tx, epee::span<const std::uint8_t> blob, const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments, const crypto::hash* tx_hash_ptr, const crypto::hash* tx_prunable_hash_ptr)
{
  bool miner_tx = false;
  crypto::hash tx_hash, tx_prunable_hash;
  if (!tx_hash_ptr)
  {
    // should only need to compute hash for miner transactions
    tx_hash = get_transaction_hash(tx);
    LOG_PRINT_L3("null tx_hash_ptr - needed to compute: " << tx_hash);
  }
  else
  {
    tx_hash = *tx_hash_ptr;
  }
  if (tx.version >= 2)
  {
    const boost::string_ref ref{reinterpret_cast<const char*>(blob.data()), blob.size()};
    if (!tx_prunable_hash_ptr)
      tx_prunable_hash = get_transaction_prunable_hash(tx, &ref);
    else
      tx_prunable_hash = *tx_prunable_hash_ptr;
  }

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      add_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
    else if (tx_input.type() == typeid(txin_gen))
    {
      /* nothing to do here */
      miner_tx = true;
    }
    else
    {
      LOG_PRINT_L1("Unsupported input type, aborting transaction addition");
      throw std::runtime_error("Unexpected input type, aborting");
    }
  }

  uint64_t tx_id = add_transaction_data(blk_hash, tx, blob, tx_hash, tx_prunable_hash);

  std::vector<uint64_t> amount_output_indices(tx.vout.size());

  // iterate tx.vout using indices instead of C++11 foreach syntax because
  // we need the index
  for (uint64_t i = 0; i < tx.vout.size(); ++i)
  {
    // miner v2 txes have their coinbase output in one single out to save space,
    // and we store them as rct outputs with an identity mask
    if (miner_tx && tx.version == 2)
    {
      cryptonote::tx_out vout = tx.vout[i];
      const auto commitment_it = transparent_amount_commitments.find(vout.amount);
      if (commitment_it == transparent_amount_commitments.end())
        throw std::runtime_error("Failed to get miner tx commitment, aborting");
      vout.amount = 0;
      amount_output_indices[i] = add_output(tx_hash, vout, i, tx.unlock_time,
        &commitment_it->second);
    }
    else
    {
      amount_output_indices[i] = add_output(tx_hash, tx.vout[i], i, tx.unlock_time,
        tx.version > 1 ? &tx.rct_signatures.outPk[i].mask : NULL);
    }
  }
  add_tx_amount_output_indices(tx_id, amount_output_indices);
}

uint64_t BlockchainDB::add_block( const std::pair<block, blobdata>& blck
                                , size_t block_weight
                                , uint64_t long_term_block_weight
                                , const difficulty_type& cumulative_difficulty
                                , const uint64_t& coins_generated
                                , const std::vector<std::pair<transaction, blobdata>>& txs
                                , const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments
                                )
{
  const block &blk = blck.first;

  // sanity
  if (blk.tx_hashes.size() != txs.size())
    throw std::runtime_error("Inconsistent tx/hashes sizes");

  TIME_MEASURE_START(time1);
  crypto::hash blk_hash = get_block_hash(blk);
  TIME_MEASURE_FINISH(time1);
  time_blk_hash += time1;

  const uint64_t blk_idx = height();
  const uint64_t first_unified_id = total_outputs(); // Call this before adding this block's outputs

  // call out to add the transactions

  time1 = epee::misc_utils::get_tick_count();

  uint64_t num_rct_outs = 0;
  blobdata miner_bd = tx_to_blob(blk.miner_tx);
  add_transaction(blk_hash, blk.miner_tx, epee::strspan<std::uint8_t>(miner_bd), transparent_amount_commitments);
  if (blk.miner_tx.version == 2)
    num_rct_outs += blk.miner_tx.vout.size();
  int tx_i = 0;
  crypto::hash tx_hash = crypto::null_hash;
  for (const std::pair<transaction, blobdata>& tx : txs)
  {
    tx_hash = blk.tx_hashes[tx_i];
    add_transaction(blk_hash, tx.first, epee::strspan<std::uint8_t>(tx.second), transparent_amount_commitments, &tx_hash);
    for (const auto &vout: tx.first.vout)
    {
      if (vout.amount == 0)
        ++num_rct_outs;
    }
    ++tx_i;
  }
  TIME_MEASURE_FINISH(time1);
  time_add_transaction += time1;

  // call out to subclass implementation to add the block & metadata
  time1 = epee::misc_utils::get_tick_count();
  add_block(blk, block_weight, long_term_block_weight, cumulative_difficulty, coins_generated, num_rct_outs, blk_hash);
  TIME_MEASURE_FINISH(time1);
  time_add_block1 += time1;

#ifdef ENABLE_FCMP_INTEGRATION
  // call out to handle growing the FCMP tree
  time1 = epee::misc_utils::get_tick_count();
  handle_fcmp_tree(blk_idx, first_unified_id, blk.miner_tx, txs, transparent_amount_commitments);
  TIME_MEASURE_FINISH(time1);
  time_grow_tree += time1;
#endif

  m_hardfork->add(blk, blk_idx);

  ++num_calls;

  return blk_idx;
}

void BlockchainDB::handle_fcmp_tree(const uint64_t block_idx,
  const uint64_t first_unified_id,
  const transaction &miner_tx,
  const std::vector<std::pair<transaction, blobdata>> &txs,
  const std::unordered_map<uint64_t, rct::key> &transparent_amount_commitments)
{
  // Collect outs by last locked block to add to the db
  OutsByLastLockedBlockMeta new_locked_outs = cryptonote::get_outs_by_last_locked_block(
      miner_tx,
      txs,
      transparent_amount_commitments,
      first_unified_id,
      block_idx
    );

  // Get the outputs with default last locked block
  const uint64_t default_last_locked_block = cryptonote::get_default_last_locked_block_index(block_idx);
  auto new_default_locked_outs_it = new_locked_outs.outs_by_last_locked_block.find(default_last_locked_block);
  const auto new_default_locked_outs = new_default_locked_outs_it != new_locked_outs.outs_by_last_locked_block.end()
    ? std::move(new_default_locked_outs_it->second)
    : std::vector<fcmp_pp::UnifiedOutput>{};

  // Insert the new locked outputs into the db, excluding outputs created in
  // this block with default last locked block. Outputs with default last locked
  // block will be added to the tree immediately below. Outputs with last
  // locked block higher than the default will be added to the locked outputs
  // tables, staged for insertion to the tree later.
  new_locked_outs.outs_by_last_locked_block.erase(default_last_locked_block);
  this->add_locked_outs(new_locked_outs.outs_by_last_locked_block, new_locked_outs.timelocked_outputs);

  // Assume we just added block n. The soonest that outputs from block n can be
  // included in the chain is in block n + CRYPTNOTE_DEFAULT_SPENDABLE_AGE. So
  // we grow the tree with these outputs (and any others with last locked block
  // n + CRYPTNOTE_DEFAULT_SPENDABLE_AGE - 1). We then expect this tree root
  // be included in block header n+1. This way miners will build on top of the
  // tree root usable in FCMP++'s in a future block. After block
  // n + (CRYPTNOTE_DEFAULT_SPENDABLE_AGE - 1) is added to the chain, SPV
  // clients syncing just block headers will have a solid assurance that the
  // root usable to construct FCMP++ proofs is the correct root, since it will
  // have 9 blocks of PoW on top of it.
  // To be clear, block header n+1 includes the tree root usable to spend
  // outputs with last locked block n + CRYPTNOTE_DEFAULT_SPENDABLE_AGE - 1.
  static_assert(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > 0, "Expect a non-0 spendable age");
  this->advance_tree(block_idx, new_default_locked_outs);
}

void BlockchainDB::advance_tree(const uint64_t blk_idx, const std::vector<fcmp_pp::UnifiedOutput> &known_new_outputs)
{
  LOG_PRINT_L3("BlockchainDB::" << __func__);

  // Get the earliest possible last locked block of outputs created in blk_idx
  const uint64_t earliest_last_locked_block = cryptonote::get_default_last_locked_block_index(blk_idx);

  // If we're advancing the genesis block, make sure to initialize the tree
  if (blk_idx == 0)
  {
    // Expected: tree meta table is currently empty

    // We grow the first blocks with empty outputs, since no outputs in this range should be spendable yet
    for (uint64_t new_blk_idx = blk_idx; new_blk_idx < earliest_last_locked_block; ++new_blk_idx)
    {
      this->grow_tree(new_blk_idx, {});
    }
  }
  // Expected: earliest_last_locked_block == last block idx + 1 in tree meta

  // Now we can advance the tree 1 block
  auto unlocked_outputs = this->get_outs_at_last_locked_block_idx(earliest_last_locked_block);

  // Include known new outputs if provided
  unlocked_outputs.insert(unlocked_outputs.end(), known_new_outputs.begin(), known_new_outputs.end());

  // Grow the tree with outputs that are spendable once the earliest_last_locked_block is in the chain
  this->grow_tree(earliest_last_locked_block, std::move(unlocked_outputs));

  // Now that we've used the unlocked leaves to grow the tree, we delete them from the locked outputs table
  this->del_locked_outs_at_block_idx(earliest_last_locked_block);
}

void BlockchainDB::grow_tree(const uint64_t blk_idx, std::vector<fcmp_pp::UnifiedOutput> &&new_outputs)
{
  LOG_PRINT_L3("BlockchainDB::" << __func__);

  MDEBUG("Growing tree usable once block " << blk_idx << " is in the chain");

  // Get the number of leaf tuples that exist in the current tree
  const uint64_t old_n_leaf_tuples = this->get_n_leaf_tuples();

  if (blk_idx == 0)
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples == 0, "Tree is not empty at blk idx 0");

  // Get the prev block's tree edge (i.e. the current tree edge before growing)
  std::vector<crypto::ec_point> prev_tree_edge;
  uint64_t prev_blk_idx = 0;
  if (blk_idx > 0)
  {
    prev_blk_idx = blk_idx - 1;

    // Make sure tree tip lines up to expected block
    const uint64_t tree_block_idx = this->get_tree_block_idx();

    CHECK_AND_ASSERT_THROW_MES(tree_block_idx == prev_blk_idx,
      "Unexpected tree block idx mismatch to prev block (" + std::to_string(tree_block_idx) + " vs " + std::to_string(prev_blk_idx) + ")");

    prev_tree_edge = this->get_tree_edge(prev_blk_idx);
  }

  // We re-save the prev tree edge at this next block if the tree doesn't grow
  const auto save_prev_tree_edge = [&, this]() { this->save_tree_meta(blk_idx, old_n_leaf_tuples, prev_tree_edge); };
  if (new_outputs.empty())
  {
    save_prev_tree_edge();
    return;
  }

  // Set the tree's existing last hashes from the existing edge
  const auto last_hashes = m_curve_trees->tree_edge_to_last_hashes(prev_tree_edge);

  // Use the number of leaf tuples and the existing last hashes to get a struct we can use to extend the tree
  auto tree_extension = m_curve_trees->get_tree_extension(old_n_leaf_tuples, last_hashes, {std::move(new_outputs)});
  if (tree_extension.leaves.tuples.empty())
  {
    save_prev_tree_edge();
    return;
  }

  const auto compressed_tree_extension = m_curve_trees->compress_tree_extension(std::move(tree_extension));
  const auto tree_edge = this->grow_with_tree_extension(compressed_tree_extension);

  const uint64_t new_n_leaf_tuples = compressed_tree_extension.leaves.tuples.size() + old_n_leaf_tuples;
  this->save_tree_meta(blk_idx, new_n_leaf_tuples, tree_edge);
}

void BlockchainDB::trim_block()
{
  LOG_PRINT_L3("BlockchainDB::" << __func__);

  const uint64_t n_blocks = this->height();
  if (n_blocks == 0)
    return;

  const uint64_t removing_block_idx = n_blocks - 1;

  // Get the earliest possible last locked block of outputs created in removing_block_idx
  const uint64_t default_last_locked_block = cryptonote::get_default_last_locked_block_index(removing_block_idx);
  const uint64_t tree_block_idx = this->get_tree_block_idx();

  CHECK_AND_ASSERT_THROW_MES(tree_block_idx > 0, "tree block idx must be >0");
  CHECK_AND_ASSERT_THROW_MES(tree_block_idx == default_last_locked_block,
    "Unexpected tree block idx mismatch (" + std::to_string(tree_block_idx) + " vs " + std::to_string(default_last_locked_block) + ")");

  const uint64_t prev_tree_block_idx = tree_block_idx - 1;

  MDEBUG("Trimming tree to block " << prev_tree_block_idx << " (removing block " << removing_block_idx << ")");

  // Read n leaf tuples from the prev tree block to see how how many leaves
  // should remain in the tree after trimming a block from the tree.
  const uint64_t new_n_leaf_tuples = this->get_block_n_leaf_tuples(prev_tree_block_idx);

  // Trim the tree to the new n leaf tuples
  this->trim_tree(new_n_leaf_tuples, tree_block_idx);

  // Remove block from tree meta
  this->del_tree_meta(tree_block_idx);

  // If we're removing the genesis block, we need to reverse the initially added in advance_tree.
  // Since we know they're all empty, we can just delete the tree meta.
  if (removing_block_idx == 0)
  {
    for (uint64_t rm_blk_idx = 0; rm_blk_idx < default_last_locked_block; ++rm_blk_idx)
    {
      this->del_tree_meta(rm_blk_idx);
    }
  }
}

void BlockchainDB::trim_tree(const uint64_t new_n_leaf_tuples, const uint64_t trim_block_idx)
{
  LOG_PRINT_L3("BlockchainDB::" << __func__);

  const uint64_t old_n_leaf_tuples = this->trim_leaves(new_n_leaf_tuples, trim_block_idx);

  // If nothing to trim, return
  if (old_n_leaf_tuples == new_n_leaf_tuples)
    return;

  if (new_n_leaf_tuples == 0)
  {
    // Empty the tree
    this->trim_layers(new_n_leaf_tuples, {}/*new_n_elems_per_layer*/, {}/*new_tree_edge*/, 0/*new_root_layer_idx*/);
    return;
  }

  // Trim the expected layers
  const auto new_n_elems_per_layer = m_curve_trees->n_elems_per_layer(new_n_leaf_tuples);
  const auto new_tree_edge = this->get_tree_edge(trim_block_idx - 1);
  const uint64_t new_root_layer_idx = m_curve_trees->n_layers(new_n_leaf_tuples) - 1;
  this->trim_layers(new_n_leaf_tuples, new_n_elems_per_layer, new_tree_edge, new_root_layer_idx);
}

void BlockchainDB::set_hard_fork(HardFork* hf)
{
  m_hardfork = hf;
}

void BlockchainDB::pop_block(block& blk, std::vector<transaction>* txs)
{
  blk = get_top_block();

  remove_block();

  for (const auto& h : boost::adaptors::reverse(blk.tx_hashes))
  {
    if (txs)
    {
      cryptonote::transaction tx;
      if (!get_tx(h, tx) && !get_pruned_tx(h, tx))
        throw DB_ERROR("Failed to get pruned or unpruned transaction from the db");
      txs->push_back(std::move(tx));
    }
    remove_transaction(h);
  }
  remove_transaction(get_transaction_hash(blk.miner_tx));
}

bool BlockchainDB::is_open() const
{
  return m_open;
}

void BlockchainDB::remove_transaction(const crypto::hash& tx_hash)
{
  transaction tx = get_pruned_tx(tx_hash);

  for (const txin_v& tx_input : tx.vin)
  {
    if (tx_input.type() == typeid(txin_to_key))
    {
      remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
    }
  }

  // need tx as tx.vout has the tx outputs, and the output amounts are needed
  remove_transaction_data(tx_hash, tx);
}

block BlockchainDB::get_block_from_height(const uint64_t& height) const
{
  blobdata bd = get_block_blob_from_height(height);
  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw DB_ERROR("Failed to parse block from blob retrieved from the db");

  return b;
}

block BlockchainDB::get_block(const crypto::hash& h) const
{
  blobdata bd = get_block_blob(h);
  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw DB_ERROR("Failed to parse block from blob retrieved from the db");

  return b;
}

bool BlockchainDB::get_tx(const crypto::hash& h, cryptonote::transaction &tx) const
{
  blobdata bd;
  if (!get_tx_blob(h, bd))
    return false;
  if (!parse_and_validate_tx_from_blob(bd, tx))
    throw DB_ERROR("Failed to parse transaction from blob retrieved from the db");

  return true;
}

bool BlockchainDB::get_pruned_tx(const crypto::hash& h, cryptonote::transaction &tx) const
{
  blobdata bd;
  if (!get_pruned_tx_blob(h, bd))
    return false;
  if (!parse_and_validate_tx_base_from_blob(bd, tx))
    throw DB_ERROR("Failed to parse transaction base from blob retrieved from the db");

  return true;
}

transaction BlockchainDB::get_tx(const crypto::hash& h) const
{
  transaction tx;
  if (!get_tx(h, tx))
    throw TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str());
  return tx;
}

transaction BlockchainDB::get_pruned_tx(const crypto::hash& h) const
{
  transaction tx;
  if (!get_pruned_tx(h, tx))
    throw TX_DNE(std::string("pruned tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str());
  return tx;
}

std::vector<bool> BlockchainDB::has_key_images(const epee::span<const crypto::key_image> img) const
{
  std::vector<bool> spent(img.size(), true);
  for (std::size_t i = 0; i < img.size(); ++i)
  {
    const crypto::key_image &ki = img[i];
    spent[i] = this->has_key_image(ki);
  }
  return spent;
}

void BlockchainDB::reset_stats()
{
  num_calls = 0;
  time_blk_hash = 0;
  time_tx_exists = 0;
  time_add_block1 = 0;
  time_add_transaction = 0;
  time_grow_tree = 0;
  time_commit1 = 0;
}

void BlockchainDB::show_stats()
{
  LOG_PRINT_L1(ENDL
    << "*********************************"
    << ENDL
    << "num_calls: " << num_calls
    << ENDL
    << "time_blk_hash: " << time_blk_hash << "ms"
    << ENDL
    << "time_tx_exists: " << time_tx_exists << "ms"
    << ENDL
    << "time_add_block1: " << time_add_block1 << "ms"
    << ENDL
    << "time_add_transaction: " << time_add_transaction << "ms"
    << ENDL
    << "time_grow_tree: " << time_grow_tree << "ms"
    << ENDL
    << "time_commit1: " << time_commit1 << "ms"
    << ENDL
    << "*********************************"
    << ENDL
  );
}

void BlockchainDB::fixup()
{
  if (is_read_only()) {
    LOG_PRINT_L1("Database is opened read only - skipping fixup check");
    return;
  }

  // There was a bug that would cause key images for transactions without
  // any outputs to not be added to the spent key image set. There are two
  // instances of such transactions on mainnet, in blocks 202612 and 685498.
  // On testnet, there are no such transactions
  // See commit 533acc30eda7792c802ea8b6417917fa99b8bc2b for the fix
  // Since its been 8 years since the fix was implemented, we can safely force
  // nodes to re-sync if the bug is present in their chain.
  static const char * const mainnet_genesis_hex = "418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e3";
  crypto::hash mainnet_genesis_hash;
  epee::string_tools::hex_to_pod(mainnet_genesis_hex, mainnet_genesis_hash );

  if (get_block_hash_from_height(0) == mainnet_genesis_hash)
  {
    // From tx 17ce4c8feeb82a6d6adaa8a89724b32bf4456f6909c7f84c8ce3ee9ebba19163
    static const char * const first_missing_key_image_202612 =
      "121bf6ae1596983b703d62fecf60ea7dd3c3909acf1e0911652e7dadb420ed12";

    if (height() > 202612)
    {
      crypto::key_image ki;
      epee::string_tools::hex_to_pod(first_missing_key_image_202612, ki);
      if (!has_key_image(ki))
      {
        LOG_PRINT_L1("Fixup: detected missing key images from block 202612! Popping blocks...");
        block blk;
        while (height() > 202612)
          pop_block(blk, /*txs=*/nullptr);
      }
    }
  }
}

bool BlockchainDB::txpool_tx_matches_category(const crypto::hash& tx_hash, relay_category category)
{
  try
  {
    txpool_tx_meta_t meta{};
    if (!get_txpool_tx_meta(tx_hash, meta))
    {
      MERROR("Failed to get tx meta from txpool");
      return false;
    }
    return meta.matches(category);
  }
  catch (const std::exception &e)
  {
    MERROR("Failed to get tx meta from txpool: " << e.what());
  }
  return false;
}

}  // namespace cryptonote
