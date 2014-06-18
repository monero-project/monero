// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// node.cpp : Defines the entry point for the console application.
//


#include "include_base_utils.h"
#include "version.h"
#include "daemon/console_command_thread.h"

using namespace epee;

#include <boost/program_options.hpp>
#include <initializer_list>

#include "crypto/hash.h"
#include "console_handler.h"
#include "p2p/net_node.h"
#include "cryptonote_core/checkpoints_create.h"
#include "cryptonote_core/cryptonote_core.h"
#include "rpc/core_rpc_server.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"

//#ifndef WIN32
//#include "posix_daemonize.h"
//#endif

#ifdef WIN32
#include <crtdbg.h>
#endif

using namespace daemonize;

namespace po = boost::program_options;
namespace bf = boost::filesystem;

namespace
{
  const command_line::arg_descriptor<std::string> arg_config_file    = {"config-file", "Specify configuration file.  This can either be an absolute path or a path relative to the data directory"};
  const command_line::arg_descriptor<bool>        arg_os_version     = {"os-version", "OS for which this executable was compiled"};
  const command_line::arg_descriptor<std::string> arg_log_file       = {"log-file", "", ""};
  const command_line::arg_descriptor<int>         arg_log_level      = {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<bool>        arg_console        = {"no-console", "Disable daemon console commands"};

  const command_line::arg_descriptor<bool>        arg_stop_daemon    = {"stop", "Stop running daemon"};
  const command_line::arg_descriptor<bool>        arg_help_daemon    = {"command-help", "Display remote command help"};
  const command_line::arg_descriptor<std::string> arg_daemon_command = {"send-command", "Send a command string to the running daemon"};

#ifndef WIN32
  const command_line::arg_descriptor<bool>        arg_detach         = {"detach", "Run as daemon"};
#endif
}

int main(int argc, char* argv[])
{

  string_tools::set_module_name_and_folder(argv[0]);
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  LOG_PRINT_L0("Starting...");

  TRY_ENTRY();

  // Build argument description
  po::options_description argument_spec("Options");
  po::options_description core_settings_spec("Settings");
  {
    // Defaults
    bf::path default_data_dir = bf::absolute(tools::get_default_data_dir());
    bf::path default_log_file_rel = log_space::log_singletone::get_default_log_file();
    bf::path default_log_folder = log_space::log_singletone::get_default_log_folder();
    bf::path default_log_file_abs = bf::absolute( default_log_folder / default_log_file_rel );
    //bf::path default_config_file_rel = std::string(CRYPTONOTE_NAME ".conf");
    //bf::path default_config_file_abs = default_data_dir / default_config_file_rel;

    // Misc Options
    command_line::add_arg(argument_spec, command_line::arg_help);
    command_line::add_arg(argument_spec, command_line::arg_version);
    command_line::add_arg(argument_spec, arg_os_version);
    command_line::add_arg(argument_spec, arg_config_file, std::string(CRYPTONOTE_NAME ".conf"));

    // Daemon Options Descriptions
    po::options_description daemon_commands_spec("Commands");
    command_line::add_arg(daemon_commands_spec, arg_stop_daemon);
    command_line::add_arg(daemon_commands_spec, arg_help_daemon);
    command_line::add_arg(daemon_commands_spec, arg_daemon_command);

    // Settings
    command_line::add_arg(core_settings_spec, command_line::arg_data_dir, default_data_dir.string());
#ifndef WIN32
    command_line::add_arg(core_settings_spec, arg_detach);
#endif
    command_line::add_arg(core_settings_spec, arg_log_file, default_log_file_abs.string());
    command_line::add_arg(core_settings_spec, arg_log_level);
    command_line::add_arg(core_settings_spec, arg_console);
    // Add component-specific settings
    cryptonote::core::init_options(core_settings_spec);
    cryptonote::core_rpc_server::init_options(core_settings_spec);
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >::init_options(core_settings_spec);
    cryptonote::miner::init_options(core_settings_spec);

    // All Options
    argument_spec.add(daemon_commands_spec).add(core_settings_spec);
  }

  // Do command line parsing
  po::variables_map vm;
  bool success = command_line::handle_error_helper(argument_spec, [&]()
  {
    po::store(po::parse_command_line(argc, argv, argument_spec), vm);

    return true;
  });
  if (!success) return 1;

  // Check Query Options
  {
    // Help
    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << argument_spec << std::endl;
      return 0;
    }

    // Monero Version
    if (command_line::get_arg(vm, command_line::arg_version))
    {
      std::cout << CRYPTONOTE_NAME  << " v" << PROJECT_VERSION_LONG << ENDL;
      return 0;
    }

    // OS
    if (command_line::get_arg(vm, arg_os_version))
    {
      std::cout << "OS: " << tools::get_os_version_string() << ENDL;
      return 0;
    }
  }

  // Parse config file if it exists
  {
    std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
    std::string config = command_line::get_arg(vm, arg_config_file);

    bf::path data_dir_path(data_dir);
    bf::path config_path(config);
    if (!config_path.has_parent_path())
    {
      config_path = data_dir_path / config_path;
    }

    boost::system::error_code ec;
    if (bf::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), core_settings_spec), vm);
    }
    po::notify(vm);
  }

  // Set log file
  {
    bf::path log_file_path(command_line::get_arg(vm, arg_log_file));
    boost::system::error_code ec;
    if (log_file_path.empty() || !bf::exists(log_file_path, ec))
      log_file_path = log_space::log_singletone::get_default_log_file();
    std::string log_dir;
    log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : log_space::log_singletone::get_default_log_folder();
    log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str());
  }

#ifndef WIN32
  if (command_line::arg_present(vm, arg_detach)) {
    std::cout << "start daemon" << std::endl;
    return 0;
  }
#endif
  if (command_line::arg_present(vm, arg_stop_daemon)) {
    std::cout << "stop daemon" << std::endl;
    return 0;
  }
  if (command_line::arg_present(vm, arg_help_daemon)) {
    std::cout << "daemon help" << std::endl;
    return 0;
  }
  if (command_line::arg_present(vm, arg_daemon_command)) {
    std::string command = command_line::get_arg(vm, arg_daemon_command);
    std::cout << "daemon command: " << command << std::endl;
    return 0;
  }

  // Begin logging
  LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);

  // Set log level
  {
    int new_log_level = command_line::get_arg(vm, arg_log_level);
    if(new_log_level < LOG_LEVEL_MIN || new_log_level > LOG_LEVEL_MAX)
    {
      LOG_PRINT_L0("Wrong log level value: " << new_log_level);
    }
    else if (log_space::get_set_log_detalisation_level(false) != new_log_level)
    {
      log_space::get_set_log_detalisation_level(true, new_log_level);
      LOG_PRINT_L0("LOG_LEVEL set to " << new_log_level);
    }
  }

  bool res = true;
  cryptonote::checkpoints checkpoints;
  res = cryptonote::create_checkpoints(checkpoints);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize checkpoints");

  //create objects and link them
  cryptonote::core ccore(NULL);
  ccore.set_checkpoints(std::move(checkpoints));
  cryptonote::t_cryptonote_protocol_handler<cryptonote::core> cprotocol(ccore, NULL);
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > p2psrv(cprotocol);
  cryptonote::core_rpc_server rpc_server(ccore, p2psrv);
  cprotocol.set_p2p_endpoint(&p2psrv);
  ccore.set_cryptonote_protocol(&cprotocol);
  t_console_command_thread console_command_thread(p2psrv);

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  res = p2psrv.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK");

  LOG_PRINT_L0("Initializing cryptonote protocol...");
  res = cprotocol.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize cryptonote protocol.");
  LOG_PRINT_L0("Cryptonote protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  res = rpc_server.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  res = ccore.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  // start components
  if(!command_line::has_arg(vm, arg_console))
  {
    console_command_thread.start();
  }

  LOG_PRINT_L0("Starting core rpc server...");
  res = rpc_server.run(2, false);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  tools::signal_handler::install([&console_command_thread, &p2psrv] {
    console_command_thread.stop();
    p2psrv.send_stop_signal();
  });

  LOG_PRINT_L0("Starting p2p net loop...");
  p2psrv.run();
  LOG_PRINT_L0("p2p net loop stopped");

  //stop components
  LOG_PRINT_L0("Stopping core rpc server...");
  rpc_server.send_stop_signal();
  rpc_server.timed_wait_server_stop(5000);

  //deinitialize components
  LOG_PRINT_L0("Deinitializing core...");
  ccore.deinit();
  LOG_PRINT_L0("Deinitializing rpc server ...");
  rpc_server.deinit();
  LOG_PRINT_L0("Deinitializing cryptonote_protocol...");
  cprotocol.deinit();
  LOG_PRINT_L0("Deinitializing p2p...");
  p2psrv.deinit();

  ccore.set_cryptonote_protocol(NULL);
  cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  return 0;

  CATCH_ENTRY_L0("main", 1);
}
