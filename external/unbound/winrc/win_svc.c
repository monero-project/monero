/*
 * winrc/win_svc.c - windows services API implementation for unbound
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
 * This file contains functions to integrate with the windows services API.
 * This means it handles the commandline switches to install and remove
 * the service (via CreateService and DeleteService), it handles
 * the ServiceMain() main service entry point when started as a service,
 * and it handles the Handler[_ex]() to process requests to the service
 * (such as start and stop and status).
 */
#include "config.h"
#include "winrc/win_svc.h"
#include "winrc/w_inst.h"
#include "daemon/daemon.h"
#include "daemon/worker.h"
#include "daemon/remote.h"
#include "util/config_file.h"
#include "util/netevent.h"
#include "util/ub_event.h"

/** global service status */
static SERVICE_STATUS	service_status;
/** global service status handle */
static SERVICE_STATUS_HANDLE service_status_handle;
/** global service stop event */
static WSAEVENT service_stop_event = NULL;
/** event struct for stop callbacks */
static struct ub_event* service_stop_ev = NULL;
/** if stop even means shutdown or restart */
static int service_stop_shutdown = 0;
/** config file to open. global communication to service_main() */
static char* service_cfgfile = CONFIGFILE;
/** commandline verbosity. global communication to service_main() */
static int service_cmdline_verbose = 0;
/** the cron callback */
static struct comm_timer* service_cron = NULL;
/** the cron thread */
static ub_thread_type cron_thread = NULL;
/** if cron has already done its quick check */
static int cron_was_quick = 0;

/**
 * Report current service status to service control manager
 * @param state: current state
 * @param exitcode: error code (when stopped)
 * @param wait: pending operation estimated time in milliseconds.
 */
static void report_status(DWORD state, DWORD exitcode, DWORD wait)
{
	static DWORD checkpoint = 1;
	service_status.dwCurrentState = state;
	service_status.dwWin32ExitCode = exitcode;
	service_status.dwWaitHint = wait;
	if(state == SERVICE_START_PENDING)
		service_status.dwControlsAccepted = 0;
	else 	service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	if(state == SERVICE_RUNNING || state == SERVICE_STOPPED)
		service_status.dwCheckPoint = 0;
	else	service_status.dwCheckPoint = checkpoint++;
	SetServiceStatus(service_status_handle, &service_status);
}

/**
 * Service control handler. Called by serviceControlManager when a control
 * code is sent to the service (with ControlService).
 * @param ctrl: control code
 */
static void 
hdlr(DWORD ctrl)
{
	if(ctrl == SERVICE_CONTROL_STOP) {
		report_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
		service_stop_shutdown = 1;
		/* send signal to stop */
		if(!WSASetEvent(service_stop_event))
			log_err("Could not WSASetEvent: %s",
				wsa_strerror(WSAGetLastError()));
		return;
	} else {
		/* ctrl == SERVICE_CONTROL_INTERROGATE or whatever */
		/* update status */
		report_status(service_status.dwCurrentState, NO_ERROR, 0);
	}
}

/**
 * report event to system event log
 * For use during startup and shutdown.
 * @param str: the error
 */
static void
reportev(const char* str)
{
	char b[256];
	char e[256];
	HANDLE* s;
	LPCTSTR msg = b;
	/* print quickly to keep GetLastError value */
	wsvc_err2str(e, sizeof(e), str, GetLastError());
	snprintf(b, sizeof(b), "%s: %s", SERVICE_NAME, e);
	s = RegisterEventSource(NULL, SERVICE_NAME);
	if(!s) return;
	ReportEvent(s, /* event log */
		EVENTLOG_ERROR_TYPE, /* event type */
		0, /* event category */
		MSG_GENERIC_ERR, /* event ID (from gen_msg.mc) */
		NULL, /* user security context */
		1, /* numstrings */
		0, /* binary size */
		&msg, /* strings */
		NULL); /* binary data */
	DeregisterEventSource(s);
}

/**
 * Obtain registry string (if it exists).
 * @param key: key string
 * @param name: name of value to fetch.
 * @return malloced string with the result or NULL if it did not
 * exist on an error (logged) was encountered.
 */
static char*
lookup_reg_str(const char* key, const char* name)
{
	HKEY hk = NULL;
	DWORD type = 0;
	BYTE buf[1024];
	DWORD len = (DWORD)sizeof(buf);
	LONG ret;
	char* result = NULL;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hk);
	if(ret == ERROR_FILE_NOT_FOUND)
		return NULL; /* key does not exist */
	else if(ret != ERROR_SUCCESS) {
		reportev("RegOpenKeyEx failed");
		return NULL;
	}
	ret = RegQueryValueEx(hk, (LPCTSTR)name, 0, &type, buf, &len);
	if(RegCloseKey(hk))
		reportev("RegCloseKey");
	if(ret == ERROR_FILE_NOT_FOUND)
		return NULL; /* name does not exist */
	else if(ret != ERROR_SUCCESS) {
		reportev("RegQueryValueEx failed");
		return NULL;
	}
	if(type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ) {
		buf[sizeof(buf)-1] = 0;
		buf[sizeof(buf)-2] = 0; /* for multi_sz */
		result = strdup((char*)buf);
		if(!result) reportev("out of memory");
	}
	return result;
}

/**
 * Obtain registry integer (if it exists).
 * @param key: key string
 * @param name: name of value to fetch.
 * @return integer value (if it exists), or 0 on error.
 */
static int
lookup_reg_int(const char* key, const char* name)
{
	HKEY hk = NULL;
	DWORD type = 0;
	BYTE buf[1024];
	DWORD len = (DWORD)sizeof(buf);
	LONG ret;
	int result = 0;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hk);
	if(ret == ERROR_FILE_NOT_FOUND)
		return 0; /* key does not exist */
	else if(ret != ERROR_SUCCESS) {
		reportev("RegOpenKeyEx failed");
		return 0;
	}
	ret = RegQueryValueEx(hk, (LPCTSTR)name, 0, &type, buf, &len);
	if(RegCloseKey(hk))
		reportev("RegCloseKey");
	if(ret == ERROR_FILE_NOT_FOUND)
		return 0; /* name does not exist */
	else if(ret != ERROR_SUCCESS) {
		reportev("RegQueryValueEx failed");
		return 0;
	}
	if(type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ) {
		buf[sizeof(buf)-1] = 0;
		buf[sizeof(buf)-2] = 0; /* for multi_sz */
		result = atoi((char*)buf);
	} else if(type == REG_DWORD) {
		DWORD r;
		memmove(&r, buf, sizeof(r));
		result = r;
	} 
	return result;
}

/** wait for unbound-anchor process to finish */
static void
waitforubanchor(PROCESS_INFORMATION* pinfo)
{
	/* we have 5 seconds scheduled for it, usually it will be very fast,
	 * with only a UDP message or two (100 msec or so), but the https
	 * connections could take some time */
	DWORD count = 7900;
	DWORD ret = WAIT_TIMEOUT;
	/* decrease timer every 1/10 second, we are still starting up */
	while(ret == WAIT_TIMEOUT) {
		ret = WaitForSingleObject(pinfo->hProcess, 100);
		if(count > 4000) count -= 100;
		else count--; /* go slow, it is taking long */
		if(count > 3000)
			report_status(SERVICE_START_PENDING, NO_ERROR, count);
	}
	verbose(VERB_ALGO, "unbound-anchor done");
	if(ret != WAIT_OBJECT_0) {
		return; /* did not end successfully */
	}
	if(!GetExitCodeProcess(pinfo->hProcess, &ret)) {
		log_err("GetExitCodeProcess failed");
		return;
	}
	verbose(VERB_ALGO, "unbound-anchor exit code is %d", (int)ret);
	if(ret != 0) {
		log_info("The root trust anchor has been updated.");
	}
}


/**
 * Perform root anchor update if so configured, by calling that process
 */
static void
call_root_update(void)
{
	char* rootanchor;
 	rootanchor = lookup_reg_str("Software\\Unbound", "RootAnchor");
	if(rootanchor && strlen(rootanchor)>0) {
		STARTUPINFO sinfo;
		PROCESS_INFORMATION pinfo;
		memset(&pinfo, 0, sizeof(pinfo));
		memset(&sinfo, 0, sizeof(sinfo));
		sinfo.cb = sizeof(sinfo);
		verbose(VERB_ALGO, "rootanchor: %s", rootanchor);
		report_status(SERVICE_START_PENDING, NO_ERROR, 8000);
		if(!CreateProcess(NULL, rootanchor, NULL, NULL, 0, 
			CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo))
			log_err("CreateProcess error for unbound-anchor.exe");
		else {
			waitforubanchor(&pinfo);
			CloseHandle(pinfo.hProcess);
			CloseHandle(pinfo.hThread);
		}
	}
	free(rootanchor);
}

/**
 * Init service. Keeps calling status pending to tell service control
 * manager that this process is not hanging.
 * @param r: restart, true on restart
 * @param d: daemon returned here.
 * @param c: config file returned here.
 * @return false if failed.
 */
static int
service_init(int r, struct daemon** d, struct config_file** c)
{
	struct config_file* cfg = NULL;
	struct daemon* daemon = NULL;

	if(!service_cfgfile) {
		char* newf = lookup_reg_str("Software\\Unbound", "ConfigFile");
		if(newf) service_cfgfile = newf;
		else 	service_cfgfile = strdup(CONFIGFILE);
		if(!service_cfgfile) fatal_exit("out of memory");
	}

	/* create daemon */
	if(r) 	daemon = *d;
	else 	daemon = daemon_init();
	if(!daemon) return 0;
	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2800);

	/* read config */
	cfg = config_create();
	if(!cfg) return 0;
	if(!config_read(cfg, service_cfgfile, daemon->chroot)) {
		if(errno != ENOENT) {
			log_err("error in config file");
			return 0;
		}
		log_warn("could not open config file, using defaults");
	}
	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2600);

	verbose(VERB_QUERY, "winservice - apply settings");
	/* apply settings and init */
	verbosity = cfg->verbosity + service_cmdline_verbose;
	w_config_adjust_directory(cfg);
	if(cfg->directory && cfg->directory[0]) {
		char* dir = cfg->directory;
		if(chdir(dir)) {
			log_err("could not chdir to %s: %s", 
				dir, strerror(errno));
			if(errno != ENOENT)
				return 0;
			log_warn("could not change directory - continuing");
		} else
			verbose(VERB_QUERY, "chdir to %s", dir);
	}
	log_init(cfg->logfile, cfg->use_syslog, cfg->chrootdir);
	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2400);
	verbose(VERB_QUERY, "winservice - apply cfg");
	daemon_apply_cfg(daemon, cfg);

	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2300);
	if(!(daemon->rc = daemon_remote_create(cfg))) {
		log_err("could not set up remote-control");
		daemon_delete(daemon);
		config_delete(cfg);
		return 0;
	}

	/* open ports */
	/* keep reporting that we are busy starting */
	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2200);
	verbose(VERB_QUERY, "winservice - open ports");
	if(!daemon_open_shared_ports(daemon)) return 0;
	verbose(VERB_QUERY, "winservice - ports opened");
	if(!r) report_status(SERVICE_START_PENDING, NO_ERROR, 2000);

	*d = daemon;
	*c = cfg;
	return 1;
}

/**
 * Deinit the service
 */
static void
service_deinit(struct daemon* daemon, struct config_file* cfg)
{
	daemon_cleanup(daemon);
	config_delete(cfg);
	daemon_delete(daemon);
}

#ifdef DOXYGEN
#define ATTR_UNUSED(x) x
#endif
/**
 * The main function for the service.
 * Called by the services API when starting unbound on windows in background.
 * Arguments could have been present in the string 'path'.
 * @param argc: nr args
 * @param argv: arg text.
 */
static void 
service_main(DWORD ATTR_UNUSED(argc), LPTSTR* ATTR_UNUSED(argv))
{
	struct config_file* cfg = NULL;
	struct daemon* daemon = NULL;

	service_status_handle = RegisterServiceCtrlHandler(SERVICE_NAME, 
		(LPHANDLER_FUNCTION)hdlr);
	if(!service_status_handle) {
		reportev("Could not RegisterServiceCtrlHandler");
		return;
	}
	
	service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	service_status.dwServiceSpecificExitCode = 0;

	/* see if we have root anchor update enabled */
	call_root_update();

	/* we are now starting up */
	report_status(SERVICE_START_PENDING, NO_ERROR, 3000);
	if(!service_init(0, &daemon, &cfg)) {
		reportev("Could not service_init");
		report_status(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	/* event that gets signalled when we want to quit; it
	 * should get registered in the worker-0 waiting loop. */
	service_stop_event = WSACreateEvent();
	if(service_stop_event == WSA_INVALID_EVENT) {
		log_err("WSACreateEvent: %s", wsa_strerror(WSAGetLastError()));
		reportev("Could not WSACreateEvent");
		report_status(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
	if(!WSAResetEvent(service_stop_event)) {
		log_err("WSAResetEvent: %s", wsa_strerror(WSAGetLastError()));
	}

	/* SetServiceStatus SERVICE_RUNNING;*/
	report_status(SERVICE_RUNNING, NO_ERROR, 0);
	verbose(VERB_QUERY, "winservice - init complete");

	/* daemon performs work */
	while(!service_stop_shutdown) {
		daemon_fork(daemon);
		if(!service_stop_shutdown) {
			daemon_cleanup(daemon);
			config_delete(cfg); cfg=NULL;
			if(!service_init(1, &daemon, &cfg)) {
				reportev("Could not service_init");
				report_status(SERVICE_STOPPED, NO_ERROR, 0);
				return;
			}
		}
	}

	/* exit */
	verbose(VERB_ALGO, "winservice - cleanup.");
	report_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
	if(service_stop_event) (void)WSACloseEvent(service_stop_event);
	service_deinit(daemon, cfg);
	free(service_cfgfile);
	verbose(VERB_QUERY, "winservice - full stop");
	report_status(SERVICE_STOPPED, NO_ERROR, 0);
}

/** start the service */
static void 
service_start(const char* cfgfile, int v, int c)
{
	SERVICE_TABLE_ENTRY myservices[2] = {
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)service_main},
		{NULL, NULL} };
	verbosity=v;
	if(verbosity >= VERB_QUERY) {
		/* log to file about start sequence */
		fclose(fopen("C:\\unbound.log", "w"));
		log_init("C:\\unbound.log", 0, 0);
		verbose(VERB_QUERY, "open logfile");
	} else log_init(0, 1, 0); /* otherwise, use Application log */
	if(c) {
		service_cfgfile = strdup(cfgfile);
		if(!service_cfgfile) fatal_exit("out of memory");
	} else 	service_cfgfile = NULL;
	service_cmdline_verbose = v;
	/* this call returns when service has stopped. */
	if(!StartServiceCtrlDispatcher(myservices)) {
		reportev("Could not StartServiceCtrlDispatcher");
	}
}

void
wsvc_command_option(const char* wopt, const char* cfgfile, int v, int c)
{
	if(strcmp(wopt, "install") == 0)
		wsvc_install(stdout, NULL);
	else if(strcmp(wopt, "remove") == 0)
		wsvc_remove(stdout);
	else if(strcmp(wopt, "service") == 0)
		service_start(cfgfile, v, c);
	else if(strcmp(wopt, "start") == 0)
		wsvc_rc_start(stdout);
	else if(strcmp(wopt, "stop") == 0)
		wsvc_rc_stop(stdout);
	else fatal_exit("unknown option: %s", wopt);
	exit(0);
}

void
worker_win_stop_cb(int ATTR_UNUSED(fd), short ATTR_UNUSED(ev), void* arg)
{
        struct worker* worker = (struct worker*)arg;
        verbose(VERB_QUERY, "caught stop signal (wsaevent)");
        worker->need_to_exit = 1;
        comm_base_exit(worker->base);
}

/** wait for cron process to finish */
static void
waitforit(PROCESS_INFORMATION* pinfo)
{
	DWORD ret = WaitForSingleObject(pinfo->hProcess, INFINITE);
	verbose(VERB_ALGO, "cronaction done");
	if(ret != WAIT_OBJECT_0) {
		return; /* did not end successfully */
	}
	if(!GetExitCodeProcess(pinfo->hProcess, &ret)) {
		log_err("GetExitCodeProcess failed");
		return;
	}
	verbose(VERB_ALGO, "exit code is %d", (int)ret);
	if(ret != 1) {
		if(!WSASetEvent(service_stop_event))
			log_err("Could not WSASetEvent: %s",
			wsa_strerror(WSAGetLastError()));
	}
}

/** Do the cron action and wait for result exit value */
static void*
win_do_cron(void* ATTR_UNUSED(arg))
{
	int mynum=65;
	char* cronaction;
	log_thread_set(&mynum);
 	cronaction = lookup_reg_str("Software\\Unbound", "CronAction");
	if(cronaction && strlen(cronaction)>0) {
		STARTUPINFO sinfo;
		PROCESS_INFORMATION pinfo;
		memset(&pinfo, 0, sizeof(pinfo));
		memset(&sinfo, 0, sizeof(sinfo));
		sinfo.cb = sizeof(sinfo);
		verbose(VERB_ALGO, "cronaction: %s", cronaction);
		if(!CreateProcess(NULL, cronaction, NULL, NULL, 0, 
			CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo))
			log_err("CreateProcess error");
		else {
			waitforit(&pinfo);
			CloseHandle(pinfo.hProcess);
			CloseHandle(pinfo.hThread);
		}
	}
	free(cronaction);
	/* stop self */
	CloseHandle(cron_thread);
	cron_thread = NULL;
	return NULL;
}

/** Set the timer for cron for the next wake up */
static void
set_cron_timer(void)
{
	struct timeval tv;
	int crontime;
	if(cron_was_quick == 0) {
		cron_was_quick = 1;
		crontime = 3600; /* first update some time after boot */
	} else {
		crontime = lookup_reg_int("Software\\Unbound", "CronTime");
		if(crontime == 0) crontime = 60*60*24; /* 24 hours */
	}
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = (time_t)crontime;
	comm_timer_set(service_cron, &tv);
}

void
wsvc_cron_cb(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	/* perform cronned operation */
	verbose(VERB_ALGO, "cron timer callback");
	if(cron_thread == NULL) {
		/* create new thread to do it */
		ub_thread_create(&cron_thread, win_do_cron, worker);
	}
	/* reschedule */
	set_cron_timer();
}

void wsvc_setup_worker(struct worker* worker)
{
	/* if not started with -w service, do nothing */
	if(!service_stop_event)
		return;
	if(!(service_stop_ev = ub_winsock_register_wsaevent(
		comm_base_internal(worker->base), service_stop_event,
		&worker_win_stop_cb, worker))) {
		fatal_exit("could not register wsaevent");
		return;
	}
	if(!service_cron) {
		service_cron = comm_timer_create(worker->base, 
			wsvc_cron_cb, worker);
		if(!service_cron)
			fatal_exit("could not create cron timer");
		set_cron_timer();
	}
}

void wsvc_desetup_worker(struct worker* ATTR_UNUSED(worker))
{
	comm_timer_delete(service_cron);
	service_cron = NULL;
}
