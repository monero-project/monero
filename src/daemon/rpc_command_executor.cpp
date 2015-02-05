// Copyright (c) 2014, The Monero Project
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

#include "string_tools.h"
#include "common/scoped_message_writer.h"
#include "daemon/rpc_command_executor.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include <boost/format.hpp>
#include <ctime>

namespace daemonize {

namespace {
  void print_peer(std::string const & prefix, cryptonote::peer const & peer)
  {
    time_t now;
    time(&now);
    time_t last_seen = static_cast<time_t>(peer.last_seen);

    std::string id_str;
    std::string port_str;
    std::string elapsed = epee::misc_utils::get_time_interval_string(now - last_seen);
    std::string ip_str = epee::string_tools::get_ip_string_from_int32(peer.ip);
    epee::string_tools::xtype_to_string(peer.id, id_str);
    epee::string_tools::xtype_to_string(peer.port, port_str);
    std::string addr_str = ip_str + ":" + port_str;
    tools::msg_writer() << boost::format("%-10s %-25s %-25s %s") % prefix % id_str % addr_str % elapsed;
  }

  void print_block_header(cryptonote::block_header_responce const & header)
  {
    tools::success_msg_writer()
      << "timestamp: " << boost::lexical_cast<std::string>(header.timestamp) << std::endl
      << "previous hash: " << header.prev_hash << std::endl
      << "nonce: " << boost::lexical_cast<std::string>(header.nonce) << std::endl
      << "is orphan: " << header.orphan_status << std::endl
      << "height: " << boost::lexical_cast<std::string>(header.height) << std::endl
      << "depth: " << boost::lexical_cast<std::string>(header.depth) << std::endl
      << "hash: " << header.hash
      << "difficulty: " << boost::lexical_cast<std::string>(header.difficulty) << std::endl
      << "reward: " << boost::lexical_cast<std::string>(header.reward);
  }
}

t_rpc_command_executor::t_rpc_command_executor(
    uint32_t ip
  , uint16_t port
  )
  : m_rpc_client{ip, port}
{}

bool t_rpc_command_executor::print_peer_list() {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  bool ok = m_rpc_client.rpc_request(req, res, "/get_peer_list", "Couldn't retrieve peer list");

  if (!ok) return false;

  for (auto & peer : res.white_list)
  {
    print_peer("white", peer);
  }

  for (auto & peer : res.gray_list)
  {
    print_peer("gray", peer);
  }

  return true;
}

bool t_rpc_command_executor::save_blockchain() {
  cryptonote::COMMAND_RPC_SAVE_BC::request req;
  cryptonote::COMMAND_RPC_SAVE_BC::response res;

  if (m_rpc_client.rpc_request(req, res, "/save_bc", "Couldn't save blockchain"))
  {
    tools::success_msg_writer() << "Blockchain saved";
  }

  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = true;

  if (m_rpc_client.rpc_request(req, res, "/set_log_hash_rate", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Hash rate logging is on";
  }

  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = false;

  if (m_rpc_client.rpc_request(req, res, "/set_log_hash_rate", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Hash rate logging is off";
  }

  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;

  if (m_rpc_client.rpc_request(req, res, "/getinfo", "Problem fetching info"))
  {
    tools::success_msg_writer() <<   "BH: " << res.height
                                << ", DIFF: " << res.difficulty
                                << ", HR: " << (int) res.difficulty / 60L << " H/s";
  }

  return true;
}

bool t_rpc_command_executor::print_connections() {
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::request req;
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::response res;

  if (m_rpc_client.rpc_request(req, res, "/get_connections", "Unsuccessful"))
  {
    for (auto & info : res.connections)
    {
      std::string address = info.ip + ":" + info.port;
      std::string in_out = info.incoming ? "INC" : "OUT";
      tools::msg_writer() << boost::format("%-25s peer_id: %-25s %s") % address % info.peer_id % in_out;
    }
  }

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;

  req.start_height = start_block_index;
  req.end_height = end_block_index;

  if (m_rpc_client.json_rpc_request(req, res, "getblockheadersrange", "Unsuccessful"))
  {
    for (auto & header : res.headers)
    {
      std::cout
        << "major version: " << header.major_version << std::endl
        << "minor version: " << header.minor_version << std::endl
        << "height: " << header.height << ", timestamp: " << header.timestamp << ", difficulty: " << header.difficulty << std::endl
        << "block id: " << header.hash << std::endl
        << "previous block id: " << header.prev_hash << std::endl
        << "difficulty: " << header.difficulty << ", nonce " << header.nonce << std::endl;
    }
  }

  return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level) {
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;
  req.level = level;

  if (m_rpc_client.rpc_request(req, res, "/set_log_level", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Log level is now " << boost::lexical_cast<std::string>(level);
  }

  return true;
}

bool t_rpc_command_executor::print_height() {
  cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

  if (m_rpc_client.rpc_request(req, res, "/getheight", "Unsuccessful"))
  {
    tools::success_msg_writer() << boost::lexical_cast<std::string>(res.height);
  }

  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response res;

  req.hash = epee::string_tools::pod_to_hex(block_hash);

  if (m_rpc_client.json_rpc_request(req, res, "getblockheaderbyhash", "Unsuccessful"))
  {
    print_block_header(res.block_header);
  }

  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response res;

  req.height = height;

  if (m_rpc_client.json_rpc_request(req, res, "getblockheaderbyheight", "Unsuccessful"))
  {
    print_block_header(res.block_header);
  }

  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  if (m_rpc_client.rpc_request(req, res, "/gettransactions", "Problem fetching transaction"))
  {
    if (1 == res.txs_as_hex.size())
    {
      tools::success_msg_writer() << res.txs_as_hex.front();
    }
    else
    {
      tools::fail_msg_writer() << "transaction wasn't found: <" << transaction_hash << '>' << std::endl;
    }
  }

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_long() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  if (m_rpc_client.rpc_request(req, res, "/get_transaction_pool", "Problem fetching transaction pool"))
  {
    if (res.transactions.empty())
    {
      tools::msg_writer() << "Pool is empty" << std::endl;
    }
    for (auto & tx_info : res.transactions)
    {
      tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                          << "blob_size: " << tx_info.blob_size << std::endl
                          << "fee: " << tx_info.fee << std::endl
                          << "kept_by_block: " << tx_info.kept_by_block << std::endl
                          << "max_used_block_height: " << tx_info.max_used_block_height << std::endl
                          << "max_used_block_id: " << tx_info.max_used_block_id_hash << std::endl
                          << "last_failed_height: " << tx_info.last_failed_height << std::endl
                          << "last_failed_id: " << tx_info.last_failed_id_hash << std::endl;
    }
  }

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_short() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  if (m_rpc_client.rpc_request(req, res, "/get_transaction_pool", "Problem fetching transaction pool"))
  {
    for (auto & tx_info : res.transactions)
    {
      if (res.transactions.empty())
      {
        tools::msg_writer() << "Pool is empty" << std::endl;
      }
      tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                          <<  tx_info.tx_json << std::endl
                          << "blob_size: " << tx_info.blob_size << std::endl
                          << "fee: " << tx_info.fee << std::endl
                          << "kept_by_block: " << tx_info.kept_by_block << std::endl
                          << "max_used_block_height: " << tx_info.max_used_block_height << std::endl
                          << "max_used_block_id: " << tx_info.max_used_block_id_hash << std::endl
                          << "last_failed_height: " << tx_info.last_failed_height << std::endl
                          << "last_failed_id: " << tx_info.last_failed_id_hash << std::endl;
    }
  }

  return true;
}

// TODO: update this for testnet
bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(false, address);
  req.threads_count = num_threads;

  if (m_rpc_client.rpc_request(req, res, "/start_mining", "Mining did not start"))
  {
    tools::success_msg_writer() << "Mining started";
  }
  return true;
}

bool t_rpc_command_executor::stop_mining() {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  if (m_rpc_client.rpc_request(req, res, "/stop_mining", "Mining did not stop"))
  {
    tools::success_msg_writer() << "Mining stopped";
  }
  return true;
}

bool t_rpc_command_executor::stop_daemon()
{
  cryptonote::COMMAND_RPC_STOP_DAEMON::request req;
  cryptonote::COMMAND_RPC_STOP_DAEMON::response res;

//# ifdef WIN32
//    // Stop via service API
//    // TODO - this is only temporary!  Get rid of hard-coded constants!
//    bool ok = windows::stop_service("BitMonero Daemon");
//    ok = windows::uninstall_service("BitMonero Daemon");
//    //bool ok = windows::stop_service(SERVICE_NAME);
//    //ok = windows::uninstall_service(SERVICE_NAME);
//    if (ok)
//    {
//      return true;
//    }
//# endif

  // Stop via RPC
  if(m_rpc_client.rpc_request(req, res, "/stop_daemon", "Daemon did not stop"))
  {
    tools::success_msg_writer() << "Stop signal sent";
  }

  return true;
}

bool t_rpc_command_executor::print_status()
{
  bool daemon_is_alive = m_rpc_client.check_connection();

  if(daemon_is_alive) {
    tools::success_msg_writer() << "bitmonerod is running";
  }
  else {
    tools::fail_msg_writer() << "bitmonerod is NOT running";
  }

  return true;
}

bool t_rpc_command_executor::set_limit(int limit)
{
/*
    epee::net_utils::connection_basic::set_rate_down_limit( limit );
    epee::net_utils::connection_basic::set_rate_up_limit( limit );
    std::cout << "Set limit-down to " << limit/1024 << " kB/s" << std::endl;
    std::cout << "Set limit-up to " << limit/1024 << " kB/s" << std::endl;
*/

    return true;
}

bool t_rpc_command_executor::set_limit_up(int limit)
{
/*
    epee::net_utils::connection_basic::set_rate_up_limit( limit );
    std::cout << "Set limit-up to " << limit/1024 << " kB/s" << std::endl;
*/

    return true;
}

bool t_rpc_command_executor::set_limit_down(int limit)
{
/*
    epee::net_utils::connection_basic::set_rate_down_limit( limit );
    std::cout << "Set limit-down to " << limit/1024 << " kB/s" << std::endl;
*/

    return true;
}

}// namespace daemonize
