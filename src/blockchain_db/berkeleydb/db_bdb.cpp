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

#include "db_bdb.h"

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
struct bdb_cur
{
  bdb_cur(DbTxn* txn, Db* dbi)
  {
    if (dbi->cursor(txn, &m_cur, 0))
      throw0(cryptonote::DB_ERROR("Error opening db cursor"));
    done = false;
  }

  ~bdb_cur() { close(); }

  operator Dbc*() { return m_cur; }
  operator Dbc**() { return &m_cur; }
  Dbc* operator->() { return m_cur; }

  void close()
  {
    if (!done)
    {
      m_cur->close();
      done = true;
    }
  }

private:
  Dbc* m_cur;
  bool done;
};

const char* const BDB_BLOCKS = "blocks";
const char* const BDB_BLOCK_TIMESTAMPS = "block_timestamps";
const char* const BDB_BLOCK_HEIGHTS = "block_heights";
const char* const BDB_BLOCK_HASHES = "block_hashes";
const char* const BDB_BLOCK_SIZES = "block_sizes";
const char* const BDB_BLOCK_DIFFS = "block_diffs";
const char* const BDB_BLOCK_COINS = "block_coins";

const char* const BDB_TXS = "txs";
const char* const BDB_TX_UNLOCKS = "tx_unlocks";
const char* const BDB_TX_HEIGHTS = "tx_heights";
const char* const BDB_TX_OUTPUTS = "tx_outputs";

const char* const BDB_OUTPUT_TXS = "output_txs";
const char* const BDB_OUTPUT_INDICES = "output_indices";
const char* const BDB_OUTPUT_AMOUNTS = "output_amounts";
const char* const BDB_OUTPUT_KEYS = "output_keys";

const char* const BDB_SPENT_KEYS = "spent_keys";

template<typename T>
struct Dbt_copy: public Dbt
{
  Dbt_copy(const T &t): t_copy(t)
  {
    init();
  }

  Dbt_copy()
  {
    init();
  }

  void init()
  {
    set_data(&t_copy);
    set_size(sizeof(T));
    set_ulen(sizeof(T));
    set_flags(DB_DBT_USERMEM);
  }

  operator T()
  {
    return t_copy;
  }
private:
  T t_copy;
};

template<>
struct Dbt_copy<cryptonote::blobdata>: public Dbt
{
  Dbt_copy(const cryptonote::blobdata &bd) : m_data(new char[bd.size()])
  {
    memcpy(m_data.get(), bd.data(), bd.size());
    set_data(m_data.get());
    set_size(bd.size());
    set_ulen(bd.size());
    set_flags(DB_DBT_USERMEM);
  }
private:
  std::unique_ptr<char[]> m_data;
};

struct Dbt_safe : public Dbt
{
  Dbt_safe()
  {
    set_data(NULL);
    set_flags(DB_DBT_MALLOC);
  }
  ~Dbt_safe()
  {
    void* buf = get_data();
    if (buf != NULL)
    {
      free(buf);
    }
  }
};

} // anonymous namespace

namespace cryptonote
{

void BlockchainBDB::add_block( const block& blk
              , const size_t& block_size
              , const difficulty_type& cumulative_difficulty
              , const uint64_t& coins_generated
              , const crypto::hash& blk_hash
              )
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<crypto::hash> val_h(blk_hash);
  if (m_block_heights->exists(*m_write_txn, &val_h, 0) == 0)
    throw1(BLOCK_EXISTS("Attempting to add block that's already in the db"));

  if (m_height > 1)
  {
    Dbt_copy<crypto::hash> parent_key(blk.prev_id);
    Dbt_copy<uint32_t> parent_h;
    if (m_block_heights->get(*m_write_txn, &parent_key, &parent_h, 0))
    {
      LOG_PRINT_L3("m_height: " << m_height);
      LOG_PRINT_L3("parent_key: " << blk.prev_id);
      throw0(DB_ERROR("Failed to get top block hash to check for new block's parent"));
    }
    uint32_t parent_height = parent_h;
    if (parent_height != m_height - 1)
      throw0(BLOCK_PARENT_DNE("Top block is not new block's parent"));
  }

  Dbt_copy<uint32_t> key(m_height);

  Dbt_copy<blobdata> blob(block_to_blob(blk));
  auto res = m_blocks->put(*m_write_txn, &key, &blob, 0);
  if (res)
    throw0(DB_ERROR("Failed to add block blob to db transaction."));

  Dbt_copy<size_t> sz(block_size);
  if (m_block_sizes->put(*m_write_txn, &key, &sz, 0))
    throw0(DB_ERROR("Failed to add block size to db transaction."));

  Dbt_copy<uint64_t> ts(blk.timestamp);
  if (m_block_timestamps->put(*m_write_txn, &key, &ts, 0))
    throw0(DB_ERROR("Failed to add block timestamp to db transaction."));

  Dbt_copy<difficulty_type> diff(cumulative_difficulty);
  if (m_block_diffs->put(*m_write_txn, &key, &diff, 0))
    throw0(DB_ERROR("Failed to add block cumulative difficulty to db transaction."));

  Dbt_copy<uint64_t> coinsgen(coins_generated);
  if (m_block_coins->put(*m_write_txn, &key, &coinsgen, 0))
    throw0(DB_ERROR("Failed to add block total generated coins to db transaction."));

  if (m_block_heights->put(*m_write_txn, &val_h, &key, 0))
    throw0(DB_ERROR("Failed to add block height by hash to db transaction."));

  if (m_block_hashes->put(*m_write_txn, &key, &val_h, 0))
    throw0(DB_ERROR("Failed to add block hash to db transaction."));
}

void BlockchainBDB::remove_block()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  if (m_height <= 1)
    throw0(BLOCK_DNE ("Attempting to remove block from an empty blockchain"));

  Dbt_copy<uint32_t> k(m_height - 1);
  Dbt_copy<crypto::hash> h;
  if (m_block_hashes->get(*m_write_txn, &k, &h, 0))
      throw1(BLOCK_DNE("Attempting to remove block that's not in the db"));

  if (m_blocks->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block to db transaction"));

  if (m_block_sizes->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block size to db transaction"));

  if (m_block_diffs->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block cumulative difficulty to db transaction"));

  if (m_block_coins->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block total generated coins to db transaction"));

  if (m_block_timestamps->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block timestamp to db transaction"));

  if (m_block_heights->del(*m_write_txn, &h, 0))
      throw1(DB_ERROR("Failed to add removal of block height by hash to db transaction"));

  if (m_block_hashes->del(*m_write_txn, &k, 0))
      throw1(DB_ERROR("Failed to add removal of block hash to db transaction"));
}

void BlockchainBDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<crypto::hash> val_h(tx_hash);

  if (m_txs->exists(*m_write_txn, &val_h, 0) == 0)
      throw1(TX_EXISTS("Attempting to add transaction that's already in the db"));

  Dbt_copy<blobdata> blob(tx_to_blob(tx));
  if (m_txs->put(*m_write_txn, &val_h, &blob, 0))
    throw0(DB_ERROR("Failed to add tx blob to db transaction"));

  Dbt_copy<uint32_t> height(m_height);
  if (m_tx_heights->put(*m_write_txn, &val_h, &height, 0))
    throw0(DB_ERROR("Failed to add tx block height to db transaction"));

  Dbt_copy<uint64_t> unlock_time(tx.unlock_time);
  if (m_tx_unlocks->put(*m_write_txn, &val_h, &unlock_time, 0))
    throw0(DB_ERROR("Failed to add tx unlock time to db transaction"));
}

void BlockchainBDB::remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<crypto::hash> val_h(tx_hash);
  if (m_txs->exists(*m_write_txn, &val_h, 0))
      throw1(TX_DNE("Attempting to remove transaction that isn't in the db"));

  if (m_txs->del(*m_write_txn, &val_h, 0))
      throw1(DB_ERROR("Failed to add removal of tx to db transaction"));
  if (m_tx_unlocks->del(*m_write_txn, &val_h, 0))
      throw1(DB_ERROR("Failed to add removal of tx unlock time to db transaction"));
  if (m_tx_heights->del(*m_write_txn, &val_h, 0))
      throw1(DB_ERROR("Failed to add removal of tx block height to db transaction"));

  remove_tx_outputs(tx_hash, tx);

  if (m_tx_outputs->del(*m_write_txn, &val_h, 0))
      throw1(DB_ERROR("Failed to add removal of tx outputs to db transaction"));

}

void BlockchainBDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<uint32_t> k(m_num_outputs);
  Dbt_copy<crypto::hash> v(tx_hash);

  if (m_output_txs->put(*m_write_txn, &k, &v, 0))
    throw0(DB_ERROR("Failed to add output tx hash to db transaction"));
  if (m_tx_outputs->put(*m_write_txn, &v, &k, 0))
    throw0(DB_ERROR("Failed to add tx output index to db transaction"));

  Dbt_copy<uint64_t> val_local_index(local_index);
  if (m_output_indices->put(*m_write_txn, &k, &val_local_index, 0))
    throw0(DB_ERROR("Failed to add tx output index to db transaction"));

  Dbt_copy<uint64_t> val_amount(tx_output.amount);
  if (m_output_amounts->put(*m_write_txn, &val_amount, &k, 0))
    throw0(DB_ERROR("Failed to add output amount to db transaction."));

  if (tx_output.target.type() == typeid(txout_to_key))
  {
    Dbt_copy<crypto::public_key> val_pubkey(boost::get<txout_to_key>(tx_output.target).key);
    if (m_output_keys->put(*m_write_txn, &k, &val_pubkey, 0))
      throw0(DB_ERROR("Failed to add output pubkey to db transaction"));
  }

  m_num_outputs++;
}

void BlockchainBDB::remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

  bdb_cur cur(*m_write_txn, m_tx_outputs);

  Dbt_copy<crypto::hash> k(tx_hash);
  Dbt_copy<uint32_t> v;

  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
  {
    throw0(DB_ERROR("Attempting to remove a tx's outputs, but none found."));
  }
  else if (result)
  {
    throw0(DB_ERROR("DB error attempting to get an output"));
  }
  else
  {
    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    for (uint64_t i = 0; i < num_elems; ++i)
    {
      const tx_out tx_output = tx.vout[i];
      remove_output(v, tx_output.amount);
      if (i < num_elems - 1)
      {
        cur->get(&k, &v, DB_NEXT_DUP);
      }
    }
  }

  cur.close();
}

// TODO: probably remove this function
void BlockchainBDB::remove_output(const tx_out& tx_output)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__ << " (unused version - does nothing)");
  return;
}

void BlockchainBDB::remove_output(const uint64_t& out_index, const uint64_t amount)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<uint32_t> k(out_index);

  auto result = m_output_indices->del(*m_write_txn, &k, 0);
  if (result == DB_NOTFOUND)
  {
    LOG_PRINT_L0("Unexpected: global output index not found in m_output_indices");
  }
  else if (result)
  {
    throw1(DB_ERROR("Error adding removal of output tx index to db transaction"));
  }

  result = m_output_txs->del(*m_write_txn, &k, 0);
  // if (result != 0 && result != DB_NOTFOUND)
  //    throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
  if (result == DB_NOTFOUND)
  {
    LOG_PRINT_L0("Unexpected: global output index not found in m_output_txs");
  }
  else if (result)
  {
    throw1(DB_ERROR("Error adding removal of output tx hash to db transaction"));
  }

  result = m_output_keys->del(*m_write_txn, &k, 0);
  if (result == DB_NOTFOUND)
  {
      LOG_PRINT_L0("Unexpected: global output index not found in m_output_keys");
  }
  else if (result)
    throw1(DB_ERROR("Error adding removal of output pubkey to db transaction"));

  remove_amount_output_index(amount, out_index);

  m_num_outputs--;
}

void BlockchainBDB::remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_cur cur(*m_write_txn, m_output_amounts);

  Dbt_copy<uint64_t> k(amount);
  Dbt_copy<uint32_t> v;

  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  db_recno_t num_elems = 0;
  cur->count(&num_elems, 0);

  uint64_t amount_output_index = 0;
  uint64_t goi = 0;
  bool found_index = false;
  for (uint64_t i = 0; i < num_elems; ++i)
  {
    goi = v;
    if (goi == global_output_index)
    {
      amount_output_index = i;
      found_index = true;
      break;
    }
    cur->get(&k, &v, DB_NEXT_DUP);
  }
  if (found_index)
  {
    // found the amount output index
    // now delete it
    result = cur->del(0);
    if (result)
      throw0(DB_ERROR(std::string("Error deleting amount output index ").append(boost::lexical_cast<std::string>(amount_output_index)).c_str()));
  }
  else
  {
    // not found
    throw1(OUTPUT_DNE("Failed to find amount output index"));
  }
  cur.close();
}

void BlockchainBDB::add_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<crypto::key_image> val_key(k_image);
  if (m_spent_keys->exists(*m_write_txn, &val_key, 0) == 0)
      throw1(KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db"));

  Dbt_copy<char> val('\0');
  if (m_spent_keys->put(*m_write_txn, &val_key, &val, 0))
    throw1(DB_ERROR("Error adding spent key image to db transaction."));
}

void BlockchainBDB::remove_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  Dbt_copy<crypto::key_image> k(k_image);
  auto result = m_spent_keys->del(*m_write_txn, &k, 0);
  if (result != 0 && result != DB_NOTFOUND)
      throw1(DB_ERROR("Error adding removal of key image to db transaction"));
}

blobdata BlockchainBDB::output_to_blob(const tx_out& output)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  blobdata b;
  if (!t_serializable_object_to_blob(output, b))
    throw1(DB_ERROR("Error serializing output to blob"));
  return b;
}

tx_out BlockchainBDB::output_from_blob(const blobdata& blob) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  std::stringstream ss;
  ss << blob;
  binary_archive<false> ba(ss);
  tx_out o;

  if (!(::serialization::serialize(ba, o)))
    throw1(DB_ERROR("Error deserializing tx output blob"));

  return o;
}

uint64_t BlockchainBDB::get_output_global_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  bdb_cur cur(txn, m_output_amounts);

  Dbt_copy<uint64_t> k(amount);
  Dbt_copy<uint32_t> v;

  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  db_recno_t num_elems;
  cur->count(&num_elems, 0);

  if (num_elems <= index)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

  for (uint64_t i = 0; i < index; ++i)
  {
    cur->get(&k, &v, DB_NEXT_DUP);
  }

  uint64_t glob_index = v;

  cur.close();

  txn.commit();

  return glob_index;
}

void BlockchainBDB::check_open() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  if (!m_open)
    throw0(DB_ERROR("DB operation attempted on a not-open DB instance"));
}

BlockchainBDB::~BlockchainBDB()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

  if (m_open)
  {
    close();
  }
}

BlockchainBDB::BlockchainBDB(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";
  m_open = false;

  m_batch_transactions = batch_transactions;
  m_write_txn = nullptr;
  m_height = 1;
}

void BlockchainBDB::open(const std::string& filename)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

  if (m_open)
    throw0(DB_OPEN_FAILURE("Attempted to open db, but it's already open"));

  boost::filesystem::path direc(filename);
  if (boost::filesystem::exists(direc))
  {
    if (!boost::filesystem::is_directory(direc))
      throw0(DB_OPEN_FAILURE("DB needs a directory path, but a file was passed"));
  }
  else
  {
    if (!boost::filesystem::create_directory(direc))
      throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
  }

  m_folder = filename;

  try
  {

    //Create BerkeleyDB environment
    m_env = new DbEnv(0);  // no flags needed for DbEnv

    uint32_t db_env_open_flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK
                               | DB_INIT_LOG | DB_INIT_TXN | DB_RECOVER
                               | DB_THREAD;

    // last parameter left 0, files will be created with default rw access
    m_env->open(filename.c_str(), db_env_open_flags, 0);

    // begin transaction to init dbs
    bdb_txn_safe txn;
    m_env->txn_begin(NULL, txn, 0);

    // create Dbs in the environment
    m_blocks = new Db(m_env, 0);
    m_block_heights = new Db(m_env, 0);
    m_block_hashes = new Db(m_env, 0);
    m_block_timestamps = new Db(m_env, 0);
    m_block_sizes = new Db(m_env, 0);
    m_block_diffs = new Db(m_env, 0);
    m_block_coins = new Db(m_env, 0);

    m_txs = new Db(m_env, 0);
    m_tx_unlocks = new Db(m_env, 0);
    m_tx_heights = new Db(m_env, 0);
    m_tx_outputs = new Db(m_env, 0);

    m_output_txs = new Db(m_env, 0);
    m_output_indices = new Db(m_env, 0);
    m_output_amounts = new Db(m_env, 0);
    m_output_keys = new Db(m_env, 0);

    m_spent_keys = new Db(m_env, 0);

    // Tell DB about Dbs that need duplicate support
    // Note: no need to tell about sorting,
    //   as the default is insertion order, which we want
    m_tx_outputs->set_flags(DB_DUP);
    m_output_amounts->set_flags(DB_DUP);

    // Tell DB about fixed-size values.
    m_block_hashes->set_re_len(sizeof(crypto::hash));
    m_block_timestamps->set_re_len(sizeof(uint64_t));
    m_block_sizes->set_re_len(sizeof(size_t));  // should really store block size as uint64_t...
    m_block_diffs->set_re_len(sizeof(difficulty_type));
    m_block_coins->set_re_len(sizeof(uint64_t));

    m_output_txs->set_re_len(sizeof(crypto::hash));
    m_output_indices->set_re_len(sizeof(uint64_t));
    m_output_keys->set_re_len(sizeof(crypto::public_key));

    //TODO: Find out if we need to do Db::set_flags(DB_RENUMBER)
    //      for the RECNO databases.  We shouldn't as we're only
    //      inserting/removing from the end, but we'll see.

    // open Dbs in the environment
    // m_tx_outputs and m_output_amounts must be DB_HASH or DB_BTREE
    //   because they need duplicate entry support.  The rest are DB_RECNO,
    //   as it seems that will be the most performant choice.
    m_blocks->open(txn, BDB_BLOCKS, NULL, DB_RECNO, DB_CREATE, 0);

    m_block_timestamps->open(txn, BDB_BLOCK_TIMESTAMPS, NULL, DB_RECNO, DB_CREATE, 0);
    m_block_heights->open(txn, BDB_BLOCK_HEIGHTS, NULL, DB_HASH, DB_CREATE, 0);
    m_block_hashes->open(txn, BDB_BLOCK_HASHES, NULL, DB_RECNO, DB_CREATE, 0);
    m_block_sizes->open(txn, BDB_BLOCK_SIZES, NULL, DB_RECNO, DB_CREATE, 0);
    m_block_diffs->open(txn, BDB_BLOCK_DIFFS, NULL, DB_RECNO, DB_CREATE, 0);
    m_block_coins->open(txn, BDB_BLOCK_COINS, NULL, DB_RECNO, DB_CREATE, 0);

    m_txs->open(txn, BDB_TXS, NULL, DB_HASH, DB_CREATE, 0);
    m_tx_unlocks->open(txn, BDB_TX_UNLOCKS, NULL, DB_HASH, DB_CREATE, 0);
    m_tx_heights->open(txn, BDB_TX_HEIGHTS, NULL, DB_HASH, DB_CREATE, 0);
    m_tx_outputs->open(txn, BDB_TX_OUTPUTS, NULL, DB_HASH, DB_CREATE, 0);

    m_output_txs->open(txn, BDB_OUTPUT_TXS, NULL, DB_RECNO, DB_CREATE, 0);
    m_output_indices->open(txn, BDB_OUTPUT_INDICES, NULL, DB_RECNO, DB_CREATE, 0);
    m_output_amounts->open(txn, BDB_OUTPUT_AMOUNTS, NULL, DB_HASH, DB_CREATE, 0);
    m_output_keys->open(txn, BDB_OUTPUT_KEYS, NULL, DB_RECNO, DB_CREATE, 0);

    m_spent_keys->open(txn, BDB_SPENT_KEYS, NULL, DB_HASH, DB_CREATE, 0);

    DB_BTREE_STAT* stats;

    // DB_FAST_STAT can apparently cause an incorrect number of records
    // to be returned.  The flag should be set to 0 instead if this proves
    // to be the case.
    m_blocks->stat(txn, &stats, DB_FAST_STAT);
    m_height = stats->bt_nkeys + 1;
    delete stats;

    // see above comment about DB_FAST_STAT
    m_output_indices->stat(txn, &stats, DB_FAST_STAT);
    m_num_outputs = stats->bt_nkeys + 1;
    delete stats;

    txn.commit();
  }
  catch (const std::exception& e)
  {
    throw0(DB_OPEN_FAILURE(e.what()));
  }

  m_open = true;
}

// unused for now, create will happen on open if doesn't exist
void BlockchainBDB::create(const std::string& filename)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  throw DB_CREATE_FAILURE("create() is not implemented for this BlockchainDB, open() will create files if needed.");
}

void BlockchainBDB::close()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  this->sync();

  // FIXME: not yet thread safe!!!  Use with care.
  m_open = false;
  m_env->close(DB_FORCESYNC);
}

void BlockchainBDB::sync()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  try
  {
    m_blocks->sync(0);
    m_block_heights->sync(0);
    m_block_hashes->sync(0);
    m_block_timestamps->sync(0);
    m_block_sizes->sync(0);
    m_block_diffs->sync(0);
    m_block_coins->sync(0);

    m_txs->sync(0);
    m_tx_unlocks->sync(0);
    m_tx_heights->sync(0);
    m_tx_outputs->sync(0);

    m_output_txs->sync(0);
    m_output_indices->sync(0);
    m_output_amounts->sync(0);
    m_output_keys->sync(0);

    m_spent_keys->sync(0);
  }
  catch (const std::exception& e)
  {
    throw0(DB_ERROR(std::string("Failed to sync database: ").append(e.what()).c_str()));
  }
}

void BlockchainBDB::reset()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // TODO: this
}

std::vector<std::string> BlockchainBDB::get_filenames() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  std::vector<std::string> filenames;

  char *fname, *dbname;
  const char **pfname, **pdbname;

  pfname = (const char **)&fname;
  pdbname = (const char **)&dbname;

  m_blocks->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_heights->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_hashes->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_timestamps->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_sizes->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_diffs->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_block_coins->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_txs->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_tx_unlocks->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_tx_heights->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_tx_outputs->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_output_txs->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_output_indices->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_output_amounts->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_output_keys->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  m_spent_keys->get_dbname(pfname, pdbname);
  filenames.push_back(fname);

  std::vector<std::string> full_paths;

  for (auto& filename : filenames)
  {
    boost::filesystem::path p(m_folder);
    p /= filename;
    full_paths.push_back(p.string());
  }

  return full_paths;
}

std::string BlockchainBDB::get_db_name() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

  return std::string("BerkeleyDB");
}

// TODO: this?
bool BlockchainBDB::lock()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  return false;
}

// TODO: this?
void BlockchainBDB::unlock()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

bool BlockchainBDB::block_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);

  auto get_result = m_block_heights->exists(txn, &key, 0);
  if (get_result == DB_NOTFOUND)
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

block BlockchainBDB::get_block(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  return get_block_from_height(get_block_height(h));
}

uint64_t BlockchainBDB::get_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);
  Dbt_copy<uint64_t> result;

  auto get_result = m_block_heights->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
    throw1(BLOCK_DNE("Attempted to retrieve non-existent block height"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block height from the db"));

  txn.commit();

  return (uint64_t)result - 1;
}

block_header BlockchainBDB::get_block_header(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  // block_header object is automatically cast from block object
  return get_block(h);
}

block BlockchainBDB::get_block_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_safe result;
  auto get_result = m_blocks->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get block from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block from the db"));

  txn.commit();

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.get_data()), result.get_size());

  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

  return b;
}

uint64_t BlockchainBDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_copy<uint64_t> result;
  auto get_result = m_block_timestamps->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get timestamp from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- timestamp not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a timestamp from the db"));

  txn.commit();
  return result;
}

uint64_t BlockchainBDB::get_top_block_timestamp() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  // if no blocks, return 0
  if (m_height <= 0)
  {
    return 0;
  }

  return get_block_timestamp(m_height - 1);
}

size_t BlockchainBDB::get_block_size(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_copy<size_t> result;
  auto get_result = m_block_sizes->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get block size from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block size from the db"));

  txn.commit();
  return result;
}

difficulty_type BlockchainBDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__ << "  height: " << height);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_copy<difficulty_type> result;
  auto get_result = m_block_diffs->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get cumulative difficulty from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- difficulty not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db"));

  txn.commit();
  return result;
}

difficulty_type BlockchainBDB::get_block_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
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

uint64_t BlockchainBDB::get_block_already_generated_coins(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_copy<uint64_t> result;
  auto get_result = m_block_coins->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(DB_ERROR(std::string("Attempt to get generated coins from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a total generated coins from the db"));

  txn.commit();
  return result;
}

crypto::hash BlockchainBDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> key(height + 1);
  Dbt_copy<crypto::hash> result;
  auto get_result = m_block_hashes->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get hash from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- hash not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block hash from the db."));

  txn.commit();
  return result;
}

std::vector<block> BlockchainBDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  std::vector<block> v;

  for (uint64_t height = h1; height <= h2; ++height)
  {
    v.push_back(get_block_from_height(height));
  }

  return v;
}

std::vector<crypto::hash> BlockchainBDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  std::vector<crypto::hash> v;

  for (uint64_t height = h1; height <= h2; ++height)
  {
    v.push_back(get_block_hash_from_height(height));
  }

  return v;
}

crypto::hash BlockchainBDB::top_block_hash() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  if (m_height != 0)
  {
    return get_block_hash_from_height(m_height - 1);
  }

  return null_hash;
}

block BlockchainBDB::get_top_block() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  if (m_height != 0)
  {
    return get_block_from_height(m_height - 1);
  }

  block b;
  return b;
}

uint64_t BlockchainBDB::height() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  return m_height - 1;
}

bool BlockchainBDB::tx_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);

  TIME_MEASURE_START(time1);
  auto get_result = m_txs->exists(txn, &key, 0);
  TIME_MEASURE_FINISH(time1);
  time_tx_exists += time1;
  if (get_result == DB_NOTFOUND)
  {
    txn.commit();
    LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
    return false;
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch transaction from hash"));

  return true;
}

uint64_t BlockchainBDB::get_tx_unlock_time(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);
  Dbt_copy<uint64_t> result;
  auto get_result = m_tx_unlocks->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
    throw1(TX_DNE(std::string("tx unlock time with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx unlock time from hash"));

  return result;
}

transaction BlockchainBDB::get_tx(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);
  Dbt_safe result;
  auto get_result = m_txs->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
    throw1(TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx from hash"));

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.get_data()), result.get_size());

  transaction tx;
  if (!parse_and_validate_tx_from_blob(bd, tx))
    throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));

  txn.commit();

  return tx;
}

uint64_t BlockchainBDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  DB_BTREE_STAT* stats;

  // DB_FAST_STAT can apparently cause an incorrect number of records
  // to be returned.  The flag should be set to 0 instead if this proves
  // to be the case.
  m_txs->stat(txn, &stats, DB_FAST_STAT);
  auto num_txs = stats->bt_nkeys;
  delete stats;

  txn.commit();

  return num_txs;
}

std::vector<transaction> BlockchainBDB::get_tx_list(const std::vector<crypto::hash>& hlist) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  std::vector<transaction> v;

  for (auto& h : hlist)
  {
    v.push_back(get_tx(h));
  }

  return v;
}

uint64_t BlockchainBDB::get_tx_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::hash> key(h);
  Dbt_copy<uint64_t> result;
  auto get_result = m_tx_heights->get(txn, &key, &result, 0);
  if (get_result == DB_NOTFOUND)
  {
    throw1(TX_DNE(std::string("tx height with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx height from hash"));

  txn.commit();

  return (uint64_t)result - 1;
}

//FIXME: make sure the random method used here is appropriate
uint64_t BlockchainBDB::get_random_output(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  uint64_t num_outputs = get_num_outputs(amount);
  if (num_outputs == 0)
    throw1(OUTPUT_DNE("Attempting to get a random output for an amount, but none exist"));

  return crypto::rand<uint64_t>() % num_outputs;
}

uint64_t BlockchainBDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  bdb_cur cur(txn, m_output_amounts);

  Dbt_copy<uint64_t> k(amount);
  Dbt_copy<uint64_t> v;
  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
  {
    return 0;
  }
  else if (result)
    throw0(DB_ERROR("DB error attempting to get number of outputs of an amount"));

  db_recno_t num_elems = 0;
  cur->count(&num_elems, 0);

  txn.commit();

  return num_elems;
}

crypto::public_key BlockchainBDB::get_output_key(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  uint64_t glob_index = get_output_global_index(amount, index);

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> k(glob_index);
  Dbt_copy<crypto::public_key> v;
  auto get_result = m_output_keys->get(txn, &k, &v, 0);
  if (get_result == DB_NOTFOUND)
    throw0(DB_ERROR("Attempting to get output pubkey by global index, but key does not exist"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));

  return v;
}

// As this is not used, its return is now a blank output.
// This will save on space in the db.
tx_out BlockchainBDB::get_output(const crypto::hash& h, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  return tx_out();
}

// As this is not used, its return is now a blank output.
// This will save on space in the db.
tx_out BlockchainBDB::get_output(const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  return tx_out();
}

tx_out_index BlockchainBDB::get_output_tx_and_index_from_global(const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<uint32_t> k(index);
  Dbt_copy<crypto::hash > v;

  auto get_result = m_output_txs->get(txn, &k, &v, 0);
  if (get_result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

  crypto::hash tx_hash = v;

  Dbt_copy<uint64_t> result;
  get_result = m_output_indices->get(txn, &k, &result, 0);
  if (get_result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx index"));

  txn.commit();

  return tx_out_index(tx_hash, result);
}

tx_out_index BlockchainBDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  bdb_cur cur(txn, m_output_amounts);

  Dbt_copy<uint64_t> k(amount);
  Dbt_copy<uint64_t> v;

  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  db_recno_t num_elems = 0;
  cur->count(&num_elems, 0);

  if (num_elems <= index)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

  for (uint64_t i = 0; i < index; ++i)
  {
    cur->get(&k, &v, DB_NEXT_DUP);
  }

  uint64_t glob_index = v;

  cur.close();

  txn.commit();

  return get_output_tx_and_index_from_global(glob_index);
}

std::vector<uint64_t> BlockchainBDB::get_tx_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  bdb_cur cur(txn, m_tx_outputs);

  Dbt_copy<crypto::hash> k(h);
  Dbt_copy<uint64_t> v;
  auto result = cur->get(&k, &v, DB_SET);
  if (result == DB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  db_recno_t num_elems = 0;
  cur->count(&num_elems, 0);

  for (uint64_t i = 0; i < num_elems; ++i)
  {
    index_vec.push_back(v);
    cur->get(&k, &v, DB_NEXT_DUP);
  }

  cur.close();
  txn.commit();

  return index_vec;
}

std::vector<uint64_t> BlockchainBDB::get_tx_amount_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;
  std::vector<uint64_t> index_vec2;

  // get the transaction's global output indices first
  index_vec = get_tx_output_indices(h);
  // these are next used to obtain the amount output indices

  transaction tx = get_tx(h);

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  uint64_t i = 0;
  uint64_t global_index;
  for (const auto& vout : tx.vout)
  {
    uint64_t amount =  vout.amount;

    global_index = index_vec[i];

    bdb_cur cur(txn, m_output_amounts);

    Dbt_copy<uint64_t> k(amount);
    Dbt_copy<uint64_t> v;

    auto result = cur->get(&k, &v, DB_SET);
    if (result == DB_NOTFOUND)
      throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
    else if (result)
      throw0(DB_ERROR("DB error attempting to get an output"));

    db_recno_t num_elems = 0;
    cur->count(&num_elems, 0);

    uint64_t amount_output_index = 0;
    uint64_t output_index = 0;
    bool found_index = false;
    for (uint64_t j = 0; j < num_elems; ++j)
    {
      output_index = v;
      if (output_index == global_index)
      {
        amount_output_index = j;
        found_index = true;
        break;
      }
      cur->get(&k, &v, DB_NEXT_DUP);
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



bool BlockchainBDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));

  Dbt_copy<crypto::key_image> val_key(img);
  if (m_spent_keys->exists(txn, &val_key, 0) == 0)
  {
    txn.commit();
    return true;
  }

  txn.commit();
  return false;
}

// Ostensibly BerkeleyDB has batch transaction support built-in,
// so the following few functions will be NOP.

void BlockchainBDB::batch_start()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::batch_commit()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::batch_stop()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::batch_abort()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::set_batch_transactions(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  m_batch_transactions = batch_transactions;
  LOG_PRINT_L3("batch transactions " << (m_batch_transactions ? "enabled" : "disabled"));
}

uint64_t BlockchainBDB::add_block( const block& blk
                                  , const size_t& block_size
                                  , const difficulty_type& cumulative_difficulty
                                  , const uint64_t& coins_generated
                                  , const std::vector<transaction>& txs
                                  )
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));
  m_write_txn = &txn;

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
    m_write_txn = NULL;

    TIME_MEASURE_START(time1);
    txn.commit();
    TIME_MEASURE_FINISH(time1);
    time_commit1 += time1;
  }
  catch (const std::exception& e)
  {
    m_num_outputs = num_outputs;
    m_write_txn = NULL;
    throw;
  }

  m_height++;

  return m_height - 1;
}

void BlockchainBDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  bdb_txn_safe txn;
  if (m_env->txn_begin(NULL, txn, 0))
    throw0(DB_ERROR("Failed to create a transaction for the db"));
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
