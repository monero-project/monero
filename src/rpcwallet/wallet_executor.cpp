#include "rpcwallet/wallet_executor.h"

#include "misc_log_ex.h"

#include "cryptonote_config.h"
#include "version.h"

#include <string>

namespace tools
{
  std::string const t_wallet_executor::BASE_NAME = "BitMonero Wallet Daemon";

  void t_wallet_executor::init_options(
      boost::program_options::options_description & hidden_options
    , boost::program_options::options_description & normal_options
    , boost::program_options::options_description & configurable_options
    )
  {
    // Unused
  }

  t_wallet_executor::t_wallet_executor(
      std::string wallet_file
    , std::string wallet_password
    , std::string daemon_address
    , std::string bind_ip
    , std::string port
    )
    : m_name{BASE_NAME + " - " + bind_ip + ":" + port}
    , m_wallet_file{std::move(wallet_file)}
    , m_wallet_password{std::move(wallet_password)}
    , m_daemon_address{std::move(daemon_address)}
    , m_bind_ip{std::move(bind_ip)}
    , m_port{std::move(port)}
  {}

  std::string const & t_wallet_executor::name()
  {
    return m_name;
  }

  t_wallet_daemon t_wallet_executor::create_daemon(
      boost::program_options::variables_map const &
    )
  {
    LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
    return t_wallet_daemon{
      m_wallet_file
    , m_wallet_password
    , m_daemon_address
    , m_bind_ip
    , m_port
    };
  }

  bool t_wallet_executor::run_interactive(
      boost::program_options::variables_map const &
    )
  {
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, nullptr, nullptr, LOG_LEVEL_2);
    return t_wallet_daemon{
      m_wallet_file
    , m_wallet_password
    , m_daemon_address
    , m_bind_ip
    , m_port
    }.run();
  }
}
