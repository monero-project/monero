// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/command_line.h"
#include "common/util.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/miner.h"
#include "daemon/command_server.h"
#include "daemon/daemon.h"
#include "daemon/posix_fork.h"
#include "daemon/windows_service.h"
#include "daemon/windows_service_runner.h"
#include "misc_log_ex.h"
#include "p2p/net_node.h"
#include "rpc/core_rpc_server.h"
#include <boost/program_options.hpp>
#ifdef WIN32
#  include <crtdbg.h>
#endif

namespace po = boost::program_options;
namespace bf = boost::filesystem;

namespace
{
  std::string const WINDOWS_SERVICE_NAME = "BitMonero Daemon";

  const command_line::arg_descriptor<std::string> arg_config_file = {
    "config-file"
  , "Specify configuration file.  This can either be an absolute path or a path relative to the data directory"
  };
  const command_line::arg_descriptor<bool> arg_os_version = {
    "os-version"
  , "OS for which this executable was compiled"
  };
  const command_line::arg_descriptor<std::string> arg_log_file = {
    "log-file"
  , "Specify log file.  This can either be an absolute path or a path relative to the data directory"
  };
  const command_line::arg_descriptor<int> arg_log_level = {
    "log-level"
  , ""
  , LOG_LEVEL_0
  };
  const command_line::arg_descriptor<std::vector<std::string>> arg_command = {
    "daemon_command"
  , "Hidden"
  };
  const command_line::arg_descriptor<bool> arg_detach = {
    "detach"
  , "Run as daemon"
  };
  const command_line::arg_descriptor<bool> arg_windows_service = {
    "run-as-service"
  , "Hidden -- true if running as windows service"
  };
}

int main(int argc, char* argv[])
{
  epee::string_tools::set_module_name_and_folder(argv[0]);
# ifdef WIN32
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
# endif
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
    command_line::add_arg(visible_options, arg_detach);

    // Settings
    command_line::add_arg(core_settings, arg_log_file, std::string(CRYPTONOTE_NAME ".log"));
    command_line::add_arg(core_settings, arg_log_level);
    cryptonote::core::init_options(core_settings);
    cryptonote::core_rpc_server::init_options(core_settings);
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >::init_options(core_settings);
    cryptonote::miner::init_options(core_settings);

    // Hidden options
    command_line::add_arg(all_options, arg_command);
#   ifdef WIN32
      command_line::add_arg(all_options, arg_windows_service);
#   endif

    visible_options.add(core_settings);
    all_options.add(visible_options);

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

  // Try to create requested/defaulted data directory
  {
    bf::path data_dir = bf::absolute(command_line::get_arg(vm, command_line::arg_data_dir));
    tools::create_directories_if_necessary(data_dir.string());
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

  // If there are positional options, we're running a daemon command
  if (command_line::arg_present(vm, arg_command))
  {
    auto command = command_line::get_arg(vm, arg_command);
    auto rpc_ip_str = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_ip);
    auto rpc_port_str = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_port);

    uint32_t rpc_ip;
    uint16_t rpc_port;
    if (!epee::string_tools::get_ip_int32_from_string(rpc_ip, rpc_ip_str))
    {
      std::cerr << "Invalid IP: " << rpc_ip_str << std::endl;
      return 1;
    }
    if (!epee::string_tools::get_xtype_from_string(rpc_port, rpc_port_str))
    {
      std::cerr << "Invalid port: " << rpc_port_str << std::endl;
      return 1;
    }

    daemonize::t_command_server rpc_commands{rpc_ip, rpc_port};
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

  // Start with log level 0
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);

  // Set log level
  {
    int new_log_level = command_line::get_arg(vm, arg_log_level);
    if(new_log_level < LOG_LEVEL_MIN || new_log_level > LOG_LEVEL_MAX)
    {
      LOG_PRINT_L0("Wrong log level value: " << new_log_level);
    }
    else if (epee::log_space::get_set_log_detalisation_level(false) != new_log_level)
    {
      epee::log_space::get_set_log_detalisation_level(true, new_log_level);
      LOG_PRINT_L0("LOG_LEVEL set to " << new_log_level);
    }
  }

  bool detach = command_line::arg_present(vm, arg_detach);
  bool win_service = command_line::arg_present(vm, arg_windows_service);

  // Set log file
  {
    bf::path data_dir{bf::absolute(command_line::get_arg(vm, command_line::arg_data_dir))};
    bf::path log_file_path{command_line::get_arg(vm, arg_log_file)};

    if (log_file_path.is_relative())
    {
      log_file_path = data_dir / log_file_path;
    }

    // Fall back to relative path for log file
    boost::system::error_code ec;
    if (!log_file_path.has_parent_path() || !bf::exists(log_file_path.parent_path(), ec))
    {
      log_file_path = epee::log_space::log_singletone::get_default_log_file();
    }

    std::string log_dir;
    log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : epee::log_space::log_singletone::get_default_log_folder();
    epee::log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str());
  }

  if (!detach && !win_service)
  {
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }

  /**
   * On first launch, we ask Windows to relaunch the executable as a
   * service with the added --run-as-service argument, which indicates that the
   * process is running in the background.
   *
   * On relaunch the --run-as-service argument is detected, and the
   * t_service_runner class finishes registering as a service and installs
   * the required service lifecycle handler callback.
   */
  if (win_service) // running as windows service
  {
#   ifdef WIN32
      LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
      windows::t_service_runner<daemonize::t_daemon>::run(
        WINDOWS_SERVICE_NAME
      , daemonize::t_daemon{vm}
      );
#   endif
  }
  else if (detach)
  {
#   ifdef WIN32
      // install windows service
      bool install = windows::install_service(WINDOWS_SERVICE_NAME, "--run-as-service");
      bool start = windows::start_service(WINDOWS_SERVICE_NAME);
      if (install && !start)
      {
        windows::uninstall_service(WINDOWS_SERVICE_NAME);
      }
#   else
      // fork
      posix::fork();
      LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
      daemonize::t_daemon{vm}.run();
#   endif
  }
  else // interactive
  {
    LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
    daemonize::t_daemon{vm}.run();
  }

  return 0;

  CATCH_ENTRY_L0("main", 1);
}
