#pragma once

#include "net/http_client.h"
#include "misc_log_ex.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/command_executor.h"
#include "p2p/net_node.h"
#include <boost/optional.hpp>

namespace daemonize {

class t_rpc_command_executor final : public t_command_executor {
private:
  epee::net_utils::http::http_simple_client m_http_client = {};
  uint32_t m_rpc_host_ip;
  uint16_t m_rpc_host_port;
public:
  static t_rpc_command_executor * parse_host_and_create(
      std::string rpc_host_ip_str, std::string rpc_host_port_str);

  t_rpc_command_executor(uint32_t rpc_host_ip, uint16_t rpc_host_port);

  bool print_peer_list() override;

  bool save_blockchain() override;

  bool show_hash_rate() override;

  bool hide_hash_rate() override;

  bool show_difficulty() override;

  bool print_connections() override;

  bool print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) override;

  bool set_log_level(uint16_t level) override;

  bool print_block_by_hash(crypto::hash block_hash) override;

  bool print_block_by_height(uint64_t height) override;

  bool print_transaction(crypto::hash transaction_hash) override;

  bool print_transaction_pool_long() override;

  bool print_transaction_pool_short() override;

  bool start_mining(cryptonote::account_public_address address, size_t num_threads) override;

  bool stop_mining() override;
};

} // namespace daemonize
