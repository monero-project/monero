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

#include "db_lmdb.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/circular_buffer.hpp>
#include <memory>  // std::unique_ptr
#include <cstring>  // memcpy

#include "string_tools.h"
#include "file_io_utils.h"
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
#define VERSION 5

namespace
{

#pragma pack(push, 1)
// This MUST be identical to output_data_t, without the extra rct data at the end
struct pre_rct_output_data_t
{
  crypto::public_key pubkey;       //!< the output's public key (for spend verification)
  uint64_t           unlock_time;  //!< the output's unlock time (or height)
  uint64_t           height;       //!< the height of the block which created the output
};
#pragma pack(pop)

template <typename T>
[[noreturn]] inline void throw0(const T &e)
{
  LOG_PRINT_L0(e.what());
  throw e;
}

template <typename T>
[[noreturn]] inline void throw1(const T &e)
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
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    mv_size = sizeof (T);
    mv_data = &t_copy;
  }
private:
  T t_copy;
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
 * The output_amounts table doesn't use a dummy key, but uses DUPSORT.
 */

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
    auto m_cur_ ## name = m_write_txn.cursor(source::name);

#define RCURSOR(name) \
    auto m_cur_ ## name = m_txn.cursor(source::name);

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

typedef struct pre_rct_outkey {
    uint64_t amount_index;
    uint64_t output_id;
    pre_rct_output_data_t data;
} pre_rct_outkey;

typedef struct outkey {
    uint64_t amount_index;
    uint64_t output_id;
    output_data_t data;
} outkey;

typedef struct outtx {
    uint64_t output_id;
    crypto::hash tx_hash;
    uint64_t local_index;
} outtx;

const char* mdb_databases::name(source database)
{
  constexpr static std::array<const char*, 18> names{
    "blocks",
    "block_heights",
    "block_info",

    "output_txs",
    "output_amounts",

    "txs",
    "txs_pruned",
    "txs_prunable",
    "txs_prunable_hash",
    "txs_prunable_tip",
    "tx_indices",
    "tx_outputs",

    "spent_keys",

    "txpool_meta",
    "txpool_blob",

    "alt_blocks",

    "hf_versions",

    "properties",
  };

  return names.at(static_cast<uint8_t>(database));
}

MDB_stat mdb_databases::stat(MDB_txn* txn, source database) const
{
  MDB_stat stat;
  auto res = mdb_stat(txn, get(database), &stat);

  if (res == 0)
    return stat;

  auto error_message =
    std::string{"Failed to query "} +
    mdb_databases::name(database) +
    ": " +
    mdb_strerror(res);

  throw0(DB_ERROR(error_message.c_str()));
}

void mdb_databases::drop(MDB_txn* txn, source database, bool close)
{
  auto res = mdb_drop(txn, get(database), close);

  if (res == 0)
    return;

  auto error_message =
    std::string{"Failed to drop "} +
    mdb_databases::name(database) +
    ": " +
    mdb_strerror(res);

  throw0(DB_ERROR(error_message.c_str()));
}

std::atomic<uint64_t> mdb_txn_safe::num_active_txns{0};
std::atomic_flag mdb_txn_safe::creation_gate = ATOMIC_FLAG_INIT;

void lmdb_resized(MDB_env *env)
{
  mdb_txn_safe::prevent_new_txns();

  MGINFO("LMDB map resize detected.");

  MDB_envinfo mei;

  mdb_env_info(env, &mei);
  uint64_t old = mei.me_mapsize;

  mdb_txn_safe::wait_no_active_txns();

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
    lmdb_resized(env);
    res = mdb_txn_begin(env, parent, flags, txn);
  }
  return res;
}

inline int lmdb_txn_renew(MDB_txn *txn)
{
  int res = mdb_txn_renew(txn);
  if (res == MDB_MAP_RESIZED) {
    lmdb_resized(mdb_txn_env(txn));
    res = mdb_txn_renew(txn);
  }
  return res;
}

mdb_threadinfo::~mdb_threadinfo()
{
  if (m_ti_rtxn)
    mdb_txn_abort(m_ti_rtxn);
}

MDB_txn *mdb_threadinfo::transaction(MDB_env *env, bool *started)
{
  bool start = false;

  if (m_ti_rtxn == nullptr || mdb_txn_env(m_ti_rtxn) != env)
  {
    mdb_txn_safe::wait_and_register();

    auto result = lmdb_txn_begin(env, nullptr, MDB_RDONLY, &m_ti_rtxn);

    if (result != 0)
    {
      mdb_txn_safe::unregister();
      throw0(DB_ERROR{lmdb_error("Failed to create a transaction for the db: ", result).c_str()});
    }

    m_rf_txn = true;
    start = true;
  }
  else if (!m_rf_txn)
  {
    mdb_txn_safe::wait_and_register();

    auto result = lmdb_txn_renew(m_ti_rtxn);

    if (result != 0)
    {
      mdb_txn_safe::unregister();
      throw0(DB_ERROR_TXN_START(lmdb_error("Failed to renew a read transaction for the db: ", result).c_str()));
    }

    m_rf_txn = true;
    start = true;
  }

  if (started)
    *started = start;

  return m_ti_rtxn;
}

void mdb_threadinfo::reset()
{
  if (m_rf_txn)
  {
    mdb_txn_reset(m_ti_rtxn);
    mdb_txn_safe::unregister();
  }

  m_rf_txn = false;
  m_valid_cursor.reset();
}

MDB_txn* mdb_threadinfo::acquire(MDB_env* env)
{
  auto txn = transaction(env);
  ++m_handles;
  return txn;
}

void mdb_threadinfo::release()
{
  if (--m_handles == 0)
    reset();
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

void mdb_txn_safe::wait_and_register()
{
  while (creation_gate.test_and_set());
  ++num_active_txns;
  creation_gate.clear();
}

void mdb_txn_safe::unregister()
{
  --num_active_txns;
}

mdb_writer_data::~mdb_writer_data()
{
  LOG_PRINT_L3("mdb_writer_data: destructor");

  if (m_txn == nullptr)
    return;

  LOG_PRINT_L3("mdb_writer_data: m_txn not nullptr in destructor - calling mdb_txn_abort()");
  mdb_txn_abort(m_txn);
}

std::thread::id mdb_writer_data::current_thread() const noexcept
{
  return m_writer_thread.load(std::memory_order_relaxed);
}

mdb_databases& mdb_writer_data::databases() noexcept
{
  return m_databases;
}

mdb_txn_cursors& mdb_writer_data::cursors()
{
  return *m_cursors;
}

MDB_cursor* mdb_writer_data::cursor(source database)
{
  if (current_thread() != std::this_thread::get_id())
    throw1(DB_ERROR{"transaction owned by other thread"});

  auto& cursor = m_cursors->get(database);

  if (cursor != nullptr)
    return cursor;

  auto res = mdb_cursor_open(m_txn, m_databases.get(database), &cursor);

  if (res == 0)
    return cursor;

  auto error_message =
    std::string{"Failed to open a cursor for "} +
    mdb_databases::name(database) +
    ": " +
    mdb_strerror(res);

  throw0(DB_ERROR(error_message.c_str()));
}

MDB_txn* mdb_writer_data::try_acquire() noexcept
{
  if (current_thread() != std::this_thread::get_id())
    return nullptr;

  ++m_writer_depth;
  return m_txn;
}

MDB_txn* mdb_writer_data::acquire()
{
  LOG_PRINT_L3("mdb_writer_data: acquire()");

  if (auto txn = try_acquire())
    return txn;

  mdb_txn_safe::wait_and_register();

  auto result = lmdb_txn_begin(m_env, nullptr, 0, &m_txn);

  if (result != 0)
  {
    mdb_txn_safe::unregister();
    throw0(DB_ERROR{lmdb_error("Failed to create a transaction for the db: ", result).c_str()});
  }

  m_cursors.emplace();
  m_writer_thread.store(std::this_thread::get_id(), std::memory_order_relaxed);
  m_writer_depth = 1;

  return m_txn;
}

void mdb_writer_data::release(bool error)
{
  if (current_thread() != std::this_thread::get_id())
    throw1(DB_ERROR{"transaction owned by other thread"});

  if (m_writer_depth == 0)
    throw1(DB_ERROR{"transaction already released"});

  if (error)
    m_error = error;

  if (--m_writer_depth != 0 || m_txn == nullptr)
    return;

  auto txn = m_txn;
  m_txn = nullptr;
  m_cursors.reset();
  m_writer_thread.store({}, std::memory_order_relaxed);

  if (m_error)
  {
    mdb_txn_abort(txn);
    mdb_txn_safe::unregister();
    mdb_txn_safe::num_active_txns--;
  }
  else
  {
    auto result = mdb_txn_commit(txn);
    mdb_txn_safe::unregister();

    if (result)
      throw0(DB_ERROR{mdb_strerror(result)});
  }
}

mdb_write_handle::~mdb_write_handle()
{
  if (m_data == nullptr || m_txn == nullptr)
    return;

  LOG_PRINT_L3("mdb_write_handle: destructor");
  m_data->release(static_cast<bool>(std::current_exception()));
}

mdb_write_handle& mdb_write_handle::operator=(mdb_write_handle&& that) noexcept
{
  m_data = that.m_data;
  m_txn = that.m_txn;
  that.m_data = nullptr;
  that.m_txn = nullptr;

  return *this;
}

void mdb_write_handle::commit()
{
  if (m_data == nullptr || m_txn == nullptr)
    throw1(DB_ERROR{"transaction already released"});

  m_data->release(false);
  m_data = nullptr;
  m_txn = nullptr;
}

void mdb_write_handle::abort()
{
  if (m_data == nullptr || m_txn == nullptr)
    throw1(DB_ERROR{"transaction already released"});

  m_data->release(true);
  m_data = nullptr;
  m_txn = nullptr;
}

MDB_cursor* mdb_write_handle::cursor(source database)
{
  if (m_data == nullptr || m_txn == nullptr)
    throw1(DB_ERROR{"transaction already released"});

  return m_data->cursor(database);
}

mdb_read_handle::mdb_read_handle(MDB_env* env, mdb_databases& databases, mdb_writer_data& writer, boost::thread_specific_ptr<mdb_threadinfo>& tinfo) :
  m_databases{&databases},
  m_reader{tinfo.get()},
  m_writer{&writer},
  m_txn{writer.try_acquire()}
{
  if (m_txn != nullptr)
  {
    m_reader = nullptr;
    return;
  }

  if (m_reader == nullptr)
  {
    m_reader = new mdb_threadinfo{};
    tinfo.reset(m_reader);
  }

  m_writer = nullptr;
  m_txn = m_reader->acquire(env);
}

mdb_read_handle::~mdb_read_handle()
{
  if (m_txn == nullptr)
    return;

  LOG_PRINT_L3("mdb_read_handle: destructor");

  if (m_reader != nullptr)
  {
    m_reader->release();
  }
  else if (m_writer != nullptr)
  {
    m_writer->release(static_cast<bool>(std::current_exception()));
  }
}

mdb_read_handle& mdb_read_handle::operator=(mdb_read_handle&& that) noexcept
{
  std::swap(m_databases, that.m_databases);
  std::swap(m_reader, that.m_reader);
  std::swap(m_writer, that.m_writer);
  std::swap(m_txn, that.m_txn);
  return *this;
}

void mdb_read_handle::abort()
{
  if (m_writer != nullptr)
  {
    m_writer->release();
    m_writer = nullptr;
  }
  else if (m_reader != nullptr)
  {
    m_reader->release();
    m_reader = nullptr;
  }
}

MDB_cursor* mdb_read_handle::cursor(source database)
{
  if (m_writer != nullptr)
    return m_writer->cursor(database);

  if (m_databases == nullptr || m_reader == nullptr)
    throw1(DB_ERROR{"transaction already released"});

  auto& cursor = m_reader->m_ti_rcursors.get(database);

  if (cursor == nullptr)
  {
    auto res = mdb_cursor_open(m_txn, m_databases->get(database), &cursor);

    if (res == 0)
    {
      m_reader->m_valid_cursor[static_cast<uint8_t>(database)] = true;
      return cursor;
    }

    auto error_message =
      std::string{"Failed to open a cursor for "} +
      mdb_databases::name(database) +
      ": " +
      mdb_strerror(res);

    throw0(DB_ERROR(error_message.c_str()));
  }
  else if (!m_reader->m_valid_cursor[static_cast<uint8_t>(database)])
  {
    auto res = mdb_cursor_renew(m_txn, cursor);

    if (res == 0)
    {
      m_reader->m_valid_cursor[static_cast<uint8_t>(database)] = true;
      return cursor;
    }

    auto error_message =
      std::string{"Failed to renew a cursor for "} +
      mdb_databases::name(database) +
      ": " +
      mdb_strerror(res);

    throw0(DB_ERROR(error_message.c_str()));
  }

  return cursor;
}

inline void BlockchainLMDB::check_open() const
{
  if (!m_open)
    throw0(DB_ERROR("DB operation attempted on a not-open DB instance"));
}

void BlockchainLMDB::db_open(MDB_txn* txn, int flags, source database)
{
  auto res = mdb_dbi_open(txn, mdb_databases::name(database), flags, &m_databases.get(database));

  if (res == 0)
    return;

  auto error_message =
    std::string{"Failed to open db handle for "} +
    mdb_databases::name(database) +
    ": " +
    mdb_strerror(res) +
    " - you may want to start with --db-salvage";

  throw0(cryptonote::DB_OPEN_FAILURE(error_message.data()));
}

void BlockchainLMDB::do_resize(uint64_t increase_size)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  CRITICAL_REGION_LOCAL(m_synchronization_lock);
  uint64_t add_size = 1LL << 30;

  // If given, use increase_size to set additional environment size
  // This is currently used for increasing by an estimated size at start of new
  // batch txn.
  if (increase_size > 0)
    add_size = increase_size;

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

  // add increase_size, or 1Gb if not given
  uint64_t new_mapsize = (uint64_t) mei.me_mapsize + add_size;

  new_mapsize += (new_mapsize % mst.ms_psize);

  mdb_txn_safe::prevent_new_txns();

  if (m_batch_handle)
  {
    mdb_txn_safe::allow_new_txns();
    throw0(DB_ERROR("lmdb resizing not yet supported when batch transactions enabled!"));
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
    MDB_txn *rtxn;
    mdb_txn_cursors *rcurs;
    bool my_rtxn = block_rtxn_start(&rtxn, &rcurs);
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
    if (my_rtxn) block_rtxn_stop();
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
    uint64_t num_rct_outs, const crypto::hash& blk_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};
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
  mdb_write_handle m_write_txn{*m_write_data};
  uint64_t m_height = height();

  if (m_height == 0)
    throw0(BLOCK_DNE ("Attempting to remove block from an empty blockchain"));

  CURSOR(block_info)
  CURSOR(block_heights)
  CURSOR(blocks)
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

uint64_t BlockchainLMDB::add_transaction_data(const crypto::hash& blk_hash, const std::pair<transaction, blobdata>& txp, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};
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

  const cryptonote::transaction &tx = txp.first;
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

  const cryptonote::blobdata &blob = txp.second;
  MDB_val_sized(blobval, blob);

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

  mdb_write_handle m_write_txn{*m_write_data};
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
  mdb_write_handle m_write_txn{*m_write_data};
  uint64_t m_height = height();
  uint64_t m_num_outputs = num_outputs();

  int result = 0;

  CURSOR(output_txs)
  CURSOR(output_amounts)

  if (tx_output.target.type() != typeid(txout_to_key))
    throw0(DB_ERROR("Wrong output type: expected txout_to_key"));
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
  ok.data.pubkey = boost::get < txout_to_key > (tx_output.target).key;
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
  mdb_write_handle m_write_txn{*m_write_data};
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

  // ensure all removes use a single lmdb transaction
  mdb_write_handle m_write_txn{*m_write_data};

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
  mdb_write_handle m_write_txn{*m_write_data};
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
  mdb_write_handle m_write_txn{*m_write_data};
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
  mdb_write_handle m_write_txn{*m_write_data};

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
  mdb_write_handle m_write_txn{*m_write_data};

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

BlockchainLMDB::~BlockchainLMDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);

  // batch transaction shouldn't be active at this point. If it is, consider it aborted.
  if (m_batch_handle)
  {
    try { batch_abort(); }
    catch (...) { /* ignore */ }
  }
  if (m_open)
    close();
}

BlockchainLMDB::BlockchainLMDB(bool batch_transactions): BlockchainDB()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  // initialize folder to something "safe" just in case
  // someone accidentally misuses this class...
  m_folder = "thishsouldnotexistbecauseitisgibberish";

  m_batch_transactions = batch_transactions;
  m_cum_size = 0;
  m_cum_count = 0;

  // reset may also need changing when initialize things here

  m_hardfork = nullptr;
}

void BlockchainLMDB::open(const std::string& filename, const int db_flags)
{
  int result;
  int mdb_flags = MDB_NORDAHEAD;

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
  if (boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_FILENAME)
      || boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME))
  {
    LOG_PRINT_L0("Found existing LMDB files in " << old_files.string());
    LOG_PRINT_L0("Move " << CRYPTONOTE_BLOCKCHAINDATA_FILENAME << " and/or " << CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME << " to " << filename << ", or delete them, and then restart");
    throw DB_ERROR("Database could not be opened");
  }

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

  m_write_data.emplace(m_env, m_databases);

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

  boost::optional<mdb_write_handle> write_handle;

  if ((mdb_flags & MDB_RDONLY) == 0)
    write_handle.emplace(*m_write_data);

  mdb_read_handle txn{m_env, m_databases, *m_write_data, m_tinfo};

  // open necessary databases, and set properties as needed
  // uses macros to avoid having to change things too many places
  // also change blockchain_prune.cpp to match
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::blocks);

  db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::block_info);
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::block_heights);

  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::txs);
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::txs_pruned);
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::txs_prunable);
  db_open(txn, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, source::txs_prunable_hash);
  if (!(mdb_flags & MDB_RDONLY))
    db_open(txn, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, source::txs_prunable_tip);
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::tx_indices);
  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::tx_outputs);

  db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::output_txs);
  db_open(txn, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, source::output_amounts);

  db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::spent_keys);

  db_open(txn, MDB_CREATE, source::txpool_meta);
  db_open(txn, MDB_CREATE, source::txpool_blob);

  db_open(txn, MDB_CREATE, source::alt_blocks);

  // this subdb is dropped on sight, so it may not be present when we open the DB.
  // Since we use MDB_CREATE, we'll get an exception if we open read-only and it does not exist.
  // So we don't open for read-only, and also not drop below. It is not used elsewhere.
  if (!(mdb_flags & MDB_RDONLY))
    lmdb_db_open(txn, "hf_starting_heights", MDB_CREATE, m_hf_starting_heights, "Failed to open db handle for m_hf_starting_heights");

  db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::hf_versions);

  db_open(txn, MDB_CREATE, source::properties);

  mdb_set_dupsort(txn, m_databases.get(source::spent_keys), compare_hash32);
  mdb_set_dupsort(txn, m_databases.get(source::block_heights), compare_hash32);
  mdb_set_dupsort(txn, m_databases.get(source::tx_indices), compare_hash32);
  mdb_set_dupsort(txn, m_databases.get(source::output_amounts), compare_uint64);
  mdb_set_dupsort(txn, m_databases.get(source::output_txs), compare_uint64);
  mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);
  if (!(mdb_flags & MDB_RDONLY))
    mdb_set_dupsort(txn, m_databases.get(source::txs_prunable_tip), compare_uint64);
  mdb_set_compare(txn, m_databases.get(source::txs_prunable), compare_uint64);
  mdb_set_dupsort(txn, m_databases.get(source::txs_prunable_hash), compare_uint64);

  mdb_set_compare(txn, m_databases.get(source::txpool_meta), compare_hash32);
  mdb_set_compare(txn, m_databases.get(source::txpool_blob), compare_hash32);
  mdb_set_compare(txn, m_databases.get(source::alt_blocks), compare_hash32);
  mdb_set_compare(txn, m_databases.get(source::properties), compare_string);

  if (!(mdb_flags & MDB_RDONLY))
  {
    result = mdb_drop(txn, m_hf_starting_heights, 1);
    if (result && result != MDB_NOTFOUND)
      throw0(DB_ERROR(lmdb_error("Failed to drop m_hf_starting_heights: ", result).c_str()));
  }

  // get and keep current height
  auto db_stats = m_databases.stat(txn, source::blocks);
  LOG_PRINT_L2("Setting m_height to: " << db_stats.ms_entries);
  uint64_t m_height = db_stats.ms_entries;

  bool compatible = true;

  MDB_val_str(k, "version");
  MDB_val v;
  auto get_result = mdb_get(txn, m_databases.get(source::properties), &k, &v);
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
      write_handle->commit();
      m_open = true;
      migrate(db_version);
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
      auto put_result = mdb_put(txn, m_databases.get(source::properties), &k, &v, 0);
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
  if (write_handle)
    write_handle->commit();

  m_open = true;
  // from here, init should be finished
}

void BlockchainLMDB::close()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (m_batch_handle)
  {
    LOG_PRINT_L3("close() first calling batch_abort() due to active batch transaction");
    batch_abort();
  }
  this->sync();
  m_tinfo.reset();

  m_write_data.reset();

  // FIXME: not yet thread safe!!!  Use with care.
  mdb_env_close(m_env);
  m_open = false;
}

void BlockchainLMDB::sync()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  if (is_read_only())
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

  mdb_write_handle txn{*m_write_data};

  m_databases.drop(txn, source::blocks);
  m_databases.drop(txn, source::block_info);
  m_databases.drop(txn, source::block_heights);
  m_databases.drop(txn, source::txs_pruned);
  m_databases.drop(txn, source::txs_prunable);
  m_databases.drop(txn, source::txs_prunable_hash);
  m_databases.drop(txn, source::txs_prunable_tip);
  m_databases.drop(txn, source::tx_indices);
  m_databases.drop(txn, source::tx_outputs);
  m_databases.drop(txn, source::output_txs);
  m_databases.drop(txn, source::output_amounts);
  m_databases.drop(txn, source::spent_keys);
  (void)mdb_drop(txn, m_hf_starting_heights, 0); // this one is dropped in new code
  m_databases.drop(txn, source::hf_versions);
  m_databases.drop(txn, source::properties);

  // init with current version
  MDB_val_str(k, "version");
  MDB_val_copy<uint32_t> v(VERSION);
  if (auto result = mdb_put(txn, m_databases.get(source::properties), &k, &v, 0))
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

#define TXN_PREFIX(); \
  mdb_write_handle txn{*m_write_data};

#define TXN_PREFIX_RDONLY() \
  mdb_read_handle m_txn{m_env, m_databases, *m_write_data, m_tinfo};
#define TXN_POSTFIX_RDONLY()

#define TXN_POSTFIX_SUCCESS()


// The below two macros are for DB access within block add/remove, whether
// regular batch txn is in use or not. m_write_txn is used as a batch txn, even
// if it's only within block add/remove.
//
// DB access functions that may be called both within block add/remove and
// without should use these. If the function will be called ONLY within block
// add/remove, m_write_txn alone may be used instead of these macros.

#define TXN_BLOCK_PREFIX(); \
  mdb_write_handle txn{*m_write_data};

#define TXN_BLOCK_POSTFIX_SUCCESS()

void BlockchainLMDB::add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata &blob, const txpool_tx_meta_t &meta)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};

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
  mdb_write_handle m_write_txn{*m_write_data};

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
    auto db_stats = m_databases.stat(m_txn, source::txpool_meta);
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
  return found;
}

void BlockchainLMDB::remove_txpool_tx(const crypto::hash& txid)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};

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
  return true;
}

bool BlockchainLMDB::get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd, relay_category tx_category) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX_RDONLY();
  RCURSOR(txpool_meta);
  RCURSOR(txpool_blob);

  MDB_val k = {sizeof(txid), (void *)&txid};
  MDB_val v;

  // if filtering, make sure those requirements are met before copying blob
  if (tx_category != relay_category::all)
  {
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
  pruning_seed = tools::get_pruning_stripe(pruning_seed);;
  if (pruning_seed > (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES))
    throw0(DB_ERROR("Pruning seed not in range"));
  check_open();

  TIME_MEASURE_START(t);

  size_t n_total_records = 0, n_prunable_records = 0, n_pruned_records = 0, commit_counter = 0;
  uint64_t n_bytes = 0;

  mdb_write_handle txn{*m_write_data};

  auto db_stats = m_databases.stat(txn, source::txs_prunable);
  const size_t pages0 = db_stats.ms_branch_pages + db_stats.ms_leaf_pages + db_stats.ms_overflow_pages;

  MDB_val_str(k, "pruning_seed");
  MDB_val v;
  auto result = mdb_get(txn, m_databases.get(source::properties), &k, &v);
  bool prune_tip_table = false;
  if (result == MDB_NOTFOUND)
  {
    // not pruned yet
    if (mode != prune_mode_prune)
    {
      TIME_MEASURE_FINISH(t);
      MDEBUG("Pruning not enabled, nothing to do");
      return true;
    }
    if (pruning_seed == 0)
      pruning_seed = tools::get_random_stripe();
    pruning_seed = tools::make_pruning_seed(pruning_seed, CRYPTONOTE_PRUNING_LOG_STRIPES);
    v.mv_data = &pruning_seed;
    v.mv_size = sizeof(pruning_seed);
    auto result = mdb_put(txn, m_databases.get(source::properties), &k, &v, 0);
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
  result = mdb_cursor_open(txn, m_databases.get(source::txs_pruned), &c_txs_pruned);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
  result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable), &c_txs_prunable);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
  result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable_tip), &c_txs_prunable_tip);
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
          txn = mdb_write_handle{*m_write_data};
          result = mdb_cursor_open(txn, m_databases.get(source::txs_pruned), &c_txs_pruned);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
          result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable), &c_txs_prunable);
          if (result)
            throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
          result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable_tip), &c_txs_prunable_tip);
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
    result = mdb_cursor_open(txn, m_databases.get(source::tx_indices), &c_tx_indices);
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
        txn = mdb_write_handle{*m_write_data};
        result = mdb_cursor_open(txn, m_databases.get(source::txs_pruned), &c_txs_pruned);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable), &c_txs_prunable);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable_tip), &c_txs_prunable_tip);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_tip: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::tx_indices), &c_tx_indices);
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

  db_stats = m_databases.stat(txn, source::txs_prunable);
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

bool BlockchainLMDB::for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata*)> f, bool include_blob, relay_category category) const
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
    const cryptonote::blobdata *passed_bd = NULL;
    cryptonote::blobdata bd;
    if (include_blob)
    {
      MDB_val b;
      result = mdb_cursor_get(m_cur_txpool_blob, &k, &b, MDB_SET);
      if (result == MDB_NOTFOUND)
        throw0(DB_ERROR("Failed to find txpool tx blob to match metadata"));
      if (result)
        throw0(DB_ERROR(lmdb_error("Failed to enumerate txpool tx blob: ", result).c_str()));
      bd.assign(reinterpret_cast<const char*>(b.mv_data), b.mv_size);
      passed_bd = &bd;
    }

    if (!f(txid, meta, passed_bd)) {
      ret = false;
      break;
    }
  }

  return ret;
}

bool BlockchainLMDB::for_all_alt_blocks(std::function<bool(const crypto::hash&, const alt_block_data_t&, const cryptonote::blobdata*)> f, bool include_blob) const
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
    const cryptonote::blobdata *passed_bd = NULL;
    cryptonote::blobdata bd;
    if (include_blob)
    {
      bd.assign(reinterpret_cast<const char*>(v.mv_data) + sizeof(alt_block_data_t), v.mv_size - sizeof(alt_block_data_t));
      passed_bd = &bd;
    }

    if (!f(blkid, *data, passed_bd)) {
      ret = false;
      break;
    }
  }

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
  if ((result = mdb_stat(m_txn, m_databases.get(source::blocks), &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
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

  return res;
}

uint64_t BlockchainLMDB::get_top_block_timestamp() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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
  return max_block_size;
}

void BlockchainLMDB::add_max_block_size(uint64_t sz)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};

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

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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

  // ensure all lmdb calls happen within the same lmdb transaction
  TXN_PREFIX_RDONLY();

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
  if ((result = mdb_stat(m_txn, m_databases.get(source::blocks), &db_stats)))
    throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
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
    throw0(DB_ERROR(lmdb_error("Failed to query output_txs: ", result).c_str()));

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
    LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
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
    LOG_PRINT_L1("transaction with hash " << epee::string_tools::pod_to_hex(h) << " not found in db");
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
  MDB_val result, val_tx_prunable_hash;
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

  auto db_stats = m_databases.stat(m_txn, source::txs_pruned);

  TXN_POSTFIX_RDONLY();

  return db_stats.ms_entries;
}

std::vector<transaction> BlockchainLMDB::get_tx_list(const std::vector<crypto::hash>& hlist) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  std::vector<transaction> v;
  v.reserve(hlist.size());

  // ensure all calls to lmdb happen within the same transaction
  TXN_PREFIX_RDONLY();

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

output_data_t BlockchainLMDB::get_output_key(const uint64_t& amount, const uint64_t& index, bool include_commitmemt) const
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

  output_data_t ret;
  if (amount == 0)
  {
    const outkey *okp = (const outkey *)v.mv_data;
    ret = okp->data;
  }
  else
  {
    const pre_rct_outkey *okp = (const pre_rct_outkey *)v.mv_data;
    memcpy(&ret, &okp->data, sizeof(pre_rct_output_data_t));;
    if (include_commitmemt)
      ret.commitment = rct::zeroCommit(amount);
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
  if (indices.empty())
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
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
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
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    if (pruned)
    {
      if (!parse_and_validate_tx_base_from_blob(bd, tx))
        throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
    }
    else
    {
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
  if (m_batch_handle)
    return false;
  check_open();

  check_and_resize_for_batch(batch_num_blocks, batch_bytes);

  m_batch_handle.emplace(*m_write_data);

  if (m_tinfo.get())
    m_tinfo->reset();

  LOG_PRINT_L3("batch transaction: begin");
  return true;
}

void BlockchainLMDB::batch_commit()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (!m_batch_handle)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_data->current_thread() != std::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));

  check_open();

  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  m_batch_handle->commit();
  TIME_MEASURE_FINISH(time1);
  time_commit1 += time1;
  LOG_PRINT_L3("batch transaction: committed");

  m_batch_handle.reset();
}

void BlockchainLMDB::cleanup_batch()
{
  // for destruction of batch transaction
  m_batch_handle.reset();
}

void BlockchainLMDB::batch_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (! m_batch_transactions)
    throw0(DB_ERROR("batch transactions not enabled"));
  if (!m_batch_handle)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_data->current_thread() != std::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));
  check_open();
  LOG_PRINT_L3("batch transaction: committing...");
  TIME_MEASURE_START(time1);
  try
  {
    m_batch_handle->commit();
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
  if (!m_batch_handle)
    throw1(DB_ERROR("batch transaction not in progress"));
  if (m_write_data->current_thread() != std::this_thread::get_id())
    throw1(DB_ERROR("batch transaction owned by other thread"));
  check_open();
  // for destruction of batch transaction
  // explicitly call in case mdb_env_close() (BlockchainLMDB::close()) called before BlockchainLMDB destructor called.
  m_batch_handle->abort();
  m_batch_handle.reset();
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
  if (m_write_data->current_thread() == std::this_thread::get_id()) {
    // temporary additional handle, since we have at least one
    // other handle releasing this will not terminate the transaction
    mdb_write_handle m_write_txn{*m_write_data};

    *mtxn = m_write_txn;
    *mcur = &m_write_data->cursors();
    return false;
  }

  mdb_threadinfo *tinfo = m_tinfo.get();
  bool ret = false;

  if (tinfo == nullptr)
  {
    tinfo = new mdb_threadinfo;
    m_tinfo.reset(tinfo);
  }

  *mtxn = tinfo->transaction(m_env, &ret);
  *mcur = &tinfo->m_ti_rcursors;

  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  return ret;
}

void BlockchainLMDB::block_rtxn_stop() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  m_tinfo->reset();
}

bool BlockchainLMDB::block_rtxn_start() const
{
  MDB_txn *mtxn;
  mdb_txn_cursors *mcur;
  return block_rtxn_start(&mtxn, &mcur);
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
  if (!m_batch_handle && m_write_data->current_thread() != std::thread::id{})
    throw0(DB_ERROR_TXN_START((std::string("Attempted to start new write txn when write txn already exists in ")+__FUNCTION__).c_str()));
  if (!m_batch_handle)
  {
    m_write_data->acquire();
    if (m_tinfo.get())
      m_tinfo->reset();
  } else if (m_write_data->current_thread() != std::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to start new write txn when batch txn already exists in ")+__FUNCTION__).c_str()));
}

void BlockchainLMDB::block_wtxn_stop()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  if (m_write_data->current_thread() == std::thread::id{})
    throw0(DB_ERROR_TXN_START((std::string("Attempted to stop write txn when no such txn exists in ")+__FUNCTION__).c_str()));
  if (m_write_data->current_thread() != std::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to stop write txn from the wrong thread in ")+__FUNCTION__).c_str()));
  if (!m_batch_handle)
  {
    TIME_MEASURE_START(time1);
    m_write_data->release();
    TIME_MEASURE_FINISH(time1);
    time_commit1 += time1;
  }
}

void BlockchainLMDB::block_wtxn_abort()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  if (m_write_data->current_thread() == std::thread::id{})
    throw0(DB_ERROR_TXN_START((std::string("Attempted to abort write txn when no such txn exists in ")+__FUNCTION__).c_str()));
  if (m_write_data->current_thread() != std::this_thread::get_id())
    throw0(DB_ERROR_TXN_START((std::string("Attempted to abort write txn from the wrong thread in ")+__FUNCTION__).c_str()));

  if (!m_batch_handle)
  {
    m_write_data->release(true);
  }
}

void BlockchainLMDB::block_rtxn_abort() const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  m_tinfo->reset();
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
    if (!m_batch_handle && need_resize())
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

  mdb_write_handle m_write_txn{*m_write_data};

  BlockchainDB::pop_block(blk, txs);
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
    throw0(DB_ERROR("Invalid sizes of amounts and offets"));

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
      data.commitment = rct::zeroCommit(amount);
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

  bool fret = true;
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

  TXN_PREFIX();

  auto result = mdb_drop(txn, m_hf_starting_heights, 1);
  if (result)
    throw1(DB_ERROR(lmdb_error("Error dropping hard fork starting heights db: ", result).c_str()));
  m_databases.drop(txn, source::hf_versions, true);

  TXN_POSTFIX_SUCCESS();
}

void BlockchainLMDB::set_hard_fork_version(uint64_t height, uint8_t version)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_BLOCK_PREFIX();

  MDB_val_copy<uint64_t> val_key(height);
  MDB_val_copy<uint8_t> val_value(version);
  int result;
  result = mdb_put(txn, m_databases.get(source::hf_versions), &val_key, &val_value, MDB_APPEND);
  if (result == MDB_KEYEXIST)
    result = mdb_put(txn, m_databases.get(source::hf_versions), &val_key, &val_value, 0);
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

void BlockchainLMDB::add_alt_block(const crypto::hash &blkid, const cryptonote::alt_block_data_t &data, const cryptonote::blobdata &blob)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();
  mdb_write_handle m_write_txn{*m_write_data};

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
  mdb_write_handle m_write_txn{*m_write_data};

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
  int result = mdb_stat(m_txn, m_databases.get(source::alt_blocks), &db_stats);
  uint64_t count = 0;
  if (result != MDB_NOTFOUND)
  {
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to query alt_blocks: ", result).c_str()));
    count = db_stats.ms_entries;
  }
  TXN_POSTFIX_RDONLY();
  return count;
}

void BlockchainLMDB::drop_alt_blocks()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  check_open();

  TXN_PREFIX()

  m_databases.drop(txn, source::alt_blocks);

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
  uint64_t size = 0;
  boost::filesystem::path datafile(m_folder);
  datafile /= CRYPTONOTE_BLOCKCHAINDATA_FILENAME;
  if (!epee::file_io_utils::get_file_size(datafile.string(), size))
    size = 0;
  return size;
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
  mdb_write_handle txn{*m_write_data};
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 0 to 1 - this may take a while:");
  MINFO("updating blocks, hf_versions, outputs, txs, and spent_keys tables...");

  do {
    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_databases.get(source::blocks), &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
    m_height = db_stats.ms_entries;
    MINFO("Total number of blocks: " << m_height);
    MINFO("block migration will update block_heights, block_info, and hf_versions...");

    MINFO("migrating block_heights:");
    MDB_dbi o_heights;

    unsigned int flags;
    result = mdb_dbi_flags(txn, m_databases.get(source::block_heights), &flags);
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
    o_heights = m_databases.get(source::block_heights);
    lmdb_db_open(txn, "block_heightr", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_databases.get(source::block_heights), "Failed to open db handle for block_heightr");
    mdb_set_dupsort(txn, m_databases.get(source::block_heights), compare_hash32);

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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::block_heights), &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_heightr: ", result).c_str()));
        result = mdb_cursor_open(txn, o_heights, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_heights: ", result).c_str()));
        if (!i) {
          auto ms = m_databases.stat(txn, source::block_heights);
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

    txn = mdb_write_handle{*m_write_data};
    /* Delete the old table */
    result = mdb_drop(txn, o_heights, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_heights table: ", result).c_str()));

    RENAME_DB("block_heightr");

    /* close and reopen to get old dbi slot back */
    mdb_dbi_close(m_env, m_databases.get(source::block_heights));
    db_open(txn, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, source::block_heights);
    mdb_set_dupsort(txn, m_databases.get(source::block_heights), compare_hash32);
    txn.commit();

  } while(0);

  /* old tables are k(height), v(value).
   * new table is DUPFIXED, k(zeroval), v{height, values...}.
   */
  do {
    LOG_PRINT_L1("migrating block info:");

    MDB_dbi coins;
    txn = mdb_write_handle{*m_write_data};
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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::block_info), &c_cur);
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
          auto ms = m_databases.stat(txn, source::block_info);
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
    txn = mdb_write_handle{*m_write_data};
    result = mdb_dbi_flags(txn, m_databases.get(source::hf_versions), &flags);
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
    o_hfv = m_databases.get(source::hf_versions);
    lmdb_db_open(txn, "hf_versionr", MDB_INTEGERKEY | MDB_CREATE, m_databases.get(source::hf_versions), "Failed to open db handle for hf_versionr");

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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::hf_versions), &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for spent_keyr: ", result).c_str()));
        result = mdb_cursor_open(txn, o_hfv, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for spent_keys: ", result).c_str()));
        if (!i) {
          auto ms = m_databases.stat(txn, source::hf_versions);
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

    txn = mdb_write_handle{*m_write_data};
    /* Delete the old table */
    result = mdb_drop(txn, o_hfv, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old hf_versions table: ", result).c_str()));
    RENAME_DB("hf_versionr");
    mdb_dbi_close(m_env, m_databases.get(source::hf_versions));
    db_open(txn, MDB_INTEGERKEY, source::hf_versions);

    txn.commit();
  } while(0);

  do {
    LOG_PRINT_L1("deleting old indices:");

    /* Delete all other tables, we're just going to recreate them */
    MDB_dbi dbi;
    txn = mdb_write_handle{*m_write_data};

    result = mdb_dbi_open(txn, "tx_unlocks", 0, &dbi);
    if (result == MDB_NOTFOUND) {
        txn.abort();
        LOG_PRINT_L1("  old indices already deleted");
        break;
    }
    txn.abort();

#define DELETE_DB(x) do {   \
    LOG_PRINT_L1("  " x ":"); \
    txn = mdb_write_handle{*m_write_data}; \
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
    txn = mdb_write_handle{*m_write_data};
    db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::output_txs);
    mdb_set_dupsort(txn, m_databases.get(source::output_txs), compare_uint64);
    db_open(txn, MDB_INTEGERKEY | MDB_CREATE, source::tx_outputs);
    db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::spent_keys);
    mdb_set_dupsort(txn, m_databases.get(source::spent_keys), compare_hash32);
    db_open(txn, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, source::output_amounts);
    mdb_set_dupsort(txn, m_databases.get(source::output_amounts), compare_uint64);
    txn.commit();
  } while(0);

  do {
    LOG_PRINT_L1("migrating txs and outputs:");

    unsigned int flags;
    txn = mdb_write_handle{*m_write_data};
    result = mdb_dbi_flags(txn, m_databases.get(source::txs), &flags);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to retrieve txs flags: ", result).c_str()));
    /* if the flags are what we expect, this table has already been migrated */
    if (flags & MDB_INTEGERKEY) {
      txn.abort();
      LOG_PRINT_L1("  txs already migrated");
      break;
    }

    MDB_dbi o_txs;
    blobdata bd;
    block b;
    MDB_val hk;

    o_txs = m_databases.get(source::txs);
    mdb_set_compare(txn, o_txs, compare_hash32);
    lmdb_db_open(txn, "txr", MDB_INTEGERKEY | MDB_CREATE, m_databases.get(source::txs), "Failed to open db handle for txr");

    txn.commit();

    MDB_cursor *c_blocks, *c_txs, *c_props, *c_cur;
    i = 0;
    z = m_height;

    hk.mv_size = sizeof(crypto::hash);
    set_batch_transactions(true);
    batch_start(1000);
    txn = mdb_write_handle{*m_write_data};
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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::blocks), &c_blocks);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::properties), &c_props);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for properties: ", result).c_str()));
        result = mdb_cursor_open(txn, o_txs, &c_txs);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs: ", result).c_str()));
        if (!i) {
          auto ms = m_databases.stat(txn, source::txs);
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

      bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
      if (!parse_and_validate_block_from_blob(bd, b))
        throw0(DB_ERROR("Failed to parse block from blob retrieved from the db"));

      add_transaction(null_hash, std::make_pair(b.miner_tx, tx_to_blob(b.miner_tx)));
      for (unsigned int j = 0; j<b.tx_hashes.size(); j++) {
        transaction tx;
        hk.mv_data = &b.tx_hashes[j];
        result = mdb_cursor_get(c_txs, &hk, &v, MDB_SET);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to get record from txs: ", result).c_str()));
        bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
        if (!parse_and_validate_tx_from_blob(bd, tx))
          throw0(DB_ERROR("Failed to parse tx from blob retrieved from the db"));
        add_transaction(null_hash, std::make_pair(std::move(tx), bd), &b.tx_hashes[j]);
        result = mdb_cursor_del(c_txs, 0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to get record from txs: ", result).c_str()));
      }
      i++;
      m_height = i;
    }
    txn = mdb_write_handle{*m_write_data};
    result = mdb_drop(txn, o_txs, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete txs from the db: ", result).c_str()));

    RENAME_DB("txr");

    mdb_dbi_close(m_env, m_databases.get(source::txs));

    db_open(txn, MDB_INTEGERKEY, source::txs);

    txn.commit();
  } while(0);

  uint32_t version = 1;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  txn = mdb_write_handle{*m_write_data};
  result = mdb_put(txn, m_databases.get(source::properties), &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_1_2()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i, z;
  int result;
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 1 to 2 - this may take a while:");
  MINFO("updating txs_pruned and txs_prunable tables...");

  do {
    mdb_write_handle txn{*m_write_data};

    auto db_stats_txs = m_databases.stat(txn, source::txs);
    auto db_stats_txs_pruned = m_databases.stat(txn, source::txs_pruned);
    auto db_stats_txs_prunable = m_databases.stat(txn, source::txs_prunable);
    auto db_stats_txs_prunable_hash = m_databases.stat(txn, source::txs_prunable_hash);
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
          db_stats_txs = m_databases.stat(txn, source::txs);
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << (i + db_stats_txs.ms_entries) << "  \r" << std::flush;
          }
          txn.commit();
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::txs_pruned), &c_cur0);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_pruned: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable), &c_cur1);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::txs_prunable_hash), &c_cur2);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for txs_prunable_hash: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::txs), &c_old);
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

      cryptonote::blobdata bd;
      bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
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
  mdb_write_handle txn{*m_write_data};
  result = mdb_put(txn, m_databases.get(source::properties), &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_2_3()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 2 to 3 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    mdb_write_handle txn{*m_write_data};

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_databases.get(source::blocks), &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
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
    MDB_dbi o_block_info = m_databases.get(source::block_info);
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_databases.get(source::block_info), "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);

    MDB_cursor *c_old, *c_cur;
    i = 0;
    while(1) {
      if (!(i % 1000)) {
        if (i) {
          LOGIF(el::Level::Info) {
            std::cout << i << " / " << blockchain_height << "  \r" << std::flush;
          }
          txn.commit();
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::block_info), &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        if (!i) {
          auto db_stat = m_databases.stat(txn, source::block_info);
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

    txn = mdb_write_handle{*m_write_data};
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_databases.get(source::block_info));

    db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::block_info);
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 3;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  mdb_write_handle txn{*m_write_data};
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_3_4()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  MDB_val k, v;
  char *ptr;
  bool past_long_term_weight = false;

  MGINFO_YELLOW("Migrating blockchain from DB version 3 to 4 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    mdb_write_handle txn{*m_write_data};

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_databases.get(source::blocks), &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
    const uint64_t blockchain_height = db_stats.ms_entries;

    boost::circular_buffer<uint64_t> long_term_block_weights(CRYPTONOTE_LONG_TERM_BLOCK_WEIGHT_WINDOW_SIZE);

    /* the block_info table name is the same but the old version and new version
     * have incompatible data. Create a new table. We want the name to be similar
     * to the old name so that it will occupy the same location in the DB.
     */
    MDB_dbi o_block_info = m_databases.get(source::block_info);
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_databases.get(source::block_info), "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);


    MDB_cursor *c_blocks;
    result = mdb_cursor_open(txn, m_databases.get(source::blocks), &c_blocks);
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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::block_info), &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        result = mdb_cursor_open(txn, m_databases.get(source::blocks), &c_blocks);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for blocks: ", result).c_str()));
        if (!i) {
          auto db_stat = m_databases.stat(txn, source::block_info);
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
          throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
        if (vb.mv_size == 0)
          throw0(DB_ERROR("Invalid data from blocks"));
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

    txn = mdb_write_handle{*m_write_data};
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_databases.get(source::block_info));

    db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::block_info);
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 4;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  mdb_write_handle txn{*m_write_data};
  result = mdb_put(txn, m_databases.get(source::properties), &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
  txn.commit();
}

void BlockchainLMDB::migrate_4_5()
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t i;
  int result;
  MDB_val k, v;
  char *ptr;

  MGINFO_YELLOW("Migrating blockchain from DB version 4 to 5 - this may take a while:");

  do {
    LOG_PRINT_L1("migrating block info:");

    mdb_write_handle txn{*m_write_data};

    MDB_stat db_stats;
    if ((result = mdb_stat(txn, m_databases.get(source::blocks), &db_stats)))
      throw0(DB_ERROR(lmdb_error("Failed to query blocks: ", result).c_str()));
    const uint64_t blockchain_height = db_stats.ms_entries;

    /* the block_info table name is the same but the old version and new version
     * have incompatible data. Create a new table. We want the name to be similar
     * to the old name so that it will occupy the same location in the DB.
     */
    MDB_dbi o_block_info = m_databases.get(source::block_info);
    lmdb_db_open(txn, "block_infn", MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_databases.get(source::block_info), "Failed to open db handle for block_infn");
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);


    MDB_cursor *c_blocks;
    result = mdb_cursor_open(txn, m_databases.get(source::blocks), &c_blocks);
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
          txn = mdb_write_handle{*m_write_data};
        }
        result = mdb_cursor_open(txn, m_databases.get(source::block_info), &c_cur);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_infn: ", result).c_str()));
        result = mdb_cursor_open(txn, o_block_info, &c_old);
        if (result)
          throw0(DB_ERROR(lmdb_error("Failed to open a cursor for block_info: ", result).c_str()));
        if (!i) {
          auto db_stat = m_databases.stat(txn, source::block_info);
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

    txn = mdb_write_handle{*m_write_data};
    /* Delete the old table */
    result = mdb_drop(txn, o_block_info, 1);
    if (result)
      throw0(DB_ERROR(lmdb_error("Failed to delete old block_info table: ", result).c_str()));

    RENAME_DB("block_infn");
    mdb_dbi_close(m_env, m_databases.get(source::block_info));

    db_open(txn, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, source::block_info);
    mdb_set_dupsort(txn, m_databases.get(source::block_info), compare_uint64);

    txn.commit();
  } while(0);

  uint32_t version = 5;
  v.mv_data = (void *)&version;
  v.mv_size = sizeof(version);
  MDB_val_str(vk, "version");
  mdb_write_handle txn{*m_write_data};
  result = mdb_put(txn, m_databases.get(source::properties), &vk, &v, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to update version for the db: ", result).c_str()));
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
}

}  // namespace cryptonote
