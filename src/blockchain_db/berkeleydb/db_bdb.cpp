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
}

void BlockchainBDB::remove_block()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

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
}

void BlockchainBDB::remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::add_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::remove_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

blobdata BlockchainBDB::output_to_blob(const tx_out& output)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

tx_out BlockchainBDB::output_from_blob(const blobdata& blob) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

uint64_t BlockchainBDB::get_output_global_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
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

}

BlockchainBDB::BlockchainBDB(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
}

void BlockchainBDB::open(const std::string& filename)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);

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
}

void BlockchainBDB::sync()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

void BlockchainBDB::reset()
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  // TODO: this
}

std::vector<std::string> BlockchainBDB::get_filenames() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
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
}

block BlockchainBDB::get_block(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

block_header BlockchainBDB::get_block_header(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

block BlockchainBDB::get_block_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_top_block_timestamp() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

size_t BlockchainBDB::get_block_size(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

difficulty_type BlockchainBDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__ << "  height: " << height);
  check_open();
}

difficulty_type BlockchainBDB::get_block_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_block_already_generated_coins(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

crypto::hash BlockchainBDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

std::vector<block> BlockchainBDB::get_blocks_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

std::vector<crypto::hash> BlockchainBDB::get_hashes_range(const uint64_t& h1, const uint64_t& h2) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

crypto::hash BlockchainBDB::top_block_hash() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

block BlockchainBDB::get_top_block() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::height() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();

  return m_height;
}

bool BlockchainBDB::tx_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_tx_unlock_time(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

transaction BlockchainBDB::get_tx(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

std::vector<transaction> BlockchainBDB::get_tx_list(const std::vector<crypto::hash>& hlist) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_tx_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

//FIXME: make sure the random method used here is appropriate
uint64_t BlockchainBDB::get_random_output(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

uint64_t BlockchainBDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

crypto::public_key BlockchainBDB::get_output_key(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

tx_out BlockchainBDB::get_output(const crypto::hash& h, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
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
}

tx_out_index BlockchainBDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

std::vector<uint64_t> BlockchainBDB::get_tx_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

std::vector<uint64_t> BlockchainBDB::get_tx_amount_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}



bool BlockchainBDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

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
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
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
}

void BlockchainBDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainBDB::" << __func__);
  check_open();
}

}  // namespace cryptonote
