// Copyright (c) 2018-2022, The Monero Project

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

#include <array>
#include <lmdb.h>
#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include "common/command_line.h"
#include "common/pruning.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/lmdb/db_lmdb.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

#define MDB_val_set(var, val)   MDB_val var = {sizeof(val), (void *)&val}

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static std::string db_path;

// default to fast:1
static uint64_t records_per_sync = 128;
static const size_t slack = 512 * 1024 * 1024;

static std::error_code replace_file(const boost::filesystem::path& replacement_name, const boost::filesystem::path& replaced_name)
{
  std::error_code ec = tools::replace_file(replacement_name.string(), replaced_name.string());
  if (ec)
    MERROR("Error renaming " << replacement_name << " to " << replaced_name << ": " << ec.message());
  return ec;
}

static void open(MDB_env *&env, const boost::filesystem::path &path, uint64_t db_flags, bool readonly)
{
  int dbr;
  int flags = 0;

  if (db_flags & DBF_FAST)
    flags |= MDB_NOSYNC;
  if (db_flags & DBF_FASTEST)
    flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;
  if (readonly)
    flags |= MDB_RDONLY;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 32);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_open(env, path.string().c_str(), flags, 0664);
  if (dbr) throw std::runtime_error("Failed to open database file '"
      + path.string() + "': " + std::string(mdb_strerror(dbr)));
}

static void close(MDB_env *env)
{
  mdb_env_close(env);
}

static void add_size(MDB_env *env, uint64_t bytes)
{
  try
  {
    boost::filesystem::path path(db_path);
    boost::filesystem::space_info si = boost::filesystem::space(path);
    if(si.available < bytes)
    {
      MERROR("!! WARNING: Insufficient free space to extend database !!: " <<
          (si.available >> 20L) << " MB available, " << (bytes >> 20L) << " MB needed");
      return;
    }
  }
  catch(...)
  {
    // print something but proceed.
    MWARNING("Unable to query free disk space.");
  }

  MDB_envinfo mei;
  mdb_env_info(env, &mei);
  MDB_stat mst;
  mdb_env_stat(env, &mst);

  uint64_t new_mapsize = (uint64_t)mei.me_mapsize + bytes;
  new_mapsize += (new_mapsize % mst.ms_psize);

  int result = mdb_env_set_mapsize(env, new_mapsize);
  if (result)
    throw std::runtime_error("Failed to set new mapsize to " + std::to_string(new_mapsize) + ": " + std::string(mdb_strerror(result)));

  MGINFO("LMDB Mapsize increased." << "  Old: " << mei.me_mapsize / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB");
}

static void check_resize(MDB_env *env, size_t bytes)
{
  MDB_envinfo mei;
  MDB_stat mst;

  mdb_env_info(env, &mei);
  mdb_env_stat(env, &mst);

  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;
  if (size_used + bytes + slack >= mei.me_mapsize)
    add_size(env, size_used + bytes + 2 * slack - mei.me_mapsize);
}

static bool resize_point(size_t nrecords, MDB_env *env, MDB_txn **txn, size_t &bytes)
{
  if (nrecords % records_per_sync && bytes <= slack / 2)
    return false;
  int dbr = mdb_txn_commit(*txn);
  if (dbr) throw std::runtime_error("Failed to commit txn: " + std::string(mdb_strerror(dbr)));
  check_resize(env, bytes);
  dbr = mdb_txn_begin(env, NULL, 0, txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  bytes = 0;
  return true;
}

static void copy_table(MDB_env *env0, MDB_env *env1, const char *table, unsigned int flags, unsigned int putflags, int (*cmp)(const MDB_val*, const MDB_val*)=0)
{
  MDB_dbi dbi0, dbi1;
  MDB_txn *txn0, *txn1;
  MDB_cursor *cur0, *cur1;
  bool tx_active0 = false, tx_active1 = false;
  int dbr;

  MINFO("Copying " << table);

  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){
    if (tx_active1) mdb_txn_abort(txn1);
    if (tx_active0) mdb_txn_abort(txn0);
  });

  dbr = mdb_txn_begin(env0, NULL, MDB_RDONLY, &txn0);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  tx_active0 = true;
  dbr = mdb_txn_begin(env1, NULL, 0, &txn1);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  tx_active1 = true;

  dbr = mdb_dbi_open(txn0, table, flags, &dbi0);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  if (cmp)
    ((flags & MDB_DUPSORT) ? mdb_set_dupsort : mdb_set_compare)(txn0, dbi0, cmp);

  dbr = mdb_dbi_open(txn1, table, flags, &dbi1);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  if (cmp)
    ((flags & MDB_DUPSORT) ? mdb_set_dupsort : mdb_set_compare)(txn1, dbi1, cmp);

  dbr = mdb_txn_commit(txn1);
  if (dbr) throw std::runtime_error("Failed to commit txn: " + std::string(mdb_strerror(dbr)));
  tx_active1 = false;
  MDB_stat stats;
  dbr = mdb_env_stat(env0, &stats);
  if (dbr) throw std::runtime_error("Failed to stat " + std::string(table) + " LMDB table: " + std::string(mdb_strerror(dbr)));
  check_resize(env1, (stats.ms_branch_pages + stats.ms_overflow_pages + stats.ms_leaf_pages) * stats.ms_psize);
  dbr = mdb_txn_begin(env1, NULL, 0, &txn1);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  tx_active1 = true;

  dbr = mdb_drop(txn1, dbi1, 0);
  if (dbr) throw std::runtime_error("Failed to empty " + std::string(table) + " LMDB table: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_cursor_open(txn0, dbi0, &cur0);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_cursor_open(txn1, dbi1, &cur1);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  MDB_val k;
  MDB_val v;
  MDB_cursor_op op = MDB_FIRST;
  size_t nrecords = 0, bytes = 0;
  while (1)
  {
    int ret = mdb_cursor_get(cur0, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw std::runtime_error("Failed to enumerate " + std::string(table) + " records: " + std::string(mdb_strerror(ret)));

    bytes += k.mv_size + v.mv_size;
    if (resize_point(++nrecords, env1, &txn1, bytes))
    {
      dbr = mdb_cursor_open(txn1, dbi1, &cur1);
      if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
    }

    ret = mdb_cursor_put(cur1, &k, &v, putflags);
    if (ret)
      throw std::runtime_error("Failed to write " + std::string(table) + " record: " + std::string(mdb_strerror(ret)));
  }

  mdb_cursor_close(cur1);
  mdb_cursor_close(cur0);
  mdb_txn_commit(txn1);
  tx_active1 = false;
  mdb_txn_commit(txn0);
  tx_active0 = false;
  mdb_dbi_close(env1, dbi1);
  mdb_dbi_close(env0, dbi0);
}

static bool is_v1_tx(MDB_cursor *c_txs_pruned, MDB_val *tx_id)
{
  MDB_val v;
  int ret = mdb_cursor_get(c_txs_pruned, tx_id, &v, MDB_SET);
  if (ret)
    throw std::runtime_error("Failed to find transaction pruned data: " + std::string(mdb_strerror(ret)));
  if (v.mv_size == 0)
    throw std::runtime_error("Invalid transaction pruned data");
  return cryptonote::is_v1_tx(cryptonote::blobdata_ref{(const char*)v.mv_data, v.mv_size});
}

static void prune(MDB_env *env0, MDB_env *env1)
{
  MDB_dbi dbi0_blocks, dbi0_txs_pruned, dbi0_txs_prunable, dbi0_tx_indices, dbi1_txs_prunable, dbi1_txs_prunable_tip, dbi1_properties;
  MDB_txn *txn0, *txn1;
  MDB_cursor *cur0_txs_pruned, *cur0_txs_prunable, *cur0_tx_indices, *cur1_txs_prunable, *cur1_txs_prunable_tip;
  bool tx_active0 = false, tx_active1 = false;
  int dbr;

  MGINFO("Creating pruned txs_prunable");

  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){
    if (tx_active1) mdb_txn_abort(txn1);
    if (tx_active0) mdb_txn_abort(txn0);
  });

  dbr = mdb_txn_begin(env0, NULL, MDB_RDONLY, &txn0);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  tx_active0 = true;
  dbr = mdb_txn_begin(env1, NULL, 0, &txn1);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  tx_active1 = true;

  dbr = mdb_dbi_open(txn0, "txs_pruned", MDB_INTEGERKEY, &dbi0_txs_pruned);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn0, dbi0_txs_pruned, BlockchainLMDB::compare_uint64);
  dbr = mdb_cursor_open(txn0, dbi0_txs_pruned, &cur0_txs_pruned);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn0, "txs_prunable", MDB_INTEGERKEY, &dbi0_txs_prunable);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn0, dbi0_txs_prunable, BlockchainLMDB::compare_uint64);
  dbr = mdb_cursor_open(txn0, dbi0_txs_prunable, &cur0_txs_prunable);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn0, "tx_indices", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi0_tx_indices);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(txn0, dbi0_tx_indices, BlockchainLMDB::compare_hash32);
  dbr = mdb_cursor_open(txn0, dbi0_tx_indices, &cur0_tx_indices);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn1, "txs_prunable", MDB_INTEGERKEY, &dbi1_txs_prunable);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_compare(txn1, dbi1_txs_prunable, BlockchainLMDB::compare_uint64);
  dbr = mdb_cursor_open(txn1, dbi1_txs_prunable, &cur1_txs_prunable);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn1, "txs_prunable_tip", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, &dbi1_txs_prunable_tip);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  mdb_set_dupsort(txn1, dbi1_txs_prunable_tip, BlockchainLMDB::compare_uint64);
  dbr = mdb_cursor_open(txn1, dbi1_txs_prunable_tip, &cur1_txs_prunable_tip);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_drop(txn1, dbi1_txs_prunable, 0);
  if (dbr) throw std::runtime_error("Failed to empty LMDB table: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_drop(txn1, dbi1_txs_prunable_tip, 0);
  if (dbr) throw std::runtime_error("Failed to empty LMDB table: " + std::string(mdb_strerror(dbr)));

  dbr = mdb_dbi_open(txn1, "properties", 0, &dbi1_properties);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));

  MDB_val k, v;
  uint32_t pruning_seed = tools::make_pruning_seed(tools::get_random_stripe(), CRYPTONOTE_PRUNING_LOG_STRIPES);
  static char pruning_seed_key[] = "pruning_seed";
  k.mv_data = pruning_seed_key;
  k.mv_size = strlen("pruning_seed") + 1;
  v.mv_data = (void*)&pruning_seed;
  v.mv_size = sizeof(pruning_seed);
  dbr = mdb_put(txn1, dbi1_properties, &k, &v, 0);
  if (dbr) throw std::runtime_error("Failed to save pruning seed: " + std::string(mdb_strerror(dbr)));

  MDB_stat stats;
  dbr = mdb_dbi_open(txn0, "blocks", 0, &dbi0_blocks);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_stat(txn0, dbi0_blocks, &stats);
  if (dbr) throw std::runtime_error("Failed to query size of blocks: " + std::string(mdb_strerror(dbr)));
  mdb_dbi_close(env0, dbi0_blocks);
  const uint64_t blockchain_height = stats.ms_entries;
  size_t nrecords = 0, bytes = 0;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(cur0_tx_indices, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret) throw std::runtime_error("Failed to enumerate records: " + std::string(mdb_strerror(ret)));

    const txindex *ti = (const txindex*)v.mv_data;
    const uint64_t block_height = ti->data.block_id;
    MDB_val_set(kk, ti->data.tx_id);
    if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS >= blockchain_height)
    {
      MDEBUG(block_height << "/" << blockchain_height << " is in tip");
      MDB_val_set(vv, block_height);
      dbr = mdb_cursor_put(cur1_txs_prunable_tip, &kk, &vv, 0);
      if (dbr) throw std::runtime_error("Failed to write prunable tx tip data: " + std::string(mdb_strerror(dbr)));
      bytes += kk.mv_size + vv.mv_size;
    }
    if (tools::has_unpruned_block(block_height, blockchain_height, pruning_seed) || is_v1_tx(cur0_txs_pruned, &kk))
    {
      MDB_val vv;
      dbr = mdb_cursor_get(cur0_txs_prunable, &kk, &vv, MDB_SET);
      if (dbr) throw std::runtime_error("Failed to read prunable tx data: " + std::string(mdb_strerror(dbr)));
      bytes += kk.mv_size + vv.mv_size;
      if (resize_point(++nrecords, env1, &txn1, bytes))
      {
        dbr = mdb_cursor_open(txn1, dbi1_txs_prunable, &cur1_txs_prunable);
        if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
        dbr = mdb_cursor_open(txn1, dbi1_txs_prunable_tip, &cur1_txs_prunable_tip);
        if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));
      }
      dbr = mdb_cursor_put(cur1_txs_prunable, &kk, &vv, 0);
      if (dbr) throw std::runtime_error("Failed to write prunable tx data: " + std::string(mdb_strerror(dbr)));
    }
    else
    {
      MDEBUG("" << block_height << "/" << blockchain_height << " should be pruned, dropping");
    }
  }

  mdb_cursor_close(cur1_txs_prunable_tip);
  mdb_cursor_close(cur1_txs_prunable);
  mdb_cursor_close(cur0_txs_prunable);
  mdb_cursor_close(cur0_txs_pruned);
  mdb_cursor_close(cur0_tx_indices);
  mdb_txn_commit(txn1);
  tx_active1 = false;
  mdb_txn_commit(txn0);
  tx_active0 = false;
  mdb_dbi_close(env1, dbi1_properties);
  mdb_dbi_close(env1, dbi1_txs_prunable_tip);
  mdb_dbi_close(env1, dbi1_txs_prunable);
  mdb_dbi_close(env0, dbi0_txs_prunable);
  mdb_dbi_close(env0, dbi0_txs_pruned);
  mdb_dbi_close(env0, dbi0_tx_indices);
}

static bool parse_db_sync_mode(std::string db_sync_mode, uint64_t &db_flags)
{
  std::vector<std::string> options;
  boost::trim(db_sync_mode);
  boost::split(options, db_sync_mode, boost::is_any_of(" :"));

  for(const auto &option : options)
    MDEBUG("option: " << option);

  // default to fast:async:1
  uint64_t DEFAULT_FLAGS = DBF_FAST;

  db_flags = 0;

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
      return false;
  }

  if(options.size() >= 2 && !safemode)
  {
    char *endptr;
    uint64_t bps = strtoull(options[1].c_str(), &endptr, 0);
    if (*endptr != '\0')
      return false;
    records_per_sync = bps;
  }

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
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
    "db-sync-mode"
  , "Specify sync option, using format [safe|fast|fastest]:[nrecords_per_sync]."
  , "fast:1000"
  };
  const command_line::arg_descriptor<bool> arg_copy_pruned_database  = {"copy-pruned-database",  "Copy database anyway if already pruned"};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_db_sync_mode);
  command_line::add_arg(desc_cmd_sett, arg_copy_pruned_database);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    auto parser = po::command_line_parser(argc, argv).options(desc_options);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-prune.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  MINFO("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  bool opt_copy_pruned_database = command_line::get_arg(vm, arg_copy_pruned_database);
  std::string data_dir = command_line::get_arg(vm, cryptonote::arg_data_dir);
  while (boost::ends_with(data_dir, "/") || boost::ends_with(data_dir, "\\"))
    data_dir.pop_back();

  std::string db_sync_mode = command_line::get_arg(vm, arg_db_sync_mode);
  uint64_t db_flags = 0;
  if (!parse_db_sync_mode(db_sync_mode, db_flags))
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
  MINFO("Initializing source blockchain (BlockchainDB)");
  std::array<std::unique_ptr<Blockchain>, 2> core_storage;
  Blockchain *blockchain = NULL;
  tx_memory_pool m_mempool(*blockchain);
  boost::filesystem::path paths[2];
  bool already_pruned = false;
  for (size_t n = 0; n < core_storage.size(); ++n)
  {
    core_storage[n].reset(new Blockchain(m_mempool));

    BlockchainDB* db = new_db();
    if (db == NULL)
    {
      MERROR("Failed to initialize a database");
      throw std::runtime_error("Failed to initialize a database");
    }

    if (n == 1)
    {
      paths[1] = boost::filesystem::path(data_dir) / (db->get_db_name() + "-pruned");
      if (boost::filesystem::exists(paths[1]))
      {
        if (!boost::filesystem::is_directory(paths[1]))
        {
          MERROR("LMDB needs a directory path, but a file was passed: " << paths[1].string());
          return 1;
        }
      }
      else
      {
        if (!boost::filesystem::create_directories(paths[1]))
        {
          MERROR("Failed to create directory: " << paths[1].string());
          return 1;
        }
      }
      db_path = paths[1].string();
    }
    else
    {
      paths[0] = boost::filesystem::path(data_dir) / db->get_db_name();
    }

    MINFO("Loading blockchain from folder " << paths[n] << " ...");

    try
    {
      db->open(paths[n].string(), n == 0 ? DBF_RDONLY : 0);
    }
    catch (const std::exception& e)
    {
      MERROR("Error opening database: " << e.what());
      return 1;
    }
    r = core_storage[n]->init(db, net_type);

    std::string source_dest = n == 0 ? "source" : "pruned";
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize " << source_dest << " blockchain storage");
    MINFO(source_dest << " blockchain storage initialized OK");
    if (n == 0 && core_storage[0]->get_blockchain_pruning_seed())
    {
      if (!opt_copy_pruned_database)
      {
        MERROR("Blockchain is already pruned, use --" << arg_copy_pruned_database.name << " to copy it anyway");
        return 1;
      }
      already_pruned = true;
    }
  }
  core_storage[0]->deinit();
  core_storage[0].reset(NULL);
  core_storage[1]->deinit();
  core_storage[1].reset(NULL);

  MINFO("Pruning...");
  MDB_env *env0 = NULL, *env1 = NULL;
  open(env0, paths[0], db_flags, true);
  open(env1, paths[1], db_flags, false);
  copy_table(env0, env1, "blocks", MDB_INTEGERKEY, MDB_APPEND);
  copy_table(env0, env1, "block_info", MDB_INTEGERKEY | MDB_DUPSORT| MDB_DUPFIXED, MDB_APPENDDUP, BlockchainLMDB::compare_uint64);
  copy_table(env0, env1, "block_heights", MDB_INTEGERKEY | MDB_DUPSORT| MDB_DUPFIXED, 0, BlockchainLMDB::compare_hash32);
  //copy_table(env0, env1, "txs", MDB_INTEGERKEY);
  copy_table(env0, env1, "txs_pruned", MDB_INTEGERKEY, MDB_APPEND);
  copy_table(env0, env1, "txs_prunable_hash", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, MDB_APPEND);
  // not copied: prunable, prunable_tip
  copy_table(env0, env1, "tx_indices", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, 0, BlockchainLMDB::compare_hash32);
  copy_table(env0, env1, "tx_outputs", MDB_INTEGERKEY, MDB_APPEND);
  copy_table(env0, env1, "output_txs", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, MDB_APPENDDUP, BlockchainLMDB::compare_uint64);
  copy_table(env0, env1, "output_amounts", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, MDB_APPENDDUP, BlockchainLMDB::compare_uint64);
  copy_table(env0, env1, "spent_keys", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, MDB_NODUPDATA, BlockchainLMDB::compare_hash32);
  copy_table(env0, env1, "txpool_meta", 0, MDB_NODUPDATA, BlockchainLMDB::compare_hash32);
  copy_table(env0, env1, "txpool_blob", 0, MDB_NODUPDATA, BlockchainLMDB::compare_hash32);
  copy_table(env0, env1, "hf_versions", MDB_INTEGERKEY, MDB_APPEND);
  copy_table(env0, env1, "properties", 0, 0, BlockchainLMDB::compare_string);
  if (already_pruned)
  {
    copy_table(env0, env1, "txs_prunable", MDB_INTEGERKEY, MDB_APPEND, BlockchainLMDB::compare_uint64);
    copy_table(env0, env1, "txs_prunable_tip", MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED, MDB_NODUPDATA, BlockchainLMDB::compare_uint64);
  }
  else
  {
    prune(env0, env1);
  }
  close(env1);
  close(env0);

  MINFO("Swapping databases, pre-pruning blockchain will be left in " << paths[0].string() + "-old and can be removed if desired");
  if (replace_file(paths[0].string(), paths[0].string() + "-old") || replace_file(paths[1].string(), paths[0].string()))
  {
    MERROR("Blockchain pruned OK, but renaming failed");
    return 1;
  }

  MINFO("Blockchain pruned OK");
  return 0;

  CATCH_ENTRY("Pruning error", 1);
}
