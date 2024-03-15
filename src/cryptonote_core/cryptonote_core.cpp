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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/algorithm/string.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "string_tools.h"
using namespace epee;

#include <unordered_set>
#include "cryptonote_core.h"
#include "common/util.h"
#include "common/updates.h"
#include "common/download.h"
#include "common/threadpool.h"
#include "common/command_line.h"
#include "cryptonote_basic/events.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "misc_language.h"
#include "file_io_utils.h"
#include <csignal>
#include "checkpoints/checkpoints.h"
#include "ringct/rctTypes.h"
#include "blockchain_db/blockchain_db.h"
#include "ringct/rctSigs.h"
#include "rpc/zmq_pub.h"
#include "common/notify.h"
#include "hardforks/hardforks.h"
#include "version.h"

#include <boost/filesystem.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

DISABLE_VS_WARNINGS(4355)

#define MERROR_VER(x) MCERROR("verify", x)

#define BAD_SEMANTICS_TXES_MAX_SIZE 100

// basically at least how many bytes the block itself serializes to without the miner tx
#define BLOCK_SIZE_SANITY_LEEWAY 100

namespace cryptonote
{
  const command_line::arg_descriptor<bool, false> arg_testnet_on  = {
    "testnet"
  , "Run on testnet. The wallet must be launched with --testnet flag."
  , false
  };
  const command_line::arg_descriptor<bool, false> arg_stagenet_on  = {
    "stagenet"
  , "Run on stagenet. The wallet must be launched with --stagenet flag."
  , false
  };
  const command_line::arg_descriptor<bool> arg_regtest_on  = {
    "regtest"
  , "Run in a regression testing mode."
  , false
  };
  const command_line::arg_descriptor<bool> arg_keep_fakechain = {
    "keep-fakechain"
  , "Don't delete any existing database when in fakechain mode."
  , false
  };
  const command_line::arg_descriptor<difficulty_type> arg_fixed_difficulty  = {
    "fixed-difficulty"
  , "Fixed difficulty used for testing."
  , 0
  };
  const command_line::arg_descriptor<std::string, false, true, 2> arg_data_dir = {
    "data-dir"
  , "Specify data directory"
  , tools::get_default_data_dir()
  , {{ &arg_testnet_on, &arg_stagenet_on }}
  , [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
      if (testnet_stagenet[0])
        return (boost::filesystem::path(val) / "testnet").string();
      else if (testnet_stagenet[1])
        return (boost::filesystem::path(val) / "stagenet").string();
      return val;
    }
  };
  const command_line::arg_descriptor<bool> arg_offline = {
    "offline"
  , "Do not listen for peers, nor connect to any"
  };
  const command_line::arg_descriptor<bool> arg_disable_dns_checkpoints = {
    "disable-dns-checkpoints"
  , "Do not retrieve checkpoints from DNS"
  };
  const command_line::arg_descriptor<size_t> arg_block_download_max_size  = {
    "block-download-max-size"
  , "Set maximum size of block download queue in bytes (0 for default)"
  , 0
  };
  const command_line::arg_descriptor<bool> arg_sync_pruned_blocks  = {
    "sync-pruned-blocks"
  , "Allow syncing from nodes with only pruned blocks"
  };

  static const command_line::arg_descriptor<bool> arg_test_drop_download = {
    "test-drop-download"
  , "For net tests: in download, discard ALL blocks instead checking/saving them (very fast)"
  };
  static const command_line::arg_descriptor<uint64_t> arg_test_drop_download_height = {
    "test-drop-download-height"
  , "Like test-drop-download but discards only after around certain height"
  , 0
  };
  static const command_line::arg_descriptor<int> arg_test_dbg_lock_sleep = {
    "test-dbg-lock-sleep"
  , "Sleep time in ms, defaults to 0 (off), used to debug before/after locking mutex. Values 100 to 1000 are good for tests."
  , 0
  };
  static const command_line::arg_descriptor<bool> arg_dns_checkpoints  = {
    "enforce-dns-checkpointing"
  , "checkpoints from DNS server will be enforced"
  , false
  };
  static const command_line::arg_descriptor<uint64_t> arg_fast_block_sync = {
    "fast-block-sync"
  , "Sync up most of the way by using embedded, known block hashes."
  , 1
  };
  static const command_line::arg_descriptor<uint64_t> arg_prep_blocks_threads = {
    "prep-blocks-threads"
  , "Max number of threads to use when preparing block hashes in groups."
  , 4
  };
  static const command_line::arg_descriptor<uint64_t> arg_show_time_stats  = {
    "show-time-stats"
  , "Show time-stats when processing blocks/txs and disk synchronization."
  , 0
  };
  static const command_line::arg_descriptor<size_t> arg_block_sync_size  = {
    "block-sync-size"
  , "How many blocks to sync at once during chain synchronization (0 = adaptive)."
  , 0
  };
  static const command_line::arg_descriptor<std::string> arg_check_updates = {
    "check-updates"
  , "Check for new versions of monero: [disabled|notify|download|update]"
  , "notify"
  };
  static const command_line::arg_descriptor<bool> arg_no_fluffy_blocks  = {
    "no-fluffy-blocks"
  , "Relay blocks as normal blocks"
  , false
  };
  static const command_line::arg_descriptor<size_t> arg_max_txpool_weight  = {
    "max-txpool-weight"
  , "Set maximum txpool weight in bytes."
  , DEFAULT_TXPOOL_MAX_WEIGHT
  };
  static const command_line::arg_descriptor<std::string> arg_block_notify = {
    "block-notify"
  , "Run a program for each new block, '%s' will be replaced by the block hash"
  , ""
  };
  static const command_line::arg_descriptor<bool> arg_prune_blockchain  = {
    "prune-blockchain"
  , "Prune blockchain"
  , false
  };
  static const command_line::arg_descriptor<std::string> arg_reorg_notify = {
    "reorg-notify"
  , "Run a program for each reorg, '%s' will be replaced by the split height, "
    "'%h' will be replaced by the new blockchain height, '%n' will be "
    "replaced by the number of new blocks in the new chain, and '%d' will be "
    "replaced by the number of blocks discarded from the old chain"
  , ""
  };
  static const command_line::arg_descriptor<std::string> arg_block_rate_notify = {
    "block-rate-notify"
  , "Run a program when the block rate undergoes large fluctuations. This might "
    "be a sign of large amounts of hash rate going on and off the Monero network, "
    "and thus be of potential interest in predicting attacks. %t will be replaced "
    "by the number of minutes for the observation window, %b by the number of "
    "blocks observed within that window, and %e by the number of blocks that was "
    "expected in that window. It is suggested that this notification is used to "
    "automatically increase the number of confirmations required before a payment "
    "is acted upon."
  , ""
  };
  static const command_line::arg_descriptor<bool> arg_keep_alt_blocks  = {
    "keep-alt-blocks"
  , "Keep alternative blocks on restart"
  , false
  };

  //-----------------------------------------------------------------------------------------------
  core::core(i_cryptonote_protocol* pprotocol):
              m_bap(),
              m_mempool(m_bap.tx_pool),
              m_blockchain_storage(m_bap.blockchain),
              m_miner(this, [this](const cryptonote::block &b, uint64_t height, const crypto::hash *seed_hash, unsigned int threads, crypto::hash &hash) {
                return cryptonote::get_block_longhash(&m_blockchain_storage, b, hash, height, seed_hash, threads);
              }),
              m_starter_message_showed(false),
              m_target_blockchain_height(0),
              m_checkpoints_path(""),
              m_last_dns_checkpoints_update(0),
              m_last_json_checkpoints_update(0),
              m_disable_dns_checkpoints(false),
              m_update_download(0),
              m_nettype(UNDEFINED),
              m_update_available(false)
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
  const checkpoints& core::get_checkpoints() const
  {
    return m_blockchain_storage.get_checkpoints();
  }
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
  //-----------------------------------------------------------------------------------
  void core::set_txpool_listener(boost::function<void(std::vector<txpool_event>)> zmq_pub)
  {
    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);
    m_zmq_pub = std::move(zmq_pub);
  }

  //-----------------------------------------------------------------------------------------------
  bool core::update_checkpoints(const bool skip_dns /* = false */)
  {
    if (m_nettype != MAINNET || m_disable_dns_checkpoints) return true;

    if (m_checkpoints_updating.test_and_set()) return true;

    bool res = true;
    if (!skip_dns && time(NULL) - m_last_dns_checkpoints_update >= 3600)
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
    m_miner.stop();
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
    command_line::add_arg(desc, arg_data_dir);

    command_line::add_arg(desc, arg_test_drop_download);
    command_line::add_arg(desc, arg_test_drop_download_height);

    command_line::add_arg(desc, arg_testnet_on);
    command_line::add_arg(desc, arg_stagenet_on);
    command_line::add_arg(desc, arg_regtest_on);
    command_line::add_arg(desc, arg_keep_fakechain);
    command_line::add_arg(desc, arg_fixed_difficulty);
    command_line::add_arg(desc, arg_dns_checkpoints);
    command_line::add_arg(desc, arg_prep_blocks_threads);
    command_line::add_arg(desc, arg_fast_block_sync);
    command_line::add_arg(desc, arg_show_time_stats);
    command_line::add_arg(desc, arg_block_sync_size);
    command_line::add_arg(desc, arg_check_updates);
    command_line::add_arg(desc, arg_no_fluffy_blocks);
    command_line::add_arg(desc, arg_test_dbg_lock_sleep);
    command_line::add_arg(desc, arg_offline);
    command_line::add_arg(desc, arg_disable_dns_checkpoints);
    command_line::add_arg(desc, arg_block_download_max_size);
    command_line::add_arg(desc, arg_sync_pruned_blocks);
    command_line::add_arg(desc, arg_max_txpool_weight);
    command_line::add_arg(desc, arg_block_notify);
    command_line::add_arg(desc, arg_prune_blockchain);
    command_line::add_arg(desc, arg_reorg_notify);
    command_line::add_arg(desc, arg_block_rate_notify);
    command_line::add_arg(desc, arg_keep_alt_blocks);

    miner::init_options(desc);
    BlockchainDB::init_options(desc);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_command_line(const boost::program_options::variables_map& vm)
  {
    if (m_nettype != FAKECHAIN)
    {
      const bool testnet = command_line::get_arg(vm, arg_testnet_on);
      const bool stagenet = command_line::get_arg(vm, arg_stagenet_on);
      m_nettype = testnet ? TESTNET : stagenet ? STAGENET : MAINNET;
    }

    m_config_folder = command_line::get_arg(vm, arg_data_dir);

    auto data_dir = boost::filesystem::path(m_config_folder);

    if (m_nettype == MAINNET)
    {
      cryptonote::checkpoints checkpoints;
      if (!checkpoints.init_default_checkpoints(m_nettype))
      {
        throw std::runtime_error("Failed to initialize checkpoints");
      }
      set_checkpoints(std::move(checkpoints));

      boost::filesystem::path json(JSON_HASH_FILE_NAME);
      boost::filesystem::path checkpoint_json_hashfile_fullpath = data_dir / json;

      set_checkpoints_file_path(checkpoint_json_hashfile_fullpath.string());
    }


    set_enforce_dns_checkpoints(command_line::get_arg(vm, arg_dns_checkpoints));
    test_drop_download_height(command_line::get_arg(vm, arg_test_drop_download_height));
    m_fluffy_blocks_enabled = !get_arg(vm, arg_no_fluffy_blocks);
    m_offline = get_arg(vm, arg_offline);
    m_disable_dns_checkpoints = get_arg(vm, arg_disable_dns_checkpoints);

    if (command_line::get_arg(vm, arg_test_drop_download) == true)
      test_drop_download();

    epee::debug::g_test_dbg_lock_sleep() = command_line::get_arg(vm, arg_test_dbg_lock_sleep);

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_current_blockchain_height() const
  {
    return m_blockchain_storage.get_current_blockchain_height();
  }
  //-----------------------------------------------------------------------------------------------
  void core::get_blockchain_top(uint64_t& height, crypto::hash& top_id) const
  {
    top_id = m_blockchain_storage.get_tail_id(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata,block>>& blocks, std::vector<cryptonote::blobdata>& txs) const
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks, txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata,block>>& blocks) const
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::vector<block>& blocks) const
  {
    std::vector<std::pair<cryptonote::blobdata, cryptonote::block>> bs;
    if (!m_blockchain_storage.get_blocks(start_offset, count, bs))
      return false;
    for (const auto &b: bs)
      blocks.push_back(b.second);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::vector<cryptonote::blobdata>& txs, std::vector<crypto::hash>& missed_txs, bool pruned) const
  {
    return m_blockchain_storage.get_transactions_blobs(txs_ids, txs, missed_txs, pruned);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_split_transactions_blobs(const std::vector<crypto::hash>& txs_ids, std::vector<std::tuple<crypto::hash, cryptonote::blobdata, crypto::hash, cryptonote::blobdata>>& txs, std::vector<crypto::hash>& missed_txs) const
  {
    return m_blockchain_storage.get_split_transactions_blobs(txs_ids, txs, missed_txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_txpool_backlog(std::vector<tx_backlog_entry>& backlog, bool include_sensitive_txes) const
  {
    m_mempool.get_transaction_backlog(backlog, include_sensitive_txes);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::vector<transaction>& txs, std::vector<crypto::hash>& missed_txs, bool pruned) const
  {
    return m_blockchain_storage.get_transactions(txs_ids, txs, missed_txs, pruned);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_alternative_blocks(std::vector<block>& blocks) const
  {
    return m_blockchain_storage.get_alternative_blocks(blocks);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_alternative_blocks_count() const
  {
    return m_blockchain_storage.get_alternative_blocks_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::init(const boost::program_options::variables_map& vm, const cryptonote::test_options *test_options, const GetCheckpointsCallback& get_checkpoints/* = nullptr */, bool allow_dns)
  {
    start_time = std::time(nullptr);

    const bool regtest = command_line::get_arg(vm, arg_regtest_on);
    if (test_options != NULL || regtest)
    {
      m_nettype = FAKECHAIN;
    }
    bool r = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to handle command line");
    m_disable_dns_checkpoints |= not allow_dns;

    std::string db_sync_mode = command_line::get_arg(vm, cryptonote::arg_db_sync_mode);
    bool db_salvage = command_line::get_arg(vm, cryptonote::arg_db_salvage) != 0;
    bool fast_sync = command_line::get_arg(vm, arg_fast_block_sync) != 0;
    uint64_t blocks_threads = command_line::get_arg(vm, arg_prep_blocks_threads);
    std::string check_updates_string = command_line::get_arg(vm, arg_check_updates);
    size_t max_txpool_weight = command_line::get_arg(vm, arg_max_txpool_weight);
    bool prune_blockchain = command_line::get_arg(vm, arg_prune_blockchain);
    bool keep_alt_blocks = command_line::get_arg(vm, arg_keep_alt_blocks);
    bool keep_fakechain = command_line::get_arg(vm, arg_keep_fakechain);

    boost::filesystem::path folder(m_config_folder);
    if (m_nettype == FAKECHAIN)
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

    std::unique_ptr<BlockchainDB> db(new_db());
    if (db == NULL)
    {
      LOG_ERROR("Failed to initialize a database");
      return false;
    }

    folder /= db->get_db_name();
    MGINFO("Loading blockchain from folder " << folder.string() << " ...");

    const std::string filename = folder.string();
    // default to fast:async:1 if overridden
    blockchain_db_sync_mode sync_mode = db_defaultsync;
    bool sync_on_blocks = true;
    uint64_t sync_threshold = 1;

    if (m_nettype == FAKECHAIN && !keep_fakechain)
    {
      // reset the db by removing the database file before opening it
      if (!db->remove_data_file(filename))
      {
        MERROR("Failed to remove data file in " << filename);
        return false;
      }
    }

    try
    {
      uint64_t db_flags = 0;

      std::vector<std::string> options;
      boost::trim(db_sync_mode);
      boost::split(options, db_sync_mode, boost::is_any_of(" :"));
      const bool db_sync_mode_is_default = command_line::is_arg_defaulted(vm, cryptonote::arg_db_sync_mode);

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
          sync_mode = db_sync_mode_is_default ? db_defaultsync : db_nosync;
        }
        else if(options[0] == "fast")
        {
          db_flags = DBF_FAST;
          sync_mode = db_sync_mode_is_default ? db_defaultsync : db_async;
        }
        else if(options[0] == "fastest")
        {
          db_flags = DBF_FASTEST;
#ifdef _WIN32
          sync_threshold = 1000; // default to fastest:async:1000
#else
          sync_threshold = 100000; // default to fastest:async:100000
#endif
          sync_mode = db_sync_mode_is_default ? db_defaultsync : db_async;
        }
        else
          db_flags = DEFAULT_FLAGS;
      }

      if(options.size() >= 2 && !safemode)
      {
        if(options[1] == "sync")
          sync_mode = db_sync_mode_is_default ? db_defaultsync : db_sync;
        else if(options[1] == "async")
          sync_mode = db_sync_mode_is_default ? db_defaultsync : db_async;
      }

      if(options.size() >= 3 && !safemode)
      {
        char *endptr;
        uint64_t threshold = strtoull(options[2].c_str(), &endptr, 0);
        if (*endptr == '\0' || !strcmp(endptr, "blocks"))
        {
          sync_on_blocks = true;
          sync_threshold = threshold;
        }
        else if (!strcmp(endptr, "bytes"))
        {
          sync_on_blocks = false;
          sync_threshold = threshold;
        }
        else
        {
          LOG_ERROR("Invalid db sync mode: " << options[2]);
          return false;
        }
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
        sync_on_blocks, sync_threshold, sync_mode, fast_sync);

    try
    {
      if (!command_line::is_arg_defaulted(vm, arg_block_notify))
      {
        struct hash_notify
        {
          tools::Notify cmdline;

          void operator()(std::uint64_t, epee::span<const block> blocks) const
          {
            for (const block& bl : blocks)
              cmdline.notify("%s", epee::string_tools::pod_to_hex(get_block_hash(bl)).c_str(), NULL);
          }
        };

        m_blockchain_storage.add_block_notify(hash_notify{{command_line::get_arg(vm, arg_block_notify).c_str()}});
      }
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to parse block notify spec: " << e.what());
    }

    try
    {
      if (!command_line::is_arg_defaulted(vm, arg_reorg_notify))
        m_blockchain_storage.set_reorg_notify(std::shared_ptr<tools::Notify>(new tools::Notify(command_line::get_arg(vm, arg_reorg_notify).c_str())));
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to parse reorg notify spec: " << e.what());
    }

    try
    {
      if (!command_line::is_arg_defaulted(vm, arg_block_rate_notify))
        m_block_rate_notify.reset(new tools::Notify(command_line::get_arg(vm, arg_block_rate_notify).c_str()));
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to parse block rate notify spec: " << e.what());
    }

    const std::pair<uint8_t, uint64_t> regtest_hard_forks[3] = {std::make_pair(1, 0), std::make_pair(mainnet_hard_forks[num_mainnet_hard_forks-1].version, 1), std::make_pair(0, 0)};
    const cryptonote::test_options regtest_test_options = {
      regtest_hard_forks,
      0
    };
    const difficulty_type fixed_difficulty = command_line::get_arg(vm, arg_fixed_difficulty);
    r = m_blockchain_storage.init(db.release(), m_nettype, m_offline, regtest ? &regtest_test_options : test_options, fixed_difficulty, get_checkpoints);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    r = m_mempool.init(max_txpool_weight, m_nettype == FAKECHAIN);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize memory pool");

    // now that we have a valid m_blockchain_storage, we can clean out any
    // transactions in the pool that do not conform to the current fork
    m_mempool.validate(m_blockchain_storage.get_current_hard_fork_version());

    bool show_time_stats = command_line::get_arg(vm, arg_show_time_stats) != 0;
    m_blockchain_storage.set_show_time_stats(show_time_stats);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    block_sync_size = command_line::get_arg(vm, arg_block_sync_size);
    if (block_sync_size > BLOCKS_SYNCHRONIZING_MAX_COUNT)
      MERROR("Error --block-sync-size cannot be greater than " << BLOCKS_SYNCHRONIZING_MAX_COUNT);

    MGINFO("Loading checkpoints");

    // load json & DNS checkpoints, and verify them
    // with respect to what blocks we already have
    const bool skip_dns_checkpoints = !command_line::get_arg(vm, arg_dns_checkpoints);
    CHECK_AND_ASSERT_MES(update_checkpoints(skip_dns_checkpoints), false, "One or more checkpoints loaded from json or dns conflicted with existing checkpoints.");

   // DNS versions checking
    if (check_updates_string == "disabled" || not allow_dns)
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

    r = m_miner.init(vm, m_nettype);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize miner instance");

    if (!keep_alt_blocks && !m_blockchain_storage.get_db().is_read_only())
      m_blockchain_storage.get_db().drop_alt_blocks();

    if (prune_blockchain)
    {
      // display a message if the blockchain is not pruned yet
      if (!m_blockchain_storage.get_blockchain_pruning_seed())
      {
        MGINFO("Pruning blockchain...");
        CHECK_AND_ASSERT_MES(m_blockchain_storage.prune_blockchain(), false, "Failed to prune blockchain");
      }
      else
      {
        CHECK_AND_ASSERT_MES(m_blockchain_storage.update_blockchain_pruning(), false, "Failed to update blockchain pruning");
      }
    }

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
  bool core::handle_incoming_tx_pre(const tx_blob_entry& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash)
  {
    tvc = {};

    if(tx_blob.blob.size() > get_max_tx_size())
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, too big size " << tx_blob.blob.size() << ", rejected");
      tvc.m_verifivation_failed = true;
      tvc.m_too_big = true;
      return false;
    }

    tx_hash = crypto::null_hash;

    bool r;
    if (tx_blob.prunable_hash == crypto::null_hash)
    {
      r = parse_tx_from_blob(tx, tx_hash, tx_blob.blob);
    }
    else
    {
      r = parse_and_validate_tx_base_from_blob(tx_blob.blob, tx);
      if (r)
      {
        tx.set_prunable_hash(tx_blob.prunable_hash);
        tx_hash = cryptonote::get_pruned_transaction_hash(tx, tx_blob.prunable_hash);
        tx.set_hash(tx_hash);
      }
    }

    if (!r)
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
      MERROR_VER("Bad tx version (" << tx.version << ", max is " << max_tx_version << ")");
      tvc.m_verifivation_failed = true;
      return false;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx_post(const tx_blob_entry& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash)
  {
    if(!check_tx_syntax(tx))
    {
      LOG_PRINT_L1("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " syntax, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::set_semantics_failed(const crypto::hash &tx_hash)
  {
    LOG_PRINT_L1("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " semantic, rejected");
    bad_semantics_txes_lock.lock();
    bad_semantics_txes[0].insert(tx_hash);
    if (bad_semantics_txes[0].size() >= BAD_SEMANTICS_TXES_MAX_SIZE)
    {
      std::swap(bad_semantics_txes[0], bad_semantics_txes[1]);
      bad_semantics_txes[0].clear();
    }
    bad_semantics_txes_lock.unlock();
  }
  //-----------------------------------------------------------------------------------------------
  static bool is_canonical_bulletproof_layout(const std::vector<rct::Bulletproof> &proofs)
  {
    if (proofs.size() != 1)
      return false;
    const size_t sz = proofs[0].V.size();
    if (sz == 0 || sz > BULLETPROOF_MAX_OUTPUTS)
      return false;
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  static bool is_canonical_bulletproof_plus_layout(const std::vector<rct::BulletproofPlus> &proofs)
  {
    if (proofs.size() != 1)
      return false;
    const size_t sz = proofs[0].V.size();
    if (sz == 0 || sz > BULLETPROOF_PLUS_MAX_OUTPUTS)
      return false;
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx_accumulated_batch(std::vector<tx_verification_batch_info> &tx_info, bool keeped_by_block)
  {
    bool ret = true;
    if (keeped_by_block && get_blockchain_storage().is_within_compiled_block_hash_area())
    {
      MTRACE("Skipping semantics check for tx kept by block in embedded hash area");
      return true;
    }

    std::vector<const rct::rctSig*> rvv;
    for (size_t n = 0; n < tx_info.size(); ++n)
    {
      if (!check_tx_semantic(*tx_info[n].tx, keeped_by_block))
      {
        set_semantics_failed(tx_info[n].tx_hash);
        tx_info[n].tvc.m_verifivation_failed = true;
        tx_info[n].result = false;
        continue;
      }

      if (tx_info[n].tx->version < 2)
        continue;
      const rct::rctSig &rv = tx_info[n].tx->rct_signatures;
      switch (rv.type) {
        case rct::RCTTypeNull:
          // coinbase should not come here, so we reject for all other types
          MERROR_VER("Unexpected Null rctSig type");
          set_semantics_failed(tx_info[n].tx_hash);
          tx_info[n].tvc.m_verifivation_failed = true;
          tx_info[n].result = false;
          break;
        case rct::RCTTypeSimple:
          if (!rct::verRctSemanticsSimple(rv))
          {
            MERROR_VER("rct signature semantics check failed");
            set_semantics_failed(tx_info[n].tx_hash);
            tx_info[n].tvc.m_verifivation_failed = true;
            tx_info[n].result = false;
            break;
          }
          break;
        case rct::RCTTypeFull:
          if (!rct::verRct(rv, true))
          {
            MERROR_VER("rct signature semantics check failed");
            set_semantics_failed(tx_info[n].tx_hash);
            tx_info[n].tvc.m_verifivation_failed = true;
            tx_info[n].result = false;
            break;
          }
          break;
        case rct::RCTTypeBulletproof:
        case rct::RCTTypeBulletproof2:
        case rct::RCTTypeCLSAG:
          if (!is_canonical_bulletproof_layout(rv.p.bulletproofs))
          {
            MERROR_VER("Bulletproof does not have canonical form");
            set_semantics_failed(tx_info[n].tx_hash);
            tx_info[n].tvc.m_verifivation_failed = true;
            tx_info[n].result = false;
            break;
          }
          rvv.push_back(&rv); // delayed batch verification
          break;
        case rct::RCTTypeBulletproofPlus:
          if (!is_canonical_bulletproof_plus_layout(rv.p.bulletproofs_plus))
          {
            MERROR_VER("Bulletproof_plus does not have canonical form");
            set_semantics_failed(tx_info[n].tx_hash);
            tx_info[n].tvc.m_verifivation_failed = true;
            tx_info[n].result = false;
            break;
          }
          rvv.push_back(&rv); // delayed batch verification
          break;
        default:
          MERROR_VER("Unknown rct type: " << rv.type);
          set_semantics_failed(tx_info[n].tx_hash);
          tx_info[n].tvc.m_verifivation_failed = true;
          tx_info[n].result = false;
          break;
      }
    }
    if (!rvv.empty() && !rct::verRctSemanticsSimple(rvv))
    {
      LOG_PRINT_L1("One transaction among this group has bad semantics, verifying one at a time");
      ret = false;
      const bool assumed_bad = rvv.size() == 1; // if there's only one tx, it must be the bad one
      for (size_t n = 0; n < tx_info.size(); ++n)
      {
        if (!tx_info[n].result)
          continue;
        if (tx_info[n].tx->rct_signatures.type != rct::RCTTypeBulletproof && tx_info[n].tx->rct_signatures.type != rct::RCTTypeBulletproof2 && tx_info[n].tx->rct_signatures.type != rct::RCTTypeCLSAG && tx_info[n].tx->rct_signatures.type != rct::RCTTypeBulletproofPlus)
          continue;
        if (assumed_bad || !rct::verRctSemanticsSimple(tx_info[n].tx->rct_signatures))
        {
          set_semantics_failed(tx_info[n].tx_hash);
          tx_info[n].tvc.m_verifivation_failed = true;
          tx_info[n].result = false;
        }
      }
    }

    return ret;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_txs(const epee::span<const tx_blob_entry> tx_blobs, epee::span<tx_verification_context> tvc, relay_method tx_relay, bool relayed)
  {
    TRY_ENTRY();

    if (tx_blobs.size() != tvc.size())
    {
      MERROR("tx_blobs and tx_verification_context spans must have equal size");
      return false;
    }

    std::vector<txpool_event> results(tx_blobs.size());

    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);

    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);
    epee::span<tx_blob_entry>::const_iterator it = tx_blobs.begin();
    for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
      tpool.submit(&waiter, [&, i, it] {
        try
        {
          results[i].res = handle_incoming_tx_pre(*it, tvc[i], results[i].tx, results[i].hash);
        }
        catch (const std::exception &e)
        {
          MERROR_VER("Exception in handle_incoming_tx_pre: " << e.what());
          tvc[i].m_verifivation_failed = true;
          results[i].res = false;
        }
      });
    }
    if (!waiter.wait())
      return false;
    it = tx_blobs.begin();
    std::vector<bool> already_have(tx_blobs.size(), false);
    for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
      if (!results[i].res)
        continue;
      if(m_mempool.have_tx(results[i].hash, relay_category::legacy))
      {
        LOG_PRINT_L2("tx " << results[i].hash << "already have transaction in tx_pool");
        already_have[i] = true;
      }
      else if(m_blockchain_storage.have_tx(results[i].hash))
      {
        LOG_PRINT_L2("tx " << results[i].hash << " already have transaction in blockchain");
        already_have[i] = true;
      }
      else
      {
        tpool.submit(&waiter, [&, i, it] {
          try
          {
            results[i].res = handle_incoming_tx_post(*it, tvc[i], results[i].tx, results[i].hash);
          }
          catch (const std::exception &e)
          {
            MERROR_VER("Exception in handle_incoming_tx_post: " << e.what());
            tvc[i].m_verifivation_failed = true;
            results[i].res = false;
          }
        });
      }
    }
    if (!waiter.wait())
      return false;

    std::vector<tx_verification_batch_info> tx_info;
    tx_info.reserve(tx_blobs.size());
    for (size_t i = 0; i < tx_blobs.size(); i++) {
      if (!results[i].res || already_have[i])
        continue;
      tx_info.push_back({&results[i].tx, results[i].hash, tvc[i], results[i].res});
    }
    if (!tx_info.empty())
      handle_incoming_tx_accumulated_batch(tx_info, tx_relay == relay_method::block);

    bool valid_events = false;
    bool ok = true;
    it = tx_blobs.begin();
    for (size_t i = 0; i < tx_blobs.size(); i++, ++it) {
      if (!results[i].res)
      {
        ok = false;
        continue;
      }
      if (tx_relay == relay_method::block)
        get_blockchain_storage().on_new_tx_from_block(results[i].tx);
      if (already_have[i])
        continue;

      results[i].blob_size = it->blob.size();
      results[i].weight = results[i].tx.pruned ? get_pruned_transaction_weight(results[i].tx) : get_transaction_weight(results[i].tx, it->blob.size());
      ok &= add_new_tx(results[i].tx, results[i].hash, tx_blobs[i].blob, results[i].weight, tvc[i], tx_relay, relayed);

      if(tvc[i].m_verifivation_failed)
      {MERROR_VER("Transaction verification failed: " << results[i].hash);}
      else if(tvc[i].m_verifivation_impossible)
      {MERROR_VER("Transaction verification impossible: " << results[i].hash);}

      if(tvc[i].m_added_to_pool && results[i].tx.extra.size() <= MAX_TX_EXTRA_SIZE)
      {
        MDEBUG("tx added: " << results[i].hash);
        valid_events = true;
      }
      else
        results[i].res = false;
    }

    if (valid_events && m_zmq_pub && matches_category(tx_relay, relay_category::legacy))
      m_zmq_pub(std::move(results));

    return ok;
    CATCH_ENTRY_L0("core::handle_incoming_txs()", false);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx(const tx_blob_entry& tx_blob, tx_verification_context& tvc, relay_method tx_relay, bool relayed)
  {
    return handle_incoming_txs({std::addressof(tx_blob), 1}, {std::addressof(tvc), 1}, tx_relay, relayed);
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

    if(!keeped_by_block && get_transaction_weight(tx) >= m_blockchain_storage.get_current_cumulative_block_weight_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE)
    {
      MERROR_VER("tx is too large " << get_transaction_weight(tx) << ", expected not bigger than " << m_blockchain_storage.get_current_cumulative_block_weight_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
      return false;
    }

    //check if tx use different key images
    if(!check_tx_inputs_keyimages_diff(tx))
    {
      MERROR_VER("tx uses a single key image more than once");
      return false;
    }

    const uint8_t hf_version = m_blockchain_storage.get_current_hard_fork_version();
    if (!check_tx_inputs_ring_members_diff(tx, hf_version))
    {
      MERROR_VER("tx uses duplicate ring members");
      return false;
    }

    if (!check_tx_inputs_keyimages_domain(tx))
    {
      MERROR_VER("tx uses key image not in the valid domain");
      return false;
    }

    if (!check_output_types(tx, hf_version))
    {
      MERROR_VER("tx does not use valid output type(s)");
      return false;
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
    static const uint64_t quick_height = m_nettype == TESTNET ? 801219 : m_nettype == MAINNET ? 1220516 : 0;
    size_t res = 0;
    if (block_sync_size > 0)
      res = block_sync_size;
    else if (height >= quick_height)
      res = BLOCKS_SYNCHRONIZING_DEFAULT_COUNT;
    else
      res = BLOCKS_SYNCHRONIZING_DEFAULT_COUNT_PRE_V4;

    static size_t max_block_size = 0;
    if (max_block_size == 0)
    {
      const char *env = getenv("SEEDHASH_EPOCH_BLOCKS");
      if (env)
      {
        int n = atoi(env);
        if (n <= 0)
          n = BLOCKS_SYNCHRONIZING_MAX_COUNT;
        size_t p = 1;
        while (p < (size_t)n)
          p <<= 1;
        max_block_size = p;
      }
      else
        max_block_size = BLOCKS_SYNCHRONIZING_MAX_COUNT;
    }
    if (res > max_block_size)
    {
      static bool warned = false;
      if (!warned)
      {
        MWARNING("Clamping block sync size to " << max_block_size);
        warned = true;
      }
      res = max_block_size;
    }
    return res;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::are_key_images_spent_in_pool(const std::vector<crypto::key_image>& key_im, std::vector<bool> &spent) const
  {
    spent.clear();

    return m_mempool.check_for_key_images(key_im, spent);
  }
  //-----------------------------------------------------------------------------------------------
  std::pair<boost::multiprecision::uint128_t, boost::multiprecision::uint128_t> core::get_coinbase_tx_sum(const uint64_t start_offset, const size_t count)
  {
    boost::multiprecision::uint128_t emission_amount = 0;
    boost::multiprecision::uint128_t total_fee_amount = 0;
    if (count)
    {
      const uint64_t end = start_offset + count - 1;
      m_blockchain_storage.for_blocks_range(start_offset, end,
        [this, &emission_amount, &total_fee_amount](uint64_t, const crypto::hash& hash, const block& b){
      std::vector<transaction> txs;
      std::vector<crypto::hash> missed_txs;
      uint64_t coinbase_amount = get_outs_money_amount(b.miner_tx);
      this->get_transactions(b.tx_hashes, txs, missed_txs, true);
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

    return std::pair<boost::multiprecision::uint128_t, boost::multiprecision::uint128_t>(emission_amount, total_fee_amount);
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
  bool core::check_tx_inputs_ring_members_diff(const transaction& tx, const uint8_t hf_version) const
  {
    if (hf_version >= 6)
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
  bool core::add_new_tx(transaction& tx, tx_verification_context& tvc, relay_method tx_relay, bool relayed)
  {
    crypto::hash tx_hash = get_transaction_hash(tx);
    blobdata bl;
    t_serializable_object_to_blob(tx, bl);
    size_t tx_weight = get_transaction_weight(tx, bl.size());
    return add_new_tx(tx, tx_hash, bl, tx_weight, tvc, tx_relay, relayed);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_blockchain_total_transactions() const
  {
    return m_blockchain_storage.get_total_transactions();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(transaction& tx, const crypto::hash& tx_hash, const cryptonote::blobdata &blob, size_t tx_weight, tx_verification_context& tvc, relay_method tx_relay, bool relayed)
  {
    if(m_mempool.have_tx(tx_hash, relay_category::legacy))
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
    return m_mempool.add_tx(tx, tx_hash, blob, tx_weight, tvc, tx_relay, relayed, version);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::relay_txpool_transactions()
  {
    // we attempt to relay txes that should be relayed, but were not
    std::vector<std::tuple<crypto::hash, cryptonote::blobdata, relay_method>> txs;
    if (m_mempool.get_relayable_transactions(txs) && !txs.empty())
    {
      NOTIFY_NEW_TRANSACTIONS::request public_req{};
      NOTIFY_NEW_TRANSACTIONS::request private_req{};
      NOTIFY_NEW_TRANSACTIONS::request stem_req{};
      for (auto& tx : txs)
      {
        switch (std::get<2>(tx))
        {
          default:
          case relay_method::none:
            break;
          case relay_method::local:
            private_req.txs.push_back(std::move(std::get<1>(tx)));
            break;
          case relay_method::forward:
            stem_req.txs.push_back(std::move(std::get<1>(tx)));
            break;
          case relay_method::block:
          case relay_method::fluff:
          case relay_method::stem:
            public_req.txs.push_back(std::move(std::get<1>(tx)));
            break;
        }
      }

      /* All txes are sent on randomized timers per connection in
         `src/cryptonote_protocol/levin_notify.cpp.` They are either sent with
         "white noise" delays or via  diffusion (Dandelion++ fluff). So
         re-relaying public and private _should_ be acceptable here. */
      const boost::uuids::uuid source = boost::uuids::nil_uuid();
      if (!public_req.txs.empty())
        get_protocol()->relay_transactions(public_req, source, epee::net_utils::zone::public_, relay_method::fluff);
      if (!private_req.txs.empty())
        get_protocol()->relay_transactions(private_req, source, epee::net_utils::zone::invalid, relay_method::local);
      if (!stem_req.txs.empty())
        get_protocol()->relay_transactions(stem_req, source, epee::net_utils::zone::public_, relay_method::stem);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::notify_txpool_event(const epee::span<const cryptonote::blobdata> tx_blobs, epee::span<const crypto::hash> tx_hashes, epee::span<const cryptonote::transaction> txs, const std::vector<bool> &just_broadcasted) const
  {
    if (!m_zmq_pub)
      return true;

    if (tx_blobs.size() != tx_hashes.size() || tx_blobs.size() != txs.size() || tx_blobs.size() != just_broadcasted.size())
      return false;

    /* Publish txs via ZMQ that are "just broadcasted" by the daemon. This is
       done here in addition to `handle_incoming_txs` in order to guarantee txs
       are pub'd via ZMQ when we know the daemon has/will broadcast to other
       nodes & *after* the tx is visible in the pool. This should get called
       when the user submits a tx to a daemon in the "fluff" epoch relaying txs
       via a public network. */
    if (std::count(just_broadcasted.begin(), just_broadcasted.end(), true) == 0)
      return true;

    std::vector<txpool_event> results{};
    results.resize(tx_blobs.size());
    for (std::size_t i = 0; i < results.size(); ++i)
    {
      results[i].tx = std::move(txs[i]);
      results[i].hash = std::move(tx_hashes[i]);
      results[i].blob_size = tx_blobs[i].size();
      results[i].weight = results[i].tx.pruned ? get_pruned_transaction_weight(results[i].tx) : get_transaction_weight(results[i].tx, results[i].blob_size);
      results[i].res = just_broadcasted[i];
    }

    m_zmq_pub(std::move(results));

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::on_transactions_relayed(const epee::span<const cryptonote::blobdata> tx_blobs, const relay_method tx_relay)
  {
    // lock ensures duplicate txs aren't pub'd via zmq
    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);

    std::vector<crypto::hash> tx_hashes{};
    tx_hashes.resize(tx_blobs.size());

    std::vector<cryptonote::transaction> txs{};
    txs.resize(tx_blobs.size());

    for (std::size_t i = 0; i < tx_blobs.size(); ++i)
    {
      if (!parse_and_validate_tx_from_blob(tx_blobs[i], txs[i], tx_hashes[i]))
      {
        LOG_ERROR("Failed to parse relayed transaction");
        return;
      }
    }

    std::vector<bool> just_broadcasted{};
    just_broadcasted.reserve(tx_hashes.size());

    m_mempool.set_relayed(epee::to_span(tx_hashes), tx_relay, just_broadcasted);

    if (m_zmq_pub && matches_category(tx_relay, relay_category::legacy))
      notify_txpool_event(tx_blobs, epee::to_span(tx_hashes), epee::to_span(txs), just_broadcasted);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce, uint64_t &seed_height, crypto::hash &seed_hash)
  {
    return m_blockchain_storage.create_block_template(b, adr, diffic, height, expected_reward, ex_nonce, seed_height, seed_hash);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(block& b, const crypto::hash *prev_block, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce, uint64_t &seed_height, crypto::hash &seed_hash)
  {
    return m_blockchain_storage.create_block_template(b, prev_block, adr, diffic, height, expected_reward, ex_nonce, seed_height, seed_hash);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_miner_data(uint8_t& major_version, uint64_t& height, crypto::hash& prev_id, crypto::hash& seed_hash, difficulty_type& difficulty, uint64_t& median_weight, uint64_t& already_generated_coins, std::vector<tx_block_template_backlog_entry>& tx_backlog)
  {
    return m_blockchain_storage.get_miner_data(major_version, height, prev_id, seed_hash, difficulty, median_weight, already_generated_coins, tx_backlog);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, bool clip_pruned, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, clip_pruned, resp);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::vector<std::pair<std::pair<cryptonote::blobdata, crypto::hash>, std::vector<std::pair<crypto::hash, cryptonote::blobdata> > > >& blocks, uint64_t& total_height, uint64_t& start_height, bool pruned, bool get_miner_tx_hash, size_t max_block_count, size_t max_tx_count) const
  {
    return m_blockchain_storage.find_blockchain_supplement(req_start_block, qblock_ids, blocks, total_height, start_height, pruned, get_miner_tx_hash, max_block_count, max_tx_count);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_outs(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const
  {
    return m_blockchain_storage.get_outs(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) const
  {
    return m_blockchain_storage.get_output_distribution(amount, from_height, to_height, start_height, distribution, base);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const
  {
    return m_blockchain_storage.get_tx_outputs_gindexs(tx_id, indexs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_tx_outputs_gindexs(const crypto::hash& tx_id, size_t n_txes, std::vector<std::vector<uint64_t>>& indexs) const
  {
    return m_blockchain_storage.get_tx_outputs_gindexs(tx_id, n_txes, indexs);
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
    bce.block_weight = 0; // we can leave it to 0, those txes aren't pruned
    for (const auto &tx_hash: b.tx_hashes)
    {
      cryptonote::blobdata txblob;
      CHECK_AND_ASSERT_THROW_MES(pool.get_transaction(tx_hash, txblob, relay_category::all), "Transaction not found in pool");
      bce.txs.push_back({txblob, crypto::null_hash});
    }
    return bce;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_block_found(block& b, block_verification_context &bvc)
  {
    bvc = {};
    m_miner.pause();
    std::vector<block_complete_entry> blocks;
    try
    {
      blocks.push_back(get_block_complete_entry(b, m_mempool));
    }
    catch (const std::exception &e)
    {
      m_miner.resume();
      return false;
    }
    std::vector<block> pblocks;
    if (!prepare_handle_incoming_blocks(blocks, pblocks))
    {
      MERROR("Block found, but failed to prepare to add");
      m_miner.resume();
      return false;
    }
    m_blockchain_storage.add_new_block(b, bvc);
    const bool force_sync = m_nettype != FAKECHAIN;
    cleanup_handle_incoming_blocks(force_sync);
    //anyway - update miner template
    update_miner_block_template();
    m_miner.resume();


    CHECK_AND_ASSERT_MES(!bvc.m_verifivation_failed, false, "mined block failed verification");
    if(bvc.m_added_to_main_chain)
    {
      cryptonote_connection_context exclude_context = {};
      NOTIFY_NEW_BLOCK::request arg = AUTO_VAL_INIT(arg);
      arg.current_blockchain_height = m_blockchain_storage.get_current_blockchain_height();
      std::vector<crypto::hash> missed_txs;
      std::vector<cryptonote::blobdata> txs;
      m_blockchain_storage.get_transactions_blobs(b.tx_hashes, txs, missed_txs);
      if(missed_txs.size() &&  m_blockchain_storage.get_block_id_by_height(get_block_height(b)) != get_block_hash(b))
      {
        LOG_PRINT_L1("Block found but, seems that reorganize just happened after that, do not relay this block");
        return true;
      }
      CHECK_AND_ASSERT_MES(txs.size() == b.tx_hashes.size() && !missed_txs.size(), false, "can't find some transactions in found block:" << get_block_hash(b) << " txs.size()=" << txs.size()
        << ", b.tx_hashes.size()=" << b.tx_hashes.size() << ", missed_txs.size()" << missed_txs.size());

      block_to_blob(b, arg.b.block);
      //pack transactions
      for(auto& tx:  txs)
        arg.b.txs.push_back({tx, crypto::null_hash});

      m_pprotocol->relay_block(arg, exclude_context);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::is_synchronized() const
  {
    return m_pprotocol != nullptr && m_pprotocol->is_synchronized();
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
  bool core::prepare_handle_incoming_blocks(const std::vector<block_complete_entry> &blocks_entry, std::vector<block> &blocks)
  {
    m_incoming_tx_lock.lock();
    if (!m_blockchain_storage.prepare_handle_incoming_blocks(blocks_entry, blocks))
    {
      cleanup_handle_incoming_blocks(false);
      return false;
    }
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
  bool core::handle_incoming_block(const blobdata& block_blob, const block *b, block_verification_context& bvc, bool update_miner_blocktemplate)
  {
    TRY_ENTRY();

    bvc = {};

    if (!check_incoming_block_size(block_blob))
    {
      bvc.m_verifivation_failed = true;
      return false;
    }

    if (((size_t)-1) <= 0xffffffff && block_blob.size() >= 0x3fffffff)
      MWARNING("This block's size is " << block_blob.size() << ", closing on the 32 bit limit");

    block lb;
    if (!b)
    {
      crypto::hash block_hash;
      if(!parse_and_validate_block_from_blob(block_blob, lb, block_hash))
      {
        LOG_PRINT_L1("Failed to parse and validate new block");
        bvc.m_verifivation_failed = true;
        return false;
      }
      b = &lb;
    }
    add_new_block(*b, bvc);
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
    // note: we assume block weight is always >= block blob size, so we check incoming
    // blob size against the block weight limit, which acts as a sanity check without
    // having to parse/weigh first; in fact, since the block blob is the block header
    // plus the tx hashes, the weight will typically be much larger than the blob size
    if(block_blob.size() > m_blockchain_storage.get_current_cumulative_block_weight_limit() + BLOCK_SIZE_SANITY_LEEWAY)
    {
      LOG_PRINT_L1("WRONG BLOCK BLOB, sanity check failed on size " << block_blob.size() << ", rejected");
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
  size_t core::get_pool_transactions_count(bool include_sensitive_txes) const
  {
    return m_mempool.get_transactions_count(include_sensitive_txes);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::have_block_unlocked(const crypto::hash& id, int *where) const
  {
    return m_blockchain_storage.have_block_unlocked(id, where);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::have_block(const crypto::hash& id, int *where) const
  {
    return m_blockchain_storage.have_block(id, where);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, const blobdata& blob) const
  {
    return parse_and_validate_tx_from_blob(blob, tx, tx_hash);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_syntax(const transaction& tx) const
  {
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions_info(const std::vector<crypto::hash>& txids, std::vector<std::pair<crypto::hash, tx_memory_pool::tx_details>>& txs, bool include_sensitive_txes) const
  {
    return m_mempool.get_transactions_info(txids, txs, include_sensitive_txes);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions(std::vector<transaction>& txs, bool include_sensitive_data) const
  {
    m_mempool.get_transactions(txs, include_sensitive_data);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction_hashes(std::vector<crypto::hash>& txs, bool include_sensitive_data) const
  {
    m_mempool.get_transaction_hashes(txs, include_sensitive_data);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_info(time_t start_time, bool include_sensitive_txes, size_t max_tx_count, std::vector<std::pair<crypto::hash, tx_memory_pool::tx_details>>& added_txs, std::vector<crypto::hash>& remaining_added_txids, std::vector<crypto::hash>& removed_txs, bool& incremental) const
  {
    return m_mempool.get_pool_info(start_time, include_sensitive_txes, max_tx_count, added_txs, remaining_added_txids, removed_txs, incremental);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction_stats(struct txpool_stats& stats, bool include_sensitive_data) const
  {
    m_mempool.get_transaction_stats(stats, include_sensitive_data);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transaction(const crypto::hash &id, cryptonote::blobdata& tx, relay_category tx_category) const
  {
    return m_mempool.get_transaction(id, tx, tx_category);
  }  
  //-----------------------------------------------------------------------------------------------
  bool core::pool_has_tx(const crypto::hash &id) const
  {
    return m_mempool.have_tx(id, relay_category::legacy);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos, bool include_sensitive_data) const
  {
    return m_mempool.get_transactions_and_spent_keys_info(tx_infos, key_image_infos, include_sensitive_data);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_for_rpc(std::vector<cryptonote::rpc::tx_in_pool>& tx_infos, cryptonote::rpc::key_images_with_tx_hashes& key_image_infos) const
  {
    return m_mempool.get_pool_for_rpc(tx_infos, key_image_infos);
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
      std::string main_message;
      if (m_offline)
        main_message = "The daemon is running offline and will not attempt to sync to the Monero network.";
      else
        main_message = "The daemon will start synchronizing with the network. This may take a long time to complete.";
      MGINFO_YELLOW(ENDL << "**********************************************************************" << ENDL
        << main_message << ENDL
        << ENDL
        << "You can set the level of process detailization through \"set_log <level|categories>\" command," << ENDL
        << "where <level> is between 0 (no details) and 4 (very verbose), or custom category based levels (eg, *:WARNING)." << ENDL
        << ENDL
        << "Use the \"help\" command to see the list of available commands." << ENDL
        << "Use \"help <command>\" to see a command's documentation." << ENDL
        << "**********************************************************************" << ENDL);
      m_starter_message_showed = true;
    }

    relay_txpool_transactions(); // txpool handles periodic DB checking
    m_check_updates_interval.do_call(boost::bind(&core::check_updates, this));
    m_check_disk_space_interval.do_call(boost::bind(&core::check_disk_space, this));
    m_block_rate_interval.do_call(boost::bind(&core::check_block_rate, this));
    m_blockchain_pruning_interval.do_call(boost::bind(&core::update_blockchain_pruning, this));
    m_diff_recalc_interval.do_call(boost::bind(&core::recalculate_difficulties, this));
    m_miner.on_idle();
    m_mempool.on_idle();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint8_t core::get_ideal_hard_fork_version() const
  {
    return get_blockchain_storage().get_ideal_hard_fork_version();
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
  uint64_t core::get_earliest_ideal_height_for_version(uint8_t version) const
  {
    return get_blockchain_storage().get_earliest_ideal_height_for_version(version);
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

    if (m_offline)
      return true;

    if (check_updates_level == UPDATES_DISABLED)
      return true;

    std::string version, hash;
    MCDEBUG("updates", "Checking for a new " << software << " version for " << buildtag);
    if (!tools::check_updates(software, buildtag, version, hash))
      return false;

    if (tools::vercmp(version.c_str(), MONERO_VERSION) <= 0)
    {
      m_update_available = false;
      return true;
    }

    std::string url = tools::get_update_url(software, subdir, buildtag, version, true);
    MCLOG_CYAN(el::Level::Info, "global", "Version " << version << " of " << software << " for " << buildtag << " is available: " << url << ", SHA256 hash " << hash);
    m_update_available = true;

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
      const std::string tmppath = path.string() + ".tmp";
      if (epee::file_io_utils::is_file_exist(tmppath))
      {
        MCDEBUG("updates", "We have part of the file already, resuming download");
      }
      m_last_update_length = 0;
      m_update_download = tools::download_async(tmppath, url, [this, hash, path](const std::string &tmppath, const std::string &uri, bool success) {
        bool remove = false, good = true;
        if (success)
        {
          crypto::hash file_hash;
          if (!tools::sha256sum(tmppath, file_hash))
          {
            MCERROR("updates", "Failed to hash " << tmppath);
            remove = true;
            good = false;
          }
          else if (hash != epee::string_tools::pod_to_hex(file_hash))
          {
            MCERROR("updates", "Download from " << uri << " does not match the expected hash");
            remove = true;
            good = false;
          }
        }
        else
        {
          MCERROR("updates", "Failed to download " << uri);
          good = false;
        }
        boost::unique_lock<boost::mutex> lock(m_update_mutex);
        m_update_download = 0;
        if (success && !remove)
        {
          std::error_code e = tools::replace_file(tmppath, path.string());
          if (e)
          {
            MCERROR("updates", "Failed to rename downloaded file");
            good = false;
          }
        }
        else if (remove)
        {
          if (!boost::filesystem::remove(tmppath))
          {
            MCERROR("updates", "Failed to remove invalid downloaded file");
            good = false;
          }
        }
        if (good)
          MCLOG_CYAN(el::Level::Info, "updates", "New version downloaded to " << path.string());
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
  bool core::check_disk_space()
  {
    uint64_t free_space = get_free_space();
    if (free_space < 1ull * 1024 * 1024 * 1024) // 1 GB
    {
      const el::Level level = el::Level::Warning;
      MCLOG_RED(level, "global", "Free space is below 1 GB on " << m_config_folder);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  double factorial(unsigned int n)
  {
    if (n <= 1)
      return 1.0;
    double f = n;
    while (n-- > 1)
      f *= n;
    return f;
  }
  //-----------------------------------------------------------------------------------------------
  static double probability1(unsigned int blocks, unsigned int expected)
  {
    // https://www.umass.edu/wsp/resources/poisson/#computing
    return pow(expected, blocks) / (factorial(blocks) * exp(expected));
  }
  //-----------------------------------------------------------------------------------------------
  static double probability(unsigned int blocks, unsigned int expected)
  {
    double p = 0.0;
    if (blocks <= expected)
    {
      for (unsigned int b = 0; b <= blocks; ++b)
        p += probability1(b, expected);
    }
    else if (blocks > expected)
    {
      for (unsigned int b = blocks; b <= expected * 3 /* close enough */; ++b)
        p += probability1(b, expected);
    }
    return p;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_block_rate()
  {
    if (m_offline || m_nettype == FAKECHAIN || m_target_blockchain_height > get_current_blockchain_height() || m_target_blockchain_height == 0)
    {
      MDEBUG("Not checking block rate, offline or syncing");
      return true;
    }

    static constexpr double threshold = 1. / (864000 / DIFFICULTY_TARGET_V2); // one false positive every 10 days
    static constexpr unsigned int max_blocks_checked = 150;

    const time_t now = time(NULL);
    const std::vector<time_t> timestamps = m_blockchain_storage.get_last_block_timestamps(max_blocks_checked);

    static const unsigned int seconds[] = { 5400, 3600, 1800, 1200, 600 };
    for (size_t n = 0; n < sizeof(seconds)/sizeof(seconds[0]); ++n)
    {
      unsigned int b = 0;
      const time_t time_boundary = now - static_cast<time_t>(seconds[n]);
      for (time_t ts: timestamps) b += ts >= time_boundary;
      const double p = probability(b, seconds[n] / DIFFICULTY_TARGET_V2);
      MDEBUG("blocks in the last " << seconds[n] / 60 << " minutes: " << b << " (probability " << p << ")");
      if (p < threshold)
      {
        MWARNING("There were " << b << (b == max_blocks_checked ? " or more" : "") << " blocks in the last " << seconds[n] / 60 << " minutes, there might be large hash rate changes, or we might be partitioned, cut off from the Monero network or under attack, or your computer's time is off. Or it could be just sheer bad luck.");

        std::shared_ptr<tools::Notify> block_rate_notify = m_block_rate_notify;
        if (block_rate_notify)
        {
          auto expected = seconds[n] / DIFFICULTY_TARGET_V2;
          block_rate_notify->notify("%t", std::to_string(seconds[n] / 60).c_str(), "%b", std::to_string(b).c_str(), "%e", std::to_string(expected).c_str(), NULL);
        }

        break; // no need to look further
      }
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::recalculate_difficulties()
  {
    m_blockchain_storage.recalculate_difficulties();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  void core::flush_bad_txs_cache()
  {
    bad_semantics_txes_lock.lock();
    for (int idx = 0; idx < 2; ++idx)
      bad_semantics_txes[idx].clear();
    bad_semantics_txes_lock.unlock();
  }
  //-----------------------------------------------------------------------------------------------
  void core::flush_invalid_blocks()
  {
    m_blockchain_storage.flush_invalid_blocks();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_txpool_complement(const std::vector<crypto::hash> &hashes, std::vector<cryptonote::blobdata> &txes)
  {
    return m_mempool.get_complement(hashes, txes);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::update_blockchain_pruning()
  {
    return m_blockchain_storage.update_blockchain_pruning();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_blockchain_pruning()
  {
    return m_blockchain_storage.check_blockchain_pruning();
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
  uint64_t core::prevalidate_block_hashes(uint64_t height, const std::vector<crypto::hash> &hashes, const std::vector<uint64_t> &weights)
  {
    return get_blockchain_storage().prevalidate_block_hashes(height, hashes, weights);
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_free_space() const
  {
    boost::filesystem::path path(m_config_folder);
    boost::filesystem::space_info si = boost::filesystem::space(path);
    return si.available;
  }
  //-----------------------------------------------------------------------------------------------
  uint32_t core::get_blockchain_pruning_seed() const
  {
    return get_blockchain_storage().get_blockchain_pruning_seed();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::prune_blockchain(uint32_t pruning_seed)
  {
    return get_blockchain_storage().prune_blockchain(pruning_seed);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::is_within_compiled_block_hash_area(uint64_t height) const
  {
    return get_blockchain_storage().is_within_compiled_block_hash_area(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::has_block_weights(uint64_t height, uint64_t nblocks) const
  {
    return get_blockchain_storage().has_block_weights(height, nblocks);
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
