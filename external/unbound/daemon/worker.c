/*
 * daemon/worker.c - worker that handles a pending list of requests.
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
 * This file implements the worker that handles callbacks on events, for
 * pending requests.
 */
#include "config.h"
#include "util/log.h"
#include "util/net_help.h"
#include "util/random.h"
#include "daemon/worker.h"
#include "daemon/daemon.h"
#include "daemon/remote.h"
#include "daemon/acl_list.h"
#include "util/netevent.h"
#include "util/config_file.h"
#include "util/module.h"
#include "util/regional.h"
#include "util/storage/slabhash.h"
#include "services/listen_dnsport.h"
#include "services/outside_network.h"
#include "services/outbound_list.h"
#include "services/cache/rrset.h"
#include "services/cache/infra.h"
#include "services/cache/dns.h"
#include "services/mesh.h"
#include "services/localzone.h"
#include "util/data/msgparse.h"
#include "util/data/msgencode.h"
#include "util/data/dname.h"
#include "util/fptr_wlist.h"
#include "util/tube.h"
#include "iterator/iter_fwd.h"
#include "iterator/iter_hints.h"
#include "validator/autotrust.h"
#include "validator/val_anchor.h"
#include "libunbound/context.h"
#include "libunbound/libworker.h"
#include "sldns/sbuffer.h"

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <signal.h>
#ifdef UB_ON_WINDOWS
#include "winrc/win_svc.h"
#endif

/** Size of an UDP datagram */
#define NORMAL_UDP_SIZE	512 /* bytes */
/** ratelimit for error responses */
#define ERROR_RATELIMIT 100 /* qps */

/** 
 * seconds to add to prefetch leeway.  This is a TTL that expires old rrsets
 * earlier than they should in order to put the new update into the cache.
 * This additional value is to make sure that if not all TTLs are equal in
 * the message to be updated(and replaced), that rrsets with up to this much
 * extra TTL are also replaced.  This means that the resulting new message
 * will have (most likely) this TTL at least, avoiding very small 'split
 * second' TTLs due to operators choosing relative primes for TTLs (or so).
 * Also has to be at least one to break ties (and overwrite cached entry).
 */
#define PREFETCH_EXPIRY_ADD 60

#ifdef UNBOUND_ALLOC_STATS
/** measure memory leakage */
static void
debug_memleak(size_t accounted, size_t heap, 
	size_t total_alloc, size_t total_free)
{
	static int init = 0;
	static size_t base_heap, base_accounted, base_alloc, base_free;
	size_t base_af, cur_af, grow_af, grow_acc;
	if(!init) {
		init = 1;
		base_heap = heap;
		base_accounted = accounted;
		base_alloc = total_alloc;
		base_free = total_free;
	}
	base_af = base_alloc - base_free;
	cur_af = total_alloc - total_free;
	grow_af = cur_af - base_af;
	grow_acc = accounted - base_accounted;
	log_info("Leakage: %d leaked. growth: %u use, %u acc, %u heap",
		(int)(grow_af - grow_acc), (unsigned)grow_af, 
		(unsigned)grow_acc, (unsigned)(heap - base_heap));
}

/** give debug heap size indication */
static void
debug_total_mem(size_t calctotal)
{
#ifdef HAVE_SBRK
	extern void* unbound_start_brk;
	extern size_t unbound_mem_alloc, unbound_mem_freed;
	void* cur = sbrk(0);
	int total = cur-unbound_start_brk;
	log_info("Total heap memory estimate: %u  total-alloc: %u  "
		"total-free: %u", (unsigned)total, 
		(unsigned)unbound_mem_alloc, (unsigned)unbound_mem_freed);
	debug_memleak(calctotal, (size_t)total, 
		unbound_mem_alloc, unbound_mem_freed);
#else
	(void)calctotal;
#endif /* HAVE_SBRK */
}
#endif /* UNBOUND_ALLOC_STATS */

/** Report on memory usage by this thread and global */
static void
worker_mem_report(struct worker* ATTR_UNUSED(worker), 
	struct serviced_query* ATTR_UNUSED(cur_serv))
{
#ifdef UNBOUND_ALLOC_STATS
	/* debug func in validator module */
	size_t total, front, back, mesh, msg, rrset, infra, ac, superac;
	size_t me, iter, val, anch;
	int i;
	if(verbosity < VERB_ALGO) 
		return;
	front = listen_get_mem(worker->front);
	back = outnet_get_mem(worker->back);
	msg = slabhash_get_mem(worker->env.msg_cache);
	rrset = slabhash_get_mem(&worker->env.rrset_cache->table);
	infra = infra_get_mem(worker->env.infra_cache);
	mesh = mesh_get_mem(worker->env.mesh);
	ac = alloc_get_mem(&worker->alloc);
	superac = alloc_get_mem(&worker->daemon->superalloc);
	anch = anchors_get_mem(worker->env.anchors);
	iter = 0;
	val = 0;
	for(i=0; i<worker->env.mesh->mods.num; i++) {
		fptr_ok(fptr_whitelist_mod_get_mem(worker->env.mesh->
			mods.mod[i]->get_mem));
		if(strcmp(worker->env.mesh->mods.mod[i]->name, "validator")==0)
			val += (*worker->env.mesh->mods.mod[i]->get_mem)
				(&worker->env, i);
		else	iter += (*worker->env.mesh->mods.mod[i]->get_mem)
				(&worker->env, i);
	}
	me = sizeof(*worker) + sizeof(*worker->base) + sizeof(*worker->comsig)
		+ comm_point_get_mem(worker->cmd_com) 
		+ sizeof(worker->rndstate) 
		+ regional_get_mem(worker->scratchpad) 
		+ sizeof(*worker->env.scratch_buffer) 
		+ sldns_buffer_capacity(worker->env.scratch_buffer)
		+ forwards_get_mem(worker->env.fwds)
		+ hints_get_mem(worker->env.hints);
	if(worker->thread_num == 0)
		me += acl_list_get_mem(worker->daemon->acl);
	if(cur_serv) {
		me += serviced_get_mem(cur_serv);
	}
	total = front+back+mesh+msg+rrset+infra+iter+val+ac+superac+me;
	log_info("Memory conditions: %u front=%u back=%u mesh=%u msg=%u "
		"rrset=%u infra=%u iter=%u val=%u anchors=%u "
		"alloccache=%u globalalloccache=%u me=%u",
		(unsigned)total, (unsigned)front, (unsigned)back, 
		(unsigned)mesh, (unsigned)msg, (unsigned)rrset, 
		(unsigned)infra, (unsigned)iter, (unsigned)val, (unsigned)anch,
		(unsigned)ac, (unsigned)superac, (unsigned)me);
	debug_total_mem(total);
#else /* no UNBOUND_ALLOC_STATS */
	size_t val = 0;
	int i;
	if(verbosity < VERB_QUERY)
		return;
	for(i=0; i<worker->env.mesh->mods.num; i++) {
		fptr_ok(fptr_whitelist_mod_get_mem(worker->env.mesh->
			mods.mod[i]->get_mem));
		if(strcmp(worker->env.mesh->mods.mod[i]->name, "validator")==0)
			val += (*worker->env.mesh->mods.mod[i]->get_mem)
				(&worker->env, i);
	}
	verbose(VERB_QUERY, "cache memory msg=%u rrset=%u infra=%u val=%u",
		(unsigned)slabhash_get_mem(worker->env.msg_cache),
		(unsigned)slabhash_get_mem(&worker->env.rrset_cache->table),
		(unsigned)infra_get_mem(worker->env.infra_cache),
		(unsigned)val);
#endif /* UNBOUND_ALLOC_STATS */
}

void 
worker_send_cmd(struct worker* worker, enum worker_commands cmd)
{
	uint32_t c = (uint32_t)htonl(cmd);
	if(!tube_write_msg(worker->cmd, (uint8_t*)&c, sizeof(c), 0)) {
		log_err("worker send cmd %d failed", (int)cmd);
	}
}

int 
worker_handle_reply(struct comm_point* c, void* arg, int error, 
	struct comm_reply* reply_info)
{
	struct module_qstate* q = (struct module_qstate*)arg;
	struct worker* worker = q->env->worker;
	struct outbound_entry e;
	e.qstate = q;
	e.qsent = NULL;

	if(error != 0) {
		mesh_report_reply(worker->env.mesh, &e, reply_info, error);
		worker_mem_report(worker, NULL);
		return 0;
	}
	/* sanity check. */
	if(!LDNS_QR_WIRE(sldns_buffer_begin(c->buffer))
		|| LDNS_OPCODE_WIRE(sldns_buffer_begin(c->buffer)) != 
			LDNS_PACKET_QUERY
		|| LDNS_QDCOUNT(sldns_buffer_begin(c->buffer)) > 1) {
		/* error becomes timeout for the module as if this reply
		 * never arrived. */
		mesh_report_reply(worker->env.mesh, &e, reply_info, 
			NETEVENT_TIMEOUT);
		worker_mem_report(worker, NULL);
		return 0;
	}
	mesh_report_reply(worker->env.mesh, &e, reply_info, NETEVENT_NOERROR);
	worker_mem_report(worker, NULL);
	return 0;
}

int 
worker_handle_service_reply(struct comm_point* c, void* arg, int error, 
	struct comm_reply* reply_info)
{
	struct outbound_entry* e = (struct outbound_entry*)arg;
	struct worker* worker = e->qstate->env->worker;
	struct serviced_query *sq = e->qsent;

	verbose(VERB_ALGO, "worker svcd callback for qstate %p", e->qstate);
	if(error != 0) {
		mesh_report_reply(worker->env.mesh, e, reply_info, error);
		worker_mem_report(worker, sq);
		return 0;
	}
	/* sanity check. */
	if(!LDNS_QR_WIRE(sldns_buffer_begin(c->buffer))
		|| LDNS_OPCODE_WIRE(sldns_buffer_begin(c->buffer)) != 
			LDNS_PACKET_QUERY
		|| LDNS_QDCOUNT(sldns_buffer_begin(c->buffer)) > 1) {
		/* error becomes timeout for the module as if this reply
		 * never arrived. */
		verbose(VERB_ALGO, "worker: bad reply handled as timeout");
		mesh_report_reply(worker->env.mesh, e, reply_info, 
			NETEVENT_TIMEOUT);
		worker_mem_report(worker, sq);
		return 0;
	}
	mesh_report_reply(worker->env.mesh, e, reply_info, NETEVENT_NOERROR);
	worker_mem_report(worker, sq);
	return 0;
}

/** ratelimit error replies
 * @param worker: the worker struct with ratelimit counter
 * @param err: error code that would be wanted.
 * @return value of err if okay, or -1 if it should be discarded instead.
 */
static int
worker_err_ratelimit(struct worker* worker, int err)
{
	if(worker->err_limit_time == *worker->env.now) {
		/* see if limit is exceeded for this second */
		if(worker->err_limit_count++ > ERROR_RATELIMIT)
			return -1;
	} else {
		/* new second, new limits */
		worker->err_limit_time = *worker->env.now;
		worker->err_limit_count = 1;
	}
	return err;
}

/** check request sanity.
 * @param pkt: the wire packet to examine for sanity.
 * @param worker: parameters for checking.
 * @return error code, 0 OK, or -1 discard.
*/
static int 
worker_check_request(sldns_buffer* pkt, struct worker* worker)
{
	if(sldns_buffer_limit(pkt) < LDNS_HEADER_SIZE) {
		verbose(VERB_QUERY, "request too short, discarded");
		return -1;
	}
	if(sldns_buffer_limit(pkt) > NORMAL_UDP_SIZE && 
		worker->daemon->cfg->harden_large_queries) {
		verbose(VERB_QUERY, "request too large, discarded");
		return -1;
	}
	if(LDNS_QR_WIRE(sldns_buffer_begin(pkt))) {
		verbose(VERB_QUERY, "request has QR bit on, discarded");
		return -1;
	}
	if(LDNS_TC_WIRE(sldns_buffer_begin(pkt))) {
		LDNS_TC_CLR(sldns_buffer_begin(pkt));
		verbose(VERB_QUERY, "request bad, has TC bit on");
		return worker_err_ratelimit(worker, LDNS_RCODE_FORMERR);
	}
	if(LDNS_OPCODE_WIRE(sldns_buffer_begin(pkt)) != LDNS_PACKET_QUERY) {
		verbose(VERB_QUERY, "request unknown opcode %d", 
			LDNS_OPCODE_WIRE(sldns_buffer_begin(pkt)));
		return worker_err_ratelimit(worker, LDNS_RCODE_NOTIMPL);
	}
	if(LDNS_QDCOUNT(sldns_buffer_begin(pkt)) != 1) {
		verbose(VERB_QUERY, "request wrong nr qd=%d", 
			LDNS_QDCOUNT(sldns_buffer_begin(pkt)));
		return worker_err_ratelimit(worker, LDNS_RCODE_FORMERR);
	}
	if(LDNS_ANCOUNT(sldns_buffer_begin(pkt)) != 0) {
		verbose(VERB_QUERY, "request wrong nr an=%d", 
			LDNS_ANCOUNT(sldns_buffer_begin(pkt)));
		return worker_err_ratelimit(worker, LDNS_RCODE_FORMERR);
	}
	if(LDNS_NSCOUNT(sldns_buffer_begin(pkt)) != 0) {
		verbose(VERB_QUERY, "request wrong nr ns=%d", 
			LDNS_NSCOUNT(sldns_buffer_begin(pkt)));
		return worker_err_ratelimit(worker, LDNS_RCODE_FORMERR);
	}
	if(LDNS_ARCOUNT(sldns_buffer_begin(pkt)) > 1) {
		verbose(VERB_QUERY, "request wrong nr ar=%d", 
			LDNS_ARCOUNT(sldns_buffer_begin(pkt)));
		return worker_err_ratelimit(worker, LDNS_RCODE_FORMERR);
	}
	return 0;
}

void 
worker_handle_control_cmd(struct tube* ATTR_UNUSED(tube), uint8_t* msg,
	size_t len, int error, void* arg)
{
	struct worker* worker = (struct worker*)arg;
	enum worker_commands cmd;
	if(error != NETEVENT_NOERROR) {
		free(msg);
		if(error == NETEVENT_CLOSED)
			comm_base_exit(worker->base);
		else	log_info("control event: %d", error);
		return;
	}
	if(len != sizeof(uint32_t)) {
		fatal_exit("bad control msg length %d", (int)len);
	}
	cmd = sldns_read_uint32(msg);
	free(msg);
	switch(cmd) {
	case worker_cmd_quit:
		verbose(VERB_ALGO, "got control cmd quit");
		comm_base_exit(worker->base);
		break;
	case worker_cmd_stats:
		verbose(VERB_ALGO, "got control cmd stats");
		server_stats_reply(worker, 1);
		break;
	case worker_cmd_stats_noreset:
		verbose(VERB_ALGO, "got control cmd stats_noreset");
		server_stats_reply(worker, 0);
		break;
	case worker_cmd_remote:
		verbose(VERB_ALGO, "got control cmd remote");
		daemon_remote_exec(worker);
		break;
	default:
		log_err("bad command %d", (int)cmd);
		break;
	}
}

/** check if a delegation is secure */
static enum sec_status
check_delegation_secure(struct reply_info *rep) 
{
	/* return smallest security status */
	size_t i;
	enum sec_status sec = sec_status_secure;
	enum sec_status s;
	size_t num = rep->an_numrrsets + rep->ns_numrrsets;
	/* check if answer and authority are OK */
	for(i=0; i<num; i++) {
		s = ((struct packed_rrset_data*)rep->rrsets[i]->entry.data)
			->security;
		if(s < sec)
			sec = s;
	}
	/* in additional, only unchecked triggers revalidation */
	for(i=num; i<rep->rrset_count; i++) {
		s = ((struct packed_rrset_data*)rep->rrsets[i]->entry.data)
			->security;
		if(s == sec_status_unchecked)
			return s;
	}
	return sec;
}

/** remove nonsecure from a delegation referral additional section */
static void
deleg_remove_nonsecure_additional(struct reply_info* rep)
{
	/* we can simply edit it, since we are working in the scratch region */
	size_t i;
	enum sec_status s;

	for(i = rep->an_numrrsets+rep->ns_numrrsets; i<rep->rrset_count; i++) {
		s = ((struct packed_rrset_data*)rep->rrsets[i]->entry.data)
			->security;
		if(s != sec_status_secure) {
			memmove(rep->rrsets+i, rep->rrsets+i+1, 
				sizeof(struct ub_packed_rrset_key*)* 
				(rep->rrset_count - i - 1));
			rep->ar_numrrsets--; 
			rep->rrset_count--;
			i--;
		}
	}
}

/** answer nonrecursive query from the cache */
static int
answer_norec_from_cache(struct worker* worker, struct query_info* qinfo,
	uint16_t id, uint16_t flags, struct comm_reply* repinfo, 
	struct edns_data* edns)
{
	/* for a nonrecursive query return either:
	 * 	o an error (servfail; we try to avoid this)
	 * 	o a delegation (closest we have; this routine tries that)
	 * 	o the answer (checked by answer_from_cache) 
	 *
	 * So, grab a delegation from the rrset cache. 
	 * Then check if it needs validation, if so, this routine fails,
	 * so that iterator can prime and validator can verify rrsets.
	 */
	uint16_t udpsize = edns->udp_size;
	int secure = 0;
	time_t timenow = *worker->env.now;
	int must_validate = (!(flags&BIT_CD) || worker->env.cfg->ignore_cd)
		&& worker->env.need_to_validate;
	struct dns_msg *msg = NULL;
	struct delegpt *dp;

	dp = dns_cache_find_delegation(&worker->env, qinfo->qname, 
		qinfo->qname_len, qinfo->qtype, qinfo->qclass,
		worker->scratchpad, &msg, timenow);
	if(!dp) { /* no delegation, need to reprime */
		regional_free_all(worker->scratchpad);
		return 0;
	}
	if(must_validate) {
		switch(check_delegation_secure(msg->rep)) {
		case sec_status_unchecked:
			/* some rrsets have not been verified yet, go and 
			 * let validator do that */
			regional_free_all(worker->scratchpad);
			return 0;
		case sec_status_bogus:
			/* some rrsets are bogus, reply servfail */
			edns->edns_version = EDNS_ADVERTISED_VERSION;
			edns->udp_size = EDNS_ADVERTISED_SIZE;
			edns->ext_rcode = 0;
			edns->bits &= EDNS_DO;
			error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
				&msg->qinfo, id, flags, edns);
			regional_free_all(worker->scratchpad);
			if(worker->stats.extended) {
				worker->stats.ans_bogus++;
				worker->stats.ans_rcode[LDNS_RCODE_SERVFAIL]++;
			}
			return 1;
		case sec_status_secure:
			/* all rrsets are secure */
			/* remove non-secure rrsets from the add. section*/
			if(worker->env.cfg->val_clean_additional)
				deleg_remove_nonsecure_additional(msg->rep);
			secure = 1;
			break;
		case sec_status_indeterminate:
		case sec_status_insecure:
		default:
			/* not secure */
			secure = 0;
			break;
		}
	}
	/* return this delegation from the cache */
	edns->edns_version = EDNS_ADVERTISED_VERSION;
	edns->udp_size = EDNS_ADVERTISED_SIZE;
	edns->ext_rcode = 0;
	edns->bits &= EDNS_DO;
	msg->rep->flags |= BIT_QR|BIT_RA;
	if(!reply_info_answer_encode(&msg->qinfo, msg->rep, id, flags, 
		repinfo->c->buffer, 0, 1, worker->scratchpad,
		udpsize, edns, (int)(edns->bits & EDNS_DO), secure)) {
		error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
			&msg->qinfo, id, flags, edns);
	}
	regional_free_all(worker->scratchpad);
	if(worker->stats.extended) {
		if(secure) worker->stats.ans_secure++;
		server_stats_insrcode(&worker->stats, repinfo->c->buffer);
	}
	return 1;
}

/** answer query from the cache */
static int
answer_from_cache(struct worker* worker, struct query_info* qinfo,
	struct reply_info* rep, uint16_t id, uint16_t flags, 
	struct comm_reply* repinfo, struct edns_data* edns)
{
	time_t timenow = *worker->env.now;
	uint16_t udpsize = edns->udp_size;
	int secure;
	int must_validate = (!(flags&BIT_CD) || worker->env.cfg->ignore_cd)
		&& worker->env.need_to_validate;
	/* see if it is possible */
	if(rep->ttl < timenow) {
		/* the rrsets may have been updated in the meantime.
		 * we will refetch the message format from the
		 * authoritative server 
		 */
		return 0;
	}
	if(!rrset_array_lock(rep->ref, rep->rrset_count, timenow))
		return 0;
	/* locked and ids and ttls are OK. */
	/* check CNAME chain (if any) */
	if(rep->an_numrrsets > 0 && (rep->rrsets[0]->rk.type == 
		htons(LDNS_RR_TYPE_CNAME) || rep->rrsets[0]->rk.type == 
		htons(LDNS_RR_TYPE_DNAME))) {
		if(!reply_check_cname_chain(qinfo, rep)) {
			/* cname chain invalid, redo iterator steps */
			verbose(VERB_ALGO, "Cache reply: cname chain broken");
		bail_out:
			rrset_array_unlock_touch(worker->env.rrset_cache, 
				worker->scratchpad, rep->ref, rep->rrset_count);
			regional_free_all(worker->scratchpad);
			return 0;
		}
	}
	/* check security status of the cached answer */
	if( rep->security == sec_status_bogus && must_validate) {
		/* BAD cached */
		edns->edns_version = EDNS_ADVERTISED_VERSION;
		edns->udp_size = EDNS_ADVERTISED_SIZE;
		edns->ext_rcode = 0;
		edns->bits &= EDNS_DO;
		error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
			qinfo, id, flags, edns);
		rrset_array_unlock_touch(worker->env.rrset_cache, 
			worker->scratchpad, rep->ref, rep->rrset_count);
		regional_free_all(worker->scratchpad);
		if(worker->stats.extended) {
			worker->stats.ans_bogus ++;
			worker->stats.ans_rcode[LDNS_RCODE_SERVFAIL] ++;
		}
		return 1;
	} else if( rep->security == sec_status_unchecked && must_validate) {
		verbose(VERB_ALGO, "Cache reply: unchecked entry needs "
			"validation");
		goto bail_out; /* need to validate cache entry first */
	} else if(rep->security == sec_status_secure) {
		if(reply_all_rrsets_secure(rep))
			secure = 1;
		else	{
			if(must_validate) {
				verbose(VERB_ALGO, "Cache reply: secure entry"
					" changed status");
				goto bail_out; /* rrset changed, re-verify */
			}
			secure = 0;
		}
	} else	secure = 0;

	edns->edns_version = EDNS_ADVERTISED_VERSION;
	edns->udp_size = EDNS_ADVERTISED_SIZE;
	edns->ext_rcode = 0;
	edns->bits &= EDNS_DO;
	if(!reply_info_answer_encode(qinfo, rep, id, flags, 
		repinfo->c->buffer, timenow, 1, worker->scratchpad,
		udpsize, edns, (int)(edns->bits & EDNS_DO), secure)) {
		error_encode(repinfo->c->buffer, LDNS_RCODE_SERVFAIL, 
			qinfo, id, flags, edns);
	}
	/* cannot send the reply right now, because blocking network syscall
	 * is bad while holding locks. */
	rrset_array_unlock_touch(worker->env.rrset_cache, worker->scratchpad,
		rep->ref, rep->rrset_count);
	regional_free_all(worker->scratchpad);
	if(worker->stats.extended) {
		if(secure) worker->stats.ans_secure++;
		server_stats_insrcode(&worker->stats, repinfo->c->buffer);
	}
	/* go and return this buffer to the client */
	return 1;
}

/** Reply to client and perform prefetch to keep cache up to date */
static void
reply_and_prefetch(struct worker* worker, struct query_info* qinfo, 
	uint16_t flags, struct comm_reply* repinfo, time_t leeway)
{
	/* first send answer to client to keep its latency 
	 * as small as a cachereply */
	comm_point_send_reply(repinfo);
	server_stats_prefetch(&worker->stats, worker);
	
	/* create the prefetch in the mesh as a normal lookup without
	 * client addrs waiting, which has the cache blacklisted (to bypass
	 * the cache and go to the network for the data). */
	/* this (potentially) runs the mesh for the new query */
	mesh_new_prefetch(worker->env.mesh, qinfo, flags, leeway + 
		PREFETCH_EXPIRY_ADD);
}

/**
 * Fill CH class answer into buffer. Keeps query.
 * @param pkt: buffer
 * @param str: string to put into text record (<255).
 * @param edns: edns reply information.
 */
static void
chaos_replystr(sldns_buffer* pkt, const char* str, struct edns_data* edns)
{
	size_t len = strlen(str);
	unsigned int rd = LDNS_RD_WIRE(sldns_buffer_begin(pkt));
	unsigned int cd = LDNS_CD_WIRE(sldns_buffer_begin(pkt));
	if(len>255) len=255; /* cap size of TXT record */
	sldns_buffer_clear(pkt);
	sldns_buffer_skip(pkt, (ssize_t)sizeof(uint16_t)); /* skip id */
	sldns_buffer_write_u16(pkt, (uint16_t)(BIT_QR|BIT_RA));
	if(rd) LDNS_RD_SET(sldns_buffer_begin(pkt));
	if(cd) LDNS_CD_SET(sldns_buffer_begin(pkt));
	sldns_buffer_write_u16(pkt, 1); /* qdcount */
	sldns_buffer_write_u16(pkt, 1); /* ancount */
	sldns_buffer_write_u16(pkt, 0); /* nscount */
	sldns_buffer_write_u16(pkt, 0); /* arcount */
	(void)query_dname_len(pkt); /* skip qname */
	sldns_buffer_skip(pkt, (ssize_t)sizeof(uint16_t)); /* skip qtype */
	sldns_buffer_skip(pkt, (ssize_t)sizeof(uint16_t)); /* skip qclass */
	sldns_buffer_write_u16(pkt, 0xc00c); /* compr ptr to query */
	sldns_buffer_write_u16(pkt, LDNS_RR_TYPE_TXT);
	sldns_buffer_write_u16(pkt, LDNS_RR_CLASS_CH);
	sldns_buffer_write_u32(pkt, 0); /* TTL */
	sldns_buffer_write_u16(pkt, sizeof(uint8_t) + len);
	sldns_buffer_write_u8(pkt, len);
	sldns_buffer_write(pkt, str, len);
	sldns_buffer_flip(pkt);
	edns->edns_version = EDNS_ADVERTISED_VERSION;
	edns->udp_size = EDNS_ADVERTISED_SIZE;
	edns->bits &= EDNS_DO;
	attach_edns_record(pkt, edns);
}

/**
 * Answer CH class queries.
 * @param w: worker
 * @param qinfo: query info. Pointer into packet buffer.
 * @param edns: edns info from query.
 * @param pkt: packet buffer.
 * @return: true if a reply is to be sent.
 */
static int
answer_chaos(struct worker* w, struct query_info* qinfo, 
	struct edns_data* edns, sldns_buffer* pkt)
{
	struct config_file* cfg = w->env.cfg;
	if(qinfo->qtype != LDNS_RR_TYPE_ANY && qinfo->qtype != LDNS_RR_TYPE_TXT)
		return 0;
	if(query_dname_compare(qinfo->qname, 
		(uint8_t*)"\002id\006server") == 0 ||
		query_dname_compare(qinfo->qname, 
		(uint8_t*)"\010hostname\004bind") == 0)
	{
		if(cfg->hide_identity) 
			return 0;
		if(cfg->identity==NULL || cfg->identity[0]==0) {
			char buf[MAXHOSTNAMELEN+1];
			if (gethostname(buf, MAXHOSTNAMELEN) == 0) {
				buf[MAXHOSTNAMELEN] = 0;
				chaos_replystr(pkt, buf, edns);
			} else 	{
				log_err("gethostname: %s", strerror(errno));
				chaos_replystr(pkt, "no hostname", edns);
			}
		}
		else 	chaos_replystr(pkt, cfg->identity, edns);
		return 1;
	}
	if(query_dname_compare(qinfo->qname, 
		(uint8_t*)"\007version\006server") == 0 ||
		query_dname_compare(qinfo->qname, 
		(uint8_t*)"\007version\004bind") == 0)
	{
		if(cfg->hide_version) 
			return 0;
		if(cfg->version==NULL || cfg->version[0]==0)
			chaos_replystr(pkt, PACKAGE_STRING, edns);
		else 	chaos_replystr(pkt, cfg->version, edns);
		return 1;
	}
	return 0;
}

static int
deny_refuse(struct comm_point* c, enum acl_access acl,
	enum acl_access deny, enum acl_access refuse,
	struct worker* worker, struct comm_reply* repinfo)
{
	if(acl == deny) {
		comm_point_drop_reply(repinfo);
		if(worker->stats.extended)
			worker->stats.unwanted_queries++;
		return 0;
	} else if(acl == refuse) {
		log_addr(VERB_ALGO, "refused query from",
			&repinfo->addr, repinfo->addrlen);
		log_buf(VERB_ALGO, "refuse", c->buffer);
		if(worker->stats.extended)
			worker->stats.unwanted_queries++;
		if(worker_check_request(c->buffer, worker) == -1) {
			comm_point_drop_reply(repinfo);
			return 0; /* discard this */
		}
		sldns_buffer_set_limit(c->buffer, LDNS_HEADER_SIZE);
		sldns_buffer_write_at(c->buffer, 4, 
			(uint8_t*)"\0\0\0\0\0\0\0\0", 8);
		LDNS_QR_SET(sldns_buffer_begin(c->buffer));
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), 
			LDNS_RCODE_REFUSED);
		return 1;
	}

	return -1;
}

static int
deny_refuse_all(struct comm_point* c, enum acl_access acl,
	struct worker* worker, struct comm_reply* repinfo)
{
	return deny_refuse(c, acl, acl_deny, acl_refuse, worker, repinfo);
}

static int
deny_refuse_non_local(struct comm_point* c, enum acl_access acl,
	struct worker* worker, struct comm_reply* repinfo)
{
	return deny_refuse(c, acl, acl_deny_non_local, acl_refuse_non_local, worker, repinfo);
}

int 
worker_handle_request(struct comm_point* c, void* arg, int error,
	struct comm_reply* repinfo)
{
	struct worker* worker = (struct worker*)arg;
	int ret;
	hashvalue_t h;
	struct lruhash_entry* e;
	struct query_info qinfo;
	struct edns_data edns;
	enum acl_access acl;
	int rc = 0;

	if(error != NETEVENT_NOERROR) {
		/* some bad tcp query DNS formats give these error calls */
		verbose(VERB_ALGO, "handle request called with err=%d", error);
		return 0;
	}
#ifdef USE_DNSTAP
	if(worker->dtenv.log_client_query_messages)
		dt_msg_send_client_query(&worker->dtenv, &repinfo->addr, c->type,
			c->buffer);
#endif
	acl = acl_list_lookup(worker->daemon->acl, &repinfo->addr, 
		repinfo->addrlen);
	if((ret=deny_refuse_all(c, acl, worker, repinfo)) != -1)
	{
		if(ret == 1)
			goto send_reply;
		return ret;
	}
	if((ret=worker_check_request(c->buffer, worker)) != 0) {
		verbose(VERB_ALGO, "worker check request: bad query.");
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		if(ret != -1) {
			LDNS_QR_SET(sldns_buffer_begin(c->buffer));
			LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), ret);
			return 1;
		}
		comm_point_drop_reply(repinfo);
		return 0;
	}
	worker->stats.num_queries++;
	/* see if query is in the cache */
	if(!query_info_parse(&qinfo, c->buffer)) {
		verbose(VERB_ALGO, "worker parse request: formerror.");
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		if(worker_err_ratelimit(worker, LDNS_RCODE_FORMERR) == -1) {
			comm_point_drop_reply(repinfo);
			return 0;
		}
		sldns_buffer_rewind(c->buffer);
		LDNS_QR_SET(sldns_buffer_begin(c->buffer));
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), 
			LDNS_RCODE_FORMERR);
		server_stats_insrcode(&worker->stats, c->buffer);
		goto send_reply;
	}
	if(worker->env.cfg->log_queries) {
		char ip[128];
		addr_to_str(&repinfo->addr, repinfo->addrlen, ip, sizeof(ip));
		log_nametypeclass(0, ip, qinfo.qname, qinfo.qtype, qinfo.qclass);
	}
	if(qinfo.qtype == LDNS_RR_TYPE_AXFR || 
		qinfo.qtype == LDNS_RR_TYPE_IXFR) {
		verbose(VERB_ALGO, "worker request: refused zone transfer.");
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		sldns_buffer_rewind(c->buffer);
		LDNS_QR_SET(sldns_buffer_begin(c->buffer));
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), 
			LDNS_RCODE_REFUSED);
		if(worker->stats.extended) {
			worker->stats.qtype[qinfo.qtype]++;
			server_stats_insrcode(&worker->stats, c->buffer);
		}
		goto send_reply;
	}
	if((ret=parse_edns_from_pkt(c->buffer, &edns)) != 0) {
		struct edns_data reply_edns;
		verbose(VERB_ALGO, "worker parse edns: formerror.");
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		memset(&reply_edns, 0, sizeof(reply_edns));
		reply_edns.edns_present = 1;
		reply_edns.udp_size = EDNS_ADVERTISED_SIZE;
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), ret);
		error_encode(c->buffer, ret, &qinfo,
			*(uint16_t*)(void *)sldns_buffer_begin(c->buffer),
			sldns_buffer_read_u16_at(c->buffer, 2), &reply_edns);
		server_stats_insrcode(&worker->stats, c->buffer);
		goto send_reply;
	}
	if(edns.edns_present && edns.edns_version != 0) {
		edns.ext_rcode = (uint8_t)(EDNS_RCODE_BADVERS>>4);
		edns.edns_version = EDNS_ADVERTISED_VERSION;
		edns.udp_size = EDNS_ADVERTISED_SIZE;
		edns.bits &= EDNS_DO;
		verbose(VERB_ALGO, "query with bad edns version.");
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		error_encode(c->buffer, EDNS_RCODE_BADVERS&0xf, &qinfo,
			*(uint16_t*)(void *)sldns_buffer_begin(c->buffer),
			sldns_buffer_read_u16_at(c->buffer, 2), NULL);
		attach_edns_record(c->buffer, &edns);
		goto send_reply;
	}
	if(edns.edns_present && edns.udp_size < NORMAL_UDP_SIZE &&
		worker->daemon->cfg->harden_short_bufsize) {
		verbose(VERB_QUERY, "worker request: EDNS bufsize %d ignored",
			(int)edns.udp_size);
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		edns.udp_size = NORMAL_UDP_SIZE;
	}
	if(edns.udp_size > worker->daemon->cfg->max_udp_size &&
		c->type == comm_udp) {
		verbose(VERB_QUERY,
			"worker request: max UDP reply size modified"
			" (%d to max-udp-size)", (int)edns.udp_size);
		log_addr(VERB_CLIENT,"from",&repinfo->addr, repinfo->addrlen);
		edns.udp_size = worker->daemon->cfg->max_udp_size;
	}
	if(edns.udp_size < LDNS_HEADER_SIZE) {
		verbose(VERB_ALGO, "worker request: edns is too small.");
		log_addr(VERB_CLIENT, "from", &repinfo->addr, repinfo->addrlen);
		LDNS_QR_SET(sldns_buffer_begin(c->buffer));
		LDNS_TC_SET(sldns_buffer_begin(c->buffer));
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), 
			LDNS_RCODE_SERVFAIL);
		sldns_buffer_set_position(c->buffer, LDNS_HEADER_SIZE);
		sldns_buffer_write_at(c->buffer, 4, 
			(uint8_t*)"\0\0\0\0\0\0\0\0", 8);
		sldns_buffer_flip(c->buffer);
		goto send_reply;
	}
	if(worker->stats.extended)
		server_stats_insquery(&worker->stats, c, qinfo.qtype,
			qinfo.qclass, &edns, repinfo);
	if(c->type != comm_udp)
		edns.udp_size = 65535; /* max size for TCP replies */
	if(qinfo.qclass == LDNS_RR_CLASS_CH && answer_chaos(worker, &qinfo,
		&edns, c->buffer)) {
		server_stats_insrcode(&worker->stats, c->buffer);
		goto send_reply;
	}
	if(local_zones_answer(worker->daemon->local_zones, &qinfo, &edns, 
		c->buffer, worker->scratchpad, repinfo)) {
		regional_free_all(worker->scratchpad);
		if(sldns_buffer_limit(c->buffer) == 0) {
			comm_point_drop_reply(repinfo);
			return 0;
		}
		server_stats_insrcode(&worker->stats, c->buffer);
		goto send_reply;
	}

	/* We've looked in our local zones. If the answer isn't there, we
	 * might need to bail out based on ACLs now. */
	if((ret=deny_refuse_non_local(c, acl, worker, repinfo)) != -1)
	{
		if(ret == 1)
			goto send_reply;
		return ret;
	}

	/* If this request does not have the recursion bit set, verify
	 * ACLs allow the snooping. */
	if(!(LDNS_RD_WIRE(sldns_buffer_begin(c->buffer))) &&
		acl != acl_allow_snoop ) {
		sldns_buffer_set_limit(c->buffer, LDNS_HEADER_SIZE);
		sldns_buffer_write_at(c->buffer, 4, 
			(uint8_t*)"\0\0\0\0\0\0\0\0", 8);
		LDNS_QR_SET(sldns_buffer_begin(c->buffer));
		LDNS_RCODE_SET(sldns_buffer_begin(c->buffer), 
			LDNS_RCODE_REFUSED);
		sldns_buffer_flip(c->buffer);
		server_stats_insrcode(&worker->stats, c->buffer);
		log_addr(VERB_ALGO, "refused nonrec (cache snoop) query from",
			&repinfo->addr, repinfo->addrlen);
		goto send_reply;
	}
	h = query_info_hash(&qinfo, sldns_buffer_read_u16_at(c->buffer, 2));
	if((e=slabhash_lookup(worker->env.msg_cache, h, &qinfo, 0))) {
		/* answer from cache - we have acquired a readlock on it */
		if(answer_from_cache(worker, &qinfo, 
			(struct reply_info*)e->data, 
			*(uint16_t*)(void *)sldns_buffer_begin(c->buffer), 
			sldns_buffer_read_u16_at(c->buffer, 2), repinfo, 
			&edns)) {
			/* prefetch it if the prefetch TTL expired */
			if(worker->env.cfg->prefetch && *worker->env.now >=
				((struct reply_info*)e->data)->prefetch_ttl) {
				time_t leeway = ((struct reply_info*)e->
					data)->ttl - *worker->env.now;
				lock_rw_unlock(&e->lock);
				reply_and_prefetch(worker, &qinfo, 
					sldns_buffer_read_u16_at(c->buffer, 2),
					repinfo, leeway);
				rc = 0;
				goto send_reply_rc;
			}
			lock_rw_unlock(&e->lock);
			goto send_reply;
		}
		verbose(VERB_ALGO, "answer from the cache failed");
		lock_rw_unlock(&e->lock);
	}
	if(!LDNS_RD_WIRE(sldns_buffer_begin(c->buffer))) {
		if(answer_norec_from_cache(worker, &qinfo,
			*(uint16_t*)(void *)sldns_buffer_begin(c->buffer), 
			sldns_buffer_read_u16_at(c->buffer, 2), repinfo, 
			&edns)) {
			goto send_reply;
		}
		verbose(VERB_ALGO, "answer norec from cache -- "
			"need to validate or not primed");
	}
	sldns_buffer_rewind(c->buffer);
	server_stats_querymiss(&worker->stats, worker);

	if(verbosity >= VERB_CLIENT) {
		if(c->type == comm_udp)
			log_addr(VERB_CLIENT, "udp request from",
				&repinfo->addr, repinfo->addrlen);
		else	log_addr(VERB_CLIENT, "tcp request from",
				&repinfo->addr, repinfo->addrlen);
	}

	/* grab a work request structure for this new request */
	mesh_new_client(worker->env.mesh, &qinfo, 
		sldns_buffer_read_u16_at(c->buffer, 2),
		&edns, repinfo, *(uint16_t*)(void *)sldns_buffer_begin(c->buffer));
	worker_mem_report(worker, NULL);
	return 0;

send_reply:
	rc = 1;
send_reply_rc:
#ifdef USE_DNSTAP
	if(worker->dtenv.log_client_response_messages)
		dt_msg_send_client_response(&worker->dtenv, &repinfo->addr,
			c->type, c->buffer);
#endif
	return rc;
}

void 
worker_sighandler(int sig, void* arg)
{
	/* note that log, print, syscalls here give race conditions. 
	 * And cause hangups if the log-lock is held by the application. */
	struct worker* worker = (struct worker*)arg;
	switch(sig) {
#ifdef SIGHUP
		case SIGHUP:
			comm_base_exit(worker->base);
			break;
#endif
		case SIGINT:
			worker->need_to_exit = 1;
			comm_base_exit(worker->base);
			break;
#ifdef SIGQUIT
		case SIGQUIT:
			worker->need_to_exit = 1;
			comm_base_exit(worker->base);
			break;
#endif
		case SIGTERM:
			worker->need_to_exit = 1;
			comm_base_exit(worker->base);
			break;
		default:
			/* unknown signal, ignored */
			break;
	}
}

/** restart statistics timer for worker, if enabled */
static void
worker_restart_timer(struct worker* worker)
{
	if(worker->env.cfg->stat_interval > 0) {
		struct timeval tv;
#ifndef S_SPLINT_S
		tv.tv_sec = worker->env.cfg->stat_interval;
		tv.tv_usec = 0;
#endif
		comm_timer_set(worker->stat_timer, &tv);
	}
}

void worker_stat_timer_cb(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	server_stats_log(&worker->stats, worker, worker->thread_num);
	mesh_stats(worker->env.mesh, "mesh has");
	worker_mem_report(worker, NULL);
	if(!worker->daemon->cfg->stat_cumulative) {
		worker_stats_clear(worker);
	}
	/* start next timer */
	worker_restart_timer(worker);
}

void worker_probe_timer_cb(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	struct timeval tv;
#ifndef S_SPLINT_S
	tv.tv_sec = (time_t)autr_probe_timer(&worker->env);
	tv.tv_usec = 0;
#endif
	if(tv.tv_sec != 0)
		comm_timer_set(worker->env.probe_timer, &tv);
}

struct worker* 
worker_create(struct daemon* daemon, int id, int* ports, int n)
{
	unsigned int seed;
	struct worker* worker = (struct worker*)calloc(1, 
		sizeof(struct worker));
	if(!worker) 
		return NULL;
	worker->numports = n;
	worker->ports = (int*)memdup(ports, sizeof(int)*n);
	if(!worker->ports) {
		free(worker);
		return NULL;
	}
	worker->daemon = daemon;
	worker->thread_num = id;
	if(!(worker->cmd = tube_create())) {
		free(worker->ports);
		free(worker);
		return NULL;
	}
	/* create random state here to avoid locking trouble in RAND_bytes */
	seed = (unsigned int)time(NULL) ^ (unsigned int)getpid() ^
		(((unsigned int)worker->thread_num)<<17);
		/* shift thread_num so it does not match out pid bits */
	if(!(worker->rndstate = ub_initstate(seed, daemon->rand))) {
		seed = 0;
		log_err("could not init random numbers.");
		tube_delete(worker->cmd);
		free(worker->ports);
		free(worker);
		return NULL;
	}
	seed = 0;
#ifdef USE_DNSTAP
	if(daemon->cfg->dnstap) {
		log_assert(daemon->dtenv != NULL);
		memcpy(&worker->dtenv, daemon->dtenv, sizeof(struct dt_env));
		if(!dt_init(&worker->dtenv))
			fatal_exit("dt_init failed");
	}
#endif
	return worker;
}

int
worker_init(struct worker* worker, struct config_file *cfg, 
	struct listen_port* ports, int do_sigs)
{
#ifdef USE_DNSTAP
	struct dt_env* dtenv = &worker->dtenv;
#else
	void* dtenv = NULL;
#endif
	worker->need_to_exit = 0;
	worker->base = comm_base_create(do_sigs);
	if(!worker->base) {
		log_err("could not create event handling base");
		worker_delete(worker);
		return 0;
	}
	comm_base_set_slow_accept_handlers(worker->base, &worker_stop_accept,
		&worker_start_accept, worker);
	if(do_sigs) {
#ifdef SIGHUP
		ub_thread_sig_unblock(SIGHUP);
#endif
		ub_thread_sig_unblock(SIGINT);
#ifdef SIGQUIT
		ub_thread_sig_unblock(SIGQUIT);
#endif
		ub_thread_sig_unblock(SIGTERM);
#ifndef LIBEVENT_SIGNAL_PROBLEM
		worker->comsig = comm_signal_create(worker->base, 
			worker_sighandler, worker);
		if(!worker->comsig 
#ifdef SIGHUP
			|| !comm_signal_bind(worker->comsig, SIGHUP)
#endif
#ifdef SIGQUIT
			|| !comm_signal_bind(worker->comsig, SIGQUIT)
#endif
			|| !comm_signal_bind(worker->comsig, SIGTERM)
			|| !comm_signal_bind(worker->comsig, SIGINT)) {
			log_err("could not create signal handlers");
			worker_delete(worker);
			return 0;
		}
#endif /* LIBEVENT_SIGNAL_PROBLEM */
		if(!daemon_remote_open_accept(worker->daemon->rc, 
			worker->daemon->rc_ports, worker)) {
			worker_delete(worker);
			return 0;
		}
#ifdef UB_ON_WINDOWS
		wsvc_setup_worker(worker);
#endif /* UB_ON_WINDOWS */
	} else { /* !do_sigs */
		worker->comsig = NULL;
	}
	worker->front = listen_create(worker->base, ports,
		cfg->msg_buffer_size, (int)cfg->incoming_num_tcp, 
		worker->daemon->listen_sslctx, dtenv, worker_handle_request,
		worker);
	if(!worker->front) {
		log_err("could not create listening sockets");
		worker_delete(worker);
		return 0;
	}
	worker->back = outside_network_create(worker->base,
		cfg->msg_buffer_size, (size_t)cfg->outgoing_num_ports, 
		cfg->out_ifs, cfg->num_out_ifs, cfg->do_ip4, cfg->do_ip6, 
		cfg->do_tcp?cfg->outgoing_num_tcp:0, 
		worker->daemon->env->infra_cache, worker->rndstate,
		cfg->use_caps_bits_for_id, worker->ports, worker->numports,
		cfg->unwanted_threshold, &worker_alloc_cleanup, worker,
		cfg->do_udp, worker->daemon->connect_sslctx, cfg->delay_close,
		dtenv);
	if(!worker->back) {
		log_err("could not create outgoing sockets");
		worker_delete(worker);
		return 0;
	}
	/* start listening to commands */
	if(!tube_setup_bg_listen(worker->cmd, worker->base,
		&worker_handle_control_cmd, worker)) {
		log_err("could not create control compt.");
		worker_delete(worker);
		return 0;
	}
	worker->stat_timer = comm_timer_create(worker->base, 
		worker_stat_timer_cb, worker);
	if(!worker->stat_timer) {
		log_err("could not create statistics timer");
	}

	/* we use the msg_buffer_size as a good estimate for what the 
	 * user wants for memory usage sizes */
	worker->scratchpad = regional_create_custom(cfg->msg_buffer_size);
	if(!worker->scratchpad) {
		log_err("malloc failure");
		worker_delete(worker);
		return 0;
	}

	server_stats_init(&worker->stats, cfg);
	alloc_init(&worker->alloc, &worker->daemon->superalloc, 
		worker->thread_num);
	alloc_set_id_cleanup(&worker->alloc, &worker_alloc_cleanup, worker);
	worker->env = *worker->daemon->env;
	comm_base_timept(worker->base, &worker->env.now, &worker->env.now_tv);
	if(worker->thread_num == 0)
		log_set_time(worker->env.now);
	worker->env.worker = worker;
	worker->env.send_query = &worker_send_query;
	worker->env.alloc = &worker->alloc;
	worker->env.rnd = worker->rndstate;
	worker->env.scratch = worker->scratchpad;
	worker->env.mesh = mesh_create(&worker->daemon->mods, &worker->env);
	worker->env.detach_subs = &mesh_detach_subs;
	worker->env.attach_sub = &mesh_attach_sub;
	worker->env.kill_sub = &mesh_state_delete;
	worker->env.detect_cycle = &mesh_detect_cycle;
	worker->env.scratch_buffer = sldns_buffer_new(cfg->msg_buffer_size);
	if(!(worker->env.fwds = forwards_create()) ||
		!forwards_apply_cfg(worker->env.fwds, cfg)) {
		log_err("Could not set forward zones");
		worker_delete(worker);
		return 0;
	}
	if(!(worker->env.hints = hints_create()) ||
		!hints_apply_cfg(worker->env.hints, cfg)) {
		log_err("Could not set root or stub hints");
		worker_delete(worker);
		return 0;
	}
	/* one probe timer per process -- if we have 5011 anchors */
	if(autr_get_num_anchors(worker->env.anchors) > 0
#ifndef THREADS_DISABLED
		&& worker->thread_num == 0
#endif
		) {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		worker->env.probe_timer = comm_timer_create(worker->base,
			worker_probe_timer_cb, worker);
		if(!worker->env.probe_timer) {
			log_err("could not create 5011-probe timer");
		} else {
			/* let timer fire, then it can reset itself */
			comm_timer_set(worker->env.probe_timer, &tv);
		}
	}
	if(!worker->env.mesh || !worker->env.scratch_buffer) {
		worker_delete(worker);
		return 0;
	}
	worker_mem_report(worker, NULL);
	/* if statistics enabled start timer */
	if(worker->env.cfg->stat_interval > 0) {
		verbose(VERB_ALGO, "set statistics interval %d secs", 
			worker->env.cfg->stat_interval);
		worker_restart_timer(worker);
	}
	return 1;
}

void 
worker_work(struct worker* worker)
{
	comm_base_dispatch(worker->base);
}

void 
worker_delete(struct worker* worker)
{
	if(!worker) 
		return;
	if(worker->env.mesh && verbosity >= VERB_OPS) {
		server_stats_log(&worker->stats, worker, worker->thread_num);
		mesh_stats(worker->env.mesh, "mesh has");
		worker_mem_report(worker, NULL);
	}
	outside_network_quit_prepare(worker->back);
	mesh_delete(worker->env.mesh);
	sldns_buffer_free(worker->env.scratch_buffer);
	forwards_delete(worker->env.fwds);
	hints_delete(worker->env.hints);
	listen_delete(worker->front);
	outside_network_delete(worker->back);
	comm_signal_delete(worker->comsig);
	tube_delete(worker->cmd);
	comm_timer_delete(worker->stat_timer);
	comm_timer_delete(worker->env.probe_timer);
	free(worker->ports);
	if(worker->thread_num == 0) {
		log_set_time(NULL);
#ifdef UB_ON_WINDOWS
		wsvc_desetup_worker(worker);
#endif /* UB_ON_WINDOWS */
	}
	comm_base_delete(worker->base);
	ub_randfree(worker->rndstate);
	alloc_clear(&worker->alloc);
	regional_destroy(worker->scratchpad);
	free(worker);
}

struct outbound_entry*
worker_send_query(uint8_t* qname, size_t qnamelen, uint16_t qtype,
	uint16_t qclass, uint16_t flags, int dnssec, int want_dnssec,
	int nocaps, struct sockaddr_storage* addr, socklen_t addrlen,
	uint8_t* zone, size_t zonelen, struct module_qstate* q)
{
	struct worker* worker = q->env->worker;
	struct outbound_entry* e = (struct outbound_entry*)regional_alloc(
		q->region, sizeof(*e));
	if(!e) 
		return NULL;
	e->qstate = q;
	e->qsent = outnet_serviced_query(worker->back, qname,
		qnamelen, qtype, qclass, flags, dnssec, want_dnssec, nocaps,
		q->env->cfg->tcp_upstream, q->env->cfg->ssl_upstream, addr,
		addrlen, zone, zonelen, worker_handle_service_reply, e,
		worker->back->udp_buff);
	if(!e->qsent) {
		return NULL;
	}
	return e;
}

void 
worker_alloc_cleanup(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	slabhash_clear(&worker->env.rrset_cache->table);
	slabhash_clear(worker->env.msg_cache);
}

void worker_stats_clear(struct worker* worker)
{
	server_stats_init(&worker->stats, worker->env.cfg);
	mesh_stats_clear(worker->env.mesh);
	worker->back->unwanted_replies = 0;
	worker->back->num_tcp_outgoing = 0;
}

void worker_start_accept(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	listen_start_accept(worker->front);
	if(worker->thread_num == 0)
		daemon_remote_start_accept(worker->daemon->rc);
}

void worker_stop_accept(void* arg)
{
	struct worker* worker = (struct worker*)arg;
	listen_stop_accept(worker->front);
	if(worker->thread_num == 0)
		daemon_remote_stop_accept(worker->daemon->rc);
}

/* --- fake callbacks for fptr_wlist to work --- */
struct outbound_entry* libworker_send_query(uint8_t* ATTR_UNUSED(qname), 
	size_t ATTR_UNUSED(qnamelen), uint16_t ATTR_UNUSED(qtype), 
	uint16_t ATTR_UNUSED(qclass), uint16_t ATTR_UNUSED(flags), 
	int ATTR_UNUSED(dnssec), int ATTR_UNUSED(want_dnssec),
	int ATTR_UNUSED(nocaps), struct sockaddr_storage* ATTR_UNUSED(addr), 
	socklen_t ATTR_UNUSED(addrlen), uint8_t* ATTR_UNUSED(zone),
	size_t ATTR_UNUSED(zonelen), struct module_qstate* ATTR_UNUSED(q))
{
	log_assert(0);
	return 0;
}

int libworker_handle_reply(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply* ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

int libworker_handle_service_reply(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply* ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

void libworker_handle_control_cmd(struct tube* ATTR_UNUSED(tube),
        uint8_t* ATTR_UNUSED(buffer), size_t ATTR_UNUSED(len),
        int ATTR_UNUSED(error), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void libworker_fg_done_cb(void* ATTR_UNUSED(arg), int ATTR_UNUSED(rcode),
        sldns_buffer* ATTR_UNUSED(buf), enum sec_status ATTR_UNUSED(s),
	char* ATTR_UNUSED(why_bogus))
{
	log_assert(0);
}

void libworker_bg_done_cb(void* ATTR_UNUSED(arg), int ATTR_UNUSED(rcode),
        sldns_buffer* ATTR_UNUSED(buf), enum sec_status ATTR_UNUSED(s),
	char* ATTR_UNUSED(why_bogus))
{
	log_assert(0);
}

void libworker_event_done_cb(void* ATTR_UNUSED(arg), int ATTR_UNUSED(rcode),
        sldns_buffer* ATTR_UNUSED(buf), enum sec_status ATTR_UNUSED(s),
	char* ATTR_UNUSED(why_bogus))
{
	log_assert(0);
}

int context_query_cmp(const void* ATTR_UNUSED(a), const void* ATTR_UNUSED(b))
{
	log_assert(0);
	return 0;
}

int order_lock_cmp(const void* ATTR_UNUSED(e1), const void* ATTR_UNUSED(e2))
{
        log_assert(0);
        return 0;
}

int codeline_cmp(const void* ATTR_UNUSED(a), const void* ATTR_UNUSED(b))
{
        log_assert(0);
        return 0;
}

