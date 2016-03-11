// Copyright (c) 2014-2016, The Monero Project
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

#pragma once

#ifdef WIN32

#undef UNICODE
#undef _UNICODE

#include "daemonizer/windows_service.h"
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace windows {
  namespace
  {
    std::vector<char> vecstring(std::string const & str)
    {
      std::vector<char> result{str.begin(), str.end()};
      result.push_back('\0');
      return result;
    }
  }

  template <typename T_handler>
  class t_service_runner final
  {
  private:
    SERVICE_STATUS_HANDLE m_status_handle{nullptr};
    SERVICE_STATUS m_status{};
    boost::mutex m_lock{};
    std::string m_name;
    T_handler m_handler;

    static std::unique_ptr<t_service_runner<T_handler>> sp_instance;
  public:
    t_service_runner(
        std::string name
      , T_handler handler
      )
      : m_name{std::move(name)}
      , m_handler{std::move(handler)}
    {
      m_status.dwServiceType = SERVICE_WIN32;
      m_status.dwCurrentState = SERVICE_STOPPED;
      m_status.dwControlsAccepted = 0;
      m_status.dwWin32ExitCode = NO_ERROR;
      m_status.dwServiceSpecificExitCode = NO_ERROR;
      m_status.dwCheckPoint = 0;
      m_status.dwWaitHint = 0;
    }

    t_service_runner & operator=(t_service_runner && other)
    {
      if (this != &other)
      {
        m_status_handle = std::move(other.m_status_handle);
        m_status = std::move(other.m_status);
        m_name = std::move(other.m_name);
        m_handler = std::move(other.m_handler);
      }
      return *this;
    }

    static void run(
        std::string name
      , T_handler handler
      )
    {
      sp_instance.reset(new t_service_runner<T_handler>{
        std::move(name)
      , std::move(handler)
      });

      sp_instance->run_();
    }

  private:
    void run_()
    {
      SERVICE_TABLE_ENTRY table[] =
      {
        { vecstring(m_name).data(), &service_main }
      , { 0, 0 }
      };

      StartServiceCtrlDispatcher(table);
    }

    void report_status(DWORD status)
    {
      m_status.dwCurrentState = status;
      if (status == SERVICE_RUNNING)
      {
        m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
      }
      else if(status == SERVICE_STOP_PENDING)
      {
        m_status.dwControlsAccepted = 0;
      }
      SetServiceStatus(m_status_handle, &m_status);
    }

    static void WINAPI service_main(DWORD argc, LPSTR * argv)
    {
      sp_instance->service_main_(argc, argv);
    }

    void service_main_(DWORD argc, LPSTR * argv)
    {
      m_status_handle = RegisterServiceCtrlHandler(m_name.c_str(), &on_state_change_request);
      if (m_status_handle == nullptr) return;

      report_status(SERVICE_START_PENDING);

      report_status(SERVICE_RUNNING);

      m_handler.run();

      on_state_change_request_(SERVICE_CONTROL_STOP);

      // Ensure that the service is uninstalled
      uninstall_service(m_name);
    }

    static void WINAPI on_state_change_request(DWORD control_code)
    {
      sp_instance->on_state_change_request_(control_code);
    }

    void on_state_change_request_(DWORD control_code)
    {
      switch (control_code)
      {
        case SERVICE_CONTROL_INTERROGATE:
          break;
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
          report_status(SERVICE_STOP_PENDING);
          m_handler.stop();
          report_status(SERVICE_STOPPED);
          break;
        case SERVICE_CONTROL_PAUSE:
          break;
        case SERVICE_CONTROL_CONTINUE:
          break;
        default:
          break;
      }
    }
  };

  template <typename T_handler>
  std::unique_ptr<t_service_runner<T_handler>> t_service_runner<T_handler>::sp_instance;
}

#endif
