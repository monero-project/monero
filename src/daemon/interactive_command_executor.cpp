#include "daemon/interactive_command_executor.h"

using namespace daemonize;

t_interactive_command_executor::t_interactive_command_executor(t_node_server & srv) :
    m_srv(srv)
{}

t_interactive_command_executor::t_interactive_command_executor(t_interactive_command_executor && other) = default;

bool t_interactive_command_executor::print_peer_list() {
  m_srv.log_peerlist();
  return true;
}

bool t_interactive_command_executor::save_blockchain() {
  m_srv.get_payload_object().get_core().get_blockchain_storage().store_blockchain();
  return true;
}

bool t_interactive_command_executor::show_hash_rate() {
  if(!m_srv.get_payload_object().get_core().get_miner().is_mining())
  {
    std::cout << "Mining is not started. You need start mining before you can see hash rate." << std::endl;
  } else
  {
    m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(true);
  }
  return true;
}

bool t_interactive_command_executor::hide_hash_rate() {
  m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(false);
  return true;
}

bool t_interactive_command_executor::show_difficulty() {
  cryptonote::difficulty_type difficulty = m_srv.get_payload_object().get_core().get_blockchain_storage().get_difficulty_for_next_block();
  uint64_t height = m_srv.get_payload_object().get_core().get_blockchain_storage().get_current_blockchain_height();

  std::cout << "BH: " << height << ", DIFF: " << difficulty
    << ", HR: " << (int) difficulty / 60L << " H/s" << std::endl;

  return true;
}

bool t_interactive_command_executor::print_connections() {
   m_srv.get_payload_object().log_connections();
   return true;
}

bool t_interactive_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
  uint64_t end_block_parametr = m_srv.get_payload_object().get_core().get_current_blockchain_height();
  if (end_block_index == 0)
  {
    end_block_index = end_block_parametr;
  }
  if (end_block_index > end_block_parametr)
  {
    std::cout << "end block index parameter shouldn't be greater than " << end_block_parametr << std::endl;
    return false;
  }
  if (end_block_index <= start_block_index)
  {
    std::cout << "end block index should be greater than starter block index" << std::endl;
    return false;
  }

  m_srv.get_payload_object().get_core().print_blockchain(start_block_index, end_block_index);
  return true;
}

bool t_interactive_command_executor::set_log_level(int8_t level) {
  epee::log_space::log_singletone::get_set_log_detalisation_level(true, level);
  return true;
}

bool t_interactive_command_executor::print_block_by_hash(crypto::hash block_hash) {
  std::list<crypto::hash> block_ids;
  block_ids.push_back(block_hash);
  std::list<cryptonote::block> blocks;
  std::list<crypto::hash> missed_ids;
  m_srv.get_payload_object().get_core().get_blocks(block_ids, blocks, missed_ids);

  if (1 == blocks.size())
  {
    cryptonote::block block = blocks.front();
    std::cout << cryptonote::obj_to_json_str(block) << std::endl;
  }
  else
  {
    std::cout << "block wasn't found: " << block_hash << std::endl;
    return false;
  }

  return true;
}

bool t_interactive_command_executor::print_block_by_height(uint64_t height) {
  std::list<cryptonote::block> blocks;
  m_srv.get_payload_object().get_core().get_blocks(height, 1, blocks);

  if (1 == blocks.size())
  {
    cryptonote::block& block = blocks.front();
    std::cout << "block_id: " << get_block_hash(block) << std::endl;
    std::cout << cryptonote::obj_to_json_str(block) << std::endl;
  }
  else
  {
    uint64_t current_height;
    crypto::hash top_id;
    m_srv.get_payload_object().get_core().get_blockchain_top(current_height, top_id);
    std::cout << "block wasn't found. Current block chain height: " << current_height << ", requested: " << height << std::endl;
    return false;
  }

  return true;
}

bool t_interactive_command_executor::print_transaction(crypto::hash transaction_hash) {
  std::vector<crypto::hash> tx_ids;
  tx_ids.push_back(transaction_hash);
  std::list<cryptonote::transaction> txs;
  std::list<crypto::hash> missed_ids;
  m_srv.get_payload_object().get_core().get_transactions(tx_ids, txs, missed_ids);

  if (1 == txs.size())
  {
    cryptonote::transaction tx = txs.front();
    std::cout << cryptonote::obj_to_json_str(tx) << std::endl;
  }
  else
  {
    std::cout << "transaction wasn't found: <" << transaction_hash << '>' << std::endl;
    return false;
  }

  return true;
}

bool t_interactive_command_executor::print_transaction_pool_long() {
  std::cout << "Pool state: " << std::endl << m_srv.get_payload_object().get_core().print_pool(false) << std::endl;
  return true;
}

bool t_interactive_command_executor::print_transaction_pool_short() {
  std::cout << "Pool state: " << std::endl << m_srv.get_payload_object().get_core().print_pool(true) << std::endl;
  return true;
}

bool t_interactive_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads) {
  boost::thread::attributes attrs;
  attrs.set_stack_size(THREAD_STACK_SIZE);

  m_srv.get_payload_object().get_core().get_miner().start(address, num_threads, attrs);
  return true;
}

bool t_interactive_command_executor::stop_mining() {
  m_srv.get_payload_object().get_core().get_miner().stop();
  return true;
}

bool t_interactive_command_executor::stop_daemon() {
  std::cout << "This is a stub!  The old interactive commands should go away once testing is complete!" << std::endl;
  return true;
}
