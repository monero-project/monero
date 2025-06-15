// Copyright (c) 2014-2024, The Monero Project
//
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

#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "common/unordered_containers_boost_serialization.h"
#include "common/command_line.h"
#include "common/varint.h"
#include "serialization/crypto.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "wallet/ringdb.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static uint64_t records_per_sync = 200;
static uint64_t db_flags = 0;
static MDB_dbi dbi_relative_rings;
static MDB_dbi dbi_outputs;
static MDB_dbi dbi_processed_txidx;
static MDB_dbi dbi_spent;
static MDB_dbi dbi_per_amount;
static MDB_dbi dbi_ring_instances;
static MDB_dbi dbi_stats;
static MDB_env *env = NULL;

struct output_data
{
  uint64_t amount;
  uint64_t offset;
  output_data(): amount(0), offset(0) {}
  output_data(uint64_t a, uint64_t i): amount(a), offset(i) {}
  bool operator==(const output_data &other) const { return other.amount == amount && other.offset == offset; }
};

//
// relative_rings: key_image -> vector<uint64_t>
// outputs: 128 bits -> set of key images
// processed_txidx: string -> uint64_t
// spent: amount -> offset
// ring_instances: vector<uint64_t> -> uint64_t
// stats: string -> arbitrary
//

static bool parse_db_sync_mode(std::string db_sync_mode)
{
  std::vector<std::string> options;
  boost::trim(db_sync_mode);
  boost::split(options, db_sync_mode, boost::is_any_of(" :"));

  for(const auto &option : options)
    MDEBUG("option: " << option);

  // default to fast:async:1
  uint64_t DEFAULT_FLAGS = DBF_FAST;

  if(options.size() == 0)
  {
    // default to fast:async:1
    db_flags = DEFAULT_FLAGS;
  }

  bool safemode = false;
  if(options.size() >= 1)
  {
    if(options[0] == "safe")
    {
      safemode = true;
      db_flags = DBF_SAFE;
    }
    else if(options[0] == "fast")
    {
      db_flags = DBF_FAST;
    }
    else if(options[0] == "fastest")
    {
      db_flags = DBF_FASTEST;
      records_per_sync = 1000; // default to fastest:async:1000
    }
    else
      db_flags = DEFAULT_FLAGS;
  }

  if(options.size() >= 2 && !safemode)
  {
    char *endptr;
    uint64_t bps = strtoull(options[1].c_str(), &endptr, 0);
    if (*endptr == '\0')
      records_per_sync = bps;
  }

  return true;
}

static std::string get_default_db_path()
{
  boost::filesystem::path dir = tools::get_default_data_dir();
  // remove .bitmonero, replace with .shared-ringdb
  dir = dir.remove_filename();
  dir /= ".shared-ringdb";
  return dir.string();
}

static std::string get_cache_filename(boost::filesystem::path filename)
{
  if (!boost::filesystem::is_directory(filename))
    filename.remove_filename();
  return filename.string();
}

static int compare_hash32(const MDB_val *a, const MDB_val *b)
{
  const uint32_t *va = (const uint32_t*) a->mv_data;
  const uint32_t *vb = (const uint32_t*) b->mv_data;
  for (int n = 7; n >= 0; n--)
  {
    if (va[n] == vb[n])
      continue;
    return va[n] < vb[n] ? -1 : 1;
  }

  return 0;
}

int compare_uint64(const MDB_val *a, const MDB_val *b)
{
  const uint64_t va = *(const uint64_t *)a->mv_data;
  const uint64_t vb = *(const uint64_t *)b->mv_data;
  return (va < vb) ? -1 : va > vb;
}

static int compare_double64(const MDB_val *a, const MDB_val *b)
{
  const uint64_t va = *(const uint64_t*) a->mv_data;
  const uint64_t vb = *(const uint64_t*) b->mv_data;
  if (va == vb)
  {
    const uint64_t va = ((const uint64_t*) a->mv_data)[1];
    const uint64_t vb = ((const uint64_t*) b->mv_data)[1];
    return va < vb ? -1 : va > vb;
  }
  return va < vb ? -1 : va > vb;
}

static int resize_env(const char *db_path)
{
  MDB_envinfo mei;
  MDB_stat mst;
  int ret;

  size_t needed = 1000ul * 1024 * 1024; // at least 1000 MB

  ret = mdb_env_info(env, &mei);
  if (ret)
    return ret;
  ret = mdb_env_stat(env, &mst);
  if (ret)
    return ret;
  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;
  uint64_t mapsize = mei.me_mapsize;
  if (size_used + needed > mei.me_mapsize)
  {
    try
    {
      boost::filesystem::path path(db_path);
      boost::filesystem::space_info si = boost::filesystem::space(path);
      if(si.available < needed)
      {
        MERROR("!! WARNING: Insufficient free space to extend database !!: " << (si.available >> 20L) << " MB available");
        return ENOSPC;
      }
    }
    catch(...)
    {
      // print something but proceed.
      MWARNING("Unable to query free disk space.");
    }

    mapsize += needed;
  }
  return mdb_env_set_mapsize(env, mapsize);
}

static void init(std::string cache_filename)
{
  MDB_txn *txn;
  bool tx_active = false;
  int dbr;

  MINFO("Creating spent output cache in " << cache_filename);

  tools::create_directories_if_necessary(cache_filename);

  int flags = 0;
  if (db_flags & DBF_FAST)
    flags |= MDB_NOSYNC;
  if (db_flags & DBF_FASTEST)
    flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;

  dbr = mdb_env_create(&env);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 7);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = get_cache_filename(cache_filename); 
  dbr = mdb_env_open(env, actual_filename.c_str(), flags, 0664);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "relative_rings", MDB_CREATE | MDB_INTEGERKEY, &dbi_relative_rings);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn, dbi_relative_rings, compare_hash32);

  dbr = mdb_dbi_open(txn, "outputs", MDB_CREATE | MDB_INTEGERKEY, &dbi_outputs);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn, dbi_outputs, compare_double64);

  dbr = mdb_dbi_open(txn, "processed_txidx", MDB_CREATE, &dbi_processed_txidx);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn, "spent", MDB_CREATE | MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi_spent);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(txn, dbi_spent, compare_uint64);

  dbr = mdb_dbi_open(txn, "per_amount", MDB_CREATE | MDB_INTEGERKEY, &dbi_per_amount);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn, dbi_per_amount, compare_uint64);

  dbr = mdb_dbi_open(txn, "ring_instances", MDB_CREATE, &dbi_ring_instances);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn, "stats", MDB_CREATE, &dbi_stats);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_commit(txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
}

static void close()
{
  if (env)
  {
    mdb_dbi_close(env, dbi_relative_rings);
    mdb_dbi_close(env, dbi_outputs);
    mdb_dbi_close(env, dbi_processed_txidx);
    mdb_dbi_close(env, dbi_per_amount);
    mdb_dbi_close(env, dbi_spent);
    mdb_dbi_close(env, dbi_ring_instances);
    mdb_dbi_close(env, dbi_stats);
    mdb_env_close(env);
    env = NULL;
  }
}

static std::string compress_ring(const std::vector<uint64_t> &ring, std::string s = "")
{
  const size_t sz = s.size();
  s.resize(s.size() + 12 * ring.size());
  char *ptr = (char*)s.data() + sz;
  for (uint64_t out: ring)
    tools::write_varint(ptr, out);
  if (ptr > s.data() + sz + 12 * ring.size())
    throw std::runtime_error("varint output overflow");
  s.resize(ptr - s.data());
  return s;
}

static std::string compress_ring(uint64_t amount, const std::vector<uint64_t> &ring)
{
  char s[12], *ptr = s;
  tools::write_varint(ptr, amount);
  if (ptr > s + sizeof(s))
    throw std::runtime_error("varint output overflow");
  return compress_ring(ring, std::string(s, ptr-s));
}

static std::vector<uint64_t> decompress_ring(const std::string &s)
{
  std::vector<uint64_t> ring;
  int read = 0;
  for (std::string::const_iterator i = s.begin(); i != s.cend(); std::advance(i, read))
  {
    uint64_t out;
    std::string tmp(i, s.cend());
    read = tools::read_varint(tmp.begin(), tmp.end(), out);
    CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Internal error decompressing ring");
    ring.push_back(out);
  }
  return ring;
}

static bool for_all_transactions(const std::string &filename, uint64_t &start_idx, uint64_t &n_txes, const std::function<bool(const cryptonote::transaction_prefix&)> &f)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  MDB_cursor *cur;
  int dbr;
  bool tx_active = false;
  MDB_val k;
  MDB_val v;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 2);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  if (dbr) throw std::runtime_error("Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "txs_pruned", MDB_INTEGERKEY, &dbi);
  if (dbr)
    dbr = mdb_dbi_open(txn, "txs", MDB_INTEGERKEY, &dbi);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_cursor_open(txn, dbi, &cur);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
  MDB_stat stat;
  dbr = mdb_stat(txn, dbi, &stat);
  if (dbr) throw std::runtime_error("Failed to query m_block_info: " + std::string(mdb_strerror(dbr)));
  n_txes = stat.ms_entries;

  bool fret = true;

  k.mv_size = sizeof(uint64_t);
  k.mv_data = &start_idx;
  MDB_cursor_op op = MDB_SET;
  while (1)
  {
    int ret = mdb_cursor_get(cur, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw std::runtime_error("Failed to enumerate transactions: " + std::string(mdb_strerror(ret)));

    if (k.mv_size != sizeof(uint64_t))
      throw std::runtime_error("Bad key size");
    const uint64_t idx = *(uint64_t*)k.mv_data;
    if (idx < start_idx)
      continue;

    cryptonote::transaction_prefix tx;
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    binary_archive<false> ba{epee::strspan<std::uint8_t>(bd)};
    bool r = do_serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");

    start_idx = *(uint64_t*)k.mv_data;
    if (!f(tx)) {
      fret = false;
      break;
    }
  }

  mdb_cursor_close(cur);
  dbr = mdb_txn_commit(txn);
  if (dbr) throw std::runtime_error("Failed to commit db transaction: " + std::string(mdb_strerror(dbr)));
  tx_active = false;
  mdb_dbi_close(env, dbi);
  mdb_env_close(env);
  return fret;
}

static bool for_all_transactions(const std::string &filename, const uint64_t &start_idx, uint64_t &n_txes, const std::function<bool(bool, uint64_t, const cryptonote::transaction_prefix&)> &f)
{
  MDB_env *env;
  MDB_dbi dbi_blocks, dbi_txs;
  MDB_txn *txn;
  MDB_cursor *cur_blocks, *cur_txs;
  int dbr;
  bool tx_active = false;
  MDB_val k;
  MDB_val v;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 3);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  if (dbr) throw std::runtime_error("Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "blocks", MDB_INTEGERKEY, &dbi_blocks);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_dbi_open(txn, "txs_pruned", MDB_INTEGERKEY, &dbi_txs);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_cursor_open(txn, dbi_blocks, &cur_blocks);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_cursor_open(txn, dbi_txs, &cur_txs);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  MDB_stat stat;
  dbr = mdb_stat(txn, dbi_blocks, &stat);
  if (dbr) throw std::runtime_error("Failed to query txs stat: " + std::string(mdb_strerror(dbr)));
  uint64_t n_blocks = stat.ms_entries;
  dbr = mdb_stat(txn, dbi_txs, &stat);
  if (dbr) throw std::runtime_error("Failed to query txs stat: " + std::string(mdb_strerror(dbr)));
  n_txes = stat.ms_entries;

  bool fret = true;

  MDB_cursor_op op_blocks = MDB_FIRST;
  MDB_cursor_op op_txs = MDB_FIRST;
  uint64_t tx_idx = 0;
  while (1)
  {
    int ret = mdb_cursor_get(cur_blocks, &k, &v, op_blocks);
    op_blocks = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw std::runtime_error("Failed to enumerate blocks: " + std::string(mdb_strerror(ret)));

    if (k.mv_size != sizeof(uint64_t))
      throw std::runtime_error("Bad key size");
    uint64_t height = *(const uint64_t*)k.mv_data;
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    block b;
    if (!parse_and_validate_block_from_blob(bd, b))
      throw std::runtime_error("Failed to parse block from blob retrieved from the db");

    ret = mdb_cursor_get(cur_txs, &k, &v, op_txs);
    if (ret)
      throw std::runtime_error("Failed to fetch transaction " + string_tools::pod_to_hex(get_transaction_hash(b.miner_tx)) + ": " + std::string(mdb_strerror(ret)));
    op_txs = MDB_NEXT;

    bool last_block = height == n_blocks - 1;
    if (start_idx <= tx_idx++  && !f(last_block && b.tx_hashes.empty(), height, b.miner_tx))
    {
      fret = false;
      break;
    }
    for (size_t i = 0; i < b.tx_hashes.size(); ++i)
    {
      const crypto::hash& txid = b.tx_hashes[i];
      ret = mdb_cursor_get(cur_txs, &k, &v, op_txs);
      if (ret)
        throw std::runtime_error("Failed to fetch transaction " + string_tools::pod_to_hex(txid) + ": " + std::string(mdb_strerror(ret)));
      if (start_idx <= tx_idx++)
      {
        cryptonote::transaction_prefix tx;
        bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
        CHECK_AND_ASSERT_MES(parse_and_validate_tx_prefix_from_blob(bd, tx), false, "Failed to parse transaction from blob");
        if (!f(last_block && i == b.tx_hashes.size() - 1, height, tx))
        {
          fret = false;
          break;
        }
      }
    }
    if (!fret)
      break;
  }

  mdb_cursor_close(cur_blocks);
  mdb_cursor_close(cur_txs);
  mdb_txn_commit(txn);
  tx_active = false;
  mdb_dbi_close(env, dbi_blocks);
  mdb_dbi_close(env, dbi_txs);
  mdb_env_close(env);
  return fret;
}

static uint64_t find_first_diverging_transaction(const std::string &first_filename, const std::string &second_filename)
{
  MDB_env *env[2];
  MDB_dbi dbi[2];
  MDB_txn *txn[2];
  MDB_cursor *cur[2];
  int dbr;
  bool tx_active[2] = { false, false };
  uint64_t n_txes[2];
  MDB_val k;
  MDB_val v[2];

  epee::misc_utils::auto_scope_leave_caller txn_dtor[2];
  for (int i = 0; i < 2; ++i)
  {
    dbr = mdb_env_create(&env[i]);
    if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
    dbr = mdb_env_set_maxdbs(env[i], 2);
    if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
    const std::string actual_filename = i ? second_filename : first_filename;
    dbr = mdb_env_open(env[i], actual_filename.c_str(), 0, 0664);
    if (dbr) throw std::runtime_error("Failed to open rings database file '"
        + actual_filename + "': " + std::string(mdb_strerror(dbr)));

    dbr = mdb_txn_begin(env[i], NULL, MDB_RDONLY, &txn[i]);
    if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    txn_dtor[i] = epee::misc_utils::create_scope_leave_handler([&, i](){if (tx_active[i]) mdb_txn_abort(txn[i]);});
    tx_active[i] = true;

    dbr = mdb_dbi_open(txn[i], "txs_pruned", MDB_INTEGERKEY, &dbi[i]);
    if (dbr)
      dbr = mdb_dbi_open(txn[i], "txs", MDB_INTEGERKEY, &dbi[i]);
    if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
    dbr = mdb_cursor_open(txn[i], dbi[i], &cur[i]);
    if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
    MDB_stat stat;
    dbr = mdb_stat(txn[i], dbi[i], &stat);
    if (dbr) throw std::runtime_error("Failed to query m_block_info: " + std::string(mdb_strerror(dbr)));
    n_txes[i] = stat.ms_entries;
  }

  if (n_txes[0] == 0 || n_txes[1] == 0)
    throw std::runtime_error("No transaction in the database");
  uint64_t lo = 0, hi = std::min(n_txes[0], n_txes[1]) - 1;
  while (lo <= hi)
  {
    uint64_t mid = (lo + hi) / 2;

    k.mv_size = sizeof(uint64_t);
    k.mv_data = (void*)&mid;
    dbr = mdb_cursor_get(cur[0], &k, &v[0], MDB_SET);
    if (dbr) throw std::runtime_error("Failed to query transaction: " + std::string(mdb_strerror(dbr)));
    dbr = mdb_cursor_get(cur[1], &k, &v[1], MDB_SET);
    if (dbr) throw std::runtime_error("Failed to query transaction: " + std::string(mdb_strerror(dbr)));
    if (v[0].mv_size == v[1].mv_size && !memcmp(v[0].mv_data, v[1].mv_data, v[0].mv_size))
      lo = mid + 1;
    else
      hi = mid - 1;
  }

  for (int i = 0; i < 2; ++i)
  {
    mdb_cursor_close(cur[i]);
    dbr = mdb_txn_commit(txn[i]);
    if (dbr) throw std::runtime_error("Failed to query transaction: " + std::string(mdb_strerror(dbr)));
    tx_active[i] = false;
    mdb_dbi_close(env[i], dbi[i]);
    mdb_env_close(env[i]);
  }
  return hi;
}

static std::vector<uint64_t> canonicalize(const std::vector<uint64_t> &v)
{
  std::vector<uint64_t> c;
  c.reserve(v.size());
  c.push_back(v[0]);
  for (size_t n = 1; n < v.size(); ++n)
  {
    if (v[n] != 0)
      c.push_back(v[n]);
  }
  if (c.size() < v.size())
  {
    MINFO("Ring has duplicate member(s): " <<
        boost::join(v | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
  }
  return c;
}

static uint64_t get_num_spent_outputs()
{
  MDB_txn *txn;
  bool tx_active = false;

  int dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  MDB_cursor *cur;
  dbr = mdb_cursor_open(txn, dbi_spent, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for spent outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val k, v;
  mdb_size_t count = 0, tmp;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    dbr = mdb_cursor_get(cur, &k, &v, op);
    op = MDB_NEXT_NODUP;
    if (dbr == MDB_NOTFOUND)
      break;
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get first/next spent output: " + std::string(mdb_strerror(dbr)));
    dbr = mdb_cursor_count(cur, &tmp);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to count entries: " + std::string(mdb_strerror(dbr)));
    count += tmp;
  }

  mdb_cursor_close(cur);
  dbr = mdb_txn_commit(txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn: " + std::string(mdb_strerror(dbr)));
  tx_active = false;

  return count;
}

static bool add_spent_output(MDB_cursor *cur, const output_data &od)
{
  MDB_val k = {sizeof(od.amount), (void*)&od.amount};
  MDB_val v = {sizeof(od.offset), (void*)&od.offset};
  int dbr = mdb_cursor_put(cur, &k, &v, MDB_NODUPDATA);
  if (dbr == MDB_KEYEXIST)
    return false;
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to add spent output: " + std::string(mdb_strerror(dbr)));
  return true;
}

static bool is_output_spent(MDB_cursor *cur, const output_data &od)
{
  MDB_val k = {sizeof(od.amount), (void*)&od.amount};
  MDB_val v = {sizeof(od.offset), (void*)&od.offset};
  int dbr = mdb_cursor_get(cur, &k, &v, MDB_GET_BOTH);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_NOTFOUND, "Failed to get spent output: " + std::string(mdb_strerror(dbr)));
  bool spent = dbr == 0;
  return spent;
}

static std::vector<output_data> get_spent_outputs(MDB_txn *txn)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, dbi_spent, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for spent outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val k, v;
  mdb_size_t count = 0;
  dbr = mdb_cursor_get(cur, &k, &v, MDB_FIRST);
  if (dbr != MDB_NOTFOUND)
  {
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get first spent output: " + std::string(mdb_strerror(dbr)));
    dbr = mdb_cursor_count(cur, &count);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to count entries: " + std::string(mdb_strerror(dbr)));
  }
  std::vector<output_data> outs;
  outs.reserve(count);
  while (1)
  {
    outs.push_back({*(const uint64_t*)k.mv_data, *(const uint64_t*)v.mv_data});
    dbr = mdb_cursor_get(cur, &k, &v, MDB_NEXT);
    if (dbr == MDB_NOTFOUND)
      break;
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get next spent output: " + std::string(mdb_strerror(dbr)));
  }
  mdb_cursor_close(cur);
  return outs;
}

static void get_per_amount_outputs(MDB_txn *txn, uint64_t amount, uint64_t &total, uint64_t &spent)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, dbi_per_amount, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for per amount outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val k, v;
  k.mv_size = sizeof(uint64_t);
  k.mv_data = (void*)&amount;
  dbr = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (dbr == MDB_NOTFOUND)
  {
    total = spent = 0;
  }
  else
  {
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get per amount outputs: " + std::string(mdb_strerror(dbr)));
    total = ((const uint64_t*)v.mv_data)[0];
    spent = ((const uint64_t*)v.mv_data)[1];
  }
  mdb_cursor_close(cur);
}

static void inc_per_amount_outputs(MDB_txn *txn, uint64_t amount, uint64_t total, uint64_t spent)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, dbi_per_amount, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for per amount outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val k, v;
  k.mv_size = sizeof(uint64_t);
  k.mv_data = (void*)&amount;
  dbr = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (dbr == 0)
  {
    total += ((const uint64_t*)v.mv_data)[0];
    spent += ((const uint64_t*)v.mv_data)[1];
  }
  else
  {
    CHECK_AND_ASSERT_THROW_MES(dbr == MDB_NOTFOUND, "Failed to get per amount outputs: " + std::string(mdb_strerror(dbr)));
  }
  uint64_t data[2] = {total, spent};
  v.mv_size = 2 * sizeof(uint64_t);
  v.mv_data = (void*)data;
  dbr = mdb_cursor_put(cur, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to write record for per amount outputs: " + std::string(mdb_strerror(dbr)));
  mdb_cursor_close(cur);
}

static uint64_t get_processed_txidx(const std::string &name)
{
  MDB_txn *txn;
  bool tx_active = false;

  int dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  uint64_t height = 0;
  MDB_val k, v;
  k.mv_data = (void*)name.c_str();
  k.mv_size = name.size();
  dbr = mdb_get(txn, dbi_processed_txidx, &k, &v);
  if (dbr != MDB_NOTFOUND)
  {
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get processed height: " + std::string(mdb_strerror(dbr)));
    height = *(const uint64_t*)v.mv_data;
  }

  dbr = mdb_txn_commit(txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn: " + std::string(mdb_strerror(dbr)));
  tx_active = false;

  return height;
}

static void set_processed_txidx(MDB_txn *txn, const std::string &name, uint64_t height)
{
  MDB_val k, v;
  k.mv_data = (void*)name.c_str();
  k.mv_size = name.size();
  v.mv_data = (void*)&height;
  v.mv_size = sizeof(height);
  int dbr = mdb_put(txn, dbi_processed_txidx, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set processed height: " + std::string(mdb_strerror(dbr)));
}

static bool get_relative_ring(MDB_txn *txn, const crypto::key_image &ki, std::vector<uint64_t> &ring)
{
  MDB_val k, v;
  k.mv_data = (void*)&ki;
  k.mv_size = sizeof(ki);
  int dbr = mdb_get(txn, dbi_relative_rings, &k, &v);
  if (dbr == MDB_NOTFOUND)
    return false;
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get relative ring: " + std::string(mdb_strerror(dbr)));
  ring = decompress_ring(std::string((const char*)v.mv_data, v.mv_size));
  return true;
}

static void set_relative_ring(MDB_txn *txn, const crypto::key_image &ki, const std::vector<uint64_t> &ring)
{
  const std::string sring = compress_ring(ring);
  MDB_val k, v;
  k.mv_data = (void*)&ki;
  k.mv_size = sizeof(ki);
  v.mv_data = (void*)sring.c_str();
  v.mv_size = sring.size();
  int dbr = mdb_put(txn, dbi_relative_rings, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set relative ring: " + std::string(mdb_strerror(dbr)));
}

static std::string keep_under_511(const std::string &s)
{
  if (s.size() <= 511)
    return s;
  crypto::hash hash;
  crypto::cn_fast_hash(s.data(), s.size(), hash);
  return std::string((const char*)&hash, 32);
}

static uint64_t get_ring_instances(MDB_txn *txn, uint64_t amount, const std::vector<uint64_t> &ring)
{
  const std::string sring = keep_under_511(compress_ring(amount, ring));
  MDB_val k, v;
  k.mv_data = (void*)sring.data();
  k.mv_size = sring.size();
  int dbr = mdb_get(txn, dbi_ring_instances, &k, &v);
  if (dbr == MDB_NOTFOUND)
    return 0;
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get ring instances: " + std::string(mdb_strerror(dbr)));
  return *(const uint64_t*)v.mv_data;
}

static uint64_t get_ring_subset_instances(MDB_txn *txn, uint64_t amount, const std::vector<uint64_t> &ring)
{
  uint64_t instances = get_ring_instances(txn, amount, ring);
  if (ring.size() > 11)
    return instances;

  uint64_t extra = 0;
  std::vector<uint64_t> subset;
  subset.reserve(ring.size());
  for (uint64_t mask = 1; mask < (((uint64_t)1) << ring.size()) - 1; ++mask)
  {
    subset.resize(0);
    for (size_t i = 0; i < ring.size(); ++i)
      if ((mask >> i) & 1)
        subset.push_back(ring[i]);
    extra += get_ring_instances(txn, amount, subset);
  }
  return instances + extra;
}

static uint64_t inc_ring_instances(MDB_txn *txn, uint64_t amount, const std::vector<uint64_t> &ring)
{
  const std::string sring = keep_under_511(compress_ring(amount, ring));
  MDB_val k, v;
  k.mv_data = (void*)sring.data();
  k.mv_size = sring.size();

  int dbr = mdb_get(txn, dbi_ring_instances, &k, &v);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_NOTFOUND, "Failed to get ring instances: " + std::string(mdb_strerror(dbr)));

  uint64_t count;
  if (dbr == MDB_NOTFOUND)
    count = 1;
  else
    count = 1 + *(const uint64_t*)v.mv_data;

  v.mv_data = &count;
  v.mv_size = sizeof(count);
  dbr = mdb_put(txn, dbi_ring_instances, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set ring instances: " + std::string(mdb_strerror(dbr)));

  return count;
}

static std::vector<crypto::key_image> get_key_images(MDB_txn *txn, const output_data &od)
{
  MDB_val k, v;
  k.mv_data = (void*)&od;
  k.mv_size = sizeof(od);
  int dbr = mdb_get(txn, dbi_outputs, &k, &v);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_NOTFOUND, "Failed to get output: " + std::string(mdb_strerror(dbr)));
  if (dbr == MDB_NOTFOUND)
    return {};
  CHECK_AND_ASSERT_THROW_MES(v.mv_size % 32 == 0, "Unexpected record size");
  std::vector<crypto::key_image> key_images;
  key_images.reserve(v.mv_size / 32);
  const crypto::key_image *ki = (const crypto::key_image*)v.mv_data;
  for (size_t n = 0; n < v.mv_size / 32; ++n)
    key_images.push_back(*ki++);
  return key_images;
}

static void add_key_image(MDB_txn *txn, const output_data &od, const crypto::key_image &ki)
{
  MDB_val k, v;
  k.mv_data = (void*)&od;
  k.mv_size = sizeof(od);
  int dbr = mdb_get(txn, dbi_outputs, &k, &v);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_NOTFOUND, "Failed to get output");
  std::string data;
  if (!dbr)
  {
    CHECK_AND_ASSERT_THROW_MES(v.mv_size % 32 == 0, "Unexpected record size");
    data = std::string((const char*)v.mv_data, v.mv_size);
  }
  data += std::string((const char*)&ki, sizeof(ki));

  v.mv_data = (void*)data.data();
  v.mv_size = data.size();
  dbr = mdb_put(txn, dbi_outputs, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set outputs: " + std::string(mdb_strerror(dbr)));
}

static bool get_stat(MDB_txn *txn, const char *key, uint64_t &data)
{
  MDB_val k, v;
  k.mv_data = (void*)key;
  k.mv_size = strlen(key);
  int dbr = mdb_get(txn, dbi_stats, &k, &v);
  if (dbr == MDB_NOTFOUND)
    return false;
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get stat record");
  CHECK_AND_ASSERT_THROW_MES(v.mv_size == sizeof(uint64_t), "Unexpected record size");
  data = *(const uint64_t*)v.mv_data;
  return true;
}

static void set_stat(MDB_txn *txn, const char *key, uint64_t data)
{
  MDB_val k, v;
  k.mv_data = (void*)key;
  k.mv_size = strlen(key);
  v.mv_data = (void*)&data;
  v.mv_size = sizeof(uint64_t);
  int dbr = mdb_put(txn, dbi_stats, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set stat record");
}

static void inc_stat(MDB_txn *txn, const char *key)
{
  uint64_t data;
  if (!get_stat(txn, key, data))
    data = 0;
  ++data;
  set_stat(txn, key, data);
}

static void open_db(const std::string &filename, MDB_env **env, MDB_txn **txn, MDB_cursor **cur, MDB_dbi *dbi)
{
  tools::create_directories_if_necessary(filename);

  int flags = MDB_RDONLY;
  if (db_flags & DBF_FAST)
    flags |= MDB_NOSYNC;
  if (db_flags & DBF_FASTEST)
    flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;

  int dbr = mdb_env_create(env);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(*env, 1);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  MINFO("Opening monero blockchain at " << actual_filename);
  dbr = mdb_env_open(*env, actual_filename.c_str(), flags, 0664);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(*env, NULL, MDB_RDONLY, txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(*txn, "output_amounts", MDB_CREATE | MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, dbi);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(*txn, *dbi, compare_uint64);

  dbr = mdb_cursor_open(*txn, *dbi, cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
}

static void close_db(MDB_env *env, MDB_txn *txn, MDB_cursor *cur, MDB_dbi dbi)
{
  mdb_txn_abort(txn);
  mdb_cursor_close(cur);
  mdb_dbi_close(env, dbi);
  mdb_env_close(env);
}

static void get_num_outputs(MDB_txn *txn, MDB_cursor *cur, MDB_dbi dbi, uint64_t &pre_rct, uint64_t &rct)
{
  uint64_t amount = 0;
  MDB_val k = { sizeof(amount), (void*)&amount }, v;
  int dbr = mdb_cursor_get(cur, &k, &v, MDB_SET);
  if (dbr == MDB_NOTFOUND)
  {
    rct = 0;
  }
  else
  {
    if (dbr) throw std::runtime_error("Record 0 not found: " + std::string(mdb_strerror(dbr)));
    mdb_size_t count = 0;
    dbr = mdb_cursor_count(cur, &count);
    if (dbr) throw std::runtime_error("Failed to count records: " + std::string(mdb_strerror(dbr)));
    rct = count;
  }
  MDB_stat s;
  dbr = mdb_stat(txn, dbi, &s);
  if (dbr) throw std::runtime_error("Failed to count records: " + std::string(mdb_strerror(dbr)));
  if (s.ms_entries < rct) throw std::runtime_error("Inconsistent records: " + std::string(mdb_strerror(dbr)));
  pre_rct = s.ms_entries - rct;
}

static crypto::hash get_genesis_block_hash(const std::string &filename)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  int dbr;
  bool tx_active = false;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 1);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  if (dbr) throw std::runtime_error("Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "block_info", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi);
  mdb_set_dupsort(txn, dbi, compare_uint64);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  uint64_t zero = 0;
  MDB_val k = { sizeof(uint64_t), (void*)&zero}, v;
  dbr = mdb_get(txn, dbi, &k, &v);
  if (dbr) throw std::runtime_error("Failed to retrieve genesis block: " + std::string(mdb_strerror(dbr)));
  crypto::hash genesis_block_hash = *(const crypto::hash*)(((const uint64_t*)v.mv_data) + 5);
  mdb_dbi_close(env, dbi);
  mdb_txn_abort(txn);
  mdb_env_close(env);
  tx_active = false;
  return genesis_block_hash;
}

static std::vector<std::pair<uint64_t, uint64_t>> load_outputs(const std::string &filename)
{
  std::vector<std::pair<uint64_t, uint64_t>> outputs;
  uint64_t amount = std::numeric_limits<uint64_t>::max();
  FILE *f;

  f = fopen(filename.c_str(), "r");
  if (!f)
  {
    MERROR("Failed to load outputs from " << filename << ": " << strerror(errno));
    return {};
  }
  while (1)
  {
    char s[256];
    if (!fgets(s, sizeof(s), f))
    {
      MERROR("Error reading from " << filename << ": " << strerror(errno));
      break;
    }
    if (feof(f))
      break;
    const size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
      s[len - 1] = 0;
    if (!s[0])
      continue;
    uint64_t offset, num_offsets;
    if (sscanf(s, "@%" PRIu64, &amount) == 1)
    {
      continue;
    }
    if (amount == std::numeric_limits<uint64_t>::max())
    {
      MERROR("Bad format in " << filename);
      continue;
    }
    if (sscanf(s, "%" PRIu64 "*%" PRIu64, &offset, &num_offsets) == 2 && num_offsets < std::numeric_limits<uint64_t>::max() - offset)
    {
      while (num_offsets-- > 0)
        outputs.push_back(std::make_pair(amount, offset++));
    }
    else if (sscanf(s, "%" PRIu64, &offset) == 1)
    {
      outputs.push_back(std::make_pair(amount, offset));
    }
    else
    {
      MERROR("Bad format in " << filename);
      continue;
    }
  }
  fclose(f);
  return outputs;
}

static bool export_spent_outputs(MDB_cursor *cur, const std::string &filename)
{
  FILE *f = fopen(filename.c_str(), "w");
  if (!f)
  {
    MERROR("Failed to open " << filename << ": " << strerror(errno));
    return false;
  }

  uint64_t pending_amount = std::numeric_limits<uint64_t>::max();
  std::vector<uint64_t> pending_offsets;
  MDB_val k, v;
  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int dbr = mdb_cursor_get(cur, &k, &v, op);
    if (dbr == MDB_NOTFOUND)
      break;
    op = MDB_NEXT;
    if (dbr)
    {
      fclose(f);
      MERROR("Failed to enumerate spent outputs: " << mdb_strerror(dbr));
      return false;
    }
    const uint64_t amount = *(const uint64_t*)k.mv_data;
    const uint64_t offset = *(const uint64_t*)v.mv_data;
    if (!pending_offsets.empty() && (amount != pending_amount || pending_offsets.back()+1 != offset))
    {
      if (pending_offsets.size() == 1)
        fprintf(f, "%" PRIu64 "\n", pending_offsets.front());
      else
        fprintf(f, "%" PRIu64 "*%zu\n", pending_offsets.front(), pending_offsets.size());
      pending_offsets.clear();
    }
    if (pending_amount != amount)
    {
      fprintf(f, "@%" PRIu64 "\n", amount);
      pending_amount = amount;
    }
    pending_offsets.push_back(offset);
  }
  if (!pending_offsets.empty())
  {
    if (pending_offsets.size() == 1)
      fprintf(f, "%" PRIu64 "\n", pending_offsets.front());
    else
      fprintf(f, "%" PRIu64 "*%zu\n", pending_offsets.front(), pending_offsets.size());
    pending_offsets.clear();
  }
  fclose(f);
  return true;
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_blackball_db_dir = {
      "spent-output-db-dir", "Specify spent output database directory",
      get_default_db_path(),
  };
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<bool> arg_rct_only  = {"rct-only", "Only work on ringCT outputs", false};
  const command_line::arg_descriptor<bool> arg_check_subsets  = {"check-subsets", "Check ring subsets (very expensive)", false};
  const command_line::arg_descriptor<bool> arg_verbose  = {"verbose", "Verbose output)", false};
  const command_line::arg_descriptor<std::vector<std::string> > arg_inputs = {"inputs", "Path to Monero DB, and path to any fork DBs"};
  const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
    "db-sync-mode"
  , "Specify sync option, using format [safe|fast|fastest]:[nrecords_per_sync]." 
  , "fast:1000"
  };
  const command_line::arg_descriptor<std::string> arg_extra_spent_list = {"extra-spent-list", "Optional list of known spent outputs",""};
  const command_line::arg_descriptor<std::string> arg_export = {"export", "Filename to export the backball list to"};
  const command_line::arg_descriptor<bool> arg_force_chain_reaction_pass = {"force-chain-reaction-pass", "Run the chain reaction pass even if no new blockchain data was processed"};
  const command_line::arg_descriptor<bool> arg_historical_stat = {"historical-stat", "Report historical stat of spent outputs for every 10000 blocks window"};

  command_line::add_arg(desc_cmd_sett, arg_blackball_db_dir);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_rct_only);
  command_line::add_arg(desc_cmd_sett, arg_check_subsets);
  command_line::add_arg(desc_cmd_sett, arg_verbose);
  command_line::add_arg(desc_cmd_sett, arg_db_sync_mode);
  command_line::add_arg(desc_cmd_sett, arg_extra_spent_list);
  command_line::add_arg(desc_cmd_sett, arg_export);
  command_line::add_arg(desc_cmd_sett, arg_force_chain_reaction_pass);
  command_line::add_arg(desc_cmd_sett, arg_historical_stat);
  command_line::add_arg(desc_cmd_sett, arg_inputs);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::positional_options_description positional_options;
  positional_options.add(arg_inputs.name, -1);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    auto parser = po::command_line_parser(argc, argv).options(desc_options).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  mlog_configure(mlog_get_default_log_path("monero-blockchain-mark-spent-outputs.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  output_file_path = command_line::get_arg(vm, arg_blackball_db_dir);
  bool opt_rct_only = command_line::get_arg(vm, arg_rct_only);
  bool opt_check_subsets = command_line::get_arg(vm, arg_check_subsets);
  bool opt_verbose = command_line::get_arg(vm, arg_verbose);
  bool opt_force_chain_reaction_pass = command_line::get_arg(vm, arg_force_chain_reaction_pass);
  bool opt_historical_stat = command_line::get_arg(vm, arg_historical_stat);
  std::string opt_export = command_line::get_arg(vm, arg_export);
  std::string extra_spent_list = command_line::get_arg(vm, arg_extra_spent_list);
  std::vector<std::pair<uint64_t, uint64_t>> extra_spent_outputs = extra_spent_list.empty() ? std::vector<std::pair<uint64_t, uint64_t>>() : load_outputs(extra_spent_list);


  std::string db_sync_mode = command_line::get_arg(vm, arg_db_sync_mode);
  if (!parse_db_sync_mode(db_sync_mode))
  {
    MERROR("Invalid db sync mode: " << db_sync_mode);
    return 1;
  }

  const std::vector<std::string> inputs = command_line::get_arg(vm, arg_inputs);
  if (inputs.empty())
  {
    LOG_PRINT_L0("No inputs given");
    return 1;
  }

  const std::string cache_dir = (output_file_path / "spent-outputs-cache").string();
  init(cache_dir);

  LOG_PRINT_L0("Scanning for spent outputs...");

  const uint64_t start_blackballed_outputs = get_num_spent_outputs();

  tools::ringdb ringdb(output_file_path.string(), epee::string_tools::pod_to_hex(get_genesis_block_hash(inputs[0])));

  bool stop_requested = false;
  tools::signal_handler::install([&stop_requested](int type) {
    stop_requested = true;
  });

  int dbr = resize_env(cache_dir.c_str());
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));

  // open first db
  MDB_env *env0;
  MDB_txn *txn0;
  MDB_dbi dbi0;
  MDB_cursor *cur0;
  open_db(inputs[0], &env0, &txn0, &cur0, &dbi0);

  std::vector<output_data> work_spent;

  if (opt_historical_stat)
  {
    if (!start_blackballed_outputs)
    {
      MINFO("Spent outputs database is empty. Either you haven't run the analysis mode yet, or there is really no output marked as spent.");
      goto skip_secondary_passes;
    }
    MDB_txn *txn;
    int dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    MDB_cursor *cur;
    dbr = mdb_cursor_open(txn, dbi_spent, &cur);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));

    const uint64_t STAT_WINDOW = 10000;
    uint64_t outs_total = 0;
    uint64_t outs_spent = 0;
    std::unordered_map<uint64_t, uint64_t> outs_per_amount;
    uint64_t start_idx = 0, n_txes;
    uint64_t prev_height = 0;
    for_all_transactions(inputs[0], start_idx, n_txes, [&](bool last_tx, uint64_t height, const cryptonote::transaction_prefix &tx)->bool
    {
      if (height != prev_height)
      {
        if (height % 100 == 0) std::cout << "\r" << height << ": " << (100.0f * outs_spent / outs_total) << "% ( " << outs_spent << " / " << outs_total << " )       \r" << std::flush;
        if (height % STAT_WINDOW == 0)
        {
          uint64_t window_front = (height / STAT_WINDOW - 1) * STAT_WINDOW;
          uint64_t window_back = window_front + STAT_WINDOW - 1;
          LOG_PRINT_L0(window_front << "-" << window_back << ": " << (100.0f * outs_spent / outs_total) << "% ( " << outs_spent << " / " << outs_total << " )");
          outs_total = outs_spent = 0;
        }
      }
      prev_height = height;
      for (const auto &out: tx.vout)
      {
        ++outs_total;
        CHECK_AND_ASSERT_THROW_MES(out.target.type() == typeid(txout_to_key), "Out target type is not txout_to_key: height=" + std::to_string(height));
        uint64_t out_global_index = outs_per_amount[out.amount]++;
        if (is_output_spent(cur, output_data(out.amount, out_global_index)))
          ++outs_spent;
      }
      if (last_tx)
      {
        uint64_t window_front = (height / STAT_WINDOW) * STAT_WINDOW;
        uint64_t window_back = height;
        LOG_PRINT_L0(window_front << "-" << window_back << ": " << (100.0f * outs_spent / outs_total) << "% ( " << outs_spent << " / " << outs_total << " )");
      }
      if (stop_requested)
      {
        MINFO("Stopping scan...");
        return false;
      }
      return true;
    });
    mdb_cursor_close(cur);
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
    goto skip_secondary_passes;
  }

  if (!extra_spent_outputs.empty())
  {
    MINFO("Adding " << extra_spent_outputs.size() << " extra spent outputs");
    MDB_txn *txn;
    int dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    MDB_cursor *cur;
    dbr = mdb_cursor_open(txn, dbi_spent, &cur);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));

    std::vector<std::pair<uint64_t, uint64_t>> blackballs;
    for (const std::pair<uint64_t, uint64_t> &output: extra_spent_outputs)
    {
      if (!is_output_spent(cur, output_data(output.first, output.second)))
      {
        blackballs.push_back(output);
        if (add_spent_output(cur, output_data(output.first, output.second)))
          inc_stat(txn, output.first ? "pre-rct-extra" : "rct-ring-extra");
      }
    }
    if (!blackballs.empty())
    {
      ringdb.blackball(blackballs);
      blackballs.clear();
    }
    mdb_cursor_close(cur);
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
  }

  for (size_t n = 0; n < inputs.size(); ++n)
  {
    const std::string canonical = boost::filesystem::canonical(inputs[n]).string();
    uint64_t start_idx = get_processed_txidx(canonical);
    if (n > 0 && start_idx == 0)
    {
      start_idx = find_first_diverging_transaction(inputs[0], inputs[n]);
      LOG_PRINT_L0("First diverging transaction at " << start_idx);
    }
    LOG_PRINT_L0("Reading blockchain from " << inputs[n] << " from " << start_idx);
    MDB_txn *txn;
    int dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    MDB_cursor *cur;
    dbr = mdb_cursor_open(txn, dbi_spent, &cur);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));
    size_t records = 0;
    const std::string filename = inputs[n];
    std::vector<std::pair<uint64_t, uint64_t>> blackballs;
    uint64_t n_txes;
    for_all_transactions(filename, start_idx, n_txes, [&](const cryptonote::transaction_prefix &tx)->bool
    {
      std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
      const bool miner_tx = tx.vin.size() == 1 && tx.vin[0].type() == typeid(txin_gen);
      for (const auto &in: tx.vin)
      {
        if (in.type() != typeid(txin_to_key))
          continue;
        const auto &txin = boost::get<txin_to_key>(in);
        if (opt_rct_only && txin.amount != 0)
          continue;

        const std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
        if (n == 0)
          for (uint64_t out: absolute)
            add_key_image(txn, output_data(txin.amount, out), txin.k_image);

        std::vector<uint64_t> relative_ring;
        std::vector<uint64_t> new_ring = canonicalize(txin.key_offsets);
        const uint32_t ring_size = txin.key_offsets.size();
        const uint64_t instances = inc_ring_instances(txn, txin.amount, new_ring);
        uint64_t pa_total = 0, pa_spent = 0;
        if (!opt_rct_only)
          get_per_amount_outputs(txn, txin.amount, pa_total, pa_spent);
        if (n == 0 && ring_size == 1)
        {
          const std::pair<uint64_t, uint64_t> output = std::make_pair(txin.amount, absolute[0]);
          if (opt_verbose)
          {
            MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to being used in a 1-ring");
            std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
          }
          blackballs.push_back(output);
          if (add_spent_output(cur, output_data(txin.amount, absolute[0])))
            inc_stat(txn, txin.amount ? "pre-rct-ring-size-1" : "rct-ring-size-1");
        }
        else if (n == 0 && instances == new_ring.size())
        {
          for (size_t o = 0; o < new_ring.size(); ++o)
          {
            const std::pair<uint64_t, uint64_t> output = std::make_pair(txin.amount, absolute[o]);
            if (opt_verbose)
            {
              MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to being used in " << new_ring.size() << " identical " << new_ring.size() << "-rings");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            }
            blackballs.push_back(output);
            if (add_spent_output(cur, output_data(txin.amount, absolute[o])))
              inc_stat(txn, txin.amount ? "pre-rct-duplicate-rings" : "rct-duplicate-rings");
          }
        }
        else if (n == 0 && !opt_rct_only && pa_spent + 1 == pa_total)
        {
          for (size_t o = 0; o < pa_total; ++o)
          {
            const std::pair<uint64_t, uint64_t> output = std::make_pair(txin.amount, o);
            if (opt_verbose)
            {
              MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to as many outputs of that amount being spent as exist so far");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            }
            blackballs.push_back(output);
            if (add_spent_output(cur, output_data(txin.amount, o)))
              inc_stat(txn, txin.amount ? "pre-rct-full-count" : "rct-full-count");
          }
        }
        else if (n == 0 && opt_check_subsets && get_ring_subset_instances(txn, txin.amount, new_ring) >= new_ring.size())
        {
          for (size_t o = 0; o < new_ring.size(); ++o)
          {
            const std::pair<uint64_t, uint64_t> output = std::make_pair(txin.amount, absolute[o]);
            if (opt_verbose)
            {
              MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to being used in " << new_ring.size() << " subsets of " << new_ring.size() << "-rings");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            }
            blackballs.push_back(output);
            if (add_spent_output(cur, output_data(txin.amount, absolute[o])))
              inc_stat(txn, txin.amount ? "pre-rct-subset-rings" : "rct-subset-rings");
          }
        }
        else if (n > 0 && get_relative_ring(txn, txin.k_image, relative_ring))
        {
          MDEBUG("Key image " << txin.k_image << " already seen: rings " <<
              boost::join(relative_ring | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " ") <<
              ", " << boost::join(txin.key_offsets | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
          std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
          if (relative_ring != txin.key_offsets)
          {
            MDEBUG("Rings are different");
            std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            const std::vector<uint64_t> r0 = cryptonote::relative_output_offsets_to_absolute(relative_ring);
            const std::vector<uint64_t> r1 = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
            std::vector<uint64_t> common;
            for (uint64_t out: r0)
            {
              if (std::find(r1.begin(), r1.end(), out) != r1.end())
                common.push_back(out);
            }
            if (common.empty())
            {
              MERROR("Rings for the same key image are disjoint");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            }
            else if (common.size() == 1)
            {
              const std::pair<uint64_t, uint64_t> output = std::make_pair(txin.amount, common[0]);
              if (opt_verbose)
              {
                MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to being used in rings with a single common element");
                std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
              }
              blackballs.push_back(output);
              if (add_spent_output(cur, output_data(txin.amount, common[0])))
                inc_stat(txn, txin.amount ? "pre-rct-key-image-attack" : "rct-key-image-attack");
            }
            else
            {
              MDEBUG("The intersection has more than one element, it's still ok");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
              for (const auto &out: r0)
                if (std::find(common.begin(), common.end(), out) != common.end())
                  new_ring.push_back(out);
              new_ring = cryptonote::absolute_output_offsets_to_relative(new_ring);
            }
          }
        }
        if (n == 0)
        {
          set_relative_ring(txn, txin.k_image, new_ring);
          if (!opt_rct_only)
            inc_per_amount_outputs(txn, txin.amount, 0, 1);
        }
      }
      set_processed_txidx(txn, canonical, start_idx+1);
      if (!opt_rct_only)
      {
        for (const auto &out: tx.vout)
        {
          uint64_t amount = out.amount;
          if (miner_tx && tx.version >= 2)
            amount = 0;

          if (opt_rct_only && amount != 0)
            continue;
          if (out.target.type() != typeid(txout_to_key))
            continue;
          inc_per_amount_outputs(txn, amount, 1, 0);
        }
      }

      ++records;
      if (records >= records_per_sync)
      {
        if (!blackballs.empty())
        {
          ringdb.blackball(blackballs);
          blackballs.clear();
        }
        mdb_cursor_close(cur);
        dbr = mdb_txn_commit(txn);
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
        int dbr = resize_env(cache_dir.c_str());
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));
        dbr = mdb_txn_begin(env, NULL, 0, &txn);
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
        dbr = mdb_cursor_open(txn, dbi_spent, &cur);
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));
        records = 0;
      }

      if (stop_requested)
      {
        MINFO("Stopping scan...");
        return false;
      }
      return true;
    });
    mdb_cursor_close(cur);
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
    LOG_PRINT_L0("blockchain from " << inputs[n] << " processed till tx idx " << start_idx);
    if (stop_requested)
      break;
  }

  if (stop_requested)
    goto skip_secondary_passes;

  if (opt_force_chain_reaction_pass || get_num_spent_outputs() > start_blackballed_outputs)
  {
    MDB_txn *txn;
    dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    work_spent = get_spent_outputs(txn);
    mdb_txn_abort(txn);
  }

  while (!work_spent.empty())
  {
    LOG_PRINT_L0("Secondary pass on " << work_spent.size() << " spent outputs");

    int dbr = resize_env(cache_dir.c_str());
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));

    MDB_txn *txn;
    dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    MDB_cursor *cur;
    dbr = mdb_cursor_open(txn, dbi_spent, &cur);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));

    std::vector<std::pair<uint64_t, uint64_t>> blackballs;
    std::vector<output_data> scan_spent = std::move(work_spent);
    work_spent.clear();
    for (const output_data &od: scan_spent)
    {
      std::vector<crypto::key_image> key_images = get_key_images(txn, od);
      for (const crypto::key_image &ki: key_images)
      {
        std::vector<uint64_t> relative_ring;
        CHECK_AND_ASSERT_THROW_MES(get_relative_ring(txn, ki, relative_ring), "Relative ring not found");
        std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(relative_ring);
        size_t known = 0;
        uint64_t last_unknown = 0;
        for (uint64_t out: absolute)
        {
          output_data new_od(od.amount, out);
          if (is_output_spent(cur, new_od))
            ++known;
          else
            last_unknown = out;
        }
        if (known == absolute.size() - 1)
        {
          const std::pair<uint64_t, uint64_t> output = std::make_pair(od.amount, last_unknown);
          if (opt_verbose)
          {
            MINFO("Marking output " << output.first << "/" << output.second << " as spent, due to being used in a " <<
                absolute.size() << "-ring where all other outputs are known to be spent");
          }
          blackballs.push_back(output);
          if (add_spent_output(cur, output_data(od.amount, last_unknown)))
            inc_stat(txn, od.amount ? "pre-rct-chain-reaction" : "rct-chain-reaction");
          work_spent.push_back(output_data(od.amount, last_unknown));
        }
      }

      if (stop_requested)
      {
        MINFO("Stopping secondary passes. Secondary passes are not incremental, they will re-run fully.");
        return 0;
      }
    }
    if (!blackballs.empty())
    {
      ringdb.blackball(blackballs);
      blackballs.clear();
    }
    mdb_cursor_close(cur);
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
  }

skip_secondary_passes:
  uint64_t diff = get_num_spent_outputs() - start_blackballed_outputs;
  LOG_PRINT_L0(std::to_string(diff) << " new outputs marked as spent, " << get_num_spent_outputs() << " total outputs marked as spent");

  MDB_txn *txn;
  dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  uint64_t pre_rct = 0, rct = 0;
  get_num_outputs(txn0, cur0, dbi0, pre_rct, rct);
  MINFO("Total pre-rct outputs: " << pre_rct);
  MINFO("Total rct outputs: " << rct);
  static const struct { const char *key; uint64_t base; } stat_keys[] = {
    { "pre-rct-ring-size-1", pre_rct }, { "rct-ring-size-1", rct },
    { "pre-rct-duplicate-rings", pre_rct }, { "rct-duplicate-rings", rct },
    { "pre-rct-subset-rings", pre_rct }, { "rct-subset-rings", rct },
    { "pre-rct-full-count", pre_rct }, { "rct-full-count", rct },
    { "pre-rct-key-image-attack", pre_rct }, { "rct-key-image-attack", rct },
    { "pre-rct-extra", pre_rct }, { "rct-ring-extra", rct },
    { "pre-rct-chain-reaction", pre_rct }, { "rct-chain-reaction", rct },
  };
  for (const auto &key: stat_keys)
  {
    uint64_t data;
    if (!get_stat(txn, key.key, data))
      data = 0;
    float percent = key.base ? 100.0f * data / key.base : 0.0f;
    MINFO(key.key << ": " << data << " (" << percent << "%)");
  }
  mdb_txn_abort(txn);

  if (!opt_export.empty())
  {
    MDB_txn *txn;
    int dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    MDB_cursor *cur;
    dbr = mdb_cursor_open(txn, dbi_spent, &cur);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB cursor: " + std::string(mdb_strerror(dbr)));
    export_spent_outputs(cur, opt_export);
    mdb_cursor_close(cur);
    mdb_txn_abort(txn);
  }

  LOG_PRINT_L0("Blockchain spent output data exported OK");
  close_db(env0, txn0, cur0, dbi0);
  close();
  return 0;

  CATCH_ENTRY("Error", 1);
}
