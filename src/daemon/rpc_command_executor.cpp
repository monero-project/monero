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

    std::string id_str;
    std::string port_str;
    std::string elapsed = epee::misc_utils::get_time_interval_string(now - peer.last_seen);
    std::string ip_str = epee::string_tools::get_ip_string_from_int32(peer.ip);
    epee::string_tools::xtype_to_string(peer.id, id_str);
    epee::string_tools::xtype_to_string(peer.port, port_str);
    std::string addr_str = ip_str + ":" + port_str;
    tools::msg_writer() << boost::format("%-10s %-25s %-25s %s") % prefix % id_str % addr_str % elapsed;
  }
}

t_rpc_command_executor::t_host_result t_rpc_command_executor::parse_host(
    std::string const & rpc_host_ip_str, std::string const & rpc_host_port_str)
{
  t_host_result result;

  bool ip_ok = epee::string_tools::get_ip_int32_from_string(result.rpc_host_ip, rpc_host_ip_str);
  bool port_ok = epee::string_tools::get_xtype_from_string(result.rpc_host_port, rpc_host_port_str);
  result.ok = ip_ok && port_ok;

  return result;
}

t_rpc_command_executor::t_rpc_command_executor(std::string && rpc_host_ip_str, std::string && rpc_host_port_str)
  : mp_http_client(new epee::net_utils::http::http_simple_client())
  , m_rpc_host_ip_str(std::move(rpc_host_ip_str))
  , m_rpc_host_port_str(std::move(rpc_host_port_str))
{}

t_rpc_command_executor::t_rpc_command_executor(t_rpc_command_executor && other) = default;

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
  std::cout << "save blockchain" << std::endl;
  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  std::cout << "show hash rate" << std::endl;
  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  std::cout << "hide hash rate" << std::endl;
  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;

  bool ok = rpc_request(req, res, "/getinfo", "Problem fetching info");
  if (ok)
  {
    tools::success_msg_writer() <<   "BH: " << res.height
                                << ", DIFF: " << res.difficulty
                                << ", HR: " << (int) res.difficulty / 60L << " H/s";
  }
  return ok;
}

bool t_rpc_command_executor::print_connections() {
  std::cout << "print connections" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
  std::cout << "print blockchain info" << std::endl;
  return true;
}

bool t_rpc_command_executor::set_log_level(uint16_t level) {
  std::cout << "set log level" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash) {
  std::cout << "print block by hash" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  std::cout << "print block by height" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  std::cout << "print transaction" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_transaction_pool_long() {
  std::cout << "print transaction pool long" << std::endl;
  return true;
}

bool t_rpc_command_executor::print_transaction_pool_short() {
  std::cout << "print transaction pool short" << std::endl;
  return true;
}

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(address);
  req.threads_count = num_threads;

  bool ok = rpc_request(req, res, "/start_mining", "Mining did not start");
  if (ok) tools::success_msg_writer() << "Mining started";
  return ok;
}

bool t_rpc_command_executor::stop_mining() {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  bool ok = rpc_request(req, res, "/stop_mining", "Mining did not stop");
  if (ok) tools::success_msg_writer() << "Mining stopped";
  return ok;
}

template <typename T_req, typename T_res>
bool t_rpc_command_executor::rpc_request(
    T_req & req
  , T_res & res
  , std::string const & relative_url
  , std::string const & fail_msg
  )
{
  std::string rpc_url = "http://" + m_rpc_host_ip_str + ":" + m_rpc_host_port_str + relative_url;
  t_http_connection connection(mp_http_client.get(), m_rpc_host_ip_str, m_rpc_host_port_str);

  bool ok = connection.is_open();
  ok = ok && epee::net_utils::invoke_http_json_remote_command2(rpc_url, req, res, *mp_http_client);
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
