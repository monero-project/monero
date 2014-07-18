#include "common/command_line.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "daemonizer/daemonizer.h"
#include "misc_log_ex.h"
#include "rpcwallet/wallet_executor.h"
#include "string_tools.h"
#include "version.h"

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string>   arg_wallet_file      = {"wallet-file", "Use wallet <arg>", ""};
  const command_line::arg_descriptor<std::string>   arg_daemon_address   = {"daemon-address", "Use daemon instance at <host>:<port>", ""};
  const command_line::arg_descriptor<std::string>   arg_daemon_host      = {"daemon-host", "Use daemon instance at host <arg> instead of localhost", ""};
  const command_line::arg_descriptor<int>           arg_daemon_port      = {"daemon-port", "Use daemon instance at port <arg> instead of 8081", 0};
  const command_line::arg_descriptor<std::string>   arg_password         = {"password", "Wallet password", "", true};
  const command_line::arg_descriptor<uint32_t>      arg_log_level        = {"set_log", "", 0, true};
  const command_line::arg_descriptor<std::string>   arg_rpc_bind_port    = {"rpc-bind-port", "Starts wallet as rpc server for wallet operations, sets bind port for server", "", true};
  const command_line::arg_descriptor<std::string>   arg_rpc_bind_ip      = {"rpc-bind-ip", "Specify ip to bind rpc server", "127.0.0.1"};
  const command_line::arg_descriptor<std::string>   arg_log_file         = {"log-file", "Specify log file"};
  const command_line::arg_descriptor<std::string>   arg_log_file         = {"stop", "Stop running daemon"};
}  // file-local namespace

int main(int argc, char const * argv[])
{
  epee::string_tools::set_module_name_and_folder(argv[0]);

  po::options_description hidden_options("Hidden options");

  po::options_description general_options("General options");
  command_line::add_arg(general_options, command_line::arg_help);
  command_line::add_arg(general_options, command_line::arg_version);
  command_line::add_arg(general_options, command_line::arg_stop);

  po::options_description wallet_options("Wallet options");
  command_line::add_arg(wallet_options, arg_wallet_file);
  command_line::add_arg(wallet_options, arg_password);
  command_line::add_arg(wallet_options, arg_daemon_address);
  command_line::add_arg(wallet_options, arg_daemon_host);
  command_line::add_arg(wallet_options, arg_daemon_port);
  command_line::add_arg(wallet_options, arg_log_level);
  command_line::add_arg(wallet_options, arg_rpc_bind_ip);
  command_line::add_arg(wallet_options, arg_rpc_bind_port);

  po::positional_options_description positional_options;

  daemonizer::init_options(hidden_options, general_options);

  po::options_description visible_options;
  visible_options.add(general_options).add(wallet_options);

  po::options_description all_options;
  all_options.add(visible_options).add(hidden_options);

  po::variables_map vm;

  bool r = command_line::handle_error_helper(visible_options, [&]()
  {
    auto parser = po::command_line_parser(argc, argv).options(all_options).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);

    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl << std::endl;
    std::cout << "Usage: rpcwallet --wallet-file=<file> --rpc-bind-port=<port> [--daemon-address=<host>:<port>] [--rpc-bind-address=127.0.0.1]" << std::endl;
    std::cout << visible_options << std::endl;
    return 0;
  }
  else if (command_line::get_arg(vm, command_line::arg_version))
  {
    std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl;
    return 0;
  }

  //set up logging options
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  {
    boost::filesystem::path log_file_path{boost::filesystem::absolute(command_line::get_arg(vm, arg_log_file))};

    epee::log_space::log_singletone::add_logger(
        LOGGER_FILE
      , log_file_path.filename().string().c_str()
      , log_file_path.parent_path().string().c_str()
      , LOG_LEVEL_4
      );
  }

  std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl;

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    epee::log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }


  //runs wallet with rpc interface
  if(!command_line::has_arg(vm, arg_wallet_file) )
  {
    tools::fail_msg_writer() << "Wallet file not set.";
    return 1;
  }
  if(!command_line::has_arg(vm, arg_daemon_address) )
  {
    tools::fail_msg_writer() << "Daemon address not set.";
    return 1;
  }
  if(!command_line::has_arg(vm, arg_password) )
  {
    tools::fail_msg_writer() << "Wallet password not set.";
    return 1;
  }

  std::string wallet_file     = boost::filesystem::absolute(command_line::get_arg(vm, arg_wallet_file)).string();
  std::string wallet_password = command_line::get_arg(vm, arg_password);
  std::string daemon_address  = command_line::get_arg(vm, arg_daemon_address);
  std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
  int daemon_port = command_line::get_arg(vm, arg_daemon_port);
  std::string bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
  std::string bind_port = command_line::get_arg(vm, arg_rpc_bind_port);
  if (daemon_host.empty())
    daemon_host = "localhost";
  if (!daemon_port)
    daemon_port = RPC_DEFAULT_PORT;
  if (daemon_address.empty())
    daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

  tools::t_wallet_executor executor{
      wallet_file
    , wallet_password
    , daemon_address
    , bind_ip
    , bind_port
  };
  return daemonizer::daemonize(argc, argv, std::move(executor), vm);
}
