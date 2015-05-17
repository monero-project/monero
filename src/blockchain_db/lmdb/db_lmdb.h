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
#pragma once

#include <atomic>

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_protocol/blobdatatype.h" // for type blobdata

#include <lmdb.h>

namespace cryptonote
{

struct mdb_txn_safe
{
  mdb_txn_safe();
  ~mdb_txn_safe();

  void commit(std::string message = "");

  // This should only be needed for batch transaction which must be ensured to
  // be aborted before mdb_env_close, not after. So we can't rely on
  // BlockchainLMDB destructor to call mdb_txn_safe destructor, as that's too late
  // to properly abort, since mdb_env_close would have been called earlier.
  void abort();

  operator MDB_txn*()
  {
    return m_txn;
  }

  operator MDB_txn**()
  {
    return &m_txn;
  }

  uint64_t num_active_tx();

  static void prevent_new_txns();
  static void wait_no_active_txns();
  static void allow_new_txns();

  MDB_txn* m_txn;
  bool m_batch_txn = false;
  static std::atomic<uint64_t> num_active_txns;

  // could use a mutex here, but this should be sufficient.
  static std::atomic_flag creation_gate;
};


// If m_batch_active is set, a batch transaction exists beyond this class, such
// as a batch import with verification enabled, or possibly (later) a batch
// network sync.
//
// For some of the lookup methods, such as get_block_timestamp(), tx_exists(),
// and get_tx(), when m_batch_active is set, the lookup uses the batch
// transaction. This isn't only because the transaction is available, but it's
// necessary so that lookups include the database updates only present in the
// current batch write.
//
// A regular network sync without batch writes is expected to open a new read
// transaction, as those lookups are part of the validation done prior to the
// write for block and tx data, so no write transaction is open at the time.
class BlockchainLMDB : public BlockchainDB
{
public:
  BlockchainLMDB(bool batch_transactions=false);
  ~BlockchainLMDB();

  virtual void open(const std::string& filename, const int mdb_flags=0);

  virtual void close();

  virtual void sync();

  virtual void reset();

  virtual std::vector<std::string> get_filenames() const;

  virtual std::string get_db_name() const;

  virtual bool lock();

  virtual void unlock();

  virtual bool block_exists(const crypto::hash& h) const;

  virtual block get_block(const crypto::hash& h) const;

  virtual uint64_t get_block_height(const crypto::hash& h) const;

  virtual block_header get_block_header(const crypto::hash& h) const;

  virtual block get_block_from_height(const uint64_t& height) const;

  virtual uint64_t get_block_timestamp(const uint64_t& height) const;

  virtual uint64_t get_top_block_timestamp() const;

  virtual size_t get_block_size(const uint64_t& height) const;

  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const;

  virtual difficulty_type get_block_difficulty(const uint64_t& height) const;

  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const;

  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const;

  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual crypto::hash top_block_hash() const;

  virtual block get_top_block() const;

  virtual uint64_t height() const;

  virtual bool tx_exists(const crypto::hash& h) const;

  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const;

  virtual transaction get_tx(const crypto::hash& h) const;

  virtual uint64_t get_tx_count() const;

  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const;

  virtual uint64_t get_tx_block_height(const crypto::hash& h) const;

  virtual uint64_t get_random_output(const uint64_t& amount) const;

  virtual uint64_t get_num_outputs(const uint64_t& amount) const;

  virtual crypto::public_key get_output_key(const uint64_t& amount, const uint64_t& index) const;

  virtual tx_out get_output(const crypto::hash& h, const uint64_t& index) const;

  /**
   * @brief get an output from its global index
   *
   * @param index global index of the output desired
   *
   * @return the output associated with the index.
   * Will throw OUTPUT_DNE if not output has that global index.
   * Will throw DB_ERROR if there is a non-specific LMDB error in fetching
   */
  tx_out get_output(const uint64_t& index) const;

  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const;

  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const;

  virtual std::vector<uint64_t> get_tx_output_indices(const crypto::hash& h) const;
  virtual std::vector<uint64_t> get_tx_amount_output_indices(const crypto::hash& h) const;

  virtual bool has_key_image(const crypto::key_image& img) const;

  virtual uint64_t add_block( const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            );

  virtual void set_batch_transactions(bool batch_transactions);
  virtual void batch_start();
  virtual void batch_commit();
  virtual void batch_stop();
  virtual void batch_abort();

  virtual void pop_block(block& blk, std::vector<transaction>& txs);

private:
  void do_resize();

  bool need_resize();

  virtual void add_block( const block& blk
                , const size_t& block_size
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , const crypto::hash& block_hash
                );

  virtual void remove_block();

  virtual void add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash);

  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx);

  virtual void add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index);

  virtual void remove_output(const tx_out& tx_output);

  void remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx);

  void remove_output(const uint64_t& out_index, const uint64_t amount);
  void remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index);

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
  tx_out output_from_blob(const blobdata& blob) const;

  /**
   * @brief get the global index of the index-th output of the given amount
   *
   * @param amount the output amount
   * @param index the index into the set of outputs of that amount
   *
   * @return the global index of the desired output
   */
  uint64_t get_output_global_index(const uint64_t& amount, const uint64_t& index) const;

  void check_open() const;

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
  MDB_dbi m_output_keys;
  MDB_dbi m_outputs;

  MDB_dbi m_spent_keys;

  uint64_t m_height;
  uint64_t m_num_outputs;
  std::string m_folder;
  mdb_txn_safe* m_write_txn; // may point to either a short-lived txn or a batch txn
  mdb_txn_safe* m_write_batch_txn; // persist batch txn outside of BlockchainLMDB

  bool m_batch_transactions; // support for batch transactions
  bool m_batch_active; // whether batch transaction is in progress

  constexpr static uint64_t DEFAULT_MAPSIZE = 1 << 30;
  constexpr static float RESIZE_PERCENT = 0.8f;
  constexpr static float RESIZE_FACTOR = 1.5f;
};

}  // namespace cryptonote
