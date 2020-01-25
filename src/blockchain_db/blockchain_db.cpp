// Copyright (c) 2014-2019, The Monero Project
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
      if (method == relay_method::none)
        return false;
      return true;
    case relay_category::broadcasted:
    case relay_category::legacy:
      break;
  }
  // check for "broadcasted" or "legacy" methods:
  switch (method)
  {
    default:
    case relay_method::local:
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

  switch (method)
  {
    case relay_method::none:
      do_not_relay = 1;
      break;
    case relay_method::local:
      is_local = 1;
      break;
    default:
    case relay_method::fluff:
      break;
    case relay_method::block:
      kept_by_block = 1;
      break;
  }
}

relay_method txpool_tx_meta_t::get_relay_method() const noexcept
{
  if (kept_by_block)
    return relay_method::block;
  if (do_not_relay)
    return relay_method::none;
  if (is_local)
    return relay_method::local;
  return relay_method::fluff;
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

void BlockchainDB::add_transaction(const crypto::hash& blk_hash, const std::pair<transaction, blobdata>& txp, const crypto::hash* tx_hash_ptr, const crypto::hash* tx_prunable_hash_ptr)
{
  const transaction &tx = txp.first;

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
    if (!tx_prunable_hash_ptr)
      tx_prunable_hash = get_transaction_prunable_hash(tx, &txp.second);
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
      LOG_PRINT_L1("Unsupported input type, removing key images and aborting transaction addition");
      for (const txin_v& tx_input : tx.vin)
      {
        if (tx_input.type() == typeid(txin_to_key))
        {
          remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
        }
      }
      return;
    }
  }

  uint64_t tx_id = add_transaction_data(blk_hash, txp, tx_hash, tx_prunable_hash);

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
      rct::key commitment = rct::zeroCommit(vout.amount);
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

  // call out to add the transactions

  time1 = epee::misc_utils::get_tick_count();

  uint64_t num_rct_outs = 0;
  add_transaction(blk_hash, std::make_pair(blk.miner_tx, tx_to_blob(blk.miner_tx)));
  if (blk.miner_tx.version == 2)
    num_rct_outs += blk.miner_tx.vout.size();
  int tx_i = 0;
  crypto::hash tx_hash = crypto::null_hash;
  for (const std::pair<transaction, blobdata>& tx : txs)
  {
    tx_hash = blk.tx_hashes[tx_i];
    add_transaction(blk_hash, tx, &tx_hash);
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
  // instances of such transactions, in blocks 202612 and 685498.
  // The key images below are those from the inputs in those transactions.
  // On testnet, there are no such transactions
  // See commit 533acc30eda7792c802ea8b6417917fa99b8bc2b for the fix
  static const char * const mainnet_genesis_hex = "418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e3";
  crypto::hash mainnet_genesis_hash;
  epee::string_tools::hex_to_pod(mainnet_genesis_hex, mainnet_genesis_hash );
  set_batch_transactions(true);
  batch_start();

  if (get_block_hash_from_height(0) == mainnet_genesis_hash)
  {
    // block 202612 (511 key images in 511 transactions)
    static const char * const key_images_202612[] =
    {
      "51fc647fb27439fbb3672197d2068e4110391edf80d822f58607bd5757cba7f3",
      "d8cf1c1bd41f13c4553186e130e6e2c1cd80135ddb418f350088926997a95ca9",
      "95d2556c8acd1457dce7bfd9c83b1d82b821a55a9c9588b04b7b5cf562a65949",
      "4b5d987fee1bb563a162d23e41741ad73560c003e26a09b6655f09496538daac",
      "1d25ea86323d1578579d3894a54b99ea1c3e2dca547c6726c44aef67db958b02",
      "92e46fb70be5d9df39ca83c4fc6ae26c594118314bb75502a9c9752a781d0b33",
      "924d0cb9060d429be7e59d164a0f80a4dabc3607d44401b26fb93e7182ab435d",
      "f63e4a23fec860fd4c3734623891330ac1ff5af251e83a0e6247287818b8a72f",
      "5b14c5ef13738d015619b61dacefc2ade3660d25b35ef96330a8f4e2afc26526",
      "d5016b012a2fb6ca23fd56ece544d847962264b4aee15efe1465805fd824a8fb",
      "0a7f3da1d9dd341cd96829e484b07163099763ac7bd60603b7ee14f7dbcb278d",
      "d716c03d7447d2b693f6f61b6ad36bd57344033fc1a11feaf60d569f40014530",
      "23154a812e99ce226f6a87087e0812f419aed51289f1c0c359b0b61303b53a36",
      "03940341e1a99d5b0c68eacfdf5a20df90d7d0a3d6089d39709cdd964490755c",
      "ef09648814cfe071f5d8e9cfde57247ad09409265c4b6c84697bbb046809bd7e",
      "8843ec52b0496ca4e895813cfe00bb18ea777d3618e9bd2e200287e888e2f5c7",
      "8558bf39baf3df62b5d33cdf97163a365e6c44f4d6deef920730b4982b66449f",
      "504d9380ce581de0af97d5800d5ca9e61d78df368907151ab1e567eb6445332a",
      "673797763593c23b3ee07b43bd8760365e2c251a8a60a275528ff34a477110cc",
      "25178c95e4d402c58d79c160d2c52dd3c45db2c78e6aaa8d24d35c64f19d4957",
      "407c3a05dcc8bdcb0446b5d562cf05b4536fc7337344765215130d5f1e7ee906",
      "4e7fa771a5455d8ee8295f01181a360cdc6467cc185c2834c7daf9fbf85b6f1d",
      "6beb64cb024f9c5c988f942177fc9c1ec5ecfa85b7db0f13a17f9f98e8e46fe7",
      "6406bfc4e486e64c889ea15577d66e5835c65c6a39ec081af8ac5acdc153b4b5",
      "1b1da638709f9f85898af70ffaa5b88d5a4c9f2663ca92113c400ab25caf553c",
      "da49407a9e1ed27abd28076a647177157c42517e2542e9b6a4921fdadf4e8742",
      "3c3fdba2a792fddaeb033605163991a09933e8f05c6c934d718e50a613b64d69",
      "82c60429171173739fa67c4807cab359620299e6ed2a9da80139b5b1e23c5456",
      "0a19e5767e1381ac16f57cfa5aabd8463551d19f069f7f3465c11a8583253f3e",
      "d0fae6ffdd4818399eae6224978f170f696150eaf699f2f218abc00c68530f96",
      "0937889aeb3af5c64608d9a9f88229a068e53417448f00f9aa5e28c570cca8f8",
      "d6072d269753020912524961ce8c6930cf35abe5c4b2bdc7fd678338d20a68fb",
      "0e8bc9b06fcc842bdaa7df029bfd1f252d5defc014d58a006c10ab678ecf543f",
      "9d42f90520f02c9258be53b123d69ddbce5f33f92a903d3fb4cf3358ff0e07b5",
      "1cc05416b12cbe719617158773c3e6173435fc64e1ee44310dc696baecaeaa95",
      "266b15222913c11ef6403ee15dc42c8c0e16bc5fa2f49110447802236e045797",
      "791b123af3b71ac9497a010610f72207ff8ec642969b5cf0d2891b21e7eee562",
      "946d4d7b084dc32495f22b35fc30394144c8e0ba04f3ad6e2c2bfb0173a2266d",
      "2c015cb990c1583228d08b2d5de9227b950c3f57364fc1033bca5c0fbfd08c58",
      "13fdc41862fd85507f579c69accb9cc6a40f5971bfa41e3caff598a3dcffd2fc",
      "64b06d9874a83917c583c9439d1c736083377d67fda2961b623f7124663134c3",
      "2fa49cd19e0aa02989991a4c3760f44be800fe8fb4d58b23aca382e10dc0d2d6",
      "377628f265f799772e9fb6065be8b6eee200c329f729fe36c25ee179e4d20df9",
      "ba94fa79134ce383b6a98b04dc6ad3d1b950e410d50a292bc770f9685e59fe91",
      "875c924329f0733e31fe8d8aed70dc1906335b8a9984932b6368ea24edb39765",
      "f31f4abb3f5ee42a5aae86d70b3bd9a9c1934933641893864dd333f89719d608",
      "2bcd629e125514a780f568d3c2e8b12a2e7fbbee06e652bbeed3e7825508e31c",
      "918b43581163ca1963de21bb9ac401756e75c3f00ac8dcfafc139f1ad5d7d998",
      "5730dd57fa52749a0d6502b11c9d802ac495875542431310c674a65655b7c2a3",
      "03f84b990683e569e2f6143bb963a2a8de411e7c4b7923117b94c7afcb4b43ea",
      "b298c8510d35bd2be0ff0753ad7d98d480f4c6490bb67fb93cd4632ea726e8a7",
      "0a771afbf9be104c01b89eaeb57073297d35ac8fbbcc0816820fdb9a29d26625",
      "713d90d6ca1be1a4e05a5f8441dc528c699caa09eda49c09072f5f8291354c2e",
      "988827f45c19330d9404309f63d536a447803cca7cb182ef005b074def09ab7d",
      "9dcaa105b4def895f3faee704c250bdc924316f153cb972f3fb565beec0b7942",
      "1c06c30afe65b59e9e22d6bb454e4209a03efe53cdbf27b3945d5d75b1b90427",
      "49e08c13d1da209ec1aea7b7fbe0daa648e30febeb2aa5ffbaaabdd71a278ac2",
      "e1c2e49ab7b829854c46a64772ded35459908e0f563edbcf5c612913b7767046",
      "e08bb7d133490ca85a6325d46807170cd07618b6a5f6e1d069e44890cc366fab",
      "5c73ca0691cde2f35b7933d6db33f0b642ec70d0bd3f2b0ebbd97754ca67e248",
      "6404399151872a521dae767311712225dba65d810ba2feba209204221b5d772d",
      "4a0c3aa6cef36f44edf08ad8fb1533d7e1186e317da8a3afb3d81af072043233",
      "104b3e1af37cf10b663a7ec8452ea882082018c4d5be4cd49e7f532e2fea64e5",
      "e723a46bf9684b4476c3005eb5c26511c58b7eb3c708ddf7470ee30a40834b32",
      "18e6f0fa3aa779a73ceefabea27bff3202003fd2c558ec5f5d07920528947d57",
      "c97e73eb593ff39e63307220796cc64974c0c8adac860a2559ab47c49bc0c860",
      "13c363a962955b00db6d5a68b8307cd900ae9202d9b2deb357b8d433545244ac",
      "76a488865151fab977d3639bac6cba4ba9b52aa17d28ac3580775ed0bff393e4",
      "a14de587c9f4cd50bb470ecffd10026de97b9b5e327168a0a8906891d39d4319",
      "b1d38ee1c4ca8ae2754a719706e6f71865e8c512310061b8d26438fedf78707e",
      "772bb8a3f74be96fa84be5fa8f9a8ef355e2df54869c2e8ae6ad2bf54ed5057e",
      "3083a7011da36be63e3f7cacd70ab52e364dd58783302f1cb07535a66b5735f5",
      "2b1d892e3002aa3201deb4ffe28c0c43b75b8f30c96b5d43f6d5829049ecbd94",
      "cb738aabe44c6fb17ade284bf27db0169e309bf8cf9c91c4e4e62856619a4c64",
      "1707e04b792f4953f460f217b9bb94c84cef60736a749fb01277cfe0eaaa48c7",
      "ab8b6bac9b8a4f00b78acb4bd50ed2758e0fa100964b6f298d2a943eb2af2b30",
      "dd317193fef72490f3be01293b29e9c2f94eda10824a76ca74bf39dd7cb40ab2",
      "4fb3d995b087af7517fcb79e71f43bac0c4fbda64d89417a40ca4a708f2e8bc1",
      "549ba38c31bf926b2cb7e8f7f15d15df6388dce477a3aff0245caa44606849fc",
      "7585c14ab9abbffb89d0fa9f00c78ecae9f7c9062e5d4f1fae8453b3951fc60b",
      "953f855323f72461b7167e3df0f4fd746a06f5a7f98aa42acdce2eef822a0b2f",
      "0931932d57dde94dcfb017179a5a0954b7d671422149738260a365ca44f50eb8",
      "a3d179d16a4a275a3bb0f260cee9284db243abad637a9dbe92d02940f1c7ee8c",
      "959843f1e76ff0785dafe312c2ea66380fdc32b9d6180920f05f874c74599a80",
      "fbc36b3e1718fe6c338968b04caa01a7adb315d206abc63e56768d69e008a65d",
      "f054de7eac5e2ea48072e7fb4db93594c5f5d2dfa0afe8266042b6adc80dfdca",
      "39dfc68dc6ba8c457b2995562c867cef2f2cf994e8d6776a6b20869e25053f70",
      "19ad7ca7629758c22ac83643605c8a32a6665bae8e35dbc9b4ad90343756ebb3",
      "e89e80ea5c64cf7840f614f26e35a12c9c3091fa873e63b298835d9eda31a9ea",
      "572c1b9a83c947f62331b83009cc2ec9e62eab7260b49929388e6500c45cd917",
      "df0b21f679e6c0bf97f7b874e9f07c93c3467b092f3d9e1484e5646fda6eca5f",
      "8f3b7c0f4b403af62fe83d3cfac3f1e2572af8afa4cea3f3e2e04291efe84cf6",
      "aae8b8db243009d021d8c9897d52ee8125a17212f0a8b85f681ad8950ae45f0e",
      "3d45a4957d27447dea83d9ae2ef392a3a86619bfcf8dda2db405a7b304997797",
      "a5b0a619a8e3030b691bdba1ed951cd54e4bc2063602eae26d9791fb18e60301",
      "14650df217dd64a2905cd058114e761502dff37d40e80789200bc53af29b265f",
      "fd6a245ab5e4e6e18d7ba9b37478ce38248f0ab864e5511d2208ae3d25017e5f",
      "fbe0be6dd42a11feb5db5ae56fcbbac41041ab04a35f1df075580e960c8eeab0",
      "72f3f1213d9bec92ba9705b447d99cd0a6a446e37a3c1c50bb8ece1090bfe56e",
      "20df836554e1534f62b2a6df9ce58e11c1b9b4746ce8ee3c462300a8c01f5e76",
      "5c3d2a81b7331c86420ad32b6e9a212b73b1c3413724a0f91bf073eba18e2f1f",
      "63264ddfb29cd36fc18f3ee6614c4101ba7229bc5ac375f912590d3f0df982f4",
      "5ec4eb637761c1c9dbc6aa6649d4410508ef8d25d61ad6caa40c6ee3236d5515",
      "270c70940536017915e1cdbc003de7279ec1c94cba1ef6130f4236f7e306e4f0",
      "c1d1d57a7c03f6ddeeab5230a4910db8355e2143f473dea6e1d57c2f8d882b76",
      "218c030a7fdc9917e9f87e2921e258d34d7740a68b5bee48a392b8a2acf1f347",
      "ac47861c01c89ea64abee14cf6e1f317859ed56b69ae66377dc63e6575b7b1eb",
      "23bf549c8a03f9870983c8098e974308ec362354b0dcf636c242a88f24fc2718",
      "a3ce8b817e5212c851c6b95e693849a396c79b0d04b2a554de9b78933fbea2b7",
      "7310120c1cc1961b0d3fce13743c8a7075ae426fe6cccaf83600f24cee106236",
      "8fa0630f193777dcc4f5eccd1ad9ceacf80acdf65e52e4e01bf3a2b2fdd0dac6",
      "4a5f5c87f67d573d0673f01abaebc26eaa62e6d04627588549cc9e6c142dc994",
      "78971cccacc645116f9d380c167f955d54b386a22af112233f7de004fc0c8316",
      "badc67216868e1de1bbe044bf0e6070e6ee0353d05c13fa0c43b1897db5219a2",
      "c45b2a168bc51cbb615a79f97432cc4bb6b104da9cdc1fc640c930657452f71b",
      "c17eda13541d14554c5db542155b08b6bf9cb403d425745b662ebc2b2b9b3a3b",
      "313210cd9d2efc1603f07859bae7bd5fb5914f4a631b943f2f6ff5927a4e681a",
      "6ee94ec8af4e6828f9b46c590ea55da640ef50e810a247d3e8cdf4b91c42d2c2",
      "505b7a4d9f1ba6577aa2a941843f86c205b23b1ea21035925e587022e2f0aeed",
      "98e6a7cd687e8192e300a8202999ec31ad57bc34f656f2ae90d148607ff6d29f",
      "1be5db002c0a446cc2c1da363e5d08ae045cd8f5e76c8cccd65d5166393c0bdf",
      "17c02ac6d390c5c735e1e873c40294220e89071fca08a5da396a131fa1ba8670",
      "2540507c39ae6fdcd90de826077f0ca390da126166a25c15c048a60606a27367",
      "5ab9328e525c7a017ef4f591d995ad4595d74cbf8ff4112af33b08c70661a304",
      "9c105587a96c51d81422f64e46016564d22329760648c95dcac7218f3733f915",
      "525afb1b94a75f1edc2b55c700071e14a2166acda003370047c30dba8ea80278",
      "745d4a5d9f95ca4efa6261b6bcd4ecacd504b5b901a2ce1353c522a5c0c15dcc",
      "5a5a568cd87ba34252ba254e6a320e1a7f52f13e7451bb887efb34ff881785f2",
      "1ec50a80198cd830b51f4f7a0222015a268d9b40f04e7f838c7b8dc7abf63b01",
      "68836b662d79349cb42f5cef54e6a066157d398cc87d3b13f29fc04e5cf364a5",
      "658db317f355a9cbd86f673775cac0c308fe14967428fd283a36e300a6a53b2f",
      "677d79a8c467dd9db38f0ef45c2787dd368f701a6b47bf7a5f06112c38da643e",
      "2baa455d4066f5d628f9ecd315cb57deca71069db5d0d112ae0aa18a84b6f0d7",
      "5e7b0889f351560081360ac2b1429b48b2f7d886227f144e3b198e2f1fa56ed9",
      "c3d317fbf26e15add8b4f8f93df9de9b22297b8e4945ebab9ee249d4f72f4e45",
      "3c0b705a5c1e31abc7e46d8ff3c148a033f6875454cfb67f8d2a2b9a57a5ba7e",
      "a0ab74663561af2adc2d38be3569fbe7aa2454346416ac96e5eb26b1e01b1e2f",
      "53526cffdb74327670566c1dacacffb7d30a43a7f1862ff8bab87737bfa5edb6",
      "24c5d36ab98d88f87b2c71afb4ea8562e05c7aa0b50f3bc0f9ed50a4cd52989b",
      "c3ce4de5f94dff65d11e33a865855a4404259cf45263914c884f79db4f35169d",
      "f1009b6dcf30030cff872d636fb96ed233eb6ecb8ffed003c7da64e4f5a02f4c",
      "e3729f58614d3b42450d1599d863983ab7e3e5c29fb57aad7958c8923a2627c4",
      "31cf4792f7b5ce01b217ec80184edd2a7c49c0b21701f5494ee2c9bac67c28ca",
      "b42a5c9c92a656c5bb2b759ce160fdfd245243aeb1786338faea63b62e9a60ce",
      "a1efc8d5d0855933d5ac8fe5960c7acacb92fcb09bfbc929e5002f168251e648",
      "c4322c7f3682ec94b0dcb42f13b498c36cf575d505aacc8ec8bf67a6a2abf4c9",
      "684ee5aa3c98357aeaddcc30c6620939b52aeef729e24b4a46ccafc44f24d831",
      "36180f2ae11d105e0efbfbddb94e6b45b08609a383e4e1a8fa3b06d7a8051de9",
      "96c2d183eacc87581a0b17b8d07878bc10d436728510715109a7565d9972f8b5",
      "3068c9d04d561c7e29e3f621280b61a61885de0e9ec35a66a3116ca7a9e09627",
      "2eb94b9673ad6f8f88288fddfceae4baaeccb37bed88a35611d826ba06a5363b",
      "fc8cd5fae8b81121001f7767dcd5f185c0fdcc88cce1fbb184ddbcfad697ba54",
      "51521da1ecedea6d588d774eb155d936b32a14913c2f11d989bcc5116e65bf41",
      "3d23542e597a83dd6307700d79058b920f281c65f832333734d8a0adec510495",
      "11d2e01913ff0d4bd21970d709d88e63289492c0bbad7bff99c0d36858a841ca",
      "de674f1eee3068d2bc8c2f2897d8556e5deb872869652f7d3a4e5dbc6f1063c8",
      "e722d7f728526110c0921791b417afde4af1e87ae48ccc01911786197843104b",
      "aaba3a4e2a7d20ab76edfbcccefc27acfd509db3638582c28230e73ffd71d340",
      "1385a7209dafb9622dd4274179832e40c7fae19445383c34ef79adb0e4de0c98",
      "108408531fca288d74de4a2c596eab8569e355d9ab2f8380f4d24073a6b3fa95",
      "294476a86fcd39351ae452cdb8af9584067ec4501ec6182d0062bb154559fed3",
      "e64b175e0284c5cb69c8db46344ed43b5ced8abfe3cbf0c197103cfd116944cd",
      "cdd73a0f1fa7c14ed2177ae2163035333718847e49dd5cca6775bd20fc7553ad",
      "d423d2a374bc66a4587a5e3affa327ca75b8116051320759a3b88a868a7b80d4",
      "f13ad1e5b1315557d5497b58516eb3b0759d909725ddd0eb8a0dee439c6c0a48",
      "3a600b547a6061186a54e491344fd50cc7d4f0566a386a40aba6545254773728",
      "37a6f3f221fe17cc04a65aa544d5135e8297ecaf0853ba784dffacb74feb481b",
      "0ca42d67d0f84b28861d63e919e6ce5ad527447fdc53a03d8497a0241bee9376",
      "c7dbda42459e6fadb92c416eaef3e04674fc57915a93f3be4e656634c9654075",
      "0e34d728ae4fe347a5afecdf886fbd4d48a65c5d0dfab807da6ae95b6b2d7a3a",
      "f1bc69257ed510db5b2ed370010b037f452a29c49e36337264b3011ce2516216",
      "33f98f6b8a8e202463955998fba3b790199daa893e5471554826cfd9daa5c02f",
      "f8a0a37a2c9ebd7022d7cded1ee0318fd363020070b4cdaea800e44dcc1300d2",
      "6862714daedb908a4d86a3a3f1e65ec2c29ae61501b4ddcaf184243dd095d71b",
      "555cd19a6d21941c1174129b8bbcc70edcf0d6874262ce9e1e542351990d523d",
      "2cd6b44326828f23a2aa33699754bfa072c4883f39d53616f6a6b74149b664b6",
      "127f45d2eacb565c21c1030fe8054fd0a3a75705bc368924712aa145d414fa47",
      "19225e2dae6e1166f21cdab1290194470ded09df9b66f3faad3c1cc9ebcf340f",
      "b7b3f53f0539b2b4837b8bb9dae0ccbd200c8d36126d9f50199d68a4293a46d3",
      "6b6323be01f27d6d759d9670825e8ebb9c4cd8016351702328df91cef36cfec8",
      "020c31cfdfc5b22b10235745b89b311d271cf82f2ba16d03fdf7a8bc8538694b",
      "62573218530182b79e40d0113b7d281dace6da33bfcd0f9318558de5e5c76f08",
      "37d928416b15982f5bb8be40c5b62fae0b664e412c25891f8860c4242927e610",
      "b07ad11134a5c0542d2b418ef3863e8ea8477de68d9310681818ddd40825fdb0",
      "4af77cb76bab845b56470c95ce7b8cd84ce49a095984c1f3eed67b0ee344316e",
      "e3fdd4668d8726ba6adc401ac662c0cf6b5c1872082c488ed7da966d425fb1c0",
      "3dec71c81c7e78e879abc8da8b30e2446edbe98eeb8df9dafe9201ebb4c6a834",
      "7105467d9c5e855f1362fbddf820ed5e757997862efc9000317d3830a2f60ef3",
      "2821df94b021d3e77e8d9c0f3972340210f5ea2c0935cbf125cfc578d4d6722f",
      "114e5807accc337a22598bded30ebf3e0cfd75877e239f10cb043f829c315ab5",
      "d658a1c0354628cd7312593ab25d5b9083de8f0def6e8425f188101d256cd136",
      "4818d3be9b2a38fcc8c85d6c46f69b502943f79cf2462dfb0b6499e761bcc836",
      "92b8c943cb017b5f2d39264640c069f1ecced1d3ce9b3fd755d6df2fddb99458",
      "6edbd0fdf064fcbccd4a9e7a8ea520b87cb7faf867f7fe8a5f53625beb575119",
      "bf3b49c477dafb06af65bf09851c0fbef9dbc3152a7268d31b55a8e7a9a95860",
      "0e234dbadfda1393be2f068182615dbb83736f84f87710b5c7965bdce9f4a26a",
      "df5ceae34429e47b92bbd5505ba27666552e0eb619997f359c55699c3252b1ff",
      "08c1c7d940d699a91a83249bd578772e8958ffe23179e6150f07b96f1b47ce1e",
      "6f919a429270da0022d70062537bdc1b21c43a8abc552d8e366647e5b719d310",
      "63c66e5fd5d27f6fda87912ce46fa91a5e5b3634ed147fa2986330fc2696d3fa",
      "bde070b75296bca3aa494e7f549cd2bd1ff003776712bc98a3164b139b2054ab",
      "66694196dac5b60cf5e0ae05db8f3894fe04d65910686806551f471a0a0472e9",
      "0d2e97524b7ce4cf30b54e61b2689df036d099c53d42e2977b1671834bac39e7",
      "e081af76e923455f408127862be5c9baf7de6b19b952aa2a1da997d4dfe594c0",
      "121bf6ae1596983b703d62fecf60ea7dd3c3909acf1e0911652e7dadb420ed12",
      "a25e7b17464df71cd84ad08b17c5268520923bc33fe78c21b756f17353ea39a0",
      "e985f078fb44dbfdf3f4f34388f0f233a4e413e02297ee9a7dcc3fcceacd44f9",
      "b9184cf45e6e6b112cd863b1719de1bcab2137eb957e8028edca3a204a9ebb96",
      "157d177d5e4bcce0040eb4bddb681eacf9e2942e1c542a57ce851b4742a9cc4f",
      "0823e06635c9a1a69fd8833d1e48df98d711c1c47c06f27bb384932db1bbe9ee",
      "8beeec1fd1bcdecba235b449cc49abca69b6486ed1c0861a2bfb6a43c970b86f",
      "349f61a1cfc9112e537522858a0edae732a2f8434cf4780d3d2ec1e96f581cca",
      "587cdf72b5914d364f7e214a70481cf1131ee4a09e6b43e52428d2e56b000c13",
      "a6aa0c179316534c7b9ffb5f42a2af98d1d3a166bfb413199579f259c7b5e6df",
      "f9f3bb1ba8da5899b79186008ecfbd416b49f3b86d94045b91e34a40e41d5cff",
      "0cdda65a60b7b4d94e794c9397e01f69fb29309ce4fac83e7392dbba6bc497f9",
      "8fa5fce5ad09d43af7218ea5724cff2c4849a59ff73caf3bbca466e3c8538ba8",
      "8874ef46008753fcc0b77eb7a4a8560e35966bf5a12bcad4203ad2b4c1f8bfbe",
      "a8ee9a3aa2d0c08a951439ffb0e6d10315fc4776997b275de1ec19663e88c2c2",
      "9c184cbbff464ab4d5f6bfa78c39bf0880fb93b1574139306a97acb940d415c9",
      "5493a38c255c91ca49b958ed417f6c57e5bc444779d8f536f290596a31bc63d3",
      "3e1e82b96cc599d9fc55ae74330692ccbfb538a4cc923975fd8876efe4b81552",
      "16aaaf820c24c2726e349b0e49bbab72ca6eef7b3a2835de88d0cececa4da684",
      "7fa52ba349f7203c3dbc2249f9881101a3318d21869dd59f17abf953d234db65",
      "713d8018bb9ba3ab55c3a110120b9d7593514111075ef05f0fdb233ff2ceddc8",
      "56063afb495759a0942f1c33f28a4fb8320c6d376cb3c9513249453e45f24a04",
      "f9a6bacd9e055749b45174ecf3c3db18b78f3474761948a68adf601f54e59660",
      "7ddd7c6d41572f93fe07c0300c34e455b6d1f4372204933bf45035241c8b060c",
      "f81021b893a36b201de71330a2ea050b59dbf7560c62fa9cbea9261ab47a0ba2",
      "a01fbe4114c18fd534ae1621404d25c08e3b6775a2631ff40279bafd8c9304f4",
      "350fad8ebc938c6eb508a1483f385f577794a002bc1229db70a1b0131d174b9d",
      "570cb8bce87f532c5051d8c4c864012408e672a7d492669e660251fb1e066bec",
      "8cb6efbb129c84eba26d894c4293b476a6e9a1fe969c6ad18b554d2a57885f36",
      "f384a98467bf7f084ca31bea121a4ec76e530f523d3225c21ed25a18544a9916",
      "da127ab58ce557c2c95c20d6a291c2e5d880fff09dc28927b7bdfec97b995d72",
      "a4d95b4f74366ec920d0a0c5d81265688cc18733ffc444cac9b01ae2431568aa",
      "5ae2a71470570733422468bb733d53c85b1c8a6e7e6df5c05941556bcf342d1a",
      "65a2d161ff0e095d3afe37584dbbe649f1e9cd440755b5a3c5b2a252d5c0b8bc",
      "25ef70a8e41bb422ed7996a41160294e33238d6af17a532232f0a50b123431a2",
      "f1f0f76ee901664a65b97104296babb9c7422370e99bb677ae07c2ee420e7f40",
      "c3c66dda180b0330e75c8139b9f315a8c6b937f55d87d7be42e172bbac60d71e",
      "5881786695a9e58e19d79f790c7d9243a847c0269c5770bdd01f5149c2a62a88",
      "f2f816d3c8ebc7df06ab68d39223181aacc7be04364d1d4e69a56c33949bb983",
      "80a1c3b6b2778d4846ad9fe0bb2dd5afd99aa897f8231bfaac45fde43d602d9f",
      "72ad67cb043aa5df0c3dcc2464953a66893259d81c9cc7778c12bca3447fbd58",
      "ad72420a7963b8d4536d8eba00b4b989992247cd8c01660e242a8e71edaf0e19",
      "999d603d1cf6068e3bb6abe1bca976fa0ab84c4660b29ea8973de8b5cf5fd283",
      "e137a5910f02a764c3a3d8f1579ac0c7e3cc34e58933216868efe010332c1e6e",
      "10e0fa2362f59317626ae989bd1f962c583339d8d74d76c3e585243d152b91e8",
      "1951c652704962f5c6e33a4d4aadfee5d53ce2253644d1ed542da3e278524a07",
      "c938bccb7ba6c0217d8ba35ed91502aee051c8ae5bff05e88aab3b322aec936f",
      "4d6386c689785edd5beb55911a3a9fc8914838c8192184199589beef8b6ddf9f",
      "26f6f45a6468bc4b1f085fd28d63c264ee17564f9e247fc03ee179a0b579dcda",
      "235b7bb82b72c61acd5979ca5f2ca740aee805a780ba22e11aae5cd34f6ec760",
      "c027ffb585a1e4844b4907490b621b08c7e40a5e6f93e97bd4bb1b615bba9908",
      "aa77fc8053d139b998577319de29457b78b1cc8b35a5f3526c0621eaa42ce6e8",
      "afd0af9a11c5ae2a7c4a4571ce39ad57d8df70ef146ed83ad8eaff97b2387fb8",
      "a1f8fee9f1da9a2b306489d00edf754187b55e97f4fe6f249432fe6c7f44d6be",
      "4f12e8a123465a862060efb656299e6bef732e5954b02194308817b243e84d32",
      "6a1ca62f7d6952ad2eba1c64035260319baf03deabf856ca860744fc886b3d3a",
      "c72dd1fe890d6e4c1f7325a4f224e85aef6cdca8bf9441d228efaf126e02ba63",
      "2f6ddcea18d891ef4252e657989de68adcc43c2175d49c0c059c5e49b9dd5aed",
      "24efac0f240ed183c30398ee40307623f52113598f66c5864c90fc62643a2aec",
      "6ba3ebc935e7cf7fbb446e7f5c12b19c4455e6894412b0eedee4fc945e429e9a",
      "3519d6e5bc9649f97d07a07ef5471a553ffce35c7619f4f63e91a2ba38cbb729",
      "65e073df352fa9917e5c2475167e6c523b68c1406e1b6e81109e2d4cc87c740d",
      "d73bf816c3648a7d53d34be938c454e515efb0c974d5a225f33635540b2db90d",
      "bce167790fc86a273db011757d09e2d1148521ce242c2ded58f26cc49993aacb",
      "2d4286ed4039916f29602e86f47ea4c5b05998c0198509ca7799fcadfb337e8d",
      "9837c495b1af4f76b09177514a0f3e1dceb509c52b91712f3c7d29dc1b91d09b",
      "5c848b8291f39759903ce77f151acf40f3ab5afa2d4a45af62b320371c29a697",
      "b92df5016ee947ce6a21365d3361977f7f2f6c14025a983c44e13d3e8cc72152",
      "71d2f57222a39b1a7ed2df5e6fb23a964439b5a8e7d49b49d87e5cd5354baa75",
      "88b44d0198fb15b0c20a97f87e021c744606bfd35eae2874f35c447aa4ac3cd4",
      "29bb4c2557714119cd684da2867e689e37e3ca9c912db83ab84746816f6092ab",
      "b1836d98a288752675b133b9018fa1edf174f311921d01926c1e1a5900c21029",
      "a00645e090c7d96f3155ffbcfc41e526a763b0f53a98151ac4a7d4a5b14066b6",
      "78aab09919d17773b0d62799b37bd2e2970f72f5d1eb6472489c367b6135374f",
      "eb6123aeb28608f1c97b2bf62ef69f977cd0658a0ab491deebb1e972caa938c5",
      "8dd7ef1650b1b30cdf7814ae4d61a237eb0acc3ec3ce0f72b1c25780329c2d7d",
      "b1998419be3172858b990eea84fe10bb24b76c219cde277cb4305955fc7e0b65",
      "1b10560016c4bc506eef9056dedc2943a17179081e6eaf85b48d37dc20eac3cc",
      "1fb1d9d4d408a6734234910f554d272739a0d6fa401918d79b57be62c3f23ba2",
      "dec878f54ce36788289b61d32de0d9539032aba22cd15522752f729659c7cc5c",
      "fdbfd0773f5a66637b093dabf812197940d1134619a7e60a931b19915b7dab0a",
      "21bd2c9aae052a1c187947d2964f2be4afa7b30248409c41a75522e88a4e7977",
      "59326adab03416ec1d36699c18e5e4fa13ca1f2212d48c23bfdecb0be7263616",
      "bcf263d39457c0aef8ef42fd98f8131198ec4fb203155dd5bcd759e499a9ca5c",
      "f1ad083bcd8c7630eef93336d8a971ae8ae37166a6a00ac39684420c5f9afef8",
      "d82ee2ac41b36e3c1787a01835bf054070486dc20f6565efedbbc37cd3bf5fa5",
      "eba91a0dcbd3986299b0a3e66c116f77bd3421829291fd769522f01da26f107b",
      "11016558b7e8c6386c6a3e862691dcba47e33e257f0e7df38901ea7c0eba894c",
      "04f02795e34a0030e5186c8e700da8a46b1aa6bc0abed6b35c9c1cd6a73776b9",
      "2628dc8ad7fb731d59456b2755a99c6701467125fa69816c21bfccabc31edf6b",
      "9b7ca249ee5b45cd264492f30df09f708a7d9caed7beb9a5c6292f22e4c31f85",
      "5c42e7caedf382092faaf392174792b3cf5f2fe29cb586387ee55611af0617c9",
      "373f2fd5940a01feb79659c8b9099477f6d3e7b570ebb749c2ac336ea4be583d",
      "fea22887147adc3a659a14902080b03e86b4b8b16985fdf0bbacaed00d812422",
      "6a3e51a1443cff62af9fa12fafc8ea78ae74dac7792c9ae0f436f570ab33eb71",
      "796be21e213d6d0cd6fbe2de1555fb73d1cf9edc041a9f1ff1ad583c4ca92460",
      "03fcbcb31d3fd17f0eedb45ac5a51678c7c8b2b8498225d05f74e2892f017f72",
      "d28da07c6c22daf9ae987b3033c33d3140e5a56fa1ffd7dc5c7853d55a45bcc7",
      "fbb0ce02f50018741a12fc08eea80a18067d7bb0fcd96153d40bb8c808473aae",
      "2bf7c05a0209b4ea31314f04bd754cd01c58102d7cde8c496c655b6494924354",
      "1968a9e6e14ae86a1e02e6078fc4631851fce5dbac6aa34f860defd1ccfd0ded",
      "d886181329c9e06462a1407f547d77b38ff2c868b86d8976aa478e1cbb3d66d4",
      "0d465e02ff2f8eb0b5fb2fa9a38579c5d66337d4a16b58f8ed28d2d89fc02392",
      "3196419015289807880ef24b6781734822d654dc260c0560d00bac65eacd5219",
      "fa08390ddc333a2a12248d5ec3e51fff9b782227069fe5a0afbd8eba967ae8d1",
      "49ae36a791cb84516688d59a1ed3e5112851d65f265078aa2d433b45fa984c8a",
      "35daa428e12c59da6730760979aca3444d8b31149c6febd99fbfefa4b2372082",
      "5ef1d697beba612ff31d1dc139817c313a4e2ad3977775943b635c141ef0f2a1",
      "674256037ec00edb66b9355fb1d33a30a47a5d1f4bce2dd5475d89f1ea6502db",
      "7b6f017bc550933af91eec32a79464f540c5e0c208703e175843ee4e9ffc0a50",
      "bf0eb91da1d18dbb18fd9ff36c837387887ba142961176a44889718b2becb9dc",
      "3e5ac43a05164b074a2ff6353e9882917c5a3dbe168c2695de895f7decf1a56c",
      "35e8f004965347c2b18a000a83dd3a8084b8a4bf00522061ed1179aa1107c413",
      "fccb0fff3a24e555ec96f702ec11d420338910d148fc7b039d67568ad3a0e032",
      "5cab231048032dbf921b4fafa1951dd2da93bc3740667f31d5a1d5665b141261",
      "ffedb24be73441fbcd069f7785ebb865870e0f3ed228190732e4ffd5962bb82d",
      "a4fcfec18adf92f4ed822f64d2da9f5ae630885a1bfa626530f641db99aa7a30",
      "f98bcee41b0e3deafa1efaa1863750dbfd9bd7430b82529b670867d852230b5d",
      "8ab8d5fca047a52364a737c1af57bf656c9ad5049f08ef4c5aa252e61aa72123",
      "91318b39ad94c1d58143586b6d90dd6092a9d7487e321f4976967b6ac445ff43",
      "fabfbd4569ab018e12d5ffa9b1a40ca8eb2ca60a685817351d90eaa770d5eccb",
      "bbc5ef34428d980e2401942ceecfe07cdf21bfb1acae0596ea1d43fccf504f69",
      "26943e4201ea407a5667103fe07ca6e08ef76940f274349b0e2e776bcfb0acb6",
      "e3b305ffe33e72841f8e2a8688cc5cc27d42aee7624b33b7b6399b42db392437",
      "17c5a763dd57e6bcc7c4cf2db0eb5cf3e97116b67fe0dd519c97e4a4d55d5a62",
      "bbd260216879ce86af8318ffcf73c9e063ca76dd8bc35d3b6be45b2b4184888b",
      "41285591d0595bc42ab663051b410d51af39fe1720592e27acb1a8af72360a76",
      "f29acd6068ce494d0c0fe294cad91bb8968e3fff3f595a113227ab545c3ca3e6",
      "ec9013c6394528e7dd788ce7cc085ca79fcdfbb37565999d5b4b5a4e39452ecc",
      "27829bd7f1a8fcddcad0cc34a3b3fc67d62a2f3e09f8e75d35035c2281e83afd",
      "666bea9db4e15087204d076294d221d4cf5864f5d94de38f29132b1934a17ace",
      "a3a30924cad3dbda3446e5a6324e0a1390c70f795d5ecfe17ee5c70b14f7d87d",
      "19567fe5fdb10711d60aa4d9843e1c49c2a6d2fd1b5cf662e2105606bb7815d3",
      "b139f1c3a2f15596b9320334e37e4184d5d584c4a81e72d821a7edcad3aa62d5",
      "08f1531e0e3e8f8bae313b2c60a72d5601bf8b60d7a4d2f60e8481650340d250",
      "c5895669e1ff182bf1dd6c00dc955265e08ded0952b8ca62a1c63ba11c99f4ca",
      "84d1c28153f66c1a4eb5fa0df695e936d83294df31a08d8d8e2d4798d19d8ce0",
      "b8699f6af853fdbe897848feb46a05d733363f304eac4c8c1932e6ea4bc062cb",
      "10eb3f6c1d0661468d9ed83593e5e9c0b43c6feec6a5405a498194905ea6ed48",
      "509e215a600d9cadcbf5d62632ba321d7314764218db00ce8c907e425fccc445",
      "e62119b7be84c8eaad41ba7f4a35da403f4ed96b967a3134e89ee8b78f8792c2",
      "f790754a95d59ea5ffe6b9b5cc386c600a9e19e8bec997c192764365f1d05376",
      "990121b5aa4d6badfb7154db4cdbb4512124bc2f442bebac71ea90b5cc86f203",
      "b6983dedaa891eb14c964d84461e5cd37ed27b61771c64978ba83e3ecea194fa",
      "00fba1ceaa6aa1e378cd5b22a771d6070250ac37f4e17d7bf1a70b3139e9a860",
      "429854e7738abf2ecf46909454039e2fc5a080eb9a3c0c5ea13b07426dac3ad9",
      "ceb3e017944b0dd787be219d8629930b3f2e20e22b12dc88fd838488ebb896f3",
      "eb9e5d14424c63e675fe771b73ca865f7d38cf334d65e2426e12a0b88c1a2236",
      "556ee713449e6e59ac4b8b2e8310180c8f6280484e9db23456526cceb9216168",
      "bc89c3aa889e0144ac526a1f861227430dde7e439cc6a7e9b25c9a049c3ca7b3",
      "56d070c62ea99be66fff741a8e45fafda5f9ff783e84d5395b251f356ce4e16f",
      "ace15859c399e5ecd13b1420d3c3703c6a88dfb4a084f7225e7ba40a4b444fc8",
      "f03f1261ab6eb879fe9c5b0028cd400b3ffdfac4344e4c75f6cde3c05ded1f26",
      "955b2fda8d0068226f89270028b316b5adac871f1c1c85435479aba14a381b0f",
      "422509a98d7461a6b8ec87cbb173b2486577b59ea9b269e01c20567b38b3b3b2",
      "007d4de62ad89a4f5985f0cd9b76a7293acf383b4e9e734e813b9df1d57f986f",
      "13a04e32948225b7e22aa0137341ebbb811e0743032fac16be9d691a652db2eb",
      "8244b11d880a52f9f9e1830a822e6eeeaf0b12fc759f8261bc2f490cb0794f3b",
      "27d3415f8f8fd3048a1ee0d9487256dd1b0f9e31be663778efa5b3add63868ec",
      "0053f888db916a8905320e253fe2f0666355e6fb6de36f897231920a3edfe39f",
      "0bc5c0a2ea47fa3bb8be107e3b9d6b7226b1c8bd16ca1bab8f51b8e1de08aa8b",
      "4ca13aaa161c79025b5cd6c9a8ac33554f5ceb479fe293d9a342c865cd9c9948",
      "333afbe82e2a3df38bd1ef998f39c5feef2554697aa21b5410c0e95ef9156249",
      "587c4fcabd18ff890064171fce3d5be0c4aa1bba46893fb6a827a85ab54d20f3",
      "964328e4d51d67c4e2f1fd102a66b861d98199f81d18d660b1b4b52504cd772a",
      "196aad5594651efd679d30b9feb0f0d172cf977b4f89aa670ec741a8bf21e669",
      "9137bfd66bbf47bfa0bfcbb9f6e71b6eb3fd9776530e9fd30d3dab19e182f35d",
      "8217392c4ed2313188f295d94148a027a7f43824a5f5fba33a0b8c1045d509b4",
      "be9e12761519a4836e73015544163254877e1c4912fcea67a10e7026421dde75",
      "7b5220421a520b876cc6cdba0d3895104d7fac914dca5b93f9fe8b226566b71e",
      "5c83fccfeb4bf0eb8a94d43ebc84a81639a32f26c7ef77d0a2b626b7de7befdb",
      "132fd6c92cf176f975efdb5ded53470b462a48a2815c6f54a93ce4f935784cc7",
      "46a3dba364022d11aa616a2bc20e3be5c4390f38b9446edfa464d90d9af5d48f",
      "34b3f3fd8a83905a37762060f51d0b116377b4820b031b8e668e16f46c5b0285",
      "f0e397e033dabec859a4b9a9043c5f1fb0dba504764d6bcf2fe9bf2ffd318474",
      "85ecf59c7dd3b24ad17f591bc4737f32f1384c370a7a6f2add06a811dc824b6c",
      "4d03cdb1e6ad8e066a83654626d8c221433e8d4fd901c671300af37e000177f2",
      "61cb9c651893e6401b25f2bdf79c9f3ddc9ffe69cf6c791c420462bd73e347e1",
      "85f2686a42158cd5ad327781ecccd1bdcd346941dd4b4edc45f717de6a011800",
      "92de2ab82cac528e6d4ccd61e5b7c79591dcad9909c8ad3c8276ece6d48c0f08",
      "23a878a06bb94bff33083349149f3c860f2b00bc3fb28f04cbaf717c08af19a3",
      "1b1cce18ff0323566b192885d7ced30f9a9531a2580240f2c593a7d5b8580974",
      "08fcdec7ea1376d84f3b13a47a4b73c7781c9c7890bb28f712b58af4fd3f24fe",
      "03cc08fc4ece807c6495272c412be23b045622cc6b786ed8d5c94156ae678a0e",
      "c4d8a61dc3f5dcf4b83f27a90cbc37e816cf4754e12309626ec5679c99087c46",
      "b29d00681e29001cdd63c4bc50e5e25715faef692aeebb678c8050e1c095e888",
      "ac154617e93f2bb1afa232675f2135437a9cc9700c14c51c40084946596ba11a",
      "ce9549de8e68ae89f424dd9e1cde8a4eea2069da667cfcfbe837691d37366668",
      "426c45a98e2af35cc9708149f6c086ff5a3972e77d62c627d5e20de5d731cad8",
      "7e21bfe240a3d9b77a129c734a1d428dbc890379fbaf862853f48b2f7470b2b0",
      "fa090a71f77223a7210de6db18d9aa809e89fb15253aea28131df6c5a7639140",
      "7094ad044c5ab025e088b43aa0b947601fabe58ed700a412fd96e4b917ced0c8",
      "936d5cdc4f081b6fe36c356af4378d472cd7990303f2ea44da645afd7d5d7f9c",
      "05342037d3b69349dce7b95529d4b2a63ceb9d9393217a68f7cc8c958a96c3ea",
      "ff9e1c414ef27b1178b1de296526f50520b7ddb06286bf9c47792bfb449e40b6",
      "2f2b7bedb34d2854b17ccb702cddd8bc0157e39721d58be0b2ad54ee291fc9f1",
      "0d8db1f34140bbf7eb809137018a74af08cb3345b8a3e368cdda8521dab45791",
      "b109e4bfabcfe4a1c38be1156d9ca851c75e6aa2e57c0869e40cd9056f571e07",
      "5cb363547ca077c806fc69bc8c2006831ab89e72fd778ac1a48fa810934e350e",
      "85ee928bb110fd64eae54a91fc8548883e7fc4c60a3c61b505c31cea2d295c86",
      "1ec3df7d10ee6fd5f0532ad4fe771e6befc28b0bed0250bf523695d6d49a8246",
      "de9db2fc07c866bd7b885fb41522b63d550d0ce2e8ac5e14464a41733c2319e6",
      "9a27136422a8f56768db29ba172a7ba26c3c7aa910324e78e5ab3a3268ac3674",
      "60213c315119bf9005cb533d1a5b403b4a13c59982fe7773d30fdd8f519f4205",
      "40eb61ef1812eb8a4d389599bf449fc86653b2c4986061b952f46fc049de53f4",
      "658ef0d8140162b5f04591be13b47456245f531208bbca3260b857ca09b803c5",
      "02270fa66255048d724894e2206b4e773cc6a7b6d17ca090cdc25f317d5f53b9",
      "2ec6a0147f419161f7198d05be5f93152d9ccc10672db0ea47ff1687c0f0dd15",
      "4be1d8ceb96eb80ef7ce30079ded31163272aeccff5c18fe3aaa32ea2f5bed9d",
      "04ecaf48f44de87243b17b4c71ebff00020738639336010fa57435a54b623798",
      "e313a9feb7cfd1d56ec87b1f1062ff9a80da498f7b761af4bef0cecd1b4c385b",
      "ede3748f971f22341f7f5844dc60fc03cdb30c7cc720ebe13ae588c17a78aa94",
      "d90c0faa70e39b7c0a8c55457ed6e6478a4e4bf3707b08104326a1ee8377c3ab",
      "c79ffd0bbc8d004cc542e212990df6498abddd3deb50fd00ed00a2ff690974d6",
      "35c37d88cf73a89c4124b0ee537347c37fdb47156c8b0ecc509efc58236ed3f0",
      "a99182f343ccf05e557ffa6df71f03688b2afcf314c59daa774fe78db6f47add",
      "01115397a78af8a4ae2727ca7a01843235b626bd3db80888d3dfd0020d4135e2",
      "4a55aced578470d2f7280096d7fe8095f294095fba4778d1977d6db9270472f0",
      "4624adf8a5633f65b213b8ca46b55cb0ee36c41495f39b1ae70cbd545779b1a0",
      "d72bcd5b57a9c47e7bd5e9a1103657d10beb7b6c6d41f2b2985bc3bd3cc74860",
      "48baadb9a46293c92f29e7617846171356a42c3b5d18d49a05a7e173993785f7",
      "3da927737af8cb0e1c77097e35c54158d18aabfb3051c45bcc7ddfe00b157b1f",
      "b4a24bfdb2cb802c8d48a3a18fbfe18622a767fe7eddfca57d4555550ccd1643",
      "c58f82ac7c49dcba1721a88358f07636c9df60d3fd383e5789b808dc57a1dc9d",
      "5e1f756eff5155df073d30f4452bdafc4adaf4f35960771bf2c1e30137fd7a79",
      "be4a332f289338d67bd4834eae3128c488a61d255e972da484b6252b67a46b89",
      "d496e4a36238d03a83d8b45cf33d9388aa7568a279b034d1cdd87b457356cc5b",
      "a1c5212730ccda34de393210e276bbd44720dda777bcfd602315a3eee582f7dc",
      "08914ec63f6ef7fe1d678937dc0f6178883440b26b4aca29fce79068947e8397",
      "49e2cd2bd9b974074d9814f93eed371620bd4ea5fbf97a625065704e8fb382d7",
      "047c194111818b48ce93a4b006e4a09b9a2650757a87357111796e11e847bf23",
      "5955b0baa8265341f35a6f24fdc79066ba3ca9c5354c69b6b37a9ef3a26be556",
      "d7c962f3ea1938c5267cca4072548acf3afcce4d438ed62027caa211a5f98e8b",
      "c8cfba67cb4ce7e291b35154a50476d9a5c6bbb5d6cbaaa5d2408547fea7b02b",
      "de8a940d8a69a64ce264d2ab7320662aef2e391c587cb2aae22a86718d5dffbb",
      "94176f1310b26e54d4de48f87b74aa0b60532f184a2268508dece86dd7f85d36",
      "9ce6ee3fca56c9256a69df404782301300a6e5e7f5a25a1f6d68c0e9e42584c9",
      "7c423c4a220c6ff43ab6432f92b166323c58ee77f8c096ba0b00d52d7bd507e8",
      "0b62b9c1ae4d4988720e8d41d980b334458189de0a3dd01699338d7b07c3894e",
      "64f45f6f75110624506c53716f2fc1d5fe5f88f82a5bc6a7459ce70eae56dbb3",
      "12ffbeb8e52fed161af4d8a015d1a5c45dcb8240e5c8933ce3a88ba2c58f97e8",
      "2ee6b7b96043c8ded9fb52f87cdd0d0580ca6f8cef183c8a656394a11c0aa393",
      "aaef26b1f5726258bf9ce305a3e54bca65cf68779f90f9d24287245c27362e27",
      "ef59588dce57c35d010bea4d209f44c62f0b7c7e65bc0226c0e4971934da9435",
      "0e606c2f6f8dcd579faf4739312bd7327ac7796fa44a81780fe0d66fc7761fb9",
      "2f307198afdbde5f95989a17e06ce1bb9ff36c441cf3b2248431534fe13bb9fe",
      "51418e6df23d450aaacc74ef2df53a6b1693727b70bda9ebc43acfc23d8fb5ea",
      "6e9e3ac46705ed80520695b924435b00d2b3079598bf7faca7fd1524be777e5e",
      "1e96241e2876aad29ce64f5d7e7fbb8db7265449df816c0d30a96633778c5cb6",
      "81788f00eb72696d811f946e65d2c96528c45590874a1defdd46651e9b79a3a1",
      "d9aa5a9f1df50e933d7105a5d72b5fe96bfbb9fd4b5b0eaa5e80af12e72d497a",
      "e1b6976a732d27fc5d6a96b6d3d0d1d5eaa6ec46bae4665f17e7a43aefc75280",
      "9151c75edcec1cc90aa2d2c240ff657b0eef3f5f1ec37418c8854b2493114f1d",
      "3e7c12d0132421f08ea0a390cfa325e422a6b35120fb2eb650f108a165237934",
      "1ee0e85c7d8a91089c03f37318cdc9127026bf789e3ce4b75046eaa3eebd3458",
      "4ae64a3ef66cad847409ce175bd5365c9097fd21647a05730ac6b45841add3c8",
      "f0ea0f334cf1d64678a6dab08c07e2f94f339e8389bd17ebc882b5c8b736cbf2",
      "da904db96060546ff69e28993ce8183766da9402ac10fae9fc1f1d67ebd83c90",
      "db11820615f7b5e47778c45d2e083e77f49b608b587dc09ec26f077aba07a242",
      "ff5c726a83bd785484de75bb03b421f9e8e382bf2740120a2fcf72326aa01c75",
      "f8643a7efd6304980db323303ab73a6fd4f4ee1047520d39d571580395b97f21",
      "8facf9737d07838aedf6030593bfb247d8c29fe8d9b18b2913408627a4424d7e",
      "f0672964aa6e4c7dd4768e18827023787386927f4db89fd661444979afb43c18",
      "6fd3649c8401f2704ed2be18518b870eb6bf2b9a6689d1b336f05bd8b49017f6",
      "ed172dac7de827493e0c0fdd8d3299333acb678e72ed499e0224b389cc1e0fba",
      "7f9a8a8cc8e34add11934a1a5882be5978a6d28405cb0a053ec5699a502b1cef",
      "6ca829ebd2a0a40994f68c1db7978ec274b45c46e9b351df869a2bfe0630bbd4",
      "0bbb017c437573a55db88258a9d9a01188bdd23bb6b26903b137814871661f47",
      "9a6358d2541f46b6d05b80fe25a2cb025fbe9e4b227a6275908d5ee31c948569",
      "a75d26c6d4ce944024f10e6d23e8b5b888d680120e15dc0e4fee8d8833ee0c6a",
      "1d43d33556699b42c124b46e41243abf48727fe488428056fdd174a3861c1e3c",
      "7b5bd3fecbaa093f005c4f806ea67846c0df6b04df7729925cb14724f6a8b582",
      "6bdf2b54f2f5ab90191261d33dee80fede75896994016422b28db8ba62327d82",
      "1a16181b250085a91ccecc118473fa2ab98515e894a7b63b347c24b5be560c7a",
      "22d24891755910b48ad632358c26245bdcc375abc41f7e2c9fb3c7773dbf4e22",
      "5b70c5d4a373d541619c944fcc3b61259550b0e9fba3eca16f0879e5845b43b9",
      "b78f9098c9d76987b7409e63426a8d49972bb4e75289576c680cf96513d44b6d",
      "916b53b8e85eb7e0a2a76d6fc8d2163430e7183ccb103d6705f54af4bb070907",
      "dc3d78f43110d2aa9df83c5485ec33663ad5452b8cdeb1aeeac9d6b1487fb781",
      "3975539ed5402cb9f5ab503584524dd141cc4296b666ec66d807f94f62b1c026",
      "bd1f97fd89183643423073f22733880616456ca41960699f18e868cb9ac35508",
      "90b468ff0f83460c3dd8cfe778c39d32c6bb1eeff9ac5de7804a4050d3b8073e",
      "0e886d49d88b82c9f8dbdd2f38a535992f35ab16629724d746394db3898235fa",
      "76c22e965242d1ca5614e829d9028dcad9c4b09393bcbdd318b0365557335fb9",
      "59b168488ec8629f820a1efd8fc5a0c2adec4253e61d6a2945a68a9a43be9035",
      "ff172b42854eaf2865caa985d2fb6283c5ab19574126623ffd615a761a5bce72",
      "cb46ac9ccc024ee74c96e3cf1c13a6949a432e855dfa881b6a307c0e6daac59e",
      "9971574924c0f413bf4c0f96bb9c2fbdfea8f475e33a8fb6f15fa40903b63444",
      "1a95567deb0f45a8941e2248f33286485984a5e9d86d16c37d42169ad864dc36",
      "205fc7f7ec7a83f0bc22d5269c91762cd00adc7428456d799be5a0cd76f08b0f",
      "849dd41ef59a722901b7a0deb2c1fd3c110a91a726120a0a119cb7a15cf98438",
      "a32880917c714612101af95e5c8d8eb5fb046fdcc68bae76c05b829b3fa73c2e",
      "70b38d6d510d13b359dffa910329952c620a4bce4ee7a8552b9bb3a14572394d",
      "ef257ab2f4226faa6ba288a6793f026609068effb866c18496a847e8b60b102d",
      "e5196ab42ff53c8352288bde6b6b7312cd6f39f7d21b556b0db178d8470d5790",
      "ca98f128bf085f2b718f2b3c12da7c4d98887cc94251a2b1705b637611bd83bb",
      "79508f0b93a49ec19c5cb05906ca1ba3d3db8ed4f9c6884873d0d7e3e985ea51",
      "9088be3f47f9debc63e928739f7163182b49eab044518b151f0b89f6b6aefdd0",
      "46b2782fd669b6288a4d7348cf6671360277ba4864cc69bce3497369ac2ec31e",
      "0fa5131557db67b430d516530be939ff25882adf68a076602f3dfad8c77c963a",
      "3404302cc097d5457244453a4c9990804201ee8161188df811bcb32404998c71",
      "856939710dbb90a8eeda875a31f9a52af759bd932b88e7b08df35414c54d4721",
      "72569573b9b41d0ac5ce17764a139c6b8b36ef3ca6d92cec625dbcdae758ba22",
      "9746da344e435a008d6acb4847211bb676376ecc76c825b5d44a28b89ceeb40e",
      "3eafded1595516f032e33ec975f4c9c3a1055d13aa5575cf8a801d6103fdbeb4",
      "e88a6d2daa863c0787cc523a2cab45c546fad788951b10d75e2b0954db24cca7",
      "38f531e67f88f66de44d3357c8e8f2db456160ca31dd2024c9562f6afd260278",
    };
    // block 685498 (13 key images in one transaction)
    static const char * const key_images_685498[] =
    {
      "749b7277aa21c70c417f255fb181c3a30b44277edf657eaaebf28a2709dd2a90",
      "5a9b3e1a87332d735cedaa2b5623a6a5e99d99f5a2887c8fc84293577e8bf25c",
      "bea438768445eb3650cf619bf6758e94035abfe0ccda91d4a58c582df143d835",
      "376e237ff4da5e5cbd6e1bba4b01412fa751f2959c0c57006589f382413df429",
      "14ac2f2e044f8635a3a42ecb46c575424073c1f6e5ed5e905f516d57f63184b5",
      "2d1d2ecb77b69a2901de00897d0509c1b7855a3b2f8eb1afe419008fc03cd15a",
      "ea01658f0972b77ae9112d525ec073e3ec5c3b98d5ad912d95ab2636354b70b6",
      "d3934864a46101d8c242415282e7fc9ee73ad16cd40355535d226ab45ecdb61a",
      "ee379b05c5d02432330ebd4ea9c4f1c87d14c388568d526a0f8a22649a14e453",
      "aeb7b842b410b13ca4af7a5ffd5ae6caddc8bfec653df1b945e478839a2e0057",
      "451806929d9f5c3a7f365472703871abadc25b2a5a2d75472a45e86cd76c610b",
      "272d9b9fcc9e253c08da9caf8233471150019582eaefef461c1f9ceff7e2c337",
      "633cdedeb3b96ec4f234c670254c6f721e0b368d00b48c6b26759db7d62cf52d",
    };

    if (height() > 202612)
    {
      for (const auto &kis: key_images_202612)
      {
        crypto::key_image ki;
        epee::string_tools::hex_to_pod(kis, ki);
        if (!has_key_image(ki))
        {
          LOG_PRINT_L1("Fixup: adding missing spent key " << ki);
          add_spent_key(ki);
        }
      }
    }
    if (height() > 685498)
    {
      for (const auto &kis: key_images_685498)
      {
        crypto::key_image ki;
        epee::string_tools::hex_to_pod(kis, ki);
        if (!has_key_image(ki))
        {
          LOG_PRINT_L1("Fixup: adding missing spent key " << ki);
          add_spent_key(ki);
        }
      }
    }
  }
  batch_stop();
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
