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

namespace cryptonote
{

const char* LMDB_BLOCKS = "blocks";
const char* LMDB_BLOCK_HASHES = "block_hashes";
const char* LMDB_BLOCK_SIZES = "block_sizes";
const char* LMDB_BLOCK_DIFFS = "block_diffs";
const char* LMDB_BLOCK_COINS = "block_coins";
const char* LMDB_TXS = "txs";

void BlockchainLMDB::add_block( const block& blk
              , const size_t& block_size
              , const difficulty_type& cumulative_difficulty
              , const uint64_t& coins_generated
              )
{
  check_open();

  crypto::hash h = get_block_hash(blk);
  MDB_val val_h;
  val_h.mv_size = sizeof(crypto::hash);
  val_h.mv_data = &h;

  MDB_val unused;
  if (mdb_get(*m_write_txn, m_block_hashes, &val_h, &unused) == 0)
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
    if (mdb_get(*m_write_txn, m_block_hashes, &parent_key, &parent_h))
    {
      LOG_PRINT_L0("Failed to get top block hash to check for new block's parent");
      throw DB_ERROR("Failed to get top block hash to check for new block's parent");
    }

    uint64_t parent_height = *(uint64_t *)parent_h.mv_data;
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

  if (mdb_put(*m_write_txn, m_block_hashes, &val_h, &key, 0))
  {
    LOG_PRINT_L0("Failed to add block size to db transaction");
    throw DB_ERROR("Failed to add block size to db transaction");
  }

}

void BlockchainLMDB::remove_block()
{
  check_open();
}

void BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx)
{
  check_open();
}

void BlockchainLMDB::remove_transaction_data(const crypto::hash& tx_hash)
{
  check_open();
}

void BlockchainLMDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
  check_open();
}

void BlockchainLMDB::remove_output(const tx_out& tx_output)
{
  check_open();
}

void BlockchainLMDB::add_spent_key(const crypto::key_image& k_image)
{
  check_open();
}

void BlockchainLMDB::remove_spent_key(const crypto::key_image& k_image)
{
  check_open();
}

void BlockchainLMDB::check_open()
{
  if (!m_open)
  {
    LOG_PRINT_L0("DB operation attempted on a not-open DB instance");
    throw DB_ERROR("DB operation attempted on a not-open DB instance");
  }
}

BlockchainLMDB::~BlockchainLMDB()
{
}

BlockchainLMDB::BlockchainLMDB()
{
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";
  m_open = false;
}

void BlockchainLMDB::open(const std::string& filename)
{

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
    throw DB_OPEN_FAILURE("Failed to create lmdb environment");
  }
  if (mdb_env_set_maxdbs(m_env, 15))
  {
    LOG_PRINT_L0("Failed to set max number of dbs");
    throw DB_OPEN_FAILURE("Failed to set max number of dbs");
  }
  if (mdb_env_open(m_env, filename.c_str(), 0, 0664))
  {
    LOG_PRINT_L0("Failed to open lmdb environment");
    throw DB_OPEN_FAILURE("Failed to open lmdb environment");
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
  if (mdb_dbi_open(txn, LMDB_BLOCKS, MDB_INTEGERKEY | MDB_CREATE, &m_blocks))
  {
    LOG_PRINT_L0("Failed to open db handle for m_blocks");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_blocks" );
  }

  if (mdb_dbi_open(txn, LMDB_BLOCK_HASHES, MDB_INTEGERKEY | MDB_CREATE, &m_block_hashes))
  {
    LOG_PRINT_L0("Failed to open db handle for m_block_hashes");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_block_hashes" );
  }
  if (mdb_dbi_open(txn, LMDB_BLOCK_SIZES, MDB_INTEGERKEY | MDB_CREATE, &m_block_sizes))
  {
    LOG_PRINT_L0("Failed to open db handle for m_block_sizes");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_block_sizes" );
  }
  if (mdb_dbi_open(txn, LMDB_BLOCK_DIFFS, MDB_INTEGERKEY | MDB_CREATE, &m_block_diffs))
  {
    LOG_PRINT_L0("Failed to open db handle for m_block_diffs");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_block_diffs" );
  }
  if (mdb_dbi_open(txn, LMDB_BLOCK_COINS, MDB_INTEGERKEY | MDB_CREATE, &m_block_coins))
  {
    LOG_PRINT_L0("Failed to open db handle for m_block_coins");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_block_coins" );
  }
  if (mdb_dbi_open(txn, LMDB_TXS, MDB_CREATE, &m_txs))
  {
    LOG_PRINT_L0("Failed to open db handle for m_txs");
    throw DB_OPEN_FAILURE( "Failed to open db handle for m_txs" );
  }

  // get and keep current height
  MDB_stat db_stats;
  if (mdb_stat(txn, m_blocks, &db_stats))
  {
    LOG_PRINT_L0("Failed to query m_blocks");
    throw DB_ERROR("Failed to query m_blocks");
  }
  m_height = db_stats.ms_entries;

  // commit the transaction
  txn.commit();

  m_open = true;
  // from here, init should be finished
}

// unused for now, create will happen on open if doesn't exist
void BlockchainLMDB::create(const std::string& filename)
{
}

void BlockchainLMDB::close()
{
}

void BlockchainLMDB::sync()
{
}

void BlockchainLMDB::reset()
{
}

std::vector<std::string> BlockchainLMDB::get_filenames()
{
  std::vector<std::string> filenames;

  boost::filesystem::path datafile(m_folder);
  datafile /= "data.mdb";
  boost::filesystem::path lockfile(m_folder);
  lockfile /= "lock.mdb";

  filenames.push_back(datafile.string());
  filenames.push_back(lockfile.string());

  return filenames;
}


bool BlockchainLMDB::lock()
{
  check_open();
  return false;
}

void BlockchainLMDB::unlock()
{
  check_open();
}


bool BlockchainLMDB::block_exists(const crypto::hash& h)
{
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
  auto get_result = mdb_get(txn, m_block_hashes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Block with hash " << epee::string_tools::pod_to_hex(h) << "not found in db");
    return false;
  }
  else if (get_result)
  {
    LOG_PRINT_L0("DB error attempting to fetch block index from hash");
    throw DB_ERROR("DB error attempting to fetch block index from hash");
  }

  return true;
}

block BlockchainLMDB::get_block(const crypto::hash& h)
{
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
  auto get_result = mdb_get(txn, m_block_hashes, &key, &result);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L1("Attempted to retrieve non-existent block");
    throw BLOCK_DNE("Attempted to retrieve non-existent block");
  }
  else if (get_result)
  {
    LOG_PRINT_L0("Error attempting to retrieve a block index from the db");
    throw DB_ERROR("Error attempting to retrieve a block index from the db");
  }

  txn.commit();

  uint64_t index = *(uint64_t*)result.mv_data;

  return get_block_from_height(index);
}

uint64_t BlockchainLMDB::get_block_height(const crypto::hash& h)
{
  check_open();
  return 0;
}

block_header BlockchainLMDB::get_block_header(const crypto::hash& h)
{
  check_open();
  block_header bh;
  return bh;
}

block BlockchainLMDB::get_block_from_height(const uint64_t& height)
{
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

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) 
{
  check_open();
  return 0;
}

uint64_t BlockchainLMDB::get_top_block_timestamp()
{
  check_open();
  return 0;
}

size_t BlockchainLMDB::get_block_size(const uint64_t& height)
{
  check_open();
  return 0;
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height)
{
  check_open();
  return 0;
}

difficulty_type BlockchainLMDB::get_block_difficulty(const uint64_t& height)
{
  check_open();
  return 0;
}

uint64_t BlockchainLMDB::get_block_already_generated_coins(const uint64_t& height)
{
  check_open();
  return 0;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height)
{
  check_open();
  crypto::hash h;
  return h;
}

std::vector<block> BlockchainLMDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2)
{
  check_open();
  std::vector<block> v;
  return v;
}

std::vector<crypto::hash> BlockchainLMDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2)
{
  check_open();
  std::vector<crypto::hash> v;
  return v;
}

crypto::hash BlockchainLMDB::top_block_hash()
{
  check_open();
  crypto::hash h;
  return h;
}

block BlockchainLMDB::get_top_block()
{
  check_open();
  block b;
  return b;
}

uint64_t BlockchainLMDB::height()
{
  check_open();
  return 0;
}


bool BlockchainLMDB::tx_exists(const crypto::hash& h)
{
  check_open();
  return false;
}

uint64_t BlockchainLMDB::get_tx_unlock_time(const crypto::hash& h)
{
  check_open();
  return 0;
}

transaction BlockchainLMDB::get_tx(const crypto::hash& h)
{
  check_open();
  transaction t;
  return t;
}

uint64_t BlockchainLMDB::get_tx_count()
{
  check_open();
  return 0;
}

std::vector<transaction> BlockchainLMDB::get_tx_list(const std::vector<crypto::hash>& hlist)
{
  check_open();
  std::vector<transaction> v;
  return v;
}

uint64_t BlockchainLMDB::get_tx_block_height(const crypto::hash& h)
{
  check_open();
  return 0;
}

uint64_t BlockchainLMDB::get_random_output(const uint64_t& amount)
{
  check_open();
  return 0;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount)
{
  check_open();
  return 0;
}

crypto::public_key BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index)
{
  check_open();
  crypto::public_key pk;
  return pk;
}

tx_out BlockchainLMDB::get_output(const crypto::hash& h, const uint64_t& index)
{
  check_open();
  tx_out o;
  return o;
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index)
{
  check_open();
  tx_out_index i;
  return i;
}

std::vector<uint64_t> BlockchainLMDB::get_tx_output_indices(const crypto::hash& h)
{
  check_open();
  std::vector<uint64_t> v;
  return v;
}

bool BlockchainLMDB::has_key_image(const crypto::key_image& img)
{
  check_open();
  return false;
}

uint64_t BlockchainLMDB::add_block( const block& blk
                                  , const size_t& block_size
                                  , const difficulty_type& cumulative_difficulty
                                  , const uint64_t& coins_generated
                                  , const std::vector<transaction>& txs
                                  )
{
  check_open();
  txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
  {
    LOG_PRINT_L0("Failed to create a transaction for the db");
    throw DB_ERROR("Failed to create a transaction for the db");
  }
  m_write_txn = &txn;

  BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);

  txn.commit();

  m_height++;
  return m_height - 1;
}

}  // namespace cryptonote
