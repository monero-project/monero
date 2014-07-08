#include "common/command_line.h"
#include "common/scoped_message_writer.h"
#include "daemon/daemon.h"
#include "daemon/daemonize.h"
#include "daemon/windows_service.h"
#include "daemon/windows_service_runner.h"

namespace daemonize {

namespace {
  std::string const SERVICE_NAME = "BitMonero Daemon";
  const command_line::arg_descriptor<bool> arg_detach = {"detach", "Run as daemon"};
  const command_line::arg_descriptor<bool> arg_zombify = {"zombify", "Hidden"};
}

void init_options(
    boost::program_options::options_description & visible
  , boost::program_options::options_description & all
  )
{
  command_line::add_arg(visible, arg_detach);
  command_line::add_arg(all, arg_zombify);
}

/**
 * On first launch, we ask Windows to relaunch the executable as a service
 * with the added --zombify argument, which indicates that the process is
 * running in the background.  On relaunch the --zombify argument is
 * detected, and the t_service_runner class finishes registering as a service
 * and installs the required service lifecycle handler callback.
 */
void daemonize(boost::program_options::variables_map const & vm)
{
  if (command_line::arg_present(vm, arg_zombify))
  {
    windows::t_service_runner<t_daemon>::run(
      SERVICE_NAME
    , t_daemon::create(vm, false)
    );
  }
  else if (command_line::arg_present(vm, arg_detach))
  {
    bool install = windows::install_service(SERVICE_NAME, "--zombify");
    bool start = windows::start_service(SERVICE_NAME);
    if (install && !start)
    {
      windows::uninstall_service(SERVICE_NAME);
    }
  }
  else
  {
    t_daemon::create(vm, true).run();
  }
}

bool unregister()
{
  bool ok = windows::stop_service(SERVICE_NAME);
  ok = windows::uninstall_service(SERVICE_NAME);
  return ok;
}

} // namespace daemonize
