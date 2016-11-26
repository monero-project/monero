// Copyright (c) 2014-2016, The Monero Project
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

#include "command_line.h"
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "common/i18n.h"
#include "cryptonote_config.h"
#include "string_tools.h"

namespace command_line
{
  namespace
  {
    const char* tr(const char* str)
    {
      return i18n_translate(str, "command_line");
    }
  }

  std::string input_line(const std::string& prompt)
  {
    std::cout << prompt;

    std::string buf;
    std::getline(std::cin, buf);

    return epee::string_tools::trim(buf);

  }

  bool is_yes(const std::string& str)
  {
    if (str == "y" || str == "Y")
      return true;

    boost::algorithm::is_iequal ignore_case{};
    if (boost::algorithm::equals("yes", str, ignore_case))
      return true;
    if (boost::algorithm::equals(command_line::tr("yes"), str, ignore_case))
      return true;

    return false;
  }

  const arg_descriptor<bool> arg_help = {"help", "Produce help message"};
  const arg_descriptor<bool> arg_version = {"version", "Output version information"};
  const arg_descriptor<std::string> arg_data_dir = {"data-dir", "Specify data directory"};
  const arg_descriptor<std::string> arg_testnet_data_dir = {"testnet-data-dir", "Specify testnet data directory"};
  const arg_descriptor<std::string> arg_user_agent = {"user-agent", "Restrict RPC use to clients using this user agent"};
  const arg_descriptor<bool>		arg_test_drop_download  		= {"test-drop-download", "For net tests: in download, discard ALL blocks instead checking/saving them (very fast)"};
  const arg_descriptor<uint64_t>	arg_test_drop_download_height  	= {"test-drop-download-height", "Like test-drop-download but disards only after around certain height", 0};
  const arg_descriptor<int> 		arg_test_dbg_lock_sleep = {"test-dbg-lock-sleep", "Sleep time in ms, defaults to 0 (off), used to debug before/after locking mutex. Values 100 to 1000 are good for tests."};
  const arg_descriptor<bool, false> arg_testnet_on  = {
    "testnet"
  , "Run on testnet. The wallet must be launched with --testnet flag."
  , false
  };
  const arg_descriptor<bool> arg_dns_checkpoints  = {
    "enforce-dns-checkpointing"
  , "checkpoints from DNS server will be enforced"
  , false
  };
  const command_line::arg_descriptor<std::string> arg_db_type = {
    "db-type"
  , "Specify database type"
  , DEFAULT_DB_TYPE
  };
  const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
    "db-sync-mode"
  , "Specify sync option, using format [safe|fast|fastest]:[sync|async]:[nblocks_per_sync]." 
  , "fast:async:1000"
  };
  const command_line::arg_descriptor<uint64_t> arg_fast_block_sync = {
    "fast-block-sync"
  , "Sync up most of the way by using embedded, known block hashes."
  , 1
  };
  const command_line::arg_descriptor<uint64_t> arg_prep_blocks_threads = {
    "prep-blocks-threads"
  , "Max number of threads to use when preparing block hashes in groups."
  , 4
  };
  const command_line::arg_descriptor<uint64_t> arg_db_auto_remove_logs  = {
    "db-auto-remove-logs"
  , "For BerkeleyDB only. Remove transactions logs automatically."
  , 1
  };
  const command_line::arg_descriptor<uint64_t> arg_show_time_stats  = {
    "show-time-stats"
  , "Show time-stats when processing blocks/txs and disk synchronization."
  , 0
  };
  const command_line::arg_descriptor<size_t> arg_block_sync_size  = {
    "block-sync-size"
  , "How many blocks to sync at once during chain synchronization."
  , BLOCKS_SYNCHRONIZING_DEFAULT_COUNT
  };
}
