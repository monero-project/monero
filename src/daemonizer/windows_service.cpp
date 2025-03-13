// Copyright (c) 2014-2024, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#undef UNICODE
#undef _UNICODE

#include "common/scoped_message_writer.h"
#include "daemonizer/windows_service.h"
#include "string_tools.h"
#include <chrono>
#include <iostream>
#include <utility>
#include <memory>
#include <shellapi.h>
#include <thread>
#include <windows.h>
#include <common/util.h>

namespace windows {

  typedef std::unique_ptr<std::remove_pointer<SC_HANDLE>::type, decltype(&::CloseServiceHandle)> service_handle;

bool install_service(
    std::string const & service_name
  , std::string const & arguments
  )
{
  std::string command = epee::string_tools::get_current_module_path();
  std::string full_command = command + arguments;

  service_handle p_manager{
    OpenSCManager(
        nullptr
      , nullptr
      , SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE
      )
  , &::CloseServiceHandle
  };
  if (p_manager == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << tools::get_last_error();
    return false;
  }

  service_handle p_service{
    CreateService(
        p_manager.get()
      , service_name.c_str()
      , service_name.c_str()
      , 0
      //, GENERIC_EXECUTE | GENERIC_READ
      , SERVICE_WIN32_OWN_PROCESS
      , SERVICE_AUTO_START
      , SERVICE_ERROR_NORMAL
      , full_command.c_str()
      , nullptr
      , nullptr
      , ""
      //, "NT AUTHORITY\\LocalService"
      , nullptr // Implies LocalSystem account
      , nullptr
      )
  , &::CloseServiceHandle
  };
  if (p_service == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't create service: " << tools::get_last_error();
    return false;
  }

  tools::success_msg_writer() << "Service installed";

  tools::pause_to_display_admin_window_messages();

  return true;
}

bool start_service(
    std::string const & service_name
  )
{
  tools::msg_writer() << "Starting service";

  SERVICE_STATUS_PROCESS service_status = {};
  DWORD unused = 0;

  service_handle p_manager{
    OpenSCManager(
        nullptr
      , nullptr
      , SC_MANAGER_CONNECT
      )
  , &::CloseServiceHandle
  };
  if (p_manager == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << tools::get_last_error();
    return false;
  }

  service_handle p_service{
    OpenService(
        p_manager.get()
      , service_name.c_str()
      //, SERVICE_START | SERVICE_QUERY_STATUS
      , SERVICE_START
      )
  , &::CloseServiceHandle
  };
  if (p_service == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't find service: " << tools::get_last_error();
    return false;
  }

  if (!StartService(
      p_service.get()
    , 0
    , nullptr
    ))
  {
    tools::fail_msg_writer() << "Service start request failed: " << tools::get_last_error();
    return false;
  }

  tools::success_msg_writer() << "Service started";

  tools::pause_to_display_admin_window_messages();

  return true;
}

bool stop_service(
    std::string const & service_name
  )
{
  tools::msg_writer() << "Stopping service";

  service_handle p_manager{
    OpenSCManager(
        nullptr
      , nullptr
      , SC_MANAGER_CONNECT
      )
  , &::CloseServiceHandle
  };
  if (p_manager == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << tools::get_last_error();
    return false;
  }

  service_handle p_service{
    OpenService(
        p_manager.get()
      , service_name.c_str()
      , SERVICE_STOP | SERVICE_QUERY_STATUS
      )
  , &::CloseServiceHandle
  };
  if (p_service == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't find service: " << tools::get_last_error();
    return false;
  }

  SERVICE_STATUS status = {};
  if (!ControlService(p_service.get(), SERVICE_CONTROL_STOP, &status))
  {
    tools::fail_msg_writer() << "Couldn't request service stop: " << tools::get_last_error();
    return false;
  }

  tools::success_msg_writer() << "Service stopped";

  tools::pause_to_display_admin_window_messages();

  return true;
}

bool uninstall_service(
    std::string const & service_name
  )
{
  service_handle p_manager{
    OpenSCManager(
        nullptr
      , nullptr
      , SC_MANAGER_CONNECT
      )
  , &::CloseServiceHandle
  };
  if (p_manager == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << tools::get_last_error();
    return false;
  }

  service_handle p_service{
    OpenService(
        p_manager.get()
      , service_name.c_str()
      , SERVICE_QUERY_STATUS | DELETE
      )
  , &::CloseServiceHandle
  };
  if (p_service == nullptr)
  {
    tools::fail_msg_writer() << "Couldn't find service: " << tools::get_last_error();
    return false;
  }

  SERVICE_STATUS status = {};
  if (!DeleteService(p_service.get()))
  {
    tools::fail_msg_writer() << "Couldn't uninstall service: " << tools::get_last_error();
    return false;
  }

  tools::success_msg_writer() << "Service uninstalled";

  tools::pause_to_display_admin_window_messages();

  return true;
}

} // namespace windows
