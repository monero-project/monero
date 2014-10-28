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

#include "cryptonote_core/blockchain_db.h"
#include "cryptonote_protocol/blobdatatype.h" // for type blobdata

#include <lmdb.h>

namespace cryptonote
{

struct txn_safe
{
  txn_safe() : m_txn(NULL) { }
  ~txn_safe()
  {
    if(m_txn != NULL)
    {
      mdb_txn_abort(m_txn);
    }
  }

  void commit(std::string message = "")
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

  operator MDB_txn*()
  {
    return m_txn;
  }

  operator MDB_txn**()
  {
    return &m_txn;
  }

  MDB_txn* m_txn;
};


class BlockchainLMDB : public BlockchainDB
{
public:
  BlockchainLMDB();
  ~BlockchainLMDB();

  virtual void open(const std::string& filename);

  virtual void create(const std::string& filename);

  virtual void close();

  virtual void sync();

  virtual void reset();

  virtual std::vector<std::string> get_filenames();

  virtual bool lock();

  virtual void unlock();

  virtual bool block_exists(const crypto::hash& h);

  virtual block get_block(const crypto::hash& h);

  virtual uint64_t get_block_height(const crypto::hash& h);

  virtual block_header get_block_header(const crypto::hash& h);

  virtual block get_block_from_height(const uint64_t& height);

  virtual uint64_t get_block_timestamp(const uint64_t& height) ;

  virtual uint64_t get_top_block_timestamp();

  virtual size_t get_block_size(const uint64_t& height);

  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height);

  virtual difficulty_type get_block_difficulty(const uint64_t& height);

  virtual uint64_t get_block_already_generated_coins(const uint64_t& height);

  virtual crypto::hash get_block_hash_from_height(const uint64_t& height);

  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2);

  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2);

  virtual crypto::hash top_block_hash();

  virtual block get_top_block();

  virtual uint64_t height();

  virtual bool tx_exists(const crypto::hash& h);

  virtual uint64_t get_tx_unlock_time(const crypto::hash& h);

  virtual transaction get_tx(const crypto::hash& h);

  virtual uint64_t get_tx_count();

  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist);

  virtual uint64_t get_tx_block_height(const crypto::hash& h);

  virtual uint64_t get_random_output(const uint64_t& amount);

  virtual uint64_t get_num_outputs(const uint64_t& amount);

  virtual crypto::public_key get_output_key(const uint64_t& amount, const uint64_t& index);

  virtual tx_out get_output(const crypto::hash& h, const uint64_t& index);

  /**
   * @brief get an output from its global index
   *
   * @param index global index of the output desired
   *
   * @return the output associated with the index.
   * Will throw OUTPUT_DNE if not output has that global index.
   * Will throw DB_ERROR if there is a non-specific LMDB error in fetching
   */
  tx_out get_output(const uint64_t& index);

  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index);

  virtual std::vector<uint64_t> get_tx_output_indices(const crypto::hash& h);

  virtual bool has_key_image(const crypto::key_image& img);

  virtual uint64_t add_block( const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            );

private:
  virtual void add_block( const block& blk
                , const size_t& block_size
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                );

  virtual void remove_block();

  virtual void add_transaction_data(const crypto::hash& blk_hash, const transaction& tx);

  virtual void remove_transaction_data(const crypto::hash& tx_hash);

  virtual void add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index);

  virtual void remove_output(const tx_out& tx_output);

  virtual void add_spent_key(const crypto::key_image& k_image);

  virtual void remove_spent_key(const crypto::key_image& k_image);

  /**
   * @brief convert a tx output to a blob for storage
   *
   * @param output the output to convert
   *
   * @return the resultant blob
   */
  blobdata output_to_blob(const tx_out& output);

  /**
   * @brief convert a tx output blob to a tx output
   *
   * @param blob the blob to convert
   *
   * @return the resultant tx output
   */
  tx_out output_from_blob(const blobdata& blob);

  /**
   * @brief get the global index of the index-th output of the given amount
   *
   * @param amount the output amount
   * @param index the index into the set of outputs of that amount
   *
   * @return the global index of the desired output
   */
  uint64_t get_output_global_index(const uint64_t& amount, const uint64_t& index);

  void check_open();

  MDB_env* m_env;

  MDB_dbi m_blocks;
  MDB_dbi m_block_heights;
  MDB_dbi m_block_hashes;
  MDB_dbi m_block_timestamps;
  MDB_dbi m_block_sizes;
  MDB_dbi m_block_diffs;
  MDB_dbi m_block_coins;

  MDB_dbi m_txs;
  MDB_dbi m_tx_unlocks;
  MDB_dbi m_tx_heights;
  MDB_dbi m_tx_outputs;

  MDB_dbi m_output_txs;
  MDB_dbi m_output_indices;
  MDB_dbi m_output_gindices;
  MDB_dbi m_output_amounts;
  MDB_dbi m_outputs;

  MDB_dbi m_spent_keys;

  bool m_open;
  uint64_t m_height;
  uint64_t m_num_outputs;
  std::string m_folder;
  txn_safe* m_write_txn;
};

}  // namespace cryptonote
