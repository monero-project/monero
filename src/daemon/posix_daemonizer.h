#pragma once

#ifndef WIN32

#include "daemon/daemon.h"
#include "daemon/daemonizer.h"

namespace daemonize
{
  namespace daemonizer
  {
    namespace
    {
      const command_line::arg_descriptor<bool> arg_detach = {
        "detach"
      , "Run as daemon"
      };
    }

    void init_options(
        boost::program_options::options_description & hidden_options
      , boost::program_options::options_description & normal_options
      , boost::program_options::options_description & configurable_options
      )
    {
      command_line::add_arg(normal_options, arg_detach);
    }

    template <typename T_executor>
    bool daemonize(
        int argc, char const * argv[]
      , T_executor & executor
      , boost::program_options::variables_map const & vm
      )
    {
      if (command_line::arg_present(vm, arg_detach))
      {
        posix::fork();
      }
      else
      {
        //LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
        executor.create_daemon(vm).run();
      }
    }
  }
}

#endif
