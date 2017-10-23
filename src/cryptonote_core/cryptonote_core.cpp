// Copyright (c) 2014-2017, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "include_base_utils.h"
using namespace epee;

#include <unordered_set>
#include "cryptonote_core.h"
#include "common/command_line.h"
#include "common/util.h"
#include "common/updates.h"
#include "common/download.h"
#include "common/task_region.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "cryptonote_tx_utils.h"
#include "misc_language.h"
#include <csignal>
#include <p2p/net_node.h>
#include "cryptonote_basic/checkpoints.h"
#include "ringct/rctTypes.h"
#include "blockchain_db/blockchain_db.h"
#include "ringct/rctSigs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

DISABLE_VS_WARNINGS(4355)

#define MERROR_VER(x) MCERROR("verify", x)

#define BAD_SEMANTICS_TXES_MAX_SIZE 100

namespace cryptonote
{

  //-----------------------------------------------------------------------------------------------
  core::core(i_cryptonote_protocol* pprotocol):
              m_mempool(m_blockchain_storage),
              m_blockchain_storage(m_mempool),
              m_miner(this),
              m_miner_address(boost::value_initialized<account_public_address>()),
              m_starter_message_showed(false),
              m_target_blockchain_height(0),
              m_checkpoints_path(""),
              m_last_dns_checkpoints_update(0),
              m_last_json_checkpoints_update(0),
              m_disable_dns_checkpoints(false),
              m_threadpool(tools::thread_group::optimal()),
              m_update_download(0)
  {
    m_checkpoints_updating.clear();
    set_cryptonote_protocol(pprotocol);
  }
  void core::set_cryptonote_protocol(i_cryptonote_protocol* pprotocol)
  {
    if(pprotocol)
      m_pprotocol = pprotocol;
    else
      m_pprotocol = &m_protocol_stub;
  }
  //-----------------------------------------------------------------------------------
  void core::set_checkpoints(checkpoints&& chk_pts)
  {
    m_blockchain_storage.set_checkpoints(std::move(chk_pts));
  }
  //-----------------------------------------------------------------------------------
  void core::set_checkpoints_file_path(const std::string& path)
  {
    m_checkpoints_path = path;
  }
  //-----------------------------------------------------------------------------------
  void core::set_enforce_dns_checkpoints(bool enforce_dns)
  {
    m_blockchain_storage.set_enforce_dns_checkpoints(enforce_dns);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::update_checkpoints()
  {
    if (m_testnet || m_fakechain || m_disable_dns_checkpoints) return true;

    if (m_checkpoints_updating.test_and_set()) return true;

    bool res = true;
    if (time(NULL) - m_last_dns_checkpoints_update >= 3600)
    {
      res = m_blockchain_storage.update_checkpoints(m_checkpoints_path, true);
      m_last_dns_checkpoints_update = time(NULL);
      m_last_json_checkpoints_update = time(NULL);
    }
    else if (time(NULL) - m_last_json_checkpoints_update >= 600)
    {
      res = m_blockchain_storage.update_checkpoints(m_checkpoints_path, false);
      m_last_json_checkpoints_update = time(NULL);
    }

    m_checkpoints_updating.clear();

    // if anything fishy happened getting new checkpoints, bring down the house
    if (!res)
    {
      graceful_exit();
    }
    return res;
  }
  //-----------------------------------------------------------------------------------
  void core::stop()
  {
    m_blockchain_storage.cancel();

    tools::download_async_handle handle;
    {
      boost::lock_guard<boost::mutex> lock(m_update_mutex);
      handle = m_update_download;
      m_update_download = 0;
    }
    if (handle)
      tools::download_cancel(handle);
  }
  //-----------------------------------------------------------------------------------
  void core::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, command_line::arg_data_dir, tools::get_default_data_dir());
    command_line::add_arg(desc, command_line::arg_testnet_data_dir, (boost::filesystem::path(tools::get_default_data_dir()) / "testnet").string());

    command_line::add_arg(desc, command_line::arg_test_drop_download);
    command_line::add_arg(desc, command_line::arg_test_drop_download_height);

    command_line::add_arg(desc, command_line::arg_testnet_on);
    command_line::add_arg(desc, command_line::arg_dns_checkpoints);
    command_line::add_arg(desc, command_line::arg_db_type);
    command_line::add_arg(desc, command_line::arg_prep_blocks_threads);
    command_line::add_arg(desc, command_line::arg_fast_block_sync);
    command_line::add_arg(desc, command_line::arg_db_sync_mode);
    command_line::add_arg(desc, command_line::arg_db_salvage);
    command_line::add_arg(desc, command_line::arg_show_time_stats);
    command_line::add_arg(desc, command_line::arg_block_sync_size);
    command_line::add_arg(desc, command_line::arg_check_updates);
    command_line::add_arg(desc, command_line::arg_fluffy_blocks);

    // we now also need some of net_node's options (p2p bind arg, for separate data dir)
    command_line::add_arg(desc, nodetool::arg_testnet_p2p_bind_port, false);
    command_line::add_arg(desc, nodetool::arg_p2p_bind_port, false);

    miner::init_options(desc);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_testnet = command_line::get_arg(vm, command_line::arg_testnet_on);

    auto data_dir_arg = m_testnet ? command_line::arg_testnet_data_dir : command_line::arg_data_dir;
    m_config_folder = command_line::get_arg(vm, data_dir_arg);

    auto data_dir = boost::filesystem::path(m_config_folder);

    if (!m_testnet && !m_fakechain)
    {
      cryptonote::checkpoints checkpoints;
      if (!checkpoints.init_default_checkpoints())
      {
        throw std::runtime_error("Failed to initialize checkpoints");
      }
      set_checkpoints(std::move(checkpoints));

      boost::filesystem::path json(JSON_HASH_FILE_NAME);
      boost::filesystem::path checkpoint_json_hashfile_fullpath = data_dir / json;

      set_checkpoints_file_path(checkpoint_json_hashfile_fullpath.string());
    }


    set_enforce_dns_checkpoints(command_line::get_arg(vm, command_line::arg_dns_checkpoints));
    test_drop_download_height(command_line::get_arg(vm, command_line::arg_test_drop_download_height));
    m_fluffy_blocks_enabled = m_testnet || get_arg(vm, command_line::arg_fluffy_blocks);

    if (command_line::get_arg(vm, command_line::arg_test_drop_download) == true)
      test_drop_download();

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_current_blockchain_height() const
  {
    return m_blockchain_storage.get_current_blockchain_height();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blockchain_top(uint64_t& height, crypto::hash& top_id) const
  {
    top_id = m_blockchain_storage.get_tail_id(height);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<std::pair<cryptonote::blobdata,block>>& blocks, std::list<cryptonote::blobdata>& txs) const
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks, txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<std::pair<cryptonote::blobdata,block>>& blocks) const
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks) const
  {
    std::list<std::pair<cryptonote::blobdata, cryptonote::block>> bs;
    if (!m_blockchain_storage.get_blocks(start_offset, count, bs))
      return false;
    for (const auto &b: bs)
      blocks.push_back(b.second);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<cryptonote::blobdata>& txs, std::list<crypto::hash>& missed_txs) const
  {
    return m_blockchain_storage.get_transactions_blobs(txs_ids, txs, missed_txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_txpool_backlog(std::vector<tx_backlog_entry>& backlog) const
  {
    m_mempool.get_transaction_backlog(backlog);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs) const
  {
    return m_blockchain_storage.get_transactions(txs_ids, txs, missed_txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_alternative_blocks(std::list<block>& blocks) const
  {
    return m_blockchain_storage.get_alternative_blocks(blocks);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_alternative_blocks_count() const
  {
    return m_blockchain_storage.get_alternative_blocks_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::init(const boost::program_options::variables_map& vm, const cryptonote::test_options *test_options)
  {
    start_time = std::time(nullptr);

    m_fakechain = test_options != NULL;
    bool r = handle_command_line(vm);
    bool testnet = command_line::get_arg(vm, command_line::arg_testnet_on);
    auto p2p_bind_arg = testnet ? nodetool::arg_testnet_p2p_bind_port : nodetool::arg_p2p_bind_port;
    std::string m_port = command_line::get_arg(vm, p2p_bind_arg);
    std::string m_config_folder_mempool = m_config_folder;

    if ((!testnet && m_port != std::to_string(::config::P2P_DEFAULT_PORT))
        || (testnet && m_port != std::to_string(::config::testnet::P2P_DEFAULT_PORT))) {
      m_config_folder_mempool = m_config_folder_mempool + "/" + m_port;
    }

    std::string db_type = command_line::get_arg(vm, command_line::arg_db_type);
    std::string db_sync_mode = command_line::get_arg(vm, command_line::arg_db_sync_mode);
    bool db_salvage = command_line::get_arg(vm, command_line::arg_db_salvage) != 0;
    bool fast_sync = command_line::get_arg(vm, command_line::arg_fast_block_sync) != 0;
    uint64_t blocks_threads = command_line::get_arg(vm, command_line::arg_prep_blocks_threads);
    std::string check_updates_string = command_line::get_arg(vm, command_line::arg_check_updates);

    boost::filesystem::path folder(m_config_folder);
    if (m_fakechain)
      folder /= "fake";

    // make sure the data directory exists, and try to lock it
    CHECK_AND_ASSERT_MES (boost::filesystem::exists(folder) || boost::filesystem::create_directories(folder), false,
      std::string("Failed to create directory ").append(folder.string()).c_str());

    // check for blockchain.bin
    try
    {
      const boost::filesystem::path old_files = folder;
      if (boost::filesystem::exists(old_files / "blockchain.bin"))
      {
        MWARNING("Found old-style blockchain.bin in " << old_files.string());
        MWARNING("Monero now uses a new format. You can either remove blockchain.bin to start syncing");
        MWARNING("the blockchain anew, or use monero-blockchain-export and monero-blockchain-import to");
        MWARNING("convert your existing blockchain.bin to the new format. See README.md for instructions.");
        return false;
      }
    }
    // folder might not be a directory, etc, etc
    catch (...) { }

    BlockchainDB* db = new_db(db_type);
    if (db == NULL)
    {
      LOG_ERROR("Attempted to use non-existent database type");
      return false;
    }

    folder /= db->get_db_name();
    MGINFO("Loading blockchain from folder " << folder.string() << " ...");

    const std::string filename = folder.string();
    // default to fast:async:1
    blockchain_db_sync_mode sync_mode = db_defaultsync;
    uint64_t blocks_per_sync = 1;

    try
    {
      uint64_t db_flags = 0;

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
          sync_mode = db_nosync;
        }
        else if(options[0] == "fast")
        {
          db_flags = DBF_FAST;
          sync_mode = db_async;
        }
        else if(options[0] == "fastest")
        {
          db_flags = DBF_FASTEST;
          blocks_per_sync = 1000; // default to fastest:async:1000
          sync_mode = db_async;
        }
        else
          db_flags = DEFAULT_FLAGS;
      }

      if(options.size() >= 2 && !safemode)
      {
        if(options[1] == "sync")
          sync_mode = db_sync;
        else if(options[1] == "async")
          sync_mode = db_async;
      }

      if(options.size() >= 3 && !safemode)
      {
        char *endptr;
        uint64_t bps = strtoull(options[2].c_str(), &endptr, 0);
        if (*endptr == '\0')
          blocks_per_sync = bps;
      }

      if (db_salvage)
        db_flags |= DBF_SALVAGE;

      db->open(filename, db_flags);
      if(!db->m_open)
        return false;
    }
    catch (const DB_ERROR& e)
    {
      LOG_ERROR("Error opening database: " << e.what());
      return false;
    }

    m_blockchain_storage.set_user_options(blocks_threads,
        blocks_per_sync, sync_mode, fast_sync);

    r = m_blockchain_storage.init(db, m_testnet, test_options);

    r = m_mempool.init();
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize memory pool");

    // now that we have a valid m_blockchain_storage, we can clean out any
    // transactions in the pool that do not conform to the current fork
    m_mempool.validate(m_blockchain_storage.get_current_hard_fork_version());

    bool show_time_stats = command_line::get_arg(vm, command_line::arg_show_time_stats) != 0;
    m_blockchain_storage.set_show_time_stats(show_time_stats);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    block_sync_size = command_line::get_arg(vm, command_line::arg_block_sync_size);

    MGINFO("Loading checkpoints");

    // load json & DNS checkpoints, and verify them
    // with respect to what blocks we already have
    CHECK_AND_ASSERT_MES(update_checkpoints(), false, "One or more checkpoints loaded from json or dns conflicted with existing checkpoints.");

   // DNS versions checking
    if (check_updates_string == "disabled")
      check_updates_level = UPDATES_DISABLED;
    else if (check_updates_string == "notify")
      check_updates_level = UPDATES_NOTIFY;
    else if (check_updates_string == "download")
      check_updates_level = UPDATES_DOWNLOAD;
    else if (check_updates_string == "update")
      check_updates_level = UPDATES_UPDATE;
    else {
      MERROR("Invalid argument to --dns-versions-check: " << check_updates_string);
      return false;
    }

    r = m_miner.init(vm, m_testnet);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize miner instance");

    return load_state_data();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::set_genesis_block(const block& b)
  {
    return m_blockchain_storage.reset_and_set_genesis_block(b);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::load_state_data()
  {
    // may be some code later
    return true;
  }
  //-----------------------------------------------------------------------------------------------
    bool core::deinit()
  {
    m_miner.stop();
    m_mempool.deinit();
    m_blockchain_storage.deinit();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::test_drop_download()
  {
    m_test_drop_download = false;
  }
  //-----------------------------------------------------------------------------------------------
  void core::test_drop_download_height(uint64_t height)
  {
    m_test_drop_download_height = height;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_test_drop_download() const
  {
    return m_test_drop_download;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_test_drop_download_height() const
  {
    if (m_test_drop_download_height == 0)
      return true;

    if (get_blockchain_storage().get_current_blockchain_height() <= m_test_drop_download_height)
      return true;

    return false;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx_pre(const blobdata& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash, crypto::hash &tx_prefixt_hash, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    tvc = boost::value_initialized<tx_verification_context>();

    if(tx_blob.size() > get_max_tx_size())
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, too big size " << tx_blob.size() << ", rejected");
      tvc.m_verifivation_failed = true;
      tvc.m_too_big = true;
      return false;
    }

    tx_hash = null_hash;
    tx_prefixt_hash = null_hash;

    if(!parse_tx_from_blob(tx, tx_hash, tx_prefixt_hash, tx_blob))
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, Failed to parse, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }
    //std::cout << "!"<< tx.vin.size() << std::endl;

    bad_semantics_txes_lock.lock();
    for (int idx = 0; idx < 2; ++idx)
    {
      if (bad_semantics_txes[idx].find(tx_hash) != bad_semantics_txes[idx].end())
      {
        bad_semantics_txes_lock.unlock();
        LOG_PRINT_L1("Transaction already seen with bad semantics, rejected");
        tvc.m_verifivation_failed = true;
        return false;
      }
    }
    bad_semantics_txes_lock.unlock();

    uint8_t version = m_blockchain_storage.get_current_hard_fork_version();
    const size_t max_tx_version = version == 1 ? 1 : 2;
    if (tx.version == 0 || tx.version > max_tx_version)
    {
      // v2 is the latest one we know
      tvc.m_verifivation_failed = true;
      return false;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx_post(const blobdata& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash, crypto::hash &tx_prefixt_hash, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    if(!check_tx_syntax(tx))
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " syntax, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }

    // resolve outPk references in rct txes
    // outPk aren't the only thing that need resolving for a fully resolved tx,
    // but outPk (1) are needed now to check range proof semantics, and
    // (2) do not need access to the blockchain to find data
    if (tx.version >= 2)
    {
      rct::rctSig &rv = tx.rct_signatures;
      if (rv.outPk.size() != tx.vout.size())
      {
        LOG_PRINT_L1("WRONG TRANSACTION BLOB, Bad outPk size in tx " << tx_hash << ", rejected");
        tvc.m_verifivation_failed = true;
        return false;
      }
      for (size_t n = 0; n < tx.rct_signatures.outPk.size(); ++n)
        rv.outPk[n].dest = rct::pk2rct(boost::get<txout_to_key>(tx.vout[n].target).key);
    }

    if (keeped_by_block && get_blockchain_storage().is_within_compiled_block_hash_area())
    {
      MTRACE("Skipping semantics check for tx kept by block in embedded hash area");
    }
    else if(!check_tx_semantic(tx, keeped_by_block))
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " semantic, rejected");
      tvc.m_verifivation_failed = true;
      bad_semantics_txes_lock.lock();
      bad_semantics_txes[0].insert(tx_hash);
      if (bad_semantics_txes[0].size() >= BAD_SEMANTICS_TXES_MAX_SIZE)
      {
        std::swap(bad_semantics_txes[0], bad_semantics_txes[1]);
        bad_semantics_txes[0].clear();
      }
      bad_semantics_txes_lock.unlock();
      return false;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_txs(const std::list<blobdata>& tx_blobs, std::vector<tx_verification_context>& tvc, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    TRY_ENTRY();

    struct result { bool res; cryptonote::transaction tx; crypto::hash hash; crypto::hash prefix_hash; bool in_txpool; bool in_blockchain; };
    std::vector<result> results(tx_blobs.size());

    tvc.resize(tx_blobs.size());
    tools::task_region(m_threadpool, [&] (tools::task_region_handle& region) {
      std::list<blobdata>::const_iterator it = tx_blobs.begin();
      for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
        region.run([&, i, it] {
          try
          {
            results[i].res = handle_incoming_tx_pre(*it, tvc[i], results[i].tx, results[i].hash, results[i].prefix_hash, keeped_by_block, relayed, do_not_relay);
          }
          catch (const std::exception &e)
          {
            MERROR_VER("Exception in handle_incoming_tx_pre: " << e.what());
            results[i].res = false;
          }
        });
      }
    });
    tools::task_region(m_threadpool, [&] (tools::task_region_handle& region) {
      std::list<blobdata>::const_iterator it = tx_blobs.begin();
      for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
        if (!results[i].res)
          continue;
        if(m_mempool.have_tx(results[i].hash))
        {
          LOG_PRINT_L2("tx " << results[i].hash << "already have transaction in tx_pool");
        }
        else if(m_blockchain_storage.have_tx(results[i].hash))
        {
          LOG_PRINT_L2("tx " << results[i].hash << " already have transaction in blockchain");
        }
        else
        {
          region.run([&, i, it] {
            try
            {
              results[i].res = handle_incoming_tx_post(*it, tvc[i], results[i].tx, results[i].hash, results[i].prefix_hash, keeped_by_block, relayed, do_not_relay);
            }
            catch (const std::exception &e)
            {
              MERROR_VER("Exception in handle_incoming_tx_post: " << e.what());
              results[i].res = false;
            }
          });
        }
      }
    });

    bool ok = true;
    std::list<blobdata>::const_iterator it = tx_blobs.begin();
    for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
      if (!results[i].res)
      {
        ok = false;
        continue;
      }

      ok &= add_new_tx(results[i].tx, results[i].hash, results[i].prefix_hash, it->size(), tvc[i], keeped_by_block, relayed, do_not_relay);
      if(tvc[i].m_verifivation_failed)
      {MERROR_VER("Transaction verification failed: " << results[i].hash);}
      else if(tvc[i].m_verifivation_impossible)
      {MERROR_VER("Transaction verification impossible: " << results[i].hash);}

      if(tvc[i].m_added_to_pool)
        MDEBUG("tx added: " << results[i].hash);
    }
    return ok;

    CATCH_ENTRY_L0("core::handle_incoming_txs()", false);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    std::list<cryptonote::blobdata> tx_blobs;
    tx_blobs.push_back(tx_blob);
    std::vector<tx_verification_context> tvcv(1);
    bool r = handle_incoming_txs(tx_blobs, tvcv, keeped_by_block, relayed, do_not_relay);
    tvc = tvcv[0];
    return r;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_stat_info(core_stat_info& st_inf) const
  {
    st_inf.mining_speed = m_miner.get_speed();
    st_inf.alternative_blocks = m_blockchain_storage.get_alternative_blocks_count();
    st_inf.blockchain_height = m_blockchain_storage.get_current_blockchain_height();
    st_inf.tx_pool_size = m_mempool.get_transactions_count();
    st_inf.top_block_id_str = epee::string_tools::pod_to_hex(m_blockchain_storage.get_tail_id());
    return true;
  }

  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_semantic(const transaction& tx, bool keeped_by_block) const
  {
    if(!tx.vin.size())
    {
      MERROR_VER("tx with empty inputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!check_inputs_types_supported(tx))
    {
      MERROR_VER("unsupported input types for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!check_outs_valid(tx))
    {
      MERROR_VER("tx with invalid outputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }
    if (tx.version > 1)
    {
      if (tx.rct_signatures.outPk.size() != tx.vout.size())
      {
        MERROR_VER("tx with mismatched vout/outPk count, rejected for tx id= " << get_transaction_hash(tx));
        return false;
      }
    }

    if(!check_money_overflow(tx))
    {
      MERROR_VER("tx has money overflow, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (tx.version == 1)
    {
      uint64_t amount_in = 0;
      get_inputs_money_amount(tx, amount_in);
      uint64_t amount_out = get_outs_money_amount(tx);

      if(amount_in <= amount_out)
      {
        MERROR_VER("tx with wrong amounts: ins " << amount_in << ", outs " << amount_out << ", rejected for tx id= " << get_transaction_hash(tx));
        return false;
      }
    }
    // for version > 1, ringct signatures check verifies amounts match

    if(!keeped_by_block && get_object_blobsize(tx) >= m_blockchain_storage.get_current_cumulative_blocksize_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE)
    {
      MERROR_VER("tx is too large " << get_object_blobsize(tx) << ", expected not bigger than " << m_blockchain_storage.get_current_cumulative_blocksize_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
      return false;
    }

    //check if tx use different key images
    if(!check_tx_inputs_keyimages_diff(tx))
    {
      MERROR_VER("tx uses a single key image more than once");
      return false;
    }

    if (!check_tx_inputs_ring_members_diff(tx))
    {
      MERROR_VER("tx uses duplicate ring members");
      return false;
    }

    if (!check_tx_inputs_keyimages_domain(tx))
    {
      MERROR_VER("tx uses key image not in the valid domain");
      return false;
    }

    if (tx.version >= 2)
    {
      const rct::rctSig &rv = tx.rct_signatures;
      switch (rv.type) {
        case rct::RCTTypeNull:
          // coinbase should not come here, so we reject for all other types
          MERROR_VER("Unexpected Null rctSig type");
          return false;
        case rct::RCTTypeSimple:
          if (!rct::verRctSimple(rv, true))
          {
            MERROR_VER("rct signature semantics check failed");
            return false;
          }
          break;
        case rct::RCTTypeFull:
          if (!rct::verRct(rv, true))
          {
            MERROR_VER("rct signature semantics check failed");
            return false;
          }
          break;
        default:
          MERROR_VER("Unknown rct type: " << rv.type);
          return false;
      }
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::is_key_image_spent(const crypto::key_image &key_image) const
  {
    return m_blockchain_storage.have_tx_keyimg_as_spent(key_image);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::are_key_images_spent(const std::vector<crypto::key_image>& key_im, std::vector<bool> &spent) const
  {
    spent.clear();
    for(auto& ki: key_im)
    {
      spent.push_back(m_blockchain_storage.have_tx_keyimg_as_spent(ki));
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_block_sync_size(uint64_t height) const
  {
    static const uint64_t quick_height = m_testnet ? 801219 : 1220516;
    if (block_sync_size > 0)
      return block_sync_size;
    if (height >= quick_height)
      return BLOCKS_SYNCHRONIZING_DEFAULT_COUNT;
    return BLOCKS_SYNCHRONIZING_DEFAULT_COUNT_PRE_V4;
  }
  //-----------------------------------------------------------------------------------------------
  std::pair<uint64_t, uint64_t> core::get_coinbase_tx_sum(const uint64_t start_offset, const size_t count)
  {
    uint64_t emission_amount = 0;
    uint64_t total_fee_amount = 0;
    if (count)
    {
      const uint64_t end = start_offset + count - 1;
      m_blockchain_storage.for_blocks_range(start_offset, end,
        [this, &emission_amount, &total_fee_amount](uint64_t, const crypto::hash& hash, const block& b){
      std::list<transaction> txs;
      std::list<crypto::hash> missed_txs;
      uint64_t coinbase_amount = get_outs_money_amount(b.miner_tx);
      this->get_transactions(b.tx_hashes, txs, missed_txs);      
      uint64_t tx_fee_amount = 0;
      for(const auto& tx: txs)
      {
        tx_fee_amount += get_tx_fee(tx);
      }
      
      emission_amount += coinbase_amount - tx_fee_amount;
      total_fee_amount += tx_fee_amount;
      return true;
      });
    }

    return std::pair<uint64_t, uint64_t>(emission_amount, total_fee_amount);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_inputs_keyimages_diff(const transaction& tx) const
  {
    std::unordered_set<crypto::key_image> ki;
    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      if(!ki.insert(tokey_in.k_image).second)
        return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_inputs_ring_members_diff(const transaction& tx) const
  {
    const uint8_t version = m_blockchain_storage.get_current_hard_fork_version();
    if (version >= 6)
    {
      for(const auto& in: tx.vin)
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
        for (size_t n = 1; n < tokey_in.key_offsets.size(); ++n)
          if (tokey_in.key_offsets[n] == 0)
            return false;
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_inputs_keyimages_domain(const transaction& tx) const
  {
    std::unordered_set<crypto::key_image> ki;
    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      if (!(rct::scalarmultKey(rct::ki2rct(tokey_in.k_image), rct::curveOrder()) == rct::identity()))
        return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(transaction& tx, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    crypto::hash tx_hash = get_transaction_hash(tx);
    crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);
    blobdata bl;
    t_serializable_object_to_blob(tx, bl);
    return add_new_tx(tx, tx_hash, tx_prefix_hash, bl.size(), tvc, keeped_by_block, relayed, do_not_relay);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_blockchain_total_transactions() const
  {
    return m_blockchain_storage.get_total_transactions();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay)
  {
    if (keeped_by_block)
      get_blockchain_storage().on_new_tx_from_block(tx);

    if(m_mempool.have_tx(tx_hash))
    {
      LOG_PRINT_L2("tx " << tx_hash << "already have transaction in tx_pool");
      return true;
    }

    if(m_blockchain_storage.have_tx(tx_hash))
    {
      LOG_PRINT_L2("tx " << tx_hash << " already have transaction in blockchain");
      return true;
    }

    uint8_t version = m_blockchain_storage.get_current_hard_fork_version();
    return m_mempool.add_tx(tx, tx_hash, blob_size, tvc, keeped_by_block, relayed, do_not_relay, version);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::relay_txpool_transactions()
  {
    // we attempt to relay txes that should be relayed, but were not
    std::list<std::pair<crypto::hash, cryptonote::blobdata>> txs;
    if (m_mempool.get_relayable_transactions(txs) && !txs.empty())
    {
      cryptonote_connection_context fake_context = AUTO_VAL_INIT(fake_context);
      tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      NOTIFY_NEW_TRANSACTIONS::request r;
      for (auto it = txs.begin(); it != txs.end(); ++it)
      {
        r.txs.push_back(it->second);
      }
      get_protocol()->relay_transactions(r, fake_context);
      m_mempool.set_relayed(txs);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::on_transaction_relayed(const cryptonote::blobdata& tx_blob)
  {
    std::list<std::pair<crypto::hash, cryptonote::blobdata>> txs;
    cryptonote::transaction tx;
    crypto::hash tx_hash, tx_prefix_hash;
    if (!parse_and_validate_tx_from_blob(tx_blob, tx, tx_hash, tx_prefix_hash))
    {
      LOG_ERROR("Failed to parse relayed transaction");
      return;
    }
    txs.push_back(std::make_pair(tx_hash, std::move(tx_blob)));
    m_mempool.set_relayed(txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce)
  {
    return m_blockchain_storage.create_block_template(b, adr, diffic, height, expected_reward, ex_nonce);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, resp);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::list<std::pair<cryptonote::blobdata, std::list<cryptonote::blobdata> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count) const
  {
    return m_blockchain_storage.find_blockchain_supplement(req_start_block, qblock_ids, blocks, total_height, start_height, max_count);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain(uint64_t start_index, uint64_t end_index) const
  {
    m_blockchain_storage.print_blockchain(start_index, end_index);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_index() const
  {
    m_blockchain_storage.print_blockchain_index();
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_outs(const std::string& file)
  {
    m_blockchain_storage.print_blockchain_outs(file);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) const
  {
    return m_blockchain_storage.get_random_outs_for_amounts(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_outs(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const
  {
    return m_blockchain_storage.get_outs(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_random_rct_outs(const COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::request& req, COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::response& res) const
  {
    return m_blockchain_storage.get_random_rct_outs(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const
  {
    return m_blockchain_storage.get_tx_outputs_gindexs(tx_id, indexs);
  }
  //-----------------------------------------------------------------------------------------------
  void core::pause_mine()
  {
    m_miner.pause();
  }
  //-----------------------------------------------------------------------------------------------
  void core::resume_mine()
  {
    m_miner.resume();
  }
  //-----------------------------------------------------------------------------------------------
  block_complete_entry get_block_complete_entry(block& b, tx_memory_pool &pool)
  {
    block_complete_entry bce;
    bce.block = cryptonote::block_to_blob(b);
    for (const auto &tx_hash: b.tx_hashes)
    {
      cryptonote::blobdata txblob;
      CHECK_AND_ASSERT_THROW_MES(pool.get_transaction(tx_hash, txblob), "Transaction not found in pool");
      bce.txs.push_back(txblob);
    }
    return bce;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_block_found(block& b)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    m_miner.pause();
    std::list<block_complete_entry> blocks;
    try
    {
      blocks.push_back(get_block_complete_entry(b, m_mempool));
    }
    catch (const std::exception &e)
    {
      m_miner.resume();
      return false;
    }
    prepare_handle_incoming_blocks(blocks);
    m_blockchain_storage.add_new_block(b, bvc);
    cleanup_handle_incoming_blocks(true);
    //anyway - update miner template
    update_miner_block_template();
    m_miner.resume();


    CHECK_AND_ASSERT_MES(!bvc.m_verifivation_failed, false, "mined block failed verification");
    if(bvc.m_added_to_main_chain)
    {
      cryptonote_connection_context exclude_context = boost::value_initialized<cryptonote_connection_context>();
      NOTIFY_NEW_BLOCK::request arg = AUTO_VAL_INIT(arg);
      arg.hop = 0;
      arg.current_blockchain_height = m_blockchain_storage.get_current_blockchain_height();
      std::list<crypto::hash> missed_txs;
      std::list<cryptonote::blobdata> txs;
      m_blockchain_storage.get_transactions_blobs(b.tx_hashes, txs, missed_txs);
      if(missed_txs.size() &&  m_blockchain_storage.get_block_id_by_height(get_block_height(b)) != get_block_hash(b))
      {
        LOG_PRINT_L1("Block found but, seems that reorganize just happened after that, do not relay this block");
        return true;
      }
      CHECK_AND_ASSERT_MES(txs.size() == b.tx_hashes.size() && !missed_txs.size(), false, "cant find some transactions in found block:" << get_block_hash(b) << " txs.size()=" << txs.size()
        << ", b.tx_hashes.size()=" << b.tx_hashes.size() << ", missed_txs.size()" << missed_txs.size());

      block_to_blob(b, arg.b.block);
      //pack transactions
      for(auto& tx:  txs)
        arg.b.txs.push_back(tx);

      m_pprotocol->relay_block(arg, exclude_context);
    }
    return bvc.m_added_to_main_chain;
  }
  //-----------------------------------------------------------------------------------------------
  void core::on_synchronized()
  {
    m_miner.on_synchronized();
  }
  //-----------------------------------------------------------------------------------------------
  void core::safesyncmode(const bool onoff)
  {
    m_blockchain_storage.safesyncmode(onoff);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_block(const block& b, block_verification_context& bvc)
  {
    return m_blockchain_storage.add_new_block(b, bvc);
  }

  //-----------------------------------------------------------------------------------------------
  bool core::prepare_handle_incoming_blocks(const std::list<block_complete_entry> &blocks)
  {
    m_incoming_tx_lock.lock();
    m_blockchain_storage.prepare_handle_incoming_blocks(blocks);
    return true;
  }

  //-----------------------------------------------------------------------------------------------
  bool core::cleanup_handle_incoming_blocks(bool force_sync)
  {
    bool success = false;
    try {
      success = m_blockchain_storage.cleanup_handle_incoming_blocks(force_sync);
    }
    catch (...) {}
    m_incoming_tx_lock.unlock();
    return success;
  }

  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate)
  {
    TRY_ENTRY();

    // load json & DNS checkpoints every 10min/hour respectively,
    // and verify them with respect to what blocks we already have
    CHECK_AND_ASSERT_MES(update_checkpoints(), false, "One or more checkpoints loaded from json or dns conflicted with existing checkpoints.");

    bvc = boost::value_initialized<block_verification_context>();
    if(block_blob.size() > get_max_block_size())
    {
      LOG_PRINT_L1("WRONG BLOCK BLOB, too big size " << block_blob.size() << ", rejected");
      bvc.m_verifivation_failed = true;
      return false;
    }

    block b = AUTO_VAL_INIT(b);
    if(!parse_and_validate_block_from_blob(block_blob, b))
    {
      LOG_PRINT_L1("Failed to parse and validate new block");
      bvc.m_verifivation_failed = true;
      return false;
    }
    add_new_block(b, bvc);
    if(update_miner_blocktemplate && bvc.m_added_to_main_chain)
       update_miner_block_template();
    return true;

    CATCH_ENTRY_L0("core::handle_incoming_block()", false);
  }
  //-----------------------------------------------------------------------------------------------
  // Used by the RPC server to check the size of an incoming
  // block_blob
  bool core::check_incoming_block_size(const blobdata& block_blob) const
  {
    if(block_blob.size() > get_max_block_size())
    {
      LOG_PRINT_L1("WRONG BLOCK BLOB, too big size " << block_blob.size() << ", rejected");
      return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_tail_id() const
  {
    return m_blockchain_storage.get_tail_id();
  }
  //-----------------------------------------------------------------------------------------------
  difficulty_type core::get_block_cumulative_difficulty(uint64_t height) const
  {
    return m_blockchain_storage.get_db().get_block_cumulative_difficulty(height);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_pool_transactions_count() const
  {
    return m_mempool.get_transactions_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::have_block(const crypto::hash& id) const
  {
    return m_blockchain_storage.have_block(id);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob) const
  {
    return parse_and_validate_tx_from_blob(blob, tx, tx_hash, tx_prefix_hash);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_syntax(const transaction& tx) const
  {
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions(std::list<transaction>& txs) const
  {
    m_mempool.get_transactions(txs);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction_hashes(std::vector<crypto::hash>& txs) const
  {
    m_mempool.get_transaction_hashes(txs);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction_stats(struct txpool_stats& stats) const
  {
    m_mempool.get_transaction_stats(stats);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction(const crypto::hash &id, cryptonote::blobdata& tx) const
  {
    return m_mempool.get_transaction(id, tx);
  }  
  //-----------------------------------------------------------------------------------------------
  bool core::pool_has_tx(const crypto::hash &id) const
  {
    return m_mempool.have_tx(id);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos) const
  {
    return m_mempool.get_transactions_and_spent_keys_info(tx_infos, key_image_infos);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_short_chain_history(std::list<crypto::hash>& ids) const
  {
    return m_blockchain_storage.get_short_chain_history(ids);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote_connection_context& context)
  {
    return m_blockchain_storage.handle_get_objects(arg, rsp);
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_block_id_by_height(uint64_t height) const
  {
    return m_blockchain_storage.get_block_id_by_height(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_by_hash(const crypto::hash &h, block &blk, bool *orphan) const
  {
    return m_blockchain_storage.get_block_by_hash(h, blk, orphan);
  }
  //-----------------------------------------------------------------------------------------------
  std::string core::print_pool(bool short_format) const
  {
    return m_mempool.print_pool(short_format);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::update_miner_block_template()
  {
    m_miner.on_block_chain_update();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::on_idle()
  {
    if(!m_starter_message_showed)
    {
      MGINFO_YELLOW(ENDL << "**********************************************************************" << ENDL
        << "The daemon will start synchronizing with the network. This may take a long time to complete." << ENDL
        << ENDL
        << "You can set the level of process detailization* through \"set_log <level|categories>\" command*," << ENDL
        << "where <level> is between 0 (no details) and 4 (very verbose), or custom category based levels (eg, *:WARNING)" << ENDL
        << ENDL
        << "Use the \"help\" command to see the list of available commands." << ENDL
        << "**********************************************************************" << ENDL);
      m_starter_message_showed = true;
    }

    m_fork_moaner.do_call(boost::bind(&core::check_fork_time, this));
    m_txpool_auto_relayer.do_call(boost::bind(&core::relay_txpool_transactions, this));
    m_check_updates_interval.do_call(boost::bind(&core::check_updates, this));
    m_miner.on_idle();
    m_mempool.on_idle();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_fork_time()
  {
    HardFork::State state = m_blockchain_storage.get_hard_fork_state();
    const el::Level level = el::Level::Warning;
    switch (state) {
      case HardFork::LikelyForked:
        MCLOG_RED(level, "global", "**********************************************************************");
        MCLOG_RED(level, "global", "Last scheduled hard fork is too far in the past.");
        MCLOG_RED(level, "global", "We are most likely forked from the network. Daemon update needed now.");
        MCLOG_RED(level, "global", "**********************************************************************");
        break;
      case HardFork::UpdateNeeded:
        MCLOG_RED(level, "global", "**********************************************************************");
        MCLOG_RED(level, "global", "Last scheduled hard fork time shows a daemon update is needed now.");
        MCLOG_RED(level, "global", "**********************************************************************");
        break;
      default:
        break;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint8_t core::get_ideal_hard_fork_version(uint64_t height) const
  {
    return get_blockchain_storage().get_ideal_hard_fork_version(height);
  }
  //-----------------------------------------------------------------------------------------------
  uint8_t core::get_hard_fork_version(uint64_t height) const
  {
    return get_blockchain_storage().get_hard_fork_version(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_updates()
  {
    static const char software[] = "monero";
#ifdef BUILD_TAG
    static const char buildtag[] = BOOST_PP_STRINGIZE(BUILD_TAG);
    static const char subdir[] = "cli"; // because it can never be simple
#else
    static const char buildtag[] = "source";
    static const char subdir[] = "source"; // because it can never be simple
#endif

    if (check_updates_level == UPDATES_DISABLED)
      return true;

    std::string version, hash;
    MCDEBUG("updates", "Checking for a new " << software << " version for " << buildtag);
    if (!tools::check_updates(software, buildtag, version, hash))
      return false;

    if (tools::vercmp(version.c_str(), MONERO_VERSION) <= 0)
      return true;

    std::string url = tools::get_update_url(software, subdir, buildtag, version, true);
    MCLOG_CYAN(el::Level::Info, "global", "Version " << version << " of " << software << " for " << buildtag << " is available: " << url << ", SHA256 hash " << hash);

    if (check_updates_level == UPDATES_NOTIFY)
      return true;

    url = tools::get_update_url(software, subdir, buildtag, version, false);
    std::string filename;
    const char *slash = strrchr(url.c_str(), '/');
    if (slash)
      filename = slash + 1;
    else
      filename = std::string(software) + "-update-" + version;
    boost::filesystem::path path(epee::string_tools::get_current_module_folder());
    path /= filename;

    boost::unique_lock<boost::mutex> lock(m_update_mutex);

    if (m_update_download != 0)
    {
      MCDEBUG("updates", "Already downloading update");
      return true;
    }

    crypto::hash file_hash;
    if (!tools::sha256sum(path.string(), file_hash) || (hash != epee::string_tools::pod_to_hex(file_hash)))
    {
      MCDEBUG("updates", "We don't have that file already, downloading");
      m_last_update_length = 0;
      m_update_download = tools::download_async(path.string(), url, [this, hash](const std::string &path, const std::string &uri, bool success) {
        if (success)
        {
          crypto::hash file_hash;
          if (!tools::sha256sum(path, file_hash))
          {
            MCERROR("updates", "Failed to hash " << path);
          }
          if (hash != epee::string_tools::pod_to_hex(file_hash))
          {
            MCERROR("updates", "Download from " << uri << " does not match the expected hash");
          }
          MCLOG_CYAN(el::Level::Info, "updates", "New version downloaded to " << path);
        }
        else
        {
          MCERROR("updates", "Failed to download " << uri);
        }
        boost::unique_lock<boost::mutex> lock(m_update_mutex);
        m_update_download = 0;
      }, [this](const std::string &path, const std::string &uri, size_t length, ssize_t content_length) {
        if (length >= m_last_update_length + 1024 * 1024 * 10)
        {
          m_last_update_length = length;
          MCDEBUG("updates", "Downloaded " << length << "/" << (content_length ? std::to_string(content_length) : "unknown"));
        }
        return true;
      });
    }
    else
    {
      MCDEBUG("updates", "We already have " << path << " with expected hash");
    }

    lock.unlock();

    if (check_updates_level == UPDATES_DOWNLOAD)
      return true;

    MCERROR("updates", "Download/update not implemented yet");
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::set_target_blockchain_height(uint64_t target_blockchain_height)
  {
    m_target_blockchain_height = target_blockchain_height;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_target_blockchain_height() const
  {
    return m_target_blockchain_height;
  }
  //-----------------------------------------------------------------------------------------------
  std::time_t core::get_start_time() const
  {
    return start_time;
  }
  //-----------------------------------------------------------------------------------------------
  void core::graceful_exit()
  {
    raise(SIGTERM);
  }
}
