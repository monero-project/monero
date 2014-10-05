/*
 * daemon/daemon.h - collection of workers that handles requests.
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
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
 * The daemon consists of global settings and a number of workers.
 */

#ifndef DAEMON_H
#define DAEMON_H

#include "util/locks.h"
#include "util/alloc.h"
#include "services/modstack.h"
#ifdef UB_ON_WINDOWS
#  include "util/winsock_event.h"
#endif
struct config_file;
struct worker;
struct listen_port;
struct slabhash;
struct module_env;
struct rrset_cache;
struct acl_list;
struct local_zones;
struct ub_randstate;
struct daemon_remote;

#include "dnstap/dnstap_config.h"
#ifdef USE_DNSTAP
struct dt_env;
#endif

/**
 * Structure holding worker list.
 * Holds globally visible information.
 */
struct daemon {
	/** The config settings */
	struct config_file* cfg;
	/** the chroot dir in use, NULL if none */
	char* chroot;
	/** pidfile that is used */
	char* pidfile;
	/** port number that has ports opened. */
	int listening_port;
	/** array of listening ports, opened.  Listening ports per worker,
	 * or just one element[0] shared by the worker threads. */
	struct listen_port** ports;
	/** size of ports array */
	size_t num_ports;
	/** reuseport is enabled if true */
	int reuseport;
	/** port number for remote that has ports opened. */
	int rc_port;
	/** listening ports for remote control */
	struct listen_port* rc_ports;
	/** remote control connections management (for first worker) */
	struct daemon_remote* rc;
	/** ssl context for listening to dnstcp over ssl, and connecting ssl */
	void* listen_sslctx, *connect_sslctx;
	/** num threads allocated */
	int num;
	/** the worker entries */
	struct worker** workers;
	/** do we need to exit unbound (or is it only a reload?) */
	int need_to_exit;
	/** master random table ; used for port div between threads on reload*/
	struct ub_randstate* rand;
	/** master allocation cache */
	struct alloc_cache superalloc;
	/** the module environment master value, copied and changed by threads*/
	struct module_env* env;
	/** stack of module callbacks */
	struct module_stack mods;
	/** access control, which client IPs are allowed to connect */
	struct acl_list* acl;
	/** local authority zones */
	struct local_zones* local_zones;
	/** last time of statistics printout */
	struct timeval time_last_stat;
	/** time when daemon started */
	struct timeval time_boot;
#ifdef USE_DNSTAP
	/** the dnstap environment master value, copied and changed by threads*/
	struct dt_env* dtenv;
#endif
};

/**
 * Initialize daemon structure.
 * @return: The daemon structure, or NULL on error.
 */
struct daemon* daemon_init(void);

/**
 * Open shared listening ports (if needed).
 * The cfg member pointer must have been set for the daemon.
 * @param daemon: the daemon.
 * @return: false on error.
 */
int daemon_open_shared_ports(struct daemon* daemon);

/**
 * Fork workers and start service.
 * When the routine exits, it is no longer forked.
 * @param daemon: the daemon.
 */
void daemon_fork(struct daemon* daemon);

/**
 * Close off the worker thread information.
 * Bring the daemon back into state ready for daemon_fork again.
 * @param daemon: the daemon.
 */
void daemon_cleanup(struct daemon* daemon);

/**
 * Delete workers, close listening ports.
 * @param daemon: the daemon.
 */
void daemon_delete(struct daemon* daemon);

/**
 * Apply config settings.
 * @param daemon: the daemon.
 * @param cfg: new config settings.
 */
void daemon_apply_cfg(struct daemon* daemon, struct config_file* cfg);

#endif /* DAEMON_H */
