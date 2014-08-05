#include "rpcwallet/wallet_daemon.h"

#include "common/util.h"
#include "rpcwallet/wallet_rpc_server.h"

namespace tools {

t_wallet_daemon::t_wallet_daemon(
      std::string wallet_file
    , std::string wallet_password
    , std::string daemon_address
    , std::string bind_ip
    , std::string port
  )
  : mp_server{new wallet_rpc_server{
      wallet_file
    , wallet_password
    , daemon_address
    , std::move(bind_ip)
    , std::move(port)
    }}
{}

t_wallet_daemon::~t_wallet_daemon() = default;

// MSVC is brain-dead and can't default this...
t_wallet_daemon::t_wallet_daemon(t_wallet_daemon && other)
{
  if (this != &other)
  {
    mp_server = std::move(other.mp_server);
    other.mp_server.reset(nullptr);
  }
}

// or this
t_wallet_daemon & t_wallet_daemon::operator=(t_wallet_daemon && other)
{
  if (this != &other)
  {
    mp_server = std::move(other.mp_server);
    other.mp_server.reset(nullptr);
  }
  return *this;
}

bool t_wallet_daemon::run()
{
  if (nullptr == mp_server)
  {
    throw std::runtime_error{"Can't run stopped wallet daemon"};
  }

  tools::signal_handler::install(std::bind(&tools::t_wallet_daemon::stop, this));

  mp_server->run();
  return true;
}

void t_wallet_daemon::stop()
{
  if (nullptr == mp_server)
  {
    throw std::runtime_error{"Can't stop stopped wallet daemon"};
  }
  mp_server->stop();
  mp_server.reset(nullptr); // Ensure resources are cleaned up before we return
}

} // namespace daemonize
