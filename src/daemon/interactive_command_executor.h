#pragma once

#include "misc_log_ex.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/command_executor.h"
#include "p2p/net_node.h"

namespace daemonize {

class t_interactive_command_executor final : public t_command_executor {
public:
  typedef nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > t_node_server;
private:
  t_node_server & m_srv;
public:
  t_interactive_command_executor(t_node_server & srv);

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

  bool start_mining(cryptonote::account_public_address address, uint64_t num_threads) override;

  bool stop_mining() override;
};

} // namespace daemonize
