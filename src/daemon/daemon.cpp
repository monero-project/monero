#include "daemon/daemon.h"

#include "common/util.h"
#include "daemon/core.h"
#include "daemon/p2p.h"
#include "daemon/protocol.h"
#include "daemon/rpc.h"
#include "misc_log_ex.h"
#include "version.h"
#include <boost/program_options.hpp>
#include <functional>

namespace daemonize {

class t_daemon_impl final {
private:
  t_core m_core;
  t_protocol m_protocol;
  t_p2p m_p2p;
  t_rpc m_rpc;
public:
  t_daemon_impl(
      boost::program_options::variables_map const & vm
    )
    : m_core{vm}
    , m_protocol{vm, m_core}
    , m_p2p{vm, m_protocol}
    , m_rpc{vm, m_core, m_p2p}
  {
    // Handle circular dependencies
    m_protocol.set_p2p_endpoint(m_p2p.get());
    m_core.set_protocol(m_protocol.get());
  }

  void run()
  {
    tools::signal_handler::install(std::bind(&daemonize::t_daemon_impl::stop, this));
    m_rpc.run();
    m_p2p.run();
    m_rpc.stop();
    LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  }

  void stop()
  {
    m_p2p.stop();
    m_rpc.stop();
  }
};

t_daemon t_daemon::create(
    boost::program_options::variables_map const & vm
  , bool has_console
  )
{
  if (has_console)
  {
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }
  LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
  return t_daemon{vm};
}

t_daemon::t_daemon(
    boost::program_options::variables_map const & vm
  )
  : mp_impl{new t_daemon_impl{vm}}
{}

t_daemon::~t_daemon() = default;

// MSVC is brain-dead and can't default this...
t_daemon::t_daemon(t_daemon && other)
{
  if (this != &other)
  {
    mp_impl = std::move(other.mp_impl);
    other.mp_impl.reset(nullptr);
  }
}

// or this
t_daemon & t_daemon::operator=(t_daemon && other)
{
  if (this != &other)
  {
    mp_impl = std::move(other.mp_impl);
    other.mp_impl.reset(nullptr);
  }
  return *this;
}

void t_daemon::run()
{
  mp_impl->run();
}

void t_daemon::stop()
{
  mp_impl->stop();
}

} // namespace daemonize
