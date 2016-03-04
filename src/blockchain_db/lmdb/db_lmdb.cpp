// Copyright (c) 2014-2016, The Monero Project
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
#include <boost/format.hpp>
#include <boost/current_function.hpp>
#include <memory>  // std::unique_ptr
#include <cstring>  // memcpy
#include <random>

#include "cryptonote_core/cryptonote_format_utils.h"
#include "crypto/crypto.h"
#include "profile_tools.h"

#if defined(__i386) || defined(__x86_64)
#define MISALIGNED_OK	1
#endif

using epee::string_tools::pod_to_hex;

// Increase when the DB changes in a non backward compatible way, and there
// is no automatic conversion, so that a full resync is needed.
#define VERSION 0

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

  ~lmdb_cur()
  {
    close();
  }

  operator MDB_cursor*()
  {
    return m_cur;
  }
  operator MDB_cursor**()
  {
    return &m_cur;
  }

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
  MDB_val_copy(const T &t) :
    t_copy(t)
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
  MDB_val_copy(const cryptonote::blobdata &bd) :
    data(new char[bd.size()])
  {
    memcpy(data.get(), bd.data(), bd.size());
    mv_size = bd.size();
    mv_data = data.get();
  }
private:
  std::unique_ptr<char[]> data;
};

template<>
struct MDB_val_copy<const char*>: public MDB_val
{
  MDB_val_copy(const char *s):
    size(strlen(s)+1), // include the NUL, makes it easier for compares
    data(new char[size])
  {
    mv_size = size;
    mv_data = data.get();
    memcpy(mv_data, s, size);
  }
private:
  size_t size;
  std::unique_ptr<char[]> data;
};

int compare_uint64(const MDB_val *a, const MDB_val *b)
{
  const uint64_t va = *(const uint64_t*)a->mv_data;
  const uint64_t vb = *(const uint64_t*)b->mv_data;
  if (va < vb) return -1;
  else if (va == vb) return 0;
  else return 1;
};

int compare_uint8(const MDB_val *a, const MDB_val *b)
{
  const uint8_t va = *(const uint8_t*)a->mv_data;
  const uint8_t vb = *(const uint8_t*)b->mv_data;
  return va - vb;
};

int compare_hash32(const MDB_val *a, const MDB_val *b)
{
  uint32_t *va = (uint32_t*) a->mv_data;
  uint32_t *vb = (uint32_t*) b->mv_data;
  for (int n = 7; n >= 0; n--)
  {
    if (va[n] == vb[n])
      continue;
    return va[n] < vb[n] ? -1 : 1;
  }

  return 0;
}

int compare_string(const MDB_val *a, const MDB_val *b)
{
  const char *va = (const char*) a->mv_data;
  const char *vb = (const char*) b->mv_data;
  return strcmp(va, vb);
}

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
const char* const LMDB_SPENT_KEYS = "spent_keys";

const char* const LMDB_HF_STARTING_HEIGHTS = "hf_starting_heights";
const char* const LMDB_HF_VERSIONS = "hf_versions";

const char* const LMDB_PROPERTIES = "properties";


const std::string lmdb_error(const std::string& error_string, int mdb_res)
{
  const std::string full_string = error_string + mdb_strerror(mdb_res);
  return full_string;
}

inline void lmdb_db_open(MDB_txn* txn, const char* name, int flags, MDB_dbi& dbi, const std::string& error_string)
{
  if (auto res = mdb_dbi_open(txn, name, flags, &dbi))
    throw0(cryptonote::DB_OPEN_FAILURE(lmdb_error(error_string + " : ", res).c_str()));
}


}  // anonymous namespace

#define CURSOR(name) \
	if (!m_cur_ ## name) { \
	  int result = mdb_cursor_open(*m_write_txn, m_ ## name, &m_cur_ ## name); \
	  if (result) \
        throw0(DB_ERROR(std::string("Failed to open cursor: ").append(mdb_strerror(result)).c_str())); \
	}

#define RCURSOR(name) \
	if (!m_cur_ ## name) { \
	  int result = mdb_cursor_open(m_txn, m_ ## name, (MDB_cursor **)&m_cur_ ## name); \
	  if (result) \
        throw0(DB_ERROR(std::string("Failed to open cursor: ").append(mdb_strerror(result)).c_str())); \
	  if (!m_write_txn) \
	    m_tinfo->m_ti_rflags.m_rf_ ## name = true; \
	} else if (!m_write_txn && !m_tinfo->m_ti_rflags.m_rf_ ## name) { \
	  mdb_cursor_renew(m_txn, m_cur_ ## name); \
	  m_tinfo->m_ti_rflags.m_rf_ ## name = true; \
	}

namespace cryptonote
{
std::atomic<uint64_t> mdb_txn_safe::num_active_txns{0};
std::atomic_flag mdb_txn_safe::creation_gate = ATOMIC_FLAG_INIT;

mdb_threadinfo::~mdb_threadinfo()
{
  MDB_cursor **cur = &m_ti_rcursors.m_txc_blocks;
  unsigned i;
  for (i=0; i<sizeof(mdb_txn_cursors)/sizeof(MDB_cursor *); i++)
    if (cur[i])
      mdb_cursor_close(cur[i]);
  if (m_ti_rtxn)
    mdb_txn_abort(m_ti_rtxn);
}

mdb_txn_safe::mdb_txn_safe() : m_txn(NULL)
{
  while (creation_gate.test_and_set());
  num_active_txns++;
  creation_gate.clear();
}

mdb_txn_safe::~mdb_txn_safe()
{
  LOG_PRINT_L3("mdb_txn_safe: destructor");
  if (m_txn != nullptr)
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

  if (auto result = mdb_txn_commit(m_txn))
  {
    m_txn = nullptr;
    throw0(DB_ERROR((message + ": ").append(mdb_strerror(result)).c_str()));
  }
  m_txn = nullptr;
}

void mdb_txn_safe::abort()
{
  LOG_PRINT_L3("mdb_txn_safe: abort()");
  if(m_txn != nullptr)
  {
    mdb_txn_abort(m_txn);
    m_txn = nullptr;
  }
  else
  {
    LOG_PRINT_L0("WARNING: mdb_txn_safe: abort() called, but m_txn is NULL");
  }
}

uint64_t mdb_txn_safe::num_active_tx() const
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



void BlockchainLMDB::do_resize(uint64_t increase_size)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  CRITICAL_REGION_LOCAL(m_synchronization_lock);
  const uint64_t add_size = 1LL << 30;

  // check disk capacity
  try
  {
    boost::filesystem::path path(m_folder);
    boost::filesystem::space_info si = boost::filesystem::space(path);
    if(si.available < add_size)
    {
      LOG_PRINT_RED_L0("!! WARNING: Insufficient free space to extend database !!: " << si.available / 1LL << 20L);
      return;
    }
  }
  catch(...)
  {
    // print something but proceed.
    LOG_PRINT_YELLOW("Unable to query free disk space.", LOG_LEVEL_0);
  }

  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  // add 1Gb per resize, instead of doing a percentage increase
  uint64_t new_mapsize = (double) mei.me_mapsize + add_size;

  // If given, use increase_size intead of above way of resizing.
  // This is currently used for increasing by an estimated size at start of new
  // batch txn.
  if (increase_size > 0)
    new_mapsize = mei.me_mapsize + increase_size;

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

  LOG_PRINT_GREEN("LMDB Mapsize increased." << "  Old: " << mei.me_mapsize / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB", LOG_LEVEL_0);

  mdb_txn_safe::allow_new_txns();
}

// threshold_size is used for batch transactions
bool BlockchainLMDB::need_resize(uint64_t threshold_size) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
#if defined(ENABLE_AUTO_RESIZE)
  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  // size_used doesn't include data yet to be committed, which can be
  // significant size during batch transactions. For that, we estimate the size
  // needed at the beginning of the batch transaction and pass in the
  // additional size needed.
  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;

  LOG_PRINT_L1("DB map size:     " << mei.me_mapsize);
  LOG_PRINT_L1("Space used:      " << size_used);
  LOG_PRINT_L1("Space remaining: " << mei.me_mapsize - size_used);
  LOG_PRINT_L1("Size threshold:  " << threshold_size);
  float resize_percent_old = RESIZE_PERCENT;
  LOG_PRINT_L1(boost::format("Percent used: %.04f  Percent threshold: %.04f") % ((double)size_used/mei.me_mapsize) % resize_percent_old);

  if (threshold_size > 0)
  {
    if (mei.me_mapsize - size_used < threshold_size)
    {
      LOG_PRINT_L1("Threshold met (size-based)");
      return true;
    }
    else
      return false;
  }

  std::mt19937 engine(std::random_device{}());
  std::uniform_real_distribution<double> fdis(0.6, 0.9);
  double resize_percent = fdis(engine);

  if ((double)size_used / mei.me_mapsize  > resize_percent)
  {
    LOG_PRINT_L1("Threshold met (percent-based)");
    return true;
  }
  return false;
#else
  return false;
#endif
}

void BlockchainLMDB::check_and_resize_for_batch(uint64_t batch_num_blocks)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  LOG_PRINT_L1("[" << __func__ << "] " << "checking DB size");
  const uint64_t min_increase_size = 512 * (1 << 20);
  uint64_t threshold_size = 0;
  uint64_t increase_size = 0;
  if (batch_num_blocks > 0)
  {
    threshold_size = get_estimated_batch_size(batch_num_blocks);
    LOG_PRINT_L1("calculated batch size: " << threshold_size);

    // The increased DB size could be a multiple of threshold_size, a fixed
    // size increase (> threshold_size), or other variations.
    //
    // Currently we use the greater of threshold size and a minimum size. The
    // minimum size increase is used to avoid frequent resizes when the batch
    // size is set to a very small numbers of blocks.
    increase_size = (threshold_size > min_increase_size) ? threshold_size : min_increase_size;
    LOG_PRINT_L1("increase size: " << increase_size);
  }

  // if threshold_size is 0 (i.e. number of blocks for batch not passed in), it
  // will fall back to the percent-based threshold check instead of the
  // size-based check
  if (need_resize(threshold_size))
  {
    LOG_PRINT_L0("[batch] DB resize needed");
    do_resize(increase_size);
  }
}

uint64_t BlockchainLMDB::get_estimated_batch_size(uint64_t batch_num_blocks) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t threshold_size = 0;

  // batch size estimate * batch safety factor = final size estimate
  // Takes into account "reasonable" block size increases in batch.
  float batch_safety_factor = 1.7f;
  float batch_fudge_factor = batch_safety_factor * batch_num_blocks;
  // estimate of stored block expanded from raw block, including denormalization and db overhead.
  // Note that this probably doesn't grow linearly with block size.
  float db_expand_factor = 4.5f;
  uint64_t num_prev_blocks = 500;
  // For resizing purposes, allow for at least 4k average block size.
  uint64_t min_block_size = 4 * 1024;

  uint64_t block_stop = 0;
  if (m_height > 1)
    block_stop = m_height - 1;
  uint64_t block_start = 0;
  if (block_stop >= num_prev_blocks)
    block_start = block_stop - num_prev_blocks + 1;
  uint32_t num_blocks_used = 0;
  uint64_t total_block_size = 0;
  LOG_PRINT_L1("[" << __func__ << "] " << "m_height: " << m_height << "  block_start: " << block_start << "  block_stop: " << block_stop);
  size_t avg_block_size = 0;
  if (m_height == 0)
  {
    LOG_PRINT_L1("No existing blocks to check for average block size");
  }
  else if (m_cum_count)
  {
    avg_block_size = m_cum_size / m_cum_count;
    LOG_PRINT_L1("average block size across recent " << m_cum_count << " blocks: " << avg_block_size);
    m_cum_size = 0;
    m_cum_count = 0;
  }
  else
  {
    for (uint64_t block_num = block_start; block_num <= block_stop; ++block_num)
    {
      uint32_t block_size = get_block_size(block_num);
      total_block_size += block_size;
      // Track number of blocks being totalled here instead of assuming, in case
      // some blocks were to be skipped for being outliers.
      ++num_blocks_used;
    }
    avg_block_size = total_block_size / num_blocks_used;
    LOG_PRINT_L1("average block size across recent " << num_blocks_used << " blocks: " << avg_block_size);
  }
  if (avg_block_size < min_block_size)
    avg_block_size = min_block_size;
  LOG_PRINT_L1("estimated average block size for batch: " << avg_block_size);

  // bigger safety margin on smaller block sizes
  if (batch_fudge_factor < 5000.0)
    batch_fudge_factor = 5000.0;
  threshold_size = avg_block_size * db_expand_factor * batch_fudge_factor;
  return threshold_size;
}

void BlockchainLMDB::add_block(const block& blk, const size_t& block_size, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated,
    const crypto::hash& blk_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(block_heights)
  MDB_val_copy<crypto::hash> val_h(blk_hash);
  if (mdb_cursor_get(m_cur_block_heights, &val_h, NULL, MDB_SET) == 0)
    throw1(BLOCK_EXISTS("Attempting to add block that's already in the db"));

  if (m_height > 0)
  {
    MDB_val_copy<crypto::hash> parent_key(blk.prev_id);
    MDB_val parent_h;
    if (mdb_cursor_get(m_cur_block_heights, &parent_key, &parent_h, MDB_SET))
    {
      LOG_PRINT_L3("m_height: " << m_height);
      LOG_PRINT_L3("parent_key: " << blk.prev_id);
      throw0(DB_ERROR("Failed to get top block hash to check for new block's parent"));
    }
    uint64_t parent_height = *(const uint64_t *)parent_h.mv_data;
    if (parent_height != m_height - 1)
      throw0(BLOCK_PARENT_DNE("Top block is not new block's parent"));
  }

  int result = 0;

  MDB_val_copy<uint64_t> key(m_height);

  CURSOR(blocks)
  CURSOR(block_sizes)
  CURSOR(block_timestamps)
  CURSOR(block_diffs)
  CURSOR(block_coins)
  CURSOR(block_hashes)

  MDB_val_copy<blobdata> blob(block_to_blob(blk));
  result = mdb_cursor_put(m_cur_blocks, &key, &blob, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block blob to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<size_t> sz(block_size);
  result = mdb_cursor_put(m_cur_block_sizes, &key, &sz, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block size to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> ts(blk.timestamp);
  result = mdb_cursor_put(m_cur_block_timestamps, &key, &ts, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block timestamp to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<difficulty_type> diff(cumulative_difficulty);
  result = mdb_cursor_put(m_cur_block_diffs, &key, &diff, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block cumulative difficulty to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> coinsgen(coins_generated);
  result = mdb_cursor_put(m_cur_block_coins, &key, &coinsgen, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block total generated coins to db transaction: ").append(mdb_strerror(result)).c_str()));

  result = mdb_cursor_put(m_cur_block_heights, &val_h, &key, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block height by hash to db transaction: ").append(mdb_strerror(result)).c_str()));

  result = mdb_cursor_put(m_cur_block_hashes, &key, &val_h, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add block hash to db transaction: ").append(mdb_strerror(result)).c_str()));

  m_cum_size += block_size;
  m_cum_count++;
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
  mdb_txn_cursors *m_cursors = &m_wcursors;

  int result = 0;

  CURSOR(txs)
  CURSOR(tx_heights)
  CURSOR(tx_unlocks)

  MDB_val_copy<crypto::hash> val_h(tx_hash);
  MDB_val unused;
  if (mdb_cursor_get(m_cur_txs, &val_h, &unused, MDB_SET) == 0)
      throw1(TX_EXISTS("Attempting to add transaction that's already in the db"));

  MDB_val_copy<blobdata> blob(tx_to_blob(tx));
  result = mdb_cursor_put(m_cur_txs, &val_h, &blob, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add tx blob to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> height(m_height);
  result = mdb_cursor_put(m_cur_tx_heights, &val_h, &height, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add tx block height to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> unlock_time(tx.unlock_time);
  result = mdb_cursor_put(m_cur_tx_unlocks, &val_h, &unlock_time, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add tx unlock time to db transaction: ").append(mdb_strerror(result)).c_str()));
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

  auto result = mdb_del(*m_write_txn, m_tx_outputs, &val_h, NULL);
  if (result == MDB_NOTFOUND)
    LOG_PRINT_L1("tx has no outputs to remove: " << tx_hash);
  else if (result)
    throw1(DB_ERROR(std::string("Failed to add removal of tx outputs to db transaction: ").append(mdb_strerror(result)).c_str()));
}

void BlockchainLMDB::add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  int result = 0;

  CURSOR(output_txs)
  CURSOR(tx_outputs)
  CURSOR(output_indices)
  CURSOR(output_amounts)
  CURSOR(output_keys)

  MDB_val_copy<uint64_t> k(m_num_outputs);
  MDB_val_copy<crypto::hash> v(tx_hash);

  result = mdb_cursor_put(m_cur_output_txs, &k, &v, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add output tx hash to db transaction: ").append(mdb_strerror(result)).c_str()));
  result = mdb_cursor_put(m_cur_tx_outputs, &v, &k, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add <tx hash, global output index> to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> val_local_index(local_index);
  result = mdb_cursor_put(m_cur_output_indices, &k, &val_local_index, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add tx output index to db transaction: ").append(mdb_strerror(result)).c_str()));

  MDB_val_copy<uint64_t> val_amount(tx_output.amount);
  result = mdb_cursor_put(m_cur_output_amounts, &val_amount, &k, 0);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add output amount to db transaction: ").append(mdb_strerror(result)).c_str()));

  if (tx_output.target.type() == typeid(txout_to_key))
  {
    output_data_t od;
    od.pubkey = boost::get < txout_to_key > (tx_output.target).key;
    od.unlock_time = unlock_time;
    od.height = m_height;

    MDB_val_copy<output_data_t> data(od);
    //MDB_val_copy<crypto::public_key> val_pubkey(boost::get<txout_to_key>(tx_output.target).key);
    if (mdb_cursor_put(m_cur_output_keys, &k, &data, MDB_APPEND))
      throw0(DB_ERROR("Failed to add output pubkey to db transaction"));
  }
  else
  {
    throw0(DB_ERROR("Wrong output type: expected txout_to_key"));
  }

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
    LOG_PRINT_L2("tx has no outputs, so no global output indices");
  }
  else if (result)
  {
    throw0(DB_ERROR("DB error attempting to get an output"));
  }
  else
  {
    mdb_size_t num_elems = 0;
    mdb_cursor_count(cur, &num_elems);

    mdb_cursor_get(cur, &k, &v, MDB_LAST_DUP);

    for (uint64_t i = num_elems; i > 0; --i)
    {
      const tx_out tx_output = tx.vout[i-1];
      remove_output(*(const uint64_t*)v.mv_data, tx_output.amount);
      if (i > 1)
      {
        mdb_cursor_get(cur, &k, &v, MDB_PREV_DUP);
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

  mdb_size_t num_elems = 0;
  mdb_cursor_count(cur, &num_elems);

  mdb_cursor_get(cur, &k, &v, MDB_LAST_DUP);

  uint64_t amount_output_index = 0;
  uint64_t goi = 0;
  bool found_index = false;
  for (uint64_t i = num_elems; i > 0; --i)
  {
    mdb_cursor_get(cur, &k, &v, MDB_GET_CURRENT);
    goi = *(const uint64_t *)v.mv_data;
    if (goi == global_output_index)
    {
      amount_output_index = i-1;
      found_index = true;
      break;
    }
    if (i > 1)
      mdb_cursor_get(cur, &k, &v, MDB_PREV_DUP);
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
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(spent_keys)

  MDB_val_copy<crypto::key_image> val_key(k_image);
  MDB_val unused;
  if (mdb_cursor_get(m_cur_spent_keys, &val_key, &unused, MDB_SET) == 0)
      throw1(KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db"));

  char anything = '\0';
  unused.mv_size = sizeof(char);
  unused.mv_data = &anything;
  if (auto result = mdb_cursor_put(m_cur_spent_keys, &val_key, &unused, 0))
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

blobdata BlockchainLMDB::output_to_blob(const tx_out& output) const
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

uint64_t BlockchainLMDB::get_output_global_index(const uint64_t& amount, const uint64_t& index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  std::vector <uint64_t> offsets;
  std::vector <uint64_t> global_indices;
  offsets.push_back(index);
  get_output_global_indices(amount, offsets, global_indices);
  if (!global_indices.size())
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));

  return global_indices[0];
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
  if (m_open)
    close();
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
  m_cum_size = 0;
  m_cum_count = 0;

  m_hardfork = nullptr;
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
    if (!boost::filesystem::create_directories(direc))
      throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
  }

  // check for existing LMDB files in base directory
  boost::filesystem::path old_files = direc.parent_path();
  if (boost::filesystem::exists(old_files / "data.mdb") || boost::filesystem::exists(old_files / "lock.mdb"))
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

  if (auto result = mdb_env_open(m_env, filename.c_str(), mdb_flags, 0644))
    throw0(DB_ERROR(std::string("Failed to open lmdb environment: ").append(mdb_strerror(result)).c_str()));

  MDB_envinfo mei;
  mdb_env_info(m_env, &mei);
  uint64_t cur_mapsize = (double)mei.me_mapsize;

  if (cur_mapsize < mapsize)
  {
    if (auto result = mdb_env_set_mapsize(m_env, mapsize))
      throw0(DB_ERROR(std::string("Failed to set max memory map size: ").append(mdb_strerror(result)).c_str()));
    mdb_env_info(m_env, &mei);
    cur_mapsize = (double)mei.me_mapsize;
    LOG_PRINT_L1("LMDB memory map size: " << cur_mapsize);
  }

  if (need_resize())
  {
    LOG_PRINT_L0("LMDB memory map needs resized, doing that now.");
    do_resize();
  }

  int txn_flags = 0;
  if (mdb_flags & MDB_RDONLY)
    txn_flags |= MDB_RDONLY;

  // get a read/write MDB_txn, depending on mdb_flags
  mdb_txn_safe txn;
  if (auto mdb_res = mdb_txn_begin(m_env, NULL, txn_flags, txn))
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", mdb_res).c_str()));

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
  lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_output_amounts, "Failed to open db handle for m_output_amounts");
  lmdb_db_open(txn, LMDB_OUTPUT_KEYS, MDB_INTEGERKEY | MDB_CREATE, m_output_keys, "Failed to open db handle for m_output_keys");

  lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_CREATE, m_spent_keys, "Failed to open db handle for m_spent_keys");

  lmdb_db_open(txn, LMDB_HF_STARTING_HEIGHTS, MDB_CREATE, m_hf_starting_heights, "Failed to open db handle for m_hf_starting_heights");
  lmdb_db_open(txn, LMDB_HF_VERSIONS, MDB_CREATE, m_hf_versions, "Failed to open db handle for m_hf_versions");

  lmdb_db_open(txn, LMDB_PROPERTIES, MDB_CREATE, m_properties, "Failed to open db handle for m_properties");

  mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
  mdb_set_dupsort(txn, m_tx_outputs, compare_uint64);
  mdb_set_compare(txn, m_spent_keys, compare_hash32);
  mdb_set_compare(txn, m_block_heights, compare_hash32);
  mdb_set_compare(txn, m_txs, compare_hash32);
  mdb_set_compare(txn, m_tx_unlocks, compare_hash32);
  mdb_set_compare(txn, m_tx_heights, compare_hash32);
  mdb_set_compare(txn, m_hf_starting_heights, compare_uint8);
  mdb_set_compare(txn, m_hf_versions, compare_uint64);
  mdb_set_compare(txn, m_properties, compare_string);

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

  bool compatible = true;

  // ND: This "new" version of the lmdb database is incompatible with
  // the previous version. Ensure that the output_keys database is
  // sizeof(output_data_t) in length. Otherwise, inform user and
  // terminate.
  if(m_height > 0)
  {
    MDB_val_copy<uint64_t> k(0);
    MDB_val v;
    auto get_result = mdb_get(txn, m_output_keys, &k, &v);
    if(get_result != MDB_SUCCESS)
    {
      txn.abort();
      m_open = false;
      return;
    }

    // LOG_PRINT_L0("Output keys size: " << v.mv_size);
    if(v.mv_size != sizeof(output_data_t))
      compatible = false;
  }

  MDB_val_copy<const char*> k("version");
  MDB_val v;
  auto get_result = mdb_get(txn, m_properties, &k, &v);
  if(get_result == MDB_SUCCESS)
  {
    if (*(const uint32_t*)v.mv_data > VERSION)
    {
      LOG_PRINT_RED_L0("Existing lmdb database was made by a later version. We don't know how it will change yet.");
      compatible = false;
    }
#if VERSION > 0
    else if (*(const uint32_t*)v.mv_data < VERSION)
    {
      compatible = false;
    }
#endif
  }
  else
  {
    // if not found, but we're on version 0, it's fine. If the DB's empty, it's fine too.
    if (VERSION > 0 && m_height > 0)
      compatible = false;
  }

  if (!compatible)
  {
    txn.abort();
    mdb_env_close(m_env);
    m_open = false;
    LOG_PRINT_RED_L0("Existing lmdb database is incompatible with this version.");
    LOG_PRINT_RED_L0("Please delete the existing database and resync.");
    return;
  }

  if (!(mdb_flags & MDB_RDONLY))
  {
    // only write version on an empty DB
    if (m_height == 0)
    {
      MDB_val_copy<const char*> k("version");
      MDB_val_copy<uint32_t> v(VERSION);
      auto put_result = mdb_put(txn, m_properties, &k, &v, 0);
      if (put_result != MDB_SUCCESS)
      {
        txn.abort();
        mdb_env_close(m_env);
        m_open = false;
        LOG_PRINT_RED_L0("Failed to write version to database.");
        return;
      }
    }
  }

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
  m_tinfo.reset();

  // FIXME: not yet thread safe!!!  Use with care.
  mdb_env_close(m_env);
  m_open = false;
}

void BlockchainLMDB::sync()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // Does nothing unless LMDB environment was opened with MDB_NOSYNC or in part
  // MDB_NOMETASYNC. Force flush to be synchronous.
  if (auto result = mdb_env_sync(m_env, true))
  {
    throw0(DB_ERROR(std::string("Failed to sync database: ").append(mdb_strerror(result)).c_str()));
  }
}

void BlockchainLMDB::reset()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (mdb_txn_begin(m_env, NULL, 0, txn))
    throw0(DB_ERROR("Failed to create a transaction for the db"));
  mdb_drop(txn, m_blocks, 0);
  mdb_drop(txn, m_block_timestamps, 0);
  mdb_drop(txn, m_block_heights, 0);
  mdb_drop(txn, m_block_hashes, 0);
  mdb_drop(txn, m_block_sizes, 0);
  mdb_drop(txn, m_block_diffs, 0);
  mdb_drop(txn, m_block_coins, 0);
  mdb_drop(txn, m_txs, 0);
  mdb_drop(txn, m_tx_unlocks, 0);
  mdb_drop(txn, m_tx_heights, 0);
  mdb_drop(txn, m_tx_outputs, 0);
  mdb_drop(txn, m_output_txs, 0);
  mdb_drop(txn, m_output_indices, 0);
  mdb_drop(txn, m_output_amounts, 0);
  mdb_drop(txn, m_output_keys, 0);
  mdb_drop(txn, m_spent_keys, 0);
  mdb_drop(txn, m_hf_starting_heights, 0);
  mdb_drop(txn, m_hf_versions, 0);
  mdb_drop(txn, m_properties, 0);
  txn.commit();
  m_height = 0;
  m_num_outputs = 0;
  m_cum_size = 0;
  m_cum_count = 0;
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

#define TXN_PREFIX(flags); \
  mdb_txn_safe auto_txn; \
  mdb_txn_safe* txn_ptr = &auto_txn; \
  if (m_batch_active) \
    txn_ptr = m_write_txn; \
  else \
  { \
    if (auto mdb_res = mdb_txn_begin(m_env, NULL, flags, auto_txn)) \
      throw0(DB_ERROR(lmdb_error(std::string("Failed to create a transaction for the db in ")+__FUNCTION__+": ", mdb_res).c_str())); \
  } \

#define TXN_PREFIX_RDONLY() \
  bool my_rtxn = block_rtxn_start(); \
  MDB_txn *m_txn = m_write_txn ? m_write_txn->m_txn : m_tinfo->m_ti_rtxn
#define TXN_POSTFIX_RDONLY() \
  if (my_rtxn) block_rtxn_stop()

#define TXN_POSTFIX_SUCCESS() \
  do { \
    if (! m_batch_active) \
      auto_txn.commit(); \
  } while(0)


// The below two macros are for DB access within block add/remove, whether
// regular batch txn is in use or not. m_write_txn is used as a batch txn, even
// if it's only within block add/remove.
//
// DB access functions that may be called both within block add/remove and
// without should use these. If the function will be called ONLY within block
// add/remove, m_write_txn alone may be used instead of these macros.

#define TXN_BLOCK_PREFIX(flags); \
  mdb_txn_safe auto_txn; \
  mdb_txn_safe* txn_ptr = &auto_txn; \
  if (m_batch_active || m_write_txn) \
    txn_ptr = m_write_txn; \
  else \
  { \
    if (auto mdb_res = mdb_txn_begin(m_env, NULL, flags, auto_txn)) \
      throw0(DB_ERROR(lmdb_error(std::string("Failed to create a transaction for the db in ")+__FUNCTION__+": ", mdb_res).c_str())); \
  } \

#define TXN_BLOCK_POSTFIX_SUCCESS() \
  do { \
    if (! m_batch_active && ! m_write_txn) \
      auto_txn.commit(); \
  } while(0)

bool BlockchainLMDB::block_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_heights);

  MDB_val_copy<crypto::hash> key(h);
  auto get_result = mdb_cursor_get(m_cur_block_heights, &key, NULL, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    TXN_POSTFIX_RDONLY();
    LOG_PRINT_L3("Block with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
    return false;
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch block index from hash"));

  TXN_POSTFIX_RDONLY();
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_heights);

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_heights, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(BLOCK_DNE("Attempted to retrieve non-existent block height"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block height from the db"));

  uint64_t ret = *(const uint64_t *)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(blocks);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_blocks, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get block from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block from the db"));

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  block b;
  if (!parse_and_validate_block_from_blob(bd, b))
    throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

  TXN_POSTFIX_RDONLY();

  return b;
}

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_timestamps);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_timestamps, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get timestamp from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- timestamp not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a timestamp from the db"));

  uint64_t ret = *(const uint64_t *)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_sizes);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_sizes, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get block size from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block size from the db"));

  size_t ret = *(const size_t *)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__ << "  height: " << height);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_diffs);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_diffs, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get cumulative difficulty from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- difficulty not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db"));

  difficulty_type ret = *(const difficulty_type*)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_coins);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_coins, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get generated coins from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a total generated coins from the db"));

  uint64_t ret = *(const uint64_t*)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(block_hashes);

  MDB_val_copy<uint64_t> key(height);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_block_hashes, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get hash from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- hash not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR(std::string("Error attempting to retrieve a block hash from the db: ").
          append(mdb_strerror(get_result)).c_str()));

  crypto::hash ret = *(const crypto::hash*)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(txs);

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;

  TIME_MEASURE_START(time1);
  auto get_result = mdb_cursor_get(m_cur_txs, &key, &result, MDB_SET);
  TIME_MEASURE_FINISH(time1);
  time_tx_exists += time1;

  TXN_POSTFIX_RDONLY();

  if (get_result == MDB_NOTFOUND)
  {
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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(tx_unlocks);

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_tx_unlocks, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(TX_DNE(std::string("tx unlock time with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx unlock time from hash"));

  uint64_t ret = *(const uint64_t*)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

transaction BlockchainLMDB::get_tx(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(txs);

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_txs, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx from hash"));

  blobdata bd;
  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  transaction tx;
  if (!parse_and_validate_tx_from_blob(bd, tx))
    throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));

  TXN_POSTFIX_RDONLY();

  return tx;
}

uint64_t BlockchainLMDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  MDB_stat db_stats;
  if (mdb_stat(m_txn, m_txs, &db_stats))
    throw0(DB_ERROR("Failed to query m_txs"));

  TXN_POSTFIX_RDONLY();

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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(tx_heights);

  MDB_val_copy<crypto::hash> key(h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_tx_heights, &key, &result, MDB_SET);
  if (get_result == MDB_NOTFOUND)
  {
    throw1(TX_DNE(std::string("tx height with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch tx height from hash"));

  uint64_t ret = *(const uint64_t*)result.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    TXN_POSTFIX_RDONLY();
    return 0;
  }
  else if (result)
    throw0(DB_ERROR("DB error attempting to get number of outputs of an amount"));

  mdb_size_t num_elems = 0;
  mdb_cursor_count(m_cur_output_amounts, &num_elems);

  TXN_POSTFIX_RDONLY();

  return num_elems;
}

output_data_t BlockchainLMDB::get_output_key(const uint64_t &global_index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_keys);

  MDB_val_copy<uint64_t> k(global_index);
  MDB_val v;
  auto get_result = mdb_cursor_get(m_cur_output_keys, &k, &v, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get output pubkey by global index, but key does not exist"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));
  output_data_t ret = *(const output_data_t *) v.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

output_data_t BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  uint64_t glob_index = get_output_global_index(amount, index);
  return get_output_key(glob_index);
}

tx_out_index BlockchainLMDB::get_output_tx_and_index_from_global(const uint64_t& index) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_txs);
  RCURSOR(output_indices);

  MDB_val_copy<uint64_t> k(index);
  MDB_val v;

  auto get_result = mdb_cursor_get(m_cur_output_txs, &k, &v, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

  crypto::hash tx_hash = *(const crypto::hash*)v.mv_data;

  get_result = mdb_cursor_get(m_cur_output_indices, &k, &v, MDB_SET);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx index"));

  tx_out_index ret = tx_out_index(tx_hash, *(const uint64_t *)v.mv_data);
  TXN_POSTFIX_RDONLY();
  return ret;
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  std::vector < uint64_t > offsets;
  std::vector<tx_out_index> indices;
  offsets.push_back(index);
  get_output_tx_and_index(amount, offsets, indices);
  if (!indices.size())
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));

  return indices[0];
}

std::vector<uint64_t> BlockchainLMDB::get_tx_output_indices(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<uint64_t> index_vec;

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(tx_outputs);

  MDB_val_copy<crypto::hash> k(h);
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_tx_outputs, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output by tx hash and tx index, but output not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  mdb_size_t num_elems = 0;
  mdb_cursor_count(m_cur_tx_outputs, &num_elems);

  mdb_cursor_get(m_cur_tx_outputs, &k, &v, MDB_FIRST_DUP);

  for (uint64_t i = 0; i < num_elems; ++i)
  {
    mdb_cursor_get(m_cur_tx_outputs, &k, &v, MDB_GET_CURRENT);
    index_vec.push_back(*(const uint64_t *)v.mv_data);
    mdb_cursor_get(m_cur_tx_outputs, &k, &v, MDB_NEXT_DUP);
  }

  TXN_POSTFIX_RDONLY();

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

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_amounts);

  uint64_t i = 0;
  uint64_t global_index;
  BOOST_FOREACH(const auto& vout, tx.vout)
  {
    uint64_t amount =  vout.amount;

    global_index = index_vec[i];

    MDB_val_copy<uint64_t> k(amount);
    MDB_val v;

    auto result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
    if (result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
    else if (result)
      throw0(DB_ERROR("DB error attempting to get an output"));

    mdb_size_t num_elems = 0;
    mdb_cursor_count(m_cur_output_amounts, &num_elems);

    mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_FIRST_DUP);

    uint64_t amount_output_index = 0;
    uint64_t output_index = 0;
    bool found_index = false;
    for (uint64_t j = 0; j < num_elems; ++j)
    {
      mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_CURRENT);
      output_index = *(const uint64_t *)v.mv_data;
      if (output_index == global_index)
      {
        amount_output_index = j;
        found_index = true;
        break;
      }
      mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_NEXT_DUP);
    }
    if (found_index)
    {
      index_vec2.push_back(amount_output_index);
    }
    else
    {
      // not found
      TXN_POSTFIX_RDONLY();
      throw1(OUTPUT_DNE("specified output not found in db"));
    }

    ++i;
  }

  TXN_POSTFIX_RDONLY();

  return index_vec2;
}



bool BlockchainLMDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(spent_keys);

  MDB_val_copy<crypto::key_image> val_key(img);
  if (mdb_cursor_get(m_cur_spent_keys, &val_key, NULL, MDB_SET) == 0)
  {
    TXN_POSTFIX_RDONLY();
    return true;
  }

  TXN_POSTFIX_RDONLY();
  return false;
}

bool BlockchainLMDB::for_all_key_images(std::function<bool(const crypto::key_image&)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(spent_keys);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_spent_keys, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret < 0)
      throw0(DB_ERROR("Failed to enumerate key images"));
    const crypto::key_image k_image = *(const crypto::key_image*)k.mv_data;
    if (!f(k_image)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

bool BlockchainLMDB::for_all_blocks(std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(blocks);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_blocks, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate blocks"));
    uint64_t height = *(const uint64_t*)k.mv_data;
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    block b;
    if (!parse_and_validate_block_from_blob(bd, b))
      throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));
    crypto::hash hash;
    if (!get_block_hash(b, hash))
        throw0(DB_ERROR("Failed to get block hash from blob retrieved from the db"));
    if (!f(height, hash, b)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

bool BlockchainLMDB::for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(txs);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_txs, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate transactions"));
    const crypto::hash hash = *(const crypto::hash*)k.mv_data;
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    transaction tx;
    if (!parse_and_validate_tx_from_blob(bd, tx))
      throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
    if (!f(hash, tx)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

bool BlockchainLMDB::for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, size_t tx_idx)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_amounts);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_output_amounts, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate outputs"));
    uint64_t amount = *(const uint64_t*)k.mv_data;
    uint64_t global_index = *(const uint64_t*)v.mv_data;
    tx_out_index toi = get_output_tx_and_index_from_global(global_index);
    if (!f(amount, toi.first, toi.second)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

// batch_num_blocks: (optional) Used to check if resize needed before batch transaction starts.
void BlockchainLMDB::batch_start(uint64_t batch_num_blocks)
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

  check_and_resize_for_batch(batch_num_blocks);

  m_write_batch_txn = new mdb_txn_safe();

  // NOTE: need to make sure it's destroyed properly when done
  if (auto mdb_res = mdb_txn_begin(m_env, NULL, 0, *m_write_batch_txn))
  {
    delete m_write_batch_txn;
    m_write_batch_txn = nullptr;
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", mdb_res).c_str()));
  }
  // indicates this transaction is for batch transactions, but not whether it's
  // active
  m_write_batch_txn->m_batch_txn = true;
  m_write_txn = m_write_batch_txn;

  m_batch_active = true;
  memset(&m_wcursors, 0, sizeof(m_wcursors));

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
  memset(&m_wcursors, 0, sizeof(m_wcursors));
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
  memset(&m_wcursors, 0, sizeof(m_wcursors));
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
  memset(&m_wcursors, 0, sizeof(m_wcursors));
  LOG_PRINT_L3("batch transaction: aborted");
}

void BlockchainLMDB::set_batch_transactions(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  m_batch_transactions = batch_transactions;
  LOG_PRINT_L3("batch transactions " << (m_batch_transactions ? "enabled" : "disabled"));
}

// return true if we started the txn, false if already started
bool BlockchainLMDB::block_rtxn_start() const
{
  if (m_write_txn)
    return false;
  if (!m_tinfo.get())
  {
    m_tinfo.reset(new mdb_threadinfo);
    memset(&m_tinfo->m_ti_rcursors, 0, sizeof(m_tinfo->m_ti_rcursors));
    memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
    if (auto mdb_res = mdb_txn_begin(m_env, NULL, MDB_RDONLY, &m_tinfo->m_ti_rtxn))
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to create a read transaction for the db: ", mdb_res).c_str()));
  } else if (!m_tinfo->m_ti_rflags.m_rf_txn)
  {
    if (auto mdb_res = mdb_txn_renew(m_tinfo->m_ti_rtxn))
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to renew a read transaction for the db: ", mdb_res).c_str()));
  } else
  {
    return false;
  }
  m_tinfo->m_ti_rflags.m_rf_txn = true;
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  return true;
}

void BlockchainLMDB::block_rtxn_stop() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  mdb_txn_reset(m_tinfo->m_ti_rtxn);
  memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
}

void BlockchainLMDB::block_txn_start(bool readonly)
{
  if (readonly)
  {
    bool didit = false;
    if (m_write_txn)
      return;
    if (!m_tinfo.get())
    {
      m_tinfo.reset(new mdb_threadinfo);
      memset(&m_tinfo->m_ti_rcursors, 0, sizeof(m_tinfo->m_ti_rcursors));
      memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
      if (auto mdb_res = mdb_txn_begin(m_env, NULL, MDB_RDONLY, &m_tinfo->m_ti_rtxn))
        throw0(DB_ERROR_TXN_START(lmdb_error("Failed to create a read transaction for the db: ", mdb_res).c_str()));
      didit = true;
    } else if (!m_tinfo->m_ti_rflags.m_rf_txn)
    {
      if (auto mdb_res = mdb_txn_renew(m_tinfo->m_ti_rtxn))
        throw0(DB_ERROR_TXN_START(lmdb_error("Failed to renew a read transaction for the db: ", mdb_res).c_str()));
      didit = true;
    }
    if (didit)
    {
      m_tinfo->m_ti_rflags.m_rf_txn = true;
      LOG_PRINT_L3("BlockchainLMDB::" << __func__ << " RO");
    }
    return;
  }

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // Distinguish the exceptions here from exceptions that would be thrown while
  // using the txn and committing it.
  //
  // If an exception is thrown in this setup, we don't want the caller to catch
  // it and proceed as if there were an existing write txn, such as trying to
  // call block_txn_abort(). It also indicates a serious issue which will
  // probably be thrown up another layer.
  if (! m_batch_active && m_write_txn)
    throw0(DB_ERROR_TXN_START((std::string("Attempted to start new write txn when write txn already exists in ")+__FUNCTION__).c_str()));
  if (! m_batch_active)
  {
    m_write_txn = new mdb_txn_safe();
    if (auto mdb_res = mdb_txn_begin(m_env, NULL, 0, *m_write_txn))
    {
      delete m_write_txn;
      m_write_txn = nullptr;
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to create a transaction for the db: ", mdb_res).c_str()));
    }
    memset(&m_wcursors, 0, sizeof(m_wcursors));
  }
}

void BlockchainLMDB::block_txn_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_active)
  {
    if (m_write_txn)
	{
      TIME_MEASURE_START(time1);
      m_write_txn->commit();
      TIME_MEASURE_FINISH(time1);
      time_commit1 += time1;

      delete m_write_txn;
      m_write_txn = nullptr;
      memset(&m_wcursors, 0, sizeof(m_wcursors));
	}
	else if (m_tinfo->m_ti_rtxn)
	{
	  mdb_txn_reset(m_tinfo->m_ti_rtxn);
	  memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
	}
  }
}

void BlockchainLMDB::block_txn_abort()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_active)
  {
    if (m_write_txn != nullptr)
    {
      delete m_write_txn;
      m_write_txn = nullptr;
      memset(&m_wcursors, 0, sizeof(m_wcursors));
    }
	else if (m_tinfo->m_ti_rtxn)
	{
	  mdb_txn_reset(m_tinfo->m_ti_rtxn);
	  memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
	}
    else
    {
      // This would probably mean an earlier exception was caught, but then we
      // proceeded further than we should have.
      throw0(DB_ERROR((std::string("BlockchainLMDB::") + __func__ +
                       std::string(": block-level DB transaction abort called when write txn doesn't exist")
                      ).c_str()));
    }
  }
}

uint64_t BlockchainLMDB::add_block(const block& blk, const size_t& block_size, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated,
    const std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (m_height % 1000 == 0)
  {
    // for batch mode, DB resize check is done at start of batch transaction
    if (! m_batch_active && need_resize())
    {
      LOG_PRINT_L0("LMDB memory map needs resized, doing that now.");
      do_resize();
    }
  }

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::add_block(blk, block_size, cumulative_difficulty, coins_generated, txs);
  }
  catch (DB_ERROR_TXN_START& e)
  {
    throw;
  }
  catch (...)
  {
    m_num_outputs = num_outputs;
    block_txn_abort();
    throw;
  }

  return ++m_height;
}

void BlockchainLMDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  block_txn_start(false);

  uint64_t num_outputs = m_num_outputs;
  try
  {
    BlockchainDB::pop_block(blk, txs);
	block_txn_stop();
  }
  catch (...)
  {
    m_num_outputs = num_outputs;
	block_txn_abort();
    throw;
  }

  --m_height;
}

void BlockchainLMDB::get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
    std::vector<tx_out_index> &tx_out_indices) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  tx_out_indices.clear();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_txs);
  RCURSOR(output_indices);

  for (const uint64_t &index : global_indices)
  {
    MDB_val_copy<uint64_t> k(index);
    MDB_val v;

    auto get_result = mdb_cursor_get(m_cur_output_txs, &k, &v, MDB_SET);
    if (get_result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("output with given index not in db"));
    else if (get_result)
      throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

    crypto::hash tx_hash = *(const crypto::hash*) v.mv_data;

    get_result = mdb_cursor_get(m_cur_output_indices, &k, &v, MDB_SET);
    if (get_result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("output with given index not in db"));
    else if (get_result)
      throw0(DB_ERROR("DB error attempting to fetch output tx index"));

    auto result = tx_out_index(tx_hash, *(const uint64_t *) v.mv_data);
    tx_out_indices.push_back(result);
  }

  TXN_POSTFIX_RDONLY();
}

void BlockchainLMDB::get_output_global_indices(const uint64_t& amount, const std::vector<uint64_t> &offsets,
    std::vector<uint64_t> &global_indices)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  TIME_MEASURE_START(txx);
  check_open();
  global_indices.clear();

  uint64_t max = 0;
  for (const uint64_t &index : offsets)
  {
    if (index > max)
      max = index;
  }

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR("DB error attempting to get an output"));

  mdb_size_t num_elems = 0;
  mdb_cursor_count(m_cur_output_amounts, &num_elems);
  if (max <= 1 && num_elems <= max)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but output not found"));

  uint64_t t_dbmul = 0;
  uint64_t t_dbscan = 0;
  if (max <= 1)
  {
    for (const uint64_t& index : offsets)
    {
      mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_FIRST_DUP);
      for (uint64_t i = 0; i < index; ++i)
      {
        mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_NEXT_DUP);
      }

      mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_CURRENT);
      uint64_t glob_index = *(const uint64_t*) v.mv_data;
      LOG_PRINT_L3("Amount: " << amount << " M0->v: " << glob_index);
      global_indices.push_back(glob_index);
    }
  }
  else
  {
    uint32_t curcount = 0;
    uint32_t blockstart = 0;
    for (const uint64_t& index : offsets)
    {
      if (index >= num_elems)
      {
        LOG_PRINT_L1("Index: " << index << " Elems: " << num_elems << " partial results found for get_output_tx_and_index");
        break;
      }
      if (!curcount && index > num_elems/2)
      {
        mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_LAST_DUP);
        mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_PREV);    /* kludge to unset C_EOF */
        mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_NEXT);
        mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_MULTIPLE);

        curcount = num_elems;
        while(1)
        {
          TIME_MEASURE_START(db1);
          int count = v.mv_size / sizeof(uint64_t);
          curcount -= count;
          if (curcount > index)
          {
            mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_PREV_MULTIPLE);
          } else
          {
            blockstart = curcount;
            curcount += count;
            break;
          }
          TIME_MEASURE_FINISH(db1);
          t_dbmul += db1;
        }

      } else
      {
        while (index >= curcount)
        {
          TIME_MEASURE_START(db1);
          if (mdb_cursor_get(m_cur_output_amounts, &k, &v, curcount == 0 ? MDB_GET_MULTIPLE : MDB_NEXT_MULTIPLE) != 0)
          {
            // allow partial results
            result = false;
            break;
          }

          int count = v.mv_size / sizeof(uint64_t);

          blockstart = curcount;
          curcount += count;
          TIME_MEASURE_FINISH(db1);
          t_dbmul += db1;
        }
      }

      LOG_PRINT_L3("Records returned: " << curcount << " Index: " << index);
      TIME_MEASURE_START(db2);
      uint64_t actual_index = index - blockstart;
      uint64_t glob_index = ((const uint64_t*) v.mv_data)[actual_index];

      LOG_PRINT_L3("Amount: " << amount << " M1->v: " << glob_index);
      global_indices.push_back(glob_index);

      TIME_MEASURE_FINISH(db2);
      t_dbscan += db2;

    }
  }

  TXN_POSTFIX_RDONLY();

  TIME_MEASURE_FINISH(txx);
  LOG_PRINT_L3("txx: " << txx << " db1: " << t_dbmul << " db2: " << t_dbscan);
}

void BlockchainLMDB::get_output_key(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  TIME_MEASURE_START(db3);
  check_open();
  outputs.clear();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  std::vector <uint64_t> global_indices;
  get_output_global_indices(amount, offsets, global_indices);

  if (global_indices.size() > 0)
  {
    RCURSOR(output_keys);

    for (const uint64_t &index : global_indices)
    {
      MDB_val_copy<uint64_t> k(index);
      MDB_val v;

      auto get_result = mdb_cursor_get(m_cur_output_keys, &k, &v, MDB_SET);
      if (get_result == MDB_NOTFOUND)
        throw1(OUTPUT_DNE("Attempting to get output pubkey by global index, but key does not exist"));
      else if (get_result)
        throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));

      output_data_t data = *(const output_data_t *) v.mv_data;
      outputs.push_back(data);
    }

  }
  TXN_POSTFIX_RDONLY();

  TIME_MEASURE_FINISH(db3);
  LOG_PRINT_L3("db3: " << db3);
}

void BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  indices.clear();

  std::vector <uint64_t> global_indices;
  get_output_global_indices(amount, offsets, global_indices);

  TIME_MEASURE_START(db3);
  if(global_indices.size() > 0)
  {
    get_output_tx_and_index_from_global(global_indices, indices);
  }
  TIME_MEASURE_FINISH(db3);
  LOG_PRINT_L3("db3: " << db3);
}

void BlockchainLMDB::check_hard_fork_info()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX(0);

  MDB_stat db_stat1, db_stat2;
  if (mdb_stat(*txn_ptr, m_blocks, &db_stat1))
    throw0(DB_ERROR("Failed to query m_blocks"));
  if (mdb_stat(*txn_ptr, m_hf_versions, &db_stat2))
    throw0(DB_ERROR("Failed to query m_hf_starting_heights"));
  if (db_stat1.ms_entries != db_stat2.ms_entries)
  {
    // Empty, but don't delete. This allows this function to be called after
    // startup, after the subdbs have already been created, and rest of startup
    // can proceed. If these don't exist, hard fork's init() will fail.
    //
    // If these are empty, hard fork's init() will repopulate the hard fork
    // data.
    mdb_drop(*txn_ptr, m_hf_starting_heights, 0);
    mdb_drop(*txn_ptr, m_hf_versions, 0);
  }

  TXN_POSTFIX_SUCCESS();
}

void BlockchainLMDB::drop_hard_fork_info()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX(0);

  mdb_drop(*txn_ptr, m_hf_starting_heights, 1);
  mdb_drop(*txn_ptr, m_hf_versions, 1);

  TXN_POSTFIX_SUCCESS();
}

void BlockchainLMDB::set_hard_fork_starting_height(uint8_t version, uint64_t height)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_BLOCK_PREFIX(0);

  MDB_val_copy<uint8_t> val_key(version);
  MDB_val_copy<uint64_t> val_value(height);
  if (auto result = mdb_put(*txn_ptr, m_hf_starting_heights, &val_key, &val_value, MDB_APPEND))
    throw1(DB_ERROR(std::string("Error adding hard fork starting height to db transaction: ").append(mdb_strerror(result)).c_str()));

  TXN_BLOCK_POSTFIX_SUCCESS();
}

uint64_t BlockchainLMDB::get_hard_fork_starting_height(uint8_t version) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  MDB_val_copy<uint8_t> val_key(version);
  MDB_val val_ret;
  auto result = mdb_get(m_txn, m_hf_starting_heights, &val_key, &val_ret);
  if (result == MDB_NOTFOUND)
    return std::numeric_limits<uint64_t>::max();
  if (result)
    throw0(DB_ERROR("Error attempting to retrieve a hard fork starting height from the db"));

  uint64_t ret;
#ifdef MISALIGNED_OK
  ret = *(const uint64_t*)val_ret.mv_data;
#else
  memcpy(&ret, val_ret.mv_data, sizeof(uint64_t));
#endif
  TXN_POSTFIX_RDONLY();
  return ret;
}

void BlockchainLMDB::set_hard_fork_version(uint64_t height, uint8_t version)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_BLOCK_PREFIX(0);

  MDB_val_copy<uint64_t> val_key(height);
  MDB_val_copy<uint8_t> val_value(version);
  int result;
  result = mdb_put(*txn_ptr, m_hf_versions, &val_key, &val_value, MDB_APPEND);
  if (result == MDB_KEYEXIST)
    result = mdb_put(*txn_ptr, m_hf_versions, &val_key, &val_value, 0);
  if (result)
    throw1(DB_ERROR(std::string("Error adding hard fork version to db transaction: ").append(mdb_strerror(result)).c_str()));

  TXN_BLOCK_POSTFIX_SUCCESS();
}

uint8_t BlockchainLMDB::get_hard_fork_version(uint64_t height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  const mdb_txn_cursors *m_cursors = m_write_txn ? &m_wcursors : &m_tinfo->m_ti_rcursors;
  RCURSOR(hf_versions);

  MDB_val_copy<uint64_t> val_key(height);
  MDB_val val_ret;
  auto result = mdb_cursor_get(m_cur_hf_versions, &val_key, &val_ret, MDB_SET);
  if (result == MDB_NOTFOUND || result)
    throw0(DB_ERROR(std::string("Error attempting to retrieve a hard fork version at height ").append(boost::lexical_cast<std::string>(height)).append(" from the db: ").append(mdb_strerror(result)).c_str()));

  uint8_t ret = *(const uint8_t*)val_ret.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

bool BlockchainLMDB::is_read_only() const
{
  unsigned int flags;
  auto result = mdb_env_get_flags(m_env, &flags);
  if (result)
    throw0(DB_ERROR(std::string("Error getting database environment info: ").append(mdb_strerror(result)).c_str()));

  if (flags & MDB_RDONLY)
    return true;

  return false;
}

void BlockchainLMDB::fixup()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // Always call parent as well
  BlockchainDB::fixup();
}

}  // namespace cryptonote
