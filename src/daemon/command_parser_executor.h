#pragma once

#include "daemon/rpc_command_executor.h"

namespace daemonize {

class t_command_parser_executor final
{
private:
  t_rpc_command_executor m_executor;
public:
  t_command_parser_executor(
      uint32_t ip
    , uint16_t port
    );

  bool print_peer_list(const std::vector<std::string>& args);

  bool save_blockchain(const std::vector<std::string>& args);

  bool show_hash_rate(const std::vector<std::string>& args);

  bool hide_hash_rate(const std::vector<std::string>& args);

  bool show_difficulty(const std::vector<std::string>& args);

  bool print_connections(const std::vector<std::string>& args);

  bool print_blockchain_info(const std::vector<std::string>& args);

  bool set_log_level(const std::vector<std::string>& args);

  bool print_height(const std::vector<std::string>& args);

  bool print_block(const std::vector<std::string>& args);

  bool print_transaction(const std::vector<std::string>& args);

  bool print_transaction_pool_long(const std::vector<std::string>& args);

  bool print_transaction_pool_short(const std::vector<std::string>& args);

  bool start_mining(const std::vector<std::string>& args);

  bool stop_mining(const std::vector<std::string>& args);

  bool stop_daemon(const std::vector<std::string>& args);

  bool print_status(const std::vector<std::string>& args);
};

} // namespace daemonize
