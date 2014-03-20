// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <thread>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "include_base_utils.h"
#include "common/command_line.h"
#include "common/util.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "simplewallet.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "version.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;

#define EXTENDED_LOGS_FILE "wallet_details.log"


namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", "Use wallet <arg>", ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", "Generate new wallet and save it to <arg> or <address>.wallet by default", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", "Use daemon instance at <host>:<port>", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", "Use daemon instance at host <arg> instead of localhost", ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", "Wallet password", "", true};
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", "Use daemon instance at port <arg> instead of 8080", 0};
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"set_log", "", 0, true};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  void print_success_msg(const std::string& msg, bool color = false)
  {
    LOG_PRINT_L4(msg);
    if (color) epee::log_space::set_console_color(epee::log_space::console_color_green, false);
    std::cout << msg;
    if (color) epee::log_space::reset_console_color();
    std::cout << std::endl;
  }

  void print_fail_msg(const std::string& msg)
  {
    LOG_PRINT_L1("Error:" << msg);
    epee::log_space::set_console_color(epee::log_space::console_color_red, true);
    std::cout << "Error: " << msg;
    epee::log_space::reset_console_color();
    std::cout << std::endl;
  }
}


std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << "Commands: " << ENDL;
  std::string usage = m_cmd_binder.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  std::cout << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_daemon_port(0)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), "Start mining in daemon, start_mining <threads_count>");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), "Stop mining in daemon");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("show_balance", boost::bind(&simple_wallet::show_balance, this, _1), "Show current wallet balance");
  m_cmd_binder.set_handler("show_incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), "Show incoming transfers");
  m_cmd_binder.set_handler("show_bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), "transfer <mixin_count> {<addr> <amount>} Transfer <amount> to <address>. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available)");
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), "Change current log detalization level, <level> is a number 0-4");
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), "Show current wallet public address");
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), "Save wallet synchronized data");
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), "Show this help");
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1) 
  {
    std::cout << "use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }
  uint16_t l = 0;
  if(!string_tools::get_xtype_from_string(l, args[0]))
  {
    std::cout << "wrong number format, use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    std::cout << "wrong number range, use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  handle_command_line(vm);

  CHECK_AND_ASSERT_MES(m_daemon_address.empty() || (m_daemon_host.empty() && !m_daemon_port), false, "you can't specify daemon host or port several times");

  size_t c = 0; 
  if(!m_generate_new.empty()) ++c;
  if(!m_wallet_file.empty()) ++c;
  CHECK_AND_ASSERT_MES(c == 1, false, "you must specify --wallet-file or --generate-new-wallet params");

  if (m_daemon_host.empty())
    m_daemon_host = "localhost";
  if (!m_daemon_port)
    m_daemon_port = RPC_DEFAULT_PORT;
  if (m_daemon_address.empty())
    m_daemon_address = string("http://") + m_daemon_host + ":" + lexical_cast<string>(m_daemon_port);

  tools::password_container pwd_container;
  if (command_line::has_arg(vm, arg_password))
  {
    pwd_container.password(command_line::get_arg(vm, arg_password));
  }
  else
  {
    bool r = pwd_container.read_password();
    CHECK_AND_ASSERT_MES(r, false, "failed to read wallet password");
  }

  if (!m_generate_new.empty())
  {
    bool r = new_wallet(m_generate_new, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "account creation failed");
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "could not open account");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet.get())
    return true;

  return close_wallet();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file    = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new   = command_line::get_arg(vm, arg_generate_new_wallet);
  m_daemon_address = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host    = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port    = command_line::get_arg(vm, arg_daemon_port);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon()
{
  if (!m_wallet->check_connection())
  {
    std::string msg = "wallet failed to connect to daemon (" + m_daemon_address + "). " +
      "Daemon either is not started or passed wrong port. " +
      "Please, make sure that daemon is running or restart the wallet with correct daemon address.";
    print_fail_msg(msg);
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  if(boost::filesystem::exists(wallet_file))
  {
    std::cout << "wallet creation failed, file " << wallet_file << " already exists" << std::endl;
    return false;
  }

  m_wallet.reset(new tools::wallet2());
  bool r = m_wallet->generate(wallet_file, password);
  if(!r)
    return false;

  cout << "Generated new wallet" << ENDL;
  print_address(std::vector<std::string>());
  r = m_wallet->init(m_daemon_address);
  CHECK_AND_ASSERT_MES(r, false, "failed to init wallet");
  std::cout << "**********************************************************************" << ENDL 
    << "Your wallet has been generated. " << ENDL 
    << "To start synchronizing with the daemon use \"refresh\" command." << ENDL
    << "Use \"help\" command to see the list of available commands." << ENDL
    << "Always use \"exit\" command when closing simplewallet to save "
    << "current session's state. Otherwise, you will possibly need to synchronize " << ENDL 
    << "your wallet again. Your wallet key is NOT under risk anyway." << ENDL 
    << "**********************************************************************" << ENDL;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2());

  bool r = m_wallet->load(m_wallet_file, password);
  CHECK_AND_ASSERT_MES(r, false, "failed to load wallet " + m_wallet_file);
  r = m_wallet->init(m_daemon_address);
  CHECK_AND_ASSERT_MES(r, false, "failed to init wallet");

  refresh(std::vector<std::string>());
  std::cout << "**********************************************************************" << ENDL 
    << "Use \"help\" command to see the list of available commands." << ENDL 
    << "**********************************************************************" << ENDL ;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  bool r = m_wallet->deinit();
  CHECK_AND_ASSERT_MES(r, false, "failed to deinit wallet");
  r = m_wallet->store();
  CHECK_AND_ASSERT_MES(r, false, "failed to store wallet " + m_wallet_file);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  bool r = m_wallet->store();
  if (r)
    print_success_msg("Wallet data saved");
  else
    print_fail_msg("failed to store wallet " + m_wallet_file);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str();

  if (0 == args.size())
  {
    req.threads_count = 1;
  }
  else if (1 == args.size())
  {
    uint16_t num;
    bool ok = string_tools::get_xtype_from_string(num, args[0]);
    if(!ok || 0 == num)
    {
      print_fail_msg("wrong number of mining threads: \"" + args[0] + "\"");
      return true;
    }
    req.threads_count = num;
  }
  else
  {
    print_fail_msg("wrong number of arguments, expected the number of mining threads");
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  std::string err = tools::interpret_rpc_response(r, res.status);
  if (err.empty())
    print_success_msg("Mining started in daemon");
  else
    print_fail_msg("mining has NOT been started: " + err);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/stop_mining", req, res, m_http_client);
  std::string err = tools::interpret_rpc_response(r, res.status);
  if (err.empty())
    print_success_msg("Mining stopped in daemon");
  else
    print_fail_msg("mining has NOT been stopped: " + err);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::cout << "Starting refresh..." << endl;
  std::atomic<bool> refresh_is_done(false);
  std::thread th([&]()
  {
    epee::misc_utils::sleep_no_w(1000);
    while(!refresh_is_done)
    {
      std::string err;
      uint64_t bc_height = get_daemon_blockchain_height(err);
      if (err.empty())
        cout << "Height " << m_wallet->get_blockchain_current_height() << " of " << bc_height << endl;
      epee::misc_utils::sleep_no_w(1000);
    }
  });
  uint64_t initial_height = m_wallet->get_blockchain_current_height();
  uint64_t fetched_blocks = 0;
  bool money_received = false;
  tools::wallet2::fail_details fd;
  bool ok = m_wallet->refresh(fetched_blocks, money_received, fd);
  refresh_is_done = true;
  th.join();
  if (ok)
  {
    std::stringstream ss;
    ss << "Refresh done, blocks received: " << fetched_blocks;
    print_success_msg(ss.str(), true);

    show_balance();
  }
  else
  {
    fetched_blocks = m_wallet->get_blockchain_current_height() - initial_height;
    std::stringstream ss;
    ss << "refresh failed: " << fd.what() << ". Blocks received: " << fetched_blocks;
    print_fail_msg(ss.str());
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  std::stringstream ss;
  ss << "balance: " << print_money(m_wallet->balance()) << ", unlocked balance: " << print_money(m_wallet->unlocked_balance());
  print_success_msg(ss.str());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  std::cout << "        amount       \tspent\tglobal index\t                              tx id" << std::endl;
  bool ok = m_wallet->enum_incoming_transfers([](const cryptonote::transaction& tx, uint64_t global_out_index, uint64_t amount, bool spent) {
    epee::log_space::set_console_color(spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, true);
    std::cout << std::setw(21) << print_money(amount) << '\t'
      << std::setw(3) << (spent ? 'T' : 'F') << "  \t"
      << std::setw(12) << global_out_index << '\t'
      << get_transaction_hash(tx)
      << '\n';
  });
  epee::log_space::reset_console_color();
  if (ok)
    std::cout.flush();
  else
    print_fail_msg("No incoming transfers");
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  err = tools::interpret_rpc_response(r, res.status);
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
    print_success_msg(boost::lexical_cast<std::string>(bc_height));
  else
    print_fail_msg("failed to get blockchain height: " + err);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;
  if(local_args.size() < 3)
  {
    print_fail_msg("wrong number of arguments, expected at least 3, got " + boost::lexical_cast<std::string>(local_args.size()));
    return true;
  }

  size_t fake_outs_count;
  if(!string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
  {
    print_fail_msg("mixin_count should be non-negative integer, got " + local_args[0]);
    return true;
  }
  local_args.erase(local_args.begin());

  vector<cryptonote::tx_destination_entry> dsts;
  uint64_t summary_amount = 0;
  for (size_t i = 0; i < local_args.size(); i += 2) 
  {
    cryptonote::tx_destination_entry de;
    if(!get_account_address_from_str(de.addr, local_args[i]))
    {
      print_fail_msg("wrong address: " + local_args[i]);
      return true;
    }

    if (local_args.size() <= i + 1)
    {
      print_fail_msg("amount for the last address " + local_args[i] + " is not specified");
      return true;
    }

    bool ok = cryptonote::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      print_fail_msg("amount is wrong: " + local_args[i] + " " + local_args[i + 1]);
      return true;
    }

    summary_amount += de.amount;
    dsts.push_back(de);
  }

  if(summary_amount > m_wallet->unlocked_balance())
  {
    print_fail_msg("not enough money to transfer " + print_money(summary_amount) + ", available (unlocked) only " + print_money(m_wallet->unlocked_balance()));
    return true;
  }

  cryptonote::transaction tx;
  tools::wallet2::fail_details tfd;
  bool ok = m_wallet->transfer(dsts, fake_outs_count, 0, DEFAULT_FEE, tx, tfd);
  if (ok)
    print_success_msg("Money successfully sent", true);
  else
    print_fail_msg("failed to transfer money: " + tfd.what());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  return m_cmd_binder.run_handling("[wallet]# ", "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();
  m_wallet->stop();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args)
{
  print_success_msg(m_wallet->get_account().get_public_address_str());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  //TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params("Wallet options");
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  cryptonote::simple_wallet w;
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << "Usage: simplewallet [--wallet-file=<file>|--generate-new-wallet=<file>] [--daemon-address=<host>:<port>] [<COMMAND>]\n";
      std::cout << desc_all << '\n' << w.get_commands_str() << std::endl;
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << ENDL;
      return false;
    }

    auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_space::log_singletone::get_default_log_file().c_str(),
    log_space::log_singletone::get_default_log_folder().c_str(), LOG_LEVEL_4);

  LOG_PRINT_L0(CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG);

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }

  r = w.init(vm);
  CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet");

  std::vector<std::string> command = command_line::get_arg(vm, arg_command);
  if (!command.empty())
    w.process_command(command);

  tools::signal_handler::install([&w] {
    w.stop();
  });
  w.run();

  w.deinit();

  return 1;
  //CATCH_ENTRY_L0("main", 1);
}
