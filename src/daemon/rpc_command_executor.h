#pragma once

#include "net/http_client.h"
#include "misc_log_ex.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "p2p/net_node.h"

namespace daemonize {

class t_rpc_command_executor final {
private:
  std::unique_ptr<epee::net_utils::http::http_simple_client> mp_http_client;
  uint32_t m_rpc_host_ip;
  uint16_t m_rpc_host_port;
public:
  struct t_constructor_args {
    bool ok;
    uint32_t rpc_host_ip;
    uint16_t rpc_host_port;
  };

  static t_constructor_args parse_host(
      std::string rpc_host_ip_str, std::string rpc_host_port_str);

  t_rpc_command_executor(t_constructor_args const & args);

  t_rpc_command_executor(t_rpc_command_executor && other);

  bool print_peer_list();

  bool save_blockchain();

  bool show_hash_rate();

  bool hide_hash_rate();

  bool show_difficulty();

  bool print_connections();

  bool print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index);

  bool set_log_level(uint16_t level);

  bool print_block_by_hash(crypto::hash block_hash);

  bool print_block_by_height(uint64_t height);

  bool print_transaction(crypto::hash transaction_hash);

  bool print_transaction_pool_long();

  bool print_transaction_pool_short();

  bool start_mining(cryptonote::account_public_address address, size_t num_threads);

  bool stop_mining();
};

} // namespace daemonize
