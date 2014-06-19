#pragma once

//#include "cryptonote_core/cryptonote_core.h"
//#include "cryptonote_protocol/cryptonote_protocol_handler.h"
//#include "p2p/net_node.h"
//#include <functional>
//#include <boost/algorithm/string.hpp>
#include "misc_log_ex.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "daemon/console_command_executor.h"
#include "daemon/command_parser_executor.h"
#include "p2p/net_node.h"

namespace daemonize {

namespace p = std::placeholders;
using namespace epee;

class t_command_server {
private:
  t_command_parser_executor m_parser;
  command_handler m_handler;
public:
  typedef nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > t_node_server;
  t_command_server(
      t_node_server & node_server
    ) :
      m_parser(new t_console_command_executor(node_server))
    , m_handler()
  {
    m_handler.set_handler("help", std::bind(&t_command_server::help, this, p::_1), "Show this help");
    m_handler.set_handler("print_pl", std::bind(&t_command_parser_executor::print_peer_list, &m_parser, p::_1), "Print peer list");
    m_handler.set_handler("print_cn", std::bind(&t_command_parser_executor::print_connections, &m_parser, p::_1), "Print connections");
    m_handler.set_handler("print_bc", std::bind(&t_command_parser_executor::print_blockchain_info, &m_parser, p::_1), "Print blockchain info in a given blocks range, print_bc <begin_height> [<end_height>]");
    m_handler.set_handler("print_block", std::bind(&t_command_parser_executor::print_block, &m_parser, p::_1), "Print block, print_block <block_hash> | <block_height>");
    m_handler.set_handler("print_tx", std::bind(&t_command_parser_executor::print_transaction, &m_parser, p::_1), "Print transaction, print_tx <transaction_hash>");
    m_handler.set_handler("start_mining", std::bind(&t_command_parser_executor::start_mining, &m_parser, p::_1), "Start mining for specified address, start_mining <addr> [threads=1]");
    m_handler.set_handler("stop_mining", std::bind(&t_command_parser_executor::stop_mining, &m_parser, p::_1), "Stop mining");
    m_handler.set_handler("print_pool", std::bind(&t_command_parser_executor::print_transaction_pool_long, &m_parser, p::_1), "Print transaction pool (long format)");
    m_handler.set_handler("print_pool_sh", std::bind(&t_command_parser_executor::print_transaction_pool_short, &m_parser, p::_1), "Print transaction pool (short format)");
    m_handler.set_handler("show_hr", std::bind(&t_command_parser_executor::show_hash_rate, &m_parser, p::_1), "Start showing hash rate");
    m_handler.set_handler("hide_hr", std::bind(&t_command_parser_executor::hide_hash_rate, &m_parser, p::_1), "Stop showing hash rate");
    m_handler.set_handler("save", std::bind(&t_command_parser_executor::save_blockchain, &m_parser, p::_1), "Save blockchain");
    m_handler.set_handler("set_log", std::bind(&t_command_parser_executor::set_log_level, &m_parser, p::_1), "set_log <level> - Change current log detalization level, <level> is a number 0-4");
    m_handler.set_handler("diff", std::bind(&t_command_parser_executor::show_difficulty, &m_parser, p::_1), "Show difficulty");
  }

  bool process_command(const std::string& cmd)
  {
    return m_handler.process_command_str(cmd);
  }

private:
  bool help(const std::vector<std::string>& args)
  {
    std::cout << get_commands_str() << std::endl;
    return true;
  }

  std::string get_commands_str()
  {
    std::stringstream ss;
    ss << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << std::endl;
    ss << "Commands: " << std::endl;
    std::string usage = m_handler.get_usage();
    boost::replace_all(usage, "\n", "\n  ");
    usage.insert(0, "  ");
    ss << usage << std::endl;
    return ss.str();
  }
};

} // namespace daemonize
