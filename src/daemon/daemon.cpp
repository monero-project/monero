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

void t_daemon::init_options(boost::program_options::options_description & option_spec)
{
  t_core::init_options(option_spec);
  t_p2p::init_options(option_spec);
  t_rpc::init_options(option_spec);
}

t_daemon::t_daemon(
    boost::program_options::variables_map const & vm
  )
  : mp_internals{new t_internals{vm}}
{}

t_daemon::~t_daemon() = default;

// MSVC is brain-dead and can't default this...
t_daemon::t_daemon(t_daemon && other)
{
  if (this != &other)
  {
    mp_internals = std::move(other.mp_internals);
    other.mp_internals.reset(nullptr);
  }
}

// or this
t_daemon & t_daemon::operator=(t_daemon && other)
{
  if (this != &other)
  {
    mp_internals = std::move(other.mp_internals);
    other.mp_internals.reset(nullptr);
  }
  return *this;
}

void t_daemon::run()
{
  if (nullptr == mp_internals)
  {
    throw std::runtime_error{"Can't run stopped daemon"};
  }
  tools::signal_handler::install(std::bind(&daemonize::t_daemon::stop, this));

  try
  {
    mp_internals->rpc.run();
    mp_internals->p2p.run();
    mp_internals->rpc.stop();
    LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  }
  catch (std::exception const & ex)
  {
    LOG_ERROR("Uncaught exception! " << ex.what());
  }
  catch (...)
  {
    LOG_ERROR("Uncaught exception!");
  }
}

void t_daemon::stop()
{
  if (nullptr == mp_internals)
  {
    throw std::runtime_error{"Can't stop stopped daemon"};
  }
  mp_internals->p2p.stop();
  mp_internals->rpc.stop();
  mp_internals.reset(nullptr); // Ensure resources are cleaned up before we return
}

} // namespace daemonize
