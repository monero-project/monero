#pragma once

#ifndef WIN32

#include "daemonizer/posix_fork.h"

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
      )
    {
      command_line::add_arg(normal_options, arg_detach);
    }

    template <typename T_executor>
    bool daemonize(
        int argc, char const * argv[]
      , T_executor && executor // universal ref
      , boost::program_options::variables_map const & vm
      )
    {
      if (command_line::arg_present(vm, arg_detach))
      {
        posix::fork();
        return executor.create_daemon(vm).run();
      }
      else
      {
        //LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG);
        return executor.run_interactive(vm);
      }
    }
  }
}

#endif
