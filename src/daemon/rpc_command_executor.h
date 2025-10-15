/**
@file
@details

@image html images/other/runtime-commands.png

*/

// Copyright (c) 2014-2025, The Monero Project
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

#pragma once

#include <boost/optional/optional_fwd.hpp>

#include "common/common_fwd.h"
#include "common/rpc_client.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "rpc/core_rpc_server.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize {

class t_rpc_command_executor final {
private:
  tools::t_rpc_client* m_rpc_client;
  cryptonote::core_rpc_server* m_rpc_server;
  bool m_is_rpc;

public:
  t_rpc_command_executor(
      uint32_t ip
    , uint16_t port
    , const boost::optional<tools::login>& user
    , const epee::net_utils::ssl_options_t& ssl_options
    , bool is_rpc = true
    , cryptonote::core_rpc_server* rpc_server = NULL
    );

  ~t_rpc_command_executor();

  bool print_peer_list(bool white = true, bool gray = true, size_t limit = 0, bool pruned_only = false, bool publicrpc_only = false);

  bool print_peer_list_stats();

  bool save_blockchain();

  bool show_hash_rate();

  bool hide_hash_rate();

  bool show_difficulty();

  bool show_status();

  bool print_connections();

  bool print_blockchain_info(int64_t start_block_index, uint64_t end_block_index);

  bool set_log_level(int8_t level);

  bool set_log_categories(const std::string &categories);

  bool print_height();

  bool print_block_by_hash(crypto::hash block_hash, bool include_hex);

  bool print_block_by_height(uint64_t height, bool include_hex);

  bool print_transaction(crypto::hash transaction_hash, bool include_metadata, bool include_hex, bool include_json);

  bool is_key_image_spent(const crypto::key_image &ki);

  bool print_transaction_pool_long();

  bool print_transaction_pool_short();

  bool print_transaction_pool_stats();

  bool start_mining(cryptonote::account_public_address address, uint64_t num_threads, cryptonote::network_type nettype, bool do_background_mining = false, bool ignore_battery = false);

  bool stop_mining();

  bool mining_status();

  bool stop_daemon();

  bool print_status();

  bool get_limit();

  bool get_limit_up();

  bool get_limit_down();

  bool set_limit(int64_t limit_down, int64_t limit_up);

  bool out_peers(bool set, uint32_t limit);

  bool in_peers(bool set, uint32_t limit);

  bool hard_fork_info(uint8_t version);

  bool print_bans();

  bool ban(const std::string &address, time_t seconds);

  bool unban(const std::string &address);

  bool banned(const std::string &address);

  bool flush_txpool(const std::string &txid);

  bool output_histogram(const std::vector<uint64_t> &amounts, uint64_t min_count, uint64_t max_count);

  bool print_coinbase_tx_sum(uint64_t height, uint64_t count);

  bool alt_chain_info(const std::string &tip, size_t above, uint64_t last_blocks);

  bool print_blockchain_dynamic_stats(uint64_t nblocks);

  bool update(const std::string &command);

  bool relay_tx(const std::string &txid);

  bool sync_info();

  bool pop_blocks(uint64_t num_blocks);

  bool prune_blockchain();

  bool check_blockchain_pruning();

  bool print_net_stats();

  bool version();

  bool set_bootstrap_daemon(
    const std::string &address,
    const std::string &username,
    const std::string &password,
    const std::string &proxy);

  bool rpc_payments();

  bool flush_cache(bool invalid_blocks);
};

} // namespace daemonize
