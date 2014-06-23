#pragma once

#include "crypto/hash.h"
#include "cryptonote_core/cryptonote_basic.h"
#include <cstdint>

namespace daemonize {

class t_command_executor {
public:
  virtual ~t_command_executor() {}
  virtual bool print_peer_list() = 0;
  virtual bool save_blockchain() = 0;
  virtual bool show_hash_rate() = 0;
  virtual bool hide_hash_rate() = 0;
  virtual bool show_difficulty() = 0;
  virtual bool print_connections() = 0;
  virtual bool print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) = 0;
  virtual bool set_log_level(uint16_t level) = 0;
  virtual bool print_block_by_hash(crypto::hash block_hash) = 0;
  virtual bool print_block_by_height(uint64_t height) = 0;
  virtual bool print_transaction(crypto::hash transaction_hash) = 0;
  virtual bool print_transaction_pool_long() = 0;
  virtual bool print_transaction_pool_short() = 0;
  virtual bool start_mining(cryptonote::account_public_address address, uint64_t num_threads) = 0;
  virtual bool stop_mining() = 0;
};

} // namespace daemonize
