// Copyright (c) 2014-2018, The Monero Project
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
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include "common/unordered_containers_boost_serialization.h"
#include "common/command_line.h"
#include "common/varint.h"
#include "serialization/crypto.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/db_types.h"
#include "wallet/ringdb.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static const char zerokey[8] = {0};
static const MDB_val zerokval = { sizeof(zerokey), (void *)zerokey };

static uint64_t records_per_sync = 200;
static uint64_t db_flags = 0;
static MDB_dbi dbi_relative_rings;
static MDB_dbi dbi_outputs;
static MDB_dbi dbi_processed_txidx;
static MDB_dbi dbi_spent;
static MDB_dbi dbi_ring_instances;
static MDB_dbi dbi_newly_spent;
static MDB_env *env = NULL;

struct output_data
{
  uint64_t amount;
  uint64_t index;
  output_data(): amount(0), index(0) {}
  output_data(uint64_t a, uint64_t i): amount(a), index(i) {}
  bool operator==(const output_data &other) const { return other.amount == amount && other.index == index; }
};

//
// relative_rings: key_image -> vector<uint64_t>
// outputs: 128 bits -> set of key images
// processed_txidx: string -> uint64_t
// spent: 128 bits, zerokval
// ring_instances: vector<uint64_t> -> uint64_t
// newly_spent: 128 bits, zerokval
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

  MINFO("Creating blackball cache in " << cache_filename);

  tools::create_directories_if_necessary(cache_filename);

  int flags = 0;
  if (db_flags & DBF_FAST)
    flags |= MDB_NOSYNC;
  if (db_flags & DBF_FASTEST)
    flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;

  dbr = mdb_env_create(&env);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 6);
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
  mdb_set_dupsort(txn, dbi_spent, compare_double64);

  dbr = mdb_dbi_open(txn, "ring_instances", MDB_CREATE, &dbi_ring_instances);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn, "newly_spent", MDB_CREATE | MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi_newly_spent);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(txn, dbi_newly_spent, compare_double64);

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
    mdb_dbi_close(env, dbi_spent);
    mdb_dbi_close(env, dbi_ring_instances);
    mdb_dbi_close(env, dbi_newly_spent);
    mdb_env_close(env);
    env = NULL;
  }
}

static std::string compress_ring(const std::vector<uint64_t> &ring)
{
  std::string s;
  for (uint64_t out: ring)
    s += tools::get_varint_data(out);
  return s;
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

static bool for_all_transactions(const std::string &filename, uint64_t &start_idx, const std::function<bool(const cryptonote::transaction_prefix&)> &f)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  MDB_cursor *cur;
  int dbr;
  bool tx_active = false;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 2);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  if (dbr) throw std::runtime_error("Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "txs_pruned", MDB_INTEGERKEY, &dbi);
  if (dbr)
    dbr = mdb_dbi_open(txn, "txs", MDB_INTEGERKEY, &dbi);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_cursor_open(txn, dbi, &cur);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  MDB_val k;
  MDB_val v;
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
    std::stringstream ss;
    ss << bd;
    binary_archive<false> ba(ss);
    bool r = do_serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");

    start_idx = *(uint64_t*)k.mv_data;
    if (!f(tx)) {
      fret = false;
      break;
    }
  }

  mdb_cursor_close(cur);
  mdb_txn_commit(txn);
  tx_active = false;
  mdb_dbi_close(env, dbi);
  mdb_env_close(env);
  return fret;
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

static uint64_t get_num_spent_outputs(bool newly)
{
  MDB_txn *txn;
  bool tx_active = false;

  int dbr = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  MDB_cursor *cur;
  dbr = mdb_cursor_open(txn, newly ? dbi_newly_spent : dbi_spent, &cur);
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

  mdb_cursor_close(cur);
  dbr = mdb_txn_commit(txn);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn: " + std::string(mdb_strerror(dbr)));
  tx_active = false;

  return count;
}

static void add_spent_output(MDB_txn *txn, const output_data &od, bool newly)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, newly ? dbi_newly_spent : dbi_spent, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for spent outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val v = {sizeof(od), (void*)&od};
  dbr = mdb_cursor_put(cur, (MDB_val *)&zerokval, &v, MDB_NODUPDATA);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_KEYEXIST, "Failed to add spent output: " + std::string(mdb_strerror(dbr)));
  mdb_cursor_close(cur);
}

static bool is_output_spent(MDB_txn *txn, const output_data &od, bool newly)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, newly ? dbi_newly_spent : dbi_spent, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for spent outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val v = {sizeof(od), (void*)&od};
  dbr = mdb_cursor_get(cur, (MDB_val *)&zerokval, &v, MDB_GET_BOTH);
  CHECK_AND_ASSERT_THROW_MES(!dbr || dbr == MDB_NOTFOUND, "Failed to get spent output: " + std::string(mdb_strerror(dbr)));
  bool spent = dbr == 0;
  mdb_cursor_close(cur);
  return spent;
}

static std::vector<output_data> get_spent_outputs(MDB_txn *txn, bool newly)
{
  MDB_cursor *cur;
  int dbr = mdb_cursor_open(txn, newly ? dbi_newly_spent : dbi_spent, &cur);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to open cursor for spent outputs: " + std::string(mdb_strerror(dbr)));
  MDB_val k, v;
  uint64_t count = 0;
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
    const output_data *od = (const output_data*)v.mv_data;
    outs.push_back(*od);
    dbr = mdb_cursor_get(cur, &k, &v, MDB_NEXT);
    if (dbr == MDB_NOTFOUND)
      break;
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get next spent output: " + std::string(mdb_strerror(dbr)));
  }
  mdb_cursor_close(cur);
  return outs;
}

static void clear_spent_outputs(MDB_txn *txn, bool newly)
{
  int dbr = mdb_drop(txn, newly ? dbi_newly_spent : dbi_spent, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to clear spent outputs: " + std::string(mdb_strerror(dbr)));
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
  const std::string sring = compress_ring(ring);
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

static uint64_t get_ring_instances(MDB_txn *txn, const std::vector<uint64_t> &ring)
{
  const std::string sring = keep_under_511(compress_ring(ring));
  MDB_val k, v;
  k.mv_data = (void*)sring.data();
  k.mv_size = sring.size();
  int dbr = mdb_get(txn, dbi_ring_instances, &k, &v);
  if (dbr == MDB_NOTFOUND)
    return 0;
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get ring instances: " + std::string(mdb_strerror(dbr)));
  return *(const uint64_t*)v.mv_data;
}

static void set_ring_instances(MDB_txn *txn, const std::vector<uint64_t> &ring, uint64_t count)
{
  const std::string sring = keep_under_511(compress_ring(ring));
  MDB_val k, v;
  k.mv_data = (void*)sring.data();
  k.mv_size = sring.size();
  v.mv_data = &count;
  v.mv_size = sizeof(count);
  int dbr = mdb_put(txn, dbi_ring_instances, &k, &v, 0);
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to get ring instances: " + std::string(mdb_strerror(dbr)));
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

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  std::string default_db_type = "lmdb";

  std::string available_dbs = cryptonote::blockchain_db_types(", ");
  available_dbs = "available: " + available_dbs;

  uint32_t log_level = 0;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string, false, true, 2> arg_blackball_db_dir = {
      "blackball-db-dir", "Specify blackball database directory",
      get_default_db_path(),
      {{ &arg_testnet_on, &arg_stagenet_on }},
      [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
        if (testnet_stagenet[0])
          return (boost::filesystem::path(val) / "testnet").string();
        else if (testnet_stagenet[1])
          return (boost::filesystem::path(val) / "stagenet").string();
        return val;
      }
  };
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<std::string> arg_database = {
    "database", available_dbs.c_str(), default_db_type
  };
  const command_line::arg_descriptor<bool> arg_rct_only  = {"rct-only", "Only work on ringCT outputs", false};
  const command_line::arg_descriptor<std::vector<std::string> > arg_inputs = {"inputs", "Path to Monero DB, and path to any fork DBs"};
  const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
    "db-sync-mode"
  , "Specify sync option, using format [safe|fast|fastest]:[nrecords_per_sync]." 
  , "fast:1000"
  };

  command_line::add_arg(desc_cmd_sett, arg_blackball_db_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_database);
  command_line::add_arg(desc_cmd_sett, arg_rct_only);
  command_line::add_arg(desc_cmd_sett, arg_db_sync_mode);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-blackball.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  output_file_path = command_line::get_arg(vm, arg_blackball_db_dir);
  bool opt_rct_only = command_line::get_arg(vm, arg_rct_only);

  std::string db_type = command_line::get_arg(vm, arg_database);
  if (!cryptonote::blockchain_valid_db_type(db_type))
  {
    std::cerr << "Invalid database type: " << db_type << std::endl;
    return 1;
  }

  std::string db_sync_mode = command_line::get_arg(vm, arg_db_sync_mode);
  if (!parse_db_sync_mode(db_sync_mode))
  {
    MERROR("Invalid db sync mode: " << db_sync_mode);
    return 1;
  }

  // If we wanted to use the memory pool, we would set up a fake_core.

  // Use Blockchain instead of lower-level BlockchainDB for two reasons:
  // 1. Blockchain has the init() method for easy setup
  // 2. exporter needs to use get_current_blockchain_height(), get_block_id_by_height(), get_block_by_hash()
  //
  // cannot match blockchain_storage setup above with just one line,
  // e.g.
  //   Blockchain* core_storage = new Blockchain(NULL);
  // because unlike blockchain_storage constructor, which takes a pointer to
  // tx_memory_pool, Blockchain's constructor takes tx_memory_pool object.
  LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
  const std::vector<std::string> inputs = command_line::get_arg(vm, arg_inputs);
  if (inputs.empty())
  {
    LOG_PRINT_L0("No inputs given");
    return 1;
  }
  std::vector<std::unique_ptr<Blockchain>> core_storage(inputs.size());
  Blockchain *blockchain = NULL;
  tx_memory_pool m_mempool(*blockchain);
  for (size_t n = 0; n < inputs.size(); ++n)
  {
    core_storage[n].reset(new Blockchain(m_mempool));

    BlockchainDB* db = new_db(db_type);
    if (db == NULL)
    {
      LOG_ERROR("Attempted to use non-existent database type: " << db_type);
      throw std::runtime_error("Attempting to use non-existent database type");
    }
    LOG_PRINT_L0("database: " << db_type);

    std::string filename = (boost::filesystem::path(inputs[n]) / db->get_db_name()).string();
    while (boost::ends_with(filename, "/") || boost::ends_with(filename, "\\"))
      filename.pop_back();
    LOG_PRINT_L0("Loading blockchain from folder " << filename << " ...");

    try
    {
      db->open(filename, DBF_RDONLY);
    }
    catch (const std::exception& e)
    {
      LOG_PRINT_L0("Error opening database: " << e.what());
      return 1;
    }
    r = core_storage[n]->init(db, net_type);

    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
    LOG_PRINT_L0("Source blockchain storage initialized OK");
  }

  const std::string cache_dir = (output_file_path / "blackball-cache").string();
  init(cache_dir);

  LOG_PRINT_L0("Scanning for blackballable outputs...");

  size_t done = 0;

  const uint64_t start_blackballed_outputs = get_num_spent_outputs(false);

  cryptonote::block b = core_storage[0]->get_db().get_block_from_height(0);
  tools::ringdb ringdb(output_file_path.string(), epee::string_tools::pod_to_hex(get_block_hash(b)));

  bool stop_requested = false;
  tools::signal_handler::install([&stop_requested](int type) {
    stop_requested = true;
  });

  int dbr = resize_env(cache_dir.c_str());
  CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));

  for (size_t n = 0; n < inputs.size(); ++n)
  {
    const std::string canonical = boost::filesystem::canonical(inputs[n]).string();
    uint64_t start_idx = get_processed_txidx(canonical);
    LOG_PRINT_L0("Reading blockchain from " << inputs[n] << " from " << start_idx);
    MDB_txn *txn;
    int dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
    size_t records = 0;
    const uint64_t n_txes = core_storage[n]->get_db().get_tx_count();
    const std::string filename = (boost::filesystem::path(inputs[n]) / core_storage[n]->get_db().get_db_name()).string();
    std::vector<crypto::public_key> blackballs;
    for_all_transactions(filename, start_idx, [&](const cryptonote::transaction_prefix &tx)->bool
    {
      std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
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
        uint64_t instances = get_ring_instances(txn, new_ring);
        ++instances;
        set_ring_instances(txn, new_ring, instances);
        if (ring_size == 1)
        {
          const crypto::public_key pkey = core_storage[n]->get_output_key(txin.amount, absolute[0]);
          MINFO("Blackballing output " << pkey << ", due to being used in a 1-ring");
          std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
          blackballs.push_back(pkey);
          add_spent_output(txn, output_data(txin.amount, absolute[0]), true);
        }
        else if (instances == new_ring.size())
        {
          for (size_t o = 0; o < new_ring.size(); ++o)
          {
            const crypto::public_key pkey = core_storage[n]->get_output_key(txin.amount, absolute[o]);
            MINFO("Blackballing output " << pkey << ", due to being used in " << new_ring.size() << " identical " << new_ring.size() << "-rings");
            std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
            blackballs.push_back(pkey);
            add_spent_output(txn, output_data(txin.amount, absolute[o]), true);
          }
        }
        else if (get_relative_ring(txn, txin.k_image, relative_ring))
        {
          MINFO("Key image " << txin.k_image << " already seen: rings " <<
              boost::join(relative_ring | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " ") <<
              ", " << boost::join(txin.key_offsets | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
          std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
          if (relative_ring != txin.key_offsets)
          {
            MINFO("Rings are different");
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
              const crypto::public_key pkey = core_storage[n]->get_output_key(txin.amount, common[0]);
              MINFO("Blackballing output " << pkey << ", due to being used in rings with a single common element");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
              blackballs.push_back(pkey);
              add_spent_output(txn, output_data(txin.amount, common[0]), true);
            }
            else
            {
              MINFO("The intersection has more than one element, it's still ok");
              std::cout << "\r" << start_idx << "/" << n_txes << "         \r" << std::flush;
              for (const auto &out: r0)
                if (std::find(common.begin(), common.end(), out) != common.end())
                  new_ring.push_back(out);
              new_ring = cryptonote::absolute_output_offsets_to_relative(new_ring);
            }
          }
        }
        set_relative_ring(txn, txin.k_image, new_ring);
      }
      if (!blackballs.empty())
      {
        ringdb.blackball(blackballs);
        blackballs.clear();
      }
      set_processed_txidx(txn, canonical, start_idx);

      ++records;
      if (records >= records_per_sync)
      {
        dbr = mdb_txn_commit(txn);
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
        int dbr = resize_env(cache_dir.c_str());
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));
        dbr = mdb_txn_begin(env, NULL, 0, &txn);
        CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
        records = 0;
      }

      if (stop_requested)
      {
        MINFO("Stopping scan, secondary passes will still happen...");
        return false;
      }
      return true;
    });
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
    LOG_PRINT_L0("blockchain from " << inputs[n] << " processed till tx idx " << start_idx);
    if (stop_requested)
      break;
  }

  while (get_num_spent_outputs(true) != 0)
  {
    LOG_PRINT_L0("Secondary pass due to " << get_num_spent_outputs(true) << " newly found spent outputs");

    int dbr = resize_env(cache_dir.c_str());
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to resize LMDB database: " + std::string(mdb_strerror(dbr)));

    MDB_txn *txn;
    dbr = mdb_txn_begin(env, NULL, 0, &txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));

    std::vector<output_data> work_spent = get_spent_outputs(txn, true);
    clear_spent_outputs(txn, true);

    for (const auto &od: work_spent)
      add_spent_output(txn, od, false);

    std::vector<crypto::public_key> blackballs;
    for (const output_data &od: work_spent)
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
          if (is_output_spent(txn, new_od, false))
            ++known;
          else
            last_unknown = out;
        }
        if (known == absolute.size() - 1)
        {
          const crypto::public_key pkey = core_storage[0]->get_output_key(od.amount, last_unknown);
          MINFO("Blackballing output " << pkey << ", due to being used in a " <<
              absolute.size() << "-ring where all other outputs are known to be spent");
          blackballs.push_back(pkey);
          add_spent_output(txn, output_data(od.amount, last_unknown), true);
        }
      }
    }
    if (!blackballs.empty())
    {
      ringdb.blackball(blackballs);
      blackballs.clear();
    }
    dbr = mdb_txn_commit(txn);
    CHECK_AND_ASSERT_THROW_MES(!dbr, "Failed to commit txn creating/opening database: " + std::string(mdb_strerror(dbr)));
  }

  uint64_t diff = get_num_spent_outputs(false) - start_blackballed_outputs;
  LOG_PRINT_L0(std::to_string(diff) << " new outputs blackballed, " << get_num_spent_outputs(false) << " total outputs blackballed");
  LOG_PRINT_L0("Blockchain blackball data exported OK");
  close();
  return 0;

  CATCH_ENTRY("Error", 1);
}
