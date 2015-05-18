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
#include "profile_tools.h"

using epee::string_tools::pod_to_hex;

namespace
{

template <typename T>
inline void throw0(const T &e)
{
  LOG_PRINT_L0(e.what());
  throw e;
}

template <typename T>
inline void throw1(const T &e)
{
  LOG_PRINT_L1(e.what());
  throw e;
}

//  cursor needs to be closed when it goes out of scope,
//  this helps if the function using it throws
struct lmdb_cur
{
  lmdb_cur(MDB_txn* txn, MDB_dbi dbi)
  {
    if (mdb_cursor_open(txn, dbi, &m_cur))
      throw0(cryptonote::DB_ERROR("Error opening db cursor"));
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

template<typename T>
struct MDB_val_copy: public MDB_val
{
  MDB_val_copy(const T &t): t_copy(t)
  {
    mv_size = sizeof (T);
    mv_data = &t_copy;
  }
private:
  T t_copy;
};

template<>
struct MDB_val_copy<cryptonote::blobdata>: public MDB_val
{
  MDB_val_copy(const cryptonote::blobdata &bd): data(new char[bd.size()])
  {
    memcpy(data.get(), bd.data(), bd.size());
    mv_size = bd.size();
    mv_data = data.get();
  }
private:
  std::unique_ptr<char[]> data;
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
    throw0(cryptonote::DB_OPEN_FAILURE(error_string.c_str()));
}

}  // anonymous namespace

namespace cryptonote
{
std::atomic<uint64_t> mdb_txn_safe::num_active_txns{0};
std::atomic_flag mdb_txn_safe::creation_gate = ATOMIC_FLAG_INIT;

mdb_txn_safe::mdb_txn_safe() : m_txn(NULL)
{
  while (creation_gate.test_and_set());
  num_active_txns++;
  creation_gate.clear();
}

mdb_txn_safe::~mdb_txn_safe()
{
  LOG_PRINT_L3("mdb_txn_safe: destructor");
  if (m_txn != NULL)
  {
    if (m_batch_txn) // this is a batch txn and should have been handled before this point for safety
    {
      LOG_PRINT_L0("WARNING: mdb_txn_safe: m_txn is a batch txn and it's not NULL in destructor - calling mdb_txn_abort()");
    }
    else
    {
      // Example of when this occurs: a lookup fails, so a read-only txn is
      // aborted through this destructor. However, successful read-only txns
      // ideally should have been committed when done and not end up here.
      //
      // NOTE: not sure if this is ever reached for a non-batch write
      // transaction, but it's probably not ideal if it did.
      LOG_PRINT_L3("mdb_txn_safe: m_txn not NULL in destructor - calling mdb_txn_abort()");
    }
    mdb_txn_abort(m_txn);
  }
  num_active_txns--;
}

void mdb_txn_safe::commit(std::string message)
{
  if (message.size() == 0)
  {
    message = "Failed to commit a transaction to the db";
  }

  if (mdb_txn_commit(m_txn))
  {
    m_txn = NULL;
    LOG_PRINT_L0(message);
    throw DB_ERROR(message.c_str());
  }
  m_txn = NULL;
}

void mdb_txn_safe::abort()
{
  LOG_PRINT_L3("mdb_txn_safe: abort()");
  if(m_txn != NULL)
  {
    mdb_txn_abort(m_txn);
    m_txn = NULL;
  }
  else
  {
    LOG_PRINT_L0("WARNING: mdb_txn_safe: abort() called, but m_txn is NULL");
  }
}

uint64_t mdb_txn_safe::num_active_tx()
{
  return num_active_txns;
}

void mdb_txn_safe::prevent_new_txns()
{
  while (creation_gate.test_and_set());
}

void mdb_txn_safe::wait_no_active_txns()
{
  while (num_active_txns > 0);
}

void mdb_txn_safe::allow_new_txns()
{
  creation_gate.clear();
}



void BlockchainLMDB::do_resize()
{
  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  uint64_t new_mapsize = (double)mei.me_mapsize * RESIZE_FACTOR;

  new_mapsize += (new_mapsize % mst.ms_psize);

  mdb_txn_safe::prevent_new_txns();

  if (m_write_txn != nullptr)
  {
    if (m_batch_active)
    {
      throw0(DB_ERROR("lmdb resizing not yet supported when batch transactions enabled!"));
    }
    else
    {
      throw0(DB_ERROR("attempting resize with write transaction in progress, this should not happen!"));
    }
  }

  mdb_txn_safe::wait_no_active_txns();

  mdb_env_set_mapsize(m_env, new_mapsize);

  LOG_PRINT_L0("LMDB Mapsize increased."
      << "  Old: " << mei.me_mapsize / (1024 * 1024) << "MiB"
      << ", New: " << new_mapsize / (1024 * 1024) << "MiB");

  mdb_txn_safe::allow_new_txns();
}

bool BlockchainLMDB::need_resize()
{
  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;

  if ((double)size_used / mei.me_mapsize  > 0.8)
  {
    return true;
  }
  return false;
}

void BlockchainLMDB::add_block( const block& blk
              , const size_t& block_size
              , const difficulty_type& cumulative_difficulty
              , const uint64_t& coins_generated
              , const crypto::hash& blk_hash
              )
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<crypto::hash> val_h(blk_hash);
  MDB_val unused;
  if (mdb_get(*m_write_txn, m_block_heights, &val_h, &unused) == 0)
    throw1(BLOCK_EXISTS("Attempting to add block that's already in the db"));

  if (m_height > 0)
  {
    MDB_val_copy<crypto::hash> parent_key(blk.prev_id);
    MDB_val parent_h;
    if (mdb_get(*m_write_txn, m_block_heights, &parent_key, &parent_h))
    {
      LOG_PRINT_L3("m_height: " << m_height);
      LOG_PRINT_L3("parent_key: " << blk.prev_id);
      throw0(DB_ERROR("Failed to get top block hash to check for new block's parent"));
    }
    uint64_t parent_height = *(const uint64_t *)parent_h.mv_data;
    if (parent_height != m_height - 1)
      throw0(BLOCK_PARENT_DNE("Top block is not new block's parent"));
  }

  MDB_val_copy<uint64_t> key(m_height);

  MDB_val_copy<blobdata> blob(block_to_blob(blk));
  auto res = mdb_put(*m_write_txn, m_blocks, &key, &blob, 0);
  if (res)
    throw0(DB_ERROR(std::string("Failed to add block blob to db transaction: ").append(mdb_strerror(res)).c_str()));

  MDB_val_copy<size_t> sz(block_size);
  if (mdb_put(*m_write_txn, m_block_sizes, &key, &sz, 0))
    throw0(DB_ERROR("Failed to add block size to db transaction"));

  MDB_val_copy<uint64_t> ts(blk.timestamp);
  if (mdb_put(*m_write_txn, m_block_timestamps, &key, &ts, 0))
    throw0(DB_ERROR("Failed to add block timestamp to db transaction"));

  MDB_val_copy<difficulty_type> diff(cumulative_difficulty);
  if (mdb_put(*m_write_txn, m_block_diffs, &key, &diff, 0))
    throw0(DB_ERROR("Failed to add block cumulative difficulty to db transaction"));

  MDB_val_copy<uint64_t> coinsgen(coins_generated);
  if (mdb_put(*m_write_txn, m_block_coins, &key, &coinsgen, 0))
    throw0(DB_ERROR("Failed to add block total generated coins to db transaction"));

  if (mdb_put(*m_write_txn, m_block_heights, &val_h, &key, 0))
    throw0(DB_ERROR("Failed to add block height by hash to db transaction"));

  if (mdb_put(*m_write_txn, m_block_hashes, &key, &val_h, 0))
    throw0(DB_ERROR("Failed to add block hash to db transaction"));

}

void BlockchainLMDB::remove_block()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (m_height == 0)
    throw0(BLOCK_DNE ("Attempting to remove block from an empty blockchain"));

  MDB_val_copy<uint64_t> k(m_height - 1);
  MDB_val h;
  if (mdb_get(*m_write_txn, m_block_hashes, &k, &h))
      throw1(BLOCK_DNE("Attempting to remove block that's not in the db"));

  if (mdb_del(*m_write_txn, m_blocks, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block to db transaction"));

  if (mdb_del(*m_write_txn, m_block_sizes, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block size to db transaction"));

  if (mdb_del(*m_write_txn, m_block_diffs, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block cumulative difficulty to db transaction"));

  if (mdb_del(*m_write_txn, m_block_coins, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block total generated coins to db transaction"));

  if (mdb_del(*m_write_txn, m_block_timestamps, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block timestamp to db transaction"));

  if (mdb_del(*m_write_txn, m_block_heights, &h, NULL))
      throw1(DB_ERROR("Failed to add removal of block height by hash to db transaction"));

  if (mdb_del(*m_write_txn, m_block_hashes, &k, NULL))
      throw1(DB_ERROR("Failed to add removal of block hash to db transaction"));
}

void BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<crypto::hash> val_h(tx_hash);
  MDB_val unused;
  if (mdb_get(*m_write_txn, m_txs, &val_h, &unused) == 0)
      throw1(TX_EXISTS("Attempting to add transaction that's already in the db"));

  MDB_val_copy<blobdata> blob(tx_to_blob(tx));
  if (mdb_put(*m_write_txn, m_txs, &val_h, &blob, 0))
    throw0(DB_ERROR("Failed to add tx blob to db transaction"));

  MDB_val_copy<uint64_t> height(m_height);
  if (mdb_put(*m_write_txn, m_tx_heights, &val_h, &height, 0))
    throw0(DB_ERROR("Failed to add tx block height to db transaction"));

  MDB_val_copy<uint64_t> unlock_time(tx.unlock_time);
  if (mdb_put(*m_write_txn, m_tx_unlocks, &val_h, &unlock_time, 0))
    throw0(DB_ERROR("Failed to add tx unlock time to db transaction"));
}

void BlockchainLMDB::remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<crypto::hash> val_h(tx_hash);
  MDB_val unused;
  if (mdb_get(*m_write_txn, m_txs, &val_h, &unused))
      throw1(TX_DNE("Attempting to remove transaction that isn't in the db"));

  if (mdb_del(*m_write_txn, m_txs, &val_h, NULL))
      throw1(DB_ERROR("Failed to add removal of tx to db transaction"));
  if (mdb_del(*m_write_txn, m_tx_unlocks, &val_h, NULL))
      throw1(DB_ERROR("Failed to add removal of tx unlock time to db transaction"));
  if (mdb_del(*m_write_txn, m_tx_heights, &val_h, NULL))
      throw1(DB_ERROR("Failed to add removal of tx block height to db transaction"));

  remove_tx_outputs(tx_hash, tx);

  if (mdb_del(*m_write_txn, m_tx_outputs, &val_h, NULL))
      throw1(DB_ERROR("Failed to add removal of tx outputs to db transaction"));

}

void BlockchainLMDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<uint64_t> k(m_num_outputs);
  MDB_val_copy<crypto::hash> v(tx_hash);

  if (mdb_put(*m_write_txn, m_output_txs, &k, &v, 0))
    throw0(DB_ERROR("Failed to add output tx hash to db transaction"));
  if (mdb_put(*m_write_txn, m_tx_outputs, &v, &k, 0))
    throw0(DB_ERROR("Failed to add tx output index to db transaction"));

  MDB_val_copy<uint64_t> val_local_index(local_index);
  if (mdb_put(*m_write_txn, m_output_indices, &k, &val_local_index, 0))
    throw0(DB_ERROR("Failed to add tx output index to db transaction"));

  MDB_val_copy<uint64_t> val_amount(tx_output.amount);
  if (auto result = mdb_put(*m_write_txn, m_output_amounts, &val_amount, &k, 0))
    throw0(DB_ERROR(std::string("Failed to add output amount to db transaction").append(mdb_strerror(result)).c_str()));

  if (tx_output.target.type() == typeid(txout_to_key))
  {
    MDB_val_copy<crypto::public_key> val_pubkey(boost::get<txout_to_key>(tx_output.target).key);
    if (mdb_put(*m_write_txn, m_output_keys, &k, &val_pubkey, 0))
      throw0(DB_ERROR("Failed to add output pubkey to db transaction"));
  }


/****** Uncomment if ever outputs actually need to be stored in this manner
 *
  blobdata b = output_to_blob(tx_output);

  v.mv_size = b.size();
  v.mv_data = &b;
  if (mdb_put(*m_write_txn, m_outputs, &k, &v, 0))
    throw0(DB_ERROR("Failed to add output to db transaction"));
  if (mdb_put(*m_write_txn, m_output_gindices, &v, &k, 0))
    throw0(DB_ERROR("Failed to add output global index to db transaction"));
************************************************************************/

  m_num_outputs++;
}

void BlockchainLMDB::remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  lmdb_cur cur(*m_write_txn, m_tx_outputs);

  MDB_val_copy<crypto::hash> k(tx_hash);
  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_ERROR("Attempting to remove a tx's outputs, but none found.  Continuing, but...be wary, because that's weird.");
  }
  else if (result)
  {
    throw0(DB_ERROR("DB error attempting to get an output"));
  }
  else
  {
    size_t num_elems = 0;
    mdb_cursor_count(cur, &num_elems);

    mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

    for (uint64_t i = 0; i < num_elems; ++i)
    {
      const tx_out tx_output = tx.vout[i];
      remove_output(*(const uint64_t*)v.mv_data, tx_output.amount);
      if (i < num_elems - 1)
      {
        mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
      }
    }
  }

  cur.close();
}

// TODO: probably remove this function
void BlockchainLMDB::remove_output(const tx_out& tx_output)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__ << " (unused version - does nothing)");
  return;
}

void BlockchainLMDB::remove_output(const uint64_t& out_index, const uint64_t amount)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<uint64_t> k(out_index);

/****** Uncomment if ever outputs actually need to be stored in this manner
  blobdata b;
  t_serializable_object_to_blob(tx_output, b);
  k.mv_size = b.size();
  k.mv_data = &b;

  if (mdb_get(*m_write_txn, m_output_gindices, &k, &v))
      throw1(OUTPUT_DNE("Attempting to remove output that does not exist"));

  uint64_t gindex = *(uint64_t*)v.mv_data;

  auto result = mdb_del(*m_write_txn, m_output_gindices, &k, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
      throw1(DB_ERROR("Error adding removal of output global index to db transaction"));

  result = mdb_del(*m_write_txn, m_outputs, &v, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
      throw1(DB_ERROR("Error adding removal of output to db transaction"));
*********************************************************************/

  auto result = mdb_del(*m_write_txn, m_output_indices, &k, NULL);
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Unexpected: global output index not found in m_output_indices");
  }
  else if (result)
  {
    throw1(DB_ERROR("Error adding removal of output tx index to db transaction"));
  }

  result = mdb_del(*m_write_txn, m_output_txs, &k, NULL);
  // if (result != 0 && result != MDB_NOTFOUND)
  //    throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("Unexpected: global output index not found in m_output_txs");
  }
  else if (result)
  {
    throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
  }

  result = mdb_del(*m_write_txn, m_output_keys, &k, NULL);
  if (result == MDB_NOTFOUND)
  {
      LOG_PRINT_L0("Unexpected: global output index not found in m_output_keys");
  }
  else if (result)
    throw1(DB_ERROR("Error adding removal of output pubkey to db transaction"));

  remove_amount_output_index(amount, out_index);

  m_num_outputs--;
}

void BlockchainLMDB::remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  lmdb_cur cur(*m_write_txn, m_output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  uint64_t amount_output_index = 0;
  uint64_t goi = 0;
  bool found_index = false;
  for (uint64_t i = 0; i < num_elems; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);
    goi = *(const uint64_t *)v.mv_data;
    if (goi == global_output_index)
    {
      amount_output_index = i;
      found_index = true;
      break;
    }
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
  }
  if (found_index)
  {
    // found the amount output index
    // now delete it
    result = mdb_cursor_del(cur, 0);
    if (result)
      throw0(DB_ERROR(std::string("Error deleting amount output index ").append(boost::lexical_cast<std::string>(amount_output_index)).c_str()));
  }
  else
  {
    // not found
    cur.close();
    throw1(OUTPUT_DNE("Failed to find amount output index"));
  }
  cur.close();
}

void BlockchainLMDB::add_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<crypto::key_image> val_key(k_image);
  MDB_val unused;
  if (mdb_get(*m_write_txn, m_spent_keys, &val_key, &unused) == 0)
      throw1(KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db"));

  char anything = '\0';
  unused.mv_size = sizeof(char);
  unused.mv_data = &anything;
  if (auto result = mdb_put(*m_write_txn, m_spent_keys, &val_key, &unused, 0))
    throw1(DB_ERROR(std::string("Error adding spent key image to db transaction: ").append(mdb_strerror(result)).c_str()));
}

void BlockchainLMDB::remove_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  MDB_val_copy<crypto::key_image> k(k_image);
  auto result = mdb_del(*m_write_txn, m_spent_keys, &k, NULL);
  if (result != 0 && result != MDB_NOTFOUND)
      throw1(DB_ERROR("Error adding removal of key image to db transaction"));
}

blobdata BlockchainLMDB::output_to_blob(const tx_out& output)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  blobdata b;
  if (!t_serializable_object_to_blob(output, b))
    throw1(DB_ERROR("Error serializing output to blob"));
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
    throw1(DB_ERROR("Error deserializing tx output blob"));

  return o;
}

uint64_t BlockchainLMDB::get_output_global_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  lmdb_cur cur(txn, m_output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);
  if (num_elems <= index)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < index; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
  }

  mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);

  uint64_t glob_index = *(const uint64_t*)v.mv_data;

  cur.close();

  txn.commit();

  return glob_index;
}

void BlockchainLMDB::check_open() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (!m_open)
    throw0(DB_ERROR("DB operation attempted on a not-open DB instance"));
}

BlockchainLMDB::~BlockchainLMDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  // batch transaction shouldn't be active at this point. If it is, consider it aborted.
  if (m_batch_active)
    batch_abort();
}

BlockchainLMDB::BlockchainLMDB(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";
  m_open = false;

  m_batch_transactions = batch_transactions;
  m_write_txn = nullptr;
  m_write_batch_txn = nullptr;
  m_batch_active = false;
  m_height = 0;
}

void BlockchainLMDB::open(const std::string& filename, const int mdb_flags)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  if (m_open)
    throw0(DB_OPEN_FAILURE("Attempted to open db, but it's already open"));

  boost::filesystem::path direc(filename);
  if (boost::filesystem::exists(direc))
  {
    if (!boost::filesystem::is_directory(direc))
      throw0(DB_OPEN_FAILURE("LMDB needs a directory path, but a file was passed"));
  }
  else
  {
    if (!boost::filesystem::create_directory(direc))
      throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
  }

  // check for existing LMDB files in base directory
  boost::filesystem::path old_files = direc.parent_path();
  if (boost::filesystem::exists(old_files / "data.mdb") ||
      boost::filesystem::exists(old_files / "lock.mdb"))
  {
    LOG_PRINT_L0("Found existing LMDB files in " << old_files.string());
    LOG_PRINT_L0("Move data.mdb and/or lock.mdb to " << filename << ", or delete them, and then restart");
    throw DB_ERROR("Database could not be opened");
  }

  m_folder = filename;

  // set up lmdb environment
  if (mdb_env_create(&m_env))
    throw0(DB_ERROR("Failed to create lmdb environment"));
  if (mdb_env_set_maxdbs(m_env, 20))
    throw0(DB_ERROR("Failed to set max number of dbs"));

  size_t mapsize = DEFAULT_MAPSIZE;
  if (auto result = mdb_env_set_mapsize(m_env, mapsize))
    throw0(DB_ERROR(std::string("Failed to set max memory map size: ").append(mdb_strerror(result)).c_str()));
  if (auto result = mdb_env_open(m_env, filename.c_str(), mdb_flags, 0644))
    throw0(DB_ERROR(std::string("Failed to open lmdb environment: ").append(mdb_strerror(result)).c_str()));

  // get a read/write MDB_txn
  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

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

  lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_CREATE, m_spent_keys, "Failed to open db handle for m_spent_keys");

  mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
  mdb_set_dupsort(txn, m_tx_outputs, compare_uint64);

  // get and keep current height
  MDB_stat db_stats;
  if (mdb_stat(txn, m_blocks, &db_stats))
    throw0(DB_ERROR("Failed to query m_blocks"));
  LOG_PRINT_L2("Setting m_height to: " << db_stats.ms_entries);
  m_height = db_stats.ms_entries;

  // get and keep current number of outputs
  if (mdb_stat(txn, m_output_indices, &db_stats))
    throw0(DB_ERROR("Failed to query m_output_indices"));
  m_num_outputs = db_stats.ms_entries;

  // commit the transaction
  txn.commit();

  m_open = true;
  // from here, init should be finished
}

void BlockchainLMDB::close()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (m_batch_active)
  {
    LOG_PRINT_L3("close() first calling batch_abort() due to active batch transaction");
    batch_abort();
  }
  this->sync();

  // FIXME: not yet thread safe!!!  Use with care.
  mdb_env_close(m_env);
}

void BlockchainLMDB::sync()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // Does nothing unless LMDB environment was opened with MDB_NOSYNC or in part
  // MDB_NOMETASYNC. Force flush to be synchronous.
  if (auto result = mdb_env_sync(m_env, true))
  {
    throw0(DB_ERROR(std::string("Failed to sync database").append(mdb_strerror(result)).c_str()));
  }
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

std::string BlockchainLMDB::get_db_name() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  return std::string("lmdb");
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

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    txn.commit();
    LOG_PRINT_L3("Block with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
    return false;
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch block index from hash"));

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

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_get(txn, m_block_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
    throw1(BLOCK_DNE("Attempted to retrieve non-existent block height"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block height from the db"));

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

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(txn, m_blocks, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get block from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block from the db"));

  txn.commit();

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

  return b;
}

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }
  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_block_timestamps, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get timestamp from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- timestamp not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a timestamp from the db"));

  if (! m_batch_active)
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

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_block_sizes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get block size from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block size from the db"));

  if (! m_batch_active)
    txn.commit();
  return *(const size_t*)result.mv_data;
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__ << "  height: " << height);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }
  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_block_diffs, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get cumulative difficulty from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- difficulty not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db"));

  if (! m_batch_active)
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

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_block_coins, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get generated coins from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a total generated coins from the db"));

  if (! m_batch_active)
    txn.commit();
  return *(const uint64_t*)result.mv_data;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_block_hashes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get hash from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- hash not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR(std::string("Error attempting to retrieve a block hash from the db: ").
          append(mdb_strerror(get_result)).c_str()));

  if (! m_batch_active)
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

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;

  TIME_MEASURE_START(time1);
  auto get_result = mdb_get(*txn_ptr, m_txs, &key, &result);
  TIME_MEASURE_FINISH(time1);
  time_tx_exists += time1;
  if (get_result == MDB_NOTFOUND)
  {
    if (! m_batch_active)
      txn.commit();
    LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
    return false;
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch transaction from hash"));

  return true;
}

uint64_t BlockchainLMDB::get_tx_unlock_time(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_get(txn, m_tx_unlocks, &key, &result);
  if (get_result == MDB_NOTFOUND)
    throw1(TX_DNE(std::string("tx unlock time with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx unlock time from hash"));

  return *(const uint64_t*)result.mv_data;
}

transaction BlockchainLMDB::get_tx(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_txs, &key, &result);
  if (get_result == MDB_NOTFOUND)
    throw1(TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx from hash"));

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  transaction tx;
  if (!parse_and_validate_tx_from_blob(bd, tx))
    throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
  if (! m_batch_active)
    txn.commit();

  return tx;
}

uint64_t BlockchainLMDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_stat db_stats;
  if (mdb_stat(txn, m_txs, &db_stats))
    throw0(DB_ERROR("Failed to query m_txs"));

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

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_get(*txn_ptr, m_tx_heights, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    throw1(TX_DNE(std::string("tx height with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx height from hash"));

  if (! m_batch_active)
    txn.commit();

  return *(const uint64_t*)result.mv_data;
}

//FIXME: make sure the random method used here is appropriate
uint64_t BlockchainLMDB::get_random_output(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  uint64_t num_outputs = get_num_outputs(amount);
  if (num_outputs == 0)
    throw1(OUTPUT_DNE("Attempting to get a random output for an amount, but none exist"));

  return crypto::rand<uint64_t>() % num_outputs;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  lmdb_cur cur(txn, m_output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;
  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    return 0;
  }
  else if (result)
    throw0(DB_ERROR("DB error attempting to get number of outputs of an amount"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  txn.commit();

  return num_elems;
}

crypto::public_key BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  uint64_t glob_index = get_output_global_index(amount, index);

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<uint64_t> k(glob_index);
  MDB_val v;
  auto get_result = mdb_get(txn, m_output_keys, &k, &v);
  if (get_result == MDB_NOTFOUND)
    throw0(DB_ERROR("Attempting to get output pubkey by global index, but key does not exist"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));

  return *(crypto::public_key*)v.mv_data;
}

tx_out BlockchainLMDB::get_output(const crypto::hash& h, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  lmdb_cur cur(txn, m_tx_outputs);

  MDB_val_copy<crypto::hash> k(h);
  MDB_val v;
  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);
  if (num_elems <= index)
    throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < index; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
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

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<uint64_t> k(index);
  MDB_val v;
  auto get_result = mdb_get(txn, m_outputs, &k, &v);
  if (get_result == MDB_NOTFOUND)
  {
    throw OUTPUT_DNE("Attempting to get output by global index, but output does not exist");
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve an output from the db"));

  blobdata b = *(blobdata*)v.mv_data;

  return output_from_blob(b);
}

tx_out_index BlockchainLMDB::get_output_tx_and_index_from_global(const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }
  MDB_val_copy<uint64_t> k(index);
  MDB_val v;

  auto get_result = mdb_get(*txn_ptr, m_output_txs, &k, &v);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

  crypto::hash tx_hash = *(crypto::hash*)v.mv_data;

  get_result = mdb_get(*txn_ptr, m_output_indices, &k, &v);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx index"));
  if (! m_batch_active)
    txn.commit();

  return tx_out_index(tx_hash, *(const uint64_t *)v.mv_data);
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  mdb_txn_safe* txn_ptr = &txn;
  if (m_batch_active)
    txn_ptr = m_write_txn;
  else
  {
    if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
  }
  lmdb_cur cur(*txn_ptr, m_output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;

  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);
  if (num_elems <= index)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < index; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
  }

  mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);

  uint64_t glob_index = *(const uint64_t*)v.mv_data;

  cur.close();

  if (! m_batch_active)
    txn.commit();

  return get_output_tx_and_index_from_global(glob_index);
}

std::vector<uint64_t> BlockchainLMDB::get_tx_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  lmdb_cur cur(txn, m_tx_outputs);

  MDB_val_copy<crypto::hash> k(h);
  MDB_val v;
  auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < num_elems; ++i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);
    index_vec.push_back(*(const uint64_t *)v.mv_data);
    mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
  }

  cur.close();
  txn.commit();

  return index_vec;
}

std::vector<uint64_t> BlockchainLMDB::get_tx_amount_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;
  std::vector<uint64_t> index_vec2;

  // get the transaction's global output indices first
  index_vec = get_tx_output_indices(h);
  // these are next used to obtain the amount output indices

  transaction tx = get_tx(h);

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  uint64_t i = 0;
  uint64_t global_index;
  BOOST_FOREACH(const auto& vout, tx.vout)
  {
    uint64_t amount =  vout.amount;

    global_index = index_vec[i];

    lmdb_cur cur(txn, m_output_amounts);

    MDB_val_copy<uint64_t> k(amount);
    MDB_val v;

    auto result = mdb_cursor_get(cur, &k, &v, MDB_SET);
    if (result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
    else if (result)
      throw0(DB_ERROR("DB error attempting to get an output"));

    size_t num_elems = 0;
    mdb_cursor_count(cur, &num_elems);

    mdb_cursor_get(cur, &k, &v, MDB_FIRST_DUP);

    uint64_t amount_output_index = 0;
    uint64_t output_index = 0;
    bool found_index = false;
    for (uint64_t j = 0; j < num_elems; ++j)
    {
      mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);
      output_index = *(const uint64_t *)v.mv_data;
      if (output_index == global_index)
      {
        amount_output_index = j;
        found_index = true;
        break;
      }
      mdb_cursor_get(cur, &k, &v, MDB_NEXT_DUP);
    }
    if (found_index)
    {
      index_vec2.push_back(amount_output_index);
    }
    else
    {
      // not found
      cur.close();
      txn.commit();
      throw1(OUTPUT_DNE("specified output not found in db"));
    }

    cur.close();
    ++i;
  }

  txn.commit();

  return index_vec2;
}



bool BlockchainLMDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, MDB_RDONLY, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  MDB_val_copy<crypto::key_image> val_key(img);
  MDB_val unused;
  if (mdb_get(txn, m_spent_keys, &val_key, &unused) == 0)
  {
    txn.commit();
    return true;
  }

  txn.commit();
  return false;
}

void BlockchainLMDB::batch_start()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (m_batch_active)
    throw0(DB_ERROR("batch transaction already in progress"));
  if (m_write_batch_txn != nullptr)
    throw0(DB_ERROR("batch transaction already in progress"));
  if (m_write_txn)
    throw0(DB_ERROR("batch transaction attempted, but m_write_txn already in use"));
  check_open();

  m_write_batch_txn = new mdb_txn_safe();

  // NOTE: need to make sure it's destroyed properly when done
  if (mdb_txn_begin(m_env, NULL, 0, *m_write_batch_txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));
  // indicates this transaction is for batch transactions, but not whether it's
  // active
  m_write_batch_txn->m_batch_txn = true;
  m_write_txn = m_write_batch_txn;
  m_batch_active = true;
  LOG_PRINT_L3("batch transaction: begin");
}

void BlockchainLMDB::batch_commit()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw0(DB_ERROR("batch transaction not in progress"));
  if (m_write_batch_txn == nullptr)
    throw0(DB_ERROR("batch transaction not in progress"));
  check_open();

  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  m_write_txn->commit();
  TIME_MEASURE_FINISH(time1);
  time_commit1 += time1;
  LOG_PRINT_L3("batch transaction: committed");

  m_write_txn = nullptr;
  delete m_write_batch_txn;
}

void BlockchainLMDB::batch_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw0(DB_ERROR("batch transaction not in progress"));
  if (m_write_batch_txn == nullptr)
    throw0(DB_ERROR("batch transaction not in progress"));
  check_open();
  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  m_write_txn->commit();
  TIME_MEASURE_FINISH(time1);
  time_commit1 += time1;
  // for destruction of batch transaction
  m_write_txn = nullptr;
  delete m_write_batch_txn;
  m_write_batch_txn = nullptr;
  m_batch_active = false;
  LOG_PRINT_L3("batch transaction: end");
}

void BlockchainLMDB::batch_abort()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw0(DB_ERROR("batch transaction not in progress"));
  check_open();
  // for destruction of batch transaction
  m_write_txn = nullptr;
  // explicitly call in case mdb_env_close() (BlockchainLMDB::close()) called before BlockchainLMDB destructor called.
  m_write_batch_txn->abort();
  m_batch_active = false;
  m_write_batch_txn = nullptr;
  LOG_PRINT_L3("batch transaction: aborted");
}

void BlockchainLMDB::set_batch_transactions(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  m_batch_transactions = batch_transactions;
  LOG_PRINT_L3("batch transactions " << (m_batch_transactions ? "enabled" : "disabled"));
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

  if (m_height % 10 == 0)
  {
    if (need_resize())
    {
      LOG_PRINT_L0("LMDB memory map needs resized, doing that now.");
      do_resize();
    }
  }

  mdb_txn_safe txn;
  if (! m_batch_active)
  {
    if (mdb_txn_begin(m_env, NULL, 0, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
    m_write_txn = &txn;
  }

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
    if (! m_batch_active)
    {
      m_write_txn = NULL;

      TIME_MEASURE_START(time1);
      txn.commit();
      TIME_MEASURE_FINISH(time1);
      time_commit1 += time1;
    }
  }
  catch (...)
  {
    m_num_outputs = num_outputs;
    if (! m_batch_active)
      m_write_txn = NULL;
    throw;
  }

  return ++m_height;
}

void BlockchainLMDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (! m_batch_active)
  {
    if (mdb_txn_begin(m_env, NULL, 0, txn))
      throw0(DB_ERROR("Failed to create a transaction for the db"));
    m_write_txn = &txn;
  }

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::pop_block(blk, txs);
    if (! m_batch_active)
    {
      m_write_txn = NULL;

      txn.commit();
    }
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
