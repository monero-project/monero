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

#include <boost/range/adaptor/reversed.hpp>

#include "string_tools.h"
#include "blockchain_db.h"
#include "blockchain_db_utils.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"

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

void BlockchainDB::pop_block()
{
  block blk;
  std::vector<transaction> txs;
  pop_block(blk, txs);
}

void BlockchainDB::add_transaction(const crypto::hash& blk_hash, const transaction& tx, epee::span<const std::uint8_t> blob, const crypto::hash* tx_hash_ptr, const crypto::hash* tx_prunable_hash_ptr)
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
    // note: get_outs_by_last_locked_block mirrors this logic
    if (miner_tx && tx.version == 2)
    {
      cryptonote::tx_out vout = tx.vout[i];
      // TODO: avoid multiple expensive zeroCommitVartime call here + get_outs_by_last_locked_block + ver_non_input_consensus
      rct::key commitment;
      if (!rct::getCommitment(tx, i, commitment))
        throw std::runtime_error("Failed to get miner tx commitment, aborting");
      vout.amount = 0;
      amount_output_indices[i] = add_output(tx_hash, vout, i, tx.unlock_time,
        &commitment);
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

  uint64_t prev_height = height();
  const uint64_t total_n_outputs = num_outputs();

  // call out to add the transactions

  time1 = epee::misc_utils::get_tick_count();

  uint64_t num_rct_outs = 0;
  blobdata miner_bd = tx_to_blob(blk.miner_tx);
  add_transaction(blk_hash, blk.miner_tx, epee::strspan<std::uint8_t>(miner_bd));
  if (blk.miner_tx.version == 2)
    num_rct_outs += blk.miner_tx.vout.size();
  int tx_i = 0;
  crypto::hash tx_hash = crypto::null_hash;
  std::vector<std::reference_wrapper<const transaction>> _txs; // ref wrapper to avoid extra copies
  _txs.reserve(txs.size());
  for (const std::pair<transaction, blobdata>& tx : txs)
  {
    tx_hash = blk.tx_hashes[tx_i];
    add_transaction(blk_hash, tx.first, epee::strspan<std::uint8_t>(tx.second), &tx_hash);
    for (const auto &vout: tx.first.vout)
    {
      if (vout.amount == 0)
        ++num_rct_outs;
    }
    ++tx_i;
    _txs.push_back(std::ref(tx.first));
  }
  TIME_MEASURE_FINISH(time1);
  time_add_transaction += time1;

  // When adding a block, we also need to keep track of when outputs unlock, so
  // we can use them to grow the merkle tree used in fcmp's at that point.
  const auto outs_by_last_locked_block_meta = cryptonote::get_outs_by_last_locked_block(blk.miner_tx, _txs, total_n_outputs, prev_height);
  const auto &outs_by_last_locked_block = outs_by_last_locked_block_meta.outs_by_last_locked_block;
  const auto &timelocked_outputs = outs_by_last_locked_block_meta.timelocked_outputs;

  // call out to subclass implementation to add the block & metadata
  time1 = epee::misc_utils::get_tick_count();
  add_block(blk, block_weight, long_term_block_weight, cumulative_difficulty, coins_generated, num_rct_outs, blk_hash, outs_by_last_locked_block, timelocked_outputs);
  TIME_MEASURE_FINISH(time1);
  time_add_block1 += time1;

  m_hardfork->add(blk, prev_height);

  ++num_calls;

  return prev_height;
}

void BlockchainDB::set_hard_fork(HardFork* hf)
{
  m_hardfork = hf;
}

void BlockchainDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  blk = get_top_block();

  remove_block();

  for (const auto& h : boost::adaptors::reverse(blk.tx_hashes))
  {
    cryptonote::transaction tx;
    if (!get_tx(h, tx) && !get_pruned_tx(h, tx))
      throw DB_ERROR("Failed to get pruned or unpruned transaction from the db");
    txs.push_back(std::move(tx));
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

void BlockchainDB::reset_stats()
{
  num_calls = 0;
  time_blk_hash = 0;
  time_tx_exists = 0;
  time_add_block1 = 0;
  time_add_transaction = 0;
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
        while (height() > 202612)
          pop_block();
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
