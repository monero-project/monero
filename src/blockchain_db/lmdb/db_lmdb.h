// Copyright (c) 2014-2024, The Monero Project
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
#include "cryptonote_basic/blobdatatype.h" // for type blobdata
#include "ringct/rctTypes.h"
#include <boost/thread/tss.hpp>

#include <lmdb.h>

#define ENABLE_AUTO_RESIZE

namespace cryptonote
{

typedef struct txindex {
    crypto::hash key;
    tx_data_t data;
} txindex;

typedef struct mdb_txn_cursors
{
  MDB_cursor *m_txc_blocks;
  MDB_cursor *m_txc_block_heights;
  MDB_cursor *m_txc_block_info;

  MDB_cursor *m_txc_output_txs;
  MDB_cursor *m_txc_output_amounts;

  MDB_cursor *m_txc_txs;
  MDB_cursor *m_txc_txs_pruned;
  MDB_cursor *m_txc_txs_prunable;
  MDB_cursor *m_txc_txs_prunable_hash;
  MDB_cursor *m_txc_txs_prunable_tip;
  MDB_cursor *m_txc_tx_indices;
  MDB_cursor *m_txc_tx_outputs;

  MDB_cursor *m_txc_spent_keys;

  MDB_cursor *m_txc_txpool_meta;
  MDB_cursor *m_txc_txpool_blob;

  MDB_cursor *m_txc_alt_blocks;

  MDB_cursor *m_txc_hf_versions;

  MDB_cursor *m_txc_properties;
} mdb_txn_cursors;

#define m_cur_blocks	m_cursors->m_txc_blocks
#define m_cur_block_heights	m_cursors->m_txc_block_heights
#define m_cur_block_info	m_cursors->m_txc_block_info
#define m_cur_output_txs	m_cursors->m_txc_output_txs
#define m_cur_output_amounts	m_cursors->m_txc_output_amounts
#define m_cur_txs	m_cursors->m_txc_txs
#define m_cur_txs_pruned	m_cursors->m_txc_txs_pruned
#define m_cur_txs_prunable	m_cursors->m_txc_txs_prunable
#define m_cur_txs_prunable_hash	m_cursors->m_txc_txs_prunable_hash
#define m_cur_txs_prunable_tip	m_cursors->m_txc_txs_prunable_tip
#define m_cur_tx_indices	m_cursors->m_txc_tx_indices
#define m_cur_tx_outputs	m_cursors->m_txc_tx_outputs
#define m_cur_spent_keys	m_cursors->m_txc_spent_keys
#define m_cur_txpool_meta	m_cursors->m_txc_txpool_meta
#define m_cur_txpool_blob	m_cursors->m_txc_txpool_blob
#define m_cur_alt_blocks	m_cursors->m_txc_alt_blocks
#define m_cur_hf_versions	m_cursors->m_txc_hf_versions
#define m_cur_properties	m_cursors->m_txc_properties

typedef struct mdb_rflags
{
  bool m_rf_txn;
  bool m_rf_blocks;
  bool m_rf_block_heights;
  bool m_rf_block_info;
  bool m_rf_output_txs;
  bool m_rf_output_amounts;
  bool m_rf_txs;
  bool m_rf_txs_pruned;
  bool m_rf_txs_prunable;
  bool m_rf_txs_prunable_hash;
  bool m_rf_txs_prunable_tip;
  bool m_rf_tx_indices;
  bool m_rf_tx_outputs;
  bool m_rf_spent_keys;
  bool m_rf_txpool_meta;
  bool m_rf_txpool_blob;
  bool m_rf_alt_blocks;
  bool m_rf_hf_versions;
  bool m_rf_properties;
} mdb_rflags;

typedef struct mdb_threadinfo
{
  MDB_txn *m_ti_rtxn;	// per-thread read txn
  mdb_txn_cursors m_ti_rcursors;	// per-thread read cursors
  mdb_rflags m_ti_rflags;	// per-thread read state

  ~mdb_threadinfo();
} mdb_threadinfo;

struct mdb_txn_safe
{
  mdb_txn_safe(const bool check=true);
  ~mdb_txn_safe();

  void commit(std::string message = "");

  // This should only be needed for batch transaction which must be ensured to
  // be aborted before mdb_env_close, not after. So we can't rely on
  // BlockchainLMDB destructor to call mdb_txn_safe destructor, as that's too late
  // to properly abort, since mdb_env_close would have been called earlier.
  void abort();
  void uncheck();

  operator MDB_txn*()
  {
    return m_txn;
  }

  operator MDB_txn**()
  {
    return &m_txn;
  }

  uint64_t num_active_tx() const;

  static void prevent_new_txns();
  static void wait_no_active_txns();
  static void allow_new_txns();
  static void increment_txns(int);

  mdb_threadinfo* m_tinfo;
  MDB_txn* m_txn;
  bool m_batch_txn = false;
  bool m_check;
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
class BlockchainLMDB final: public BlockchainDB
{
public:
  BlockchainLMDB(bool batch_transactions=true);
  ~BlockchainLMDB();

  void open(const std::string& filename, const int mdb_flags=0) override;

  void close() override;

  void sync() override;

  void safesyncmode(const bool onoff) override;

  void reset() override;

  std::vector<std::string> get_filenames() const override;

  bool remove_data_file(const std::string& folder) const override;

  std::string get_db_name() const override;

  bool block_exists(const crypto::hash& h, uint64_t *height = NULL) const override;

  uint64_t get_block_height(const crypto::hash& h) const override;

  block_header get_block_header(const crypto::hash& h) const override;

  cryptonote::blobdata get_block_blob(const crypto::hash& h) const override;

  cryptonote::blobdata get_block_blob_from_height(const uint64_t& height) const override;

  std::vector<uint64_t> get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const override;

  uint64_t get_block_timestamp(const uint64_t& height) const override;

  uint64_t get_top_block_timestamp() const override;

  size_t get_block_weight(const uint64_t& height) const override;

  std::vector<uint64_t> get_block_weights(uint64_t start_height, size_t count) const override;

  difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const override;

  difficulty_type get_block_difficulty(const uint64_t& height) const override;

  void correct_block_cumulative_difficulties(const uint64_t& start_height, const std::vector<difficulty_type>& new_cumulative_difficulties) override;

  uint64_t get_block_already_generated_coins(const uint64_t& height) const override;

  uint64_t get_block_long_term_weight(const uint64_t& height) const override;

  std::vector<uint64_t> get_long_term_block_weights(uint64_t start_height, size_t count) const override;

  crypto::hash get_block_hash_from_height(const uint64_t& height) const override;

  std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const override;

  std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const override;

  crypto::hash top_block_hash(uint64_t *block_height = NULL) const override;

  block get_top_block() const override;

  uint64_t height() const override;

  bool tx_exists(const crypto::hash& h) const override;
  bool tx_exists(const crypto::hash& h, uint64_t& tx_index) const override;

  uint64_t get_tx_unlock_time(const crypto::hash& h) const override;

  bool get_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const override;
  bool get_pruned_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const override;
  bool get_pruned_tx_blobs_from(const crypto::hash& h, size_t count, std::vector<cryptonote::blobdata> &bd) const override;
  bool get_blocks_from(uint64_t start_height, size_t min_block_count, size_t max_block_count, size_t max_tx_count, size_t max_size, std::vector<std::pair<std::pair<cryptonote::blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, cryptonote::blobdata>>>>& blocks, bool pruned, bool skip_coinbase, bool get_miner_tx_hash) const override;
  bool get_prunable_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const override;
  bool get_prunable_tx_hash(const crypto::hash& tx_hash, crypto::hash &prunable_hash) const override;

  std::vector<crypto::hash> get_txids_loose(const crypto::hash& h, std::uint32_t bits, uint64_t max_num_txs = 0) override;

  uint64_t get_tx_count() const override;

  std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const override;

  uint64_t get_tx_block_height(const crypto::hash& h) const override;

  uint64_t get_num_outputs(const uint64_t& amount) const override;

  output_data_t get_output_key(const uint64_t& amount, const uint64_t& index, bool include_commitmemt) const override;
  void get_output_key(const epee::span<const uint64_t> &amounts, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs, bool allow_partial = false) const override;

  tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const override;
  void get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
      std::vector<tx_out_index> &tx_out_indices) const;

  tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const override;
  void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices) const override;

  std::vector<std::vector<uint64_t>> get_tx_amount_output_indices(const uint64_t tx_id, size_t n_txes) const override;

  bool has_key_image(const crypto::key_image& img) const override;

  void add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata_ref &blob, const txpool_tx_meta_t& meta) override;
  void update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t& meta) override;
  uint64_t get_txpool_tx_count(relay_category category = relay_category::broadcasted) const override;
  bool txpool_has_tx(const crypto::hash &txid, relay_category tx_category) const override;
  void remove_txpool_tx(const crypto::hash& txid) override;
  bool get_txpool_tx_meta(const crypto::hash& txid, txpool_tx_meta_t &meta) const override;
  bool get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata& bd, relay_category tx_category) const override;
  cryptonote::blobdata get_txpool_tx_blob(const crypto::hash& txid, relay_category tx_category) const override;
  uint32_t get_blockchain_pruning_seed() const override;
  bool prune_blockchain(uint32_t pruning_seed = 0) override;
  bool update_pruning() override;
  bool check_pruning() override;

  void add_alt_block(const crypto::hash &blkid, const cryptonote::alt_block_data_t &data, const cryptonote::blobdata_ref &blob) override;
  bool get_alt_block(const crypto::hash &blkid, alt_block_data_t *data, cryptonote::blobdata *blob) override;
  void remove_alt_block(const crypto::hash &blkid) override;
  uint64_t get_alt_block_count() override;
  void drop_alt_blocks() override;

  bool for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata_ref*)> f, bool include_blob = false, relay_category category = relay_category::broadcasted) const override;

  bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const override;
  bool for_blocks_range(const uint64_t& h1, const uint64_t& h2, std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const override;
  bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>, bool pruned) const override;
  bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, uint64_t height, size_t tx_idx)> f) const override;
  bool for_all_outputs(uint64_t amount, const std::function<bool(uint64_t height)> &f) const override;
  bool for_all_alt_blocks(std::function<bool(const crypto::hash &blkid, const alt_block_data_t &data, const cryptonote::blobdata_ref *blob)> f, bool include_blob = false) const override;

  uint64_t add_block( const std::pair<block, blobdata>& blk
                            , size_t block_weight
                            , uint64_t long_term_block_weight
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<std::pair<transaction, blobdata>>& txs
                            ) override;

  void set_batch_transactions(bool batch_transactions) override;
  bool batch_start(uint64_t batch_num_blocks=0, uint64_t batch_bytes=0) override;
  void batch_commit();
  void batch_stop() override;
  void batch_abort() override;

  void block_wtxn_start() override;
  void block_wtxn_stop() override;
  void block_wtxn_abort() override;
  bool block_rtxn_start() const override;
  void block_rtxn_stop() const override;
  void block_rtxn_abort() const override;

  bool block_rtxn_start(MDB_txn **mtxn, mdb_txn_cursors **mcur) const;

  void pop_block(block& blk, std::vector<transaction>& txs) override;

  bool can_thread_bulk_indices() const { return true; }

  /**
   * @brief return a histogram of outputs on the blockchain
   *
   * @param amounts optional set of amounts to lookup
   * @param unlocked whether to restrict count to unlocked outputs
   * @param recent_cutoff timestamp to determine which outputs are recent
   * @param min_count return only amounts with at least that many instances
   *
   * @return a set of amount/instances
   */
  std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff, uint64_t min_count) const override;

  bool get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, std::vector<uint64_t> &distribution, uint64_t &base) const override;

  // helper functions
  static int compare_uint64(const MDB_val *a, const MDB_val *b);
  static int compare_hash32(const MDB_val *a, const MDB_val *b);
  static int compare_string(const MDB_val *a, const MDB_val *b);

private:
  void do_resize(uint64_t size_increase=0);

  bool need_resize(uint64_t threshold_size=0) const;
  void check_and_resize_for_batch(uint64_t batch_num_blocks, uint64_t batch_bytes);
  uint64_t get_estimated_batch_size(uint64_t batch_num_blocks, uint64_t batch_bytes) const;

  void add_block( const block& blk
                , size_t block_weight
                , uint64_t long_term_block_weight
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , uint64_t num_rct_outs
                , const crypto::hash& block_hash
                ) override;

  void remove_block() override;

  uint64_t add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, epee::span<const std::uint8_t> blob, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash) override;

  void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx) override;

  uint64_t add_output(const crypto::hash& tx_hash,
      const tx_out& tx_output,
      const uint64_t& local_index,
      const uint64_t unlock_time,
      const rct::key *commitment
      ) override;

  void add_tx_amount_output_indices(const uint64_t tx_id,
      const std::vector<uint64_t>& amount_output_indices
      ) override;

  void remove_tx_outputs(const uint64_t tx_id, const transaction& tx);

  void remove_output(const uint64_t amount, const uint64_t& out_index);

  void prune_outputs(uint64_t amount) override;

  void add_spent_key(const crypto::key_image& k_image) override;

  void remove_spent_key(const crypto::key_image& k_image) override;

  uint64_t num_outputs() const;

  // Hard fork
  void set_hard_fork_version(uint64_t height, uint8_t version) override;
  uint8_t get_hard_fork_version(uint64_t height) const override;
  void check_hard_fork_info() override;
  void drop_hard_fork_info() override;

  inline void check_open() const;

  bool prune_worker(int mode, uint32_t pruning_seed);

  bool is_read_only() const override;

  uint64_t get_database_size() const override;

  std::vector<uint64_t> get_block_info_64bit_fields(uint64_t start_height, size_t count, off_t offset) const;

  uint64_t get_max_block_size() override;
  void add_max_block_size(uint64_t sz) override;

  // fix up anything that may be wrong due to past bugs
  void fixup() override;

  // migrate from older DB version to current
  void migrate(const uint32_t oldversion);

  // migrate from DB version 0 to 1
  void migrate_0_1();

  // migrate from DB version 1 to 2
  void migrate_1_2();

  // migrate from DB version 2 to 3
  void migrate_2_3();

  // migrate from DB version 3 to 4
  void migrate_3_4();

  // migrate from DB version 4 to 5
  void migrate_4_5();

  void cleanup_batch();

private:
  MDB_env* m_env;

  MDB_dbi m_blocks;
  MDB_dbi m_block_heights;
  MDB_dbi m_block_info;

  MDB_dbi m_txs;
  MDB_dbi m_txs_pruned;
  MDB_dbi m_txs_prunable;
  MDB_dbi m_txs_prunable_hash;
  MDB_dbi m_txs_prunable_tip;
  MDB_dbi m_tx_indices;
  MDB_dbi m_tx_outputs;

  MDB_dbi m_output_txs;
  MDB_dbi m_output_amounts;

  MDB_dbi m_spent_keys;

  MDB_dbi m_txpool_meta;
  MDB_dbi m_txpool_blob;

  MDB_dbi m_alt_blocks;

  MDB_dbi m_hf_starting_heights;
  MDB_dbi m_hf_versions;

  MDB_dbi m_properties;

  mutable uint64_t m_cum_size;	// used in batch size estimation
  mutable unsigned int m_cum_count;
  std::string m_folder;
  mdb_txn_safe* m_write_txn; // may point to either a short-lived txn or a batch txn
  mdb_txn_safe* m_write_batch_txn; // persist batch txn outside of BlockchainLMDB
  boost::thread::id m_writer;

  bool m_batch_transactions; // support for batch transactions
  bool m_batch_active; // whether batch transaction is in progress

  mdb_txn_cursors m_wcursors;
  mutable boost::thread_specific_ptr<mdb_threadinfo> m_tinfo;

#if defined(__arm__)
  // force a value so it can compile with 32-bit ARM
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 31;
#else
#if defined(ENABLE_AUTO_RESIZE)
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 30;
#else
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 33;
#endif
#endif

  constexpr static float RESIZE_PERCENT = 0.9f;
};

}  // namespace cryptonote
