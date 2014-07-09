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
#include <memory>

namespace daemonize {

namespace
{
  struct t_internals {
  private:
    t_protocol protocol;
    t_core core;
  public:
    t_p2p p2p;
    t_rpc rpc;

    t_internals(
        boost::program_options::variables_map const & vm
      )
      : core{vm}
      , protocol{vm, core}
      , p2p{vm, protocol}
      , rpc{vm, core, p2p}
    {
      // Handle circular dependencies
      protocol.set_p2p_endpoint(p2p.get());
      core.set_protocol(protocol.get());
    }
  };
}

class t_daemon_impl final {
private:
  std::unique_ptr<t_internals> mp_internals;
public:
  t_daemon_impl(
      boost::program_options::variables_map const & vm
    )
    : mp_internals{new t_internals{vm}}
  {}

  void run()
  {
    if (nullptr == mp_internals)
    {
      throw std::runtime_error{"Can't run stopped daemon"};
    }
    tools::signal_handler::install(std::bind(&daemonize::t_daemon_impl::stop, this));
    mp_internals->rpc.run();
    mp_internals->p2p.run();
    mp_internals->rpc.stop();
    LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  }

  void stop()
  {
    mp_internals->p2p.stop();
    mp_internals->rpc.stop();
    mp_internals.reset(nullptr); // Ensure resources are cleaned up before we return
  }
};

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
