/**
@file
@details

@image html images/other/runtime-commands.png

*/

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

#pragma once

#include <boost/optional/optional.hpp>

#include "daemon/rpc_command_executor.h"
#include "common/common_fwd.h"
#include "rpc/core_rpc_server.h"

namespace daemonize {

class t_command_parser_executor final
{
private:
  t_rpc_command_executor m_executor;
public:
  t_command_parser_executor(
      uint32_t ip
    , uint16_t port
    , const boost::optional<tools::login>& login
    , bool is_rpc
    , cryptonote::core_rpc_server* rpc_server = NULL
    );

  bool print_peer_list(const std::vector<std::string>& args);

  bool print_peer_list_stats(const std::vector<std::string>& args);

  bool save_blockchain(const std::vector<std::string>& args);

  bool show_hash_rate(const std::vector<std::string>& args);

  bool hide_hash_rate(const std::vector<std::string>& args);

  bool show_difficulty(const std::vector<std::string>& args);

  bool show_status(const std::vector<std::string>& args);

  bool print_connections(const std::vector<std::string>& args);

  bool print_blockchain_info(const std::vector<std::string>& args);

  bool set_log_level(const std::vector<std::string>& args);

  bool set_log_categories(const std::vector<std::string>& args);

  bool print_height(const std::vector<std::string>& args);

  bool print_block(const std::vector<std::string>& args);

  bool print_transaction(const std::vector<std::string>& args);

  bool is_key_image_spent(const std::vector<std::string>& args);

  bool print_transaction_pool_long(const std::vector<std::string>& args);

  bool print_transaction_pool_short(const std::vector<std::string>& args);

  bool print_transaction_pool_stats(const std::vector<std::string>& args);

  bool start_mining(const std::vector<std::string>& args);

  bool stop_mining(const std::vector<std::string>& args);

  bool stop_daemon(const std::vector<std::string>& args);

  bool print_status(const std::vector<std::string>& args);

  bool set_limit(const std::vector<std::string>& args);

  bool set_limit_up(const std::vector<std::string>& args);

  bool set_limit_down(const std::vector<std::string>& args);

  bool out_peers(const std::vector<std::string>& args);

  bool in_peers(const std::vector<std::string>& args);

  bool start_save_graph(const std::vector<std::string>& args);
  
  bool stop_save_graph(const std::vector<std::string>& args);
  
  bool hard_fork_info(const std::vector<std::string>& args);

  bool show_bans(const std::vector<std::string>& args);

  bool ban(const std::vector<std::string>& args);

  bool unban(const std::vector<std::string>& args);

  bool flush_txpool(const std::vector<std::string>& args);

  bool output_histogram(const std::vector<std::string>& args);

  bool print_coinbase_tx_sum(const std::vector<std::string>& args);

  bool alt_chain_info(const std::vector<std::string>& args);

  bool print_blockchain_dynamic_stats(const std::vector<std::string>& args);

  bool update(const std::vector<std::string>& args);

  bool relay_tx(const std::vector<std::string>& args);

  bool sync_info(const std::vector<std::string>& args);

  bool version(const std::vector<std::string>& args);
};

} // namespace daemonize
