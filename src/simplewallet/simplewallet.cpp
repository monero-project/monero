// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <thread>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "include_base_utils.h"
#include "common/command_line.h"
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
}

/*const char *commands_help =
  "Commands:\n"
  "  help                              Show this help\n"
  "  address                           Show current account public address\n"
  "  exit\n"
  "  refresh\n"
  "  start_mining                      Start mining\n"
  "  set_log\n"
  "  show_balance                      Show current account balance\n"
  "  show_bc_height                    Show blockchain height\n"
  "  show_incoming_transfers           Show coins\n"
  "  transfer <mixin_count> (<addr> <amount>)...     Transfer <amount> to <addr>\n";*/



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

bool simple_wallet::help(const std::vector<std::string> &args)
{
  std::cout << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_daemon_port(0)
  , m_tried_to_connect(false)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), "Start mining in daemon");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), "Stop mining in daemon");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("show_balance", boost::bind(&simple_wallet::show_balance, this, _1), "Show current wallet balance");
  m_cmd_binder.set_handler("show_incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), "Show incoming transfers");
  m_cmd_binder.set_handler("show_bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), "transfer <mixin_count> <<addr> <amount>> Transfer <amount> to <address>. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available)");
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
  int l = 0;
  if(!string_tools::get_xtype_from_string(l, args[0]))
  {
    std::cout << "wrong number format, use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }
  if(l < 0 || l > LOG_LEVEL_4)
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
void simple_wallet::try_connect_to_daemon()
{
  if (!m_tried_to_connect)
  {
    m_tried_to_connect = true;

    if(!m_wallet->check_connection())
    {
      std::cout <<
        "**********************************************************************" << ENDL <<
        "Wallet failed to connect to daemon. Daemon either is not started or passed wrong port. Please, make sure that daemon is running or restart the wallet with correct daemon address." << ENDL <<
        "**********************************************************************" << ENDL;
    }
  }
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

  refresh(vector<string>());
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
  CHECK_AND_ASSERT_MES(r, false, "failed to store wallet " + m_wallet_file);
  std::cout << "Wallet data saved" << ENDL;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const vector<string>& args)
{
  try_connect_to_daemon();

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str();
  req.threads_count = 1;
  if(args.size() == 1)
  {
    if(!string_tools::get_xtype_from_string(req.threads_count, args[0]))
    {
      std::cout << "Threads count value invalid \""  << args[0] << "\"" << ENDL;
      return false;
    }
  }
  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  if (!r)
    std::cout << "Mining has NOT been started" << std::endl;
  CHECK_AND_ASSERT_MES(r, EXIT_FAILURE, "failed to invoke http request");
  std::cout << "Mining started in daemon." << ENDL;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const vector<string>& args)
{
  try_connect_to_daemon();

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/stop_mining", req, res, m_http_client);
  if (!r)
    std::cout << "Mining has NOT been stopped" << std::endl;
  CHECK_AND_ASSERT_MES(r, EXIT_FAILURE, "failed to invoke http request");
  std::cout << "Mining stopped in daemon." << ENDL;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const vector<string>& args)
{
  try_connect_to_daemon();

  std::cout << "Starting refresh..." << endl;
  std::atomic<bool> refresh_is_done(false);
  std::thread th([&]()
  {
    epee::misc_utils::sleep_no_w(1000);
    while(!refresh_is_done)
    {
      bool ok;
      uint64_t bc_height = get_daemon_blockchain_height(ok);
      if (ok)
        cout << "Height " << m_wallet->get_blockchain_current_height() << " of " << bc_height << endl;
      epee::misc_utils::sleep_no_w(1000);
    }
  });
  uint64_t fetched_blocks = 0;
  bool money_received = false;
  bool ok = m_wallet->refresh(fetched_blocks, money_received);
  refresh_is_done = true;
  th.join();
  if (ok)
  std::cout << "Refresh done, blocks received: " << fetched_blocks << endl;
  else
    std::cout << "Refresh failed, no blocks received" << std::endl;
  show_balance(vector<string>());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const vector<string>& args)
{
  cout << "balance: " << print_money(m_wallet->balance()) << ", unlocked balance: " << print_money(m_wallet->unlocked_balance()) << endl;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const vector<string>& args)
{
  m_wallet->show_incoming_transfers();
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(bool& ok)
{
  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  ok = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  CHECK_AND_ASSERT_MES(ok, 0, "failed to invoke http request");
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const vector<string>& args)
{
  try_connect_to_daemon();

  bool ok;
  uint64_t bc_height = get_daemon_blockchain_height(ok);
  if (ok)
    cout << "core returned height: " << bc_height << endl;
  else
    std::cout << "Failed to get blockchain height" << std::endl;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const vector<string> &args_)
{
  try_connect_to_daemon();

  vector<string> local_args = args_;
  if(local_args.size() < 3)
  {
    std::cout << "wrong transfer arguments" << std::endl;
    help(vector<string>());
    return true;
  }
  size_t fake_outs_count;
  if(!string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
  {
    std::cout << " ambiguity_degree set wrong" << std::endl;
    help(vector<string>());
    return true;
  }
  local_args.erase(local_args.begin());
  if(local_args.size() % 2 != 0)
  {
    cout << "wrong transfer arguments" << endl;
    help(vector<string>());
    return true;
  }

  vector<cryptonote::tx_destination_entry> dsts;
  uint64_t summary_amount = 0;
  for (size_t i = 0; i < local_args.size(); i += 2) 
  {
    cryptonote::tx_destination_entry de;
    if(!cryptonote::parse_amount(de.amount, local_args[i+1]))
    {
      cout << "Wrong transfer arguments" << endl;;
      help(vector<string>());
      return true;
    }
    if(de.amount <= 0)
    {
      cout << "Wrong transfer amount: " << de.amount << endl;;
      help(vector<string>());
      return true;
    }
    summary_amount += de.amount;
    if(!get_account_address_from_str(de.addr, local_args[i])) 
    {
      cout << "Wrong address: " << local_args[i] << endl;
      help(vector<string>());
      return true;
    }
    dsts.push_back(de);
  }

  if(summary_amount > m_wallet->unlocked_balance())
  {
    cout << "Not enough money to transfer " << print_money(summary_amount) << ", available(unlocked) only " << print_money(m_wallet->unlocked_balance()) << endl;
    return true;
  }

  m_wallet->transfer(dsts, fake_outs_count, 0, DEFAULT_FEE);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  m_cmd_binder.run_handling("");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args)
{
  std::cout << "Public address: " << m_wallet->get_account().get_public_address_str() << ENDL;
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
      std::cout << "BYTECOIN WALLET v" << PROJECT_VERSION_LONG << ENDL;
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
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_1);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_space::log_singletone::get_default_log_file().c_str(),
    log_space::log_singletone::get_default_log_folder().c_str());

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }
  LOG_PRINT("simplewallet starting", LOG_LEVEL_0);

  r = w.init(vm);
  CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet");

  std::vector<std::string> command = command_line::get_arg(vm, arg_command);
  if (!command.empty())
    w.process_command(command);
  w.run();

  w.deinit();

  return 1;
  //CATCH_ENTRY_L0("main", 1);
}
