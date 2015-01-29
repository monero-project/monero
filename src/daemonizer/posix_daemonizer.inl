#pragma once

#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "daemonizer/posix_fork.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace daemonizer
{
  namespace
  {
    const command_line::arg_descriptor<bool> arg_detach = {
      "detach"
    , "Run as daemon"
    };
  }

  inline void init_options(
      boost::program_options::options_description & hidden_options
    , boost::program_options::options_description & normal_options
    )
  {
    command_line::add_arg(normal_options, arg_detach);
  }

  inline boost::filesystem::path get_default_data_dir()
  {
    return boost::filesystem::absolute(tools::get_default_data_dir());
  }

  inline boost::filesystem::path get_relative_path_base(
      boost::program_options::variables_map const & vm
    )
  {
    return boost::filesystem::current_path();
  }

  template <typename T_executor>
  inline bool daemonize(
      int argc, char const * argv[]
    , T_executor && executor // universal ref
    , boost::program_options::variables_map const & vm
    )
  {
    if (command_line::has_arg(vm, arg_detach))
    {
      auto daemon = executor.create_daemon(vm);
      tools::success_msg_writer() << "Forking to background...";
      posix::fork();
      return daemon.run();
    }
    else
    {
      //LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << MONERO_VERSION_FULL);
      return executor.run_interactive(vm);
    }
  }
}
