#include "cryptonote_core/cryptonote_basic_impl.h"
#include "daemon/command_parser_executor.h"
#include "daemon/interactive_command_executor.h"
#include "daemon/rpc_command_executor.h"

namespace daemonize {

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_peer_list(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.print_peer_list();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
save_blockchain(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.save_blockchain();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
show_hash_rate(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.show_hash_rate();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
hide_hash_rate(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.hide_hash_rate();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
show_difficulty(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.show_difficulty();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_connections(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.print_connections();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_blockchain_info(const std::vector<std::string>& args)
{
  if(!args.size())
  {
    std::cout << "need block index parameter" << std::endl;
    return false;
  }
  uint64_t start_index = 0;
  uint64_t end_index = 0;
  if(!epee::string_tools::get_xtype_from_string(start_index, args[0]))
  {
    std::cout << "wrong starter block index parameter" << std::endl;
    return false;
  }
  if(args.size() >1 && !epee::string_tools::get_xtype_from_string(end_index, args[1]))
  {
    std::cout << "wrong end block index parameter" << std::endl;
    return false;
  }

  return m_executor.print_blockchain_info(start_index, end_index);
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
set_log_level(const std::vector<std::string>& args)
{
  if(args.size() != 1)
  {
    std::cout << "use: set_log <log_level_number_0-4>" << std::endl;
    return true;
  }

  uint16_t l = 0;
  if(!epee::string_tools::get_xtype_from_string(l, args[0]))
  {
    std::cout << "wrong number format, use: set_log <log_level_number_0-4>" << std::endl;
    return true;
  }

  if(LOG_LEVEL_4 < l)
  {
    std::cout << "wrong number range, use: set_log <log_level_number_0-4>" << std::endl;
    return true;
  }

  return m_executor.set_log_level(l);
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_block(const std::vector<std::string>& args)
{
  if (args.empty())
  {
    std::cout << "expected: print_block (<block_hash> | <block_height>)" << std::endl;
    return false;
  }

  const std::string& arg = args.front();
  try
  {
    uint64_t height = boost::lexical_cast<uint64_t>(arg);
    return m_executor.print_block_by_height(height);
  }
  catch (boost::bad_lexical_cast&)
  {
    crypto::hash block_hash;
    if (parse_hash256(arg, block_hash))
    {
      return m_executor.print_block_by_hash(block_hash);
    }
  }

  return false;
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_transaction(const std::vector<std::string>& args)
{
  if (args.empty())
  {
    std::cout << "expected: print_tx <transaction hash>" << std::endl;
    return true;
  }

  const std::string& str_hash = args.front();
  crypto::hash tx_hash;
  if (parse_hash256(str_hash, tx_hash))
  {
    m_executor.print_transaction(tx_hash);
  }

  return true;
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_transaction_pool_long(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.print_transaction_pool_long();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
print_transaction_pool_short(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.print_transaction_pool_short();
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
start_mining(const std::vector<std::string>& args)
{
  if(!args.size())
  {
    std::cout << "Please specify a wallet address to mine for: start_mining <addr> [threads=1]" << std::endl;
    return true;
  }

  cryptonote::account_public_address adr;
  if(!cryptonote::get_account_address_from_str(adr, args.front()))
  {
    std::cout << "target account address has wrong format" << std::endl;
    return true;
  }
  uint64_t threads_count = 1;
  if(args.size() > 2)
  {
    return false;
  }
  else if(args.size() == 2)
  {
    bool ok = epee::string_tools::get_xtype_from_string(threads_count, args[1]);
    threads_count = (ok && 0 < threads_count) ? threads_count : 1;
  }

  m_executor.start_mining(adr, threads_count);

  return true;
}

template <typename T_command_executor>
bool t_command_parser_executor<T_command_executor>::
stop_mining(const std::vector<std::string>& args)
{
  if (!args.empty()) return false;

  return m_executor.stop_mining();
}

template class t_command_parser_executor<t_interactive_command_executor>;
template class t_command_parser_executor<t_rpc_command_executor>;

} // namespace daemonize
