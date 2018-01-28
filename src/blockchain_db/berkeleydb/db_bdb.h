// Copyright (c) 2014-2018, The Monero Project
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

#include <db_cxx.h>

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_basic/blobdatatype.h" // for type blobdata

#include <unordered_map>
#include <condition_variable>

// ND: Enables multi-threaded bulk reads for when getting indices.
//     TODO: Disabled for now, as it doesn't seem to provide noticeable improvements (??. Reason: TBD.
// #define BDB_BULK_CAN_THREAD
namespace cryptonote
{

struct bdb_txn_safe
{
  bdb_txn_safe() : m_txn(NULL) { }
  ~bdb_txn_safe()
  {
    LOG_PRINT_L3("bdb_txn_safe: destructor");

    if (m_txn != NULL)
      abort();
  }

  void commit(std::string message = "")
  {
    if (message.size() == 0)
    {
      message = "Failed to commit a transaction to the db";
    }

    if (m_txn->commit(0))
    {
      m_txn = NULL;
      LOG_PRINT_L0(message);
      throw DB_ERROR(message.c_str());
    }
    m_txn = NULL;
  }

  void abort()
  {
    LOG_PRINT_L3("bdb_txn_safe: abort()");
    if(m_txn != NULL)
    {
      m_txn->abort();
      m_txn = NULL;
    }
    else
    {
      LOG_PRINT_L0("WARNING: bdb_txn_safe: abort() called, but m_txn is NULL");
    }
  }

  operator DbTxn*()
  {
    return m_txn;
  }

  operator DbTxn**()
  {
    return &m_txn;
  }
private:
  DbTxn* m_txn;
};

// ND: Class to handle buffer management when doing bulk queries
// (DB_MULTIPLE). Allocates buffers then handles thread queuing
// so a fixed set of buffers can be used (instead of allocating
// every time a bulk query is needed).
template <typename T>
class bdb_safe_buffer
{
  // limit the number of buffers to 8
  const size_t MaxAllowedBuffers = 8;
public:
    bdb_safe_buffer(size_t num_buffers, size_t count)
    {
      if(num_buffers > MaxAllowedBuffers)
        num_buffers = MaxAllowedBuffers;

      set_count(num_buffers);
      for (size_t i = 0; i < num_buffers; i++)
        m_buffers.push_back((T) malloc(sizeof(T) * count));
      m_buffer_count = count;
    }

    ~bdb_safe_buffer()
    {
        for (size_t i = 0; i < m_buffers.size(); i++)
        {
            if (m_buffers[i])
            {
                free(m_buffers[i]);
                m_buffers[i] = nullptr;
            }
        }

        m_buffers.resize(0);
    }

    T acquire_buffer()
    {
        boost::unique_lock<boost::mutex> lock(m_lock);
        m_cv.wait(lock, [&]{ return m_count > 0; });

        --m_count;
        size_t index = -1;
        for (size_t i = 0; i < m_open_slot.size(); i++)
        {
            if (m_open_slot[i])
            {
                m_open_slot[i] = false;
                index = i;
                break;
            }
        }

        assert(index >= 0);

        T buffer = m_buffers[index];
        m_buffer_map.emplace(buffer, index);
        return buffer;
    }

    void release_buffer(T buffer)
    {
        boost::unique_lock<boost::mutex> lock(m_lock);

        assert(buffer != nullptr);
        auto it = m_buffer_map.find(buffer);
        if (it != m_buffer_map.end())
        {
            auto index = it->second;

            assert(index < m_open_slot.size());
            assert(m_open_slot[index] == false);
            assert(m_count < m_open_slot.size());

            ++m_count;
            m_open_slot[index] = true;
            m_buffer_map.erase(it);
            m_cv.notify_one();
        }
    }

    size_t get_buffer_size() const
    {
        return m_buffer_count * sizeof(T);
    }

    size_t get_buffer_count() const
    {
        return m_buffer_count;
    }

    typedef T type;

private:
    void set_count(size_t count)
    {
        assert(count > 0);
        m_open_slot.resize(count, true);
        m_count = count;
    }

    std::vector<T> m_buffers;
    std::unordered_map<T, size_t> m_buffer_map;

    boost::condition_variable m_cv;
    std::vector<bool> m_open_slot;
    size_t m_count;
    boost::mutex m_lock;

    size_t m_buffer_count;
};

template <typename T>
class bdb_safe_buffer_autolock
{
public:
    bdb_safe_buffer_autolock(T &safe_buffer, typename T::type &buffer) :
        m_safe_buffer(safe_buffer), m_buffer(nullptr)
    {
        m_buffer = m_safe_buffer.acquire_buffer();
        buffer = m_buffer;
    }

    ~bdb_safe_buffer_autolock()
    {
        if (m_buffer != nullptr)
        {
            m_safe_buffer.release_buffer(m_buffer);
            m_buffer = nullptr;
        }
    }
private:
    T &m_safe_buffer;
    typename T::type m_buffer;
};

class BlockchainBDB : public BlockchainDB
{
public:
  BlockchainBDB(bool batch_transactions=false);
  ~BlockchainBDB();

  virtual void open(const std::string& filename, const int db_flags);

  virtual void close();

  virtual void sync();

  virtual void reset();

  virtual std::vector<std::string> get_filenames() const;

  virtual std::string get_db_name() const;

  virtual bool lock();

  virtual void unlock();

  virtual bool block_exists(const crypto::hash& h, uint64_t *height = NULL) const;

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

  virtual uint64_t get_num_outputs(const uint64_t& amount) const;

  virtual uint64_t get_indexing_base() const { return 1; }

  virtual output_data_t get_output_key(const uint64_t& amount, const uint64_t& index);
  virtual output_data_t get_output_key(const uint64_t& global_index) const;
  virtual void get_output_key(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs);

  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const;
  virtual void get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
          std::vector<tx_out_index> &tx_out_indices) const;

  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index);
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices);

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
  virtual bool batch_start(uint64_t batch_num_blocks=0);
  virtual void batch_commit();
  virtual void batch_stop();
  virtual void batch_abort();

  virtual void block_txn_start(bool readonly);
  virtual void block_txn_stop();
  virtual void block_txn_abort();

  virtual void pop_block(block& blk, std::vector<transaction>& txs);

#if defined(BDB_BULK_CAN_THREAD)
  virtual bool can_thread_bulk_indices() const { return true; }
#else
  virtual bool can_thread_bulk_indices() const { return false; }
#endif

  /**
   * @brief return a histogram of outputs on the blockchain
   *
   * @param amounts optional set of amounts to lookup
   *
   * @return a set of amount/instances
   */
  std::map<uint64_t, uint64_t> get_output_histogram(const std::vector<uint64_t> &amounts) const;

private:
  virtual void add_block( const block& blk
                , const size_t& block_size
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , const crypto::hash& block_hash
                );

  virtual void remove_block();

  virtual void add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash);

  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx);

  virtual void add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time, const rct::key *commitment);

  virtual void remove_output(const tx_out& tx_output);

  void remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx);

  void remove_output(const uint64_t& out_index, const uint64_t amount);
  void remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index);

  virtual void add_spent_key(const crypto::key_image& k_image);

  virtual void remove_spent_key(const crypto::key_image& k_image);

  void get_output_global_indices(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<uint64_t> &global_indices);

  virtual bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const;
  virtual bool for_all_blocks(std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const;
  virtual bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>) const;
  virtual bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, size_t tx_idx)> f) const;

  // Hard fork related storage
  virtual void set_hard_fork_version(uint64_t height, uint8_t version);
  virtual uint8_t get_hard_fork_version(uint64_t height) const;
  virtual void check_hard_fork_info();
  virtual void drop_hard_fork_info();

  /**
   * @brief convert a tx output to a blob for storage
   *
   * @param output the output to convert
   *
   * @return the resultant blob
   */
  blobdata output_to_blob(const tx_out& output) const;

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
  uint64_t get_output_global_index(const uint64_t& amount, const uint64_t& index);
  void checkpoint_worker() const;
  void check_open() const;

  virtual bool is_read_only() const;

  //
  // fix up anything that may be wrong due to past bugs
  virtual void fixup();

  bool m_run_checkpoint;
  std::unique_ptr<boost::thread> m_checkpoint_thread;
  typedef bdb_safe_buffer<void *> bdb_safe_buffer_t;
  bdb_safe_buffer_t m_buffer;

  DbEnv* m_env;

  Db* m_blocks;
  Db* m_block_heights;
  Db* m_block_hashes;
  Db* m_block_timestamps;
  Db* m_block_sizes;
  Db* m_block_diffs;
  Db* m_block_coins;

  Db* m_txs;
  Db* m_tx_unlocks;
  Db* m_tx_heights;
  Db* m_tx_outputs;

  Db* m_output_txs;
  Db* m_output_indices;
  Db* m_output_amounts;
  Db* m_output_keys;

  Db* m_spent_keys;

  Db* m_hf_starting_heights;
  Db* m_hf_versions;

  Db* m_properties;

  uint64_t m_height;
  uint64_t m_num_outputs;
  std::string m_folder;
  bdb_txn_safe *m_write_txn;

  bool m_batch_transactions; // support for batch transactions
};

}  // namespace cryptonote
