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
  , bool is_rpc
  , cryptonote::core_rpc_server* rpc_server
  )
  : m_rpc_client(NULL), m_rpc_server(rpc_server)
{
  if (is_rpc)
  {
    m_rpc_client = new tools::t_rpc_client(ip, port);
  }
  else
  {
    if (rpc_server == NULL)
    {
      throw std::runtime_error("If not calling commands via RPC, rpc_server pointer must be non-null");
    }
  }

  m_is_rpc = is_rpc;
}

t_rpc_command_executor::~t_rpc_command_executor()
{
  if (m_rpc_client != NULL)
  {
    delete m_rpc_client;
  }
}

bool t_rpc_command_executor::print_peer_list() {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  std::string failure_message = "Couldn't retrieve peer list";
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_peer_list", failure_message.c_str()))
    {
      return false;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_peer_list(req, res))
    {
      tools::fail_msg_writer() << failure_message;
      return false;
    }
  }

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

  std::string fail_message = "Couldn't save blockchain";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/save_bc", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_save_bc(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
    }
  }

  tools::success_msg_writer() << "Blockchain saved";

  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = true;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_hash_rate(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
    }
  }

  tools::success_msg_writer() << "Hash rate logging is on";

  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = false;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_hash_rate(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() << "Hash rate logging is off";

  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;

  std::string fail_message = "Problem fetching info";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_info(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() <<   "BH: " << res.height
                              << ", DIFF: " << res.difficulty
                              << ", HR: " << (int) res.difficulty / 60L << " H/s";

  return true;
}

bool t_rpc_command_executor::print_connections() {
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::request req;
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::response res;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "/get_connections", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_connections(req, res, error_resp))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  for (auto & info : res.connections)
  {
    std::string address = info.ip + ":" + info.port;
    std::string in_out = info.incoming ? "INC" : "OUT";
    tools::msg_writer() << boost::format("%-25s peer_id: %-25s %s") % address % info.peer_id % in_out;
  }

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {

// this function appears to not exist in the json rpc api, and so is commented
// until such a time as it does.

/*
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;
  epee::json_rpc::error error_resp;

  req.start_height = start_block_index;
  req.end_height = end_block_index;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblockheadersrange", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_getblockheadersrange(req, res, error_resp))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

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

*/
  return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level) {
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;
  req.level = level;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_level", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_level(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() << "Log level is now " << boost::lexical_cast<std::string>(level);

  return true;
}

bool t_rpc_command_executor::print_height() {
  cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/getheight", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_height(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() << boost::lexical_cast<std::string>(res.height);

  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response res;
  epee::json_rpc::error error_resp;

  req.hash = epee::string_tools::pod_to_hex(block_hash);

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblockheaderbyhash", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_block_header_by_hash(req, res, error_resp))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  print_block_header(res.block_header);

  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response res;
  epee::json_rpc::error error_resp;

  req.height = height;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblockheaderbyheight", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_block_header_by_height(req, res, error_resp))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  print_block_header(res.block_header);

  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  std::string fail_message = "Problem fetching transaction";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/gettransactions", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transactions(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  if (1 == res.txs_as_hex.size())
  {
    tools::success_msg_writer() << res.txs_as_hex.front();
  }
  else
  {
    tools::fail_msg_writer() << "transaction wasn't found: <" << transaction_hash << '>' << std::endl;
  }

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_long() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transaction_pool(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

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

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_short() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transaction_pool(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  if (res.transactions.empty())
  {
    tools::msg_writer() << "Pool is empty" << std::endl;
  }
  for (auto & tx_info : res.transactions)
  {
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

  return true;
}

// TODO: update this for testnet
bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(false, address);
  req.threads_count = num_threads;

  if (m_rpc_client->rpc_request(req, res, "/start_mining", "Mining did not start"))
  {
    tools::success_msg_writer() << "Mining started";
  }
  return true;
}

bool t_rpc_command_executor::stop_mining() {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  std::string fail_message = "Mining did not stop";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/stop_mining", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_stop_mining(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() << "Mining stopped";
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
  std::string fail_message = "Daemon did not stop";

  if (m_is_rpc)
  {
    if(!m_rpc_client->rpc_request(req, res, "/stop_daemon", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_stop_daemon(req, res))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  tools::success_msg_writer() << "Stop signal sent";

  return true;
}

bool t_rpc_command_executor::print_status()
{
  if (!m_is_rpc)
  {
    tools::success_msg_writer() << "print_status makes no sense in interactive mode";
    return true;
  }

  bool daemon_is_alive = m_rpc_client->check_connection();

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
