// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 


#ifndef _SERVICE_IMPL_BASE_H_
#define _SERVICE_IMPL_BASE_H_

#pragma comment(lib, "advapi32.lib")


namespace epee
{
class service_impl_base {
  public:
    service_impl_base();
    virtual ~service_impl_base();

    virtual const char *get_name() = 0;
    virtual const char *get_caption() = 0;
    virtual const char *get_description() = 0;

    bool run_service();
    virtual bool install();
    virtual bool remove();
    virtual bool init();
    void set_control_accepted(unsigned controls);
    void set_status(unsigned state, unsigned pending = 0);
    unsigned get_control_accepted();

  private:
    virtual void service_main() = 0;
    virtual unsigned service_handler(unsigned control, unsigned event_code,
        void *pdata) = 0;
    //-------------------------------------------------------------------------
    static service_impl_base*& instance();
    //-------------------------------------------------------------------------
    static DWORD __stdcall _service_handler(DWORD control, DWORD event,
        void *pdata, void *pcontext);
    static void __stdcall service_entry(DWORD argc, char **pargs);
    virtual SERVICE_FAILURE_ACTIONSA* get_failure_actions();

  private:
    SC_HANDLE m_manager;
    SC_HANDLE m_service;
    SERVICE_STATUS_HANDLE m_status_handle;
    DWORD m_accepted_control;
};

inline service_impl_base::service_impl_base() {
  m_manager = 0;
  m_service = 0;
  m_status_handle = 0;
  m_accepted_control = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN
      | SERVICE_ACCEPT_PAUSE_CONTINUE;

  instance() = this;
}
//-----------------------------------------------------------------------------
inline service_impl_base::~service_impl_base() {
  if (m_service) {
    ::CloseServiceHandle(m_service);
  }
  m_service = 0;
  if (m_manager) {
    ::CloseServiceHandle(m_manager);
  }
  m_manager = 0;
  instance() = 0;
}
//-----------------------------------------------------------------------------
inline service_impl_base*& service_impl_base::instance() {
  static service_impl_base *pservice = NULL;
  return pservice;
}
//-----------------------------------------------------------------------------
inline
bool service_impl_base::install() {
  CHECK_AND_ASSERT(!m_service, false);
  const char *psz_descr = get_description();
  SERVICE_FAILURE_ACTIONSA* fail_acts = get_failure_actions();

  char sz_path[MAX_PATH];
  ::GetModuleFileNameA(0, sz_path, sizeof(sz_path));
  ::GetShortPathNameA(sz_path, sz_path, sizeof(sz_path));

  while (TRUE) {
    if (!m_manager) {
      m_manager = ::OpenSCManager(NULL, NULL, GENERIC_ALL);
      if (!m_manager) {
        int err = GetLastError();
        LOG_ERROR(
            "Failed to OpenSCManager(), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }
    m_service = ::CreateServiceA(m_manager, get_name(), get_caption(),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE, sz_path, 0, 0, 0, 0, 0);
    if (!m_service) {
      int err = GetLastError();
      LOG_ERROR(
          "Failed to CreateService(), last err="
              << log_space::get_win32_err_descr(err));
      break;
    }

    if (psz_descr) {
      SERVICE_DESCRIPTIONA sd = { (char*) psz_descr };
      if (!::ChangeServiceConfig2A(m_service, SERVICE_CONFIG_DESCRIPTION,
          &sd)) {
        int err = GetLastError();
        LOG_ERROR(
            "Failed to ChangeServiceConfig2(SERVICE_CONFIG_DESCRIPTION), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }

    if (fail_acts) {
      if (!::ChangeServiceConfig2A(m_service, SERVICE_CONFIG_FAILURE_ACTIONS,
          fail_acts)) {
        int err = GetLastError();
        LOG_ERROR(
            "Failed to ChangeServiceConfig2(SERVICE_CONFIG_FAILURE_ACTIONS), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }
    LOG_PRINT("Installed succesfully.", LOG_LEVEL_0);
    return true;
  }
  LOG_PRINT("Failed to install.", LOG_LEVEL_0);
  return false;
}
//-----------------------------------------------------------------------------
inline
bool service_impl_base::remove() {
  CHECK_AND_ASSERT(!m_service, false);

  while (TRUE) {
    if (!m_manager) {
      m_manager = ::OpenSCManager(0, 0, GENERIC_ALL);
      if (!m_manager) {
        int err = GetLastError();
        LOG_ERROR(
            "Failed to OpenSCManager(), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }

    if (!m_service) {
      m_service = ::OpenServiceA(m_manager, get_name(), SERVICE_STOP | DELETE);
      if (!m_service) {
        int err = GetLastError();
        LOG_ERROR(
            "Failed to OpenService(), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }

    SERVICE_STATUS status = { };
    if (!::ControlService(m_service, SERVICE_CONTROL_STOP, &status)) {
      int err = ::GetLastError();
      if (err == ERROR_SHUTDOWN_IN_PROGRESS)
        continue;
      else if (err != ERROR_SERVICE_NOT_ACTIVE) {
        LOG_ERROR(
            "Failed to ControlService(SERVICE_CONTROL_STOP), last err="
                << log_space::get_win32_err_descr(err));
        break;
      }
    }

    if (!::DeleteService(m_service)) {
      int err = ::GetLastError();
      LOG_ERROR(
          "Failed to ControlService(SERVICE_CONTROL_STOP), last err="
              << log_space::get_win32_err_descr(err));
      break;
    }

    LOG_PRINT("Removed successfully.", LOG_LEVEL_0);
    break;
  }

  return true;
}
//-----------------------------------------------------------------------------
inline
bool service_impl_base::init() {
  return true;
}
//-----------------------------------------------------------------------------
inline
bool service_impl_base::run_service() {
  CHECK_AND_ASSERT(!m_service, false);

  long error_code = 0;

  SERVICE_TABLE_ENTRYA service_table[2];
  ZeroMemory(&service_table, sizeof(service_table));

  service_table->lpServiceName = (char*) get_name();
  service_table->lpServiceProc = service_entry;

  LOG_PRINT("[+] Start service control dispatcher for \"" << get_name() << "\"",
      LOG_LEVEL_1);

  error_code = 1;
  BOOL res = ::StartServiceCtrlDispatcherA(service_table);
  if (!res) {
    int err = GetLastError();
    LOG_PRINT(
        "[+] Error starting service control dispatcher, err="
            << log_space::get_win32_err_descr(err), LOG_LEVEL_1);
    return false;
  } else {
    LOG_PRINT("[+] End service control dispatcher for \"" << get_name() << "\"",
        LOG_LEVEL_1);
  }
  return true;
}
//-----------------------------------------------------------------------------
inline DWORD __stdcall service_impl_base::_service_handler(DWORD control,
    DWORD event, void *pdata, void *pcontext) {
  CHECK_AND_ASSERT(pcontext, ERROR_CALL_NOT_IMPLEMENTED);

  service_impl_base *pservice = (service_impl_base*) pcontext;
  return pservice->service_handler(control, event, pdata);
}
//-----------------------------------------------------------------------------
inline
void __stdcall service_impl_base::service_entry(DWORD argc, char **pargs) {
  service_impl_base *pme = instance();
  LOG_PRINT("instance: " << pme, LOG_LEVEL_4);
  if (!pme) {
    LOG_ERROR("Error: at service_entry() pme = NULL");
    return;
  }
  pme->m_status_handle = ::RegisterServiceCtrlHandlerExA(pme->get_name(),
      _service_handler, pme);

  pme->set_status(SERVICE_RUNNING);
  pme->service_main();
  pme->set_status(SERVICE_STOPPED);
}
//-----------------------------------------------------------------------------
inline
void service_impl_base::set_status(unsigned state, unsigned pending) {
  if (!m_status_handle)
    return;

  SERVICE_STATUS status = { 0 };
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = state;
  status.dwControlsAccepted = m_accepted_control;
  /*status.dwWin32ExitCode = NO_ERROR;
   status.dwServiceSpecificExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 0;

   status.dwCurrentState = state;*/

  if (state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING
      || state == SERVICE_CONTINUE_PENDING || state == SERVICE_PAUSE_PENDING) {
    status.dwWaitHint = 2000;
    status.dwCheckPoint = pending;
  }
  ::SetServiceStatus(m_status_handle, &status);
}
//-----------------------------------------------------------------------------------------
inline
void service_impl_base::set_control_accepted(unsigned controls) {
  m_accepted_control = controls;
}
//-----------------------------------------------------------------------------------------
inline
unsigned service_impl_base::get_control_accepted() {
  return m_accepted_control;
}
//-----------------------------------------------------------------------------------------
inline SERVICE_FAILURE_ACTIONSA* service_impl_base::get_failure_actions() {
  // first 3 failures in 30 minutes. Service will be restarted.
  // do nothing for next failures
  static SC_ACTION sa[] = { { SC_ACTION_RESTART, 3 * 1000 }, {
      SC_ACTION_RESTART, 3 * 1000 }, { SC_ACTION_RESTART, 3 * 1000 }, {
      SC_ACTION_NONE, 0 } };

  static SERVICE_FAILURE_ACTIONSA sfa = { 1800,  // interval for failures counter - 30 min
      "", NULL, 4, (SC_ACTION*) &sa };

  // TODO: refactor this code, really unsafe!
  return &sfa;
}
}

#endif //_SERVICE_IMPL_BASE_H_
