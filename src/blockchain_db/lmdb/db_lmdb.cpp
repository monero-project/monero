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

#include "db_lmdb.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/circular_buffer.hpp>
#include <memory>  // std::unique_ptr
#include <cstring>  // memcpy

#ifdef WIN32
#include <winioctl.h>
#endif

#include "string_tools.h"
#include "common/util.h"
#include "common/pruning.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "crypto/crypto.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "blockchain.db.lmdb"


#if defined(__i386) || defined(__x86_64)
#define MISALIGNED_OK	1
#endif

using epee::string_tools::pod_to_hex;
using namespace crypto;

// Increase when the DB structure changes
#define VERSION 6

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

#define MDB_val_set(var, val)   MDB_val var = {sizeof(val), (void *)&val}

#define MDB_val_sized(var, val) MDB_val var = {val.size(), (void *)val.data()}

#define MDB_val_str(var, val) MDB_val var = {strlen(val) + 1, (void *)val}

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

}

namespace cryptonote
{

int BlockchainLMDB::compare_uint64(const MDB_val *a, const MDB_val *b)
{
  uint64_t va, vb;
  memcpy(&va, a->mv_data, sizeof(va));
  memcpy(&vb, b->mv_data, sizeof(vb));
  return (va < vb) ? -1 : va > vb;
}

int BlockchainLMDB::compare_uint8(const MDB_val *a, const MDB_val *b)
{
  uint8_t va, vb;
  memcpy(&va, a->mv_data, sizeof(va));
  memcpy(&vb, b->mv_data, sizeof(vb));
  return (va < vb) ? -1 : va > vb;
}

int BlockchainLMDB::compare_hash32(const MDB_val *a, const MDB_val *b)
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

int BlockchainLMDB::compare_string(const MDB_val *a, const MDB_val *b)
{
  const char *va = (const char*) a->mv_data;
  const char *vb = (const char*) b->mv_data;
  const size_t sz = std::min(a->mv_size, b->mv_size);
  int ret = strncmp(va, vb, sz);
  if (ret)
    return ret;
  if (a->mv_size < b->mv_size)
    return -1;
  if (a->mv_size > b->mv_size)
    return 1;
  return 0;
}

}

namespace
{

/* DB schema:
 *
 * Table            Key          Data
 * -----            ---          ----
 * blocks           block ID     block blob
 * block_heights    block hash   block height
 * block_info       block ID     {block metadata}
 *
 * txs_pruned       txn ID       pruned txn blob
 * txs_prunable     txn ID       prunable txn blob
 * txs_prunable_hash txn ID      prunable txn hash
 * txs_prunable_tip txn ID       height
 * tx_indices       txn hash     {txn ID, metadata}
 * tx_outputs       txn ID       [txn amount output indices]
 *
 * output_txs       output ID    {txn hash, local index}
 * output_amounts   amount       [{amount output index, metadata}...]
 *
 * spent_keys       input hash   -
 *
 * locked_outputs   block ID     [{OutputContext}...]
 * leaves           leaf_idx     {OutputContext}
 * layers           layer_idx    [{child_chunk_idx, child_chunk_hash}...]
 * tree_edges       block ID     [child_chunk_hash]
 * tree_meta        block ID     n_leaf_tuples
 *
 * timelocked_outputs block ID   [{OutputContext}...]
 *
 * txpool_meta      txn hash     txn metadata
 * txpool_blob      txn hash     txn blob
 *
 * alt_blocks       block hash   {block data, block blob}
 *
 * Note: where the data items are of uniform size, DUPFIXED tables have
 * been used to save space. In most of these cases, a dummy "zerokval"
 * key is used when accessing the table; the Key listed above will be
 * attached as a prefix on the Data to serve as the DUPSORT key.
 * (DUPFIXED saves 8 bytes per record.)
 *
 * The output_amounts, locked_outputs, layers, and timelocked_outputs
 * tables don't use a dummy key, but use DUPSORT.
 */
const char* const LMDB_BLOCKS = "blocks";
const char* const LMDB_BLOCK_HEIGHTS = "block_heights";
const char* const LMDB_BLOCK_INFO = "block_info";

const char* const LMDB_TXS = "txs";
const char* const LMDB_TXS_PRUNED = "txs_pruned";
const char* const LMDB_TXS_PRUNABLE = "txs_prunable";
const char* const LMDB_TXS_PRUNABLE_HASH = "txs_prunable_hash";
const char* const LMDB_TXS_PRUNABLE_TIP = "txs_prunable_tip";
const char* const LMDB_TX_INDICES = "tx_indices";
const char* const LMDB_TX_OUTPUTS = "tx_outputs";

const char* const LMDB_OUTPUT_TXS = "output_txs";
const char* const LMDB_OUTPUT_AMOUNTS = "output_amounts";
const char* const LMDB_SPENT_KEYS = "spent_keys";

// Curve trees merkle tree tables
const char* const LMDB_LOCKED_OUTPUTS = "locked_outputs";
const char* const LMDB_LEAVES = "leaves";
const char* const LMDB_LAYERS = "layers";
const char* const LMDB_TREE_EDGES = "tree_edges";
const char* const LMDB_TREE_META = "tree_meta";

const char* const LMDB_TIMELOCKED_OUTPUTS = "timelocked_outputs";

const char* const LMDB_TXPOOL_META = "txpool_meta";
const char* const LMDB_TXPOOL_BLOB = "txpool_blob";

const char* const LMDB_ALT_BLOCKS = "alt_blocks";

const char* const LMDB_HF_STARTING_HEIGHTS = "hf_starting_heights";
const char* const LMDB_HF_VERSIONS = "hf_versions";

const char* const LMDB_PROPERTIES = "properties";

const char zerokey[8] = {0};
const MDB_val zerokval = { sizeof(zerokey), (void *)zerokey };

const std::string lmdb_error(const std::string& error_string, int mdb_res)
{
  const std::string full_string = error_string + mdb_strerror(mdb_res);
  return full_string;
}

inline void lmdb_db_open(MDB_txn* txn, const char* name, int flags, MDB_dbi& dbi, const std::string& error_string)
{
  if (auto res = mdb_dbi_open(txn, name, flags, &dbi))
    throw0(cryptonote::DB_OPEN_FAILURE((lmdb_error(error_string + " : ", res) + std::string(" - you may want to start with --db-salvage")).c_str()));
}


}  // anonymous namespace

#define CURSOR(name) \
	if (!m_cur_ ## name) { \
	  int result = mdb_cursor_open(*m_write_txn, m_ ## name, &m_cur_ ## name); \
	  if (result) \
        throw0(DB_ERROR(lmdb_error("Failed to open cursor: ", result).c_str())); \
	}

#define RCURSOR(name) \
	if (!m_cur_ ## name) { \
	  int result = mdb_cursor_open(m_txn, m_ ## name, (MDB_cursor **)&m_cur_ ## name); \
	  if (result) \
        throw0(DB_ERROR(lmdb_error("Failed to open cursor: ", result).c_str())); \
	  if (m_cursors != &m_wcursors) \
	    m_tinfo->m_ti_rflags.m_rf_ ## name = true; \
	} else if (m_cursors != &m_wcursors && !m_tinfo->m_ti_rflags.m_rf_ ## name) { \
	  int result = mdb_cursor_renew(m_txn, m_cur_ ## name); \
      if (result) \
        throw0(DB_ERROR(lmdb_error("Failed to renew cursor: ", result).c_str())); \
	  m_tinfo->m_ti_rflags.m_rf_ ## name = true; \
	}

namespace cryptonote
{

typedef struct mdb_block_info_1
{
  uint64_t bi_height;
  uint64_t bi_timestamp;
  uint64_t bi_coins;
  uint64_t bi_weight; // a size_t really but we need 32-bit compat
  uint64_t bi_diff;
  crypto::hash bi_hash;
} mdb_block_info_1;

typedef struct mdb_block_info_2
{
  uint64_t bi_height;
  uint64_t bi_timestamp;
  uint64_t bi_coins;
  uint64_t bi_weight; // a size_t really but we need 32-bit compat
  uint64_t bi_diff;
  crypto::hash bi_hash;
  uint64_t bi_cum_rct;
} mdb_block_info_2;

typedef struct mdb_block_info_3
{
  uint64_t bi_height;
  uint64_t bi_timestamp;
  uint64_t bi_coins;
  uint64_t bi_weight; // a size_t really but we need 32-bit compat
  uint64_t bi_diff;
  crypto::hash bi_hash;
  uint64_t bi_cum_rct;
  uint64_t bi_long_term_block_weight;
} mdb_block_info_3;

typedef struct mdb_block_info_4
{
  uint64_t bi_height;
  uint64_t bi_timestamp;
  uint64_t bi_coins;
  uint64_t bi_weight; // a size_t really but we need 32-bit compat
  uint64_t bi_diff_lo;
  uint64_t bi_diff_hi;
  crypto::hash bi_hash;
  uint64_t bi_cum_rct;
  uint64_t bi_long_term_block_weight;
} mdb_block_info_4;

typedef mdb_block_info_4 mdb_block_info;

typedef struct blk_height {
    crypto::hash bh_hash;
    uint64_t bh_height;
} blk_height;

typedef struct outtx {
    uint64_t output_id;
    crypto::hash tx_hash;
    uint64_t local_index;
} outtx;

typedef struct mdb_leaf {
    uint64_t leaf_idx;
    fcmp_pp::curve_trees::OutputContext output_context;
} mdb_leaf;

typedef struct layer_val {
    uint64_t child_chunk_idx;
    crypto::ec_point child_chunk_hash;
} layer_val;

typedef struct mdb_tree_meta {
    uint64_t n_leaf_tuples;
} mdb_tree_meta;

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

mdb_txn_safe::mdb_txn_safe(const bool check) : m_txn(NULL), m_tinfo(NULL), m_check(check)
{
  if (check)
  {
    while (creation_gate.test_and_set());
    num_active_txns++;
    creation_gate.clear();
  }
}

mdb_txn_safe::~mdb_txn_safe()
{
  if (!m_check)
    return;
  LOG_PRINT_L3("mdb_txn_safe: destructor");
  if (m_tinfo != nullptr)
  {
    mdb_txn_reset(m_tinfo->m_ti_rtxn);
    memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
  } else if (m_txn != nullptr)
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

void mdb_txn_safe::uncheck()
{
  num_active_txns--;
  m_check = false;
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
    throw0(DB_ERROR(lmdb_error(message + ": ", result).c_str()));
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

void mdb_txn_safe::increment_txns(int i)
{
	num_active_txns += i;
}

#define TXN_PREFIX(flags); \
  mdb_txn_safe auto_txn; \
  mdb_txn_safe* txn_ptr = &auto_txn; \
  if (m_batch_active) \
    txn_ptr = m_write_txn; \
  else \
  { \
    if (auto mdb_res = lmdb_txn_begin(m_env, NULL, flags, auto_txn)) \
      throw0(DB_ERROR(lmdb_error(std::string("Failed to create a transaction for the db in ")+__FUNCTION__+": ", mdb_res).c_str())); \
  } \

#define TXN_PREFIX_RDONLY() \
  MDB_txn *m_txn; \
  mdb_txn_cursors *m_cursors; \
  mdb_txn_safe auto_txn; \
  bool my_rtxn = block_rtxn_start(&m_txn, &m_cursors); \
  if (my_rtxn) auto_txn.m_tinfo = m_tinfo.get(); \
  else auto_txn.uncheck()
#define TXN_POSTFIX_RDONLY()

#define TXN_POSTFIX_SUCCESS() \
  do { \
    if (! m_batch_active) \
      auto_txn.commit(); \
  } while(0)

void lmdb_resized(MDB_env *env, int isactive)
{
  mdb_txn_safe::prevent_new_txns();

  MGINFO("LMDB map resize detected.");

  MDB_envinfo mei;

  mdb_env_info(env, &mei);
  uint64_t old = mei.me_mapsize;

  if (isactive)
    mdb_txn_safe::increment_txns(-1);
  mdb_txn_safe::wait_no_active_txns();
  if (isactive)
    mdb_txn_safe::increment_txns(1);

  int result = mdb_env_set_mapsize(env, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to set new mapsize: ", result).c_str()));

  mdb_env_info(env, &mei);
  uint64_t new_mapsize = mei.me_mapsize;

  MGINFO("LMDB Mapsize increased." << "  Old: " << old / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB");

  mdb_txn_safe::allow_new_txns();
}

inline int lmdb_txn_begin(MDB_env *env, MDB_txn *parent, unsigned int flags, MDB_txn **txn)
{
  int res = mdb_txn_begin(env, parent, flags, txn);
  if (res == MDB_MAP_RESIZED) {
    lmdb_resized(env, 1);
    res = mdb_txn_begin(env, parent, flags, txn);
  }
  return res;
}

inline int lmdb_txn_renew(MDB_txn *txn)
{
  int res = mdb_txn_renew(txn);
  if (res == MDB_MAP_RESIZED) {
    lmdb_resized(mdb_txn_env(txn), 0);
    res = mdb_txn_renew(txn);
  }
  return res;
}

inline void BlockchainLMDB::check_open() const
{
  if (!m_open)
    throw0(DB_ERROR("DB operation attempted on a not-open DB instance"));
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
      MERROR("!! WARNING: Insufficient free space to extend database !!: " <<
          (si.available >> 20L) << " MB available, " << (add_size >> 20L) << " MB needed");
      return;
    }
  }
  catch(...)
  {
    // print something but proceed.
    MWARNING("Unable to query free disk space.");
  }

  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  // add 1Gb per resize, instead of doing a percentage increase
  uint64_t new_mapsize = (uint64_t) mei.me_mapsize + add_size;

  // If given, use increase_size instead of above way of resizing.
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

  int result = mdb_env_set_mapsize(m_env, new_mapsize);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to set new mapsize: ", result).c_str()));

  MGINFO("LMDB Mapsize increased." << "  Old: " << mei.me_mapsize / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB");

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

  MDEBUG("DB map size:     " << mei.me_mapsize);
  MDEBUG("Space used:      " << size_used);
  MDEBUG("Space remaining: " << mei.me_mapsize - size_used);
  MDEBUG("Size threshold:  " << threshold_size);
  float resize_percent = RESIZE_PERCENT;
  MDEBUG(boost::format("Percent used: %.04f  Percent threshold: %.04f") % (100.*size_used/mei.me_mapsize) % (100.*resize_percent));

  if (threshold_size > 0)
  {
    if (mei.me_mapsize - size_used < threshold_size)
    {
      MINFO("Threshold met (size-based)");
      return true;
    }
    else
      return false;
  }

  if ((double)size_used / mei.me_mapsize  > resize_percent)
  {
    MINFO("Threshold met (percent-based)");
    return true;
  }
  return false;
#else
  return false;
#endif
}

void BlockchainLMDB::check_and_resize_for_batch(uint64_t batch_num_blocks, uint64_t batch_bytes)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  MTRACE("[" << __func__ << "] " << "checking DB size");
  const uint64_t min_increase_size = 512 * (1 << 20);
  uint64_t threshold_size = 0;
  uint64_t increase_size = 0;
  if (batch_num_blocks > 0)
  {
    threshold_size = get_estimated_batch_size(batch_num_blocks, batch_bytes);
    MDEBUG("calculated batch size: " << threshold_size);

    // The increased DB size could be a multiple of threshold_size, a fixed
    // size increase (> threshold_size), or other variations.
    //
    // Currently we use the greater of threshold size and a minimum size. The
    // minimum size increase is used to avoid frequent resizes when the batch
    // size is set to a very small numbers of blocks.
    increase_size = (threshold_size > min_increase_size) ? threshold_size : min_increase_size;
    MDEBUG("increase size: " << increase_size);
  }

  // if threshold_size is 0 (i.e. number of blocks for batch not passed in), it
  // will fall back to the percent-based threshold check instead of the
  // size-based check
  if (need_resize(threshold_size))
  {
    MGINFO("[batch] DB resize needed");
    do_resize(increase_size);
  }
}

uint64_t BlockchainLMDB::get_estimated_batch_size(uint64_t batch_num_blocks, uint64_t batch_bytes) const
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
  uint64_t m_height = height();
  if (m_height > 1)
    block_stop = m_height - 1;
  uint64_t block_start = 0;
  if (block_stop >= num_prev_blocks)
    block_start = block_stop - num_prev_blocks + 1;
  uint32_t num_blocks_used = 0;
  uint64_t total_block_size = 0;
  MDEBUG("[" << __func__ << "] " << "m_height: " << m_height << "  block_start: " << block_start << "  block_stop: " << block_stop);
  size_t avg_block_size = 0;
  if (batch_bytes)
  {
    avg_block_size = batch_bytes / batch_num_blocks;
    goto estim;
  }
  if (m_height == 0)
  {
    MDEBUG("No existing blocks to check for average block size");
  }
  else if (m_cum_count >= num_prev_blocks)
  {
    avg_block_size = m_cum_size / m_cum_count;
    MDEBUG("average block size across recent " << m_cum_count << " blocks: " << avg_block_size);
    m_cum_size = 0;
    m_cum_count = 0;
  }
  else
  {
    {
      TXN_PREFIX_RDONLY();
      for (uint64_t block_num = block_start; block_num <= block_stop; ++block_num)
      {
        // we have access to block weight, which will be greater or equal to block size,
        // so use this as a proxy. If it's too much off, we might have to check actual size,
        // which involves reading more data, so is not really wanted
        size_t block_weight = get_block_weight(block_num);
        total_block_size += block_weight;
        // Track number of blocks being totalled here instead of assuming, in case
        // some blocks were to be skipped for being outliers.
        ++num_blocks_used;
      }
    }
    avg_block_size = total_block_size / (num_blocks_used ? num_blocks_used : 1);
    MDEBUG("average block size across recent " << num_blocks_used << " blocks: " << avg_block_size);
  }
estim:
  if (avg_block_size < min_block_size)
    avg_block_size = min_block_size;
  MDEBUG("estimated average block size for batch: " << avg_block_size);

  // bigger safety margin on smaller block sizes
  if (batch_fudge_factor < 5000.0)
    batch_fudge_factor = 5000.0;
  threshold_size = avg_block_size * db_expand_factor * batch_fudge_factor;
  return threshold_size;
}

void BlockchainLMDB::add_block(const block& blk, size_t block_weight, uint64_t long_term_block_weight, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated,
    uint64_t num_rct_outs, const crypto::hash& blk_hash, const fcmp_pp::curve_trees::OutsByLastLockedBlock& outs_by_last_locked_block, const std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/>& timelocked_outputs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  uint64_t m_height = height();

  CURSOR(block_heights)
  blk_height bh = {blk_hash, m_height};
  MDB_val_set(val_h, bh);
  if (mdb_cursor_get(m_cur_block_heights, (MDB_val *)&zerokval, &val_h, MDB_GET_BOTH) == 0)
    throw1(BLOCK_EXISTS("Attempting to add block that's already in the db"));

  if (m_height > 0)
  {
    MDB_val_set(parent_key, blk.prev_id);
    int result = mdb_cursor_get(m_cur_block_heights, (MDB_val *)&zerokval, &parent_key, MDB_GET_BOTH);
    if (result)
    {
      LOG_PRINT_L3("m_height: " << m_height);
      LOG_PRINT_L3("parent_key: " << blk.prev_id);
      throw0(DB_ERROR(lmdb_error("Failed to get top block hash to check for new block's parent: ", result).c_str()));
    }
    blk_height *prev = (blk_height *)parent_key.mv_data;
    if (prev->bh_height != m_height - 1)
      throw0(BLOCK_PARENT_DNE("Top block is not new block's parent"));
  }

  int result = 0;

  MDB_val_set(key, m_height);

  CURSOR(blocks)
  CURSOR(block_info)

  // this call to mdb_cursor_put will change height()
  cryptonote::blobdata block_blob(block_to_blob(blk));
  MDB_val_sized(blob, block_blob);
  result = mdb_cursor_put(m_cur_blocks, &key, &blob, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add block blob to db transaction: ", result).c_str()));

  mdb_block_info bi;
  bi.bi_height = m_height;
  bi.bi_timestamp = blk.timestamp;
  bi.bi_coins = coins_generated;
  bi.bi_weight = block_weight;
  bi.bi_diff_hi = ((cumulative_difficulty >> 64) & 0xffffffffffffffff).convert_to<uint64_t>();
  bi.bi_diff_lo = (cumulative_difficulty & 0xffffffffffffffff).convert_to<uint64_t>();
  bi.bi_hash = blk_hash;
  bi.bi_cum_rct = num_rct_outs;
  if (blk.major_version >= 4)
  {
    uint64_t last_height = m_height-1;
    MDB_val_set(h, last_height);
    if ((result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &h, MDB_GET_BOTH)))
        throw1(BLOCK_DNE(lmdb_error("Failed to get block info: ", result).c_str()));
    const mdb_block_info *bi_prev = (const mdb_block_info*)h.mv_data;
    bi.bi_cum_rct += bi_prev->bi_cum_rct;
  }
  bi.bi_long_term_block_weight = long_term_block_weight;

  MDB_val_set(val, bi);
  result = mdb_cursor_put(m_cur_block_info, (MDB_val *)&zerokval, &val, MDB_APPENDDUP);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add block info to db transaction: ", result).c_str()));

  result = mdb_cursor_put(m_cur_block_heights, (MDB_val *)&zerokval, &val_h, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add block height by hash to db transaction: ", result).c_str()));

  CURSOR(locked_outputs)
  CURSOR(timelocked_outputs)

  // Add the locked outputs from this block to the locked outputs and custom timelocked tables
  for (const auto &last_locked_block : outs_by_last_locked_block)
  {
    const uint64_t last_locked_block_idx = last_locked_block.first;
    for (const auto &locked_output : last_locked_block.second)
    {
      MDB_val_set(k_block_id, last_locked_block_idx);
      MDB_val_set(v_output, locked_output);
      result = mdb_cursor_put(m_cur_locked_outputs, &k_block_id, &v_output, MDB_APPENDDUP);
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to add locked output: ", result).c_str()));

      if (timelocked_outputs.find(locked_output.output_id) != timelocked_outputs.end())
      {
        MDB_val_set(k_timelocked_block_id, last_locked_block_idx);
        MDB_val_set(v_timelocked_output, locked_output);
        result = mdb_cursor_put(m_cur_timelocked_outputs, &k_timelocked_block_id, &v_timelocked_output, MDB_APPENDDUP);
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Failed to add timelocked output: ", result).c_str()));
      }
    }
  }

  // we use weight as a proxy for size, since we don't have size but weight is >= size
  // and often actually equal
  m_cum_size += block_weight;
  m_cum_count++;
}

void BlockchainLMDB::remove_block()
{
  int result;

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  uint64_t m_height = height();

  if (m_height == 0)
    throw0(BLOCK_DNE ("Attempting to remove block from an empty blockchain"));

  mdb_txn_cursors *m_cursors = &m_wcursors;
  CURSOR(block_info)
  CURSOR(block_heights)
  CURSOR(blocks)

  this->trim_block();

  MDB_val_copy<uint64_t> k(m_height - 1);
  MDB_val h = k;
  if ((result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &h, MDB_GET_BOTH)))
      throw1(BLOCK_DNE(lmdb_error("Attempting to remove block that's not in the db: ", result).c_str()));

  // must use h now; deleting from m_block_info will invalidate it
  mdb_block_info *bi = (mdb_block_info *)h.mv_data;
  blk_height bh = {bi->bi_hash, 0};
  h.mv_data = (void *)&bh;
  h.mv_size = sizeof(bh);
  if ((result = mdb_cursor_get(m_cur_block_heights, (MDB_val *)&zerokval, &h, MDB_GET_BOTH)))
      throw1(DB_ERROR(lmdb_error("Failed to locate block height by hash for removal: ", result).c_str()));
  if ((result = mdb_cursor_del(m_cur_block_heights, 0)))
      throw1(DB_ERROR(lmdb_error("Failed to add removal of block height by hash to db transaction: ", result).c_str()));

  if ((result = mdb_cursor_del(m_cur_blocks, 0)))
      throw1(DB_ERROR(lmdb_error("Failed to add removal of block to db transaction: ", result).c_str()));

  if ((result = mdb_cursor_del(m_cur_block_info, 0)))
      throw1(DB_ERROR(lmdb_error("Failed to add removal of block info to db transaction: ", result).c_str()));
}

uint64_t BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const epee::span<const std::uint8_t> blob, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  uint64_t m_height = height();

  int result;
  uint64_t tx_id = get_tx_count();

  CURSOR(txs_pruned)
  CURSOR(txs_prunable)
  CURSOR(txs_prunable_hash)
  CURSOR(txs_prunable_tip)
  CURSOR(tx_indices)

  MDB_val_set(val_tx_id, tx_id);
  MDB_val_set(val_h, tx_hash);
  result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &val_h, MDB_GET_BOTH);
  if (result == 0) {
    txindex *tip = (txindex *)val_h.mv_data;
    throw1(TX_EXISTS(std::string("Attempting to add transaction that's already in the db (tx id ").append(boost::lexical_cast<std::string>(tip->data.tx_id)).append(")").c_str()));
  } else if (result != MDB_NOTFOUND) {
    throw1(DB_ERROR(lmdb_error(std::string("Error checking if tx index exists for tx hash ") + epee::string_tools::pod_to_hex(tx_hash) + ": ", result).c_str()));
  }

  txindex ti;
  ti.key = tx_hash;
  ti.data.tx_id = tx_id;
  ti.data.unlock_time = tx.unlock_time;
  ti.data.block_id = m_height;  // we don't need blk_hash since we know m_height

  val_h.mv_size = sizeof(ti);
  val_h.mv_data = (void *)&ti;

  result = mdb_cursor_put(m_cur_tx_indices, (MDB_val *)&zerokval, &val_h, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add tx data to db transaction: ", result).c_str()));

  unsigned int unprunable_size = tx.unprunable_size;
  if (unprunable_size == 0)
  {
    std::stringstream ss;
    binary_archive<true> ba(ss);
    bool r = const_cast<cryptonote::transaction&>(tx).serialize_base(ba);
    if (!r)
      throw0(DB_ERROR("Failed to serialize pruned tx"));
    unprunable_size = ss.str().size();
  }

  if (unprunable_size > blob.size())
    throw0(DB_ERROR("pruned tx size is larger than tx size"));

  MDB_val pruned_blob = {unprunable_size, (void*)blob.data()};
  result = mdb_cursor_put(m_cur_txs_pruned, &val_tx_id, &pruned_blob, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add pruned tx blob to db transaction: ", result).c_str()));

  MDB_val prunable_blob = {blob.size() - unprunable_size, (void*)(blob.data() + unprunable_size)};
  result = mdb_cursor_put(m_cur_txs_prunable, &val_tx_id, &prunable_blob, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add prunable tx blob to db transaction: ", result).c_str()));

  if (get_blockchain_pruning_seed())
  {
    MDB_val_set(val_height, m_height);
    result = mdb_cursor_put(m_cur_txs_prunable_tip, &val_tx_id, &val_height, 0);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to add prunable tx id to db transaction: ", result).c_str()));
  }

  if (tx.version > 1)
  {
    MDB_val_set(val_prunable_hash, tx_prunable_hash);
    result = mdb_cursor_put(m_cur_txs_prunable_hash, &val_tx_id, &val_prunable_hash, MDB_APPEND);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to add prunable tx prunable hash to db transaction: ", result).c_str()));
  }

  return tx_id;
}

// TODO: compare pros and cons of looking up the tx hash's tx index once and
// passing it in to functions like this
void BlockchainLMDB::remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx)
{
  int result;

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_cursors *m_cursors = &m_wcursors;
  CURSOR(tx_indices)
  CURSOR(txs_pruned)
  CURSOR(txs_prunable)
  CURSOR(txs_prunable_hash)
  CURSOR(txs_prunable_tip)
  CURSOR(tx_outputs)

  MDB_val_set(val_h, tx_hash);

  if (mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &val_h, MDB_GET_BOTH))
      throw1(TX_DNE("Attempting to remove transaction that isn't in the db"));
  txindex *tip = (txindex *)val_h.mv_data;
  MDB_val_set(val_tx_id, tip->data.tx_id);

  if ((result = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, NULL, MDB_SET)))
      throw1(DB_ERROR(lmdb_error("Failed to locate pruned tx for removal: ", result).c_str()));
  result = mdb_cursor_del(m_cur_txs_pruned, 0);
  if (result)
      throw1(DB_ERROR(lmdb_error("Failed to add removal of pruned tx to db transaction: ", result).c_str()));

  result = mdb_cursor_get(m_cur_txs_prunable, &val_tx_id, NULL, MDB_SET);
  if (result == 0)
  {
      result = mdb_cursor_del(m_cur_txs_prunable, 0);
      if (result)
          throw1(DB_ERROR(lmdb_error("Failed to add removal of prunable tx to db transaction: ", result).c_str()));
  }
  else if (result != MDB_NOTFOUND)
      throw1(DB_ERROR(lmdb_error("Failed to locate prunable tx for removal: ", result).c_str()));

  result = mdb_cursor_get(m_cur_txs_prunable_tip, &val_tx_id, NULL, MDB_SET);
  if (result && result != MDB_NOTFOUND)
      throw1(DB_ERROR(lmdb_error("Failed to locate tx id for removal: ", result).c_str()));
  if (result == 0)
  {
    result = mdb_cursor_del(m_cur_txs_prunable_tip, 0);
    if (result)
        throw1(DB_ERROR(lmdb_error("Error adding removal of tx id to db transaction", result).c_str()));
  }

  if (tx.version > 1)
  {
    if ((result = mdb_cursor_get(m_cur_txs_prunable_hash, &val_tx_id, NULL, MDB_SET)))
        throw1(DB_ERROR(lmdb_error("Failed to locate prunable hash tx for removal: ", result).c_str()));
    result = mdb_cursor_del(m_cur_txs_prunable_hash, 0);
    if (result)
        throw1(DB_ERROR(lmdb_error("Failed to add removal of prunable hash tx to db transaction: ", result).c_str()));
  }

  remove_tx_outputs(tip->data.tx_id, tx);

  result = mdb_cursor_get(m_cur_tx_outputs, &val_tx_id, NULL, MDB_SET);
  if (result == MDB_NOTFOUND)
    LOG_PRINT_L1("tx has no outputs to remove: " << tx_hash);
  else if (result)
    throw1(DB_ERROR(lmdb_error("Failed to locate tx outputs for removal: ", result).c_str()));
  if (!result)
  {
    result = mdb_cursor_del(m_cur_tx_outputs, 0);
    if (result)
      throw1(DB_ERROR(lmdb_error("Failed to add removal of tx outputs to db transaction: ", result).c_str()));
  }

  // Don't delete the tx_indices entry until the end, after we're done with val_tx_id
  if (mdb_cursor_del(m_cur_tx_indices, 0))
      throw1(DB_ERROR("Failed to add removal of tx index to db transaction"));
}

uint64_t BlockchainLMDB::add_output(const crypto::hash& tx_hash,
    const tx_out& tx_output,
    const uint64_t& local_index,
    const uint64_t unlock_time,
    const rct::key *commitment)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  uint64_t m_height = height();
  uint64_t m_num_outputs = num_outputs();

  int result = 0;

  CURSOR(output_txs)
  CURSOR(output_amounts)

  crypto::public_key output_public_key;
  if (!get_output_public_key(tx_output, output_public_key))
    throw0(DB_ERROR("Could not get an output public key from a tx output."));
  if (tx_output.amount == 0 && !commitment)
    throw0(DB_ERROR("RCT output without commitment"));

  outtx ot = {m_num_outputs, tx_hash, local_index};
  MDB_val_set(vot, ot);

  result = mdb_cursor_put(m_cur_output_txs, (MDB_val *)&zerokval, &vot, MDB_APPENDDUP);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to add output tx hash to db transaction: ", result).c_str()));

  outkey ok;
  MDB_val data;
  MDB_val_copy<uint64_t> val_amount(tx_output.amount);
  result = mdb_cursor_get(m_cur_output_amounts, &val_amount, &data, MDB_SET);
  if (!result)
    {
      mdb_size_t num_elems = 0;
      result = mdb_cursor_count(m_cur_output_amounts, &num_elems);
      if (result)
        throw0(DB_ERROR(std::string("Failed to get number of outputs for amount: ").append(mdb_strerror(result)).c_str()));
      ok.amount_index = num_elems;
    }
  else if (result != MDB_NOTFOUND)
    throw0(DB_ERROR(lmdb_error("Failed to get output amount in db transaction: ", result).c_str()));
  else
    ok.amount_index = 0;
  ok.output_id = m_num_outputs;
  ok.data.pubkey = output_public_key;
  ok.data.unlock_time = unlock_time;
  ok.data.height = m_height;
  if (tx_output.amount == 0)
  {
    ok.data.commitment = *commitment;
    data.mv_size = sizeof(ok);
  }
  else
  {
    data.mv_size = sizeof(pre_rct_outkey);
  }
  data.mv_data = &ok;

  if ((result = mdb_cursor_put(m_cur_output_amounts, &val_amount, &data, MDB_APPENDDUP)))
      throw0(DB_ERROR(lmdb_error("Failed to add output pubkey to db transaction: ", result).c_str()));

  return ok.amount_index;
}

void BlockchainLMDB::add_tx_amount_output_indices(const uint64_t tx_id,
    const std::vector<uint64_t>& amount_output_indices)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  CURSOR(tx_outputs)

  int result = 0;

  size_t num_outputs = amount_output_indices.size();

  MDB_val_set(k_tx_id, tx_id);
  MDB_val v;
  v.mv_data = num_outputs ? (void *)amount_output_indices.data() : (void*)"";
  v.mv_size = sizeof(uint64_t) * num_outputs;
  // LOG_PRINT_L1("tx_outputs[tx_hash] size: " << v.mv_size);

  result = mdb_cursor_put(m_cur_tx_outputs, &k_tx_id, &v, MDB_APPEND);
  if (result)
    throw0(DB_ERROR(std::string("Failed to add <tx hash, amount output index array> to db transaction: ").append(mdb_strerror(result)).c_str()));
}

void BlockchainLMDB::remove_tx_outputs(const uint64_t tx_id, const transaction& tx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  std::vector<std::vector<uint64_t>> amount_output_indices_set = get_tx_amount_output_indices(tx_id, 1);
  const std::vector<uint64_t> &amount_output_indices = amount_output_indices_set.front();

  if (amount_output_indices.empty())
  {
    if (tx.vout.empty())
      LOG_PRINT_L2("tx has no outputs, so no output indices");
    else
      throw0(DB_ERROR("tx has outputs, but no output indices found"));
  }

  bool is_pseudo_rct = tx.version >= 2 && tx.vin.size() == 1 && tx.vin[0].type() == typeid(txin_gen);
  for (size_t i = tx.vout.size(); i-- > 0;)
  {
    uint64_t amount = is_pseudo_rct ? 0 : tx.vout[i].amount;
    remove_output(amount, amount_output_indices[i]);
  }
}

void BlockchainLMDB::remove_output(const uint64_t amount, const uint64_t& out_index)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  CURSOR(output_amounts);
  CURSOR(output_txs);

  MDB_val_set(k, amount);
  MDB_val_set(v, out_index);

  auto result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_BOTH);
  if (result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("Attempting to get an output index by amount and amount index, but amount not found"));
  else if (result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to get an output", result).c_str()));

  const pre_rct_outkey *ok = (const pre_rct_outkey *)v.mv_data;
  MDB_val_set(otxk, ok->output_id);
  result = mdb_cursor_get(m_cur_output_txs, (MDB_val *)&zerokval, &otxk, MDB_GET_BOTH);
  if (result == MDB_NOTFOUND)
  {
    throw0(DB_ERROR("Unexpected: global output index not found in m_output_txs"));
  }
  else if (result)
  {
    throw1(DB_ERROR(lmdb_error("Error adding removal of output tx to db transaction", result).c_str()));
  }

  // Remove output from locked outputs table if present. We expect all valid
  // outputs to be in the locked outputs table because remove_output is called
  // when removing the top block from the chain, and all outputs from the top
  // block are expected to be locked until they are at least 10 blocks old (10
  // is the lower bound). An output might not be in the locked outputs table if
  // it is invalid, then gets removed from the locked outputs table upon growing
  // the tree.
  // TODO: test case where we add an invalid output to the chain, grow the tree
  // in the block in which that output unlocks, pop blocks to remove that output
  // from the chain, then progress the chain again.
  CURSOR(locked_outputs);

  const uint64_t last_locked_block = cryptonote::get_last_locked_block_index(ok->data.unlock_time, ok->data.height);

  MDB_val_set(k_block_id, last_locked_block);
  MDB_val_set(v_output, ok->output_id);

  result = mdb_cursor_get(m_cur_locked_outputs, &k_block_id, &v_output, MDB_GET_BOTH);
  if (result == MDB_NOTFOUND)
  {
    // We expect this output is invalid
  }
  else if (result)
  {
    throw1(DB_ERROR(lmdb_error("Error adding removal of locked output to db transaction", result).c_str()));
  }
  else
  {
    result = mdb_cursor_del(m_cur_locked_outputs, 0);
    if (result)
      throw0(DB_ERROR(lmdb_error(std::string("Error deleting locked output index ").append(boost::lexical_cast<std::string>(out_index).append(": ")).c_str(), result).c_str()));
  }

  // Remove output from custom timelocked outputs table if present
  CURSOR(timelocked_outputs);

  MDB_val_set(k_timelocked_block_id, last_locked_block);
  MDB_val_set(v_timelocked_output, ok->output_id);

  result = mdb_cursor_get(m_cur_timelocked_outputs, &k_timelocked_block_id, &v_timelocked_output, MDB_GET_BOTH);
  if (result == MDB_NOTFOUND)
  {
    // Output is either not timelocked or is invalid
  }
  else if (result)
  {
    throw1(DB_ERROR(lmdb_error("Error adding removal of timelocked output to db transaction", result).c_str()));
  }
  else
  {
    result = mdb_cursor_del(m_cur_timelocked_outputs, 0);
    if (result)
      throw0(DB_ERROR(lmdb_error(std::string("Error deleting timelocked output index ").append(boost::lexical_cast<std::string>(out_index).append(": ")).c_str(), result).c_str()));
  }

  result = mdb_cursor_del(m_cur_output_txs, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error(std::string("Error deleting output index ").append(boost::lexical_cast<std::string>(out_index).append(": ")).c_str(), result).c_str()));

  // now delete the amount
  result = mdb_cursor_del(m_cur_output_amounts, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error(std::string("Error deleting amount for output index ").append(boost::lexical_cast<std::string>(out_index).append(": ")).c_str(), result).c_str()));
}

void BlockchainLMDB::prune_outputs(uint64_t amount)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;
  CURSOR(output_amounts);
  CURSOR(output_txs);

  MINFO("Pruning outputs for amount " << amount);

  MDB_val v;
  MDB_val_set(k, amount);
  int result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    return;
  if (result)
    throw0(DB_ERROR(lmdb_error("Error looking up outputs: ", result).c_str()));

  // gather output ids
  mdb_size_t num_elems;
  mdb_cursor_count(m_cur_output_amounts, &num_elems);
  MINFO(num_elems << " outputs found");
  std::vector<uint64_t> output_ids;
  output_ids.reserve(num_elems);
  while (1)
  {
    const pre_rct_outkey *okp = (const pre_rct_outkey *)v.mv_data;
    output_ids.push_back(okp->output_id);
    MDEBUG("output id " << okp->output_id);
    result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_NEXT_DUP);
    if (result == MDB_NOTFOUND)
      break;
    if (result)
      throw0(DB_ERROR(lmdb_error("Error counting outputs: ", result).c_str()));
  }
  if (output_ids.size() != num_elems)
    throw0(DB_ERROR("Unexpected number of outputs"));

  result = mdb_cursor_del(m_cur_output_amounts, MDB_NODUPDATA);
  if (result)
    throw0(DB_ERROR(lmdb_error("Error deleting outputs: ", result).c_str()));

  for (uint64_t output_id: output_ids)
  {
    MDB_val_set(v, output_id);
    result = mdb_cursor_get(m_cur_output_txs, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
    if (result)
      throw0(DB_ERROR(lmdb_error("Error looking up output: ", result).c_str()));
    result = mdb_cursor_del(m_cur_output_txs, 0);
    if (result)
      throw0(DB_ERROR(lmdb_error("Error deleting output: ", result).c_str()));
  }
}

void BlockchainLMDB::add_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(spent_keys)

  MDB_val k = {sizeof(k_image), (void *)&k_image};
  if (auto result = mdb_cursor_put(m_cur_spent_keys, (MDB_val *)&zerokval, &k, MDB_NODUPDATA)) {
    if (result == MDB_KEYEXIST)
      throw1(KEY_IMAGE_EXISTS("Attempting to add spent key image that's already in the db"));
    else
      throw1(DB_ERROR(lmdb_error("Error adding spent key image to db transaction: ", result).c_str()));
  }
}

void BlockchainLMDB::remove_spent_key(const crypto::key_image& k_image)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(spent_keys)

  MDB_val k = {sizeof(k_image), (void *)&k_image};
  auto result = mdb_cursor_get(m_cur_spent_keys, (MDB_val *)&zerokval, &k, MDB_GET_BOTH);
  if (result != 0 && result != MDB_NOTFOUND)
      throw1(DB_ERROR(lmdb_error("Error finding spent key to remove", result).c_str()));
  if (!result)
  {
    result = mdb_cursor_del(m_cur_spent_keys, 0);
    if (result)
        throw1(DB_ERROR(lmdb_error("Error adding removal of key image to db transaction", result).c_str()));
  }
}

void BlockchainLMDB::advance_tree(const uint64_t blk_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // Get the earliest possible last locked block of outputs created in blk_idx
  const uint64_t earliest_last_locked_block = cryptonote::get_default_last_locked_block_index(blk_idx);

  // If we're advancing the genesis block, make sure to initialize the tree
  if (blk_idx == 0)
  {
    // TODO: include a pre-check that tree meta is empty

    // We grow the first blocks with empty outputs, since no outputs in this range should be spendable yet
    for (uint64_t new_blk_idx = blk_idx; new_blk_idx < earliest_last_locked_block; ++new_blk_idx)
    {
      this->grow_tree(new_blk_idx, {});
    }
  }
  // TODO: include a pre-check that earliest_last_locked_block == last block idx + 1 in tree meta (when blk_idx != 0)

  // Now we can advance the tree 1 block
  auto unlocked_outputs = this->get_outs_at_last_locked_block_idx(earliest_last_locked_block);

  // Grow the tree with outputs that are spendable once the earliest_last_locked_block is in the chain
  this->grow_tree(earliest_last_locked_block, std::move(unlocked_outputs));

  // Now that we've used the unlocked leaves to grow the tree, we delete them from the locked outputs table
  this->del_locked_outs_at_block_idx(earliest_last_locked_block);
}

void BlockchainLMDB::grow_tree(const uint64_t blk_idx, std::vector<fcmp_pp::curve_trees::OutputContext> &&new_outputs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CHECK_AND_ASSERT_THROW_MES(m_write_txn != nullptr, "Must have m_write_txn set to grow tree");
  CHECK_AND_ASSERT_THROW_MES(m_curve_trees != nullptr, "curve trees must be set");

  CURSOR(leaves)

  MDEBUG("Growing tree usable once block " << blk_idx << " is in the chain");

  // Get the prev block's tree edge (i.e. the current tree edge before growing)
  std::vector<crypto::ec_point> prev_tree_edge;
  uint64_t prev_blk_idx = 0;
  if (blk_idx > 0)
  {
    prev_blk_idx = blk_idx - 1;

    // Make sure tree tip lines up to expected block
    const uint64_t tree_block_idx = this->get_tree_block_idx();
    if (tree_block_idx != prev_blk_idx)
    {
      throw0(DB_ERROR(("Unexpected tree block idx mismatch to prev block ("
          + std::to_string(tree_block_idx) + " vs " + std::to_string(prev_blk_idx) + ")").c_str()));
    }

    prev_tree_edge = this->get_tree_edge(prev_blk_idx);
  }

  // Get the number of leaf tuples that exist in the current tree
  const uint64_t old_n_leaf_tuples = this->get_n_leaf_tuples();

  if (blk_idx == 0 && old_n_leaf_tuples != 0)
    throw0(DB_ERROR("Tree is not empty at blk idx 0"));

  // We re-save the prev tree edge at this next block if the tree doesn't grow
  const auto save_prev_tree_edge = [&, this]() { this->save_tree_meta(blk_idx, old_n_leaf_tuples, prev_tree_edge); };
  if (new_outputs.empty())
  {
    save_prev_tree_edge();
    return;
  }

  // Set the tree's existing last hashes from the existing edge
  const auto last_hashes = m_curve_trees->tree_edge_to_last_hashes(prev_tree_edge);

  // Use the number of leaf tuples and the existing last hashes to get a struct we can use to extend the tree
  auto tree_extension = m_curve_trees->get_tree_extension(old_n_leaf_tuples, last_hashes, {std::move(new_outputs)}, false/*use_fast_torsion_check*/);
  if (tree_extension.leaves.tuples.empty())
  {
    save_prev_tree_edge();
    return;
  }

  // Insert the leaves
  // TODO: grow_leaves
  auto &leaves = tree_extension.leaves;
  const uint64_t new_n_leaf_tuples = leaves.tuples.size() + old_n_leaf_tuples;
  for (uint64_t i = 0; i < leaves.tuples.size(); ++i)
  {
    const uint64_t leaf_idx = i + leaves.start_leaf_tuple_idx;
    mdb_leaf val{.leaf_idx = leaf_idx, .output_context = std::move(leaves.tuples[i])};
    MDB_val_set(v, val);

    int result = mdb_cursor_put(m_cur_leaves, (MDB_val *)&zerokval, &v, MDB_APPENDDUP);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to add leaf: ", result).c_str()));
  }

  // Grow the layers
  // TODO: grow_layers
  const auto &c1_extensions = tree_extension.c1_layer_extensions;
  const auto &c2_extensions = tree_extension.c2_layer_extensions;
  const std::size_t n_layers = c1_extensions.size() + c2_extensions.size();
  if (n_layers == 0)
    throw0(DB_ERROR("Unexpected 0 n layers"));

  bool parent_is_c1 = true;
  uint64_t c1_idx = 0;
  uint64_t c2_idx = 0;
  std::vector<crypto::ec_point> tree_edge;
  tree_edge.reserve(n_layers);
  for (uint64_t i = 0; i < n_layers; ++i)
  {
    const uint64_t layer_idx = c1_idx + c2_idx;
    MTRACE("Growing layer " << layer_idx);

    if (parent_is_c1)
    {
      if (layer_idx % 2 != 0)
        throw0(DB_ERROR(("Growing odd c1 layer, expected even layer idx for c1: "
          + std::to_string(layer_idx)).c_str()));

      tree_edge.emplace_back(this->grow_layer<fcmp_pp::curve_trees::Selene>(
          m_curve_trees->m_c1,
          c1_extensions,
          c1_idx,
          layer_idx
        ));

      ++c1_idx;
    }
    else
    {
      if (layer_idx % 2 == 0)
        throw0(DB_ERROR(("Growing even c2 layer, expected odd layer idx for c2: "
          + std::to_string(layer_idx)).c_str()));

      tree_edge.emplace_back(this->grow_layer<fcmp_pp::curve_trees::Helios>(
          m_curve_trees->m_c2,
          c2_extensions,
          c2_idx,
          layer_idx
        ));

      ++c2_idx;
    }

    parent_is_c1 = !parent_is_c1;
  }

  this->save_tree_meta(blk_idx, new_n_leaf_tuples, tree_edge);
}

template<typename C>
crypto::ec_point BlockchainLMDB::grow_layer(const std::unique_ptr<C> &curve,
  const std::vector<fcmp_pp::curve_trees::LayerExtension<C>> &layer_extensions,
  const uint64_t ext_idx,
  const uint64_t layer_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(layers)

  CHECK_AND_ASSERT_THROW_MES(ext_idx < layer_extensions.size(), "unexpected layer extension");
  const auto &ext = layer_extensions[ext_idx];

  CHECK_AND_ASSERT_THROW_MES(!ext.hashes.empty(), "empty layer extension");
  std::vector<crypto::ec_point> hashes;
  hashes.reserve(ext.hashes.size());

  // TODO: make sure ext.start_idx lines up with the end of the layer

  MDB_val_copy<uint64_t> k(layer_idx);

  // 1. Update the existing last hash if necessary
  if (ext.update_existing_last_hash)
  {
    hashes.emplace_back(curve->to_bytes(ext.hashes.front()));

    // We updated the last hash, so update it
    layer_val lv;
    lv.child_chunk_idx  = ext.start_idx;
    lv.child_chunk_hash = hashes.back();
    MDB_val_set(v, lv);

    // We expect to overwrite the existing hash
    // TODO: make sure the hash already exists and is the existing last hash
    int result = mdb_cursor_put(m_cur_layers, &k, &v, 0);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to update chunk hash: ", result).c_str()));
  }

  // 2. Add all the new hashes found in the extension
  for (uint64_t i = ext.update_existing_last_hash ? 1 : 0; i < ext.hashes.size(); ++i)
  {
    hashes.emplace_back(curve->to_bytes(ext.hashes[i]));

    layer_val lv;
    lv.child_chunk_idx  = i + ext.start_idx;
    lv.child_chunk_hash = hashes.back();
    MDB_val_set(v, lv);

    int result = mdb_cursor_put(m_cur_layers, &k, &v, MDB_APPENDDUP);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to add hash: ", result).c_str()));
  }

  return hashes.back();
}

void BlockchainLMDB::save_tree_meta(const uint64_t block_idx, const uint64_t n_leaf_tuples, const std::vector<crypto::ec_point> &tree_edge)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(tree_edges);
  CURSOR(tree_meta);

  if (tree_edge.size() > std::numeric_limits<uint8_t>::max())
    throw0(DB_ERROR("too many tree edge members"));
  if (tree_edge.size() != m_curve_trees->n_layers(n_leaf_tuples))
    throw0(DB_ERROR("Number of layers in the tree edge does not match expected for the saved n leaf tuples"));

  // 1. Save the tree edge
  MDB_val_copy<uint64_t> k_tee(block_idx);
  MDB_val v;
  const std::size_t n_layers = tree_edge.size();
  v.mv_data = n_layers ? (void *)tree_edge.data() : (void*)"";
  v.mv_size = sizeof(crypto::ec_point) * n_layers;

  int result = mdb_cursor_put(m_cur_tree_edges, &k_tee, &v, MDB_NOOVERWRITE);
  if (result != MDB_SUCCESS)
    throw0(DB_ERROR(lmdb_error("Failed to set last hash: ", result).c_str()));

  // 2. Save the tree meta
  MDB_val_copy<uint64_t> k_meta(block_idx);
  mdb_tree_meta tree_meta;
  tree_meta.n_leaf_tuples = n_leaf_tuples;
  MDB_val_set(v_meta, tree_meta);

  MDEBUG("Saving tree meta for block idx " << block_idx << " (n_leaf_tuples=" << n_leaf_tuples
    << ", root=" << (tree_edge.size() ? tree_edge.back() : crypto::ec_point{}) << ")");

  result = mdb_cursor_put(m_cur_tree_meta, &k_meta, &v_meta, MDB_NOOVERWRITE);
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error setting tree meta: ", result).c_str()));
}

void BlockchainLMDB::del_tree_meta(const uint64_t block_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(tree_edges)
  CURSOR(tree_meta)

  CHECK_AND_ASSERT_THROW_MES(m_curve_trees != nullptr, "curve trees must be set");

  MDB_val_set(k_block_id, block_idx);

  // 1. Delete tree edge at this block
  int result = mdb_cursor_get(m_cur_tree_edges, &k_block_id, NULL, MDB_SET);
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error finding tree edge to remove at block " + std::to_string(block_idx) + ": ", result).c_str()));
  result = mdb_cursor_del(m_cur_tree_edges, 0);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error removing tree edge: ", result).c_str()));

  // 2. Delete tree meta at this block
  result = mdb_cursor_get(m_cur_tree_meta, &k_block_id, NULL, MDB_SET);
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error finding tree meta to remove at block " + std::to_string(block_idx) + ": ", result).c_str()));
  result = mdb_cursor_del(m_cur_tree_meta, 0);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error removing tree meta: ", result).c_str()));
}

std::pair<uint64_t, fcmp_pp::curve_trees::PathBytes> BlockchainLMDB::get_last_path(const uint64_t block_idx) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  // See how many leaves were in the tree at the given block
  const uint64_t block_n_leaf_tuples = this->get_block_n_leaf_tuples(block_idx);
  if (block_n_leaf_tuples == 0)
    return { 0, {} };

  // Get path elems from the block's last path (gets *current* path elem state)
  const auto last_path_indexes = m_curve_trees->get_path_indexes(block_n_leaf_tuples, block_n_leaf_tuples - 1);
  auto path = this->get_path(last_path_indexes);

  // See what the last hashes at every layer for the provided block were (gets *old* path elem state)
  std::vector<crypto::ec_point> tree_edge = this->get_tree_edge(block_idx);
  if (tree_edge.empty())
    throw0(DB_ERROR("Unexpected empty tree edge"));
  if (tree_edge.size() != path.layer_chunks.size())
    throw0(DB_ERROR("Unexpected tree edge size, mismatch to path layer chunks"));

  // Use tree edge at the provided block to set the last hash for each layer (so path state reflects old state)
  for (std::size_t i = 0; i < path.layer_chunks.size(); ++i)
  {
    if (path.layer_chunks[i].chunk_bytes.empty())
      throw0(DB_ERROR("Unexpected empty path"));
    path.layer_chunks[i].chunk_bytes.back() = std::move(tree_edge[i]);
  }

  TXN_POSTFIX_RDONLY();

  return { block_n_leaf_tuples, path };
}

std::vector<crypto::ec_point> BlockchainLMDB::get_tree_edge(uint64_t block_id) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tree_edges)

  MDB_val_set(k_block_id, block_id);
  MDB_val v;
  int result = mdb_cursor_get(m_cur_tree_edges, &k_block_id, &v, MDB_SET);
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error finding tree edge at block " + std::to_string(block_id) + ": ", result).c_str()));

  crypto::ec_point* tree_edge = (crypto::ec_point*)v.mv_data;
  const std::size_t n_layers = v.mv_size / sizeof(crypto::ec_point);

  std::vector<crypto::ec_point> res;
  res.reserve(n_layers);
  for (std::size_t i = 0; i < n_layers; ++i)
    res.emplace_back(std::move(tree_edge[i]));

  TXN_POSTFIX_RDONLY();

  return res;
}

fcmp_pp::curve_trees::PathBytes BlockchainLMDB::get_path(const fcmp_pp::curve_trees::PathIndexes &path_indexes) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(leaves)
  RCURSOR(layers)

  fcmp_pp::curve_trees::PathBytes path_bytes;

  auto &leaves_out = path_bytes.leaves;
  auto &layer_chunks_out = path_bytes.layer_chunks;

  // Get the leaves
  // TODO: separate function for leaves
  {
    if (path_indexes.leaf_range.second > path_indexes.leaf_range.first)
    {
      leaves_out.reserve(path_indexes.leaf_range.second - path_indexes.leaf_range.first);

      uint64_t idx = path_indexes.leaf_range.first;

      MDB_val k = zerokval;
      MDB_val_copy<uint64_t> v(idx);

      MDB_cursor_op leaf_op = MDB_GET_BOTH;
      do
      {
        int result = mdb_cursor_get(m_cur_leaves, &k, &v, leaf_op);
        leaf_op = MDB_NEXT;
        if (result == MDB_NOTFOUND)
          throw0(DB_ERROR("leaf not found")); // TODO: specific error type instead of DB_ERROR
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Failed to get leaf: ", result).c_str()));

        auto *db_leaf = (mdb_leaf *)v.mv_data;
        leaves_out.emplace_back(std::move(db_leaf->output_context));

        ++idx;
      }
      while (idx < path_indexes.leaf_range.second);
    }
  }

  // Traverse the tree layer-by-layer starting at the layer closest to leaf layer
  // TODO: separate function for layers
  std::size_t layer_idx = 0;
  for (const auto &layer_idx_range : path_indexes.layers)
  {
    fcmp_pp::curve_trees::ChunkBytes chunk;

    MDB_val_set(k, layer_idx);
    MDB_val_set(v, layer_idx_range.first);
    MDB_cursor_op op = MDB_GET_BOTH;
    for (std::size_t i = layer_idx_range.first; i < layer_idx_range.second; ++i)
    {
      MTRACE("Getting child at layer_idx: " << layer_idx << " , idx: " << i);

      int result = mdb_cursor_get(m_cur_layers, &k, &v, op);
      op = MDB_NEXT_DUP;
      if (result == MDB_NOTFOUND)
        throw0(DB_ERROR("layer elem not found")); // TODO: specific error type instead of DB_ERROR
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to get layer elem: ", result).c_str()));

      auto *lv = (layer_val *)v.mv_data;
      chunk.chunk_bytes.emplace_back(std::move(lv->child_chunk_hash));
    }

    layer_chunks_out.emplace_back(std::move(chunk));
    ++layer_idx;
  }

  TXN_POSTFIX_RDONLY();

  return path_bytes;
}

void BlockchainLMDB::trim_block()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  const uint64_t n_blocks = this->height();
  if (n_blocks == 0)
    return;

  const uint64_t removing_block_idx = n_blocks - 1;

  // Get the earliest possible last locked block of outputs created in removing_block_idx
  const uint64_t default_last_locked_block = cryptonote::get_default_last_locked_block_index(removing_block_idx);
  const uint64_t tree_block_idx = this->get_tree_block_idx();
  if (tree_block_idx != default_last_locked_block)
  {
    throw0(DB_ERROR(("Unexpected tree block idx mismatch ("
        + std::to_string(tree_block_idx) + " vs " + std::to_string(default_last_locked_block) + ")").c_str()));
  }

  if (tree_block_idx == 0)
    throw0(DB_ERROR("Unexpected 0 tree block idx"));
  const uint64_t prev_tree_block_idx = tree_block_idx - 1;

  LOG_PRINT_L1("Trimming tree to block " << prev_tree_block_idx << " (removing block " << removing_block_idx << ")");

  // Read n leaf tuples from the prev tree block to see how how many leaves
  // should remain in the tree after trimming a block from the tree.
  const uint64_t new_n_leaf_tuples = this->get_block_n_leaf_tuples(prev_tree_block_idx);

  // Trim the tree to the new n leaf tuples
  this->trim_tree(new_n_leaf_tuples, tree_block_idx);
}

void BlockchainLMDB::trim_tree(const uint64_t new_n_leaf_tuples, const uint64_t trim_block_id)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(leaves)
  CURSOR(locked_outputs)
  CURSOR(layers)

  CHECK_AND_ASSERT_THROW_MES(m_write_txn != nullptr, "Must have m_write_txn set to trim tree");

  const uint64_t old_n_leaf_tuples = this->get_n_leaf_tuples();
  if (new_n_leaf_tuples > old_n_leaf_tuples)
    throw1(DB_ERROR("Cannot have more leaves in tree after trimming than exist in the tree already"));

  // Return if we don't need to trim any leaves
  if (new_n_leaf_tuples == old_n_leaf_tuples)
    goto del_this_tree_meta;

  // Trim the leaves, re-adding to locked outputs table
  // TODO: trim_leaves
  {
    MDB_val_set(k_block_id, trim_block_id);
    for (uint64_t i = new_n_leaf_tuples; i < old_n_leaf_tuples; ++i)
    {
      MDB_val_copy<uint64_t> k(i);
      MDB_val v = k;
      int result = mdb_cursor_get(m_cur_leaves, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
      if (result == MDB_NOTFOUND)
        throw0(DB_ERROR("leaf not found")); // TODO: specific error type instead of DB_ERROR
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to get leaf: ", result).c_str()));

      // Re-add the output to the locked output table in order. The output should
      // be in the outputs tables.
      const auto *o = (mdb_leaf *)v.mv_data;
      MDB_val_set(v_output, o->output_context);
      MDEBUG("Re-adding locked output_id: " << o->output_context.output_id << " , last locked block: " << trim_block_id);
      result = mdb_cursor_put(m_cur_locked_outputs, &k_block_id, &v_output, MDB_APPENDDUP);
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to re-add locked output: ", result).c_str()));

      // Delete the leaf
      result = mdb_cursor_del(m_cur_leaves, 0);
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Error removing leaf: ", result).c_str()));

      MDEBUG("Successfully removed leaf at leaf_tuple_idx: " << i);
    }
  }

  // If the tree is supposed to be empty, empty the tree
  if (new_n_leaf_tuples == 0)
  {
    // Empty the layers table, no elems should remain
    int result = mdb_drop(*m_write_txn, m_layers, 0);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Error emptying layers table: ", result).c_str()));
    goto del_this_tree_meta;
  }

  // Shrink the layer sizes and update the last hash in each layer using the prev block's cached tree edge
  {
    // Read the tree edge from the prev block, we're trimming back to that
    if (trim_block_id == 0)
      throw0(DB_ERROR("Unexpected trim_block_id set to 0"));
    const uint64_t prev_block_idx = trim_block_id - 1;
    auto old_tree_edge = this->get_tree_edge(prev_block_idx);

    // Determine how may new elems in each layer there will be
    const auto n_elems_per_layer = m_curve_trees->n_elems_per_layer(new_n_leaf_tuples);

    // Trim each layer down to size and reset the tree edge elems using prev block's tree edge
    // TODO: trim_layers
    for (std::size_t layer_idx = 0; layer_idx < n_elems_per_layer.size(); ++layer_idx)
    {
      const uint64_t n_new_elems = n_elems_per_layer[layer_idx];
      if (n_new_elems == 0)
        throw0(DB_ERROR("Unexpected 0 new elems"));
      if (layer_idx >= old_tree_edge.size())
        throw0(DB_ERROR("Tree edge is too small"));

      // Delete all excess elems in layer
      this->trim_layer(n_new_elems, layer_idx);

      // Set the new last elem using the old tree edge elem
      MDB_val_copy<uint64_t> k_layer_idx(layer_idx);
      layer_val lv;
      lv.child_chunk_idx  = n_new_elems - 1;
      lv.child_chunk_hash = std::move(old_tree_edge[layer_idx]);
      MDB_val_set(v_lv, lv);

      // Overwrite layer last elem
      MDEBUG("Re-setting elem " << lv.child_chunk_idx << " at layer idx " << layer_idx << ": " << epee::string_tools::pod_to_hex(lv.child_chunk_hash));
      int result = mdb_cursor_put(m_cur_layers, &k_layer_idx, &v_lv, 0);
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to update chunk hash: ", result).c_str()));
    }
  }

  // Delete any remaining layers in layers after the root
  // TODO: trim_leftovers_after_root
  {
    const uint64_t expected_root_idx = m_curve_trees->n_layers(new_n_leaf_tuples) - 1;
    while (1)
    {
      MDB_val k, v;
      int result = mdb_cursor_get(m_cur_layers, &k, &v, MDB_LAST);
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to get last elem: ", result).c_str()));

      const uint64_t last_layer_idx = *(uint64_t *)k.mv_data;
      if (last_layer_idx > expected_root_idx)
      {
        // Delete all elements in layers after the root
        result = mdb_cursor_del(m_cur_layers, MDB_NODUPDATA);
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Error removing elems after root: ", result).c_str()));
      }
      else if (last_layer_idx < expected_root_idx)
      {
        throw0(DB_ERROR("Encountered unexpected last elem in tree before the root"));
      }
      else // last_layer_idx == expected_root_idx
      {
        // We've trimmed all layers past the root, we're done
        break;
      }
    }
  }

// We don't need the trimmed block's tree meta saved anymore. We save the tree
// meta in grow_tree, so delete the tree meta now in trim_tree.
del_this_tree_meta:
  this->del_tree_meta(trim_block_id);
}

void BlockchainLMDB::trim_layer(const uint64_t new_n_elems_in_layer, const uint64_t layer_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(layers)

  MDEBUG("Trimming layer " << layer_idx << ", new elems in layer: " << new_n_elems_in_layer);
  MDB_val_copy<uint64_t> k(layer_idx);

  // Get the number of existing elements in the layer
  // TODO: get_num_elems_in_layer
  uint64_t old_n_elems_in_layer = 0;
  {
    // Get the first record in a layer so we can then get the last record
    MDB_val v;
    int result = mdb_cursor_get(m_cur_layers, &k, &v, MDB_SET);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get first record in layer: ", result).c_str()));

    result = mdb_cursor_get(m_cur_layers, &k, &v, MDB_LAST_DUP);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get layer last elem on trim: ", result).c_str()));

    const auto *lv = (layer_val *)v.mv_data;
    old_n_elems_in_layer = (1 + lv->child_chunk_idx);
  }

  CHECK_AND_ASSERT_THROW_MES(old_n_elems_in_layer >= new_n_elems_in_layer, "unexpected old n elems in layer");
  const uint64_t trim_n_elems_in_layer = old_n_elems_in_layer - new_n_elems_in_layer;

  // Delete the elements
  for (uint64_t i = 0; i < trim_n_elems_in_layer; ++i)
  {
    uint64_t last_elem_idx = (old_n_elems_in_layer - 1 - i);
    MDB_val_set(v, last_elem_idx);

    int result = mdb_cursor_get(m_cur_layers, &k, &v, MDB_GET_BOTH);
    if (result == MDB_NOTFOUND)
      throw0(DB_ERROR("leaf not found")); // TODO: specific error type instead of DB_ERROR
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get elem: ", result).c_str()));

    result = mdb_cursor_del(m_cur_layers, 0);
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Error removing elem: ", result).c_str()));

    MDEBUG("Successfully removed elem at layer_idx: " << layer_idx << " , last_elem_idx: " << last_elem_idx);
  }
}

uint64_t BlockchainLMDB::get_n_leaf_tuples() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(leaves)

  // Get the number of leaf tuples in the tree
  std::uint64_t n_leaf_tuples = 0;

  {
    MDB_val k, v;
    int result = mdb_cursor_get(m_cur_leaves, &k, &v, MDB_LAST);
    if (result == MDB_NOTFOUND)
      n_leaf_tuples = 0;
    else if (result == MDB_SUCCESS)
      n_leaf_tuples = 1 + ((const mdb_leaf*)v.mv_data)->leaf_idx;
    else
      throw0(DB_ERROR(lmdb_error("Failed to get last leaf: ", result).c_str()));
  }

  TXN_POSTFIX_RDONLY();

  return n_leaf_tuples;
}

uint64_t BlockchainLMDB::get_block_n_leaf_tuples(const uint64_t block_idx) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tree_meta);

  MDB_val_set(k_block_id, block_idx);
  MDB_val v;
  int result = mdb_cursor_get(m_cur_tree_meta, &k_block_id, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    throw0(BLOCK_DNE(std::string("Attempt to get tree meta from blk idx ").append(boost::lexical_cast<std::string>(block_idx)).append(" failed -- tree meta not in db").c_str()));
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error getting tree meta n leaf tuples: ", result).c_str()));

  uint64_t n_leaf_tuples = ((mdb_tree_meta *)v.mv_data)->n_leaf_tuples;

  TXN_POSTFIX_RDONLY();

  return n_leaf_tuples;
}

uint8_t BlockchainLMDB::get_tree_root_at_blk_idx(const uint64_t blk_idx, crypto::ec_point &tree_root_out) const
{
  const std::vector<crypto::ec_point> tree_edge = this->get_tree_edge(blk_idx);
  if (tree_edge.empty())
  {
    tree_root_out = crypto::ec_point{};
    return 0;
  }
  tree_root_out = tree_edge.back();
  static_assert(sizeof(std::size_t) >= sizeof(uint8_t), "unexpected size of size_t");
  return (uint8_t) tree_edge.size();
}

bool BlockchainLMDB::audit_tree(const uint64_t expected_n_leaf_tuples) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(leaves)
  RCURSOR(layers)

  const uint64_t actual_n_leaf_tuples = this->get_n_leaf_tuples();
  CHECK_AND_ASSERT_MES(actual_n_leaf_tuples == expected_n_leaf_tuples, false, "unexpected num leaf tuples");

  MDEBUG("Auditing tree with " << actual_n_leaf_tuples << " leaf tuples");

  if (actual_n_leaf_tuples == 0)
  {
    // Make sure layers table is also empty
    MDB_stat db_stats;
    int result = mdb_stat(m_txn, m_layers, &db_stats);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to query m_layers: ", result).c_str()));
    CHECK_AND_ASSERT_MES(db_stats.ms_entries == 0, false, "unexpected num layer entries");
    return true;
  }

  CHECK_AND_ASSERT_THROW_MES(m_curve_trees != nullptr, "curve trees must be set");

  // Check chunks of leaves hash into first layer as expected
  uint64_t layer_idx = 0;
  uint64_t child_chunk_idx = 0;
  MDB_cursor_op leaf_op = MDB_FIRST;
  MDB_cursor_op parent_op = MDB_FIRST;

  MDB_val_copy<uint64_t> k_parent(layer_idx);
  MDB_val_set(v_parent, child_chunk_idx);

  while (1)
  {
    // Get next leaf chunk
    std::vector<fcmp_pp::curve_trees::CurveTreesV1::LeafTuple> leaf_tuples_chunk;
    leaf_tuples_chunk.reserve(m_curve_trees->m_c1_width);

    if (child_chunk_idx && child_chunk_idx % 1000 == 0)
      MINFO("Auditing layer " << layer_idx << ", child_chunk_idx " << child_chunk_idx);

    // Iterate until chunk is full or we get to the end of all leaves
    MDB_val k_leaf, v_leaf;
    while (1)
    {
      int result = mdb_cursor_get(m_cur_leaves, &k_leaf, &v_leaf, leaf_op);
      leaf_op = MDB_NEXT;
      if (result == MDB_NOTFOUND)
        break;
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to add leaf: ", result).c_str()));

      const auto *o = (mdb_leaf *)v_leaf.mv_data;
      auto leaf = m_curve_trees->leaf_tuple(o->output_context.output_pair);

      leaf_tuples_chunk.emplace_back(std::move(leaf));

      if (leaf_tuples_chunk.size() == m_curve_trees->m_c1_width)
        break;
    }

    // Get the actual leaf chunk hash from the db
    MDEBUG("Getting leaf chunk hash starting at child_chunk_idx " << child_chunk_idx);
    int result = mdb_cursor_get(m_cur_layers, &k_parent, &v_parent, parent_op);
    parent_op = MDB_NEXT_DUP;

    // Check end condition: no more leaf tuples in the leaf layer
    if (leaf_tuples_chunk.empty())
    {
      // No more leaves, expect to be done with parent chunks as well
      if (result != MDB_NOTFOUND)
        throw0(DB_ERROR(lmdb_error("unexpected leaf chunk parent result found at child_chunk_idx "
          + std::to_string(child_chunk_idx), result).c_str()));

      break;
    }

    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get parent in first layer: ", result).c_str()));
    if (layer_idx != *(uint64_t*)k_parent.mv_data || child_chunk_idx != ((layer_val *)v_parent.mv_data)->child_chunk_idx)
      throw0(DB_ERROR("unexpected parent encountered"));

    // Get the expected leaf chunk hash
    const auto leaves = m_curve_trees->flatten_leaves(std::move(leaf_tuples_chunk));
    const fcmp_pp::curve_trees::Selene::Chunk chunk{leaves.data(), leaves.size()};

    // Hash the chunk of leaves
    const auto chunk_hash = fcmp_pp::curve_trees::get_new_parent(m_curve_trees->m_c1, chunk);
    const auto expected_chunk_hash = m_curve_trees->m_c1->to_string(chunk_hash);
    MDEBUG("chunk_hash " << expected_chunk_hash << " , hash init point: "
      << m_curve_trees->m_c1->to_string(m_curve_trees->m_c1->hash_init_point()) << " (" << leaves.size() << " leaves)");

    // Now compare to value from the db
    const auto *lv = (layer_val *)v_parent.mv_data;
    const auto actual_chunk_hash = epee::string_tools::pod_to_hex(lv->child_chunk_hash);
    MDEBUG("Actual leaf chunk hash " << actual_chunk_hash);

    CHECK_AND_ASSERT_MES(expected_chunk_hash == actual_chunk_hash, false, "unexpected leaf chunk hash");
    CHECK_AND_ASSERT_MES(lv->child_chunk_idx == child_chunk_idx, false, "unexpected child chunk idx");

    ++child_chunk_idx;
  }

  MDEBUG("Successfully audited leaf layer");

  // Traverse up the tree auditing each layer until we've audited every layer in the tree
  bool audit_complete = false;
  while (!audit_complete)
  {
    MDEBUG("Auditing layer " << layer_idx);

    // Alternate starting with c2 as parent (we already audited c1 leaf parents), then c1 as parent, then c2, etc.
    const bool parent_is_c2 = layer_idx % 2 == 0;
    if (parent_is_c2)
    {
      audit_complete = this->audit_layer(
          /*c_child*/         m_curve_trees->m_c1,
          /*c_parent*/        m_curve_trees->m_c2,
          layer_idx,
          /*chunk_width*/     m_curve_trees->m_c2_width);
    }
    else
    {
      audit_complete = this->audit_layer(
          /*c_child*/         m_curve_trees->m_c2,
          /*c_parent*/        m_curve_trees->m_c1,
          layer_idx,
          /*chunk_width*/     m_curve_trees->m_c1_width);
    }

    ++layer_idx;
  }

  TXN_POSTFIX_RDONLY();

  return true;
}

template<typename C_CHILD, typename C_PARENT>
bool BlockchainLMDB::audit_layer(const std::unique_ptr<C_CHILD> &c_child,
  const std::unique_ptr<C_PARENT> &c_parent,
  const uint64_t child_layer_idx,
  const uint64_t chunk_width) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  // Open two separate cursors for child and parent layer
  MDB_cursor *child_layer_cursor, *parent_layer_cursor;

  int c_result = mdb_cursor_open(m_txn, m_layers, &child_layer_cursor);
  if (c_result)
    throw0(DB_ERROR(lmdb_error("Failed to open child cursor: ", c_result).c_str()));
  int p_result = mdb_cursor_open(m_txn, m_layers, &parent_layer_cursor);
  if (p_result)
    throw0(DB_ERROR(lmdb_error("Failed to open parent cursor: ", p_result).c_str()));

  // Set the cursors to the start of each layer
  const uint64_t parent_layer_idx = child_layer_idx + 1;

  MDB_val_set(k_child, child_layer_idx);
  MDB_val_set(k_parent, parent_layer_idx);

  MDB_val v_child, v_parent;

  c_result = mdb_cursor_get(child_layer_cursor, &k_child, &v_child, MDB_SET);
  p_result = mdb_cursor_get(parent_layer_cursor, &k_parent, &v_parent, MDB_SET);

  if (c_result != MDB_SUCCESS)
    throw0(DB_ERROR(lmdb_error("Failed to get child: ", c_result).c_str()));
  if (p_result != MDB_SUCCESS && p_result != MDB_NOTFOUND)
    throw0(DB_ERROR(lmdb_error("Failed to get parent: ", p_result).c_str()));

  // Begin to audit the layer
  MDB_cursor_op op_child = MDB_FIRST_DUP;
  MDB_cursor_op op_parent = MDB_FIRST_DUP;
  bool audit_complete = false;
  uint64_t child_chunk_idx = 0;
  while (1)
  {
    if (child_chunk_idx && child_chunk_idx % 1000 == 0)
      MINFO("Auditing layer " << parent_layer_idx << ", child_chunk_idx " << child_chunk_idx);

    // Get next child chunk
    std::vector<typename C_CHILD::Point> child_chunk;
    child_chunk.reserve(chunk_width);
    while (1)
    {
      int result = mdb_cursor_get(child_layer_cursor, &k_child, &v_child, op_child);
      op_child = MDB_NEXT_DUP;
      if (result == MDB_NOTFOUND)
        break;
      if (result != MDB_SUCCESS)
        throw0(DB_ERROR(lmdb_error("Failed to get child: ", result).c_str()));

      const auto *lv = (layer_val *)v_child.mv_data;
      auto child_point = c_child->from_bytes(lv->child_chunk_hash);

      child_chunk.emplace_back(std::move(child_point));

      if (child_chunk.size() == chunk_width)
        break;
    }

    // Get the actual chunk hash from the db
    int result = mdb_cursor_get(parent_layer_cursor, &k_parent, &v_parent, op_parent);
    op_parent = MDB_NEXT_DUP;

    // Check for end conditions
    // End condition A (audit_complete=false): finished auditing layer and ready to move up a layer
    // End condition B (audit_complete=true ): finished auditing the tree, no more layers remaining

    // End condition A: check if finished auditing this layer
    if (child_chunk.empty())
    {
      // No more children, expect to be done auditing layer and ready to move up a layer
      if (result != MDB_NOTFOUND)
        throw0(DB_ERROR(lmdb_error("unexpected parent result at parent_layer_idx " + std::to_string(parent_layer_idx)
          + " , child_chunk_idx " + std::to_string(child_chunk_idx) + " : ", result).c_str()));

      MDEBUG("Finished auditing layer " << child_layer_idx);
      audit_complete = false;
      break;
    }

    // End condition B: check if finished auditing the tree
    if (child_chunk_idx == 0 && child_chunk.size() == 1)
    {
      if (p_result != MDB_NOTFOUND)
        throw0(DB_ERROR(lmdb_error("unexpected parent of root at parent_layer_idx " + std::to_string(parent_layer_idx)
          + " , child_chunk_idx " + std::to_string(child_chunk_idx) + " : ", result).c_str()));

      MDEBUG("Encountered root at layer_idx " << child_layer_idx);
      audit_complete = true;
      break;
    }

    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get parent: ", result).c_str()));

    if (child_layer_idx != *(uint64_t*)k_child.mv_data)
      throw0(DB_ERROR("unexpected child encountered"));
    if (parent_layer_idx != *(uint64_t*)k_parent.mv_data)
      throw0(DB_ERROR("unexpected parent encountered"));

    // Get the expected chunk hash
    std::vector<typename C_PARENT::Scalar> child_scalars;
    child_scalars.reserve(child_chunk.size());
    for (const auto &child : child_chunk)
      child_scalars.emplace_back(c_child->point_to_cycle_scalar(child));
    const typename C_PARENT::Chunk chunk{child_scalars.data(), child_scalars.size()};

    for (uint64_t i = 0; i < child_scalars.size(); ++i)
      MDEBUG("Hashing " << c_parent->to_string(child_scalars[i]));

    const auto chunk_hash = fcmp_pp::curve_trees::get_new_parent(c_parent, chunk);
    const auto expected_chunk_hash = c_parent->to_string(chunk_hash);
    MDEBUG("Expected chunk_hash " << expected_chunk_hash << " (" << child_scalars.size() << " children)");

    const auto *lv = (layer_val *)v_parent.mv_data;
    const auto actual_chunk_hash = epee::string_tools::pod_to_hex(lv->child_chunk_hash);
    MDEBUG("Actual chunk hash " << actual_chunk_hash);

    if (expected_chunk_hash != actual_chunk_hash)
      throw0(DB_ERROR(("unexpected hash at child_chunk_idx " + std::to_string(child_chunk_idx)).c_str()));
    if (lv->child_chunk_idx != child_chunk_idx)
      throw0(DB_ERROR(("unexpected child_chunk_idx, epxected " + std::to_string(child_chunk_idx)).c_str()));

    ++child_chunk_idx;
  }

  TXN_POSTFIX_RDONLY();

  return audit_complete;
}

std::vector<fcmp_pp::curve_trees::OutputContext> BlockchainLMDB::get_outs_at_last_locked_block_idx(
  uint64_t block_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(locked_outputs)

  MDB_val_set(k_block_idx, block_idx);
  MDB_val v_output;

  // Get all the locked outputs at the provided block id
  std::vector<fcmp_pp::curve_trees::OutputContext> outs;

  MDB_cursor_op op = MDB_SET;
  while (1)
  {
    int result = mdb_cursor_get(m_cur_locked_outputs, &k_block_idx, &v_output, op);
    if (result == MDB_NOTFOUND)
      break;
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get next locked outputs: ", result).c_str()));
    op = MDB_NEXT_MULTIPLE;

    const uint64_t blk_id = *(const uint64_t*)k_block_idx.mv_data;
    if (blk_id != block_idx)
      throw0(DB_ERROR(("Blk id " + std::to_string(blk_id) + " not the expected" + std::to_string(block_idx)).c_str()));

    const auto range_begin = ((const fcmp_pp::curve_trees::OutputContext*)v_output.mv_data);
    const auto range_end = range_begin + v_output.mv_size / sizeof(fcmp_pp::curve_trees::OutputContext);

    auto it = range_begin;

    // The first MDB_NEXT_MULTIPLE includes the val from MDB_SET, so skip it
    if (outs.size() == 1)
      ++it;

    while (it < range_end)
    {
      outs.push_back(*it);
      ++it;
    }
  }

  TXN_POSTFIX_RDONLY();

  return outs;
}

void BlockchainLMDB::del_locked_outs_at_block_idx(uint64_t block_idx)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(locked_outputs)

  MDB_val_set(k_block_idx, block_idx);

  int result = mdb_cursor_get(m_cur_locked_outputs, &k_block_idx, NULL, MDB_SET);
  if (result == MDB_NOTFOUND)
    return;
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error finding locked outputs to remove: ", result).c_str()));

  result = mdb_cursor_del(m_cur_locked_outputs, MDB_NODUPDATA);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error removing locked outputs: ", result).c_str()));
}

uint64_t BlockchainLMDB::get_tree_block_idx() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  RCURSOR(tree_meta)

  MDB_val k;
  MDB_val v;
  int result = mdb_cursor_get(m_cur_tree_meta, &k, &v, MDB_LAST);
  if (result != MDB_SUCCESS)
    throw1(DB_ERROR(lmdb_error("Error finding last tree meta: ", result).c_str()));

  uint64_t block_idx = *(uint64_t *)k.mv_data;

  TXN_POSTFIX_RDONLY();

  return block_idx;
}

fcmp_pp::curve_trees::OutsByLastLockedBlock BlockchainLMDB::get_custom_timelocked_outputs(uint64_t start_block_idx) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(timelocked_outputs)

  fcmp_pp::curve_trees::OutsByLastLockedBlock outs;

  /*
    We expect the timelocked outputs table to be sorted primarily by key, i.e.
    last locked block idx. For a given key, we expect timelocked outputs to be
    sorted based on output id. Output id's increase monotonically
    based on when the output is added to the chain.
  */

  // 1. Get all custom timelocked outputs with last locked block start_block_idx or higher
  MDB_cursor_op op = MDB_LAST;
  while (1)
  {
    MDB_val k, v;
    int result = mdb_cursor_get(m_cur_timelocked_outputs, &k, &v, op);
    if (result == MDB_NOTFOUND)
      break;
    if (result != MDB_SUCCESS)
      throw0(DB_ERROR(lmdb_error("Failed to get last timelocked output: ", result).c_str()));

    const uint64_t last_locked_block_idx = *(const uint64_t*)k.mv_data;

    // We can stop as soon as we encounter a timelocked output with last locked
    // block < start_block_idx, since the table should be sorted primarily by
    // last locked block, and we're iterating in reverse.
    if (last_locked_block_idx < start_block_idx)
      break;

    // Make a new entry for the last locked block idx if not already present
    auto outs_it = outs.find(last_locked_block_idx);
    if (outs_it == outs.end())
    {
      outs[last_locked_block_idx] = {};
      outs_it = outs.find(last_locked_block_idx);
    }

    auto timelocked_output = *((const fcmp_pp::curve_trees::OutputContext*)v.mv_data);
    outs_it->second.emplace_back(std::move(timelocked_output));

    op = MDB_PREV;
  }

  // 2. Only return outputs that were created BEFORE start_block_idx
  // 2a. Place all output id's in a flat vector
  using output_id_pair_t = std::pair<uint64_t/*output_id*/, uint64_t/*last_locked_block*/>;
  std::vector<output_id_pair_t> output_ids;
  for (const auto &outs_by_last_locked_block : outs)
  {
    for (const auto &o : outs_by_last_locked_block.second)
      output_ids.push_back({ o.output_id, outs_by_last_locked_block.first });
  }

  // 2b. Sort the vector in descending order by output ID, so outputs are ordered most recently created first
  std::sort(output_ids.begin(), output_ids.end(), [](const output_id_pair_t &a, const output_id_pair_t &b)
    {return a.first > b.first; });

  // 2c. Remove all outputs from the result container that were created at height start_block_idx or higher
  for (const auto &output_id : output_ids)
  {
    const auto output_tx = this->get_output_tx_and_index_from_global(output_id.first);
    const uint64_t created_block_idx = this->get_tx_block_height(output_tx.first);

    // As soon as we encounter an output created before start_block_idx, we're done, we've removed all we needed to
    if (created_block_idx < start_block_idx)
      break;

    // Find the output in the outs container
    auto blk_it = outs.find(output_id.second/*last_locked_block*/);

    // The first one in the vec should be the output id since outputs were added from the table in descending order
    if (blk_it == outs.end())
      throw0(DB_ERROR("Missing output's last locked block"));
    if (blk_it->second.empty())
      throw0(DB_ERROR("last locked block missing outputs"));
    if (blk_it->second.front().output_id != output_id.first)
      throw0(DB_ERROR("Output id is not first in outs by last locked block"));

    // Remove the output from outs container
    blk_it->second.erase(blk_it->second.begin());
    if (blk_it->second.empty())
      outs.erase(blk_it);
  }

  // 3. Sort outputs of each last locked block by output id
  for (auto &outs_by_last_locked_block : outs)
  {
    auto &unsorted = outs_by_last_locked_block.second;
    std::sort(unsorted.begin(), unsorted.end(),
      [](const fcmp_pp::curve_trees::OutputContext &a, const fcmp_pp::curve_trees::OutputContext &b)
        {return a.output_id < b.output_id; });
  }

  TXN_POSTFIX_RDONLY();

  return outs;
}

fcmp_pp::curve_trees::OutsByLastLockedBlock BlockchainLMDB::get_recent_locked_outputs(uint64_t end_block_idx) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();

  fcmp_pp::curve_trees::OutsByLastLockedBlock outs;

  const uint64_t height = this->height();
  if (height == 0)
    return outs;

  const uint64_t coinbase_start_idx = CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW > end_block_idx
    ? 0
    : end_block_idx - CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;

  const uint64_t normal_start_idx = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > end_block_idx
    ? 0
    : end_block_idx - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

  const uint64_t end_blk_idx = std::min(height - 1, end_block_idx);

  const auto get_last_locked_blocks = [this, &outs, normal_start_idx](uint64_t b_idx, const crypto::hash, const block &b) -> bool
  {
    // Add output data grouped by last locked block idx
    auto add_by_last_locked_block = [this, &outs, b_idx](const crypto::hash &tx_hash, const bool is_coinbase)
    {
      cryptonote::transaction tx;
      auto out_data = this->get_tx_output_data(tx_hash, tx);
      if (out_data.empty())
        return;

      const uint64_t last_locked_block = cryptonote::get_last_locked_block_index(out_data.front().data.unlock_time, b_idx);

      // Ignore custom timelocked outputs
      if (cryptonote::is_custom_timelocked(is_coinbase, last_locked_block, b_idx))
        return;

      auto outs_it = outs.find(last_locked_block);
      if (outs_it == outs.end())
      {
        outs[last_locked_block] = {};
        outs_it = outs.find(last_locked_block);
      }

      const bool torsion_checked = cryptonote::tx_outs_checked_for_torsion(tx);
      for (auto &out : out_data)
        outs_it->second.push_back({ out.output_id, torsion_checked, { std::move(out.data.pubkey), std::move(out.data.commitment) }});
    };

    // Add coinbase outputs
    add_by_last_locked_block(cryptonote::get_transaction_hash(b.miner_tx), true);

    if (normal_start_idx > b_idx)
      return true;

    // Add normal outputs
    for (const auto &tx_hash : b.tx_hashes)
      add_by_last_locked_block(tx_hash, false);

    return true;
  };

  for_blocks_range(coinbase_start_idx, end_blk_idx, get_last_locked_blocks);

  TXN_POSTFIX_RDONLY();

  return outs;
}

BlockchainLMDB::~BlockchainLMDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  // batch transaction shouldn't be active at this point. If it is, consider it aborted.
  if (m_batch_active)
  {
    try { BlockchainLMDB::batch_abort(); }
    catch (...) { /* ignore */ }
  }
  if (m_open)
    BlockchainLMDB::close();
}

BlockchainLMDB::BlockchainLMDB(bool batch_transactions, std::shared_ptr<fcmp_pp::curve_trees::CurveTreesV1> curve_trees): BlockchainDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";

  m_batch_transactions = batch_transactions;
  m_write_txn = nullptr;
  m_write_batch_txn = nullptr;
  m_batch_active = false;
  m_cum_size = 0;
  m_cum_count = 0;

  // reset may also need changing when initialize things here

  m_hardfork = nullptr;

  m_curve_trees = curve_trees;
}

#ifdef WIN32
static bool disable_ntfs_compression(const boost::filesystem::path& filepath)
{
  DWORD file_attributes = ::GetFileAttributesW(filepath.c_str());
  if (file_attributes == INVALID_FILE_ATTRIBUTES)
  {
    MERROR("Failed to get " << filepath.string() << " file attributes. Error: " << ::GetLastError());
    return false;
  }
  
  if (!(file_attributes & FILE_ATTRIBUTE_COMPRESSED))
    return true; // not compressed

  LOG_PRINT_L1("Disabling NTFS compression for " << filepath.string());
  HANDLE file_handle = ::CreateFileW(
    filepath.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    OPEN_EXISTING,
    boost::filesystem::is_directory(filepath) ? FILE_FLAG_BACKUP_SEMANTICS : 0, // Needed to open handles to directories
    nullptr
  );

  if (file_handle == INVALID_HANDLE_VALUE) 
  {
    MERROR("Failed to open handle: " << filepath.string() << ". Error: " << ::GetLastError());
    return false;
  }

  USHORT compression_state = COMPRESSION_FORMAT_NONE;
  DWORD bytes_returned;
  BOOL ok = ::DeviceIoControl(
    file_handle,
    FSCTL_SET_COMPRESSION,
    &compression_state,
    sizeof(compression_state),
    nullptr,
    0,
    &bytes_returned,
    nullptr
  );

  ::CloseHandle(file_handle);
  return ok;
}
#endif

void BlockchainLMDB::open(const std::string& filename, const int db_flags)
{
  int result;
  int mdb_flags = MDB_NORDAHEAD;

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  if (m_open)
    throw0(DB_OPEN_FAILURE("Attempted to open db, but it's already open"));

  if (m_curve_trees == nullptr)
    throw0(DB_OPEN_FAILURE("curve trees not set yet, must be set before opening db"));

  boost::filesystem::path direc(filename);
  if (!boost::filesystem::exists(direc) &&
      !boost::filesystem::create_directories(direc)) {
      throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
  }

  // check for existing LMDB files in base directory
  boost::filesystem::path old_files = direc.parent_path();
  if (boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_FILENAME)
      || boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME))
  {
    LOG_PRINT_L0("Found existing LMDB files in " << old_files.string());
    LOG_PRINT_L0("Move " << CRYPTONOTE_BLOCKCHAINDATA_FILENAME << " and/or " << CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME << " to " << filename << ", or delete them, and then restart");
    throw DB_ERROR("Database could not be opened");
  }

#ifdef WIN32
  // ensure NTFS compression is disabled on the directory and database file to avoid corruption of the blockchain
  if (!disable_ntfs_compression(filename))
    LOG_PRINT_L0("Failed to disable NTFS compression on folder: " << filename << ". Error: " << ::GetLastError());
  boost::filesystem::path datafile(filename);
  datafile /= CRYPTONOTE_BLOCKCHAINDATA_FILENAME;
  if (!boost::filesystem::exists(datafile))
    boost::filesystem::ofstream(datafile).close(); // create the file to see if NTFS compression is enabled beforehand
  if (!disable_ntfs_compression(datafile))
    throw DB_ERROR("Database file is NTFS compressed and compression could not be disabled");
#endif

  boost::optional<bool> is_hdd_result = tools::is_hdd(filename.c_str());
  if (is_hdd_result)
  {
    if (is_hdd_result.value())
        MCLOG_RED(el::Level::Warning, "global", "The blockchain is on a rotating drive: this will be very slow, use an SSD if possible");
  }

  m_folder = filename;

#ifdef __OpenBSD__
  if ((mdb_flags & MDB_WRITEMAP) == 0) {
    MCLOG_RED(el::Level::Info, "global", "Running on OpenBSD: forcing WRITEMAP");
    mdb_flags |= MDB_WRITEMAP;
  }
#endif
  // set up lmdb environment
  if ((result = mdb_env_create(&m_env)))
    throw0(DB_ERROR(lmdb_error("Failed to create lmdb environment: ", result).c_str()));
  if ((result = mdb_env_set_maxdbs(m_env, 32)))
    throw0(DB_ERROR(lmdb_error("Failed to set max number of dbs: ", result).c_str()));

  int threads = tools::get_max_concurrency();
  if (threads > 110 &&	/* maxreaders default is 126, leave some slots for other read processes */
    (result = mdb_env_set_maxreaders(m_env, threads+16)))
    throw0(DB_ERROR(lmdb_error("Failed to set max number of readers: ", result).c_str()));

  size_t mapsize = DEFAULT_MAPSIZE;

  if (db_flags & DBF_FAST)
    mdb_flags |= MDB_NOSYNC;
  if (db_flags & DBF_FASTEST)
    mdb_flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;
  if (db_flags & DBF_RDONLY)
    mdb_flags = MDB_RDONLY;
  if (db_flags & DBF_SALVAGE)
    mdb_flags |= MDB_PREVSNAPSHOT;

  if (auto result = mdb_env_open(m_env, filename.c_str(), mdb_flags, 0644))
    throw0(DB_ERROR(lmdb_error("Failed to open lmdb environment: ", result).c_str()));

  MDB_envinfo mei;
  mdb_env_info(m_env, &mei);
  uint64_t cur_mapsize = (uint64_t)mei.me_mapsize;

  if (cur_mapsize < mapsize)
  {
    if (auto result = mdb_env_set_mapsize(m_env, mapsize))
      throw0(DB_ERROR(lmdb_error("Failed to set max memory map size: ", result).c_str()));
    mdb_env_info(m_env, &mei);
    cur_mapsize = (uint64_t)mei.me_mapsize;
    LOG_PRINT_L1("LMDB memory map size: " << cur_mapsize);
  }

  if (need_resize())
  {
    LOG_PRINT_L0("LMDB memory map needs to be resized, doing that now.");
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
  // also change blockchain_prune.cpp to match
  lmdb_db_open(txn, LMDB_BLOCKS, MDB_INTEGERKEY | MDB_CREATE, m_blocks, "Failed to open db handle for m_blocks");

  lmdb_db_open(txn, LMDB_BLOCK_INFO, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for m_block_info");
  lmdb_db_open(txn, LMDB_BLOCK_HEIGHTS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_heights, "Failed to open db handle for m_block_heights");

  lmdb_db_open(txn, LMDB_TXS, MDB_INTEGERKEY | MDB_CREATE, m_txs, "Failed to open db handle for m_txs");
  lmdb_db_open(txn, LMDB_TXS_PRUNED, MDB_INTEGERKEY | MDB_CREATE, m_txs_pruned, "Failed to open db handle for m_txs_pruned");
  lmdb_db_open(txn, LMDB_TXS_PRUNABLE, MDB_INTEGERKEY | MDB_CREATE, m_txs_prunable, "Failed to open db handle for m_txs_prunable");
  lmdb_db_open(txn, LMDB_TXS_PRUNABLE_HASH, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_txs_prunable_hash, "Failed to open db handle for m_txs_prunable_hash");
  if (!(mdb_flags & MDB_RDONLY))
    lmdb_db_open(txn, LMDB_TXS_PRUNABLE_TIP, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_txs_prunable_tip, "Failed to open db handle for m_txs_prunable_tip");
  lmdb_db_open(txn, LMDB_TX_INDICES, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_tx_indices, "Failed to open db handle for m_tx_indices");
  lmdb_db_open(txn, LMDB_TX_OUTPUTS, MDB_INTEGERKEY | MDB_CREATE, m_tx_outputs, "Failed to open db handle for m_tx_outputs");

  lmdb_db_open(txn, LMDB_OUTPUT_TXS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_output_txs, "Failed to open db handle for m_output_txs");
  lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_output_amounts, "Failed to open db handle for m_output_amounts");

  lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_spent_keys, "Failed to open db handle for m_spent_keys");

  lmdb_db_open(txn, LMDB_LOCKED_OUTPUTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_locked_outputs, "Failed to open db handle for m_locked_outputs");
  lmdb_db_open(txn, LMDB_LEAVES, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_leaves, "Failed to open db handle for m_leaves");
  lmdb_db_open(txn, LMDB_LAYERS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_layers, "Failed to open db handle for m_layers");
  lmdb_db_open(txn, LMDB_TREE_EDGES, MDB_INTEGERKEY | MDB_CREATE, m_tree_edges, "Failed to open db handle for m_tree_edges");
  lmdb_db_open(txn, LMDB_TREE_META, MDB_INTEGERKEY | MDB_CREATE, m_tree_meta, "Failed to open db handle for m_tree_meta");

  lmdb_db_open(txn, LMDB_TIMELOCKED_OUTPUTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_timelocked_outputs, "Failed to open db handle for m_timelocked_outputs");

  lmdb_db_open(txn, LMDB_TXPOOL_META, MDB_CREATE, m_txpool_meta, "Failed to open db handle for m_txpool_meta");
  lmdb_db_open(txn, LMDB_TXPOOL_BLOB, MDB_CREATE, m_txpool_blob, "Failed to open db handle for m_txpool_blob");

  lmdb_db_open(txn, LMDB_ALT_BLOCKS, MDB_CREATE, m_alt_blocks, "Failed to open db handle for m_alt_blocks");

  // this subdb is dropped on sight, so it may not be present when we open the DB.
  // Since we use MDB_CREATE, we'll get an exception if we open read-only and it does not exist.
  // So we don't open for read-only, and also not drop below. It is not used elsewhere.
  if (!(mdb_flags & MDB_RDONLY))
    lmdb_db_open(txn, LMDB_HF_STARTING_HEIGHTS, MDB_CREATE, m_hf_starting_heights, "Failed to open db handle for m_hf_starting_heights");

  lmdb_db_open(txn, LMDB_HF_VERSIONS, MDB_INTEGERKEY | MDB_CREATE, m_hf_versions, "Failed to open db handle for m_hf_versions");

  lmdb_db_open(txn, LMDB_PROPERTIES, MDB_CREATE, m_properties, "Failed to open db handle for m_properties");

  mdb_set_dupsort(txn, m_spent_keys, compare_hash32);
  mdb_set_dupsort(txn, m_block_heights, compare_hash32);
  mdb_set_dupsort(txn, m_tx_indices, compare_hash32);
  mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
  mdb_set_dupsort(txn, m_locked_outputs, compare_uint64);
  mdb_set_dupsort(txn, m_leaves, compare_uint64);
  mdb_set_dupsort(txn, m_layers, compare_uint64);
  mdb_set_compare(txn, m_tree_edges, compare_uint64);
  mdb_set_compare(txn, m_tree_meta, compare_uint64);
  mdb_set_dupsort(txn, m_timelocked_outputs, compare_uint64);
  mdb_set_dupsort(txn, m_output_txs, compare_uint64);
  mdb_set_dupsort(txn, m_block_info, compare_uint64);
  if (!(mdb_flags & MDB_RDONLY))
    mdb_set_dupsort(txn, m_txs_prunable_tip, compare_uint64);
  mdb_set_compare(txn, m_txs_prunable, compare_uint64);
  mdb_set_dupsort(txn, m_txs_prunable_hash, compare_uint64);

  mdb_set_compare(txn, m_txpool_meta, compare_hash32);
  mdb_set_compare(txn, m_txpool_blob, compare_hash32);
  mdb_set_compare(txn, m_alt_blocks, compare_hash32);
  mdb_set_compare(txn, m_properties, compare_string);

  if (!(mdb_flags & MDB_RDONLY))
  {
    result = mdb_drop(txn, m_hf_starting_heights, 1);
    if (result && result != MDB_NOTFOUND)
      throw0(DB_ERROR(lmdb_error("Failed to drop m_hf_starting_heights: ", result).c_str()));
  }

  // get and keep current height
  MDB_stat db_stats;
  if ((result = mdb_stat(txn, m_blocks, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
  LOG_PRINT_L2("Setting m_height to: " << db_stats.ms_entries);
  uint64_t m_height = db_stats.ms_entries;

  bool compatible = true;

  MDB_val_str(k, "version");
  MDB_val v;
  auto get_result = mdb_get(txn, m_properties, &k, &v);
  if(get_result == MDB_SUCCESS)
  {
    const uint32_t db_version = *(const uint32_t*)v.mv_data;
    if (db_version > VERSION)
    {
      MWARNING("Existing lmdb database was made by a later version (" << db_version << "). We don't know how it will change yet.");
      compatible = false;
    }
#if VERSION > 0
    else if (db_version < VERSION)
    {
      if (mdb_flags & MDB_RDONLY)
      {
        txn.abort();
        mdb_env_close(m_env);
        m_open = false;
        MFATAL("Existing lmdb database needs to be converted, which cannot be done on a read-only database.");
        MFATAL("Please run monerod once to convert the database.");
        return;
      }
      // Note that there was a schema change within version 0 as well.
      // See commit e5d2680094ee15889934fe28901e4e133cda56f2 2015/07/10
      // We don't handle the old format previous to that commit.
      txn.commit();
      m_open = true;
      // Decrement num active txs so db can resize if needed
      mdb_txn_safe::increment_txns(-1);
      migrate(db_version);
      mdb_txn_safe::increment_txns(1);
      return;
    }
#endif
  }
  else
  {
    // if not found, and the DB is non-empty, this is probably
    // an "old" version 0, which we don't handle. If the DB is
    // empty it's fine.
    if (VERSION > 0 && m_height > 0)
      compatible = false;
  }

  if (!compatible)
  {
    txn.abort();
    mdb_env_close(m_env);
    m_open = false;
    MFATAL("Existing lmdb database is incompatible with this version.");
    MFATAL("Please delete the existing database and resync.");
    return;
  }

  if (!(mdb_flags & MDB_RDONLY))
  {
    // only write version on an empty DB
    if (m_height == 0)
    {
      MDB_val_str(k, "version");
      MDB_val_copy<uint32_t> v(VERSION);
      auto put_result = mdb_put(txn, m_properties, &k, &v, 0);
      if (put_result != MDB_SUCCESS)
      {
        txn.abort();
        mdb_env_close(m_env);
        m_open = false;
        MERROR("Failed to write version to database.");
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
    BlockchainLMDB::batch_abort();
  }
  BlockchainLMDB::sync();
  m_tinfo.reset();

  // FIXME: not yet thread safe!!!  Use with care.
  mdb_env_close(m_env);
  m_open = false;
}

void BlockchainLMDB::sync()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (BlockchainLMDB::is_read_only())
    return;

  // Does nothing unless LMDB environment was opened with MDB_NOSYNC or in part
  // MDB_NOMETASYNC. Force flush to be synchronous.
  if (auto result = mdb_env_sync(m_env, true))
  {
    throw0(DB_ERROR(lmdb_error("Failed to sync database: ", result).c_str()));
  }
}

void BlockchainLMDB::safesyncmode(const bool onoff)
{
  MINFO("switching safe mode " << (onoff ? "on" : "off"));
  mdb_env_set_flags(m_env, MDB_NOSYNC|MDB_MAPASYNC, !onoff);
}

void BlockchainLMDB::reset()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  mdb_txn_safe txn;
  if (auto result = lmdb_txn_begin(m_env, NULL, 0, txn))
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

  if (auto result = mdb_drop(txn, m_blocks, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_blocks: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_block_info, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_block_info: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_block_heights, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_block_heights: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_txs_pruned, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_pruned: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_txs_prunable, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_prunable: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_txs_prunable_hash, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_prunable_hash: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_txs_prunable_tip, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_prunable_tip: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_tx_indices, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_tx_indices: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_tx_outputs, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_tx_outputs: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_output_txs, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_output_txs: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_output_amounts, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_output_amounts: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_spent_keys, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_spent_keys: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_locked_outputs, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_locked_outputs: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_leaves, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_leaves: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_layers, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_layers: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_tree_edges, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_tree_edges: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_tree_meta, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_tree_meta: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_timelocked_outputs, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_timelocked_outputs: ", result).c_str()));
  (void)mdb_drop(txn, m_hf_starting_heights, 0); // this one is dropped in new code
  if (auto result = mdb_drop(txn, m_hf_versions, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_hf_versions: ", result).c_str()));
  if (auto result = mdb_drop(txn, m_properties, 0))
    throw0(DB_ERROR(lmdb_error("Failed to drop m_properties: ", result).c_str()));

  // init with current version
  MDB_val_str(k, "version");
  MDB_val_copy<uint32_t> v(VERSION);
  if (auto result = mdb_put(txn, m_properties, &k, &v, 0))
    throw0(DB_ERROR(lmdb_error("Failed to write version to database: ", result).c_str()));

  txn.commit();
  m_cum_size = 0;
  m_cum_count = 0;
}

std::vector<std::string> BlockchainLMDB::get_filenames() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  std::vector<std::string> filenames;

  boost::filesystem::path datafile(m_folder);
  datafile /= CRYPTONOTE_BLOCKCHAINDATA_FILENAME;
  boost::filesystem::path lockfile(m_folder);
  lockfile /= CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME;

  filenames.push_back(datafile.string());
  filenames.push_back(lockfile.string());

  return filenames;
}

bool BlockchainLMDB::remove_data_file(const std::string& folder) const
{
  const std::string filename = folder + "/data.mdb";
  try
  {
    boost::filesystem::remove(filename);
  }
  catch (const std::exception &e)
  {
    MERROR("Failed to remove " << filename << ": " << e.what());
    return false;
  }
  return true;
}

std::string BlockchainLMDB::get_db_name() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  return std::string("lmdb");
}

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
    if (auto mdb_res = lmdb_txn_begin(m_env, NULL, flags, auto_txn)) \
      throw0(DB_ERROR(lmdb_error(std::string("Failed to create a transaction for the db in ")+__FUNCTION__+": ", mdb_res).c_str())); \
  } \

#define TXN_BLOCK_POSTFIX_SUCCESS() \
  do { \
    if (! m_batch_active && ! m_write_txn) \
      auto_txn.commit(); \
  } while(0)

void BlockchainLMDB::add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata_ref &blob, const txpool_tx_meta_t &meta)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(txpool_meta)
  CURSOR(txpool_blob)

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v = {sizeof(meta), (void *)&meta};
  if (auto result = mdb_cursor_put(m_cur_txpool_meta, &k, &v, MDB_NODUPDATA)) {
    if (result == MDB_KEYEXIST)
      throw1(DB_ERROR("Attempting to add txpool tx metadata that's already in the db"));
    else
      throw1(DB_ERROR(lmdb_error("Error adding txpool tx metadata to db transaction: ", result).c_str()));
  }
  MDB_val_sized(blob_val, blob);
  if (auto result = mdb_cursor_put(m_cur_txpool_blob, &k, &blob_val, MDB_NODUPDATA)) {
    if (result == MDB_KEYEXIST)
      throw1(DB_ERROR("Attempting to add txpool tx blob that's already in the db"));
    else
      throw1(DB_ERROR(lmdb_error("Error adding txpool tx blob to db transaction: ", result).c_str()));
  }
}

void BlockchainLMDB::update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t &meta)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(txpool_meta)
  CURSOR(txpool_blob)

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, MDB_SET);
  if (result != 0)
    throw1(DB_ERROR(lmdb_error("Error finding txpool tx meta to update: ", result).c_str()));
  result = mdb_cursor_del(m_cur_txpool_meta, 0);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error adding removal of txpool tx metadata to db transaction: ", result).c_str()));
  v = MDB_val({sizeof(meta), (void *)&meta});
  if ((result = mdb_cursor_put(m_cur_txpool_meta, &k, &v, MDB_NODUPDATA)) != 0) {
    if (result == MDB_KEYEXIST)
      throw1(DB_ERROR("Attempting to add txpool tx metadata that's already in the db"));
    else
      throw1(DB_ERROR(lmdb_error("Error adding txpool tx metadata to db transaction: ", result).c_str()));
  }
}

uint64_t BlockchainLMDB::get_txpool_tx_count(relay_category category) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  int result;
  uint64_t num_entries = 0;

  TXN_PREFIX_RDONLY();

  if (category == relay_category::all)
  {
    // No filtering, we can get the number of tx the "fast" way
    MDB_stat db_stats;
    if ((result = mdb_stat(m_txn, m_txpool_meta, &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_txpool_meta: ", result).c_str()));
    num_entries = db_stats.ms_entries;
  }
  else
  {
    // Filter unrelayed tx out of the result, so we need to loop over transactions and check their meta data
    RCURSOR(txpool_meta);
    RCURSOR(txpool_blob);

    MDB_val k;
    MDB_val v;
    MDB_cursor_op op = MDB_FIRST;
    while (1)
    {
      result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, op);
      op = MDB_NEXT;
      if (result == MDB_NOTFOUND)
        break;
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate txpool tx metadata: ", result).c_str()));
      const txpool_tx_meta_t &meta = *(const txpool_tx_meta_t*)v.mv_data;
      if (meta.matches(category))
        ++num_entries;
    }
  }
  TXN_POSTFIX_RDONLY();

  return num_entries;
}

bool BlockchainLMDB::txpool_has_tx(const crypto::hash& txid, relay_category tx_category) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txpool_meta)

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, MDB_SET);
  if (result != 0 && result != MDB_NOTFOUND)
    throw1(DB_ERROR(lmdb_error("Error finding txpool tx meta: ", result).c_str()));
  if (result == MDB_NOTFOUND)
    return false;

  bool found = true;
  if (tx_category != relay_category::all)
  {
    const txpool_tx_meta_t &meta = *(const txpool_tx_meta_t*)v.mv_data;
    found = meta.matches(tx_category);
  }
  TXN_POSTFIX_RDONLY();
  return found;
}

void BlockchainLMDB::remove_txpool_tx(const crypto::hash& txid)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(txpool_meta)
  CURSOR(txpool_blob)

  MDB_val k = {sizeof(txid), (void *)&txid};
  auto result = mdb_cursor_get(m_cur_txpool_meta, &k, NULL, MDB_SET);
  if (result != 0 && result != MDB_NOTFOUND)
    throw1(DB_ERROR(lmdb_error("Error finding txpool tx meta to remove: ", result).c_str()));
  if (!result)
  {
    result = mdb_cursor_del(m_cur_txpool_meta, 0);
    if (result)
      throw1(DB_ERROR(lmdb_error("Error adding removal of txpool tx metadata to db transaction: ", result).c_str()));
  }
  result = mdb_cursor_get(m_cur_txpool_blob, &k, NULL, MDB_SET);
  if (result != 0 && result != MDB_NOTFOUND)
    throw1(DB_ERROR(lmdb_error("Error finding txpool tx blob to remove: ", result).c_str()));
  if (!result)
  {
    result = mdb_cursor_del(m_cur_txpool_blob, 0);
    if (result)
      throw1(DB_ERROR(lmdb_error("Error adding removal of txpool tx blob to db transaction: ", result).c_str()));
  }
}

bool BlockchainLMDB::get_txpool_tx_meta(const crypto::hash& txid, txpool_tx_meta_t &meta) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txpool_meta)

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v;
  auto result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
      return false;
  if (result != 0)
      throw1(DB_ERROR(lmdb_error("Error finding txpool tx meta: ", result).c_str()));

  meta = *(const txpool_tx_meta_t*)v.mv_data;
  TXN_POSTFIX_RDONLY();
  return true;
}

bool BlockchainLMDB::get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd, relay_category tx_category) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txpool_blob)

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v;

  // if filtering, make sure those requirements are met before copying blob
  if (tx_category != relay_category::all)
  {
    RCURSOR(txpool_meta)
    auto result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, MDB_SET);
    if (result == MDB_NOTFOUND)
      return false;
    if (result != 0)
      throw1(DB_ERROR(lmdb_error("Error finding txpool tx meta: ", result).c_str()));

    const txpool_tx_meta_t& meta = *(const txpool_tx_meta_t*)v.mv_data;
    if (!meta.matches(tx_category))
      return false;
  }

  auto result = mdb_cursor_get(m_cur_txpool_blob, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    return false;
  if (result != 0)
      throw1(DB_ERROR(lmdb_error("Error finding txpool tx blob: ", result).c_str()));

  bd.assign(reinterpret_cast<const char*>(v.mv_data), v.mv_size);
  TXN_POSTFIX_RDONLY();
  return true;
}

cryptonote::blobdata BlockchainLMDB::get_txpool_tx_blob(const crypto::hash& txid, relay_category tx_category) const
{
  cryptonote::blobdata bd;
  if (!get_txpool_tx_blob(txid, bd, tx_category))
    throw1(DB_ERROR("Tx not found in txpool: "));
  return bd;
}

uint32_t BlockchainLMDB::get_blockchain_pruning_seed() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(properties)
  MDB_val_str(k, "pruning_seed");
  MDB_val v;
  int result = mdb_cursor_get(m_cur_properties, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    return 0;
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to retrieve pruning seed: ", result).c_str()));
  if (v.mv_size != sizeof(uint32_t))
    throw0(DB_ERROR("Failed to retrieve or create pruning seed: unexpected value size"));
  uint32_t pruning_seed;
  memcpy(&pruning_seed, v.mv_data, sizeof(pruning_seed));
  TXN_POSTFIX_RDONLY();
  return pruning_seed;
}

static bool is_v1_tx(MDB_cursor *c_txs_pruned, MDB_val *tx_id)
{
  MDB_val v;
  int ret = mdb_cursor_get(c_txs_pruned, tx_id, &v, MDB_SET);
  if (ret)
    throw0(DB_ERROR(lmdb_error("Failed to find transaction pruned data: ", ret).c_str()));
  if (v.mv_size == 0)
    throw0(DB_ERROR("Invalid transaction pruned data"));
  return cryptonote::is_v1_tx(cryptonote::blobdata_ref{(const char*)v.mv_data, v.mv_size});
}

enum { prune_mode_prune, prune_mode_update, prune_mode_check };

bool BlockchainLMDB::prune_worker(int mode, uint32_t pruning_seed)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  const uint32_t log_stripes = tools::get_pruning_log_stripes(pruning_seed);
  if (log_stripes && log_stripes != CRYPTONOTE_PRUNING_LOG_STRIPES)
    throw0(DB_ERROR("Pruning seed not in range"));
  pruning_seed = tools::get_pruning_stripe(pruning_seed);
  if (pruning_seed > (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES))
    throw0(DB_ERROR("Pruning seed not in range"));
  check_open();

  TIME_MEASURE_START(t);

  size_t n_total_records = 0, n_prunable_records = 0, n_pruned_records = 0, commit_counter = 0;
  uint64_t n_bytes = 0;

  mdb_txn_safe txn;
  auto result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

  MDB_stat db_stats;
  if ((result = mdb_stat(txn, m_txs_prunable, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_txs_prunable: ", result).c_str()));
  const size_t pages0 = db_stats.ms_branch_pages + db_stats.ms_leaf_pages + db_stats.ms_overflow_pages;

  MDB_val_str(k, "pruning_seed");
  MDB_val v;
  result = mdb_get(txn, m_properties, &k, &v);
  bool prune_tip_table = false;
  if (result == MDB_NOTFOUND)
  {
    // not pruned yet
    if (mode != prune_mode_prune)
    {
      txn.abort();
      TIME_MEASURE_FINISH(t);
      MDEBUG("Pruning not enabled, nothing to do");
      return true;
    }
    if (pruning_seed == 0)
      pruning_seed = tools::get_random_stripe();
    pruning_seed = tools::make_pruning_seed(pruning_seed, CRYPTONOTE_PRUNING_LOG_STRIPES);
    v.mv_data = &pruning_seed;
    v.mv_size = sizeof(pruning_seed);
    result = mdb_put(txn, m_properties, &k, &v, 0);
    if (result)
      throw0(DB_ERROR("Failed to save pruning seed"));
    prune_tip_table = false;
  }
  else if (result == 0)
  {
    // pruned already
    if (v.mv_size != sizeof(uint32_t))
      throw0(DB_ERROR("Failed to retrieve or create pruning seed: unexpected value size"));
    const uint32_t data = *(const uint32_t*)v.mv_data;
    if (pruning_seed == 0)
      pruning_seed = tools::get_pruning_stripe(data);
    if (tools::get_pruning_stripe(data) != pruning_seed)
      throw0(DB_ERROR("Blockchain already pruned with different seed"));
    if (tools::get_pruning_log_stripes(data) != CRYPTONOTE_PRUNING_LOG_STRIPES)
      throw0(DB_ERROR("Blockchain already pruned with different base"));
    pruning_seed = tools::make_pruning_seed(pruning_seed, CRYPTONOTE_PRUNING_LOG_STRIPES);
    prune_tip_table = (mode == prune_mode_update);
  }
  else
  {
    throw0(DB_ERROR(lmdb_error("Failed to retrieve or create pruning seed: ", result).c_str()));
  }

  if (mode == prune_mode_check)
    MINFO("Checking blockchain pruning...");
  else
    MINFO("Pruning blockchain...");

  MDB_cursor *c_txs_pruned, *c_txs_prunable, *c_txs_prunable_tip;
  result = mdb_cursor_open(txn, m_txs_pruned, &c_txs_pruned);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
  result = mdb_cursor_open(txn, m_txs_prunable, &c_txs_prunable);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
  result = mdb_cursor_open(txn, m_txs_prunable_tip, &c_txs_prunable_tip);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_tip: ", result).c_str()));
  const uint64_t blockchain_height = height();

  if (prune_tip_table)
  {
    MDB_cursor_op op = MDB_FIRST;
    while (1)
    {
      int ret = mdb_cursor_get(c_txs_prunable_tip, &k, &v, op);
      op = MDB_NEXT;
      if (ret == MDB_NOTFOUND)
        break;
      if (ret)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate transactions: ", ret).c_str()));

      uint64_t block_height;
      memcpy(&block_height, v.mv_data, sizeof(block_height));
      if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS < blockchain_height)
      {
        ++n_total_records;
        if (!tools::has_unpruned_block(block_height, blockchain_height, pruning_seed) && !is_v1_tx(c_txs_pruned, &k))
        {
          ++n_prunable_records;
          result = mdb_cursor_get(c_txs_prunable, &k, &v, MDB_SET);
          if (result == MDB_NOTFOUND)
            MDEBUG("Already pruned at height " << block_height << "/" << blockchain_height);
          else if (result)
            throw0(DB_ERROR(lmdb_error("Failed to find transaction prunable data: ", result).c_str()));
          else
          {
            MDEBUG("Pruning at height " << block_height << "/" << blockchain_height);
            ++n_pruned_records;
            ++commit_counter;
            n_bytes += k.mv_size + v.mv_size;
            result = mdb_cursor_del(c_txs_prunable, 0);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to delete transaction prunable data: ", result).c_str()));
          }
        }
        result = mdb_cursor_del(c_txs_prunable_tip, 0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to delete transaction tip data: ", result).c_str()));

        if (mode != prune_mode_check && commit_counter >= 4096)
        {
          MDEBUG("Committing txn at checkpoint...");
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
          result = mdb_cursor_open(txn, m_txs_pruned, &c_txs_pruned);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
          result = mdb_cursor_open(txn, m_txs_prunable, &c_txs_prunable);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
          result = mdb_cursor_open(txn, m_txs_prunable_tip, &c_txs_prunable_tip);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_tip: ", result).c_str()));
          commit_counter = 0;
        }
      }
    }
  }
  else
  {
    MDB_cursor *c_tx_indices;
    result = mdb_cursor_open(txn, m_tx_indices, &c_tx_indices);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to open a cursor for tx_indices: ", result).c_str()));
    MDB_cursor_op op = MDB_FIRST;
    while (1)
    {
      int ret = mdb_cursor_get(c_tx_indices, &k, &v, op);
      op = MDB_NEXT;
      if (ret == MDB_NOTFOUND)
        break;
      if (ret)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate transactions: ", ret).c_str()));

      ++n_total_records;
      //const txindex *ti = (const txindex *)v.mv_data;
      txindex ti;
      memcpy(&ti, v.mv_data, sizeof(ti));
      const uint64_t block_height = ti.data.block_id;
      if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS >= blockchain_height)
      {
        MDB_val_set(kp, ti.data.tx_id);
        MDB_val_set(vp, block_height);
        if (mode == prune_mode_check)
        {
          result = mdb_cursor_get(c_txs_prunable_tip, &kp, &vp, MDB_SET);
          if (result && result != MDB_NOTFOUND)
            throw0(DB_ERROR(lmdb_error("Error looking for transaction prunable data: ", result).c_str()));
          if (result == MDB_NOTFOUND)
            MERROR("Transaction not found in prunable tip table for height " << block_height << "/" << blockchain_height <<
                ", seed " << epee::string_tools::to_string_hex(pruning_seed));
        }
        else
        {
          result = mdb_cursor_put(c_txs_prunable_tip, &kp, &vp, 0);
          if (result && result != MDB_NOTFOUND)
            throw0(DB_ERROR(lmdb_error("Error looking for transaction prunable data: ", result).c_str()));
        }
      }
      MDB_val_set(kp, ti.data.tx_id);
      if (!tools::has_unpruned_block(block_height, blockchain_height, pruning_seed) && !is_v1_tx(c_txs_pruned, &kp))
      {
        result = mdb_cursor_get(c_txs_prunable, &kp, &v, MDB_SET);
        if (result && result != MDB_NOTFOUND)
          throw0(DB_ERROR(lmdb_error("Error looking for transaction prunable data: ", result).c_str()));
        if (mode == prune_mode_check)
        {
          if (result != MDB_NOTFOUND)
            MERROR("Prunable data found for pruned height " << block_height << "/" << blockchain_height <<
                ", seed " << epee::string_tools::to_string_hex(pruning_seed));
        }
        else
        {
          ++n_prunable_records;
          if (result == MDB_NOTFOUND)
            MDEBUG("Already pruned at height " << block_height << "/" << blockchain_height);
          else
          {
            MDEBUG("Pruning at height " << block_height << "/" << blockchain_height);
            ++n_pruned_records;
            n_bytes += kp.mv_size + v.mv_size;
            result = mdb_cursor_del(c_txs_prunable, 0);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to delete transaction prunable data: ", result).c_str()));
            ++commit_counter;
          }
        }
      }
      else
      {
        if (mode == prune_mode_check)
        {
          MDB_val_set(kp, ti.data.tx_id);
          result = mdb_cursor_get(c_txs_prunable, &kp, &v, MDB_SET);
          if (result && result != MDB_NOTFOUND)
            throw0(DB_ERROR(lmdb_error("Error looking for transaction prunable data: ", result).c_str()));
          if (result == MDB_NOTFOUND)
            MERROR("Prunable data not found for unpruned height " << block_height << "/" << blockchain_height <<
                ", seed " << epee::string_tools::to_string_hex(pruning_seed));
        }
      }

      if (mode != prune_mode_check && commit_counter >= 4096)
      {
        MDEBUG("Committing txn at checkpoint...");
        txn.commit();
        result = mdb_txn_begin(m_env, NULL, 0, txn);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs_pruned, &c_txs_pruned);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs_prunable, &c_txs_prunable);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs_prunable_tip, &c_txs_prunable_tip);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_tip: ", result).c_str()));
        result = mdb_cursor_open(txn, m_tx_indices, &c_tx_indices);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for tx_indices: ", result).c_str()));
        MDB_val val;
        val.mv_size = sizeof(ti);
        val.mv_data = (void *)&ti;
        result = mdb_cursor_get(c_tx_indices, (MDB_val*)&zerokval, &val, MDB_GET_BOTH);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to restore cursor for tx_indices: ", result).c_str()));
        commit_counter = 0;
      }
    }
    mdb_cursor_close(c_tx_indices);
  }

  if ((result = mdb_stat(txn, m_txs_prunable, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_txs_prunable: ", result).c_str()));
  const size_t pages1 = db_stats.ms_branch_pages + db_stats.ms_leaf_pages + db_stats.ms_overflow_pages;
  const size_t db_bytes = (pages0 - pages1) * db_stats.ms_psize;

  mdb_cursor_close(c_txs_prunable_tip);
  mdb_cursor_close(c_txs_prunable);
  mdb_cursor_close(c_txs_pruned);

  txn.commit();

  TIME_MEASURE_FINISH(t);

  MINFO((mode == prune_mode_check ? "Checked" : "Pruned") << " blockchain in " <<
      t << " ms: " << (n_bytes/1024.0f/1024.0f) << " MB (" << db_bytes/1024.0f/1024.0f << " MB) pruned in " <<
      n_pruned_records << " records (" << pages0 - pages1 << "/" << pages0 << " " << db_stats.ms_psize << " byte pages), " <<
      n_prunable_records << "/" << n_total_records << " pruned records");
  return true;
}

bool BlockchainLMDB::prune_blockchain(uint32_t pruning_seed)
{
  return prune_worker(prune_mode_prune, pruning_seed);
}

bool BlockchainLMDB::update_pruning()
{
  return prune_worker(prune_mode_update, 0);
}

bool BlockchainLMDB::check_pruning()
{
  return prune_worker(prune_mode_check, 0);
}

bool BlockchainLMDB::for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata_ref*)> f, bool include_blob, relay_category category) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txpool_meta);
  RCURSOR(txpool_blob);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int result = mdb_cursor_get(m_cur_txpool_meta, &k, &v, op);
    op = MDB_NEXT;
    if (result == MDB_NOTFOUND)
      break;
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to enumerate txpool tx metadata: ", result).c_str()));
    const crypto::hash txid = *(const crypto::hash*)k.mv_data;
    const txpool_tx_meta_t &meta = *(const txpool_tx_meta_t*)v.mv_data;
    if (!meta.matches(category))
      continue;
    cryptonote::blobdata_ref bd;
    if (include_blob)
    {
      MDB_val b;
      result = mdb_cursor_get(m_cur_txpool_blob, &k, &b, MDB_SET);
      if (result == MDB_NOTFOUND)
        throw0(DB_ERROR("Failed to find txpool tx blob to match metadata"));
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate txpool tx blob: ", result).c_str()));
      bd = {reinterpret_cast<const char*>(b.mv_data), b.mv_size};
    }

    if (!f(txid, meta, &bd)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

bool BlockchainLMDB::for_all_alt_blocks(std::function<bool(const crypto::hash&, const alt_block_data_t&, const cryptonote::blobdata_ref*)> f, bool include_blob) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(alt_blocks);

  MDB_val k;
  MDB_val v;
  bool ret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int result = mdb_cursor_get(m_cur_alt_blocks, &k, &v, op);
    op = MDB_NEXT;
    if (result == MDB_NOTFOUND)
      break;
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to enumerate alt blocks: ", result).c_str()));
    const crypto::hash &blkid = *(const crypto::hash*)k.mv_data;
    if (v.mv_size < sizeof(alt_block_data_t))
      throw0(DB_ERROR("alt_blocks record is too small"));
    const alt_block_data_t *data = (const alt_block_data_t*)v.mv_data;
    cryptonote::blobdata_ref bd;
    if (include_blob)
    {
      bd = {reinterpret_cast<const char*>(v.mv_data) + sizeof(alt_block_data_t), v.mv_size - sizeof(alt_block_data_t)};
    }

    if (!f(blkid, *data, &bd)) {
      ret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return ret;
}

bool BlockchainLMDB::block_exists(const crypto::hash& h, uint64_t *height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_heights);

  bool ret = false;
  MDB_val_set(key, h);
  auto get_result = mdb_cursor_get(m_cur_block_heights, (MDB_val *)&zerokval, &key, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L3("Block with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
  }
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch block index from hash", get_result).c_str()));
  else
  {
    if (height)
    {
      const blk_height *bhp = (const blk_height *)key.mv_data;
      *height = bhp->bh_height;
    }
    ret = true;
  }

  TXN_POSTFIX_RDONLY();
  return ret;
}

cryptonote::blobdata BlockchainLMDB::get_block_blob(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  return get_block_blob_from_height(get_block_height(h));
}

uint64_t BlockchainLMDB::get_block_height(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_heights);

  MDB_val_set(key, h);
  auto get_result = mdb_cursor_get(m_cur_block_heights, (MDB_val *)&zerokval, &key, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
    throw1(BLOCK_DNE("Attempted to retrieve non-existent block height"));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block height from the db"));

  blk_height *bhp = (blk_height *)key.mv_data;
  uint64_t ret = bhp->bh_height;
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

cryptonote::blobdata BlockchainLMDB::get_block_blob_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
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

  TXN_POSTFIX_RDONLY();

  return bd;
}

uint64_t BlockchainLMDB::get_block_timestamp(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get timestamp from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- timestamp not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a timestamp from the db"));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  uint64_t ret = bi->bi_timestamp;
  TXN_POSTFIX_RDONLY();
  return ret;
}

std::vector<uint64_t> BlockchainLMDB::get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<uint64_t> res;
  int result;

  if (heights.empty())
    return {};
  res.reserve(heights.size());

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_stat db_stats;
  if ((result = mdb_stat(m_txn, m_blocks, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
  for (size_t i = 0; i < heights.size(); ++i)
    if (heights[i] >= db_stats.ms_entries)
      throw0(BLOCK_DNE(std::string("Attempt to get rct distribution from height " + std::to_string(heights[i]) + " failed -- block size not in db").c_str()));

  MDB_val v;

  uint64_t prev_height = heights[0];
  uint64_t range_begin = 0, range_end = 0;
  for (uint64_t height: heights)
  {
    if (height >= range_begin && height < range_end)
    {
      // nohting to do
    }
    else
    {
      if (height == prev_height + 1)
      {
        MDB_val k2;
        result = mdb_cursor_get(m_cur_block_info, &k2, &v, MDB_NEXT_MULTIPLE);
        range_begin = ((const mdb_block_info*)v.mv_data)->bi_height;
        range_end = range_begin + v.mv_size / sizeof(mdb_block_info); // whole records please
        if (height < range_begin || height >= range_end)
          throw0(DB_ERROR(("Height " + std::to_string(height) + " not included in multuple record range: " + std::to_string(range_begin) + "-" + std::to_string(range_end)).c_str()));
      }
      else
      {
        v.mv_size = sizeof(uint64_t);
        v.mv_data = (void*)&height;
        result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
        range_begin = height;
        range_end = range_begin + 1;
      }
      if (result)
        throw0(DB_ERROR(lmdb_error("Error attempting to retrieve rct distribution from the db: ", result).c_str()));
    }
    const mdb_block_info *bi = ((const mdb_block_info *)v.mv_data) + (height - range_begin);
    res.push_back(bi->bi_cum_rct);
    prev_height = height;
  }

  TXN_POSTFIX_RDONLY();
  return res;
}

uint64_t BlockchainLMDB::get_top_block_timestamp() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  uint64_t m_height = height();

  // if no blocks, return 0
  if (m_height == 0)
  {
    return 0;
  }

  return get_block_timestamp(m_height - 1);
}

size_t BlockchainLMDB::get_block_weight(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get block size from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a block size from the db"));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  size_t ret = bi->bi_weight;
  TXN_POSTFIX_RDONLY();
  return ret;
}

std::vector<uint64_t> BlockchainLMDB::get_block_info_64bit_fields(uint64_t start_height, size_t count, off_t offset) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  const uint64_t h = height();
  if (start_height >= h)
    throw0(DB_ERROR(("Height " + std::to_string(start_height) + " not in blockchain").c_str()));

  std::vector<uint64_t> ret;
  ret.reserve(count);

  MDB_val v;
  uint64_t range_begin = 0, range_end = 0;
  for (uint64_t height = start_height; height < h && count--; ++height)
  {
    if (height >= range_begin && height < range_end)
    {
      // nothing to do
    }
    else
    {
      int result = 0;
      if (range_end > 0)
      {
        MDB_val k2;
        result = mdb_cursor_get(m_cur_block_info, &k2, &v, MDB_NEXT_MULTIPLE);
        range_begin = ((const mdb_block_info*)v.mv_data)->bi_height;
        range_end = range_begin + v.mv_size / sizeof(mdb_block_info); // whole records please
        if (height < range_begin || height >= range_end)
          throw0(DB_ERROR(("Height " + std::to_string(height) + " not included in multiple record range: " + std::to_string(range_begin) + "-" + std::to_string(range_end)).c_str()));
      }
      else
      {
        v.mv_size = sizeof(uint64_t);
        v.mv_data = (void*)&height;
        result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
        range_begin = height;
        range_end = range_begin + 1;
      }
      if (result)
        throw0(DB_ERROR(lmdb_error("Error attempting to retrieve block_info from the db: ", result).c_str()));
    }
    const mdb_block_info *bi = ((const mdb_block_info *)v.mv_data) + (height - range_begin);
    ret.push_back(*(const uint64_t*)(((const char*)bi) + offset));
  }

  TXN_POSTFIX_RDONLY();
  return ret;
}

uint64_t BlockchainLMDB::get_max_block_size()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(properties)
  MDB_val_str(k, "max_block_size");
  MDB_val v;
  int result = mdb_cursor_get(m_cur_properties, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    return std::numeric_limits<uint64_t>::max();
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to retrieve max block size: ", result).c_str()));
  if (v.mv_size != sizeof(uint64_t))
    throw0(DB_ERROR("Failed to retrieve or create max block size: unexpected value size"));
  uint64_t max_block_size;
  memcpy(&max_block_size, v.mv_data, sizeof(max_block_size));
  TXN_POSTFIX_RDONLY();
  return max_block_size;
}

void BlockchainLMDB::add_max_block_size(uint64_t sz)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(properties)

  MDB_val_str(k, "max_block_size");
  MDB_val v;
  int result = mdb_cursor_get(m_cur_properties, &k, &v, MDB_SET);
  if (result && result != MDB_NOTFOUND)
    throw0(DB_ERROR(lmdb_error("Failed to retrieve max block size: ", result).c_str()));
  uint64_t max_block_size = 0;
  if (result == 0)
  {
    if (v.mv_size != sizeof(uint64_t))
      throw0(DB_ERROR("Failed to retrieve or create max block size: unexpected value size"));
    memcpy(&max_block_size, v.mv_data, sizeof(max_block_size));
  }
  if (sz > max_block_size)
    max_block_size = sz;
  v.mv_data = (void*)&max_block_size;
  v.mv_size = sizeof(max_block_size);
  result = mdb_cursor_put(m_cur_properties, &k, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to set max_block_size: ", result).c_str()));
}


std::vector<uint64_t> BlockchainLMDB::get_block_weights(uint64_t start_height, size_t count) const
{
  return get_block_info_64bit_fields(start_height, count, offsetof(mdb_block_info, bi_weight));
}

std::vector<uint64_t> BlockchainLMDB::get_long_term_block_weights(uint64_t start_height, size_t count) const
{
  return get_block_info_64bit_fields(start_height, count, offsetof(mdb_block_info, bi_long_term_block_weight));
}

difficulty_type BlockchainLMDB::get_block_cumulative_difficulty(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__ << "  height: " << height);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get cumulative difficulty from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- difficulty not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a cumulative difficulty from the db"));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  difficulty_type ret = bi->bi_diff_hi;
  ret <<= 64;
  ret |= bi->bi_diff_lo;
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

void BlockchainLMDB::correct_block_cumulative_difficulties(const uint64_t& start_height, const std::vector<difficulty_type>& new_cumulative_difficulties)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  int result = 0;
  block_wtxn_start();
  CURSOR(block_info)

  const uint64_t bc_height = height();
  if (start_height + new_cumulative_difficulties.size() != bc_height)
  {
    block_wtxn_abort();
    throw0(DB_ERROR("Incorrect new_cumulative_difficulties size"));
  }

  for (uint64_t height = start_height; height < bc_height; ++height)
  {
    MDB_val_set(key, height);
    result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &key, MDB_GET_BOTH);
    if (result)
      throw1(BLOCK_DNE(lmdb_error("Failed to get block info: ", result).c_str()));

    mdb_block_info bi = *(mdb_block_info*)key.mv_data;
    const difficulty_type d = new_cumulative_difficulties[height - start_height];
    bi.bi_diff_hi = ((d >> 64) & 0xffffffffffffffff).convert_to<uint64_t>();
    bi.bi_diff_lo = (d & 0xffffffffffffffff).convert_to<uint64_t>();

    MDB_val_set(key2, height);
    MDB_val_set(val, bi);
    result = mdb_cursor_put(m_cur_block_info, &key2, &val, MDB_CURRENT);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to overwrite block info to db transaction: ", result).c_str()));
  }
  block_wtxn_stop();
}

uint64_t BlockchainLMDB::get_block_already_generated_coins(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get generated coins from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block size not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a total generated coins from the db"));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  uint64_t ret = bi->bi_coins;
  TXN_POSTFIX_RDONLY();
  return ret;
}

uint64_t BlockchainLMDB::get_block_long_term_weight(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get block long term weight from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- block info not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve a long term block weight from the db"));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  uint64_t ret = bi->bi_long_term_block_weight;
  TXN_POSTFIX_RDONLY();
  return ret;
}

crypto::hash BlockchainLMDB::get_block_hash_from_height(const uint64_t& height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(block_info);

  MDB_val_set(result, height);
  auto get_result = mdb_cursor_get(m_cur_block_info, (MDB_val *)&zerokval, &result, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw0(BLOCK_DNE(std::string("Attempt to get hash from height ").append(boost::lexical_cast<std::string>(height)).append(" failed -- hash not in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("Error attempting to retrieve a block hash from the db: ", get_result).c_str()));

  mdb_block_info *bi = (mdb_block_info *)result.mv_data;
  crypto::hash ret = bi->bi_hash;
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

crypto::hash BlockchainLMDB::top_block_hash(uint64_t *block_height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  uint64_t m_height = height();
  if (block_height)
    *block_height = m_height - 1;
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
  uint64_t m_height = height();

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
  TXN_PREFIX_RDONLY();
  int result;

  // get current height
  MDB_stat db_stats;
  if ((result = mdb_stat(m_txn, m_blocks, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
  return db_stats.ms_entries;
}

uint64_t BlockchainLMDB::num_outputs() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  TXN_PREFIX_RDONLY();
  int result;

  RCURSOR(output_txs)

  uint64_t num = 0;
  MDB_val k, v;
  result = mdb_cursor_get(m_cur_output_txs, &k, &v, MDB_LAST);
  if (result == MDB_NOTFOUND)
    num = 0;
  else if (result == 0)
    num = 1 + ((const outtx*)v.mv_data)->output_id;
  else
    throw0(DB_ERROR(lmdb_error("Failed to query m_output_txs: ", result).c_str()));

  return num;
}

bool BlockchainLMDB::tx_exists(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);

  MDB_val_set(key, h);
  bool tx_found = false;

  TIME_MEASURE_START(time1);
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &key, MDB_GET_BOTH);
  if (get_result == 0)
    tx_found = true;
  else if (get_result != MDB_NOTFOUND)
    throw0(DB_ERROR(lmdb_error(std::string("DB error attempting to fetch transaction index from hash ") + epee::string_tools::pod_to_hex(h) + ": ", get_result).c_str()));

  TIME_MEASURE_FINISH(time1);
  time_tx_exists += time1;

  TXN_POSTFIX_RDONLY();

  if (! tx_found)
  {
    LOG_PRINT_L3("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
    return false;
  }

  return true;
}

bool BlockchainLMDB::tx_exists(const crypto::hash& h, uint64_t& tx_id) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);

  MDB_val_set(v, h);

  TIME_MEASURE_START(time1);
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  TIME_MEASURE_FINISH(time1);
  time_tx_exists += time1;
  if (!get_result) {
    txindex *tip = (txindex *)v.mv_data;
    tx_id = tip->data.tx_id;
  }

  TXN_POSTFIX_RDONLY();

  bool ret = false;
  if (get_result == MDB_NOTFOUND)
  {
    LOG_PRINT_L3("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
  }
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch transaction from hash", get_result).c_str()));
  else
    ret = true;

  return ret;
}

uint64_t BlockchainLMDB::get_tx_unlock_time(const crypto::hash& h) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);

  MDB_val_set(v, h);
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
    throw1(TX_DNE(lmdb_error(std::string("tx data with hash ") + epee::string_tools::pod_to_hex(h) + " not found in db: ", get_result).c_str()));
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx data from hash: ", get_result).c_str()));

  txindex *tip = (txindex *)v.mv_data;
  uint64_t ret = tip->data.unlock_time;
  TXN_POSTFIX_RDONLY();
  return ret;
}

std::vector<outkey> BlockchainLMDB::get_tx_output_data(const crypto::hash& h, cryptonote::transaction &tx) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(tx_outputs);

  // Get txid from tx_indices table
  MDB_val_set(v, h);
  int result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (result == MDB_NOTFOUND)
    throw1(TX_DNE(lmdb_error(std::string("tx data with hash ") + epee::string_tools::pod_to_hex(h) + " not found in db: ", result).c_str()));
  else if (result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx data from hash: ", result).c_str()));
  txindex *tip = (txindex *)v.mv_data;

  // Get tx amount output indices from tx_outputs table
  MDB_val_set(k_tx_id, tip->data.tx_id);
  MDB_val v_tx_output;

  result = mdb_cursor_get(m_cur_tx_outputs, &k_tx_id, &v_tx_output, MDB_SET);
  if (result == MDB_NOTFOUND)
  {
    LOG_PRINT_L0("WARNING: Unexpected: tx has no amount indices stored in "
        "tx_outputs, but it should have an empty entry even if it's a tx without "
        "outputs");
    return {};
  }
  else if (result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to get data for tx_outputs[tx_index]", result).c_str()));

  const uint64_t* amount_output_indices = (const uint64_t*)v_tx_output.mv_data;
  size_t num_amount_output_indices = v_tx_output.mv_size / sizeof(uint64_t);

  // Get tx output amounts from pruned txs table
  std::vector<uint64_t> amounts;
  cryptonote::blobdata bd;
  if (!get_pruned_tx_blob(h, bd))
    throw1(TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  if (!parse_and_validate_tx_base_from_blob(bd, tx))
    throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
  amounts.reserve(tx.vout.size());
  for (const auto &out : tx.vout)
    amounts.push_back(tx.version >= 2 ? 0 : out.amount);

  if (amounts.size() != num_amount_output_indices)
    throw0(DB_ERROR("Unexpected size mismatch amount output indices <> vouts"));

  // Get output data
  std::vector<outkey> outs;
  for (size_t i = 0; i < amounts.size(); ++i)
    outs.emplace_back(get_output_key(amounts[i], amount_output_indices[i], true/*include_commitment*/));

  TXN_POSTFIX_RDONLY();
  return outs;
}

bool BlockchainLMDB::get_tx_blob(const crypto::hash& h, cryptonote::blobdata &bd) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(txs_pruned);
  RCURSOR(txs_prunable);

  MDB_val_set(v, h);
  MDB_val result0, result1;
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == 0)
  {
    txindex *tip = (txindex *)v.mv_data;
    MDB_val_set(val_tx_id, tip->data.tx_id);
    get_result = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, &result0, MDB_SET);
    if (get_result == 0)
    {
      get_result = mdb_cursor_get(m_cur_txs_prunable, &val_tx_id, &result1, MDB_SET);
    }
  }
  if (get_result == MDB_NOTFOUND)
    return false;
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx from hash", get_result).c_str()));

  bd.assign(reinterpret_cast<char*>(result0.mv_data), result0.mv_size);
  bd.append(reinterpret_cast<char*>(result1.mv_data), result1.mv_size);

  TXN_POSTFIX_RDONLY();

  return true;
}

bool BlockchainLMDB::get_pruned_tx_blob(const crypto::hash& h, cryptonote::blobdata &bd) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(txs_pruned);

  MDB_val_set(v, h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == 0)
  {
    txindex *tip = (txindex *)v.mv_data;
    MDB_val_set(val_tx_id, tip->data.tx_id);
    get_result = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, &result, MDB_SET);
  }
  if (get_result == MDB_NOTFOUND)
    return false;
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx from hash", get_result).c_str()));

  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  TXN_POSTFIX_RDONLY();

  return true;
}

bool BlockchainLMDB::get_pruned_tx_blobs_from(const crypto::hash& h, size_t count, std::vector<cryptonote::blobdata> &bd) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (!count)
    return true;

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(txs_pruned);

  bd.reserve(bd.size() + count);

  MDB_val_set(v, h);
  MDB_val result;
  int res = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (res == MDB_NOTFOUND)
    return false;
  if (res)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx from hash", res).c_str()));

  const txindex *tip = (const txindex *)v.mv_data;
  const uint64_t id = tip->data.tx_id;
  MDB_val_set(val_tx_id, id);
  MDB_cursor_op op = MDB_SET;
  while (count--)
  {
    res = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, &result, op);
    op = MDB_NEXT;
    if (res == MDB_NOTFOUND)
      return false;
    if (res)
      throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx blob", res).c_str()));
    bd.emplace_back(reinterpret_cast<char*>(result.mv_data), result.mv_size);
  }

  TXN_POSTFIX_RDONLY();

  return true;
}

std::vector<crypto::hash> BlockchainLMDB::get_txids_loose(const crypto::hash& txid_template, std::uint32_t bits, uint64_t max_num_txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  std::vector<crypto::hash> matching_hashes;

  TXN_PREFIX_RDONLY();  // Start a read-only transaction
  RCURSOR(tx_indices);  // Open cursors to the tx_indices and txpool_meta databases
  RCURSOR(txpool_meta);

  // Search on-chain and pool transactions together, starting with on-chain txs
  MDB_cursor* cursor = m_cur_tx_indices;
  MDB_val k = zerokval; // tx_indicies DB uses a dummy key
  MDB_val_set(v, txid_template); // tx_indicies DB indexes data values by crypto::hash value on front
  MDB_cursor_op op = MDB_GET_BOTH_RANGE; // Set the cursor to the first key/value pair >= the given key
  bool doing_chain = true; // this variable tells us whether we are processing chain or pool txs
  while (1)
  {
    const int get_result = mdb_cursor_get(cursor, &k, &v, op);
    op = doing_chain ? MDB_NEXT_DUP : MDB_NEXT; // Set the cursor to the next key/value pair
    if (get_result && get_result != MDB_NOTFOUND)
      throw0(DB_ERROR(lmdb_error("DB error attempting to fetch txid range", get_result).c_str()));

    // In tx_indicies, the hash is stored at the data, in txpool_meta at the key
    const crypto::hash* const p_dbtxid = (const crypto::hash*)(doing_chain ? v.mv_data : k.mv_data);

    // Check if we reached the end of a DB or the hashes no longer match the template
    if (get_result == MDB_NOTFOUND || compare_hash32_reversed_nbits(txid_template, *p_dbtxid, bits))
    {
      if (doing_chain) // done with chain processing, switch to pool processing
      {
        k.mv_size = sizeof(crypto::hash); // txpool_meta DB is indexed using crypto::hash as keys
        k.mv_data = (void*) txid_template.data;
        cursor = m_cur_txpool_meta; // switch databases
        op = MDB_SET_RANGE; // Set the cursor to the first key >= the given key
        doing_chain = false;
        continue;
      }
      break; // if we get to this point, then we finished pool processing and we are done
    }
    else if (matching_hashes.size() >= max_num_txs && max_num_txs != 0)
      throw0(TX_EXISTS("number of tx hashes in template range exceeds maximum"));

    matching_hashes.push_back(*p_dbtxid);
  }

  TXN_POSTFIX_RDONLY();  // End the read-only transaction

  return matching_hashes;
}

bool BlockchainLMDB::get_blocks_from(uint64_t start_height, size_t min_block_count, size_t max_block_count, size_t max_tx_count, size_t max_size, std::vector<std::pair<std::pair<cryptonote::blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, cryptonote::blobdata>>>>& blocks, bool pruned, bool skip_coinbase, bool get_miner_tx_hash) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(blocks);
  RCURSOR(tx_indices);
  RCURSOR(txs_pruned);
  if (!pruned)
  {
    RCURSOR(txs_prunable);
  }

  blocks.reserve(std::min<size_t>(max_block_count, 10000)); // guard against very large max count if only checking bytes
  const uint64_t blockchain_height = height();
  uint64_t size = 0;
  size_t num_txes = 0;
  MDB_val_copy<uint64_t> key(start_height);
  MDB_val v, val_tx_id;
  uint64_t tx_id = ~0;
  for (uint64_t h = start_height; h < blockchain_height && blocks.size() < max_block_count && (size < max_size || blocks.size() < min_block_count); ++h)
  {
    MDB_cursor_op op = h == start_height ? MDB_SET : MDB_NEXT;
    int result = mdb_cursor_get(m_cur_blocks, &key, &v, op);
    if (result == MDB_NOTFOUND)
      throw0(BLOCK_DNE(std::string("Attempt to get block from height ").append(boost::lexical_cast<std::string>(h)).append(" failed -- block not in db").c_str()));
    else if (result)
      throw0(DB_ERROR(lmdb_error("Error attempting to retrieve a block from the db", result).c_str()));

    blocks.resize(blocks.size() + 1);
    auto &current_block = blocks.back();

    current_block.first.first.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    size += v.mv_size;

    cryptonote::block b;
    if (!parse_and_validate_block_from_blob(current_block.first.first, b))
      throw0(DB_ERROR("Invalid block"));
    current_block.first.second = get_miner_tx_hash ? cryptonote::get_transaction_hash(b.miner_tx) : crypto::null_hash;

    // get the tx_id for the first tx (the first block's coinbase tx)
    if (h == start_height)
    {
      crypto::hash hash = cryptonote::get_transaction_hash(b.miner_tx);
      MDB_val_set(v, hash);
      result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
      if (result)
        throw0(DB_ERROR(lmdb_error("Error attempting to retrieve block coinbase transaction from the db: ", result).c_str()));

      const txindex *tip = (const txindex *)v.mv_data;
      tx_id = tip->data.tx_id;
      val_tx_id.mv_data = &tx_id;
      val_tx_id.mv_size = sizeof(tx_id);
    }

    if (skip_coinbase)
    {
      result = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, &v, op);
      if (result)
        throw0(DB_ERROR(lmdb_error("Error attempting to retrieve transaction data from the db: ", result).c_str()));
      if (!pruned)
      {
        result = mdb_cursor_get(m_cur_txs_prunable, &val_tx_id, &v, op);
        if (result)
          throw0(DB_ERROR(lmdb_error("Error attempting to retrieve transaction data from the db: ", result).c_str()));
      }
    }

    op = MDB_NEXT;

    current_block.second.reserve(b.tx_hashes.size());
    num_txes += b.tx_hashes.size() + (skip_coinbase ? 0 : 1);
    for (const auto &tx_hash: b.tx_hashes)
    {
      // get pruned data
      cryptonote::blobdata tx_blob;
      result = mdb_cursor_get(m_cur_txs_pruned, &val_tx_id, &v, op);
      if (result)
        throw0(DB_ERROR(lmdb_error("Error attempting to retrieve transaction data from the db: ", result).c_str()));
      tx_blob.assign((const char*)v.mv_data, v.mv_size);

      if (!pruned)
      {
        result = mdb_cursor_get(m_cur_txs_prunable, &val_tx_id, &v, op);
        if (result)
          throw0(DB_ERROR(lmdb_error("Error attempting to retrieve transaction data from the db: ", result).c_str()));
        tx_blob.append(reinterpret_cast<const char*>(v.mv_data), v.mv_size);
      }
      current_block.second.push_back(std::make_pair(tx_hash, std::move(tx_blob)));
      size += current_block.second.back().second.size();
    }

    if (blocks.size() >= min_block_count && num_txes >= max_tx_count)
      break;
  }

  TXN_POSTFIX_RDONLY();

  return true;
}

bool BlockchainLMDB::get_prunable_tx_blob(const crypto::hash& h, cryptonote::blobdata &bd) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(txs_prunable);

  MDB_val_set(v, h);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == 0)
  {
    const txindex *tip = (const txindex *)v.mv_data;
    MDB_val_set(val_tx_id, tip->data.tx_id);
    get_result = mdb_cursor_get(m_cur_txs_prunable, &val_tx_id, &result, MDB_SET);
  }
  if (get_result == MDB_NOTFOUND)
    return false;
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx from hash", get_result).c_str()));

  bd.assign(reinterpret_cast<char*>(result.mv_data), result.mv_size);

  TXN_POSTFIX_RDONLY();

  return true;
}

bool BlockchainLMDB::get_prunable_tx_hash(const crypto::hash& tx_hash, crypto::hash &prunable_hash) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_indices);
  RCURSOR(txs_prunable_hash);

  MDB_val_set(v, tx_hash);
  MDB_val result;
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == 0)
  {
    txindex *tip = (txindex *)v.mv_data;
    MDB_val_set(val_tx_id, tip->data.tx_id);
    get_result = mdb_cursor_get(m_cur_txs_prunable_hash, &val_tx_id, &result, MDB_SET);
  }
  if (get_result == MDB_NOTFOUND)
    return false;
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx prunable hash from tx hash", get_result).c_str()));

  prunable_hash = *(const crypto::hash*)result.mv_data;

  TXN_POSTFIX_RDONLY();

  return true;
}

uint64_t BlockchainLMDB::get_tx_count() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  int result;

  MDB_stat db_stats;
  if ((result = mdb_stat(m_txn, m_txs_pruned, &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query m_txs_pruned: ", result).c_str()));

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
  RCURSOR(tx_indices);

  MDB_val_set(v, h);
  auto get_result = mdb_cursor_get(m_cur_tx_indices, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
  {
    throw1(TX_DNE(std::string("tx_data_t with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str()));
  }
  else if (get_result)
    throw0(DB_ERROR(lmdb_error("DB error attempting to fetch tx height from hash", get_result).c_str()));

  txindex *tip = (txindex *)v.mv_data;
  uint64_t ret = tip->data.block_id;
  TXN_POSTFIX_RDONLY();
  return ret;
}

uint64_t BlockchainLMDB::get_num_outputs(const uint64_t& amount) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  MDB_val_copy<uint64_t> k(amount);
  MDB_val v;
  mdb_size_t num_elems = 0;
  auto result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
  if (result == MDB_SUCCESS)
  {
    mdb_cursor_count(m_cur_output_amounts, &num_elems);
  }
  else if (result != MDB_NOTFOUND)
    throw0(DB_ERROR("DB error attempting to get number of outputs of an amount"));

  TXN_POSTFIX_RDONLY();

  return num_elems;
}

outkey BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index, bool include_commitmemt) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  MDB_val_set(k, amount);
  MDB_val_set(v, index);
  auto get_result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE(std::string("Attempting to get output pubkey by index, but key does not exist: amount " +
        std::to_string(amount) + ", index " + std::to_string(index)).c_str()));
  else if (get_result)
    throw0(DB_ERROR("Error attempting to retrieve an output pubkey from the db"));

  outkey ret;
  if (amount == 0)
  {
    ret = *(const outkey *)v.mv_data;
  }
  else
  {
    const pre_rct_outkey *okp = (const pre_rct_outkey *)v.mv_data;
    ret.amount_index = okp->amount_index;
    ret.output_id = okp->output_id;
    memcpy(&ret.data, &okp->data, sizeof(pre_rct_output_data_t));
    if (include_commitmemt)
      ret.data.commitment = rct::zeroCommitVartime(amount);
  }
  TXN_POSTFIX_RDONLY();
  return ret;
}

tx_out_index BlockchainLMDB::get_output_tx_and_index_from_global(const uint64_t& output_id) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_txs);

  MDB_val_set(v, output_id);

  auto get_result = mdb_cursor_get(m_cur_output_txs, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  if (get_result == MDB_NOTFOUND)
    throw1(OUTPUT_DNE("output with given index not in db"));
  else if (get_result)
    throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

  outtx *ot = (outtx *)v.mv_data;
  tx_out_index ret = tx_out_index(ot->tx_hash, ot->local_index);

  TXN_POSTFIX_RDONLY();
  return ret;
}

tx_out_index BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const
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

std::vector<std::vector<uint64_t>> BlockchainLMDB::get_tx_amount_output_indices(uint64_t tx_id, size_t n_txes) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(tx_outputs);

  MDB_val_set(k_tx_id, tx_id);
  MDB_val v;
  std::vector<std::vector<uint64_t>> amount_output_indices_set;
  amount_output_indices_set.reserve(n_txes);

  MDB_cursor_op op = MDB_SET;
  while (n_txes-- > 0)
  {
    int result = mdb_cursor_get(m_cur_tx_outputs, &k_tx_id, &v, op);
    if (result == MDB_NOTFOUND)
      LOG_PRINT_L0("WARNING: Unexpected: tx has no amount indices stored in "
          "tx_outputs, but it should have an empty entry even if it's a tx without "
          "outputs");
    else if (result)
      throw0(DB_ERROR(lmdb_error("DB error attempting to get data for tx_outputs[tx_index]", result).c_str()));

    op = MDB_NEXT;

    const uint64_t* indices = (const uint64_t*)v.mv_data;
    size_t num_outputs = v.mv_size / sizeof(uint64_t);

    amount_output_indices_set.resize(amount_output_indices_set.size() + 1);
    std::vector<uint64_t> &amount_output_indices = amount_output_indices_set.back();
    amount_output_indices.reserve(num_outputs);
    for (size_t i = 0; i < num_outputs; ++i)
    {
      amount_output_indices.push_back(indices[i]);
    }
  }

  TXN_POSTFIX_RDONLY();
  return amount_output_indices_set;
}

bool BlockchainLMDB::has_key_image(const crypto::key_image& img) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  bool ret;

  TXN_PREFIX_RDONLY();
  RCURSOR(spent_keys);

  MDB_val k = {sizeof(img), (void *)&img};
  ret = (mdb_cursor_get(m_cur_spent_keys, (MDB_val *)&zerokval, &k, MDB_GET_BOTH) == 0);

  TXN_POSTFIX_RDONLY();
  return ret;
}

bool BlockchainLMDB::for_all_key_images(std::function<bool(const crypto::key_image&)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(spent_keys);

  MDB_val k, v;
  bool fret = true;

  k = zerokval;
  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_spent_keys, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret < 0)
      throw0(DB_ERROR("Failed to enumerate key images"));
    const crypto::key_image k_image = *(const crypto::key_image*)v.mv_data;
    if (!f(k_image)) {
      fret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return fret;
}

bool BlockchainLMDB::for_blocks_range(const uint64_t& h1, const uint64_t& h2, std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(blocks);

  MDB_val k;
  MDB_val v;
  bool fret = true;

  MDB_cursor_op op;
  if (h1)
  {
    k = MDB_val{sizeof(h1), (void*)&h1};
    op = MDB_SET;
  } else
  {
    op = MDB_FIRST;
  }
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_blocks, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate blocks"));
    uint64_t height = *(const uint64_t*)k.mv_data;
    blobdata_ref bd{reinterpret_cast<char*>(v.mv_data), v.mv_size};
    block b;
    if (!parse_and_validate_block_from_blob(bd, b))
      throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));
    crypto::hash hash;
    if (!get_block_hash(b, hash))
        throw0(DB_ERROR("Failed to get block hash from blob retrieved from the db"));
    if (!f(height, hash, b)) {
      fret = false;
      break;
    }
    if (height >= h2)
      break;
  }

  TXN_POSTFIX_RDONLY();

  return fret;
}

bool BlockchainLMDB::for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)> f, bool pruned) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txs_pruned);
  RCURSOR(txs_prunable);
  RCURSOR(tx_indices);

  MDB_val k;
  MDB_val v;
  bool fret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_tx_indices, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR(lmdb_error("Failed to enumerate transactions: ", ret).c_str()));

    txindex *ti = (txindex *)v.mv_data;
    const crypto::hash hash = ti->key;
    k.mv_data = (void *)&ti->data.tx_id;
    k.mv_size = sizeof(ti->data.tx_id);

    ret = mdb_cursor_get(m_cur_txs_pruned, &k, &v, MDB_SET);
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR(lmdb_error("Failed to enumerate transactions: ", ret).c_str()));
    transaction tx;
    if (pruned)
    {
      blobdata_ref bd{reinterpret_cast<char*>(v.mv_data), v.mv_size};
      if (!parse_and_validate_tx_base_from_blob(bd, tx))
        throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
    }
    else
    {
      blobdata bd;
      bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
      ret = mdb_cursor_get(m_cur_txs_prunable, &k, &v, MDB_SET);
      if (ret)
        throw0(DB_ERROR(lmdb_error("Failed to get prunable tx data the db: ", ret).c_str()));
      bd.append(reinterpret_cast<char*>(v.mv_data), v.mv_size);
      if (!parse_and_validate_tx_from_blob(bd, tx))
        throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
    }
    if (!f(hash, tx)) {
      fret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return fret;
}

bool BlockchainLMDB::for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, uint64_t height, size_t tx_idx)> f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  MDB_val k;
  MDB_val v;
  bool fret = true;

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
    outkey *ok = (outkey *)v.mv_data;
    tx_out_index toi = get_output_tx_and_index_from_global(ok->output_id);
    if (!f(amount, toi.first, ok->data.height, toi.second)) {
      fret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return fret;
}

bool BlockchainLMDB::for_all_outputs(uint64_t amount, const std::function<bool(uint64_t height)> &f) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  MDB_val_set(k, amount);
  MDB_val v;
  bool fret = true;

  MDB_cursor_op op = MDB_SET;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_output_amounts, &k, &v, op);
    op = MDB_NEXT_DUP;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate outputs"));
    uint64_t out_amount = *(const uint64_t*)k.mv_data;
    if (amount != out_amount)
    {
      MERROR("Amount is not the expected amount");
      fret = false;
      break;
    }
    const outkey *ok = (const outkey *)v.mv_data;
    if (!f(ok->data.height)) {
      fret = false;
      break;
    }
  }

  TXN_POSTFIX_RDONLY();

  return fret;
}

// batch_num_blocks: (optional) Used to check if resize needed before batch transaction starts.
bool BlockchainLMDB::batch_start(uint64_t batch_num_blocks, uint64_t batch_bytes)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (m_batch_active)
    return false;
  if (m_write_batch_txn != nullptr)
    return false;
  if (m_write_txn)
    throw0(DB_ERROR("batch transaction attempted, but m_write_txn already in use"));
  check_open();

  m_writer = boost::this_thread::get_id();
  check_and_resize_for_batch(batch_num_blocks, batch_bytes);

  m_write_batch_txn = new mdb_txn_safe();

  // NOTE: need to make sure it's destroyed properly when done
  if (auto mdb_res = lmdb_txn_begin(m_env, NULL, 0, *m_write_batch_txn))
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
  if (m_tinfo.get())
  {
    if (m_tinfo->m_ti_rflags.m_rf_txn)
      mdb_txn_reset(m_tinfo->m_ti_rtxn);
    memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
  }

  LOG_PRINT_L3("batch transaction: begin");
  return true;
}

void BlockchainLMDB::batch_commit()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_batch_txn == nullptr)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_writer != boost::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));

  check_open();

  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  m_write_txn->commit();
  TIME_MEASURE_FINISH(time1);
  time_commit1 += time1;
  LOG_PRINT_L3("batch transaction: committed");

  m_write_txn = nullptr;
  delete m_write_batch_txn;
  m_write_batch_txn = nullptr;
  memset(&m_wcursors, 0, sizeof(m_wcursors));
}

void BlockchainLMDB::cleanup_batch()
{
  // for destruction of batch transaction
  m_write_txn = nullptr;
  delete m_write_batch_txn;
  m_write_batch_txn = nullptr;
  m_batch_active = false;
  memset(&m_wcursors, 0, sizeof(m_wcursors));
}

void BlockchainLMDB::batch_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_batch_txn == nullptr)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_writer != boost::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));
  check_open();
  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  try
  {
    m_write_txn->commit();
    TIME_MEASURE_FINISH(time1);
    time_commit1 += time1;
    cleanup_batch();
  }
  catch (const std::exception &e)
  {
    cleanup_batch();
    throw;
  }
  LOG_PRINT_L3("batch transaction: end");
}

void BlockchainLMDB::batch_abort()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (! m_batch_active)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_batch_txn == nullptr)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_writer != boost::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));
  check_open();
  // for destruction of batch transaction
  m_write_txn = nullptr;
  // explicitly call in case mdb_env_close() (BlockchainLMDB::close()) called before BlockchainLMDB destructor called.
  m_write_batch_txn->abort();
  delete m_write_batch_txn;
  m_write_batch_txn = nullptr;
  m_batch_active = false;
  memset(&m_wcursors, 0, sizeof(m_wcursors));
  LOG_PRINT_L3("batch transaction: aborted");
}

void BlockchainLMDB::set_batch_transactions(bool batch_transactions)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if ((batch_transactions) && (m_batch_transactions))
  {
    MINFO("batch transaction mode already enabled, but asked to enable batch mode");
  }
  m_batch_transactions = batch_transactions;
  MINFO("batch transactions " << (m_batch_transactions ? "enabled" : "disabled"));
}

// return true if we started the txn, false if already started
bool BlockchainLMDB::block_rtxn_start(MDB_txn **mtxn, mdb_txn_cursors **mcur) const
{
  bool ret = false;
  mdb_threadinfo *tinfo;
  if (m_write_txn && m_writer == boost::this_thread::get_id()) {
    *mtxn = m_write_txn->m_txn;
    *mcur = (mdb_txn_cursors *)&m_wcursors;
    return ret;
  }
  /* Check for existing info and force reset if env doesn't match -
   * only happens if env was opened/closed multiple times in same process
   */
  if (!(tinfo = m_tinfo.get()) || mdb_txn_env(tinfo->m_ti_rtxn) != m_env)
  {
    tinfo = new mdb_threadinfo;
    m_tinfo.reset(tinfo);
    memset(&tinfo->m_ti_rcursors, 0, sizeof(tinfo->m_ti_rcursors));
    memset(&tinfo->m_ti_rflags, 0, sizeof(tinfo->m_ti_rflags));
    if (auto mdb_res = lmdb_txn_begin(m_env, NULL, MDB_RDONLY, &tinfo->m_ti_rtxn))
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to create a read transaction for the db: ", mdb_res).c_str()));
    ret = true;
  } else if (!tinfo->m_ti_rflags.m_rf_txn)
  {
    if (auto mdb_res = lmdb_txn_renew(tinfo->m_ti_rtxn))
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to renew a read transaction for the db: ", mdb_res).c_str()));
    ret = true;
  }
  if (ret)
    tinfo->m_ti_rflags.m_rf_txn = true;
  *mtxn = tinfo->m_ti_rtxn;
  *mcur = &tinfo->m_ti_rcursors;

  if (ret)
    LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  return ret;
}

void BlockchainLMDB::block_rtxn_stop() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  mdb_txn_reset(m_tinfo->m_ti_rtxn);
  memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
  /* cancel out the increment from rtxn_start */
  mdb_txn_safe::increment_txns(-1);
}

bool BlockchainLMDB::block_rtxn_start() const
{
  MDB_txn *mtxn;
  mdb_txn_cursors *mcur;
  /* auto_txn is only used for the create gate */
  mdb_txn_safe auto_txn;
  bool ret = block_rtxn_start(&mtxn, &mcur);
  if (ret)
    auto_txn.increment_txns(1); /* remember there is an active readtxn */
  return ret;
}

void BlockchainLMDB::block_wtxn_start()
{
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
    m_writer = boost::this_thread::get_id();
    m_write_txn = new mdb_txn_safe();
    if (auto mdb_res = lmdb_txn_begin(m_env, NULL, 0, *m_write_txn))
    {
      delete m_write_txn;
      m_write_txn = nullptr;
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to create a transaction for the db: ", mdb_res).c_str()));
    }
    memset(&m_wcursors, 0, sizeof(m_wcursors));
    if (m_tinfo.get())
    {
      if (m_tinfo->m_ti_rflags.m_rf_txn)
        mdb_txn_reset(m_tinfo->m_ti_rtxn);
      memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
    }
  } else if (m_writer != boost::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to start new write txn when batch txn already exists in ")+__FUNCTION__).c_str()));
}

void BlockchainLMDB::block_wtxn_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (!m_write_txn)
    throw0(DB_ERROR_TXN_START((std::string("Attempted to stop write txn when no such txn exists in ")+__FUNCTION__).c_str()));
  if (m_writer != boost::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to stop write txn from the wrong thread in ")+__FUNCTION__).c_str()));
  {
    if (! m_batch_active)
	{
      TIME_MEASURE_START(time1);
      m_write_txn->commit();
      TIME_MEASURE_FINISH(time1);
      time_commit1 += time1;

      delete m_write_txn;
      m_write_txn = nullptr;
      memset(&m_wcursors, 0, sizeof(m_wcursors));
	}
  }
}

void BlockchainLMDB::block_wtxn_abort()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (!m_write_txn)
    throw0(DB_ERROR_TXN_START((std::string("Attempted to abort write txn when no such txn exists in ")+__FUNCTION__).c_str()));
  if (m_writer != boost::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to abort write txn from the wrong thread in ")+__FUNCTION__).c_str()));

  if (! m_batch_active)
  {
    delete m_write_txn;
    m_write_txn = nullptr;
    memset(&m_wcursors, 0, sizeof(m_wcursors));
  }
}

void BlockchainLMDB::block_rtxn_abort() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  mdb_txn_reset(m_tinfo->m_ti_rtxn);
  memset(&m_tinfo->m_ti_rflags, 0, sizeof(m_tinfo->m_ti_rflags));
}

uint64_t BlockchainLMDB::add_block(const std::pair<block, blobdata>& blk, size_t block_weight, uint64_t long_term_block_weight, const difficulty_type& cumulative_difficulty, const uint64_t& coins_generated,
    const std::vector<std::pair<transaction, blobdata>>& txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  uint64_t m_height = height();

  if (m_height % 1024 == 0)
  {
    // for batch mode, DB resize check is done at start of batch transaction
    if (! m_batch_active && need_resize())
    {
      LOG_PRINT_L0("LMDB memory map needs to be resized, doing that now.");
      do_resize();
    }
  }

  try
  {
    BlockchainDB::add_block(blk, block_weight, long_term_block_weight, cumulative_difficulty, coins_generated, txs);
  }
  catch (const DB_ERROR_TXN_START& e)
  {
    throw;
  }

  return ++m_height;
}

void BlockchainLMDB::pop_block(block& blk, std::vector<transaction>& txs)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  block_wtxn_start();

  try
  {
    BlockchainDB::pop_block(blk, txs);
    block_wtxn_stop();
  }
  catch (...)
  {
    block_wtxn_abort();
    throw;
  }
}

void BlockchainLMDB::get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
    std::vector<tx_out_index> &tx_out_indices) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  tx_out_indices.clear();
  tx_out_indices.reserve(global_indices.size());

  TXN_PREFIX_RDONLY();
  RCURSOR(output_txs);

  for (const uint64_t &output_id : global_indices)
  {
    MDB_val_set(v, output_id);

    auto get_result = mdb_cursor_get(m_cur_output_txs, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
    if (get_result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("output with given index not in db"));
    else if (get_result)
      throw0(DB_ERROR("DB error attempting to fetch output tx hash"));

    const outtx *ot = (const outtx *)v.mv_data;
    tx_out_indices.push_back(tx_out_index(ot->tx_hash, ot->local_index));
  }

  TXN_POSTFIX_RDONLY();
}

void BlockchainLMDB::get_output_key(const epee::span<const uint64_t> &amounts, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs, bool allow_partial) const
{
  if (amounts.size() != 1 && amounts.size() != offsets.size())
    throw0(DB_ERROR("Invalid sizes of amounts and offsets"));

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  TIME_MEASURE_START(db3);
  check_open();
  outputs.clear();
  outputs.reserve(offsets.size());

  TXN_PREFIX_RDONLY();

  RCURSOR(output_amounts);

  for (size_t i = 0; i < offsets.size(); ++i)
  {
    const uint64_t amount = amounts.size() == 1 ? amounts[0] : amounts[i];
    MDB_val_set(k, amount);
    MDB_val_set(v, offsets[i]);

    auto get_result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_BOTH);
    if (get_result == MDB_NOTFOUND)
    {
      if (allow_partial)
      {
        MDEBUG("Partial result: " << outputs.size() << "/" << offsets.size());
        break;
      }
      throw1(OUTPUT_DNE((std::string("Attempting to get output pubkey by global index (amount ") + boost::lexical_cast<std::string>(amount) + ", index " + boost::lexical_cast<std::string>(offsets[i]) + ", count " + boost::lexical_cast<std::string>(get_num_outputs(amount)) + "), but key does not exist (current height " + boost::lexical_cast<std::string>(height()) + ")").c_str()));
    }
    else if (get_result)
      throw0(DB_ERROR(lmdb_error("Error attempting to retrieve an output pubkey from the db", get_result).c_str()));

    if (amount == 0)
    {
      const outkey *okp = (const outkey *)v.mv_data;
      outputs.push_back(okp->data);
    }
    else
    {
      const pre_rct_outkey *okp = (const pre_rct_outkey *)v.mv_data;
      outputs.resize(outputs.size() + 1);
      output_data_t &data = outputs.back();
      memcpy(&data, &okp->data, sizeof(pre_rct_output_data_t));
      data.commitment = rct::zeroCommitVartime(amount);
    }
  }

  TXN_POSTFIX_RDONLY();

  TIME_MEASURE_FINISH(db3);
  LOG_PRINT_L3("db3: " << db3);
}

void BlockchainLMDB::get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  indices.clear();

  std::vector <uint64_t> tx_indices;
  tx_indices.reserve(offsets.size());
  TXN_PREFIX_RDONLY();

  RCURSOR(output_amounts);

  MDB_val_set(k, amount);
  for (const uint64_t &index : offsets)
  {
    MDB_val_set(v, index);

    auto get_result = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_GET_BOTH);
    if (get_result == MDB_NOTFOUND)
      throw1(OUTPUT_DNE("Attempting to get output by index, but key does not exist"));
    else if (get_result)
      throw0(DB_ERROR(lmdb_error("Error attempting to retrieve an output from the db", get_result).c_str()));

    const outkey *okp = (const outkey *)v.mv_data;
    tx_indices.push_back(okp->output_id);
  }

  TIME_MEASURE_START(db3);
  if(tx_indices.size() > 0)
  {
    get_output_tx_and_index_from_global(tx_indices, indices);
  }
  TIME_MEASURE_FINISH(db3);
  LOG_PRINT_L3("db3: " << db3);
}

std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> BlockchainLMDB::get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff, uint64_t min_count) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> histogram;
  MDB_val k;
  MDB_val v;

  if (amounts.empty())
  {
    MDB_cursor_op op = MDB_FIRST;
    while (1)
    {
      int ret = mdb_cursor_get(m_cur_output_amounts, &k, &v, op);
      op = MDB_NEXT_NODUP;
      if (ret == MDB_NOTFOUND)
        break;
      if (ret)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate outputs: ", ret).c_str()));
      mdb_size_t num_elems = 0;
      mdb_cursor_count(m_cur_output_amounts, &num_elems);
      uint64_t amount = *(const uint64_t*)k.mv_data;
      if (num_elems >= min_count)
        histogram[amount] = std::make_tuple(num_elems, 0, 0);
    }
  }
  else
  {
    for (const auto &amount: amounts)
    {
      MDB_val_copy<uint64_t> k(amount);
      int ret = mdb_cursor_get(m_cur_output_amounts, &k, &v, MDB_SET);
      if (ret == MDB_NOTFOUND)
      {
        if (0 >= min_count)
          histogram[amount] = std::make_tuple(0, 0, 0);
      }
      else if (ret == MDB_SUCCESS)
      {
        mdb_size_t num_elems = 0;
        mdb_cursor_count(m_cur_output_amounts, &num_elems);
        if (num_elems >= min_count)
          histogram[amount] = std::make_tuple(num_elems, 0, 0);
      }
      else
      {
        throw0(DB_ERROR(lmdb_error("Failed to enumerate outputs: ", ret).c_str()));
      }
    }
  }

  if (unlocked || recent_cutoff > 0) {
    const uint64_t blockchain_height = height();
    for (std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>>::iterator i = histogram.begin(); i != histogram.end(); ++i) {
      uint64_t amount = i->first;
      uint64_t num_elems = std::get<0>(i->second);
      while (num_elems > 0) {
        const tx_out_index toi = get_output_tx_and_index(amount, num_elems - 1);
        const uint64_t height = get_tx_block_height(toi.first);
        if (height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE <= blockchain_height)
          break;
        --num_elems;
      }
      // modifying second does not invalidate the iterator
      std::get<1>(i->second) = num_elems;

      if (recent_cutoff > 0)
      {
        uint64_t recent = 0;
        while (num_elems > 0) {
          const tx_out_index toi = get_output_tx_and_index(amount, num_elems - 1);
          const uint64_t height = get_tx_block_height(toi.first);
          const uint64_t ts = get_block_timestamp(height);
          if (ts < recent_cutoff)
            break;
          --num_elems;
          ++recent;
        }
        // modifying second does not invalidate the iterator
        std::get<2>(i->second) = recent;
      }
    }
  }

  TXN_POSTFIX_RDONLY();

  return histogram;
}

bool BlockchainLMDB::get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, std::vector<uint64_t> &distribution, uint64_t &base) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(output_amounts);

  distribution.clear();
  const uint64_t db_height = height();
  if (from_height >= db_height)
    return false;
  distribution.resize(db_height - from_height, 0);

  MDB_val_set(k, amount);
  MDB_val v;
  MDB_cursor_op op = MDB_SET;
  base = 0;
  while (1)
  {
    int ret = mdb_cursor_get(m_cur_output_amounts, &k, &v, op);
    op = MDB_NEXT_DUP;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw0(DB_ERROR("Failed to enumerate outputs"));
    const outkey *ok = (const outkey *)v.mv_data;
    const uint64_t height = ok->data.height;
    if (height >= from_height)
      distribution[height - from_height]++;
    else
      base++;
    if (to_height > 0 && height > to_height)
      break;
  }

  distribution[0] += base;
  for (size_t n = 1; n < distribution.size(); ++n)
    distribution[n] += distribution[n - 1];
  base = 0;

  TXN_POSTFIX_RDONLY();

  return true;
}

void BlockchainLMDB::check_hard_fork_info()
{
}

void BlockchainLMDB::drop_hard_fork_info()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX(0);

  auto result = mdb_drop(*txn_ptr, m_hf_starting_heights, 1);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error dropping hard fork starting heights db: ", result).c_str()));
  result = mdb_drop(*txn_ptr, m_hf_versions, 1);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error dropping hard fork versions db: ", result).c_str()));

  TXN_POSTFIX_SUCCESS();
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
    throw1(DB_ERROR(lmdb_error("Error adding hard fork version to db transaction: ", result).c_str()));

  TXN_BLOCK_POSTFIX_SUCCESS();
}

uint8_t BlockchainLMDB::get_hard_fork_version(uint64_t height) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(hf_versions);

  MDB_val_copy<uint64_t> val_key(height);
  MDB_val val_ret;
  auto result = mdb_cursor_get(m_cur_hf_versions, &val_key, &val_ret, MDB_SET);
  if (result == MDB_NOTFOUND || result)
    throw0(DB_ERROR(lmdb_error("Error attempting to retrieve a hard fork version at height " + boost::lexical_cast<std::string>(height) + " from the db: ", result).c_str()));

  uint8_t ret = *(const uint8_t*)val_ret.mv_data;
  TXN_POSTFIX_RDONLY();
  return ret;
}

void BlockchainLMDB::add_alt_block(const crypto::hash &blkid, const cryptonote::alt_block_data_t &data, const cryptonote::blobdata_ref &blob)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(alt_blocks)

  MDB_val k = {sizeof(blkid), (void *)&blkid};
  const size_t val_size = sizeof(alt_block_data_t) + blob.size();
  std::unique_ptr<char[]> val(new char[val_size]);
  memcpy(val.get(), &data, sizeof(alt_block_data_t));
  memcpy(val.get() + sizeof(alt_block_data_t), blob.data(), blob.size());
  MDB_val v = {val_size, (void *)val.get()};
  if (auto result = mdb_cursor_put(m_cur_alt_blocks, &k, &v, MDB_NODUPDATA)) {
    if (result == MDB_KEYEXIST)
      throw1(DB_ERROR("Attempting to add alternate block that's already in the db"));
    else
      throw1(DB_ERROR(lmdb_error("Error adding alternate block to db transaction: ", result).c_str()));
  }
}

bool BlockchainLMDB::get_alt_block(const crypto::hash &blkid, alt_block_data_t *data, cryptonote::blobdata *blob)
{
  LOG_PRINT_L3("BlockchainLMDB:: " << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(alt_blocks);

  MDB_val_set(k, blkid);
  MDB_val v;
  int result = mdb_cursor_get(m_cur_alt_blocks, &k, &v, MDB_SET);
  if (result == MDB_NOTFOUND)
    return false;

  if (result)
    throw0(DB_ERROR(lmdb_error("Error attempting to retrieve alternate block " + epee::string_tools::pod_to_hex(blkid) + " from the db: ", result).c_str()));
  if (v.mv_size < sizeof(alt_block_data_t))
    throw0(DB_ERROR("Record size is less than expected"));

  const alt_block_data_t *ptr = (const alt_block_data_t*)v.mv_data;
  if (data)
    *data = *ptr;
  if (blob)
    blob->assign((const char*)(ptr + 1), v.mv_size - sizeof(alt_block_data_t));

  TXN_POSTFIX_RDONLY();
  return true;
}

void BlockchainLMDB::remove_alt_block(const crypto::hash &blkid)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_txn_cursors *m_cursors = &m_wcursors;

  CURSOR(alt_blocks)

  MDB_val k = {sizeof(blkid), (void *)&blkid};
  MDB_val v;
  int result = mdb_cursor_get(m_cur_alt_blocks, &k, &v, MDB_SET);
  if (result)
    throw0(DB_ERROR(lmdb_error("Error locating alternate block " + epee::string_tools::pod_to_hex(blkid) + " in the db: ", result).c_str()));
  result = mdb_cursor_del(m_cur_alt_blocks, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Error deleting alternate block " + epee::string_tools::pod_to_hex(blkid) + " from the db: ", result).c_str()));
}

uint64_t BlockchainLMDB::get_alt_block_count()
{
  LOG_PRINT_L3("BlockchainLMDB:: " << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(alt_blocks);

  MDB_stat db_stats;
  int result = mdb_stat(m_txn, m_alt_blocks, &db_stats);
  uint64_t count = 0;
  if (result != MDB_NOTFOUND)
  {
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to query m_alt_blocks: ", result).c_str()));
    count = db_stats.ms_entries;
  }
  TXN_POSTFIX_RDONLY();
  return count;
}

void BlockchainLMDB::drop_alt_blocks()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX(0);

  auto result = mdb_drop(*txn_ptr, m_alt_blocks, 0);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error dropping alternative blocks: ", result).c_str()));

  TXN_POSTFIX_SUCCESS();
}

bool BlockchainLMDB::is_read_only() const
{
  unsigned int flags;
  auto result = mdb_env_get_flags(m_env, &flags);
  if (result)
    throw0(DB_ERROR(lmdb_error("Error getting database environment info: ", result).c_str()));

  if (flags & MDB_RDONLY)
    return true;

  return false;
}

uint64_t BlockchainLMDB::get_database_size() const
{
  boost::filesystem::path datafile(m_folder);
  datafile /= CRYPTONOTE_BLOCKCHAINDATA_FILENAME;
  boost::system::error_code ec{};
  const boost::uintmax_t size = boost::filesystem::file_size(datafile, ec);
  return (ec ? 0 : static_cast<uint64_t>(size));
}

void BlockchainLMDB::fixup()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // Always call parent as well
  BlockchainDB::fixup();
}

#define RENAME_DB(name) do { \
    char n2[] = name; \
    MDB_dbi tdbi; \
    n2[sizeof(n2)-2]--; \
    /* play some games to put (name) on a writable page */ \
    result = mdb_dbi_open(txn, n2, MDB_CREATE, &tdbi); \
    if (result) \
      throw0(DB_ERROR(lmdb_error("Failed to create " + std::string(n2) + ": ", result).c_str())); \
    result = mdb_drop(txn, tdbi, 1); \
    if (result) \
      throw0(DB_ERROR(lmdb_error("Failed to delete " + std::string(n2) + ": ", result).c_str())); \
    k.mv_data = (void *)name; \
    k.mv_size = sizeof(name)-1; \
    result = mdb_cursor_open(txn, 1, &c_cur); \
    if (result) \
      throw0(DB_ERROR(lmdb_error("Failed to open a cursor for " name ": ", result).c_str())); \
    result = mdb_cursor_get(c_cur, &k, NULL, MDB_SET_KEY); \
    if (result) \
      throw0(DB_ERROR(lmdb_error("Failed to get DB record for " name ": ", result).c_str())); \
    ptr = (char *)k.mv_data; \
    ptr[sizeof(name)-2]++; } while(0)

#define LOGIF(y)    if (ELPP->vRegistry()->allowed(y, "global"))

void BlockchainLMDB::migrate_0_1()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i, z, m_height;
  int result;
  mdb_txn_safe txn(false);
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 0 to 1 - this may take a while:");
  MINFO("updating blocks, hf_versions, outputs, txs, and spent_keys tables...");

  do {
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_blocks, &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
    m_height = db_stats.ms_entries;
    MINFO("Total number of blocks: " << m_height);
    MINFO("block migration will update block_heights, block_info, and hf_versions...");

    MINFO("migrating block_heights:");
    MDB_dbi o_heights;

    unsigned int flags;
    result = mdb_dbi_flags(txn, m_block_heights, &flags);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to retrieve block_heights flags: ", result).c_str()));
    /* if the flags are what we expect, this table has already been migrated */
    if ((flags & (MDB_INTEGERKEY|MDB_DUPSORT|MDB_DUPFIXED)) == (MDB_INTEGERKEY|MDB_DUPSORT|MDB_DUPFIXED)) {
      txn.abort();
      LOG_PRINT_L1("  block_heights already migrated");
      break;
    }

    /* the block_heights table name is the same but the old version and new version
     * have incompatible DB flags. Create a new table with the right flags. We want
     * the name to be similar to the old name so that it will occupy the same location
     * in the DB.
     */
    o_heights = m_block_heights;
    lmdb_db_open(txn, "block_heightr", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_heights, "Failed to open db handle for block_heightr");
    mdb_set_dupsort(txn, m_block_heights, compare_hash32);

    MDB_cursor *c_old, *c_cur;
    blk_height bh;
    MDB_val_set(nv, bh);

    /* old table was k(hash), v(height).
     * new table is DUPFIXED, k(zeroval), v{hash, height}.
     */
    i = 0;
    z = m_height;
    while(1) {
      if (!(i % 2000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << z << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_block_heights, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_heightr: ", result).c_str()));
        result = mdb_cursor_open(txn, o_heights, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_heights: ", result).c_str()));
        if (!i) {
          MDB_stat ms;
          result = mdb_stat(txn, m_block_heights, &ms);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query block_heights table: ", result).c_str()));
          i = ms.ms_entries;
        }
      }
      result = mdb_cursor_get(c_old, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_heights: ", result).c_str()));
      bh.bh_hash = *(crypto::hash *)k.mv_data;
      bh.bh_height = *(uint64_t *)v.mv_data;
      result = mdb_cursor_put(c_cur, (MDB_val *)&zerokval, &nv, MDB_APPENDDUP);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into block_heightr: ", result).c_str()));
      /* we delete the old records immediately, so the overall DB and mapsize should not grow.
       * This is a little slower than just letting mdb_drop() delete it all at the end, but
       * it saves a significant amount of disk space.
       */
      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_heights: ", result).c_str()));
      i++;
    }

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    /* Delete the old table */
    result = mdb_drop(txn, o_heights, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_heights table: ", result).c_str()));

    RENAME_DB("block_heightr");

    /* close and reopen to get old dbi slot back */
    mdb_dbi_close(m_env, m_block_heights);
    lmdb_db_open(txn, "block_heights", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, m_block_heights, "Failed to open db handle for block_heights");
    mdb_set_dupsort(txn, m_block_heights, compare_hash32);
    txn.commit();

  } while(0);

  /* old tables are k(height), v(value).
   * new table is DUPFIXED, k(zeroval), v{height, values...}.
   */
  do {
    LOG_PRINT_L1("migrating block info:");

    MDB_dbi coins;
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    result = mdb_dbi_open(txn, "block_coins", 0, &coins);
    if (result == MDB_NOTFOUND) {
      txn.abort();
      LOG_PRINT_L1("  block_info already migrated");
      break;
    }
    MDB_dbi diffs, hashes, sizes, timestamps;
    mdb_block_info_1 bi;
    MDB_val_set(nv, bi);

    lmdb_db_open(txn, "block_diffs", 0, diffs, "Failed to open db handle for block_diffs");
    lmdb_db_open(txn, "block_hashes", 0, hashes, "Failed to open db handle for block_hashes");
    lmdb_db_open(txn, "block_sizes", 0, sizes, "Failed to open db handle for block_sizes");
    lmdb_db_open(txn, "block_timestamps", 0, timestamps, "Failed to open db handle for block_timestamps");
    MDB_cursor *c_cur, *c_coins, *c_diffs, *c_hashes, *c_sizes, *c_timestamps;
    i = 0;
    z = m_height;
    while(1) {
      MDB_val k, v;
      if (!(i % 2000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << z << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_block_info, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        result = mdb_cursor_open(txn, coins, &c_coins);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_coins: ", result).c_str()));
        result = mdb_cursor_open(txn, diffs, &c_diffs);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_diffs: ", result).c_str()));
        result = mdb_cursor_open(txn, hashes, &c_hashes);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_hashes: ", result).c_str()));
        result = mdb_cursor_open(txn, sizes, &c_sizes);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_coins: ", result).c_str()));
        result = mdb_cursor_open(txn, timestamps, &c_timestamps);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_timestamps: ", result).c_str()));
        if (!i) {
          MDB_stat ms;
          result = mdb_stat(txn, m_block_info, &ms);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query block_info table: ", result).c_str()));
          i = ms.ms_entries;
        }
      }
      result = mdb_cursor_get(c_coins, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        break;
      } else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_coins: ", result).c_str()));
      bi.bi_height = *(uint64_t *)k.mv_data;
      bi.bi_coins = *(uint64_t *)v.mv_data;
      result = mdb_cursor_get(c_diffs, &k, &v, MDB_NEXT);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_diffs: ", result).c_str()));
      bi.bi_diff = *(uint64_t *)v.mv_data;
      result = mdb_cursor_get(c_hashes, &k, &v, MDB_NEXT);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_hashes: ", result).c_str()));
      bi.bi_hash = *(crypto::hash *)v.mv_data;
      result = mdb_cursor_get(c_sizes, &k, &v, MDB_NEXT);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_sizes: ", result).c_str()));
      if (v.mv_size == sizeof(uint32_t))
        bi.bi_weight = *(uint32_t *)v.mv_data;
      else
        bi.bi_weight = *(uint64_t *)v.mv_data;  // this is a 32/64 compat bug in version 0
      result = mdb_cursor_get(c_timestamps, &k, &v, MDB_NEXT);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_timestamps: ", result).c_str()));
      bi.bi_timestamp = *(uint64_t *)v.mv_data;
      result = mdb_cursor_put(c_cur, (MDB_val *)&zerokval, &nv, MDB_APPENDDUP);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into block_info: ", result).c_str()));
      result = mdb_cursor_del(c_coins, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_coins: ", result).c_str()));
      result = mdb_cursor_del(c_diffs, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_diffs: ", result).c_str()));
      result = mdb_cursor_del(c_hashes, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_hashes: ", result).c_str()));
      result = mdb_cursor_del(c_sizes, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_sizes: ", result).c_str()));
      result = mdb_cursor_del(c_timestamps, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_timestamps: ", result).c_str()));
      i++;
    }
    mdb_cursor_close(c_timestamps);
    mdb_cursor_close(c_sizes);
    mdb_cursor_close(c_hashes);
    mdb_cursor_close(c_diffs);
    mdb_cursor_close(c_coins);
    result = mdb_drop(txn, timestamps, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete block_timestamps from the db: ", result).c_str()));
    result = mdb_drop(txn, sizes, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete block_sizes from the db: ", result).c_str()));
    result = mdb_drop(txn, hashes, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete block_hashes from the db: ", result).c_str()));
    result = mdb_drop(txn, diffs, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete block_diffs from the db: ", result).c_str()));
    result = mdb_drop(txn, coins, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete block_coins from the db: ", result).c_str()));
    txn.commit();
  } while(0);

  do {
    LOG_PRINT_L1("migrating hf_versions:");
    MDB_dbi o_hfv;

    unsigned int flags;
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    result = mdb_dbi_flags(txn, m_hf_versions, &flags);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to retrieve hf_versions flags: ", result).c_str()));
    /* if the flags are what we expect, this table has already been migrated */
    if (flags & MDB_INTEGERKEY) {
      txn.abort();
      LOG_PRINT_L1("  hf_versions already migrated");
      break;
    }

    /* the hf_versions table name is the same but the old version and new version
     * have incompatible DB flags. Create a new table with the right flags.
     */
    o_hfv = m_hf_versions;
    lmdb_db_open(txn, "hf_versionr", MDB_INTEGERKEY | MDB_CREATE, m_hf_versions, "Failed to open db handle for hf_versionr");

    MDB_cursor *c_old, *c_cur;
    i = 0;
    z = m_height;

    while(1) {
      if (!(i % 2000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << z << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_hf_versions, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for spent_keyr: ", result).c_str()));
        result = mdb_cursor_open(txn, o_hfv, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for spent_keys: ", result).c_str()));
        if (!i) {
          MDB_stat ms;
          result = mdb_stat(txn, m_hf_versions, &ms);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query hf_versions table: ", result).c_str()));
          i = ms.ms_entries;
        }
      }
      result = mdb_cursor_get(c_old, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from hf_versions: ", result).c_str()));
      result = mdb_cursor_put(c_cur, &k, &v, MDB_APPEND);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into hf_versionr: ", result).c_str()));
      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from hf_versions: ", result).c_str()));
      i++;
    }

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    /* Delete the old table */
    result = mdb_drop(txn, o_hfv, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old hf_versions table: ", result).c_str()));
    RENAME_DB("hf_versionr");
    mdb_dbi_close(m_env, m_hf_versions);
    lmdb_db_open(txn, "hf_versions", MDB_INTEGERKEY, m_hf_versions, "Failed to open db handle for hf_versions");

    txn.commit();
  } while(0);

  do {
    LOG_PRINT_L1("deleting old indices:");

    /* Delete all other tables, we're just going to recreate them */
    MDB_dbi dbi;
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    result = mdb_dbi_open(txn, "tx_unlocks", 0, &dbi);
    if (result == MDB_NOTFOUND) {
        txn.abort();
        LOG_PRINT_L1("  old indices already deleted");
        break;
    }
    txn.abort();

#define DELETE_DB(x) do {   \
    LOG_PRINT_L1("  " x ":"); \
    result = mdb_txn_begin(m_env, NULL, 0, txn); \
    if (result) \
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str())); \
    result = mdb_dbi_open(txn, x, 0, &dbi); \
    if (!result) { \
      result = mdb_drop(txn, dbi, 1); \
      if (result) \
        throw0(DB_ERROR(lmdb_error("Failed to delete " x ": ", result).c_str())); \
    txn.commit(); \
    } } while(0)

    DELETE_DB("tx_heights");
    DELETE_DB("output_txs");
    DELETE_DB("output_indices");
    DELETE_DB("output_keys");
    DELETE_DB("spent_keys");
    DELETE_DB("output_amounts");
    DELETE_DB("tx_outputs");
    DELETE_DB("tx_unlocks");

    /* reopen new DBs with correct flags */
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    lmdb_db_open(txn, LMDB_OUTPUT_TXS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_output_txs, "Failed to open db handle for m_output_txs");
    mdb_set_dupsort(txn, m_output_txs, compare_uint64);
    lmdb_db_open(txn, LMDB_TX_OUTPUTS, MDB_INTEGERKEY | MDB_CREATE, m_tx_outputs, "Failed to open db handle for m_tx_outputs");
    lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_spent_keys, "Failed to open db handle for m_spent_keys");
    mdb_set_dupsort(txn, m_spent_keys, compare_hash32);
    lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_output_amounts, "Failed to open db handle for m_output_amounts");
    mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
    txn.commit();
  } while(0);

  do {
    LOG_PRINT_L1("migrating txs and outputs:");

    unsigned int flags;
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    result = mdb_dbi_flags(txn, m_txs, &flags);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to retrieve txs flags: ", result).c_str()));
    /* if the flags are what we expect, this table has already been migrated */
    if (flags & MDB_INTEGERKEY) {
      txn.abort();
      LOG_PRINT_L1("  txs already migrated");
      break;
    }

    MDB_dbi o_txs;
    blobdata_ref bd;
    block b;
    MDB_val hk;

    o_txs = m_txs;
    mdb_set_compare(txn, o_txs, compare_hash32);
    lmdb_db_open(txn, "txr", MDB_INTEGERKEY | MDB_CREATE, m_txs, "Failed to open db handle for txr");

    txn.commit();

    MDB_cursor *c_blocks, *c_txs, *c_props, *c_cur;
    i = 0;
    z = m_height;

    hk.mv_size = sizeof(crypto::hash);
    set_batch_transactions(true);
    batch_start(1000);
    txn.m_txn = m_write_txn->m_txn;
    m_height = 0;

    while(1) {
      if (!(i % 1000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << z << "  \r" << std::flush;
          }
          MDB_val_set(pk, "txblk");
          MDB_val_set(pv, m_height);
          result = mdb_cursor_put(c_props, &pk, &pv, 0);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to update txblk property: ", result).c_str()));
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
          m_write_txn->m_txn = txn.m_txn;
          m_write_batch_txn->m_txn = txn.m_txn;
          memset(&m_wcursors, 0, sizeof(m_wcursors));
        }
        result = mdb_cursor_open(txn, m_blocks, &c_blocks);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));
        result = mdb_cursor_open(txn, m_properties, &c_props);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for properties: ", result).c_str()));
        result = mdb_cursor_open(txn, o_txs, &c_txs);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs: ", result).c_str()));
        if (!i) {
          MDB_stat ms;
          result = mdb_stat(txn, m_txs, &ms);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query txs table: ", result).c_str()));
          i = ms.ms_entries;
          if (i) {
            MDB_val_set(pk, "txblk");
            result = mdb_cursor_get(c_props, &pk, &k, MDB_SET);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to get a record from properties: ", result).c_str()));
            m_height = *(uint64_t *)k.mv_data;
          }
        }
        if (i) {
          result = mdb_cursor_get(c_blocks, &k, &v, MDB_SET);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to get a record from blocks: ", result).c_str()));
        }
      }
      result = mdb_cursor_get(c_blocks, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        MDB_val_set(pk, "txblk");
        result = mdb_cursor_get(c_props, &pk, &v, MDB_SET);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to get a record from props: ", result).c_str()));
        result = mdb_cursor_del(c_props, 0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to delete a record from props: ", result).c_str()));
        batch_stop();
        break;
      } else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from blocks: ", result).c_str()));

      bd = {reinterpret_cast<char*>(v.mv_data), v.mv_size};
      if (!parse_and_validate_block_from_blob(bd, b))
        throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

      const auto miner_blob = tx_to_blob(b.miner_tx);
      add_transaction(null_hash, b.miner_tx, epee::strspan<std::uint8_t>(miner_blob));
      for (unsigned int j = 0; j<b.tx_hashes.size(); j++) {
        transaction tx;
        hk.mv_data = &b.tx_hashes[j];
        result = mdb_cursor_get(c_txs, &hk, &v, MDB_SET);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to get record from txs: ", result).c_str()));
        bd = {reinterpret_cast<char*>(v.mv_data), v.mv_size};
        if (!parse_and_validate_tx_from_blob(bd, tx))
          throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
        add_transaction(null_hash, std::move(tx), epee::strspan<std::uint8_t>(bd), &b.tx_hashes[j]);
        result = mdb_cursor_del(c_txs, 0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to get record from txs: ", result).c_str()));
      }
      i++;
      m_height = i;
    }
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    result = mdb_drop(txn, o_txs, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete txs from the db: ", result).c_str()));

    RENAME_DB("txr");

    mdb_dbi_close(m_env, m_txs);

    lmdb_db_open(txn, "txs", MDB_INTEGERKEY, m_txs, "Failed to open db handle for txs");

    txn.commit();
  } while(0);

  uint32_t version = 1;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_1_2()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  mdb_txn_safe txn(false);
  MDB_val v;

  MGINFO_YELLOW("Migrating blockchain from DB version 1 to 2 - this may take a while:");
  MINFO("updating txs_pruned and txs_prunable tables...");

  do {
    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    MDB_stat db_stats_txs;
    MDB_stat db_stats_txs_pruned;
    MDB_stat db_stats_txs_prunable;
    MDB_stat db_stats_txs_prunable_hash;
    if ((result = mdb_stat(txn, m_txs, &db_stats_txs)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_txs: ", result).c_str()));
    if ((result = mdb_stat(txn, m_txs_pruned, &db_stats_txs_pruned)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_txs_pruned: ", result).c_str()));
    if ((result = mdb_stat(txn, m_txs_prunable, &db_stats_txs_prunable)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_txs_prunable: ", result).c_str()));
    if ((result = mdb_stat(txn, m_txs_prunable_hash, &db_stats_txs_prunable_hash)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_txs_prunable_hash: ", result).c_str()));
    if (db_stats_txs_pruned.ms_entries != db_stats_txs_prunable.ms_entries)
      throw0(DB_ERROR("Mismatched sizes for txs_pruned and txs_prunable"));
    if (db_stats_txs_pruned.ms_entries == db_stats_txs.ms_entries)
    {
      txn.commit();
      MINFO("txs already migrated");
      break;
    }

    MINFO("updating txs tables:");

    MDB_cursor *c_old, *c_cur0, *c_cur1, *c_cur2;
    i = 0;

    while(1) {
      if (!(i % 1000)) {
        if (i) {
          result = mdb_stat(txn, m_txs, &db_stats_txs);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query m_txs: ", result).c_str()));
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << (i + db_stats_txs.ms_entries) << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_txs_pruned, &c_cur0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs_prunable, &c_cur1);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs_prunable_hash, &c_cur2);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_hash: ", result).c_str()));
        result = mdb_cursor_open(txn, m_txs, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs: ", result).c_str()));
        if (!i) {
          i = db_stats_txs_pruned.ms_entries;
        }
      }
      MDB_val_set(k, i);
      result = mdb_cursor_get(c_old, &k, &v, MDB_SET);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from txs: ", result).c_str()));

      cryptonote::blobdata bd{reinterpret_cast<char*>(v.mv_data), v.mv_size};
      transaction tx;
      if (!parse_and_validate_tx_from_blob(bd, tx))
        throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
      std::stringstream ss;
      binary_archive<true> ba(ss);
      bool r = tx.serialize_base(ba);
      if (!r)
        throw0(DB_ERROR("Failed to serialize pruned tx"));
      std::string pruned = ss.str();

      if (pruned.size() > bd.size())
        throw0(DB_ERROR("Pruned tx is larger than raw tx"));
      if (memcmp(pruned.data(), bd.data(), pruned.size()))
        throw0(DB_ERROR("Pruned tx is not a prefix of the raw tx"));

      MDB_val nv;
      nv.mv_data = (void*)pruned.data();
      nv.mv_size = pruned.size();
      result = mdb_cursor_put(c_cur0, (MDB_val *)&k, &nv, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into txs_pruned: ", result).c_str()));

      nv.mv_data = (void*)(bd.data() + pruned.size());
      nv.mv_size = bd.size() - pruned.size();
      result = mdb_cursor_put(c_cur1, (MDB_val *)&k, &nv, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into txs_prunable: ", result).c_str()));

      if (tx.version > 1)
      {
        crypto::hash prunable_hash = get_transaction_prunable_hash(tx);
        MDB_val_set(val_prunable_hash, prunable_hash);
        result = mdb_cursor_put(c_cur2, (MDB_val *)&k, &val_prunable_hash, 0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to put a record into txs_prunable_hash: ", result).c_str()));
      }

      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from txs: ", result).c_str()));

      i++;
    }
  } while(0);

  uint32_t version = 2;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_2_3()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  mdb_txn_safe txn(false);
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 2 to 3 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_blocks, &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
    const uint64_t blockchain_height = db_stats.ms_entries;

    MDEBUG("enumerating rct outputs...");
    std::vector<uint64_t> distribution(blockchain_height, 0);
    bool r = for_all_outputs(0, [&](uint64_t height) {
      if (height >= blockchain_height)
      {
        MERROR("Output found claiming height >= blockchain height");
        return false;
      }
      distribution[height]++;
      return true;
    });
    if (!r)
      throw0(DB_ERROR("Failed to build rct output distribution"));
    for (size_t i = 1; i < distribution.size(); ++i)
      distribution[i] += distribution[i - 1];

    /* the block_info table name is the same but the old version and new version
     * have incompatible data. Create a new table. We want the name to be similar
     * to the old name so that it will occupy the same location in the DB.
     */
    MDB_dbi o_block_info = m_block_info;
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);

    MDB_cursor *c_old, *c_cur;
    i = 0;
    while(1) {
      if (!(i % 1000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << blockchain_height << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_block_info, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        if (!i) {
          result = mdb_stat(txn, m_block_info, &db_stats);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query m_block_info: ", result).c_str()));
          i = db_stats.ms_entries;
        }
      }
      result = mdb_cursor_get(c_old, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_info: ", result).c_str()));
      const mdb_block_info_1 *bi_old = (const mdb_block_info_1*)v.mv_data;
      mdb_block_info_2 bi;
      bi.bi_height = bi_old->bi_height;
      bi.bi_timestamp = bi_old->bi_timestamp;
      bi.bi_coins = bi_old->bi_coins;
      bi.bi_weight = bi_old->bi_weight;
      bi.bi_diff = bi_old->bi_diff;
      bi.bi_hash = bi_old->bi_hash;
      if (bi_old->bi_height >= distribution.size())
        throw0(DB_ERROR("Bad height in block_info record"));
      bi.bi_cum_rct = distribution[bi_old->bi_height];
      MDB_val_set(nv, bi);
      result = mdb_cursor_put(c_cur, (MDB_val *)&zerokval, &nv, MDB_APPENDDUP);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into block_infn: ", result).c_str()));
      /* we delete the old records immediately, so the overall DB and mapsize should not grow.
       * This is a little slower than just letting mdb_drop() delete it all at the end, but
       * it saves a significant amount of disk space.
       */
      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_info: ", result).c_str()));
      i++;
    }

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_block_info);

    lmdb_db_open(txn, "block_info", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 3;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_3_4()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  mdb_txn_safe txn(false);
  MDB_val k, v;
  char *ptr;
  bool past_long_term_weight = false;

  MGINFO_YELLOW("Migrating blockchain from DB version 3 to 4 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_blocks, &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
    const uint64_t blockchain_height = db_stats.ms_entries;

    boost::circular_buffer<uint64_t> long_term_block_weights(CRYPTONOTE_LONG_TERM_BLOCK_WEIGHT_WINDOW_SIZE);

    /* the block_info table name is the same but the old version and new version
     * have incompatible data. Create a new table. We want the name to be similar
     * to the old name so that it will occupy the same location in the DB.
     */
    MDB_dbi o_block_info = m_block_info;
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);


    MDB_cursor *c_blocks;
    result = mdb_cursor_open(txn, m_blocks, &c_blocks);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));

    MDB_cursor *c_old, *c_cur;
    i = 0;
    while(1) {
      if (!(i % 1000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << blockchain_height << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_block_info, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        result = mdb_cursor_open(txn, m_blocks, &c_blocks);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));
        if (!i) {
          result = mdb_stat(txn, m_block_info, &db_stats);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query m_block_info: ", result).c_str()));
          i = db_stats.ms_entries;
        }
      }
      result = mdb_cursor_get(c_old, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_info: ", result).c_str()));
      const mdb_block_info_2 *bi_old = (const mdb_block_info_2*)v.mv_data;
      mdb_block_info_3 bi;
      bi.bi_height = bi_old->bi_height;
      bi.bi_timestamp = bi_old->bi_timestamp;
      bi.bi_coins = bi_old->bi_coins;
      bi.bi_weight = bi_old->bi_weight;
      bi.bi_diff = bi_old->bi_diff;
      bi.bi_hash = bi_old->bi_hash;
      bi.bi_cum_rct = bi_old->bi_cum_rct;

      // get block major version to determine which rule is in place
      if (!past_long_term_weight)
      {
        MDB_val_copy<uint64_t> kb(bi.bi_height);
        MDB_val vb;
        result = mdb_cursor_get(c_blocks, &kb, &vb, MDB_SET);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
        if (vb.mv_size == 0)
          throw0(DB_ERROR("Invalid data from m_blocks"));
        const uint8_t block_major_version = *((const uint8_t*)vb.mv_data);
        if (block_major_version >= HF_VERSION_LONG_TERM_BLOCK_WEIGHT)
          past_long_term_weight = true;
      }

      uint64_t long_term_block_weight;
      if (past_long_term_weight)
      {
        std::vector<uint64_t> weights(long_term_block_weights.begin(), long_term_block_weights.end());
        uint64_t long_term_effective_block_median_weight = std::max<uint64_t>(CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V5, epee::misc_utils::median(weights));
        long_term_block_weight = std::min<uint64_t>(bi.bi_weight, long_term_effective_block_median_weight + long_term_effective_block_median_weight * 2 / 5);
      }
      else
      {
        long_term_block_weight = bi.bi_weight;
      }
      long_term_block_weights.push_back(long_term_block_weight);
      bi.bi_long_term_block_weight = long_term_block_weight;

      MDB_val_set(nv, bi);
      result = mdb_cursor_put(c_cur, (MDB_val *)&zerokval, &nv, MDB_APPENDDUP);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into block_infn: ", result).c_str()));
      /* we delete the old records immediately, so the overall DB and mapsize should not grow.
       * This is a little slower than just letting mdb_drop() delete it all at the end, but
       * it saves a significant amount of disk space.
       */
      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_info: ", result).c_str()));
      i++;
    }

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_block_info);

    lmdb_db_open(txn, "block_info", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 4;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_4_5()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  mdb_txn_safe txn(false);
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 4 to 5 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_blocks, &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
    const uint64_t blockchain_height = db_stats.ms_entries;

    /* the block_info table name is the same but the old version and new version
     * have incompatible data. Create a new table. We want the name to be similar
     * to the old name so that it will occupy the same location in the DB.
     */
    MDB_dbi o_block_info = m_block_info;
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);


    MDB_cursor *c_blocks;
    result = mdb_cursor_open(txn, m_blocks, &c_blocks);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));

    MDB_cursor *c_old, *c_cur;
    i = 0;
    while(1) {
      if (!(i % 1000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << blockchain_height << "  \r" << std::flush;
          }
          txn.commit();
          result = mdb_txn_begin(m_env, NULL, 0, txn);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
        }
        result = mdb_cursor_open(txn, m_block_info, &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        if (!i) {
          result = mdb_stat(txn, m_block_info, &db_stats);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to query m_block_info: ", result).c_str()));
          i = db_stats.ms_entries;
        }
      }
      result = mdb_cursor_get(c_old, &k, &v, MDB_NEXT);
      if (result == MDB_NOTFOUND) {
        txn.commit();
        break;
      }
      else if (result)
        throw0(DB_ERROR(lmdb_error("Failed to get a record from block_info: ", result).c_str()));
      const mdb_block_info_3 *bi_old = (const mdb_block_info_3*)v.mv_data;
      mdb_block_info_4 bi;
      bi.bi_height = bi_old->bi_height;
      bi.bi_timestamp = bi_old->bi_timestamp;
      bi.bi_coins = bi_old->bi_coins;
      bi.bi_weight = bi_old->bi_weight;
      bi.bi_diff_lo = bi_old->bi_diff;
      bi.bi_diff_hi = 0;
      bi.bi_hash = bi_old->bi_hash;
      bi.bi_cum_rct = bi_old->bi_cum_rct;
      bi.bi_long_term_block_weight = bi_old->bi_long_term_block_weight;

      MDB_val_set(nv, bi);
      result = mdb_cursor_put(c_cur, (MDB_val *)&zerokval, &nv, MDB_APPENDDUP);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to put a record into block_infn: ", result).c_str()));
      /* we delete the old records immediately, so the overall DB and mapsize should not grow.
       * This is a little slower than just letting mdb_drop() delete it all at the end, but
       * it saves a significant amount of disk space.
       */
      result = mdb_cursor_del(c_old, 0);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to delete a record from block_info: ", result).c_str()));
      i++;
    }

    result = mdb_txn_begin(m_env, NULL, 0, txn);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_block_info);

    lmdb_db_open(txn, "block_info", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_block_info, compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 5;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_5_6()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  mdb_txn_safe txn(false);
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 5 to 6 - this may take a while:");

  MDB_dbi m_tmp_last_output;
  do
  {
    // 1. Prepare all valid outputs to be inserted into the merkle tree and
    //    place them in a locked outputs table. The key to this new table is the
    //    block id in which the outputs unlock.
    {
      MINFO("Setting up a locked outputs table (step 1/2 of full-chain membership proof migration)");

      result = mdb_txn_begin(m_env, NULL, 0, txn);
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
      lmdb_db_open(txn, "tmp_last_output", MDB_INTEGERKEY | MDB_CREATE, m_tmp_last_output, "Failed to open db handle for m_tmp_last_output");
      txn.commit();

      if (!m_batch_transactions)
        set_batch_transactions(true);
      const std::size_t BATCH_SIZE = 10000;
      batch_start();
      txn.m_txn = m_write_txn->m_txn;

      // Use this cache to know how to restart the migration if the process is killed
      struct tmp_output_cache { uint64_t n_outputs_read; uint64_t amount; outkey ok; };
      tmp_output_cache last_output;

      MDB_cursor *c_output_amounts, *c_locked_outputs, *c_tmp_last_output, *c_timelocked_outputs;
      MDB_val k, v;

      i = 0;
      const uint64_t n_outputs = this->num_outputs();
      MDB_cursor_op op = MDB_FIRST;
      while (1)
      {
        if (!(i % BATCH_SIZE))
        {
          if (i)
          {
            LOGIF(el::Level::Info)
            {
              const uint64_t percent = std::min((i * 100) / n_outputs, (uint64_t)99);
              std::cout << i << " / " << n_outputs << " outputs (" << percent << "% of step 1/2)  \r" << std::flush;
            }

            // Update last output read
            MDB_val_set(v_last_output, last_output);
            result = mdb_cursor_put(c_tmp_last_output, (MDB_val*)&zerokval, &v_last_output, 0);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to update max output id: ", result).c_str()));

            // Commit and start a new txn
            batch_stop();
            batch_start();
            txn.m_txn = m_write_txn->m_txn;

            // Reset k and v so we continue migration from the last output
            k = {sizeof(last_output.amount), (void *)&last_output.amount};

            const std::size_t outkey_size = (last_output.amount == 0) ? sizeof(outkey) : sizeof(pre_rct_outkey);
            v = {outkey_size, (void *)&last_output.ok};
          }

          // Open all cursors
          result = mdb_cursor_open(txn, m_output_amounts, &c_output_amounts);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for output amounts: ", result).c_str()));
          result = mdb_cursor_open(txn, m_locked_outputs, &c_locked_outputs);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for locked outputs: ", result).c_str()));
          result = mdb_cursor_open(txn, m_tmp_last_output, &c_tmp_last_output);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for temp last output: ", result).c_str()));
          result = mdb_cursor_open(txn, m_timelocked_outputs, &c_timelocked_outputs);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for timelocked outputs: ", result).c_str()));

          // Get the cached last output from the db
          bool found_cached_output = false;
          tmp_output_cache cached_last_o;
          if (i == 0)
          {
            MDB_val v_last_output;
            result = mdb_cursor_get(c_tmp_last_output, (MDB_val*)&zerokval, &v_last_output, MDB_SET);
            if (result != MDB_SUCCESS && result != MDB_NOTFOUND)
              throw0(DB_ERROR(lmdb_error("Failed to get max output id: ", result).c_str()));
            if (result != MDB_NOTFOUND)
            {
              cached_last_o = *(const tmp_output_cache*)v_last_output.mv_data;

              if (n_outputs < cached_last_o.n_outputs_read)
                throw0(DB_ERROR("Unexpected n_outputs_read on cached last output"));
              if (n_outputs == cached_last_o.n_outputs_read)
                break;

              MDEBUG("Found cached output " << cached_last_o.ok.output_id
                << ", migrated " << cached_last_o.n_outputs_read << " outputs already");
              found_cached_output = true;

              // Set k and v so we can continue the migration from that output
              k = {sizeof(cached_last_o.amount), (void *)&cached_last_o.amount};

              const std::size_t outkey_size = (cached_last_o.amount == 0) ? sizeof(outkey) : sizeof(pre_rct_outkey);
              v = {outkey_size, (void *)&cached_last_o.ok};

              i = cached_last_o.n_outputs_read;
              op = MDB_NEXT;
            }
          }

          // Advance the output_amounts cursor to the last output read
          if (i || found_cached_output)
          {
            result = mdb_cursor_get(c_output_amounts, &k, &v, MDB_GET_BOTH);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to advance cursor for output amounts: ", result).c_str()));
          }
        }

        // Get the next output from the db
        result = mdb_cursor_get(c_output_amounts, &k, &v, op);
        op = MDB_NEXT;
        if (result == MDB_NOTFOUND)
        {
          // Indicate we've read all outputs so we know the migration step is complete
          last_output.n_outputs_read = n_outputs;
          MDB_val_set(v_last_output, last_output);
          result = mdb_cursor_put(c_tmp_last_output, (MDB_val*)&zerokval, &v_last_output, 0);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to update max output id: ", result).c_str()));

          batch_stop();
          break;
        }
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Failed to get a record from output amounts: ", result).c_str()));

        ++i;
        const bool commit_next_iter = i && !(i % BATCH_SIZE);

        // Read the output data
        uint64_t amount = *(const uint64_t*)k.mv_data;
        output_data_t output_data;
        uint64_t output_id;
        if (amount == 0)
        {
          const outkey *okp = (const outkey *)v.mv_data;
          output_data = okp->data;
          output_id = okp->output_id;
          if (commit_next_iter)
            memcpy(&last_output.ok, okp, sizeof(outkey));
        }
        else
        {
          const pre_rct_outkey *okp = (const pre_rct_outkey *)v.mv_data;
          memcpy(&output_data, &okp->data, sizeof(pre_rct_output_data_t));
          output_data.commitment = rct::zeroCommitVartime(amount);
          output_id = okp->output_id;
          if (commit_next_iter)
            memcpy(&last_output.ok, okp, sizeof(pre_rct_outkey));
        }

        if (commit_next_iter)
        {
          // Set last output metadata
          last_output.amount = amount;
          last_output.n_outputs_read = i;
        }

        // Prepare the output for insertion to the tree
        auto output_pair = fcmp_pp::curve_trees::OutputPair{
            .output_pubkey = std::move(output_data.pubkey),
            .commitment    = std::move(output_data.commitment)
          };

        auto output_context = fcmp_pp::curve_trees::OutputContext{
            .output_id   = output_id,
            .output_pair = std::move(output_pair)
          };

        // Get the output's last locked block
        const uint64_t last_locked_block = cryptonote::get_last_locked_block_index(output_data.unlock_time, output_data.height);

        // Add the output to the locked outputs table
        MDB_val_set(k_block_id, last_locked_block);
        MDB_val_set(v_output, output_context);

        // MDB_NODUPDATA because all output id's should be unique
        // Can't use MDB_APPENDDUP because outputs aren't inserted in order sorted by output_id
        result = mdb_cursor_put(c_locked_outputs, &k_block_id, &v_output, MDB_NODUPDATA);
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Failed to add locked output: ", result).c_str()));

        // Check if the output is a coinbase output
        bool is_coinbase = false;
        const bool has_coinbase_last_locked_block = last_locked_block == (output_data.height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW - 1);
        if (has_coinbase_last_locked_block)
        {
          // Only coinbase outputs could potentially have the coinbase last locked block (see prevalidate_miner_transaction)
          auto toi = this->get_output_tx_and_index_from_global(output_id);
          auto tx = this->get_pruned_tx(toi.first);
          is_coinbase = cryptonote::is_coinbase(tx);
        }

        // Add custom timelocked outputs to the timelocked outputs table
        if (!cryptonote::is_custom_timelocked(is_coinbase, last_locked_block, output_data.height))
          continue;

        MDB_val_set(k_timelocked_block_id, last_locked_block);
        MDB_val_set(v_timelocked_output, output_context);

        // MDB_NODUPDATA because all output id's should be unique
        // Can't use MDB_APPENDDUP because outputs aren't inserted in order sorted by output_id
        result = mdb_cursor_put(c_timelocked_outputs, &k_timelocked_block_id, &v_timelocked_output, MDB_NODUPDATA);
        if (result != MDB_SUCCESS)
          throw0(DB_ERROR(lmdb_error("Failed to add timelocked output: ", result).c_str()));
      }
    }

    // 2. Set up the curve trees merkle tree by growing the tree block by block,
    //    with leaves that are spendable in each respective block
    {
      MINFO("Setting up a merkle tree using existing cryptonote outputs (step 2/2 of full-chain membership proof migration)");

      if (!m_batch_transactions)
        set_batch_transactions(true);
      const std::size_t BATCH_SIZE = 50;
      batch_start();
      txn.m_txn = m_write_txn->m_txn;

      MDB_cursor *c_locked_outputs;

      i = 0;
      const uint64_t n_blocks = height();
      while (i < n_blocks)
      {
        if (!(i % BATCH_SIZE))
        {
          if (i)
          {
            LOGIF(el::Level::Info)
            {
              const uint64_t percent = std::min((i * 100) / n_blocks, (uint64_t)99);
              std::cout << i << " / " << n_blocks << " blocks (" << percent << "% of step 2/2)  \r" << std::flush;
            }

            batch_stop();
            batch_start();
            txn.m_txn = m_write_txn->m_txn;
          }

          // Open all cursors
          result = mdb_cursor_open(txn, m_locked_outputs, &c_locked_outputs);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for locked outputs: ", result).c_str()));

          // See what the last block inserted into the new table was
          if (i == 0)
          {
            MDB_stat db_stats;
            result = mdb_stat(txn, m_tree_meta, &db_stats);
            if (result)
              throw0(DB_ERROR(lmdb_error("Failed to query m_tree_meta: ", result).c_str()));
            i = db_stats.ms_entries;
            if (i == n_blocks)
              break;
          }
        }

        this->advance_tree(i);

        LOGIF(el::Level::Info)
        {
          if ((i % 1000) == 0)
          {
            const uint64_t n_leaf_tuples = this->get_block_n_leaf_tuples(i);
            crypto::ec_point tree_root;
            this->get_tree_root_at_blk_idx(i, tree_root);
            const std::string tree_root_str = epee::string_tools::pod_to_hex(tree_root);
            MINFO("Block: " << i << ", tree root: " << tree_root_str << ", leaves: " << n_leaf_tuples);
          }
        }

        ++i;
      }
      batch_stop();
    }
  } while(0);

  // Update db version
  uint32_t version = 6;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  result = mdb_txn_begin(m_env, NULL, 0, txn);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));
  result = mdb_put(txn, m_properties, &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));

  // We only needed the temp last output table for this migration, drop it
  result = mdb_drop(txn, m_tmp_last_output, 1);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to drop temp last output table: ", result).c_str()));

  txn.commit();
}

void BlockchainLMDB::migrate(const uint32_t oldversion)
{
  if (oldversion < 1)
    migrate_0_1();
  if (oldversion < 2)
    migrate_1_2();
  if (oldversion < 3)
    migrate_2_3();
  if (oldversion < 4)
    migrate_3_4();
  if (oldversion < 5)
    migrate_4_5();
  if (oldversion < 6)
    migrate_5_6();
}

}  // namespace cryptonote
