#undef UNICODE
#undef _UNICODE

#include "common/scoped_message_writer.h"
#include "daemon/windows_service.h"
#include "string_tools.h"
#include <iostream>
#include <utility>
#include <memory>
#include <windows.h>

namespace windows {

namespace {
  typedef std::unique_ptr<std::remove_pointer<SC_HANDLE>::type, decltype(&::CloseServiceHandle)> service_handle;
}

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
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << GetLastError();
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
      , SERVICE_DEMAND_START
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
    tools::fail_msg_writer() << "Couldn't create service: " << GetLastError();
    return false;
  }

  tools::success_msg_writer() << "Service installed";
  return true;
}

bool start_service(
    std::string const & service_name
  )
{
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
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << GetLastError();
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
    tools::fail_msg_writer() << "Couldn't find service: " << GetLastError();
    return false;
  }

  if (!StartService(
      p_service.get()
    , 0
    , nullptr
    ))
  {
    tools::fail_msg_writer() << "Service start request failed: " << GetLastError();
    return false;
  }

  tools::success_msg_writer() << "Service started";
  return true;
}

bool stop_service(
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
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << GetLastError();
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
    tools::fail_msg_writer() << "Couldn't find service: " << GetLastError();
    return false;
  }

  SERVICE_STATUS status = {};
  if (!ControlService(p_service.get(), SERVICE_CONTROL_STOP, &status))
  {
    tools::fail_msg_writer() << "Couldn't request service stop: " << GetLastError();
    return false;
  }

  tools::success_msg_writer() << "Service stopped";
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
    tools::fail_msg_writer() << "Couldn't connect to service manager: " << GetLastError();
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
    tools::fail_msg_writer() << "Couldn't find service: " << GetLastError();
    return false;
  }

  SERVICE_STATUS status = {};
  if (!DeleteService(p_service.get()))
  {
    tools::fail_msg_writer() << "Couldn't uninstall service: " << GetLastError();
    return false;
  }

  tools::success_msg_writer() << "Service uninstalled";

  return true;
}

} // namespace windows
