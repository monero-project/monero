// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// node.cpp : Defines the entry point for the console application.
//

#include "include_base_utils.h"
#include "version.h"
#include "daemon/command_server.h"
#include "daemon/rpc_command_executor.h"
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

#ifndef WIN32
#include "posix_fork.h"
#endif

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
  const command_line::arg_descriptor<std::string> arg_log_file       = {"log-file", "Specify log file.  This can either be an absolute path or a path relative to the data directory"};
  const command_line::arg_descriptor<int>         arg_log_level      = {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<bool>        arg_console        = {"no-console", "Disable daemon console commands"};
#ifndef WIN32
  const command_line::arg_descriptor<bool>        arg_detach         = {"detach", "Run as daemon"};
#endif
  const command_line::arg_descriptor<std::vector<std::string>> arg_command = {"daemon_command", "Unused"};
}

int main(int argc, char* argv[])
{

  string_tools::set_module_name_and_folder(argv[0]);
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
  TRY_ENTRY();

  // Build argument description
  po::options_description all_options("All");
  po::options_description visible_options("Options");
  po::options_description core_settings("Settings");
  po::positional_options_description positional;
  {
    bf::path default_data_dir = bf::absolute(tools::get_default_data_dir());

    // Misc Options
    command_line::add_arg(visible_options, command_line::arg_help);
    command_line::add_arg(visible_options, command_line::arg_version);
    command_line::add_arg(visible_options, arg_os_version);
    command_line::add_arg(visible_options, command_line::arg_data_dir, default_data_dir.string());
    command_line::add_arg(visible_options, arg_config_file, std::string(CRYPTONOTE_NAME ".conf"));

    // Settings
#ifndef WIN32
    command_line::add_arg(core_settings, arg_detach);
#endif
    command_line::add_arg(core_settings, arg_log_file, std::string(CRYPTONOTE_NAME ".log"));
    command_line::add_arg(core_settings, arg_log_level);
    command_line::add_arg(core_settings, arg_console);
    // Add component-specific settings
    cryptonote::core::init_options(core_settings);
    cryptonote::core_rpc_server::init_options(core_settings);
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >::init_options(core_settings);
    cryptonote::miner::init_options(core_settings);

    // All Options
    visible_options.add(core_settings);

    all_options.add(visible_options);
    //all_options.add_options()(arg_command.name, po::value<std::vector<std::string>>()->default_value(std::vector<std::string>(), ""), "Unused");
    command_line::add_arg(all_options, arg_command);

    // Positional
    positional.add(arg_command.name, -1); // -1 for unlimited arguments
  }

  // Do command line parsing
  po::variables_map vm;
  bool success = command_line::handle_error_helper(all_options, [&]()
  {
    //po::store(po::parse_command_line(argc, argv, visible_options), vm);
    po::store(po::command_line_parser(argc, argv).options(all_options).positional(positional).run(), vm);
    //po::store(po::command_line_parser(argc, argv).options(all_options).run(), vm);

    return true;
  });
  if (!success) return 1;

  // Check Query Options
  {
    // Help
    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << "Usage: " << argv[0] << " [options|settings] [daemon_command...]" << std::endl << std::endl;
      std::cout << visible_options << std::endl;
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

  // If there are positional options, we're running a daemon command
  if (command_line::arg_present(vm, arg_command))
  {
    auto command = command_line::get_arg(vm, arg_command);
    t_command_server rpc_commands(new t_rpc_command_executor());
    if (rpc_commands.process_command_vec(command))
    {
      return 0;
    }
    else
    {
      std::cerr << "Unknown command" << std::endl;
      return 1;
    }
  }

  // Parse config file if it exists
  {
    bf::path data_dir_path(bf::absolute(command_line::get_arg(vm, command_line::arg_data_dir)));
    bf::path config_path(command_line::get_arg(vm, arg_config_file));

    if (config_path.is_relative())
    {
      config_path = data_dir_path / config_path;
    }

    boost::system::error_code ec;
    if (bf::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), core_settings), vm);
    }
    po::notify(vm);
  }

  // Set log file
  {
    bf::path data_dir(bf::absolute(command_line::get_arg(vm, command_line::arg_data_dir)));
    bf::path log_file_path(command_line::get_arg(vm, arg_log_file));

    if (log_file_path.is_relative())
    {
      log_file_path = data_dir / log_file_path;
    }

    boost::system::error_code ec;
    if (!log_file_path.has_parent_path() || !bf::exists(log_file_path.parent_path(), ec))
    {
      log_file_path = log_space::log_singletone::get_default_log_file();
    }

    std::string log_dir;
    log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : log_space::log_singletone::get_default_log_folder();
    log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str());
  }

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
#ifndef WIN32
  if (command_line::arg_present(vm, arg_detach)) {
    std::cout << "forking to background..." << std::endl;
    posix_fork();
  } else {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }
#else
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
#endif

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

  // Start handling console input if requested and not detached
  if(!command_line::arg_present(vm, arg_console) && !command_line::arg_present(vm, arg_detach))
  {
    LOG_PRINT_L0("Begin handling console input");
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
