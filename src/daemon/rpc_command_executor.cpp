#include "string_tools.h"
#include "storages/http_abstract_invoke.h"
#include "common/scoped_message_writer.h"
#include "daemon/http_connection.h"
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

  std::string build_url(uint32_t host, uint16_t port, std::string const & relative_url)
  {
    std::string result =
            "http://"
           + boost::lexical_cast<std::string>(host)
           + ":"
           + boost::lexical_cast<std::string>(port)
           + relative_url;
    return result;
  }
}

t_rpc_command_executor::t_rpc_command_executor(
    uint32_t ip
  , uint16_t port
  )
  : m_http_client()
  , m_ip(ip)
  , m_port(port)
{}

bool t_rpc_command_executor::print_peer_list() {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  bool ok = rpc_request(req, res, "/get_peer_list", "Couldn't retrieve peer list");

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

  if (rpc_request(req, res, "/save_bc", "Couldn't save blockchain"))
  {
    tools::success_msg_writer() << "Blockchain saved";
  }

  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = true;

  if (rpc_request(req, res, "/set_log_hash_rate", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Hash rate logging is on";
  }

  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = false;

  if (rpc_request(req, res, "/set_log_hash_rate", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Hash rate logging is off";
  }

  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;

  if (rpc_request(req, res, "/getinfo", "Problem fetching info"))
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

  if (rpc_request(req, res, "/get_connections", "Unsuccessful"))
  {
    for (auto & info : res.connections)
    {
      std::string address = epee::string_tools::get_ip_string_from_int32(info.ip) + ":" + boost::lexical_cast<std::string>(info.port);
      std::string in_out = info.is_incoming ? "INC" : "OUT";
      tools::msg_writer() << boost::format("%-25s peer_id: %-25s conn_id: %-25s %s") % address % info.peer_id % info.uuid % in_out;
    }
  }

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;

  req.start_height = start_block_index;
  req.end_height = end_block_index;

  if (json_rpc_request(req, res, "getblockheadersrange", "Unsuccessful"))
  {
    for (auto & header : res.headers)
    {
      std::cout << "height " << header.height
                << ", timestamp " << header.timestamp
                << ", cumul_dif " << header.cumulative_difficulty
                << ", cumul_size " << header.cumulative_size << std::endl
                << "id " << header.hash << std::endl
                << "difficulty " << header.difficulty
                << ", nonce " << header.nonce
                << ", tx_count " << header.tx_count << std::endl;
    }
  }

  return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level) {
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;
  req.level = level;

  if (rpc_request(req, res, "/set_log_level", "Unsuccessful"))
  {
    tools::success_msg_writer() << "Log level is now " << boost::lexical_cast<std::string>(level);
  }

  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response res;

  req.hash = epee::string_tools::pod_to_hex(block_hash);

  if (json_rpc_request(req, res, "getblockheaderbyhash", "Unsuccessful"))
  {
    print_block_header(res.block_header);
  }

  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response res;

  req.height = height;

  if (json_rpc_request(req, res, "getblockheaderbyheight", "Unsuccessful"))
  {
    print_block_header(res.block_header);
  }

  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  if (rpc_request(req, res, "/gettransactions", "Problem fetching transaction"))
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

  if (rpc_request(req, res, "/get_transaction_pool", "Problem fetching transaction pool"))
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

  if (rpc_request(req, res, "/get_transaction_pool", "Mining did not start"))
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

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(address);
  req.threads_count = num_threads;

  if (rpc_request(req, res, "/start_mining", "Mining did not start"))
  {
    tools::success_msg_writer() << "Mining started";
  }
  return true;
}

bool t_rpc_command_executor::stop_mining() {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  if (rpc_request(req, res, "/stop_mining", "Mining did not stop"))
  {
    tools::success_msg_writer() << "Mining stopped";
  }
  return true;
}

bool t_rpc_command_executor::stop_daemon()
{
  cryptonote::COMMAND_RPC_STOP_DAEMON::request req;
  cryptonote::COMMAND_RPC_STOP_DAEMON::response res;

  if (rpc_request(req, res, "/stop_daemon", "Daemon did not stop"))
  {
    tools::success_msg_writer() << "Stop signal sent";
  }
  return true;
}

template <typename T_req, typename T_res>
bool t_rpc_command_executor::json_rpc_request(
    T_req & req
  , T_res & res
  , std::string const & method_name
  , std::string const & fail_msg
  )
{
  std::string rpc_url = build_url(m_ip, m_port, "/json_rpc");
  t_http_connection connection(&m_http_client, m_ip, m_port);

  bool ok = connection.is_open();
  ok = ok && epee::net_utils::invoke_http_json_rpc(rpc_url, method_name, req, res, m_http_client);
  if (!ok)
  {
    tools::fail_msg_writer() << "Couldn't connect to daemon";
    return false;
  }
  else if (res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
  {
    tools::fail_msg_writer() << fail_msg << " -- " << res.status;
    return false;
  }
  else
  {
    return true;
  }
}

template <typename T_req, typename T_res>
bool t_rpc_command_executor::rpc_request(
    T_req & req
  , T_res & res
  , std::string const & relative_url
  , std::string const & fail_msg
  )
{
  std::string rpc_url = build_url(m_ip, m_port, relative_url);
  t_http_connection connection(&m_http_client, m_ip, m_port);

  bool ok = connection.is_open();
  ok = ok && epee::net_utils::invoke_http_json_remote_command2(rpc_url, req, res, m_http_client);
  if (!ok)
  {
    tools::fail_msg_writer() << "Couldn't connect to daemon";
    return false;
  }
  else if (res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
  {
    tools::fail_msg_writer() << fail_msg << " -- " << res.status;
    return false;
  }
  else
  {
    return true;
  }
}

} // namespace daemonize
