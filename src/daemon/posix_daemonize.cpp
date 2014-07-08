#include "common/command_line.h"
#include "daemon/daemon.h"
#include "daemon/daemonize.h"
#include "daemon/posix_fork.h"

namespace daemonize {

namespace {
  const command_line::arg_descriptor<bool> arg_detach = {"detach", "Run as daemon"};
}

void init_options(
    boost::program_options::options_description & visible
  , boost::program_options::options_description & all
  )
{
  command_line::add_arg(visible, arg_detach);
}

void daemonize(boost::program_options::variables_map const & vm)
{
  if (command_line::arg_present(vm, arg_detach))
  {
    std::cout << "forking to background..." << std::endl;
    posix_fork();
    t_daemon::create(vm, false).run();
  }
  else
  {
    t_daemon::create(vm, true).run();
  }
}

bool unregister()
{
  return false;
}

}
