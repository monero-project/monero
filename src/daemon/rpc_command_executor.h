/**
@file
@details

@image html images/other/runtime-commands.png

*/

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

#pragma once

#include "common/rpc_client.h"
#include "misc_log_ex.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "p2p/net_node.h"
#include "rpc/core_rpc_server.h"

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
    , const std::string &user_agent
    , bool is_rpc = true
    , cryptonote::core_rpc_server* rpc_server = NULL
    );

  ~t_rpc_command_executor();

  bool print_peer_list();

  bool save_blockchain();

  bool show_hash_rate();

  bool hide_hash_rate();

  bool show_difficulty();

  bool show_status();

  bool print_connections();

  bool print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index);

  bool set_log_level(int8_t level);

  bool print_height();

  bool print_block_by_hash(crypto::hash block_hash);

  bool print_block_by_height(uint64_t height);

  bool print_transaction(crypto::hash transaction_hash);

  bool is_key_image_spent(const crypto::key_image &ki);

  bool print_transaction_pool_long();

  bool print_transaction_pool_short();

  bool print_transaction_pool_stats();

  bool start_mining(cryptonote::account_public_address address, uint64_t num_threads, bool testnet);

  bool stop_mining();

  bool stop_daemon();

  bool print_status();

  bool get_limit();

  bool get_limit_up();

  bool get_limit_down();

  bool set_limit(int limit);

  bool set_limit_up(int limit);

  bool set_limit_down(int limit);

  bool out_peers(uint64_t limit);
  
  bool start_save_graph();
  
  bool stop_save_graph();
  
  bool hard_fork_info(uint8_t version);

  bool print_bans();

  bool ban(const std::string &ip, time_t seconds);

  bool unban(const std::string &ip);

  bool flush_txpool(const std::string &txid);

  bool output_histogram(uint64_t min_count, uint64_t max_count);

  bool print_coinbase_tx_sum(uint64_t height, uint64_t count);
};

} // namespace daemonize
