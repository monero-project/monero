/*
 * util/shm_side/shm_main.c - SHM for statistics transport
 *
 * Copyright (c) 2017, NLnet Labs. All rights reserved.
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
 * This file contains functions for the SHM implementation.
 */

#include "config.h"
#include <ctype.h>
#include <stdarg.h>
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include "shm_main.h"
#include "daemon/daemon.h"
#include "daemon/worker.h"
#include "daemon/stats.h"
#include "services/mesh.h"
#include "services/cache/rrset.h"
#include "services/cache/infra.h"
#include "validator/validator.h"
#include "util/config_file.h"
#include "util/fptr_wlist.h"
#include "util/log.h"

#ifdef HAVE_SHMGET
/** subtract timers and the values do not overflow or become negative */
static void
timeval_subtract(struct timeval* d, const struct timeval* end,
	const struct timeval* start)
{
#ifndef S_SPLINT_S
	time_t end_usec = end->tv_usec;
	d->tv_sec = end->tv_sec - start->tv_sec;
	if(end_usec < start->tv_usec) {
		end_usec += 1000000;
		d->tv_sec--;
	}
	d->tv_usec = end_usec - start->tv_usec;
#endif
}
#endif /* HAVE_SHMGET */

int shm_main_init(struct daemon* daemon)
{
#ifdef HAVE_SHMGET
	struct shm_stat_info *shm_stat;
	size_t shm_size;
	
	/* sanitize */
	if(!daemon)
		return 0;
	if(!daemon->cfg->shm_enable)
		return 1;
	if(daemon->cfg->stat_interval == 0)
		log_warn("shm-enable is yes but statistics-interval is 0");

	/* Statistics to maintain the number of thread + total */
	shm_size = (sizeof(struct stats_info) * (daemon->num + 1));

	/* Allocation of needed memory */
	daemon->shm_info = (struct shm_main_info*)calloc(1, shm_size);

	/* Sanitize */
	if(!daemon->shm_info) {
		log_err("shm fail: malloc failure");
		return 0;
	}

	daemon->shm_info->key = daemon->cfg->shm_key;

	/* Check for previous create SHM */
	daemon->shm_info->id_ctl = shmget(daemon->shm_info->key, sizeof(int), SHM_R);
	daemon->shm_info->id_arr = shmget(daemon->shm_info->key + 1, sizeof(int), SHM_R);

	/* Destroy previous SHM */
	if (daemon->shm_info->id_ctl >= 0)
		shmctl(daemon->shm_info->id_ctl, IPC_RMID, NULL);

	/* Destroy previous SHM */
	if (daemon->shm_info->id_arr >= 0)
		shmctl(daemon->shm_info->id_arr, IPC_RMID, NULL);

	/* SHM: Create the segment */
	daemon->shm_info->id_ctl = shmget(daemon->shm_info->key, sizeof(struct shm_stat_info), IPC_CREAT | 0666);

	if (daemon->shm_info->id_ctl < 0)
	{
		log_err("SHM failed(id_ctl) cannot shmget(key %d) %s",
			daemon->shm_info->key, strerror(errno));

		/* Just release memory unused */
		free(daemon->shm_info);

		return 0;
	}

	daemon->shm_info->id_arr = shmget(daemon->shm_info->key + 1, shm_size, IPC_CREAT | 0666);

	if (daemon->shm_info->id_arr < 0)
	{
		log_err("SHM failed(id_arr) cannot shmget(key %d + 1) %s",
			daemon->shm_info->key, strerror(errno));

		/* Just release memory unused */
		free(daemon->shm_info);

		return 0;
	}

	/* SHM: attach the segment  */
	daemon->shm_info->ptr_ctl = (struct shm_stat_info*)
		shmat(daemon->shm_info->id_ctl, NULL, 0);
	if(daemon->shm_info->ptr_ctl == (void *) -1) {
		log_err("SHM failed(ctl) cannot shmat(%d) %s",
			daemon->shm_info->id_ctl, strerror(errno));

		/* Just release memory unused */
		free(daemon->shm_info);

		return 0;
	}

	daemon->shm_info->ptr_arr = (struct stats_info*)
		shmat(daemon->shm_info->id_arr, NULL, 0);

	if (daemon->shm_info->ptr_arr == (void *) -1)
	{
		log_err("SHM failed(arr) cannot shmat(%d) %s",
			daemon->shm_info->id_arr, strerror(errno));

		/* Just release memory unused */
		free(daemon->shm_info);

		return 0;
	}

	/* Zero fill SHM to stand clean while is not filled by other events */
	memset(daemon->shm_info->ptr_ctl, 0, sizeof(struct shm_stat_info));
	memset(daemon->shm_info->ptr_arr, 0, shm_size);

	shm_stat = daemon->shm_info->ptr_ctl;
	shm_stat->num_threads = daemon->num;

#else
	(void)daemon;
#endif /* HAVE_SHMGET */
	return 1;
}

void shm_main_shutdown(struct daemon* daemon)
{
#ifdef HAVE_SHMGET
	/* web are OK, just disabled */
	if(!daemon->cfg->shm_enable)
		return;

	verbose(VERB_DETAIL, "SHM shutdown - KEY [%d] - ID CTL [%d] ARR [%d] - PTR CTL [%p] ARR [%p]",
		daemon->shm_info->key, daemon->shm_info->id_ctl, daemon->shm_info->id_arr, daemon->shm_info->ptr_ctl, daemon->shm_info->ptr_arr);

	/* Destroy previous SHM */
	if (daemon->shm_info->id_ctl >= 0)
		shmctl(daemon->shm_info->id_ctl, IPC_RMID, NULL);

	if (daemon->shm_info->id_arr >= 0)
		shmctl(daemon->shm_info->id_arr, IPC_RMID, NULL);

	if (daemon->shm_info->ptr_ctl)
		shmdt(daemon->shm_info->ptr_ctl);

	if (daemon->shm_info->ptr_arr)
		shmdt(daemon->shm_info->ptr_arr);

#else
	(void)daemon;
#endif /* HAVE_SHMGET */
}

void shm_main_run(struct worker *worker)
{
#ifdef HAVE_SHMGET
	struct shm_stat_info *shm_stat;
	struct stats_info *stat_total;
	struct stats_info *stat_info;
	int modstack;
	int offset;

	verbose(VERB_DETAIL, "SHM run - worker [%d] - daemon [%p] - timenow(%u) - timeboot(%u)",
		worker->thread_num, worker->daemon, (unsigned)worker->env.now_tv->tv_sec, (unsigned)worker->daemon->time_boot.tv_sec);

	offset = worker->thread_num + 1;
	stat_total = worker->daemon->shm_info->ptr_arr;
	stat_info = worker->daemon->shm_info->ptr_arr + offset;

	/* Copy data to the current position */
	server_stats_compile(worker, stat_info, 0);

	/* First thread, zero fill total, and copy general info */
	if (worker->thread_num == 0) {

		/* Copy data to the current position */
		memset(stat_total, 0, sizeof(struct stats_info));

		/* Point to data into SHM */
		shm_stat = worker->daemon->shm_info->ptr_ctl;
		shm_stat->time.now = *worker->env.now_tv;

		timeval_subtract(&shm_stat->time.up, &shm_stat->time.now, &worker->daemon->time_boot);
		timeval_subtract(&shm_stat->time.elapsed, &shm_stat->time.now, &worker->daemon->time_last_stat);

		shm_stat->mem.msg = slabhash_get_mem(worker->env.msg_cache);
		shm_stat->mem.rrset = slabhash_get_mem(&worker->env.rrset_cache->table);
		shm_stat->mem.val = 0;
		shm_stat->mem.iter = 0;

		modstack = modstack_find(&worker->env.mesh->mods, "validator");
		if(modstack != -1) {
			fptr_ok(fptr_whitelist_mod_get_mem(worker->env.mesh->mods.mod[modstack]->get_mem));
			shm_stat->mem.val = (*worker->env.mesh->mods.mod[modstack]->get_mem)(&worker->env, modstack);
		}
		modstack = modstack_find(&worker->env.mesh->mods, "iterator");
		if(modstack != -1) {
			fptr_ok(fptr_whitelist_mod_get_mem(worker->env.mesh->mods.mod[modstack]->get_mem));
			shm_stat->mem.iter = (*worker->env.mesh->mods.mod[modstack]->get_mem)(&worker->env, modstack);
		}
		/* subnet mem value is available in shm, also when not enabled,
		 * to make the struct easier to memmap by other applications,
		 * independent of the configuration of unbound */
		shm_stat->mem.subnet = 0;
#ifdef CLIENT_SUBNET
		modstack = modstack_find(&worker->env.mesh->mods, "subnet");
		if(modstack != -1) {
			fptr_ok(fptr_whitelist_mod_get_mem(worker->env.mesh->mods.mod[modstack]->get_mem));
			shm_stat->mem.subnet = (*worker->env.mesh->mods.mod[modstack]->get_mem)(&worker->env, modstack);
		}
#endif
	}

	server_stats_add(stat_total, stat_info);

	/* print the thread statistics */
	stat_total->mesh_time_median /= (double)worker->daemon->num;

#else
	(void)worker;
#endif /* HAVE_SHMGET */
}
