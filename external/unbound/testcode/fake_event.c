/*
 * testcode/fake_event.c - fake event handling that replays existing scenario.
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
 * Event service that replays a scenario.
 * This implements the same exported symbols as the files:
 * util/netevent.c
 * services/listen_dnsport.c
 * services/outside_network.c
 * But these do not actually access the network or events, instead
 * the scenario is played.
 */

#include "config.h"
#include "testcode/fake_event.h"
#include "util/netevent.h"
#include "util/net_help.h"
#include "util/data/msgparse.h"
#include "util/data/msgreply.h"
#include "util/data/msgencode.h"
#include "util/data/dname.h"
#include "util/config_file.h"
#include "services/listen_dnsport.h"
#include "services/outside_network.h"
#include "services/cache/infra.h"
#include "testcode/replay.h"
#include "testcode/testpkts.h"
#include "util/log.h"
#include "util/fptr_wlist.h"
#include "sldns/sbuffer.h"
#include "sldns/wire2str.h"
#include "sldns/str2wire.h"
#include <signal.h>
struct worker;
struct daemon_remote;

/** Global variable: the scenario. Saved here for when event_init is done. */
static struct replay_scenario* saved_scenario = NULL;

/** add timers and the values do not overflow or become negative */
static void
timeval_add(struct timeval* d, const struct timeval* add)
{
#ifndef S_SPLINT_S
	d->tv_sec += add->tv_sec;
	d->tv_usec += add->tv_usec;
	if(d->tv_usec > 1000000) {
		d->tv_usec -= 1000000;
		d->tv_sec++;
	}
#endif
}

void 
fake_temp_file(const char* adj, const char* id, char* buf, size_t len)
{
#ifdef USE_WINSOCK
	snprintf(buf, len, "testbound_%u%s%s.tmp",
		(unsigned)getpid(), adj, id);
#else
	snprintf(buf, len, "/tmp/testbound_%u%s%s.tmp",
		(unsigned)getpid(), adj, id);
#endif
}

void 
fake_event_init(struct replay_scenario* scen)
{
	saved_scenario = scen;
}

void 
fake_event_cleanup(void)
{
	replay_scenario_delete(saved_scenario);
	saved_scenario = NULL;
}

/** helper function that logs a sldns_pkt packet to logfile */
static void
log_pkt(const char* desc, uint8_t* pkt, size_t len)
{
	char* str = sldns_wire2str_pkt(pkt, len);
	if(!str)
		fatal_exit("%s: (failed out of memory wire2str_pkt)", desc);
	else {
		log_info("%s%s", desc, str);
		free(str);
	}
}

/**
 * Returns a string describing the event type.
 */
static const char*
repevt_string(enum replay_event_type t)
{
	switch(t) {
	case repevt_nothing:	 return "NOTHING";
	case repevt_front_query: return "QUERY";
	case repevt_front_reply: return "CHECK_ANSWER";
	case repevt_timeout:	 return "TIMEOUT";
	case repevt_time_passes: return "TIME_PASSES";
	case repevt_back_reply:  return "REPLY";
	case repevt_back_query:  return "CHECK_OUT_QUERY";
	case repevt_autotrust_check: return "CHECK_AUTOTRUST";
	case repevt_error:	 return "ERROR";
	case repevt_assign:	 return "ASSIGN";
	case repevt_traffic:	 return "TRAFFIC";
	case repevt_infra_rtt:	 return "INFRA_RTT";
	default:		 return "UNKNOWN";
	}
}

/** delete a fake pending */
static void 
delete_fake_pending(struct fake_pending* pend)
{
	if(!pend)
		return;
	free(pend->zone);
	sldns_buffer_free(pend->buffer);
	free(pend->pkt);
	free(pend);
}

/** delete a replay answer */
static void
delete_replay_answer(struct replay_answer* a)
{
	if(!a)
		return;
	if(a->repinfo.c) {
		sldns_buffer_free(a->repinfo.c->buffer);
		free(a->repinfo.c);
	}
	free(a->pkt);
	free(a);
}

/**
 * return: true if pending query matches the now event.
 */
static int 
pending_matches_current(struct replay_runtime* runtime, 
	struct entry** entry, struct fake_pending **pend)
{
	struct fake_pending* p;
	struct entry* e;
	if(!runtime->now || runtime->now->evt_type != repevt_back_query
		|| !runtime->pending_list)
		return 0;
	/* see if any of the pending queries matches */
	for(p = runtime->pending_list; p; p = p->next) {
		if(runtime->now->addrlen != 0 &&
			sockaddr_cmp(&p->addr, p->addrlen, &runtime->now->addr,
			runtime->now->addrlen) != 0)
			continue;
		if((e=find_match(runtime->now->match, p->pkt, p->pkt_len,
			p->transport))) {
			*entry = e;
			*pend = p;
			return 1;
		}
	}
	return 0;
}

/**
 * Find the range that matches this pending message.
 * @param runtime: runtime with current moment, and range list.
 * @param entry: returns the pointer to entry that matches.
 * @param pend: the pending that the entry must match.
 * @return: true if a match is found.
 */
static int
pending_find_match(struct replay_runtime* runtime, struct entry** entry, 
	struct fake_pending* pend)
{
	int timenow = runtime->now->time_step;
	struct replay_range* p = runtime->scenario->range_list;
	while(p) {
		if(p->start_step <= timenow && timenow <= p->end_step &&
		  (p->addrlen == 0 || sockaddr_cmp(&p->addr, p->addrlen,
		  	&pend->addr, pend->addrlen) == 0) &&
		  (*entry = find_match(p->match, pend->pkt, pend->pkt_len,
		 	 pend->transport))) {
			log_info("matched query time %d in range [%d, %d] "
				"with entry line %d", timenow, 
				p->start_step, p->end_step, (*entry)->lineno);
			if(p->addrlen != 0)
				log_addr(0, "matched ip", &p->addr, p->addrlen);
			log_pkt("matched pkt: ",
				(*entry)->reply_list->reply_pkt,
				(*entry)->reply_list->reply_len);
			return 1;
		}
		p = p->next_range;
	}
	return 0;
}

/**
 * See if outgoing pending query matches an entry.
 * @param runtime: runtime.
 * @param entry: if true, the entry that matches is returned.
 * @param pend: if true, the outgoing message that matches is returned.
 * @return: true if pending query matches the now event.
 */
static int 
pending_matches_range(struct replay_runtime* runtime, 
	struct entry** entry, struct fake_pending** pend)
{
	struct fake_pending* p = runtime->pending_list;
	/* slow, O(N*N), but it works as advertised with weird matching */
	while(p) {
		log_info("check of pending");
		if(pending_find_match(runtime, entry, p)) {
			*pend = p;
			return 1;
		}
		p = p->next;
	}
	return 0;
}

/**
 * Remove the item from the pending list.
 */
static void
pending_list_delete(struct replay_runtime* runtime, struct fake_pending* pend)
{
	struct fake_pending** prev = &runtime->pending_list;
	struct fake_pending* p = runtime->pending_list;

	while(p) {
		if(p == pend) {
			*prev = p->next;
			delete_fake_pending(pend);
			return;
		}

		prev = &p->next;
		p = p->next;
	}
}

/**
 * Fill buffer with reply from the entry.
 */
static void
fill_buffer_with_reply(sldns_buffer* buffer, struct entry* entry, uint8_t* q,
	size_t qlen)
{
	uint8_t* c;
	size_t clen;
	log_assert(entry && entry->reply_list);
	sldns_buffer_clear(buffer);
	if(entry->reply_list->reply_from_hex) {
		c = sldns_buffer_begin(entry->reply_list->reply_from_hex);
		clen = sldns_buffer_limit(entry->reply_list->reply_from_hex);
		if(!c) fatal_exit("out of memory");
	} else {
		c = entry->reply_list->reply_pkt;
		clen = entry->reply_list->reply_len;
	}
	if(c) {
		if(q) adjust_packet(entry, &c, &clen, q, qlen);
		sldns_buffer_write(buffer, c, clen);
		if(q) free(c);
	}
	sldns_buffer_flip(buffer);
}

/**
 * Perform range entry on pending message.
 * @param runtime: runtime buffer size preference.
 * @param entry: entry that codes for the reply to do.
 * @param pend: pending query that is answered, callback called.
 */
static void
answer_callback_from_entry(struct replay_runtime* runtime,
        struct entry* entry, struct fake_pending* pend)
{
	struct comm_point c;
	struct comm_reply repinfo;
	void* cb_arg = pend->cb_arg;
	comm_point_callback_type* cb = pend->callback;

	memset(&c, 0, sizeof(c));
	c.fd = -1;
	c.buffer = sldns_buffer_new(runtime->bufsize);
	c.type = comm_udp;
	if(pend->transport == transport_tcp)
		c.type = comm_tcp;
	fill_buffer_with_reply(c.buffer, entry, pend->pkt, pend->pkt_len);
	repinfo.c = &c;
	repinfo.addrlen = pend->addrlen;
	memcpy(&repinfo.addr, &pend->addr, pend->addrlen);
	if(!pend->serviced)
		pending_list_delete(runtime, pend);
	if((*cb)(&c, cb_arg, NETEVENT_NOERROR, &repinfo)) {
		fatal_exit("testbound: unexpected: callback returned 1");
	}
	sldns_buffer_free(c.buffer);
}

/** Check the now moment answer check event */
static void
answer_check_it(struct replay_runtime* runtime)
{
	struct replay_answer* ans = runtime->answer_list, 
		*prev = NULL;
	log_assert(runtime && runtime->now && 
		runtime->now->evt_type == repevt_front_reply);
	while(ans) {
		enum transport_type tr = transport_tcp;
		if(ans->repinfo.c->type == comm_udp)
			tr = transport_udp;
		if((runtime->now->addrlen == 0 || sockaddr_cmp(
			&runtime->now->addr, runtime->now->addrlen,
			&ans->repinfo.addr, ans->repinfo.addrlen) == 0) &&
			find_match(runtime->now->match, ans->pkt,
				ans->pkt_len, tr)) {
			log_info("testbound matched event entry from line %d",
				runtime->now->match->lineno);
			log_info("testbound: do STEP %d %s", 
				runtime->now->time_step,
				repevt_string(runtime->now->evt_type));
			if(prev)
				prev->next = ans->next;
			else 	runtime->answer_list = ans->next;
			if(!ans->next)
				runtime->answer_last = prev;
			delete_replay_answer(ans);
			return;
		} else {
			prev = ans;
			ans = ans->next;
		}
	}
	log_info("testbound: do STEP %d %s", runtime->now->time_step,
		repevt_string(runtime->now->evt_type));
	fatal_exit("testbound: not matched");
}

/**
 * Create commpoint (as return address) for a fake incoming query.
 */
static void
fake_front_query(struct replay_runtime* runtime, struct replay_moment *todo)
{
	struct comm_reply repinfo;
	memset(&repinfo, 0, sizeof(repinfo));
	repinfo.c = (struct comm_point*)calloc(1, sizeof(struct comm_point));
	repinfo.addrlen = (socklen_t)sizeof(struct sockaddr_in);
	if(todo->addrlen != 0) {
		repinfo.addrlen = todo->addrlen;
		memcpy(&repinfo.addr, &todo->addr, todo->addrlen);
	}
	repinfo.c->fd = -1;
	repinfo.c->ev = (struct internal_event*)runtime;
	repinfo.c->buffer = sldns_buffer_new(runtime->bufsize);
	if(todo->match->match_transport == transport_tcp)
		repinfo.c->type = comm_tcp;
	else	repinfo.c->type = comm_udp;
	fill_buffer_with_reply(repinfo.c->buffer, todo->match, NULL, 0);
	log_info("testbound: incoming QUERY");
	log_pkt("query pkt", todo->match->reply_list->reply_pkt,
		todo->match->reply_list->reply_len);
	/* call the callback for incoming queries */
	if((*runtime->callback_query)(repinfo.c, runtime->cb_arg, 
		NETEVENT_NOERROR, &repinfo)) {
		/* send immediate reply */
		comm_point_send_reply(&repinfo);
	}
	/* clear it again, in case copy not done properly */
	memset(&repinfo, 0, sizeof(repinfo));
}

/**
 * Perform callback for fake pending message.
 */
static void
fake_pending_callback(struct replay_runtime* runtime, 
	struct replay_moment* todo, int error)
{
	struct fake_pending* p = runtime->pending_list;
	struct comm_reply repinfo;
	struct comm_point c;
	void* cb_arg;
	comm_point_callback_type* cb;

	memset(&c, 0, sizeof(c));
	if(!p) fatal_exit("No pending queries.");
	cb_arg = p->cb_arg;
	cb = p->callback;
	c.buffer = sldns_buffer_new(runtime->bufsize);
	c.type = comm_udp;
	if(p->transport == transport_tcp)
		c.type = comm_tcp;
	if(todo->evt_type == repevt_back_reply && todo->match) {
		fill_buffer_with_reply(c.buffer, todo->match, p->pkt,
			p->pkt_len);
	}
	repinfo.c = &c;
	repinfo.addrlen = p->addrlen;
	memcpy(&repinfo.addr, &p->addr, p->addrlen);
	if(!p->serviced)
		pending_list_delete(runtime, p);
	if((*cb)(&c, cb_arg, error, &repinfo)) {
		fatal_exit("unexpected: pending callback returned 1");
	}
	/* delete the pending item. */
	sldns_buffer_free(c.buffer);
}

/** pass time */
static void
moment_assign(struct replay_runtime* runtime, struct replay_moment* mom)
{
	char* value = macro_process(runtime->vars, runtime, mom->string);
	if(!value)
		fatal_exit("could not process macro step %d", mom->time_step);
	log_info("assign %s = %s", mom->variable, value);
	if(!macro_assign(runtime->vars, mom->variable, value))
		fatal_exit("out of memory storing macro");
	free(value);
	if(verbosity >= VERB_ALGO)
		macro_print_debug(runtime->vars);
}

/** pass time */
static void
time_passes(struct replay_runtime* runtime, struct replay_moment* mom)
{
	struct fake_timer *t;
	struct timeval tv = mom->elapse;
	if(mom->string) {
		char* xp = macro_process(runtime->vars, runtime, mom->string);
		double sec;
		if(!xp) fatal_exit("could not macro expand %s", mom->string);
		verbose(VERB_ALGO, "EVAL %s", mom->string);
		sec = atof(xp);
		free(xp);
#ifndef S_SPLINT_S
		tv.tv_sec = sec;
		tv.tv_usec = (int)((sec - (double)tv.tv_sec) *1000000. + 0.5);
#endif
	}
	timeval_add(&runtime->now_tv, &tv);
	runtime->now_secs = (time_t)runtime->now_tv.tv_sec;
#ifndef S_SPLINT_S
	log_info("elapsed %d.%6.6d  now %d.%6.6d", 
		(int)tv.tv_sec, (int)tv.tv_usec,
		(int)runtime->now_tv.tv_sec, (int)runtime->now_tv.tv_usec);
#endif
	/* see if any timers have fired; and run them */
	while( (t=replay_get_oldest_timer(runtime)) ) {
		t->enabled = 0;
		log_info("fake_timer callback");
		fptr_ok(fptr_whitelist_comm_timer(t->cb));
		(*t->cb)(t->cb_arg);
	}
}

/** check autotrust file contents */
static void
autotrust_check(struct replay_runtime* runtime, struct replay_moment* mom)
{
	char name[1024], line[1024];
	FILE *in;
	int lineno = 0, oke=1;
	char* expanded;
	struct config_strlist* p;
	line[sizeof(line)-1] = 0;
	log_assert(mom->autotrust_id);
	fake_temp_file("_auto_", mom->autotrust_id, name, sizeof(name));
	in = fopen(name, "r");
	if(!in) fatal_exit("could not open %s: %s", name, strerror(errno));
	for(p=mom->file_content; p; p=p->next) {
		lineno++;
		if(!fgets(line, (int)sizeof(line)-1, in)) {
			log_err("autotrust check failed, could not read line");
			log_err("file %s, line %d", name, lineno);
			log_err("should be: %s", p->str);
			fatal_exit("autotrust_check failed");
		}
		if(line[0]) line[strlen(line)-1] = 0; /* remove newline */
		expanded = macro_process(runtime->vars, runtime, p->str);
		if(!expanded) 
			fatal_exit("could not expand macro line %d", lineno);
		if(verbosity >= 7 && strcmp(p->str, expanded) != 0)
			log_info("expanded '%s' to '%s'", p->str, expanded);
		if(strcmp(expanded, line) != 0) {
			log_err("mismatch in file %s, line %d", name, lineno);
			log_err("file has : %s", line);
			log_err("should be: %s", expanded);
			free(expanded);
			oke = 0;
			continue;
		}
		free(expanded);
		fprintf(stderr, "%s:%2d ok : %s\n", name, lineno, line);
	}
	if(fgets(line, (int)sizeof(line)-1, in)) {
		log_err("autotrust check failed, extra lines in %s after %d",
			name, lineno);
		do {
			fprintf(stderr, "file has: %s", line);
		} while(fgets(line, (int)sizeof(line)-1, in));
		oke = 0;
	}
	fclose(in);
	if(!oke)
		fatal_exit("autotrust_check STEP %d failed", mom->time_step);
	log_info("autotrust %s is OK", mom->autotrust_id);
}

/** Store RTT in infra cache */
static void
do_infra_rtt(struct replay_runtime* runtime)
{
	struct replay_moment* now = runtime->now;
	int rto;
	size_t dplen = 0;
	uint8_t* dp = sldns_str2wire_dname(now->variable, &dplen);
	if(!dp) fatal_exit("cannot parse %s", now->variable);
	rto = infra_rtt_update(runtime->infra, &now->addr, now->addrlen,
		dp, dplen, LDNS_RR_TYPE_A, atoi(now->string),
		-1, runtime->now_secs);
	log_addr(0, "INFRA_RTT for", &now->addr, now->addrlen);
	log_info("INFRA_RTT(%s roundtrip %d): rto of %d", now->variable,
		atoi(now->string), rto);
	if(rto == 0) fatal_exit("infra_rtt_update failed");
	free(dp);
}

/** perform exponential backoff on the timeout */
static void
expon_timeout_backoff(struct replay_runtime* runtime)
{
	struct fake_pending* p = runtime->pending_list;
	int rtt, vs;
	uint8_t edns_lame_known;
	int last_rtt, rto;
	if(!p) return; /* no pending packet to backoff */
	if(!infra_host(runtime->infra, &p->addr, p->addrlen, p->zone,
		p->zonelen, runtime->now_secs, &vs, &edns_lame_known, &rtt))
		return;
	last_rtt = rtt;
	rto = infra_rtt_update(runtime->infra, &p->addr, p->addrlen, p->zone,
		p->zonelen, p->qtype, -1, last_rtt, runtime->now_secs);
	log_info("infra_rtt_update returned rto %d", rto);
}

/**
 * Advance to the next moment.
 */
static void
advance_moment(struct replay_runtime* runtime)
{
	if(!runtime->now)
		runtime->now = runtime->scenario->mom_first;
	else 	runtime->now = runtime->now->mom_next;
}

/**
 * Perform actions or checks determined by the moment.
 * Also advances the time by one step.
 * @param runtime: scenario runtime information.
 */
static void
do_moment_and_advance(struct replay_runtime* runtime)
{
	struct replay_moment* mom;
	if(!runtime->now) {
		advance_moment(runtime);
		return;
	}
	log_info("testbound: do STEP %d %s", runtime->now->time_step, 
		repevt_string(runtime->now->evt_type));
	switch(runtime->now->evt_type) {
	case repevt_nothing:
		advance_moment(runtime);
		break;
	case repevt_front_query:
		/* advance moment before doing the step, so that the next
		   moment which may check some result of the mom step
		   can catch those results. */
		mom = runtime->now;
		advance_moment(runtime);
		fake_front_query(runtime, mom);
		break;
	case repevt_front_reply:
		if(runtime->answer_list) 
			log_err("testbound: There are unmatched answers.");
		fatal_exit("testbound: query answer not matched");
		break;
	case repevt_timeout:
		mom = runtime->now;
		advance_moment(runtime);
		expon_timeout_backoff(runtime);
		fake_pending_callback(runtime, mom, NETEVENT_TIMEOUT);
		break;
	case repevt_back_reply:
		mom = runtime->now;
		advance_moment(runtime);
		fake_pending_callback(runtime, mom, NETEVENT_NOERROR);
		break;
	case repevt_back_query:
		/* Back queries are matched when they are sent out. */
		log_err("No query matching the current moment was sent.");
		fatal_exit("testbound: back query not matched");
		break;
	case repevt_error:
		mom = runtime->now;
		advance_moment(runtime);
		fake_pending_callback(runtime, mom, NETEVENT_CLOSED);
		break;
	case repevt_time_passes:
		time_passes(runtime, runtime->now);
		advance_moment(runtime);
		break;
	case repevt_autotrust_check:
		autotrust_check(runtime, runtime->now);
		advance_moment(runtime);
		break;
	case repevt_assign:
		moment_assign(runtime, runtime->now);
		advance_moment(runtime);
		break;
	case repevt_traffic:
		advance_moment(runtime);
		break;
	case repevt_infra_rtt:
		do_infra_rtt(runtime);
		advance_moment(runtime);
		break;
	default:
		fatal_exit("testbound: unknown event type %d", 
			runtime->now->evt_type);
	}
}

/** run the scenario in event callbacks */
static void
run_scenario(struct replay_runtime* runtime)
{
	struct entry* entry = NULL;
	struct fake_pending* pending = NULL;
	int max_rounds = 5000;
	int rounds = 0;
	runtime->now = runtime->scenario->mom_first;
	log_info("testbound: entering fake runloop");
	do {
		/* if moment matches pending query do it. */
		/* else if moment matches given answer, do it */
		/* else if precoded_range matches pending, do it */
		/* else do the current moment */
		if(pending_matches_current(runtime, &entry, &pending)) {
			log_info("testbound: do STEP %d CHECK_OUT_QUERY", 
				runtime->now->time_step);
			advance_moment(runtime);
			if(entry->copy_id)
				answer_callback_from_entry(runtime, entry, 
				pending);
		} else if(runtime->answer_list && runtime->now && 
			runtime->now->evt_type == repevt_front_reply) {
			answer_check_it(runtime);			
			advance_moment(runtime);
		} else if(pending_matches_range(runtime, &entry, &pending)) {
			answer_callback_from_entry(runtime, entry, pending);
		} else {
			do_moment_and_advance(runtime);
		}
		log_info("testbound: end of event stage");
		rounds++;
		if(rounds > max_rounds)
			fatal_exit("testbound: too many rounds, it loops.");
	} while(runtime->now);

	if(runtime->pending_list) {
		struct fake_pending* p;
		log_err("testbound: there are still messages pending.");
		for(p = runtime->pending_list; p; p=p->next) {
			log_pkt("pending msg", p->pkt, p->pkt_len);
			log_addr(0, "pending to", &p->addr, p->addrlen);
		}
		fatal_exit("testbound: there are still messages pending.");
	}
	if(runtime->answer_list) {
		fatal_exit("testbound: there are unmatched answers.");
	}
	log_info("testbound: exiting fake runloop.");
	runtime->exit_cleanly = 1;
}

/*********** Dummy routines ***********/

struct listen_dnsport* 
listen_create(struct comm_base* base, struct listen_port* ATTR_UNUSED(ports),
	size_t bufsize, int ATTR_UNUSED(tcp_accept_count),
	void* ATTR_UNUSED(sslctx), struct dt_env* ATTR_UNUSED(dtenv),
	comm_point_callback_type* cb, void* cb_arg)
{
	struct replay_runtime* runtime = (struct replay_runtime*)base;
	struct listen_dnsport* l= calloc(1, sizeof(struct listen_dnsport));
	if(!l)
		return NULL;
	l->base = base;
	l->udp_buff = sldns_buffer_new(bufsize);
	if(!l->udp_buff) {
		free(l);
		return NULL;
	}
	runtime->callback_query = cb;
	runtime->cb_arg = cb_arg;
	runtime->bufsize = bufsize;
	return l;
}

void 
listen_delete(struct listen_dnsport* listen)
{
	if(!listen)
		return;
	sldns_buffer_free(listen->udp_buff);
	free(listen);
}

struct comm_base* 
comm_base_create(int ATTR_UNUSED(sigs))
{
	/* we return the runtime structure instead. */
	struct replay_runtime* runtime = (struct replay_runtime*)
		calloc(1, sizeof(struct replay_runtime));
	runtime->scenario = saved_scenario;
	runtime->vars = macro_store_create();
	if(!runtime->vars) fatal_exit("out of memory");
	return (struct comm_base*)runtime;
}

void 
comm_base_delete(struct comm_base* b)
{
	struct replay_runtime* runtime = (struct replay_runtime*)b;
	struct fake_pending* p, *np;
	struct replay_answer* a, *na;
	struct fake_timer* t, *nt;
	if(!runtime)
		return;
	runtime->scenario= NULL;
	p = runtime->pending_list;
	while(p) {
		np = p->next;
		delete_fake_pending(p);
		p = np;
	}
	a = runtime->answer_list;
	while(a) {
		na = a->next;
		delete_replay_answer(a);
		a = na;
	}
	t = runtime->timer_list;
	while(t) {
		nt = t->next;
		free(t);
		t = nt;
	}
	macro_store_delete(runtime->vars);
	free(runtime);
}

void
comm_base_timept(struct comm_base* b, time_t** tt, struct timeval** tv)
{
	struct replay_runtime* runtime = (struct replay_runtime*)b;
	*tt = &runtime->now_secs;
	*tv = &runtime->now_tv;
}

void 
comm_base_dispatch(struct comm_base* b)
{
	struct replay_runtime* runtime = (struct replay_runtime*)b;
	run_scenario(runtime);
	if(runtime->sig_cb)
		(*runtime->sig_cb)(SIGTERM, runtime->sig_cb_arg);
	else	exit(0); /* OK exit when LIBEVENT_SIGNAL_PROBLEM exists */
}

void 
comm_base_exit(struct comm_base* b)
{
	struct replay_runtime* runtime = (struct replay_runtime*)b;
	if(!runtime->exit_cleanly) {
		/* some sort of failure */
		fatal_exit("testbound: comm_base_exit was called.");
	}
}

struct comm_signal* 
comm_signal_create(struct comm_base* base,
        void (*callback)(int, void*), void* cb_arg)
{
	struct replay_runtime* runtime = (struct replay_runtime*)base;
	runtime->sig_cb = callback;
	runtime->sig_cb_arg = cb_arg;
	return calloc(1, sizeof(struct comm_signal));
}

int 
comm_signal_bind(struct comm_signal* ATTR_UNUSED(comsig), int 
	ATTR_UNUSED(sig))
{
	return 1;
}

void 
comm_signal_delete(struct comm_signal* comsig)
{
	free(comsig);
}

void 
comm_point_send_reply(struct comm_reply* repinfo)
{
	struct replay_answer* ans = (struct replay_answer*)calloc(1,
		sizeof(struct replay_answer));
	struct replay_runtime* runtime = (struct replay_runtime*)repinfo->c->ev;
	log_info("testbound: comm_point_send_reply fake");
	/* dump it into the todo list */
	log_assert(ans);
	memcpy(&ans->repinfo, repinfo, sizeof(struct comm_reply));
	ans->next = NULL;
	if(runtime->answer_last)
		runtime->answer_last->next = ans;
	else 	runtime->answer_list = ans;
	runtime->answer_last = ans;

	/* try to parse packet */
	ans->pkt = memdup(sldns_buffer_begin(ans->repinfo.c->buffer),
		sldns_buffer_limit(ans->repinfo.c->buffer));
	ans->pkt_len = sldns_buffer_limit(ans->repinfo.c->buffer);
	if(!ans->pkt) fatal_exit("out of memory");
	log_pkt("reply pkt: ", ans->pkt, ans->pkt_len);
}

void 
comm_point_drop_reply(struct comm_reply* repinfo)
{
	log_info("comm_point_drop_reply fake");
	if(repinfo->c) {
		sldns_buffer_free(repinfo->c->buffer);
		free(repinfo->c);
	}
}

struct outside_network* 
outside_network_create(struct comm_base* base, size_t bufsize, 
	size_t ATTR_UNUSED(num_ports), char** ATTR_UNUSED(ifs), 
	int ATTR_UNUSED(num_ifs), int ATTR_UNUSED(do_ip4), 
	int ATTR_UNUSED(do_ip6), size_t ATTR_UNUSED(num_tcp), 
	struct infra_cache* infra,
	struct ub_randstate* ATTR_UNUSED(rnd), 
	int ATTR_UNUSED(use_caps_for_id), int* ATTR_UNUSED(availports),
	int ATTR_UNUSED(numavailports), size_t ATTR_UNUSED(unwanted_threshold),
	int ATTR_UNUSED(outgoing_tcp_mss),
	void (*unwanted_action)(void*), void* ATTR_UNUSED(unwanted_param),
	int ATTR_UNUSED(do_udp), void* ATTR_UNUSED(sslctx),
	int ATTR_UNUSED(delayclose), struct dt_env* ATTR_UNUSED(dtenv))
{
	struct replay_runtime* runtime = (struct replay_runtime*)base;
	struct outside_network* outnet =  calloc(1, 
		sizeof(struct outside_network));
	(void)unwanted_action;
	if(!outnet)
		return NULL;
	runtime->infra = infra;
	outnet->base = base;
	outnet->udp_buff = sldns_buffer_new(bufsize);
	if(!outnet->udp_buff) {
		free(outnet);
		return NULL;
	}
	return outnet;
}

void 
outside_network_delete(struct outside_network* outnet)
{
	if(!outnet)
		return;
	sldns_buffer_free(outnet->udp_buff);
	free(outnet);
}

void 
outside_network_quit_prepare(struct outside_network* ATTR_UNUSED(outnet))
{
}

struct pending* 
pending_udp_query(struct serviced_query* sq, sldns_buffer* packet,
	int timeout, comm_point_callback_type* callback, void* callback_arg)
{
	struct replay_runtime* runtime = (struct replay_runtime*)
		sq->outnet->base;
	struct fake_pending* pend = (struct fake_pending*)calloc(1,
		sizeof(struct fake_pending));
	log_assert(pend);
	pend->buffer = sldns_buffer_new(sldns_buffer_capacity(packet));
	log_assert(pend->buffer);
	sldns_buffer_write(pend->buffer, sldns_buffer_begin(packet),
		sldns_buffer_limit(packet));
	sldns_buffer_flip(pend->buffer);
	memcpy(&pend->addr, &sq->addr, sq->addrlen);
	pend->addrlen = sq->addrlen;
	pend->callback = callback;
	pend->cb_arg = callback_arg;
	pend->timeout = timeout/1000;
	pend->transport = transport_udp;
	pend->pkt = NULL;
	pend->zone = NULL;
	pend->serviced = 0;
	pend->runtime = runtime;
	pend->pkt_len = sldns_buffer_limit(packet);
	pend->pkt = memdup(sldns_buffer_begin(packet), pend->pkt_len);
	if(!pend->pkt) fatal_exit("out of memory");
	log_pkt("pending udp pkt: ", pend->pkt, pend->pkt_len);

	/* see if it matches the current moment */
	if(runtime->now && runtime->now->evt_type == repevt_back_query &&
		(runtime->now->addrlen == 0 || sockaddr_cmp(
			&runtime->now->addr, runtime->now->addrlen,
			&pend->addr, pend->addrlen) == 0) &&
		find_match(runtime->now->match, pend->pkt, pend->pkt_len,
			pend->transport)) {
		log_info("testbound: matched pending to event. "
			"advance time between events.");
		log_info("testbound: do STEP %d %s", runtime->now->time_step,
			repevt_string(runtime->now->evt_type));
		advance_moment(runtime);
		/* still create the pending, because we need it to callback */
	} 
	log_info("testbound: created fake pending");
	/* add to list */
	pend->next = runtime->pending_list;
	runtime->pending_list = pend;
	return (struct pending*)pend;
}

struct waiting_tcp*
pending_tcp_query(struct serviced_query* sq, sldns_buffer* packet,
	int timeout, comm_point_callback_type* callback, void* callback_arg)
{
	struct replay_runtime* runtime = (struct replay_runtime*)
		sq->outnet->base;
	struct fake_pending* pend = (struct fake_pending*)calloc(1,
		sizeof(struct fake_pending));
	log_assert(pend);
	pend->buffer = sldns_buffer_new(sldns_buffer_capacity(packet));
	log_assert(pend->buffer);
	sldns_buffer_write(pend->buffer, sldns_buffer_begin(packet),
		sldns_buffer_limit(packet));
	sldns_buffer_flip(pend->buffer);
	memcpy(&pend->addr, &sq->addr, sq->addrlen);
	pend->addrlen = sq->addrlen;
	pend->callback = callback;
	pend->cb_arg = callback_arg;
	pend->timeout = timeout;
	pend->transport = transport_tcp;
	pend->pkt = NULL;
	pend->zone = NULL;
	pend->runtime = runtime;
	pend->serviced = 0;
	pend->pkt_len = sldns_buffer_limit(packet);
	pend->pkt = memdup(sldns_buffer_begin(packet), pend->pkt_len);
	if(!pend->pkt) fatal_exit("out of memory");
	log_pkt("pending tcp pkt: ", pend->pkt, pend->pkt_len);

	/* see if it matches the current moment */
	if(runtime->now && runtime->now->evt_type == repevt_back_query &&
		(runtime->now->addrlen == 0 || sockaddr_cmp(
			&runtime->now->addr, runtime->now->addrlen,
			&pend->addr, pend->addrlen) == 0) &&
		find_match(runtime->now->match, pend->pkt, pend->pkt_len,
			pend->transport)) {
		log_info("testbound: matched pending to event. "
			"advance time between events.");
		log_info("testbound: do STEP %d %s", runtime->now->time_step,
			repevt_string(runtime->now->evt_type));
		advance_moment(runtime);
		/* still create the pending, because we need it to callback */
	} 
	log_info("testbound: created fake pending");
	/* add to list */
	pend->next = runtime->pending_list;
	runtime->pending_list = pend;
	return (struct waiting_tcp*)pend;
}

struct serviced_query* outnet_serviced_query(struct outside_network* outnet,
	struct query_info* qinfo, uint16_t flags, int dnssec,
	int ATTR_UNUSED(want_dnssec), int ATTR_UNUSED(nocaps),
	int ATTR_UNUSED(tcp_upstream), int ATTR_UNUSED(ssl_upstream),
	struct sockaddr_storage* addr, socklen_t addrlen, uint8_t* zone,
	size_t zonelen, struct module_qstate* qstate,
	comm_point_callback_type* callback, void* callback_arg,
	sldns_buffer* ATTR_UNUSED(buff), struct module_env* ATTR_UNUSED(env))
{
	struct replay_runtime* runtime = (struct replay_runtime*)outnet->base;
	struct fake_pending* pend = (struct fake_pending*)calloc(1,
		sizeof(struct fake_pending));
	char z[256];
	log_assert(pend);
	log_nametypeclass(VERB_OPS, "pending serviced query", 
		qinfo->qname, qinfo->qtype, qinfo->qclass);
	dname_str(zone, z);
	verbose(VERB_OPS, "pending serviced query zone %s flags%s%s%s%s", 
		z, (flags&BIT_RD)?" RD":"", (flags&BIT_CD)?" CD":"",
		(flags&~(BIT_RD|BIT_CD))?" MORE":"", (dnssec)?" DO":"");

	/* create packet with EDNS */
	pend->buffer = sldns_buffer_new(512);
	log_assert(pend->buffer);
	sldns_buffer_write_u16(pend->buffer, 0); /* id */
	sldns_buffer_write_u16(pend->buffer, flags);
	sldns_buffer_write_u16(pend->buffer, 1); /* qdcount */
	sldns_buffer_write_u16(pend->buffer, 0); /* ancount */
	sldns_buffer_write_u16(pend->buffer, 0); /* nscount */
	sldns_buffer_write_u16(pend->buffer, 0); /* arcount */
	sldns_buffer_write(pend->buffer, qinfo->qname, qinfo->qname_len);
	sldns_buffer_write_u16(pend->buffer, qinfo->qtype);
	sldns_buffer_write_u16(pend->buffer, qinfo->qclass);
	sldns_buffer_flip(pend->buffer);
	if(1) {
		struct edns_data edns;
		if(!inplace_cb_query_call(env, qinfo, flags, addr, addrlen,
			zone, zonelen, qstate, qstate->region)) {
			free(pend);
			return NULL;
		}
		/* add edns */
		edns.edns_present = 1;
		edns.ext_rcode = 0;
		edns.edns_version = EDNS_ADVERTISED_VERSION;
		edns.udp_size = EDNS_ADVERTISED_SIZE;
		edns.bits = 0;
		edns.opt_list = qstate->edns_opts_back_out;
		if(dnssec)
			edns.bits = EDNS_DO;
		attach_edns_record(pend->buffer, &edns);
	}
	memcpy(&pend->addr, addr, addrlen);
	pend->addrlen = addrlen;
	pend->zone = memdup(zone, zonelen);
	pend->zonelen = zonelen;
	pend->qtype = (int)qinfo->qtype;
	log_assert(pend->zone);
	pend->callback = callback;
	pend->cb_arg = callback_arg;
	pend->timeout = UDP_AUTH_QUERY_TIMEOUT;
	pend->transport = transport_udp; /* pretend UDP */
	pend->pkt = NULL;
	pend->runtime = runtime;
	pend->serviced = 1;
	pend->pkt_len = sldns_buffer_limit(pend->buffer);
	pend->pkt = memdup(sldns_buffer_begin(pend->buffer), pend->pkt_len);
	if(!pend->pkt) fatal_exit("out of memory");
	/*log_pkt("pending serviced query: ", pend->pkt, pend->pkt_len);*/

	/* see if it matches the current moment */
	if(runtime->now && runtime->now->evt_type == repevt_back_query &&
		(runtime->now->addrlen == 0 || sockaddr_cmp(
			&runtime->now->addr, runtime->now->addrlen,
			&pend->addr, pend->addrlen) == 0) &&
		find_match(runtime->now->match, pend->pkt, pend->pkt_len,
			pend->transport)) {
		log_info("testbound: matched pending to event. "
			"advance time between events.");
		log_info("testbound: do STEP %d %s", runtime->now->time_step,
			repevt_string(runtime->now->evt_type));
		advance_moment(runtime);
		/* still create the pending, because we need it to callback */
	} 
	log_info("testbound: created fake pending");
	/* add to list */
	pend->next = runtime->pending_list;
	runtime->pending_list = pend;
	return (struct serviced_query*)pend;
}

void outnet_serviced_query_stop(struct serviced_query* sq, void* cb_arg)
{
	struct fake_pending* pend = (struct fake_pending*)sq;
	struct replay_runtime* runtime = pend->runtime;
	/* delete from the list */
	struct fake_pending* p = runtime->pending_list, *prev=NULL;
	while(p) {
		if(p == pend) {
			log_assert(p->cb_arg == cb_arg);
			(void)cb_arg;
			log_info("serviced pending delete");
			if(prev)
				prev->next = p->next;
			else 	runtime->pending_list = p->next;
			sldns_buffer_free(p->buffer);
			free(p->pkt);
			free(p->zone);
			free(p);
			return;
		}
		prev = p;
		p = p->next;
	}
	log_info("double delete of pending serviced query");
}

struct listen_port* listening_ports_open(struct config_file* ATTR_UNUSED(cfg),
	int* ATTR_UNUSED(reuseport))
{
	return calloc(1, 1);
}

void listening_ports_free(struct listen_port* list)
{
	free(list);
}

struct comm_point* comm_point_create_local(struct comm_base* ATTR_UNUSED(base),
        int ATTR_UNUSED(fd), size_t ATTR_UNUSED(bufsize),
        comm_point_callback_type* ATTR_UNUSED(callback), 
	void* ATTR_UNUSED(callback_arg))
{
	return calloc(1, 1);
}

struct comm_point* comm_point_create_raw(struct comm_base* ATTR_UNUSED(base),
        int ATTR_UNUSED(fd), int ATTR_UNUSED(writing),
        comm_point_callback_type* ATTR_UNUSED(callback), 
	void* ATTR_UNUSED(callback_arg))
{
	/* no pipe comm possible */
	return calloc(1, 1);
}

void comm_point_start_listening(struct comm_point* ATTR_UNUSED(c), 
	int ATTR_UNUSED(newfd), int ATTR_UNUSED(sec))
{
	/* no bg write pipe comm possible */
}

void comm_point_stop_listening(struct comm_point* ATTR_UNUSED(c))
{
	/* no bg write pipe comm possible */
}

/* only cmd com _local gets deleted */
void comm_point_delete(struct comm_point* c)
{
	free(c);
}

size_t listen_get_mem(struct listen_dnsport* ATTR_UNUSED(listen))
{
	return 0;
}

size_t outnet_get_mem(struct outside_network* ATTR_UNUSED(outnet))
{
	return 0;
}

size_t comm_point_get_mem(struct comm_point* ATTR_UNUSED(c))
{
	return 0;
}

size_t serviced_get_mem(struct serviced_query* ATTR_UNUSED(c))
{
	return 0;
}

/* fake for fptr wlist */
int outnet_udp_cb(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply *ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

int outnet_tcp_cb(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply *ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

void pending_udp_timer_cb(void *ATTR_UNUSED(arg))
{
	log_assert(0);
}

void pending_udp_timer_delay_cb(void *ATTR_UNUSED(arg))
{
	log_assert(0);
}

void outnet_tcptimer(void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_udp_callback(int ATTR_UNUSED(fd), short ATTR_UNUSED(event), 
	void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_udp_ancil_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_tcp_accept_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_tcp_handle_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_timer_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_signal_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_local_handle_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_point_raw_handle_callback(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

void comm_base_handle_slow_accept(int ATTR_UNUSED(fd), 
	short ATTR_UNUSED(event), void* ATTR_UNUSED(arg))
{
	log_assert(0);
}

int serviced_udp_callback(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply* ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

int serviced_tcp_callback(struct comm_point* ATTR_UNUSED(c), 
	void* ATTR_UNUSED(arg), int ATTR_UNUSED(error),
        struct comm_reply* ATTR_UNUSED(reply_info))
{
	log_assert(0);
	return 0;
}

int pending_cmp(const void* ATTR_UNUSED(a), const void* ATTR_UNUSED(b))
{
	log_assert(0);
	return 0;
}

int serviced_cmp(const void* ATTR_UNUSED(a), const void* ATTR_UNUSED(b))
{
	log_assert(0);
	return 0;
}

/* timers in testbound for autotrust. statistics tested in tpkg. */
struct comm_timer* comm_timer_create(struct comm_base* base, 
	void (*cb)(void*), void* cb_arg)
{
	struct replay_runtime* runtime = (struct replay_runtime*)base;
	struct fake_timer* t = (struct fake_timer*)calloc(1, sizeof(*t));
	t->cb = cb;
	t->cb_arg = cb_arg;
	fptr_ok(fptr_whitelist_comm_timer(t->cb)); /* check in advance */
	t->runtime = runtime;
	t->next = runtime->timer_list;
	runtime->timer_list = t;
	return (struct comm_timer*)t;
}

void comm_timer_disable(struct comm_timer* timer)
{
	struct fake_timer* t = (struct fake_timer*)timer;
	log_info("fake timer disabled");
	t->enabled = 0;
}

void comm_timer_set(struct comm_timer* timer, struct timeval* tv)
{
	struct fake_timer* t = (struct fake_timer*)timer;
	t->enabled = 1;
	t->tv = *tv;
	log_info("fake timer set %d.%6.6d", 
		(int)t->tv.tv_sec, (int)t->tv.tv_usec);
	timeval_add(&t->tv, &t->runtime->now_tv);
}

void comm_timer_delete(struct comm_timer* timer)
{
	struct fake_timer* t = (struct fake_timer*)timer;
	struct fake_timer** pp, *p;
	if(!t) return;

	/* remove from linked list */
	pp = &t->runtime->timer_list;
	p = t->runtime->timer_list;
	while(p) {
		if(p == t) {
			/* snip from list */
			*pp = p->next;
			break;
		}
		pp = &p->next;
		p = p->next;
	}

	free(timer);
}

void comm_base_set_slow_accept_handlers(struct comm_base* ATTR_UNUSED(b),
	void (*stop_acc)(void*), void (*start_acc)(void*),
	void* ATTR_UNUSED(arg))
{
	/* ignore this */
	(void)stop_acc;
	(void)start_acc;
}

struct ub_event_base* comm_base_internal(struct comm_base* ATTR_UNUSED(b))
{
	/* no pipe comm possible in testbound */
	return NULL;
}

void daemon_remote_exec(struct worker* ATTR_UNUSED(worker))
{
}

void listen_start_accept(struct listen_dnsport* ATTR_UNUSED(listen))
{
}

void listen_stop_accept(struct listen_dnsport* ATTR_UNUSED(listen))
{
}

void daemon_remote_start_accept(struct daemon_remote* ATTR_UNUSED(rc))
{
}

void daemon_remote_stop_accept(struct daemon_remote* ATTR_UNUSED(rc))
{
}

/*********** End of Dummy routines ***********/
