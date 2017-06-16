/*
 * winrc/w_inst.h - install and remove functions
 *
 * Copyright (c) 2009, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * Contains install and remove functions that manipulate the
 * windows services API and windows registry.
 */
#include "config.h"
#include "winrc/w_inst.h"
#include "winrc/win_svc.h"

void wsvc_err2str(char* str, size_t len, const char* fixed, DWORD err)
{
	LPTSTR buf;
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
		NULL, err, 0, (LPTSTR)&buf, 0, NULL) == 0) {
		/* could not format error message */
		snprintf(str, len, "%s GetLastError=%d", fixed, (int)err);
		return;
	}
	snprintf(str, len, "%s (err=%d): %s", fixed, (int)err, buf);
	LocalFree(buf);
}

/** exit with windows error */
static void
fatal_win(FILE* out, const char* str)
{
	char e[256];
	wsvc_err2str(e, sizeof(e), str, (int)GetLastError());
        if(out) fprintf(out, "%s\n", e);
        else fprintf(stderr, "%s\n", e);
        exit(1);
}

/** install registry entries for eventlog */
static void
event_reg_install(FILE* out, const char* pathname)
{
	char buf[1024];
	HKEY hk;
	DWORD t;
	if(out) fprintf(out, "install reg entries for %s\n", pathname);
	snprintf(buf, sizeof(buf), "SYSTEM\\CurrentControlSet\\Services"
		"\\EventLog\\Application\\%s", SERVICE_NAME);
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)buf,
		0, /* reserved, mustbezero */
		NULL, /* class of key, ignored */
		REG_OPTION_NON_VOLATILE, /* values saved on disk */
		KEY_WRITE, /* we want write permission */
		NULL, /* use default security descriptor */
		&hk, /* result */
		NULL)) /* not interested if key new or existing */
		fatal_win(out, "could not create registry key");
	
	/* message file */
	if(RegSetValueEx(hk, (LPCTSTR)"EventMessageFile", 
		0, /* reserved, mustbezero */
		REG_EXPAND_SZ, /* value type (string w env subst) */
		(BYTE*)pathname, /* data */
		(DWORD)strlen(pathname)+1)) /* length of data */
	{
		RegCloseKey(hk);
		fatal_win(out, "could not registry set EventMessageFile");
	}

	/* event types */
	t = EVENTLOG_SUCCESS | EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE
		| EVENTLOG_INFORMATION_TYPE;
	if(RegSetValueEx(hk, (LPCTSTR)"TypesSupported", 0, REG_DWORD, 
		(LPBYTE)&t, sizeof(t))) {
		RegCloseKey(hk);
		fatal_win(out, "could not registry set TypesSupported");
	}

	/* category message file */
	if(RegSetValueEx(hk, (LPCTSTR)"CategoryMessageFile", 0, REG_EXPAND_SZ, 
		(BYTE*)pathname, (DWORD)strlen(pathname)+1)) {
		RegCloseKey(hk);
		fatal_win(out, "could not registry set CategoryMessageFile");
	}
	t = 1;
	if(RegSetValueEx(hk, (LPCTSTR)"CategoryCount", 0, REG_DWORD, 
		(LPBYTE)&t, sizeof(t))) {
		RegCloseKey(hk);
		fatal_win(out, "could not registry set CategoryCount");
	}


	RegCloseKey(hk);
	if(out) fprintf(out, "installed reg entries\n");
}

/** remove registry entries for eventlog */
static void
event_reg_remove(FILE* out)
{
	char buf[1024];
	HKEY hk;
	if(out) fprintf(out, "remove reg entries\n");
	snprintf(buf, sizeof(buf), "SYSTEM\\CurrentControlSet\\Services"
		"\\EventLog\\Application");
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)buf,
		0, /* reserved, mustbezero */
		NULL, /* class of key, ignored */
		REG_OPTION_NON_VOLATILE, /* values saved on disk */
		DELETE, /* we want key delete permission */
		NULL, /* use default security descriptor */
		&hk, /* result */
		NULL)) /* not interested if key new or existing */
		fatal_win(out, "could not open registry key");
	if(RegDeleteKey(hk, (LPCTSTR)SERVICE_NAME)) {
		RegCloseKey(hk);
		fatal_win(out, "could not delete registry key");
	}
	RegCloseKey(hk);
	if(out) fprintf(out, "removed reg entries\n");
}

/** 
 * put quotes around string. Needs one space in front 
 * @param out: debugfile
 * @param str: to be quoted.
 * @param maxlen: max length of the string buffer.
 */
static void
quote_it(FILE* out, char* str, size_t maxlen)
{
        if(strlen(str) == maxlen) {
                if(out) fprintf(out, "string too long %s", str);
                exit(1);
        }
        str[0]='"';
        str[strlen(str)+1]=0;
        str[strlen(str)]='"';
}

/** change suffix */
static void
change(FILE* out, char* path, size_t max, const char* from, const char* to)
{
        size_t fromlen = strlen(from);
        size_t tolen = strlen(to);
        size_t pathlen = strlen(path);
        if(pathlen - fromlen + tolen >= max) {
                if(out) fprintf(out, "string too long %s", path);
                exit(1);
        }
        snprintf(path+pathlen-fromlen, max-(pathlen-fromlen), "%s", to);
}

/* Install service in servicecontrolmanager */
void
wsvc_install(FILE* out, const char* rename)
{
        SC_HANDLE scm;
        SC_HANDLE sv;
        TCHAR path[2*MAX_PATH+4+256];
        TCHAR path_config[2*MAX_PATH+4+256];
        if(out) fprintf(out, "installing unbound service\n");
        if(!GetModuleFileName(NULL, path+1, MAX_PATH))
                fatal_win(out, "could not GetModuleFileName");
        /* change 'unbound-service-install' to 'unbound' */
	if(rename) {
        	change(out, path+1, sizeof(path)-1, rename, "unbound.exe");
		memmove(path_config+1, path+1, sizeof(path)-1);
        	change(out, path_config+1, sizeof(path_config)-1,
			"unbound.exe", "service.conf");
	}

	event_reg_install(out, path+1);

        /* have to quote it because of spaces in directory names */
        /* could append arguments to be sent to ServiceMain */
        quote_it(out, path, sizeof(path));

	/* if we started in a different directory, also read config from it. */
	if(rename) {
        	quote_it(out, path_config, sizeof(path_config));
		strcat(path, " -c ");
		strcat(path, path_config);
	}

        strcat(path, " -w service");
        scm = OpenSCManager(NULL, NULL, (int)SC_MANAGER_CREATE_SERVICE);
        if(!scm) fatal_win(out, "could not OpenSCManager");
        sv = CreateService(
                scm,
                SERVICE_NAME, /* name of service */
                "Unbound DNS validator", /* display name */
                SERVICE_ALL_ACCESS, /* desired access */
                SERVICE_WIN32_OWN_PROCESS, /* service type */
                SERVICE_AUTO_START, /* start type */
                SERVICE_ERROR_NORMAL, /* error control type */
                path, /* path to service's binary */
                NULL, /* no load ordering group */
                NULL, /* no tag identifier */
                NULL, /* no deps */
		NULL, /* on LocalSystem */
		NULL /* no password */
                );
        if(!sv) {
                CloseServiceHandle(scm);
                fatal_win(out, "could not CreateService");
        }
        CloseServiceHandle(sv);
        CloseServiceHandle(scm);
        if(out) fprintf(out, "unbound service installed\n");
}


/* Remove installed service from servicecontrolmanager */
void
wsvc_remove(FILE* out)
{
        SC_HANDLE scm;
        SC_HANDLE sv;
        if(out) fprintf(out, "removing unbound service\n");
        scm = OpenSCManager(NULL, NULL, (int)SC_MANAGER_ALL_ACCESS);
        if(!scm) fatal_win(out, "could not OpenSCManager");
        sv = OpenService(scm, SERVICE_NAME, DELETE);
        if(!sv) {
                CloseServiceHandle(scm);
                fatal_win(out, "could not OpenService");
        }
        if(!DeleteService(sv)) {
		CloseServiceHandle(sv);
		CloseServiceHandle(scm);
                fatal_win(out, "could not DeleteService");
        }
        CloseServiceHandle(sv);
        CloseServiceHandle(scm);
	event_reg_remove(out);
        if(out) fprintf(out, "unbound service removed\n");
}


/* Start daemon */
void
wsvc_rc_start(FILE* out)
{
	SC_HANDLE scm;
	SC_HANDLE sv;
        if(out) fprintf(out, "start unbound service\n");
	scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!scm) fatal_win(out, "could not OpenSCManager");
	sv = OpenService(scm, SERVICE_NAME, SERVICE_START);
	if(!sv) {
		CloseServiceHandle(scm);
		fatal_win(out, "could not OpenService");
	}
	if(!StartService(sv, 0, NULL)) {
		CloseServiceHandle(sv);
		CloseServiceHandle(scm);
		fatal_win(out, "could not StartService");
	}
	CloseServiceHandle(sv);
	CloseServiceHandle(scm);
        if(out) fprintf(out, "unbound service started\n");
}


/* Stop daemon */
void
wsvc_rc_stop(FILE* out)
{
	SC_HANDLE scm;
	SC_HANDLE sv;
	SERVICE_STATUS st;
        if(out) fprintf(out, "stop unbound service\n");
	scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!scm) fatal_win(out, "could not OpenSCManager");
	sv = OpenService(scm, SERVICE_NAME, SERVICE_STOP);
	if(!sv) {
		CloseServiceHandle(scm);
		fatal_win(out, "could not OpenService");
	}
	if(!ControlService(sv, SERVICE_CONTROL_STOP, &st)) {
		CloseServiceHandle(sv);
		CloseServiceHandle(scm);
		fatal_win(out, "could not ControlService");
	}
	CloseServiceHandle(sv);
	CloseServiceHandle(scm);
        if(out) fprintf(out, "unbound service stopped\n");
}
