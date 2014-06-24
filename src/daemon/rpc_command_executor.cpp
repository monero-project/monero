#include "string_tools.h"
#include "storages/http_abstract_invoke.h"
#include "common/scoped_message_writer.h"
#include "daemon/rpc_command_executor.h"
#include "rpc/core_rpc_server_commands_defs.h"

using namespace daemonize;

t_rpc_command_executor::t_constructor_args t_rpc_command_executor::parse_host(
    std::string rpc_host_ip_str, std::string rpc_host_port_str)
{
  t_constructor_args args;

  bool ip_ok = epee::string_tools::get_ip_int32_from_string(args.rpc_host_ip, rpc_host_ip_str);
  bool port_ok = epee::string_tools::get_xtype_from_string(args.rpc_host_port, rpc_host_port_str);
  args.ok = ip_ok && port_ok;

  return args;
}

t_rpc_command_executor::t_rpc_command_executor(t_constructor_args const & args) :
    mp_http_client(new epee::net_utils::http::http_simple_client())
  , m_rpc_host_ip(args.rpc_host_ip)
  , m_rpc_host_port(args.rpc_host_port)
{}

t_rpc_command_executor::t_rpc_command_executor(t_rpc_command_executor && other) = default;

bool t_rpc_command_executor::print_peer_list() {
  std::cout << "print peer list" << std::endl;
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
  std::cout << "show difficulty" << std::endl;
  return true;
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
  req.miner_address = cryptonote::get_account_address_as_str(address);
  req.threads_count = num_threads;

  std::string daemon_ip = epee::string_tools::get_ip_string_from_int32(m_rpc_host_ip);
  std::string daemon_port = std::to_string(m_rpc_host_port);
  std::string rpc_url = "http://" + daemon_ip + ":" + daemon_port + "/start_mining";

  cryptonote::COMMAND_RPC_START_MINING::response res;
  bool ok = epee::net_utils::invoke_http_json_remote_command2(rpc_url, req, res, *mp_http_client);
  if (!ok)
  {
    tools::fail_msg_writer() << "Couldn't connect to daemon.  Is it running?";
  }
  else if (res.status == CORE_RPC_STATUS_OK)
  {
    tools::success_msg_writer() << "Mining started!";
  }
  else
  {
    tools::fail_msg_writer() << "Mining has NOT been started: " << res.status;
  }

  return true;
}

bool t_rpc_command_executor::stop_mining() {
  std::cout << "stop mining" << std::endl;
  return true;
}
