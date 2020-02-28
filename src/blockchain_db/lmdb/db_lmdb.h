// Copyright (c) 2014-2019, The Monero Project
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
#include <boost/atomic/atomic.hpp>

#include <lmdb.h>

#define ENABLE_AUTO_RESIZE

namespace cryptonote
{

typedef struct txindex {
    crypto::hash key;
    tx_data_t data;
} txindex;

enum class source : uint8_t
{
  blocks,
  block_heights,
  block_info,

  output_txs,
  output_amounts,

  txs,
  txs_pruned,
  txs_prunable,
  txs_prunable_hash,
  txs_prunable_tip,
  tx_indices,
  tx_outputs,

  spent_keys,

  txpool_meta,
  txpool_blob,

  alt_blocks,

  hf_versions,

  properties
};

class mdb_txn_cursors
{
  public:
    mdb_txn_cursors() noexcept :
      m_cursors{{nullptr}}
    {}

    ~mdb_txn_cursors() noexcept
    {
      for (auto cursor: m_cursors) {
        if (cursor != nullptr) {
          mdb_cursor_close(cursor);
        }
      }
    }

    MDB_cursor*& get(source cursor)
    {
      return m_cursors.at(static_cast<uint8_t>(cursor));
    }
  private:
    std::array<MDB_cursor*, 18> m_cursors;
};

class mdb_databases
{
    public:
      mdb_databases() :
        m_databases{{}}
      {}

      const static std::string& name(source database);

      const MDB_dbi& get(source database) const
      {
        return m_databases.at(static_cast<uint8_t>(database));
      }

      MDB_dbi& get(source database)
      {
        return m_databases.at(static_cast<uint8_t>(database));
      }

      MDB_stat stat(MDB_txn* txn, source database) const;
      void drop(MDB_txn* txn, source database, bool close = false);
    private:
      std::array<MDB_dbi, 18> m_databases;
};



struct mdb_threadinfo
{
  mdb_threadinfo() :
    m_ti_rtxn{nullptr},
    m_rf_txn{false},
    m_handles{0}
  {}

  ~mdb_threadinfo();

  MDB_txn* transaction(MDB_env* env, bool *started = nullptr);
  void reset();

  MDB_txn* acquire(MDB_env* env);
  void release();

  MDB_txn *m_ti_rtxn;	// per-thread read txn
  mdb_txn_cursors m_ti_rcursors;	// per-thread read cursors
  bool m_rf_txn;
  std::bitset<18> m_valid_cursor;
  size_t m_handles;
};

struct mdb_txn_safe
{
  static void prevent_new_txns();
  static void wait_no_active_txns();
  static void allow_new_txns();

  static std::atomic<uint64_t> num_active_txns;

  // could use a mutex here, but this should be sufficient.
  static std::atomic_flag creation_gate;
};

class mdb_writer_data
{
  public:
    mdb_writer_data(MDB_env* env, mdb_databases& databases) :
      m_env{env},
      m_txn{nullptr},
      m_databases{databases},
      m_writer_thread{},
      m_writer_depth{0},
      m_error{false}
    {}

    ~mdb_writer_data();

    boost::thread::id current_thread() const noexcept;

    mdb_databases& databases() noexcept;
    mdb_txn_cursors& cursors();

    MDB_cursor* cursor(source database);

    MDB_txn* try_acquire() noexcept;
    MDB_txn* acquire();

    void release(bool error = false);
  private:
    MDB_env* m_env;
    MDB_txn* m_txn;
    mdb_databases& m_databases;
    boost::optional<mdb_txn_cursors> m_cursors;
    boost::atomic<boost::thread::id> m_writer_thread;
    size_t m_writer_depth;
    bool m_error;
};

class mdb_write_handle
{
  public:
    mdb_write_handle(mdb_writer_data& data) :
      m_data{&data},
      m_txn{data.acquire()}
    {}

    ~mdb_write_handle();

    mdb_write_handle& operator=(const mdb_write_handle&) = delete;
    mdb_write_handle& operator=(mdb_write_handle&& that) noexcept;

    operator MDB_txn*() const noexcept
    {
      return m_txn;
    }

    void commit();
    void abort();

    MDB_cursor* cursor(source database);
  private:
    mdb_writer_data* m_data;
    MDB_txn* m_txn;
};


class mdb_read_handle
{
  public:
    mdb_read_handle() :
      m_databases{nullptr},
      m_reader{nullptr},
      m_writer{nullptr},
      m_txn{nullptr}
    {}

    mdb_read_handle(MDB_env* env, mdb_databases& databases, mdb_writer_data& writer, boost::thread_specific_ptr<mdb_threadinfo>& tinfo);
    ~mdb_read_handle();

    mdb_read_handle& operator=(const mdb_read_handle&) = delete;
    mdb_read_handle& operator=(mdb_read_handle&& that) noexcept;

    operator MDB_txn*() const noexcept
    {
      return m_txn;
    }

    void abort();

    MDB_cursor* cursor(source database);
  private:
    mdb_databases* m_databases;
    mdb_threadinfo* m_reader;
    mdb_writer_data* m_writer;
    MDB_txn* m_txn;
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
  BlockchainLMDB(bool batch_transactions=true);
  ~BlockchainLMDB();

  virtual void open(const std::string& filename, const int mdb_flags=0);

  virtual void close();

  virtual void sync();

  virtual void safesyncmode(const bool onoff);

  virtual void reset();

  virtual std::vector<std::string> get_filenames() const;

  virtual bool remove_data_file(const std::string& folder) const;

  virtual std::string get_db_name() const;

  virtual bool lock();

  virtual void unlock();

  virtual bool block_exists(const crypto::hash& h, uint64_t *height = NULL) const;

  virtual uint64_t get_block_height(const crypto::hash& h) const;

  virtual block_header get_block_header(const crypto::hash& h) const;

  virtual cryptonote::blobdata get_block_blob(const crypto::hash& h) const;

  virtual cryptonote::blobdata get_block_blob_from_height(const uint64_t& height) const;

  virtual std::vector<uint64_t> get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const;

  virtual uint64_t get_block_timestamp(const uint64_t& height) const;

  virtual uint64_t get_top_block_timestamp() const;

  virtual size_t get_block_weight(const uint64_t& height) const;

  virtual std::vector<uint64_t> get_block_weights(uint64_t start_height, size_t count) const;

  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const;

  virtual difficulty_type get_block_difficulty(const uint64_t& height) const;

  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const;

  virtual uint64_t get_block_long_term_weight(const uint64_t& height) const;

  virtual std::vector<uint64_t> get_long_term_block_weights(uint64_t start_height, size_t count) const;

  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const;

  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual crypto::hash top_block_hash(uint64_t *block_height = NULL) const;

  virtual block get_top_block() const;

  virtual uint64_t height() const;

  virtual bool tx_exists(const crypto::hash& h) const;
  virtual bool tx_exists(const crypto::hash& h, uint64_t& tx_index) const;

  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const;

  virtual bool get_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const;
  virtual bool get_pruned_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const;
  virtual bool get_pruned_tx_blobs_from(const crypto::hash& h, size_t count, std::vector<cryptonote::blobdata> &bd) const;
  virtual bool get_prunable_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const;
  virtual bool get_prunable_tx_hash(const crypto::hash& tx_hash, crypto::hash &prunable_hash) const;

  virtual uint64_t get_tx_count() const;

  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const;

  virtual uint64_t get_tx_block_height(const crypto::hash& h) const;

  virtual uint64_t get_num_outputs(const uint64_t& amount) const;

  virtual output_data_t get_output_key(const uint64_t& amount, const uint64_t& index, bool include_commitmemt) const;
  virtual void get_output_key(const epee::span<const uint64_t> &amounts, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs, bool allow_partial = false) const;

  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const;
  virtual void get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
      std::vector<tx_out_index> &tx_out_indices) const;

  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const;
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices) const;

  virtual std::vector<std::vector<uint64_t>> get_tx_amount_output_indices(const uint64_t tx_id, size_t n_txes) const;

  virtual bool has_key_image(const crypto::key_image& img) const;

  virtual void add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata &blob, const txpool_tx_meta_t& meta);
  virtual void update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t& meta);
  virtual uint64_t get_txpool_tx_count(relay_category category = relay_category::broadcasted) const;
  virtual bool txpool_has_tx(const crypto::hash &txid, relay_category tx_category) const;
  virtual void remove_txpool_tx(const crypto::hash& txid);
  virtual bool get_txpool_tx_meta(const crypto::hash& txid, txpool_tx_meta_t &meta) const;
  virtual bool get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata& bd, relay_category tx_category) const;
  virtual cryptonote::blobdata get_txpool_tx_blob(const crypto::hash& txid, relay_category tx_category) const;
  virtual uint32_t get_blockchain_pruning_seed() const;
  virtual bool prune_blockchain(uint32_t pruning_seed = 0);
  virtual bool update_pruning();
  virtual bool check_pruning();

  virtual void add_alt_block(const crypto::hash &blkid, const cryptonote::alt_block_data_t &data, const cryptonote::blobdata &blob);
  virtual bool get_alt_block(const crypto::hash &blkid, alt_block_data_t *data, cryptonote::blobdata *blob);
  virtual void remove_alt_block(const crypto::hash &blkid);
  virtual uint64_t get_alt_block_count();
  virtual void drop_alt_blocks();

  virtual bool for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata*)> f, bool include_blob = false, relay_category category = relay_category::broadcasted) const;

  virtual bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const;
  virtual bool for_blocks_range(const uint64_t& h1, const uint64_t& h2, std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const;
  virtual bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>, bool pruned) const;
  virtual bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, uint64_t height, size_t tx_idx)> f) const;
  virtual bool for_all_outputs(uint64_t amount, const std::function<bool(uint64_t height)> &f) const;
  virtual bool for_all_alt_blocks(std::function<bool(const crypto::hash &blkid, const alt_block_data_t &data, const cryptonote::blobdata *blob)> f, bool include_blob = false) const;

  virtual uint64_t add_block( const std::pair<block, blobdata>& blk
                            , size_t block_weight
                            , uint64_t long_term_block_weight
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<std::pair<transaction, blobdata>>& txs
                            );

  virtual void set_batch_transactions(bool batch_transactions);
  virtual bool batch_start(uint64_t batch_num_blocks=0, uint64_t batch_bytes=0);
  virtual void batch_commit();
  virtual void batch_stop();
  virtual void batch_abort();

  virtual void block_wtxn_start();
  virtual void block_wtxn_stop();
  virtual void block_wtxn_abort();
  virtual bool block_rtxn_start() const;
  virtual void block_rtxn_stop() const;
  virtual void block_rtxn_abort() const;

  bool block_rtxn_start(MDB_txn **mtxn, mdb_txn_cursors **mcur) const;

  virtual void pop_block(block& blk, std::vector<transaction>& txs);

  virtual bool can_thread_bulk_indices() const { return true; }

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
  std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff, uint64_t min_count) const;

  bool get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, std::vector<uint64_t> &distribution, uint64_t &base) const;

  // helper functions
  static int compare_uint64(const MDB_val *a, const MDB_val *b);
  static int compare_hash32(const MDB_val *a, const MDB_val *b);
  static int compare_string(const MDB_val *a, const MDB_val *b);

private:
  void db_open(MDB_txn* txn, int flags, source database);
  void do_resize(uint64_t size_increase=0);

  bool need_resize(uint64_t threshold_size=0) const;
  void check_and_resize_for_batch(uint64_t batch_num_blocks, uint64_t batch_bytes);
  uint64_t get_estimated_batch_size(uint64_t batch_num_blocks, uint64_t batch_bytes) const;

  virtual void add_block( const block& blk
                , size_t block_weight
                , uint64_t long_term_block_weight
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , uint64_t num_rct_outs
                , const crypto::hash& block_hash
                );

  virtual void remove_block();

  virtual uint64_t add_transaction_data(const crypto::hash& blk_hash, const std::pair<transaction, blobdata>& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash);

  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx);

  virtual uint64_t add_output(const crypto::hash& tx_hash,
      const tx_out& tx_output,
      const uint64_t& local_index,
      const uint64_t unlock_time,
      const rct::key *commitment
      );

  virtual void add_tx_amount_output_indices(const uint64_t tx_id,
      const std::vector<uint64_t>& amount_output_indices
      );

  void remove_tx_outputs(const uint64_t tx_id, const transaction& tx);

  void remove_output(const uint64_t amount, const uint64_t& out_index);

  virtual void prune_outputs(uint64_t amount);

  virtual void add_spent_key(const crypto::key_image& k_image);

  virtual void remove_spent_key(const crypto::key_image& k_image);

  uint64_t num_outputs() const;

  // Hard fork
  virtual void set_hard_fork_version(uint64_t height, uint8_t version);
  virtual uint8_t get_hard_fork_version(uint64_t height) const;
  virtual void check_hard_fork_info();
  virtual void drop_hard_fork_info();

  inline void check_open() const;

  bool prune_worker(int mode, uint32_t pruning_seed);

  virtual bool is_read_only() const;

  virtual uint64_t get_database_size() const;

  std::vector<uint64_t> get_block_info_64bit_fields(uint64_t start_height, size_t count, off_t offset) const;

  uint64_t get_max_block_size();
  void add_max_block_size(uint64_t sz);

  // fix up anything that may be wrong due to past bugs
  virtual void fixup();

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

  mutable mdb_databases m_databases;

  MDB_dbi m_hf_starting_heights;

  mutable uint64_t m_cum_size;	// used in batch size estimation
  mutable unsigned int m_cum_count;
  std::string m_folder;

  mutable boost::optional<mdb_writer_data> m_write_data;
  boost::optional<mdb_write_handle> m_batch_handle;

  bool m_batch_transactions; // support for batch transactions

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
