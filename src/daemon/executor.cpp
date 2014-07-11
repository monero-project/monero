#include "daemon/executor.h"

#include "common/command_line.h"
#include "cryptonote_config.h"
#include "version.h"

#include <string>

namespace daemonize
{
  t_executor::name = "BitMonero Daemon";

  void t_executor::init_options(
      boost::program_options::options_description & hidden_options
    , boost::program_options::options_description & normal_options
    , boost::program_options::options_description & configurable_options
    )
  {
    command_line::add_arg(normal_options, arg_config_file, std::string(CRYPTONOTE_NAME ".conf"));
    command_line::add_arg(configurable_options, arg_log_file, std::string(CRYPTONOTE_NAME ".log"));
    command_line::add_arg(configurable_options, arg_log_level);
  }

  t_daemon create_daemon(
      boost::program_options::variables_map const & vm
    )
  {
    LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
    return t_daemon{vm};
  }

  bool run_interactive(
      boost::program_options::variables_map const & vm
    )
  {
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
    return t_daemon{vm}.run();
  }
}

