// Copyright (c) 2014, The Monero Project
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

#include "db_lmdb.h"

#include <boost/filesystem.hpp>
#include <memory>  // std::unique_ptr
#include <cstring>  // memcpy

#include "cryptonote_core/cryptonote_format_utils.h"
#include "crypto/crypto.h"

using epee::string_tools::pod_to_hex;

namespace
{

//  cursor needs to be closed when it goes out of scope,
//  this helps if the function using it throws
struct lmdb_cur
{
  lmdb_cur(MDB_txn* txn, MDB_dbi dbi)
  {
    if (mdb_cursor_open(txn, dbi, &m_cur))
    {
      LOG_PRINT_L0("Error opening db cursor");
      throw cryptonote::DB_ERROR("Error opening db cursor");
    }
    done = false;
  }

  ~lmdb_cur() { close(); }

  operator MDB_cursor*() { return m_cur; }
  operator MDB_cursor**() { return &m_cur; }

  void close()
  {
    if (!done)
    {
      mdb_cursor_close(m_cur);
      done = true;
    }
  }

private:
  MDB_cursor* m_cur;
  bool done;
};

auto compare_uint64 = [](const MDB_val *a, const MDB_val *b) {
  const uint64_t va = *(const uint64_t*)a->mv_data;
  const uint64_t vb = *(const uint64_t*)b->mv_data;
  if (va < vb) return -1;
  else if (va == vb) return 0;
  else return 1;
};

const char* const LMDB_BLOCKS = "blocks";
const char* const LMDB_BLOCK_TIMESTAMPS = "block_timestamps";
const char* const LMDB_BLOCK_HEIGHTS = "block_heights";
const char* const LMDB_BLOCK_HASHES = "block_hashes";
const char* const LMDB_BLOCK_SIZES = "block_sizes";
const char* const LMDB_BLOCK_DIFFS = "block_diffs";
const char* const LMDB_BLOCK_COINS = "block_coins";

const char* const LMDB_TXS = "txs";
const char* const LMDB_TX_UNLOCKS = "tx_unlocks";
const char* const LMDB_TX_HEIGHTS = "tx_heights";
const char* const LMDB_TX_OUTPUTS = "tx_outputs";

const char* const LMDB_OUTPUT_TXS = "output_txs";
const char* const LMDB_OUTPUT_INDICES = "output_indices";
const char* const LMDB_OUTPUT_AMOUNTS = "output_amounts";
const char* const LMDB_OUTPUT_KEYS = "output_keys";
const char* const LMDB_OUTPUTS = "outputs";
const char* const LMDB_OUTPUT_GINDICES = "output_gindices";
const char* const LMDB_SPENT_KEYS = "spent_keys";

inline void lmdb_db_open(MDB_txn* txn, const char* name, int flags, MDB_dbi& dbi, const std::string& error_string)
{
  if (mdb_dbi_open(txn, name, flags, &dbi))
  {
    LOG_PRINT_L0(error_string);
    throw cryptonote::DB_OPEN_FAILURE(error_string.c_str());
  }
}

}  // anonymous namespace

namespace cryptonote
{

void BlockchainLMDB::add_block( const block& blk
              , const size_t& block_size
              , const difficulty_type& cumulative_difficulty
              , const uint64_t& coins_generated
              )
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  crypto::hash h = get_block_hash(blk);
  MDB_val val_h;
  val_h.mv_size = sizeof(crypto::hash);
  val_h.mv_data = &h;

  MDB_val unused;
  if (mdb_get(*m_write_txn, m_block_heights, &val_h, &unused) == 0)
  {
      LOG_PRINT_L1("Attempting to add block that's already in the db");
      throw BLOCK_EXISTS("Attempting to add block that's already in the db");
  }

  if (m_height > 0)
  {
    MDB_val parent_key;
    crypto::hash parent = blk.prev_id;
    parent_key.mv_size = sizeof(crypto::hash);
    parent_key.mv_data = &parent;

    MDB_val parent_h;
    if (mdb_get(*m_write_txn, m_block_heights, &parent_key, &parent_h))
    {
      LOG_PRINT_L0("Failed to get top block hash to check for new block's parent");
      throw DB_ERROR("Failed to get top block hash to check for new block's parent");
    }

    uint64_t parent_height = *(const uint64_t *)parent_h.mv_data;
    if (parent_height != m_height - 1)
    {
      LOG_PRINT_L0("Top block is not new block's parent");
      throw BLOCK_PARENT_DNE("Top block is not new block's parent");
    }
  }

  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &m_height;

  auto bd = block_to_blob(blk);

  // const-correctness be trolling, yo
  std::unique_ptr<char[]> bd_cpy(new char[bd.size()]);
  memcpy(bd_cpy.get(), bd.data(), bd.size());

  MDB_val blob;
  blob.mv_size = bd.size();
  blob.mv_data = bd_cpy.get();
  if (mdb_put(*m_write_txn, m_blocks, &key, &blob, 0))
  {
    LOG_PRINT_L0("Failed to add block blob to db transaction");
    throw DB_ERROR("Failed to add block blob to db transaction");
  }

  size_t size_cpy = block_size;
  MDB_val sz;
  sz.mv_size = sizeof(block_size);
  sz.mv_data = &size_cpy;
  if (mdb_put(*m_write_txn, m_block_sizes, &key, &sz, 0))
  {
    LOG_PRINT_L0("Failed to add block size to db transaction");
    throw DB_ERROR("Failed to add block size to db transaction");
  }

  uint64_t time_cpy = blk.timestamp;
  sz.mv_size = sizeof(time_cpy);
  sz.mv_data = &time_cpy;
  if (mdb_put(*m_write_txn, m_block_timestamps, &key, &sz, 0))
  {
    LOG_PRINT_L0("Failed to add block timestamp to db transaction");
    throw DB_ERROR("Failed to add block timestamp to db transaction");
  }

  difficulty_type diff_cpy = cumulative_difficulty;
  sz.mv_size = sizeof(cumulative_difficulty);
  sz.mv_data = &diff_cpy;
  if (mdb_put(*m_write_txn, m_block_diffs, &key, &sz, 0))
  {
    LOG_PRINT_L0("Failed to add block cumulative difficulty to db transaction");
    throw DB_ERROR("Failed to add block cumulative difficulty to db transaction");
  }

  uint64_t coins_cpy = coins_generated;
  sz.mv_size = sizeof(coins_generated);
  sz.mv_data = &coins_cpy;
  if (mdb_put(*m_write_txn, m_block_coins, &key, &sz, 0))
  {
    LOG_PRINT_L0("Failed to add block total generated coins to db transaction");
    throw DB_ERROR("Failed to add block total generated coins to db transaction");
  }

  if (mdb_put(*m_write_txn, m_block_heights, &val_h, &key, 0))
  {
    LOG_PRINT_L0("Failed to add block height by hash to db transaction");
    throw DB_ERROR("Failed to add block height by hash to db transaction");
  }

  if (mdb_put(*m_write_txn, m_block_hashes, &key, &val_h, 0))
  {
    LOG_PRINT_L0("Failed to add block hash to db transaction");
    throw DB_ERROR("Failed to add block hash to db transaction");
  }

}

void BlockchainLMDB::remove_block()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val k;
  uint64_t height = m_height - 1;
  k.mv_size = sizeof(uint64_t);
  k.mv_data = &height;

  MDB_val h;
  if (mdb_get(*m_write_txn, m_block_hashes, &k, &h))
  {
      LOG_PRINT_L1("Attempting to remove block that's not in the db");
      throw BLOCK_DNE("Attempting to remove block that's not in the db");
  }

  if (mdb_del(*m_write_txn, m_blocks, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block to db transaction");
      throw DB_ERROR("Failed to add removal of block to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_sizes, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block size to db transaction");
      throw DB_ERROR("Failed to add removal of block size to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_diffs, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block cumulative difficulty to db transaction");
      throw DB_ERROR("Failed to add removal of block cumulative difficulty to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_coins, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block total generated coins to db transaction");
      throw DB_ERROR("Failed to add removal of block total generated coins to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_timestamps, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block timestamp to db transaction");
      throw DB_ERROR("Failed to add removal of block timestamp to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_heights, &h, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block height by hash to db transaction");
      throw DB_ERROR("Failed to add removal of block height by hash to db transaction");
  }

  if (mdb_del(*m_write_txn, m_block_hashes, &k, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of block hash to db transaction");
      throw DB_ERROR("Failed to add removal of block hash to db transaction");
  }
}

void BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  crypto::hash h = get_transaction_hash(tx);
  MDB_val val_h;
  val_h.mv_size = sizeof(crypto::hash);
  val_h.mv_data = &h;

  MDB_val unused;
  if (mdb_get(*m_write_txn, m_txs, &val_h, &unused) == 0)
  {
      LOG_PRINT_L1("Attempting to add transaction that's already in the db");
      throw TX_EXISTS("Attempting to add transaction that's already in the db");
  }

  auto bd = tx_to_blob(tx);

  // const-correctness be trolling, yo
  std::unique_ptr<char[]> bd_cpy(new char[bd.size()]);
  memcpy(bd_cpy.get(), bd.data(), bd.size());

  MDB_val blob;
  blob.mv_size = bd.size();
  blob.mv_data = bd_cpy.get();
  if (mdb_put(*m_write_txn, m_txs, &val_h, &blob, 0))
  {
    LOG_PRINT_L0("Failed to add tx blob to db transaction");
    throw DB_ERROR("Failed to add tx blob to db transaction");
  }

  MDB_val height;
  height.mv_size = sizeof(uint64_t);
  height.mv_data = &m_height;
  if (mdb_put(*m_write_txn, m_tx_heights, &val_h, &height, 0))
  {
    LOG_PRINT_L0("Failed to add tx block height to db transaction");
    throw DB_ERROR("Failed to add tx block height to db transaction");
  }

  uint64_t unlock_cpy = tx.unlock_time;
  height.mv_size = sizeof(unlock_cpy);
  height.mv_data = &unlock_cpy;
  if (mdb_put(*m_write_txn, m_tx_unlocks, &val_h, &height, 0))
  {
    LOG_PRINT_L0("Failed to add tx unlock time to db transaction");
    throw DB_ERROR("Failed to add tx unlock time to db transaction");
  }
}

void BlockchainLMDB::remove_transaction_data(const crypto::hash& tx_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  crypto::hash h = tx_hash;
  MDB_val val_h;
  val_h.mv_size = sizeof(crypto::hash);
  val_h.mv_data = &h;

  MDB_val unused;
  if (mdb_get(*m_write_txn, m_txs, &val_h, &unused))
  {
      LOG_PRINT_L1("Attempting to remove transaction that isn't in the db");
      throw TX_DNE("Attempting to remove transaction that isn't in the db");
  }

  if (mdb_del(*m_write_txn, m_txs, &val_h, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of tx to db transaction");
      throw DB_ERROR("Failed to add removal of tx to db transaction");
  }
  if (mdb_del(*m_write_txn, m_tx_unlocks, &val_h, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of tx unlock time to db transaction");
      throw DB_ERROR("Failed to add removal of tx unlock time to db transaction");
  }
  if (mdb_del(*m_write_txn, m_tx_heights, &val_h, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of tx block height to db transaction");
      throw DB_ERROR("Failed to add removal of tx block height to db transaction");
  }

  remove_tx_outputs(tx_hash);

  if (mdb_del(*m_write_txn, m_tx_outputs, &val_h, NULL))
  {
      LOG_PRINT_L1("Failed to add removal of tx outputs to db transaction");
      throw DB_ERROR("Failed to add removal of tx outputs to db transaction");
  }

}

void BlockchainLMDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val k;
  MDB_val v;

  k.mv_size = sizeof(uint64_t);
  k.mv_data = &m_num_outputs;
  crypto::hash h_cpy = tx_hash;
  v.mv_size = sizeof(crypto::hash);
  v.mv_data = &h_cpy;
  if (mdb_put(*m_write_txn, m_output_txs, &k, &v, 0))
  {
    LOG_PRINT_L0("Failed to add output tx hash to db transaction");
    throw DB_ERROR("Failed to add output tx hash to db transaction");
  }
  if (mdb_put(*m_write_txn, m_tx_outputs, &v, &k, 0))
  {
    LOG_PRINT_L0("Failed to add tx output index to db transaction");
    throw DB_ERROR("Failed to add tx output index to db transaction");
  }

  uint64_t index_cpy = local_index;
  v.mv_size = sizeof(uint64_t);
  v.mv_data = &index_cpy;
  if (mdb_put(*m_write_txn, m_output_indices, &k, &v, 0))
  {
    LOG_PRINT_L0("Failed to add tx output index to db transaction");
    throw DB_ERROR("Failed to add tx output index to db transaction");
  }

  uint64_t amount = tx_output.amount;
  v.mv_size = sizeof(uint64_t);
  v.mv_data = &amount;
  if (auto result = mdb_put(*m_write_txn, m_output_amounts, &v, &k, 0))
  {
    LOG_PRINT_L0("Failed to add output amount to db transaction");
    LOG_PRINT_L0("E: " << mdb_strerror(result));
    throw DB_ERROR("Failed to add output amount to db transaction");
  }

  if (tx_output.target.type() == typeid(txout_to_key))
  {
    crypto::public_key pubkey = boost::get<txout_to_key>(tx_output.target).key;

    v.mv_size = sizeof(pubkey);
    v.mv_data = &pubkey;
    if (mdb_put(*m_write_txn, m_output_keys, &k, &v, 0))
    {
      LOG_PRINT_L0("Failed to add output pubkey to db transaction");
      throw DB_ERROR("Failed to add output pubkey to db transaction");
    }
  }


/****** Uncomment if ever outputs actually need to be stored in this manner
 *
  blobdata b = output_to_blob(tx_output);

  v.mv_size = b.size();
  v.mv_data = &b;
  if (mdb_put(*m_write_txn, m_outputs, &k, &v, 0))
  {
    LOG_PRINT_L0("Failed to add output to db transaction");
    throw DB_ERROR("Failed to add output to db transaction");
  }
  if (mdb_put(*m_write_txn, m_output_gindices, &v, &k, 0))
  {
    LOG_PRINT_L0("Failed to add output global index to db transaction");
    throw DB_ERROR("Failed to add output global index to db transaction");
  }
************************************************************************/

  m_num_outputs++;
}

void BlockchainLMDB::remove_tx_outputs(const crypto::hash& tx_hash)
{

  lmdb_cur cur(*m_write_txn, m_tx_outputs);

  crypto::hash h_cpy = tx_hash;
  MDB_val k;
  k.mv_size = sizeof(h_cpy);
  k.mv_data = &h_cpy;

  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_ERROR("Attempting to remove a tx's outputs, but none found.  Continuing, but...be wary, because that's weird.");
  }
  else if (result)
  {
    LOG_PRINT_L0("DB error attempting to get an output");
    throw DB_ERROR("DB error attempting to get an output");
  }
  else
  {
    size_t num_elems = 0;
    mdb_cursor_count(cur, &num_elems);

    mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

    for (uint64_t i = 0; i < num_elems; ++i)
    {
      remove_output(*(const uint64_t*)v.mv_data);
      if (i < num_elems - 1)
      {
        mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
      }
    }
  }

  cur.close();
}

void BlockchainLMDB::remove_output(const tx_out& tx_output)
{
  return;
}

void BlockchainLMDB::remove_output(const uint64_t& out_index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val k;
  MDB_val v;

/****** Uncomment if ever outputs actually need to be stored in this manner
  blobdata b;
  t_serializable_object_to_blob(tx_output, b);
  k.mv_size = b.size();
  k.mv_data = &b;

  if (mdb_get(*m_write_txn, m_output_gindices, &k, &v))
  {
      LOG_PRINT_L1("Attempting to remove output that does not exist");
      throw OUTPUT_DNE("Attempting to remove output that does not exist");
  }

  uint64_t gindex = *(uint64_t*)v.mv_data;

  auto result = mdb_del(*m_write_txn, m_output_gindices, &k, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of output global index to db transaction");
      throw DB_ERROR("Error adding removal of output global index to db transaction");
  }

  result = mdb_del(*m_write_txn, m_outputs, &v, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of output to db transaction");
      throw DB_ERROR("Error adding removal of output to db transaction");
  }
*********************************************************************/

  auto result = mdb_del(*m_write_txn, m_output_indices, &v, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of output tx index to db transaction");
      throw DB_ERROR("Error adding removal of output tx index to db transaction");
  }

  result = mdb_del(*m_write_txn, m_output_txs, &v, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of output tx hash to db transaction");
      throw DB_ERROR("Error adding removal of output tx hash to db transaction");
  }

  result = mdb_del(*m_write_txn, m_output_amounts, &v, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of output amount to db transaction");
      throw DB_ERROR("Error adding removal of output amount to db transaction");
  }

  result = mdb_del(*m_write_txn, m_output_keys, &v, NULL);
  if (result == MDB_NOTFOUND)
  {
      LOG_PRINT_L2("Removing output, no public key found.");
  }
  else if (result)
  {
    LOG_PRINT_L1("Error adding removal of output pubkey to db transaction");
    throw DB_ERROR("Error adding removal of output pubkey to db transaction");
  }

  m_num_outputs--;
}

void BlockchainLMDB::add_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  crypto::key_image key = k_image;
  MDB_val val_key;
  val_key.mv_size = sizeof(crypto::key_image);
  val_key.mv_data = &key;

  MDB_val unused;
  if (mdb_get(*m_write_txn, m_spent_keys, &val_key, &unused) == 0)
  {
      LOG_PRINT_L1("Attempting to add spent key image that's already in the db");
      throw KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db");
  }

  char anything = '\0';
  unused.mv_size = sizeof(char);
  unused.mv_data = &anything;
  if (mdb_put(*m_write_txn, m_spent_keys, &val_key, &unused, 0))
  {
      LOG_PRINT_L1("Error adding spent key image to db transaction");
      throw DB_ERROR("Error adding spent key image to db transaction");
  }
}

void BlockchainLMDB::remove_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  crypto::key_image key_cpy = k_image;
  MDB_val k;
  k.mv_size = sizeof(crypto::key_image);
  k.mv_data = &key_cpy;
  auto result = mdb_del(*m_write_txn, m_spent_keys, &k, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
  {
      LOG_PRINT_L1("Error adding removal of key image to db transaction");
      throw DB_ERROR("Error adding removal of key image to db transaction");
  }
}

blobdata BlockchainLMDB::output_to_blob(const tx_out& output)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  blobdata b;
  if (!t_serializable_object_to_blob(output, b))
  {
    LOG_PRINT_L1("Error serializing output to blob");
    throw DB_ERROR("Error serializing output to blob");
  }
  return b;
}

tx_out BlockchainLMDB::output_from_blob(const blobdata& blob) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  std::stringstream ss;
  ss << blob;
  binary_archive<false> ba(ss);
  tx_out o;

  if (!(::serialization::serialize(ba, o)))
  {
    LOG_PRINT_L1("Error deserializing tx output blob");
    throw DB_ERROR("Error deserializing tx output blob");
  }

  return o;
}

uint64_t BlockchainLMDB::get_output_global_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  return 0;
}

void BlockchainLMDB::check_open() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (!m_open)
  {
    LOG_PRINT_L0("DB operation attempted on a not-open DB instance");
    throw DB_ERROR("DB operation attempted on a not-open DB instance");
  }
}

BlockchainLMDB::~BlockchainLMDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
}

BlockchainLMDB::BlockchainLMDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";
  m_open = false;
  m_height = 0;
}

void BlockchainLMDB::open(const std::string& filename)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  if (m_open)
  {
    LOG_PRINT_L0("Attempted to open db, but it's already open");
    throw DB_OPEN_FAILURE("Attempted to open db, but it's already open");
  }

  boost::filesystem::path direc(filename);
  if (boost::filesystem::exists(direc))
  {
    if (!boost::filesystem::is_directory(direc))
    {
      LOG_PRINT_L0("LMDB needs a directory path, but a file was passed");
      throw DB_OPEN_FAILURE("LMDB needs a directory path, but a file was passed");
    }
  }
  else
  {
    if (!boost::filesystem::create_directory(direc))
    {
      LOG_PRINT_L0("Failed to create directory " << filename);
      throw DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str());
    }
  }

  m_folder = filename;

  // set up lmdb environment
  if (mdb_env_create(&m_env))
  {
    LOG_PRINT_L0("Failed to create lmdb environment");
    throw DB_ERROR("Failed to create lmdb environment");
  }
  if (mdb_env_set_maxdbs(m_env, 20))
  {
    LOG_PRINT_L0("Failed to set max number of dbs");
    throw DB_ERROR("Failed to set max number of dbs");
  }
  size_t mapsize = 1LL << 34;
  if (auto result = mdb_env_set_mapsize(m_env, mapsize))
  {
    LOG_PRINT_L0("Failed to set max memory map size");
    LOG_PRINT_L0("E: " << mdb_strerror(result));
    throw DB_ERROR("Failed to set max memory map size");
  }
  if (auto result = mdb_env_open(m_env, filename.c_str(), 0, 0664))
  {
    LOG_PRINT_L0("Failed to open lmdb environment");
    LOG_PRINT_L0("E: " << mdb_strerror(result));
    throw DB_ERROR("Failed to open lmdb environment");
  }

  // get a read/write MDB_txn
  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  // open necessary databases, and set properties as needed
  // uses macros to avoid having to change things too many places
  lmdb_db_open(txn, LMDB_BLOCKS, MDB_INTEGERKEY | MDB_CREATE, m_blocks, "Failed to open db handle for m_blocks");

  lmdb_db_open(txn, LMDB_BLOCK_TIMESTAMPS, MDB_INTEGERKEY | MDB_CREATE, m_block_timestamps, "Failed to open db handle for m_block_timestamps");
  lmdb_db_open(txn, LMDB_BLOCK_HEIGHTS, MDB_CREATE, m_block_heights, "Failed to open db handle for m_block_heights");
  lmdb_db_open(txn, LMDB_BLOCK_HASHES, MDB_INTEGERKEY | MDB_CREATE, m_block_hashes, "Failed to open db handle for m_block_hashes");
  lmdb_db_open(txn, LMDB_BLOCK_SIZES, MDB_INTEGERKEY | MDB_CREATE, m_block_sizes, "Failed to open db handle for m_block_sizes");
  lmdb_db_open(txn, LMDB_BLOCK_DIFFS, MDB_INTEGERKEY | MDB_CREATE, m_block_diffs, "Failed to open db handle for m_block_diffs");
  lmdb_db_open(txn, LMDB_BLOCK_COINS, MDB_INTEGERKEY | MDB_CREATE, m_block_coins, "Failed to open db handle for m_block_coins");

  lmdb_db_open(txn, LMDB_TXS, MDB_CREATE, m_txs, "Failed to open db handle for m_txs");
  lmdb_db_open(txn, LMDB_TX_UNLOCKS, MDB_CREATE, m_tx_unlocks, "Failed to open db handle for m_tx_unlocks");
  lmdb_db_open(txn, LMDB_TX_HEIGHTS, MDB_CREATE, m_tx_heights, "Failed to open db handle for m_tx_heights");
  lmdb_db_open(txn, LMDB_TX_OUTPUTS, MDB_DUPSORT | MDB_CREATE, m_tx_outputs, "Failed to open db handle for m_tx_outputs");

  lmdb_db_open(txn, LMDB_OUTPUT_TXS, MDB_INTEGERKEY | MDB_CREATE, m_output_txs, "Failed to open db handle for m_output_txs");
  lmdb_db_open(txn, LMDB_OUTPUT_INDICES, MDB_INTEGERKEY | MDB_CREATE, m_output_indices, "Failed to open db handle for m_output_indices");
  lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_CREATE, m_output_amounts, "Failed to open db handle for m_output_amounts");
  lmdb_db_open(txn, LMDB_OUTPUT_KEYS, MDB_INTEGERKEY | MDB_CREATE, m_output_keys, "Failed to open db handle for m_output_keys");

/*************** not used, but kept for posterity
  lmdb_db_open(txn, LMDB_OUTPUTS, MDB_INTEGERKEY | MDB_CREATE, m_outputs, "Failed to open db handle for m_outputs");
  lmdb_db_open(txn, LMDB_OUTPUT_GINDICES, MDB_CREATE, m_output_gindices, "Failed to open db handle for m_output_gindices");
*************************************************/

  lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_CREATE, m_spent_keys, "Failed to open db handle for m_outputs");

  mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
  mdb_set_dupsort(txn, m_tx_outputs, compare_uint64);

  // get and keep current height
  MDB_stat db_stats;
  if (mdb_stat(txn, m_blocks, &db_stats))
  {
    LOG_PRINT_L0("Failed to query m_blocks");
    throw DB_ERROR("Failed to query m_blocks");
  }
  LOG_PRINT_L2("Setting m_height to: " << db_stats.ms_entries);
  m_height = db_stats.ms_entries;

  // get and keep current number of outputs
  if (mdb_stat(txn, m_output_indices, &db_stats))
  {
    LOG_PRINT_L0("Failed to query m_output_indices");
    throw DB_ERROR("Failed to query m_output_indices");
  }
  m_num_outputs = db_stats.ms_entries;

  // commit the transaction
  txn.commit();

  m_open = true;
  // from here, init should be finished
}

// unused for now, create will happen on open if doesn't exist
void BlockchainLMDB::create(const std::string& filename)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  throw DB_CREATE_FAILURE("create() is not implemented for this BlockchainDB, open() will create files if needed.");
}

void BlockchainLMDB::close()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // FIXME: not yet thread safe!!!  Use with care.
  mdb_env_close(m_env);
}

void BlockchainLMDB::sync()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // LMDB documentation leads me to believe this is unnecessary
}

void BlockchainLMDB::reset()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // TODO: this
}

std::vector<std::string> BlockchainLMDB::get_filenames() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  std::vector<std::string> filenames;

  boost::filesystem::path datafile(m_folder);
  datafile /= "data.mdb";
  boost::filesystem::path lockfile(m_folder);
  lockfile /= "lock.mdb";

  filenames.push_back(datafile.string());
  filenames.push_back(lockfile.string());

  return filenames;
}

// TODO: this?
bool BlockchainLMDB::lock()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  return false;
}

// TODO: this?
void BlockchainLMDB::unlock()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
}


bool BlockchainLMDB::block_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_block_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    txn.commit();
    LOG_PRINT_L1("Block with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    return false;
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch block index from hash");
    throw DB_ERROR("DB error attempting to fetch block index from hash");
  }

  txn.commit();
  return true;
}

block BlockchainLMDB::get_block(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  return get_block_from_height(get_block_height(h));
}

uint64_t BlockchainLMDB::get_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_block_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Attempted to retrieve non-existent block height");
    throw BLOCK_DNE("Attempted to retrieve non-existent block height");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a block height from the db");
    throw DB_ERROR("Error attempting to retrieve a block height from the db");
  }

  txn.commit();
  return *(const uint64_t*)result.mv_data;
}

block_header BlockchainLMDB::get_block_header(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // block_header object is automatically cast from block object
  return get_block(h);
}

block BlockchainLMDB::get_block_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_blocks, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get block from height " << height << ", but no such block exists");
    throw DB_ERROR("Attempt to get block from height failed -- block not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a block from the db");
    throw DB_ERROR("Error attempting to retrieve a block from the db");
  }

  txn.commit();

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
  {
    LOG_PRINT_L0("Failed to parse block from blob retrieved from the db");
    throw DB_ERROR("Failed to parse block from blob retrieved from the db");
  }

  return b;
}

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_timestamps, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get timestamp from height " << height << ", but no such timestamp exists");
    throw DB_ERROR("Attempt to get timestamp from height failed -- timestamp not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a timestamp from the db");
    throw DB_ERROR("Error attempting to retrieve a timestamp from the db");
  }

  txn.commit();
  return *(const uint64_t*)result.mv_data;
}

uint64_t BlockchainLMDB::get_top_block_timestamp() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // if no blocks, return 0
  if (m_height == 0)
  {
    return 0;
  }

  return get_block_timestamp(m_height - 1);
}

size_t BlockchainLMDB::get_block_size(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_sizes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get block size from height " << height << ", but no such block size exists");
    throw DB_ERROR("Attempt to get block size from height failed -- block size not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a block size from the db");
    throw DB_ERROR("Error attempting to retrieve a block size from the db");
  }

  txn.commit();
  return *(const size_t*)result.mv_data;
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_diffs, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get cumulative difficulty from height " << height << ", but no such cumulative difficulty exists");
    throw DB_ERROR("Attempt to get cumulative difficulty from height failed -- block size not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a cumulative difficulty from the db");
    throw DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db");
  }

  txn.commit();
  return *(difficulty_type*)result.mv_data;
}

difficulty_type BlockchainLMDB::get_block_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  difficulty_type diff1 = 0;
  difficulty_type diff2 = 0;

  diff1 = get_block_cumulative_difficulty(height);
  if (height != 0)
  {
    diff2 = get_block_cumulative_difficulty(height - 1);
  }

  return diff1 - diff2;
}

uint64_t BlockchainLMDB::get_block_already_generated_coins(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_coins, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get total generated coins from height " << height << ", but no such total generated coins exists");
    throw DB_ERROR("Attempt to get total generated coins from height failed -- block size not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a total generated coins from the db");
    throw DB_ERROR("Error attempting to retrieve a total generated coins from the db");
  }

  txn.commit();
  return *(const uint64_t*)result.mv_data;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t height_cpy = height;
  MDB_val key;
  key.mv_size = sizeof(uint64_t);
  key.mv_data = &height_cpy;
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_hashes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempted to get hash from height " << height << ", but no such hash exists");
    throw BLOCK_DNE("Attempt to get hash from height failed -- hash not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a block hash from the db");
    throw DB_ERROR("Error attempting to retrieve a block hash from the db");
  }

  txn.commit();
  return *(crypto::hash*)result.mv_data;
}

std::vector<block> BlockchainLMDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<block> v;

  for (uint64_t height = h1; height <= h2; ++height)
  {
    v.push_back(get_block_from_height(height));
  }

  return v;
}

std::vector<crypto::hash> BlockchainLMDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<crypto::hash> v;

  for (uint64_t height = h1; height <= h2; ++height)
  {
    v.push_back(get_block_hash_from_height(height));
  }

  return v;
}

crypto::hash BlockchainLMDB::top_block_hash() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  if (m_height != 0)
  {
    return get_block_hash_from_height(m_height - 1);
  }

  return null_hash;
}

block BlockchainLMDB::get_top_block() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (m_height != 0)
  {
    return get_block_from_height(m_height - 1);
  }

  block b;
  return b;
}

uint64_t BlockchainLMDB::height() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  return m_height;
}


bool BlockchainLMDB::tx_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_txs, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    txn.commit();
    LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    return false;
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch transaction from hash");
    throw DB_ERROR("DB error attempting to fetch transaction from hash");
  }

  return true;
}

uint64_t BlockchainLMDB::get_tx_unlock_time(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_tx_unlocks, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("tx unlock time with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    throw TX_DNE("Attempting to get unlock time for tx, but tx not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch tx unlock time from hash");
    throw DB_ERROR("DB error attempting to fetch tx unlock time from hash");
  }

  return *(const uint64_t*)result.mv_data;
}

transaction BlockchainLMDB::get_tx(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_txs, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("tx with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    throw TX_DNE("Attempting to get tx, but tx not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch tx from hash");
    throw DB_ERROR("DB error attempting to fetch tx from hash");
  }

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  transaction tx;
  if (!parse_and_validate_tx_from_blob(bd, tx))
  {
    LOG_PRINT_L0("Failed to parse tx from blob retrieved from the db");
    throw DB_ERROR("Failed to parse tx from blob retrieved from the db");
  }

  return tx;
}

uint64_t BlockchainLMDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  MDB_stat db_stats;
  if (mdb_stat(txn, m_txs, &db_stats))
  {
    LOG_PRINT_L0("Failed to query m_txs");
    throw DB_ERROR("Failed to query m_txs");
  }

  txn.commit();

  return db_stats.ms_entries;
}

std::vector<transaction> BlockchainLMDB::get_tx_list(const std::vector<crypto::hash>& hlist) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<transaction> v;

  for (auto& h : hlist)
  {
    v.push_back(get_tx(h));
  }

  return v;
}

uint64_t BlockchainLMDB::get_tx_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::hash key_cpy = h;
  MDB_val key;
  key.mv_size = sizeof(crypto::hash);
  key.mv_data = &key_cpy;

  MDB_val result;
  auto get_result = mdb_get(txn, m_tx_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("tx height with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    throw TX_DNE("Attempting to get height for tx, but tx not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch tx height from hash");
    throw DB_ERROR("DB error attempting to fetch tx height from hash");
  }

  return *(const uint64_t*)result.mv_data;
}

//FIXME: make sure the random method used here is appropriate
uint64_t BlockchainLMDB::get_random_output(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  uint64_t num_outputs = get_num_outputs(amount);
  if (num_outputs == 0)
  {
    LOG_PRINT_L1("Attempting to get a random output for an amount, but none exist");
    throw OUTPUT_DNE("Attempting to get a random output for an amount, but none exist");
  }

  return crypto::rand<uint64_t>() % num_outputs;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  lmdb_cur cur(txn, m_output_amounts);

  uint64_t amount_cpy = amount;
  MDB_val k;
  k.mv_size = sizeof(amount_cpy);
  k.mv_data = &amount_cpy;

  MDB_val v;
  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    return 0;
  }
  else if (result)
  {
    LOG_PRINT_L0("DB error attempting to get number of outputs of an amount");
    throw DB_ERROR("DB error attempting to get number of outputs of an amount");
  }

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  txn.commit();

  return num_elems;
}

crypto::public_key BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  uint64_t global = get_output_global_index(amount, index);

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  MDB_val k;
  k.mv_size = sizeof(global);
  k.mv_data = &global;

  MDB_val v;
  auto get_result = mdb_get(txn, m_output_keys, &k, &v);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempting to get output pubkey by global index, but key does not exist");
    throw DB_ERROR("Attempting to get output pubkey by global index, but key does not exist");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve an output pubkey from the db");
    throw DB_ERROR("Error attempting to retrieve an output pubkey from the db");
  }

  return *(crypto::public_key*)v.mv_data;
}

tx_out BlockchainLMDB::get_output(const crypto::hash& h, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  MDB_val k;
  crypto::hash h_cpy = h;
  k.mv_size = sizeof(h_cpy);
  k.mv_data = &h_cpy;

  lmdb_cur cur(txn, m_tx_outputs);

  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Attempting to get an output by tx hash and tx index, but output not found");
    throw OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found");
  }
  else if (result)
  {
    LOG_PRINT_L0("DB error attempting to get an output");
    throw DB_ERROR("DB error attempting to get an output");
  }

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);
  if (num_elems <= index)
  {
    LOG_PRINT_L1("Attempting to get an output by tx hash and tx index, but output not found");
    throw OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found");
  }

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  if (index != 0)
  {
    for (uint64_t i = 0; i < index; ++i)
    {
      mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
    }
  }

  mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);

  blobdata b;
  b = *(blobdata*)v.mv_data;

  cur.close();
  txn.commit();

  return output_from_blob(b);
}

// As this is not used, its return is now a blank output.
// This will save on space in the db.
tx_out BlockchainLMDB::get_output(const uint64_t& index) const
{
  return tx_out();
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  uint64_t index_cpy = index;
  MDB_val k;
  k.mv_size = sizeof(index_cpy);
  k.mv_data = &index_cpy;

  MDB_val v;
  auto get_result = mdb_get(txn, m_outputs, &k, &v);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Attempting to get output by global index, but output does not exist");
    throw OUTPUT_DNE();
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve an output from the db");
    throw DB_ERROR("Error attempting to retrieve an output from the db");
  }

  blobdata b = *(blobdata*)v.mv_data;

  return output_from_blob(b);
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  lmdb_cur cur(txn, m_output_amounts);

  uint64_t amount_cpy = amount;
  MDB_val k;
  k.mv_size = sizeof(amount_cpy);
  k.mv_data = &amount_cpy;

  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Attempting to get an output index by amount and amount index, but amount not found");
    throw OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found");
  }
  else if (result)
  {
    LOG_PRINT_L0("DB error attempting to get an output");
    throw DB_ERROR("DB error attempting to get an output");
  }

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);
  if (num_elems <= index)
  {
    LOG_PRINT_L1("Attempting to get an output index by amount and amount index, but output not found");
    throw OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found");
  }

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  if (index != 0)
  {
    for (uint64_t i = 0; i < index; ++i)
    {
      mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
    }
  }

  mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);

  uint64_t glob_index = *(const uint64_t*)v.mv_data;

  cur.close();

  k.mv_size = sizeof(glob_index);
  k.mv_data = &glob_index;
  auto get_result = mdb_get(txn, m_output_txs, &k, &v);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("output with given index not in db");
    throw OUTPUT_DNE("output with given index not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch output tx hash");
    throw DB_ERROR("DB error attempting to fetch output tx hash");
  }

  crypto::hash tx_hash = *(crypto::hash*)v.mv_data;

  get_result = mdb_get(txn, m_output_indices, &k, &v);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("output with given index not in db");
    throw OUTPUT_DNE("output with given index not in db");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch output tx index");
    throw DB_ERROR("DB error attempting to fetch output tx index");
  }

  txn.commit();

  return tx_out_index(tx_hash, *(const uint64_t *)v.mv_data);
}

std::vector<uint64_t> BlockchainLMDB::get_tx_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  MDB_val k;
  crypto::hash h_cpy = h;
  k.mv_size = sizeof(h_cpy);
  k.mv_data = &h_cpy;

  lmdb_cur cur(txn, m_tx_outputs);

  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Attempting to get an output by tx hash and tx index, but output not found");
    throw OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found");
  }
  else if (result)
  {
    LOG_PRINT_L0("DB error attempting to get an output");
    throw DB_ERROR("DB error attempting to get an output");
  }

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < num_elems; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
    mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);
    index_vec.push_back(*(const uint64_t *)v.mv_data);
  }

  cur.close();
  txn.commit();

  return index_vec;
}

bool BlockchainLMDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  crypto::key_image key = img;
  MDB_val val_key;
  val_key.mv_size = sizeof(crypto::key_image);
  val_key.mv_data = &key;

  MDB_val unused;
  if (mdb_get(txn, m_spent_keys, &val_key, &unused) == 0)
  {
    txn.commit();
    return true;
  }

  txn.commit();
  return false;
}

uint64_t BlockchainLMDB::add_block( const block& blk
                                  , const size_t& block_size
                                  , const difficulty_type& cumulative_difficulty
                                  , const uint64_t& coins_generated
                                  , const std::vector<transaction>& txs
                                  )
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }
  m_write_txn = &txn;

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
    m_write_txn = NULL;

    txn.commit();
  }
  catch (...)
  {
    m_num_outputs = num_outputs;
    m_write_txn = NULL;
    throw;
  }

  return ++m_height;
}

void BlockchainLMDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }
  m_write_txn = &txn;

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::pop_block(blk, txs);
    m_write_txn = NULL;

    txn.commit();
  }
  catch (...)
  {
    m_num_outputs = num_outputs;
    m_write_txn = NULL;
    throw;
  }

  --m_height;
}

}  // namespace cryptonote
