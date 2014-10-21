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

namespace cryptonote
{

#define CN_LMDB_DBI_OPEN( txn, flags, dbi ) \
  if (mdb_dbi_open(txn, NULL, flags, &dbi)) \
  { \
    throw DB_OPEN_FAILURE( "Failed to open db handle for ##dbi" ); \
  }


void BlockchainLMDB::add_block( const block& blk
              , const size_t& block_size
              , const difficulty_type& cumulative_difficulty
              , const uint64_t& coins_generated
              )
{
}

void BlockchainLMDB::remove_block(const crypto::hash& blk_hash)
{
}

void BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx)
{
}

void BlockchainLMDB::remove_transaction_data(const crypto::hash& tx_hash)
{
}

void BlockchainLMDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
}

void BlockchainLMDB::remove_output(const tx_out& tx_output)
{
}

void BlockchainLMDB::add_spent_key(const crypto::key_image& k_image)
{
}

void BlockchainLMDB::remove_spent_key(const crypto::key_image& k_image)
{
}

BlockchainLMDB::~BlockchainLMDB()
{
}

BlockchainLMDB::BlockchainLMDB()
{
}

void BlockchainLMDB::open(const std::string& filename)
{
  if (mdb_env_create(&m_env))
  {
    throw DB_ERROR("Failed to create lmdb environment");
  }
  if (mdb_env_open(m_env, filename.c_str(), 0, 0664))
  {
    throw DB_ERROR("Failed to open lmdb environment");
  }
  if (mdb_env_set_maxdbs(m_env, 15))
  {
    throw DB_ERROR("Failed to set max number of dbs");
  }
  MDB_txn* txn;
  if (mdb_txn_begin(m_env, NULL, 0, &txn))
  {
    throw DB_ERROR("Failed to create a transaction for the db");
  }

  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_blocks)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_block_txs)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_block_hashes)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_block_sizes)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_block_diffs)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_block_coins)
  CN_LMDB_DBI_OPEN(txn, MDB_CREATE | MDB_DUPSORT, m_txs)

  if (mdb_txn_commit(txn))
  {
    throw DB_OPEN_FAILURE("Failed to commit db open transaction");
  }

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
  std::vector<std::string> retval;
  return retval;
}


bool BlockchainLMDB::lock()
{
  return false;
}

void BlockchainLMDB::unlock()
{
}


bool BlockchainLMDB::block_exists(const crypto::hash& h)
{
  return false;
}

block BlockchainLMDB::get_block(const crypto::hash& h)
{
  block b;
  return b;
}

uint64_t BlockchainLMDB::get_block_height(const crypto::hash& h)
{
  return 0;
}

block_header BlockchainLMDB::get_block_header(const crypto::hash& h)
{
  block_header bh;
  return bh;
}

block BlockchainLMDB::get_block_from_height(const uint64_t& height)
{
  block b;
  return b;
}

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) 
{
  return 0;
}

uint64_t BlockchainLMDB::get_top_block_timestamp()
{
  return 0;
}

size_t BlockchainLMDB::get_block_size(const uint64_t& height)
{
  return 0;
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height)
{
  return 0;
}

difficulty_type BlockchainLMDB::get_block_difficulty(const uint64_t& height)
{
  return 0;
}

uint64_t BlockchainLMDB::get_block_already_generated_coins(const uint64_t& height)
{
  return 0;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height)
{
  crypto::hash h;
  return h;
}

std::vector<block> BlockchainLMDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2)
{
  std::vector<block> v;
  return v;
}

std::vector<crypto::hash> BlockchainLMDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2)
{
  std::vector<crypto::hash> v;
  return v;
}

crypto::hash BlockchainLMDB::top_block_hash()
{
  crypto::hash h;
  return h;
}

block BlockchainLMDB::get_top_block()
{
  block b;
  return b;
}

uint64_t BlockchainLMDB::height()
{
  return 0;
}


bool BlockchainLMDB::tx_exists(const crypto::hash& h)
{
  return false;
}

uint64_t BlockchainLMDB::get_tx_unlock_time(const crypto::hash& h)
{
  return 0;
}

transaction BlockchainLMDB::get_tx(const crypto::hash& h)
{
  transaction t;
  return t;
}

uint64_t BlockchainLMDB::get_tx_count()
{
  return 0;
}

std::vector<transaction> BlockchainLMDB::get_tx_list(const std::vector<crypto::hash>& hlist)
{
  std::vector<transaction> v;
  return v;
}

uint64_t BlockchainLMDB::get_tx_block_height(const crypto::hash& h)
{
  return 0;
}

uint64_t BlockchainLMDB::get_random_output(const uint64_t& amount)
{
  return 0;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount)
{
  return 0;
}

crypto::public_key BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index)
{
  crypto::public_key pk;
  return pk;
}

tx_out BlockchainLMDB::get_output(const crypto::hash& h, const uint64_t& index)
{
  tx_out o;
  return o;
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index)
{
  tx_out_index i;
  return i;
}

std::vector<uint64_t> BlockchainLMDB::get_tx_output_indices(const crypto::hash& h)
{
  std::vector<uint64_t> v;
  return v;
}

bool BlockchainLMDB::has_key_image(const crypto::key_image& img)
{
  return false;
}

}  // namespace cryptonote
