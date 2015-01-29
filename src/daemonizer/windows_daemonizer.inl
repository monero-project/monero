#pragma once

#include "common/util.h"
#include "daemonizer/windows_service.h"
#include "daemonizer/windows_service_runner.h"

#include <shlobj.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace daemonizer
{
  namespace
  {
    const command_line::arg_descriptor<bool> arg_install_service = {
      "install-service"
    , "Install Windows service"
    };
    const command_line::arg_descriptor<bool> arg_uninstall_service = {
      "uninstall-service"
    , "Uninstall Windows service"
    };
    const command_line::arg_descriptor<bool> arg_start_service = {
      "start-service"
    , "Start Windows service"
    };
    const command_line::arg_descriptor<bool> arg_stop_service = {
      "stop-service"
    , "Stop Windows service"
    };
    const command_line::arg_descriptor<bool> arg_is_service = {
      "run-as-service"
    , "Hidden -- true if running as windows service"
    };

    std::string get_argument_string(int argc, char const * argv[])
    {
      std::string result = "";
      for (int i = 1; i < argc; ++i)
      {
        result += " " + std::string{argv[i]};
      }
      return result;
    }
  }

  inline void init_options(
      boost::program_options::options_description & hidden_options
    , boost::program_options::options_description & normal_options
    )
  {
    command_line::add_arg(normal_options, arg_install_service);
    command_line::add_arg(normal_options, arg_uninstall_service);
    command_line::add_arg(normal_options, arg_start_service);
    command_line::add_arg(normal_options, arg_stop_service);
    command_line::add_arg(hidden_options, arg_is_service);
  }

  inline boost::filesystem::path get_default_data_dir()
  {
    bool admin;
    if (!windows::check_admin(admin))
    {
      admin = false;
    }
    if (admin)
    {
      return boost::filesystem::absolute(
          tools::get_special_folder_path(CSIDL_COMMON_APPDATA, true) + "\\" + CRYPTONOTE_NAME
        );
    }
    else
    {
      return boost::filesystem::absolute(
          tools::get_special_folder_path(CSIDL_APPDATA, true) + "\\" + CRYPTONOTE_NAME
        );
    }
  }

  inline boost::filesystem::path get_relative_path_base(
      boost::program_options::variables_map const & vm
    )
  {
    if (command_line::arg_present(vm, arg_is_service))
    {
      if (command_line::arg_present(vm, command_line::arg_data_dir))
      {
        return command_line::get_arg(vm, command_line::arg_data_dir);
      }
      else
      {
        return tools::get_default_data_dir();
      }
    }
    else
    {
      return boost::filesystem::current_path();
    }
  }

  template <typename T_executor>
  inline bool daemonize(
      int argc, char const * argv[]
    , T_executor && executor // universal ref
    , boost::program_options::variables_map const & vm
    )
  {
    std::string arguments = get_argument_string(argc, argv);

    if (command_line::arg_present(vm, arg_is_service))
    {
      // TODO - Set the service status here for return codes
      windows::t_service_runner<typename T_executor::t_daemon>::run(
        executor.name()
      , executor.create_daemon(vm)
      );
      return true;
    }
    else if (command_line::arg_present(vm, arg_install_service))
    {
      if (windows::ensure_admin(arguments))
      {
        arguments += " --run-as-service";
        return windows::install_service(executor.name(), arguments);
      }
    }
    else if (command_line::arg_present(vm, arg_uninstall_service))
    {
      if (windows::ensure_admin(arguments))
      {
        return windows::uninstall_service(executor.name());
      }
    }
    else if (command_line::arg_present(vm, arg_start_service))
    {
      if (windows::ensure_admin(arguments))
      {
        return windows::start_service(executor.name());
      }
    }
    else if (command_line::arg_present(vm, arg_stop_service))
    {
      if (windows::ensure_admin(arguments))
      {
        return windows::stop_service(executor.name());
      }
    }
    else // interactive
    {
      //LOG_PRINT_L0(CRYPTONOTE_NAME << " v" << MONERO_VERSION_FULL);
      return executor.run_interactive(vm);
    }

    return false;
  }
}
