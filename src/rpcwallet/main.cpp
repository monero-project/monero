#include "common/command_line.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "misc_log_ex.h"
#include "rpcwallet/wallet_rpc_server.h"
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
}  // file-local namespace

int main(int argc, char* argv[])
{
  epee::string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params("Wallet options");
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_log_level);
  command_line::add_arg(desc_params, arg_rpc_bind_ip);
  command_line::add_arg(desc_params, arg_rpc_bind_port);

  po::positional_options_description positional_options;

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  po::variables_map vm;

  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl;
      std::cout << "Usage: rpcwallet --wallet-file=<file> --rpc-bind-port=<port> [--daemon-address=<host>:<port>] [--rpc-bind-address=127.0.0.1]" << std::endl;
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl;
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
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  //epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  epee::log_space::log_singletone::add_logger(LOGGER_FILE,
    epee::log_space::log_singletone::get_default_log_file().c_str(),
    epee::log_space::log_singletone::get_default_log_folder().c_str(), LOG_LEVEL_4);

  std::cout << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG << std::endl;

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    epee::log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }


  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
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

  std::string wallet_file     = command_line::get_arg(vm, arg_wallet_file);
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

  try {
    tools::wallet_rpc_server server{
      wallet_file
    , wallet_password
    , daemon_address
    , bind_ip
    , bind_port
    };
    server.run();
  }
  catch (...)
  {
    LOG_ERROR("Uncaught exception");
    return 1;
  }
}
