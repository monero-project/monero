// Copyright (c) 2014-2017, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "cryptonote_config.h"
#include "version.h"
#include "daemon/command_server.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize {

namespace p = std::placeholders;

t_command_server::t_command_server(
    uint32_t ip
  , uint16_t port
  , const boost::optional<tools::login>& login
  , bool is_rpc
  , cryptonote::core_rpc_server* rpc_server
  )
  : m_parser(ip, port, login, is_rpc, rpc_server)
  , m_command_lookup()
  , m_is_rpc(is_rpc)
{
  m_command_lookup.set_handler(
      "help",
      std::bind(&t_command_server::help, this, p::_1),
      "help [<command>]",
      "Shows the generic help section or a <command> specific documentation."
  );
  m_command_lookup.set_handler(
      "print_cn",
      std::bind(&t_command_parser_executor::print_connections, &m_parser, p::_1),
      "Prints the current connections."
  );
  m_command_lookup.set_handler(
      "print_block",
      std::bind(&t_command_parser_executor::print_block, &m_parser, p::_1),
      "print_block (<block_hash> | <block_height>)",
      "Prints the block having the specified <block_hash> or <block_height>."
  );
  m_command_lookup.set_handler(
      "is_key_image_spent",
      std::bind(&t_command_parser_executor::is_key_image_spent, &m_parser, p::_1),
      "is_key_image_spent <key_image>",
      "Prints whether a given key image is in the spent key images set."
  );
  m_command_lookup.set_handler(
      "start_mining",
      std::bind(&t_command_parser_executor::start_mining, &m_parser, p::_1),
      "start_mining <addr> [<threads>] [do_background_mining] [ignore_battery]",
      "Starts mining for a specified address with for default 1 thread and no background mining."
  );
  m_command_lookup.set_handler(
      "stop_mining",
      std::bind(&t_command_parser_executor::stop_mining, &m_parser, p::_1),
      "Stop mining."
  );
  m_command_lookup.set_handler(
      "print_pool",
      std::bind(&t_command_parser_executor::print_transaction_pool, &m_parser, p::_1),
      "print_pool [long|stats]",
      "Prints the transaction pool, optionally in a long format."
  );
  m_command_lookup.set_handler(
      "hashrate",
      std::bind(&t_command_parser_executor::set_hash_rate_visibility, &m_parser, p::_1),
      "hashrate (show|hide)",
      "Start showing or hiding the hash rate."
  );
  m_command_lookup.set_handler(
      "save",
      std::bind(&t_command_parser_executor::save_blockchain, &m_parser, p::_1),
      "Save blockchain."
  );
  m_command_lookup.set_handler(
      "set_log",
      std::bind(&t_command_parser_executor::set_log_level, &m_parser, p::_1),
      "set_log (<level>|<{+,-,}categories>)",
      "Change current log level/categories, <level> is a number 0-4."
  );
  m_command_lookup.set_handler(
      "diff",
      std::bind(&t_command_parser_executor::show_difficulty, &m_parser, p::_1),
      "Show difficulty."
  );
  m_command_lookup.set_handler(
      "status",
      std::bind(&t_command_parser_executor::show_status, &m_parser, p::_1),
      "Show status"
  );
  m_command_lookup.set_handler(
      "exit",
      std::bind(&t_command_parser_executor::stop_daemon, &m_parser, p::_1),
      "Stops the daemon"
  );
  m_command_lookup.set_handler(
      "print_status",
      std::bind(&t_command_parser_executor::print_status, &m_parser, p::_1),
      "Prints daemon status."
  );
  m_command_lookup.set_handler(
      "limit",
      std::bind(&t_command_parser_executor::set_limit, &m_parser, p::_1),
      "limit [[up|down] <Kb/s>]",
      "Gets the down- and upload limit. Optionally sets the download and/or upload limit to <Kb/s>."
  );
  m_command_lookup.set_handler(
      "out_peers",
      std::bind(&t_command_parser_executor::out_peers, &m_parser, p::_1),
      "out_peers <amount>",
      "Set max number of out peers to <amount>."
  );
  m_command_lookup.set_handler(
      "print_pl",
      std::bind(&t_command_parser_executor::print_peer_list, &m_parser, p::_1),
      "Prints the current peer list."
  );
  m_command_lookup.set_handler(
      "print_pl_stats",
      std::bind(&t_command_parser_executor::print_peer_list_stats, &m_parser, p::_1),
      "Prints the current peer list stats."
  );
  m_command_lookup.set_handler(
      "save_graph",
      std::bind(&t_command_parser_executor::save_graph, &m_parser, p::_1),
      "save_graph (start|stop)",
      "Starts or stops to save data for dr monero."
  );
  m_command_lookup.set_handler(
      "hard_fork_info",
      std::bind(&t_command_parser_executor::hard_fork_info, &m_parser, p::_1),
      "Prints the hard fork voting information"
  );
  m_command_lookup.set_handler(
      "bans",
      std::bind(&t_command_parser_executor::show_bans, &m_parser, p::_1),
      "Shows the currently banned IPs"
  );
  m_command_lookup.set_handler(
      "ban",
      std::bind(&t_command_parser_executor::ban, &m_parser, p::_1),
      "ban <ip> [<seconds>]",
      "Bans a given IP. Optionnally for a given amount of seconds."
  );
  m_command_lookup.set_handler(
      "unban",
      std::bind(&t_command_parser_executor::unban, &m_parser, p::_1),
      "unban <ip>",
      "Unbans a given IP"
  );
  m_command_lookup.set_handler(
      "flush_txpool",
      std::bind(&t_command_parser_executor::flush_txpool, &m_parser, p::_1),
      "flush_txpool [<txid>]",
      "Flushes a transaction from the tx pool by its <txid>, or the whole tx pool."
  );
  m_command_lookup.set_handler(
      "output_histogram",
      std::bind(&t_command_parser_executor::output_histogram, &m_parser, p::_1),
      "Prints the output histogram (amount, instances)."
  );
  m_command_lookup.set_handler(
      "print_coinbase_tx_sum",
      std::bind(&t_command_parser_executor::print_coinbase_tx_sum, &m_parser, p::_1),
      "print_coinbase_tx_sum <start_height> [block_count]",
      "Prints the sum of coinbase transactions from the <start_height> for a <block_count> amount."
  );
  m_command_lookup.set_handler(
      "alt_chain_info",
      std::bind(&t_command_parser_executor::alt_chain_info, &m_parser, p::_1),
      "Prints the information about alternative chains."
  );
  m_command_lookup.set_handler(
      "update [download]",
      std::bind(&t_command_parser_executor::update, &m_parser, p::_1),
      "Checks if an update is available, optionally downloads it if there is."
  );
  m_command_lookup.set_handler(
      "relay_tx",
      std::bind(&t_command_parser_executor::relay_tx, &m_parser, p::_1),
      "relay_tx <txid>"
      "Relays a given transaction by its <txid>."
  );
  m_command_lookup.set_handler(
      "print_tx",
      std::bind(&t_command_parser_executor::print_transaction, &m_parser, p::_1),
      "print_tx <transaction_hash> [+hex] [+json]",
      "Prints the transaction."
  );
  m_command_lookup.set_handler(
      "sync_info",
      std::bind(&t_command_parser_executor::sync_info, &m_parser, p::_1),
      "Prints the information about blockchain sync state."
  );
  m_command_lookup.set_handler(
      "bc_dyn_stats",
      std::bind(&t_command_parser_executor::print_blockchain_dynamic_stats, &m_parser, p::_1),
      "bc_dyn_stats <last_n_blocks>",
      "Prints the information about current blockchain dynamic state."
  );
  m_command_lookup.set_handler(
      "print_height",
      std::bind(&t_command_parser_executor::print_height, &m_parser, p::_1),
      "Prints the local blockchain height."
  );
  m_command_lookup.set_handler(
      "print_bc",
      std::bind(&t_command_parser_executor::print_blockchain_info, &m_parser, p::_1),
      "print_bc <begin_height> [<end_height>]",
      "Prints the blockchain's info in a given blocks range."
  );
}

bool t_command_server::process_command_str(const std::string& cmd)
{
  return m_command_lookup.process_command_str(cmd);
}

bool t_command_server::process_command_vec(const std::vector<std::string>& cmd)
{
  bool result = m_command_lookup.process_command_vec(cmd);
  if (!result)
  {
    help(std::vector<std::string>());
  }
  return result;
}

bool t_command_server::start_handling(std::function<void(void)> exit_handler)
{
  if (m_is_rpc) return false;

  m_command_lookup.start_handling("", get_commands_str(), exit_handler);

  return true;
}

void t_command_server::stop_handling()
{
  if (m_is_rpc) return;

  m_command_lookup.stop_handling();
}

bool t_command_server::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
 if (args.size() != 1)
   std::cout << get_commands_str();
 else
   std::cout << get_commands_desc(args[0]);
 return true;
}

std::string t_command_server::get_commands_desc(const std::string &name)
{
 std::string desc = m_command_lookup.get_cmd_description(name);
 std::stringstream ss;
 if (desc.empty()) {
   ss << "Unknown command: " << name << ENDL;
 } else {
   std::string usage = m_command_lookup.get_cmd_usage(name);
   boost::replace_all(desc, "\n", "\n  ");
   desc.insert(0, "  ");
   boost::replace_all(usage, "\n", "\n  ");
   usage.insert(0, "  ");
   ss << "Desription: " << ENDL << desc << ENDL << "Usage: " << ENDL << usage;
 }
 return ss.str();
}

std::string t_command_server::get_commands_str() {
 std::stringstream ss;
 ss << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << std::endl;
 ss << "Commands: " << std::endl;
 std::string usage = m_command_lookup.get_usage();
 boost::replace_all(usage, "\n", "\n  ");
 usage.insert(0, "  ");
 ss << usage << std::endl;
 return ss.str();
}

} // namespace daemonize
