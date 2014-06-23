#include "cryptonote_config.h"
#include "version.h"
#include "daemon/command_server.h"
#include "daemon/interactive_command_executor.h"
#include "daemon/rpc_command_executor.h"

namespace daemonize {

namespace p = std::placeholders;

template <typename T_command_executor>
t_command_server<T_command_executor>::t_command_server(
    T_command_executor && executor
  ) :
    m_parser(std::move(executor))
  , m_command_lookup()
{
  m_command_lookup.set_handler(
      "help"
    , std::bind(&t_command_server<T_command_executor>::help, this, p::_1)
    , "Show this help"
    );
  m_command_lookup.set_handler(
      "print_pl"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_peer_list, &m_parser, p::_1)
    , "Print peer list"
    );
  m_command_lookup.set_handler(
      "print_cn"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_connections, &m_parser, p::_1)
    , "Print connections"
    );
  m_command_lookup.set_handler(
      "print_bc"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_blockchain_info, &m_parser, p::_1)
    , "Print blockchain info in a given blocks range, print_bc <begin_height> [<end_height>]"
    );
  m_command_lookup.set_handler(
      "print_block"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_block, &m_parser, p::_1)
    , "Print block, print_block <block_hash> | <block_height>"
    );
  m_command_lookup.set_handler(
      "print_tx"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_transaction, &m_parser, p::_1)
    , "Print transaction, print_tx <transaction_hash>"
    );
  m_command_lookup.set_handler(
      "start_mining"
    , std::bind(&t_command_parser_executor<T_command_executor>::start_mining, &m_parser, p::_1)
    , "Start mining for specified address, start_mining <addr> [threads=1]"
    );
  m_command_lookup.set_handler(
      "stop_mining"
    , std::bind(&t_command_parser_executor<T_command_executor>::stop_mining, &m_parser, p::_1)
    , "Stop mining"
    );
  m_command_lookup.set_handler(
      "print_pool"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_transaction_pool_long, &m_parser, p::_1)
    , "Print transaction pool (long format)"
    );
  m_command_lookup.set_handler(
      "print_pool_sh"
    , std::bind(&t_command_parser_executor<T_command_executor>::print_transaction_pool_short, &m_parser, p::_1)
    , "Print transaction pool (short format)"
    );
  m_command_lookup.set_handler(
      "show_hr"
    , std::bind(&t_command_parser_executor<T_command_executor>::show_hash_rate, &m_parser, p::_1)
    , "Start showing hash rate"
    );
  m_command_lookup.set_handler(
      "hide_hr"
    , std::bind(&t_command_parser_executor<T_command_executor>::hide_hash_rate, &m_parser, p::_1)
    , "Stop showing hash rate"
    );
  m_command_lookup.set_handler(
      "save"
    , std::bind(&t_command_parser_executor<T_command_executor>::save_blockchain, &m_parser, p::_1)
    , "Save blockchain"
    );
  m_command_lookup.set_handler(
      "set_log"
    , std::bind(&t_command_parser_executor<T_command_executor>::set_log_level, &m_parser, p::_1)
    , "set_log <level> - Change current log detalization level, <level> is a number 0-4"
    );
  m_command_lookup.set_handler(
      "diff"
    , std::bind(&t_command_parser_executor<T_command_executor>::show_difficulty, &m_parser, p::_1)
    , "Show difficulty"
    );
}

template <typename T_command_executor>
bool t_command_server<T_command_executor>::process_command_str(const std::string& cmd)
{
  return m_command_lookup.process_command_str(cmd);
}

template <typename T_command_executor>
bool t_command_server<T_command_executor>::process_command_vec(const std::vector<std::string>& cmd)
{
  return m_command_lookup.process_command_vec(cmd);
}

template <typename T_command_executor>
bool t_command_server<T_command_executor>::help(const std::vector<std::string>& args)
{
  std::cout << get_commands_str() << std::endl;
  return true;
}

template <typename T_command_executor>
std::string t_command_server<T_command_executor>::get_commands_str()
{
  std::stringstream ss;
  ss << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << std::endl;
  ss << "Commands: " << std::endl;
  std::string usage = m_command_lookup.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << std::endl;
  return ss.str();
}

template class t_command_server<t_interactive_command_executor>;
template class t_command_server<t_rpc_command_executor>;

} // namespace daemonize
