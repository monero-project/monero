/*
 * services/mesh.c - deal with mesh of query states and handle events for that.
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
 * This file contains functions to assist in dealing with a mesh of
 * query states. This mesh is supposed to be thread-specific.
 * It consists of query states (per qname, qtype, qclass) and connections
 * between query states and the super and subquery states, and replies to
 * send back to clients.
 */
#include "config.h"
#include "services/mesh.h"
#include "services/outbound_list.h"
#include "services/cache/dns.h"
#include "util/log.h"
#include "util/net_help.h"
#include "util/module.h"
#include "util/regional.h"
#include "util/data/msgencode.h"
#include "util/timehist.h"
#include "util/fptr_wlist.h"
#include "util/alloc.h"
#include "util/config_file.h"
#include "ldns/sbuffer.h"

/** subtract timers and the values do not overflow or become negative */
static void
timeval_subtract(struct timeval* d, const struct timeval* end, const struct timeval* start)
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

/** add timers and the values do not overflow or become negative */
static void
timeval_add(struct timeval* d, const struct timeval* add)
{
#ifndef S_SPLINT_S
	d->tv_sec += add->tv_sec;
	d->tv_usec += add->tv_usec;
	if(d->tv_usec > 1000000 ) {
		d->tv_usec -= 1000000;
		d->tv_sec++;
	}
#endif
}

/** divide sum of timers to get average */
static void
timeval_divide(struct timeval* avg, const struct timeval* sum, size_t d)
{
#ifndef S_SPLINT_S
	size_t leftover;
	if(d == 0) {
		avg->tv_sec = 0;
		avg->tv_usec = 0;
		return;
	}
	avg->tv_sec = sum->tv_sec / d;
	avg->tv_usec = sum->tv_usec / d;
	/* handle fraction from seconds divide */
	leftover = sum->tv_sec - avg->tv_sec*d;
	avg->tv_usec += (leftover*1000000)/d;
#endif
}

/** histogram compare of time values */
static int
timeval_smaller(const struct timeval* x, const struct timeval* y)
{
#ifndef S_SPLINT_S
	if(x->tv_sec < y->tv_sec)
		return 1;
	else if(x->tv_sec == y->tv_sec) {
		if(x->tv_usec <= y->tv_usec)
			return 1;
		else	return 0;
	}
	else	return 0;
#endif
}

int
mesh_state_compare(const void* ap, const void* bp)
{
	struct mesh_state* a = (struct mesh_state*)ap;
	struct mesh_state* b = (struct mesh_state*)bp;

	if(a->s.is_priming && !b->s.is_priming)
		return -1;
	if(!a->s.is_priming && b->s.is_priming)
		return 1;

	if(a->s.is_valrec && !b->s.is_valrec)
		return -1;
	if(!a->s.is_valrec && b->s.is_valrec)
		return 1;

	if((a->s.query_flags&BIT_RD) && !(b->s.query_flags&BIT_RD))
		return -1;
	if(!(a->s.query_flags&BIT_RD) && (b->s.query_flags&BIT_RD))
		return 1;

	if((a->s.query_flags&BIT_CD) && !(b->s.query_flags&BIT_CD))
		return -1;
	if(!(a->s.query_flags&BIT_CD) && (b->s.query_flags&BIT_CD))
		return 1;

	return query_info_compare(&a->s.qinfo, &b->s.qinfo);
}

int
mesh_state_ref_compare(const void* ap, const void* bp)
{
	struct mesh_state_ref* a = (struct mesh_state_ref*)ap;
	struct mesh_state_ref* b = (struct mesh_state_ref*)bp;
	return mesh_state_compare(a->s, b->s);
}

struct mesh_area* 
mesh_create(struct module_stack* stack, struct module_env* env)
{
	struct mesh_area* mesh = calloc(1, sizeof(struct mesh_area));
	if(!mesh) {
		log_err("mesh area alloc: out of memory");
		return NULL;
	}
	mesh->histogram = timehist_setup();
	mesh->qbuf_bak = sldns_buffer_new(env->cfg->msg_buffer_size);
	if(!mesh->histogram || !mesh->qbuf_bak) {
		free(mesh);
		log_err("mesh area alloc: out of memory");
		return NULL;
	}
	mesh->mods = *stack;
	mesh->env = env;
	rbtree_init(&mesh->run, &mesh_state_compare);
	rbtree_init(&mesh->all, &mesh_state_compare);
	mesh->num_reply_addrs = 0;
	mesh->num_reply_states = 0;
	mesh->num_detached_states = 0;
	mesh->num_forever_states = 0;
	mesh->stats_jostled = 0;
	mesh->stats_dropped = 0;
	mesh->max_reply_states = env->cfg->num_queries_per_thread;
	mesh->max_forever_states = (mesh->max_reply_states+1)/2;
#ifndef S_SPLINT_S
	mesh->jostle_max.tv_sec = (time_t)(env->cfg->jostle_time / 1000);
	mesh->jostle_max.tv_usec = (time_t)((env->cfg->jostle_time % 1000)
		*1000);
#endif
	return mesh;
}

/** help mesh delete delete mesh states */
static void
mesh_delete_helper(rbnode_t* n)
{
	struct mesh_state* mstate = (struct mesh_state*)n->key;
	/* perform a full delete, not only 'cleanup' routine,
	 * because other callbacks expect a clean state in the mesh.
	 * For 're-entrant' calls */
	mesh_state_delete(&mstate->s);
	/* but because these delete the items from the tree, postorder
	 * traversal and rbtree rebalancing do not work together */
}

void 
mesh_delete(struct mesh_area* mesh)
{
	if(!mesh)
		return;
	/* free all query states */
	while(mesh->all.count)
		mesh_delete_helper(mesh->all.root);
	timehist_delete(mesh->histogram);
	sldns_buffer_free(mesh->qbuf_bak);
	free(mesh);
}

void
mesh_delete_all(struct mesh_area* mesh)
{
	/* free all query states */
	while(mesh->all.count)
		mesh_delete_helper(mesh->all.root);
	mesh->stats_dropped += mesh->num_reply_addrs;
	/* clear mesh area references */
	rbtree_init(&mesh->run, &mesh_state_compare);
	rbtree_init(&mesh->all, &mesh_state_compare);
	mesh->num_reply_addrs = 0;
	mesh->num_reply_states = 0;
	mesh->num_detached_states = 0;
	mesh->num_forever_states = 0;
	mesh->forever_first = NULL;
	mesh->forever_last = NULL;
	mesh->jostle_first = NULL;
	mesh->jostle_last = NULL;
}

int mesh_make_new_space(struct mesh_area* mesh, sldns_buffer* qbuf)
{
	struct mesh_state* m = mesh->jostle_first;
	/* free space is available */
	if(mesh->num_reply_states < mesh->max_reply_states)
		return 1;
	/* try to kick out a jostle-list item */
	if(m && m->reply_list && m->list_select == mesh_jostle_list) {
		/* how old is it? */
		struct timeval age;
		timeval_subtract(&age, mesh->env->now_tv, 
			&m->reply_list->start_time);
		if(timeval_smaller(&mesh->jostle_max, &age)) {
			/* its a goner */
			log_nametypeclass(VERB_ALGO, "query jostled out to "
				"make space for a new one",
				m->s.qinfo.qname, m->s.qinfo.qtype,
				m->s.qinfo.qclass);
			/* backup the query */
			if(qbuf) sldns_buffer_copy(mesh->qbuf_bak, qbuf);
			/* notify supers */
			if(m->super_set.count > 0) {
				verbose(VERB_ALGO, "notify supers of failure");
				m->s.return_msg = NULL;
				m->s.return_rcode = LDNS_RCODE_SERVFAIL;
				mesh_walk_supers(mesh, m);
			}
			mesh->stats_jostled ++;
			mesh_state_delete(&m->s);
			/* restore the query - note that the qinfo ptr to
			 * the querybuffer is then correct again. */
			if(qbuf) sldns_buffer_copy(qbuf, mesh->qbuf_bak);
			return 1;
		}
	}
	/* no space for new item */
	return 0;
}

void mesh_new_client(struct mesh_area* mesh, struct query_info* qinfo,
        uint16_t qflags, struct edns_data* edns, struct comm_reply* rep,
        uint16_t qid)
{
	struct mesh_state* s = mesh_area_find(mesh, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
	int was_detached = 0;
	int was_noreply = 0;
	int added = 0;
	/* does this create a new reply state? */
	if(!s || s->list_select == mesh_no_list) {
		if(!mesh_make_new_space(mesh, rep->c->buffer)) {
			verbose(VERB_ALGO, "Too many queries. dropping "
				"incoming query.");
			comm_point_drop_reply(rep);
			mesh->stats_dropped ++;
			return;
		}
		/* for this new reply state, the reply address is free,
		 * so the limit of reply addresses does not stop reply states*/
	} else {
		/* protect our memory usage from storing reply addresses */
		if(mesh->num_reply_addrs > mesh->max_reply_states*16) {
			verbose(VERB_ALGO, "Too many requests queued. "
				"dropping incoming query.");
			mesh->stats_dropped++;
			comm_point_drop_reply(rep);
			return;
		}
	}
	/* see if it already exists, if not, create one */
	if(!s) {
#ifdef UNBOUND_DEBUG
		struct rbnode_t* n;
#endif
		s = mesh_state_create(mesh->env, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
		if(!s) {
			log_err("mesh_state_create: out of memory; SERVFAIL");
			error_encode(rep->c->buffer, LDNS_RCODE_SERVFAIL,
				qinfo, qid, qflags, edns);
			comm_point_send_reply(rep);
			return;
		}
#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_insert(&mesh->all, &s->node);
		log_assert(n != NULL);
		/* set detached (it is now) */
		mesh->num_detached_states++;
		added = 1;
	}
	if(!s->reply_list && !s->cb_list && s->super_set.count == 0)
		was_detached = 1;
	if(!s->reply_list && !s->cb_list)
		was_noreply = 1;
	/* add reply to s */
	if(!mesh_state_add_reply(s, edns, rep, qid, qflags, qinfo->qname)) {
			log_err("mesh_new_client: out of memory; SERVFAIL");
			error_encode(rep->c->buffer, LDNS_RCODE_SERVFAIL,
				qinfo, qid, qflags, edns);
			comm_point_send_reply(rep);
			if(added)
				mesh_state_delete(&s->s);
			return;
	}
	/* update statistics */
	if(was_detached) {
		log_assert(mesh->num_detached_states > 0);
		mesh->num_detached_states--;
	}
	if(was_noreply) {
		mesh->num_reply_states ++;
	}
	mesh->num_reply_addrs++;
	if(s->list_select == mesh_no_list) {
		/* move to either the forever or the jostle_list */
		if(mesh->num_forever_states < mesh->max_forever_states) {
			mesh->num_forever_states ++;
			mesh_list_insert(s, &mesh->forever_first, 
				&mesh->forever_last);
			s->list_select = mesh_forever_list;
		} else {
			mesh_list_insert(s, &mesh->jostle_first, 
				&mesh->jostle_last);
			s->list_select = mesh_jostle_list;
		}
	}
	if(added)
		mesh_run(mesh, s, module_event_new, NULL);
}

int 
mesh_new_callback(struct mesh_area* mesh, struct query_info* qinfo,
	uint16_t qflags, struct edns_data* edns, sldns_buffer* buf, 
	uint16_t qid, mesh_cb_func_t cb, void* cb_arg)
{
	struct mesh_state* s = mesh_area_find(mesh, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
	int was_detached = 0;
	int was_noreply = 0;
	int added = 0;
	/* there are no limits on the number of callbacks */

	/* see if it already exists, if not, create one */
	if(!s) {
#ifdef UNBOUND_DEBUG
		struct rbnode_t* n;
#endif
		s = mesh_state_create(mesh->env, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
		if(!s) {
			return 0;
		}
#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_insert(&mesh->all, &s->node);
		log_assert(n != NULL);
		/* set detached (it is now) */
		mesh->num_detached_states++;
		added = 1;
	}
	if(!s->reply_list && !s->cb_list && s->super_set.count == 0)
		was_detached = 1;
	if(!s->reply_list && !s->cb_list)
		was_noreply = 1;
	/* add reply to s */
	if(!mesh_state_add_cb(s, edns, buf, cb, cb_arg, qid, qflags)) {
			if(added)
				mesh_state_delete(&s->s);
			return 0;
	}
	/* update statistics */
	if(was_detached) {
		log_assert(mesh->num_detached_states > 0);
		mesh->num_detached_states--;
	}
	if(was_noreply) {
		mesh->num_reply_states ++;
	}
	mesh->num_reply_addrs++;
	if(added)
		mesh_run(mesh, s, module_event_new, NULL);
	return 1;
}

void mesh_new_prefetch(struct mesh_area* mesh, struct query_info* qinfo,
        uint16_t qflags, time_t leeway)
{
	struct mesh_state* s = mesh_area_find(mesh, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
#ifdef UNBOUND_DEBUG
	struct rbnode_t* n;
#endif
	/* already exists, and for a different purpose perhaps.
	 * if mesh_no_list, keep it that way. */
	if(s) {
		/* make it ignore the cache from now on */
		if(!s->s.blacklist)
			sock_list_insert(&s->s.blacklist, NULL, 0, s->s.region);
		if(s->s.prefetch_leeway < leeway)
			s->s.prefetch_leeway = leeway;
		return;
	}
	if(!mesh_make_new_space(mesh, NULL)) {
		verbose(VERB_ALGO, "Too many queries. dropped prefetch.");
		mesh->stats_dropped ++;
		return;
	}
	s = mesh_state_create(mesh->env, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
	if(!s) {
		log_err("prefetch mesh_state_create: out of memory");
		return;
	}
#ifdef UNBOUND_DEBUG
	n =
#else
	(void)
#endif
	rbtree_insert(&mesh->all, &s->node);
	log_assert(n != NULL);
	/* set detached (it is now) */
	mesh->num_detached_states++;
	/* make it ignore the cache */
	sock_list_insert(&s->s.blacklist, NULL, 0, s->s.region);
	s->s.prefetch_leeway = leeway;

	if(s->list_select == mesh_no_list) {
		/* move to either the forever or the jostle_list */
		if(mesh->num_forever_states < mesh->max_forever_states) {
			mesh->num_forever_states ++;
			mesh_list_insert(s, &mesh->forever_first, 
				&mesh->forever_last);
			s->list_select = mesh_forever_list;
		} else {
			mesh_list_insert(s, &mesh->jostle_first, 
				&mesh->jostle_last);
			s->list_select = mesh_jostle_list;
		}
	}
	mesh_run(mesh, s, module_event_new, NULL);
}

void mesh_report_reply(struct mesh_area* mesh, struct outbound_entry* e,
        struct comm_reply* reply, int what)
{
	enum module_ev event = module_event_reply;
	e->qstate->reply = reply;
	if(what != NETEVENT_NOERROR) {
		event = module_event_noreply;
		if(what == NETEVENT_CAPSFAIL)
			event = module_event_capsfail;
	}
	mesh_run(mesh, e->qstate->mesh_info, event, e);
}

struct mesh_state* 
mesh_state_create(struct module_env* env, struct query_info* qinfo, 
	uint16_t qflags, int prime, int valrec)
{
	struct regional* region = alloc_reg_obtain(env->alloc);
	struct mesh_state* mstate;
	int i;
	if(!region)
		return NULL;
	mstate = (struct mesh_state*)regional_alloc(region, 
		sizeof(struct mesh_state));
	if(!mstate) {
		alloc_reg_release(env->alloc, region);
		return NULL;
	}
	memset(mstate, 0, sizeof(*mstate));
	mstate->node = *RBTREE_NULL;
	mstate->run_node = *RBTREE_NULL;
	mstate->node.key = mstate;
	mstate->run_node.key = mstate;
	mstate->reply_list = NULL;
	mstate->list_select = mesh_no_list;
	mstate->replies_sent = 0;
	rbtree_init(&mstate->super_set, &mesh_state_ref_compare);
	rbtree_init(&mstate->sub_set, &mesh_state_ref_compare);
	mstate->num_activated = 0;
	/* init module qstate */
	mstate->s.qinfo.qtype = qinfo->qtype;
	mstate->s.qinfo.qclass = qinfo->qclass;
	mstate->s.qinfo.qname_len = qinfo->qname_len;
	mstate->s.qinfo.qname = regional_alloc_init(region, qinfo->qname,
		qinfo->qname_len);
	if(!mstate->s.qinfo.qname) {
		alloc_reg_release(env->alloc, region);
		return NULL;
	}
	/* remove all weird bits from qflags */
	mstate->s.query_flags = (qflags & (BIT_RD|BIT_CD));
	mstate->s.is_priming = prime;
	mstate->s.is_valrec = valrec;
	mstate->s.reply = NULL;
	mstate->s.region = region;
	mstate->s.curmod = 0;
	mstate->s.return_msg = 0;
	mstate->s.return_rcode = LDNS_RCODE_NOERROR;
	mstate->s.env = env;
	mstate->s.mesh_info = mstate;
	mstate->s.prefetch_leeway = 0;
	/* init modules */
	for(i=0; i<env->mesh->mods.num; i++) {
		mstate->s.minfo[i] = NULL;
		mstate->s.ext_state[i] = module_state_initial;
	}
	return mstate;
}

void 
mesh_state_cleanup(struct mesh_state* mstate)
{
	struct mesh_area* mesh;
	int i;
	if(!mstate)
		return;
	mesh = mstate->s.env->mesh;
	/* drop unsent replies */
	if(!mstate->replies_sent) {
		struct mesh_reply* rep;
		struct mesh_cb* cb;
		for(rep=mstate->reply_list; rep; rep=rep->next) {
			comm_point_drop_reply(&rep->query_reply);
			mesh->num_reply_addrs--;
		}
		for(cb=mstate->cb_list; cb; cb=cb->next) {
			fptr_ok(fptr_whitelist_mesh_cb(cb->cb));
			(*cb->cb)(cb->cb_arg, LDNS_RCODE_SERVFAIL, NULL,
				sec_status_unchecked, NULL);
			mesh->num_reply_addrs--;
		}
	}

	/* de-init modules */
	for(i=0; i<mesh->mods.num; i++) {
		fptr_ok(fptr_whitelist_mod_clear(mesh->mods.mod[i]->clear));
		(*mesh->mods.mod[i]->clear)(&mstate->s, i);
		mstate->s.minfo[i] = NULL;
		mstate->s.ext_state[i] = module_finished;
	}
	alloc_reg_release(mstate->s.env->alloc, mstate->s.region);
}

void 
mesh_state_delete(struct module_qstate* qstate)
{
	struct mesh_area* mesh;
	struct mesh_state_ref* super, ref;
	struct mesh_state* mstate;
	if(!qstate)
		return;
	mstate = qstate->mesh_info;
	mesh = mstate->s.env->mesh;
	mesh_detach_subs(&mstate->s);
	if(mstate->list_select == mesh_forever_list) {
		mesh->num_forever_states --;
		mesh_list_remove(mstate, &mesh->forever_first, 
			&mesh->forever_last);
	} else if(mstate->list_select == mesh_jostle_list) {
		mesh_list_remove(mstate, &mesh->jostle_first, 
			&mesh->jostle_last);
	}
	if(!mstate->reply_list && !mstate->cb_list
		&& mstate->super_set.count == 0) {
		log_assert(mesh->num_detached_states > 0);
		mesh->num_detached_states--;
	}
	if(mstate->reply_list || mstate->cb_list) {
		log_assert(mesh->num_reply_states > 0);
		mesh->num_reply_states--;
	}
	ref.node.key = &ref;
	ref.s = mstate;
	RBTREE_FOR(super, struct mesh_state_ref*, &mstate->super_set) {
		(void)rbtree_delete(&super->s->sub_set, &ref);
	}
	(void)rbtree_delete(&mesh->run, mstate);
	(void)rbtree_delete(&mesh->all, mstate);
	mesh_state_cleanup(mstate);
}

/** helper recursive rbtree find routine */
static int
find_in_subsub(struct mesh_state* m, struct mesh_state* tofind, size_t *c)
{
	struct mesh_state_ref* r;
	if((*c)++ > MESH_MAX_SUBSUB)
		return 1;
	RBTREE_FOR(r, struct mesh_state_ref*, &m->sub_set) {
		if(r->s == tofind || find_in_subsub(r->s, tofind, c))
			return 1;
	}
	return 0;
}

/** find cycle for already looked up mesh_state */
static int 
mesh_detect_cycle_found(struct module_qstate* qstate, struct mesh_state* dep_m)
{
	struct mesh_state* cyc_m = qstate->mesh_info;
	size_t counter = 0;
	if(!dep_m)
		return 0;
	if(dep_m == cyc_m || find_in_subsub(dep_m, cyc_m, &counter)) {
		if(counter > MESH_MAX_SUBSUB)
			return 2;
		return 1;
	}
	return 0;
}

void mesh_detach_subs(struct module_qstate* qstate)
{
	struct mesh_area* mesh = qstate->env->mesh;
	struct mesh_state_ref* ref, lookup;
#ifdef UNBOUND_DEBUG
	struct rbnode_t* n;
#endif
	lookup.node.key = &lookup;
	lookup.s = qstate->mesh_info;
	RBTREE_FOR(ref, struct mesh_state_ref*, &qstate->mesh_info->sub_set) {
#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_delete(&ref->s->super_set, &lookup);
		log_assert(n != NULL); /* must have been present */
		if(!ref->s->reply_list && !ref->s->cb_list
			&& ref->s->super_set.count == 0) {
			mesh->num_detached_states++;
			log_assert(mesh->num_detached_states + 
				mesh->num_reply_states <= mesh->all.count);
		}
	}
	rbtree_init(&qstate->mesh_info->sub_set, &mesh_state_ref_compare);
}

int mesh_attach_sub(struct module_qstate* qstate, struct query_info* qinfo,
        uint16_t qflags, int prime, int valrec, struct module_qstate** newq)
{
	/* find it, if not, create it */
	struct mesh_area* mesh = qstate->env->mesh;
	struct mesh_state* sub = mesh_area_find(mesh, qinfo, qflags, prime,
		valrec);
	int was_detached;
	if(mesh_detect_cycle_found(qstate, sub)) {
		verbose(VERB_ALGO, "attach failed, cycle detected");
		return 0;
	}
	if(!sub) {
#ifdef UNBOUND_DEBUG
		struct rbnode_t* n;
#endif
		/* create a new one */
		sub = mesh_state_create(qstate->env, qinfo, qflags, prime,
			valrec);
		if(!sub) {
			log_err("mesh_attach_sub: out of memory");
			return 0;
		}
#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_insert(&mesh->all, &sub->node);
		log_assert(n != NULL);
		/* set detached (it is now) */
		mesh->num_detached_states++;
		/* set new query state to run */
#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_insert(&mesh->run, &sub->run_node);
		log_assert(n != NULL);
		*newq = &sub->s;
	} else
		*newq = NULL;
	was_detached = (sub->super_set.count == 0);
	if(!mesh_state_attachment(qstate->mesh_info, sub))
		return 0;
	/* if it was a duplicate  attachment, the count was not zero before */
	if(!sub->reply_list && !sub->cb_list && was_detached && 
		sub->super_set.count == 1) {
		/* it used to be detached, before this one got added */
		log_assert(mesh->num_detached_states > 0);
		mesh->num_detached_states--;
	}
	/* *newq will be run when inited after the current module stops */
	return 1;
}

int mesh_state_attachment(struct mesh_state* super, struct mesh_state* sub)
{
#ifdef UNBOUND_DEBUG
	struct rbnode_t* n;
#endif
	struct mesh_state_ref* subref; /* points to sub, inserted in super */
	struct mesh_state_ref* superref; /* points to super, inserted in sub */
	if( !(subref = regional_alloc(super->s.region,
		sizeof(struct mesh_state_ref))) ||
		!(superref = regional_alloc(sub->s.region,
		sizeof(struct mesh_state_ref))) ) {
		log_err("mesh_state_attachment: out of memory");
		return 0;
	}
	superref->node.key = superref;
	superref->s = super;
	subref->node.key = subref;
	subref->s = sub;
	if(!rbtree_insert(&sub->super_set, &superref->node)) {
		/* this should not happen, iterator and validator do not
		 * attach subqueries that are identical. */
		/* already attached, we are done, nothing todo.
		 * since superref and subref already allocated in region,
		 * we cannot free them */
		return 1;
	}
#ifdef UNBOUND_DEBUG
	n =
#else
	(void)
#endif
	rbtree_insert(&super->sub_set, &subref->node);
	log_assert(n != NULL); /* we checked above if statement, the reverse
	  administration should not fail now, unless they are out of sync */
	return 1;
}

/**
 * callback results to mesh cb entry
 * @param m: mesh state to send it for.
 * @param rcode: if not 0, error code.
 * @param rep: reply to send (or NULL if rcode is set).
 * @param r: callback entry
 */
static void
mesh_do_callback(struct mesh_state* m, int rcode, struct reply_info* rep,
	struct mesh_cb* r)
{
	int secure;
	char* reason = NULL;
	/* bogus messages are not made into servfail, sec_status passed 
	 * to the callback function */
	if(rep && rep->security == sec_status_secure)
		secure = 1;
	else	secure = 0;
	if(!rep && rcode == LDNS_RCODE_NOERROR)
		rcode = LDNS_RCODE_SERVFAIL;
	if(!rcode && rep->security == sec_status_bogus) {
		if(!(reason = errinf_to_str(&m->s)))
			rcode = LDNS_RCODE_SERVFAIL;
	}
	/* send the reply */
	if(rcode) {
		fptr_ok(fptr_whitelist_mesh_cb(r->cb));
		(*r->cb)(r->cb_arg, rcode, r->buf, sec_status_unchecked, NULL);
	} else {
		size_t udp_size = r->edns.udp_size;
		sldns_buffer_clear(r->buf);
		r->edns.edns_version = EDNS_ADVERTISED_VERSION;
		r->edns.udp_size = EDNS_ADVERTISED_SIZE;
		r->edns.ext_rcode = 0;
		r->edns.bits &= EDNS_DO;
		if(!reply_info_answer_encode(&m->s.qinfo, rep, r->qid, 
			r->qflags, r->buf, 0, 1, 
			m->s.env->scratch, udp_size, &r->edns, 
			(int)(r->edns.bits & EDNS_DO), secure)) 
		{
			fptr_ok(fptr_whitelist_mesh_cb(r->cb));
			(*r->cb)(r->cb_arg, LDNS_RCODE_SERVFAIL, r->buf,
				sec_status_unchecked, NULL);
		} else {
			fptr_ok(fptr_whitelist_mesh_cb(r->cb));
			(*r->cb)(r->cb_arg, LDNS_RCODE_NOERROR, r->buf,
				rep->security, reason);
		}
	}
	free(reason);
	m->s.env->mesh->num_reply_addrs--;
}

/**
 * Send reply to mesh reply entry
 * @param m: mesh state to send it for.
 * @param rcode: if not 0, error code.
 * @param rep: reply to send (or NULL if rcode is set).
 * @param r: reply entry
 * @param prev: previous reply, already has its answer encoded in buffer.
 */
static void
mesh_send_reply(struct mesh_state* m, int rcode, struct reply_info* rep,
	struct mesh_reply* r, struct mesh_reply* prev)
{
	struct timeval end_time;
	struct timeval duration;
	int secure;
	/* examine security status */
	if(m->s.env->need_to_validate && (!(r->qflags&BIT_CD) ||
		m->s.env->cfg->ignore_cd) && rep && 
		rep->security <= sec_status_bogus) {
		rcode = LDNS_RCODE_SERVFAIL;
		if(m->s.env->cfg->stat_extended) 
			m->s.env->mesh->ans_bogus++;
	}
	if(rep && rep->security == sec_status_secure)
		secure = 1;
	else	secure = 0;
	if(!rep && rcode == LDNS_RCODE_NOERROR)
		rcode = LDNS_RCODE_SERVFAIL;
	/* send the reply */
	if(prev && prev->qflags == r->qflags && 
		prev->edns.edns_present == r->edns.edns_present && 
		prev->edns.bits == r->edns.bits && 
		prev->edns.udp_size == r->edns.udp_size) {
		/* if the previous reply is identical to this one, fix ID */
		if(prev->query_reply.c->buffer != r->query_reply.c->buffer)
			sldns_buffer_copy(r->query_reply.c->buffer, 
				prev->query_reply.c->buffer);
		sldns_buffer_write_at(r->query_reply.c->buffer, 0, 
			&r->qid, sizeof(uint16_t));
		sldns_buffer_write_at(r->query_reply.c->buffer, 12, 
			r->qname, m->s.qinfo.qname_len);
		comm_point_send_reply(&r->query_reply);
	} else if(rcode) {
		m->s.qinfo.qname = r->qname;
		error_encode(r->query_reply.c->buffer, rcode, &m->s.qinfo,
			r->qid, r->qflags, &r->edns);
		comm_point_send_reply(&r->query_reply);
	} else {
		size_t udp_size = r->edns.udp_size;
		r->edns.edns_version = EDNS_ADVERTISED_VERSION;
		r->edns.udp_size = EDNS_ADVERTISED_SIZE;
		r->edns.ext_rcode = 0;
		r->edns.bits &= EDNS_DO;
		m->s.qinfo.qname = r->qname;
		if(!reply_info_answer_encode(&m->s.qinfo, rep, r->qid, 
			r->qflags, r->query_reply.c->buffer, 0, 1, 
			m->s.env->scratch, udp_size, &r->edns, 
			(int)(r->edns.bits & EDNS_DO), secure)) 
		{
			error_encode(r->query_reply.c->buffer, 
				LDNS_RCODE_SERVFAIL, &m->s.qinfo, r->qid, 
				r->qflags, &r->edns);
		}
		comm_point_send_reply(&r->query_reply);
	}
	/* account */
	m->s.env->mesh->num_reply_addrs--;
	end_time = *m->s.env->now_tv;
	timeval_subtract(&duration, &end_time, &r->start_time);
	verbose(VERB_ALGO, "query took " ARG_LL "d.%6.6d sec",
		(long long)duration.tv_sec, (int)duration.tv_usec);
	m->s.env->mesh->replies_sent++;
	timeval_add(&m->s.env->mesh->replies_sum_wait, &duration);
	timehist_insert(m->s.env->mesh->histogram, &duration);
	if(m->s.env->cfg->stat_extended) {
		uint16_t rc = FLAGS_GET_RCODE(sldns_buffer_read_u16_at(r->
			query_reply.c->buffer, 2));
		if(secure) m->s.env->mesh->ans_secure++;
		m->s.env->mesh->ans_rcode[ rc ] ++;
		if(rc == 0 && LDNS_ANCOUNT(sldns_buffer_begin(r->
			query_reply.c->buffer)) == 0)
			m->s.env->mesh->ans_nodata++;
	}
}

void mesh_query_done(struct mesh_state* mstate)
{
	struct mesh_reply* r;
	struct mesh_reply* prev = NULL;
	struct mesh_cb* c;
	struct reply_info* rep = (mstate->s.return_msg?
		mstate->s.return_msg->rep:NULL);
	for(r = mstate->reply_list; r; r = r->next) {
		mesh_send_reply(mstate, mstate->s.return_rcode, rep, r, prev);
		prev = r;
	}
	mstate->replies_sent = 1;
	for(c = mstate->cb_list; c; c = c->next) {
		mesh_do_callback(mstate, mstate->s.return_rcode, rep, c);
	}
}

void mesh_walk_supers(struct mesh_area* mesh, struct mesh_state* mstate)
{
	struct mesh_state_ref* ref;
	RBTREE_FOR(ref, struct mesh_state_ref*, &mstate->super_set)
	{
		/* make super runnable */
		(void)rbtree_insert(&mesh->run, &ref->s->run_node);
		/* callback the function to inform super of result */
		fptr_ok(fptr_whitelist_mod_inform_super(
			mesh->mods.mod[ref->s->s.curmod]->inform_super));
		(*mesh->mods.mod[ref->s->s.curmod]->inform_super)(&mstate->s, 
			ref->s->s.curmod, &ref->s->s);
	}
}

struct mesh_state* mesh_area_find(struct mesh_area* mesh,
	struct query_info* qinfo, uint16_t qflags, int prime, int valrec)
{
	struct mesh_state key;
	struct mesh_state* result;

	key.node.key = &key;
	key.s.is_priming = prime;
	key.s.is_valrec = valrec;
	key.s.qinfo = *qinfo;
	key.s.query_flags = qflags;
	
	result = (struct mesh_state*)rbtree_search(&mesh->all, &key);
	return result;
}

int mesh_state_add_cb(struct mesh_state* s, struct edns_data* edns,
        sldns_buffer* buf, mesh_cb_func_t cb, void* cb_arg,
	uint16_t qid, uint16_t qflags)
{
	struct mesh_cb* r = regional_alloc(s->s.region, 
		sizeof(struct mesh_cb));
	if(!r)
		return 0;
	r->buf = buf;
	log_assert(fptr_whitelist_mesh_cb(cb)); /* early failure ifmissing*/
	r->cb = cb;
	r->cb_arg = cb_arg;
	r->edns = *edns;
	r->qid = qid;
	r->qflags = qflags;
	r->next = s->cb_list;
	s->cb_list = r;
	return 1;

}

int mesh_state_add_reply(struct mesh_state* s, struct edns_data* edns,
        struct comm_reply* rep, uint16_t qid, uint16_t qflags, uint8_t* qname)
{
	struct mesh_reply* r = regional_alloc(s->s.region, 
		sizeof(struct mesh_reply));
	if(!r)
		return 0;
	r->query_reply = *rep;
	r->edns = *edns;
	r->qid = qid;
	r->qflags = qflags;
	r->start_time = *s->s.env->now_tv;
	r->next = s->reply_list;
	r->qname = regional_alloc_init(s->s.region, qname, 
		s->s.qinfo.qname_len);
	if(!r->qname)
		return 0;
	s->reply_list = r;
	return 1;

}

/**
 * Continue processing the mesh state at another module.
 * Handles module to modules tranfer of control.
 * Handles module finished.
 * @param mesh: the mesh area.
 * @param mstate: currently active mesh state.
 * 	Deleted if finished, calls _done and _supers to 
 * 	send replies to clients and inform other mesh states.
 * 	This in turn may create additional runnable mesh states.
 * @param s: state at which the current module exited.
 * @param ev: the event sent to the module.
 * 	returned is the event to send to the next module.
 * @return true if continue processing at the new module.
 * 	false if not continued processing is needed.
 */
static int
mesh_continue(struct mesh_area* mesh, struct mesh_state* mstate,
	enum module_ext_state s, enum module_ev* ev)
{
	mstate->num_activated++;
	if(mstate->num_activated > MESH_MAX_ACTIVATION) {
		/* module is looping. Stop it. */
		log_err("internal error: looping module stopped");
		log_query_info(VERB_QUERY, "pass error for qstate",
			&mstate->s.qinfo);
		s = module_error;
	}
	if(s == module_wait_module || s == module_restart_next) {
		/* start next module */
		mstate->s.curmod++;
		if(mesh->mods.num == mstate->s.curmod) {
			log_err("Cannot pass to next module; at last module");
			log_query_info(VERB_QUERY, "pass error for qstate",
				&mstate->s.qinfo);
			mstate->s.curmod--;
			return mesh_continue(mesh, mstate, module_error, ev);
		}
		if(s == module_restart_next) {
			fptr_ok(fptr_whitelist_mod_clear(
				mesh->mods.mod[mstate->s.curmod]->clear));
			(*mesh->mods.mod[mstate->s.curmod]->clear)
				(&mstate->s, mstate->s.curmod);
			mstate->s.minfo[mstate->s.curmod] = NULL;
		}
		*ev = module_event_pass;
		return 1;
	}
	if(s == module_error && mstate->s.return_rcode == LDNS_RCODE_NOERROR) {
		/* error is bad, handle pass back up below */
		mstate->s.return_rcode = LDNS_RCODE_SERVFAIL;
	}
	if(s == module_error || s == module_finished) {
		if(mstate->s.curmod == 0) {
			mesh_query_done(mstate);
			mesh_walk_supers(mesh, mstate);
			mesh_state_delete(&mstate->s);
			return 0;
		}
		/* pass along the locus of control */
		mstate->s.curmod --;
		*ev = module_event_moddone;
		return 1;
	}
	return 0;
}

void mesh_run(struct mesh_area* mesh, struct mesh_state* mstate,
	enum module_ev ev, struct outbound_entry* e)
{
	enum module_ext_state s;
	verbose(VERB_ALGO, "mesh_run: start");
	while(mstate) {
		/* run the module */
		fptr_ok(fptr_whitelist_mod_operate(
			mesh->mods.mod[mstate->s.curmod]->operate));
		(*mesh->mods.mod[mstate->s.curmod]->operate)
			(&mstate->s, ev, mstate->s.curmod, e);

		/* examine results */
		mstate->s.reply = NULL;
		regional_free_all(mstate->s.env->scratch);
		s = mstate->s.ext_state[mstate->s.curmod];
		verbose(VERB_ALGO, "mesh_run: %s module exit state is %s", 
			mesh->mods.mod[mstate->s.curmod]->name, strextstate(s));
		e = NULL;
		if(mesh_continue(mesh, mstate, s, &ev))
			continue;

		/* run more modules */
		ev = module_event_pass;
		if(mesh->run.count > 0) {
			/* pop random element off the runnable tree */
			mstate = (struct mesh_state*)mesh->run.root->key;
			(void)rbtree_delete(&mesh->run, mstate);
		} else mstate = NULL;
	}
	if(verbosity >= VERB_ALGO) {
		mesh_stats(mesh, "mesh_run: end");
		mesh_log_list(mesh);
	}
}

void 
mesh_log_list(struct mesh_area* mesh)
{
	char buf[30];
	struct mesh_state* m;
	int num = 0;
	RBTREE_FOR(m, struct mesh_state*, &mesh->all) {
		snprintf(buf, sizeof(buf), "%d%s%s%s%s%s%s mod%d %s%s", 
			num++, (m->s.is_priming)?"p":"",  /* prime */
			(m->s.is_valrec)?"v":"",  /* prime */
			(m->s.query_flags&BIT_RD)?"RD":"",
			(m->s.query_flags&BIT_CD)?"CD":"",
			(m->super_set.count==0)?"d":"", /* detached */
			(m->sub_set.count!=0)?"c":"",  /* children */
			m->s.curmod, (m->reply_list)?"rep":"", /*hasreply*/
			(m->cb_list)?"cb":"" /* callbacks */
			); 
		log_query_info(VERB_ALGO, buf, &m->s.qinfo);
	}
}

void 
mesh_stats(struct mesh_area* mesh, const char* str)
{
	verbose(VERB_DETAIL, "%s %u recursion states (%u with reply, "
		"%u detached), %u waiting replies, %u recursion replies "
		"sent, %d replies dropped, %d states jostled out", 
		str, (unsigned)mesh->all.count, 
		(unsigned)mesh->num_reply_states,
		(unsigned)mesh->num_detached_states,
		(unsigned)mesh->num_reply_addrs,
		(unsigned)mesh->replies_sent,
		(unsigned)mesh->stats_dropped,
		(unsigned)mesh->stats_jostled);
	if(mesh->replies_sent > 0) {
		struct timeval avg;
		timeval_divide(&avg, &mesh->replies_sum_wait, 
			mesh->replies_sent);
		log_info("average recursion processing time "
			ARG_LL "d.%6.6d sec",
			(long long)avg.tv_sec, (int)avg.tv_usec);
		log_info("histogram of recursion processing times");
		timehist_log(mesh->histogram, "recursions");
	}
}

void 
mesh_stats_clear(struct mesh_area* mesh)
{
	if(!mesh)
		return;
	mesh->replies_sent = 0;
	mesh->replies_sum_wait.tv_sec = 0;
	mesh->replies_sum_wait.tv_usec = 0;
	mesh->stats_jostled = 0;
	mesh->stats_dropped = 0;
	timehist_clear(mesh->histogram);
	mesh->ans_secure = 0;
	mesh->ans_bogus = 0;
	memset(&mesh->ans_rcode[0], 0, sizeof(size_t)*16);
	mesh->ans_nodata = 0;
}

size_t 
mesh_get_mem(struct mesh_area* mesh)
{
	struct mesh_state* m;
	size_t s = sizeof(*mesh) + sizeof(struct timehist) +
		sizeof(struct th_buck)*mesh->histogram->num +
		sizeof(sldns_buffer) + sldns_buffer_capacity(mesh->qbuf_bak);
	RBTREE_FOR(m, struct mesh_state*, &mesh->all) {
		/* all, including m itself allocated in qstate region */
		s += regional_get_mem(m->s.region);
	}
	return s;
}

int 
mesh_detect_cycle(struct module_qstate* qstate, struct query_info* qinfo,
	uint16_t flags, int prime, int valrec)
{
	struct mesh_area* mesh = qstate->env->mesh;
	struct mesh_state* dep_m = mesh_area_find(mesh, qinfo, flags, prime,
		valrec);
	return mesh_detect_cycle_found(qstate, dep_m);
}

void mesh_list_insert(struct mesh_state* m, struct mesh_state** fp,
        struct mesh_state** lp)
{
	/* insert as last element */
	m->prev = *lp;
	m->next = NULL;
	if(*lp)
		(*lp)->next = m;
	else	*fp = m;
	*lp = m;
}

void mesh_list_remove(struct mesh_state* m, struct mesh_state** fp,
        struct mesh_state** lp)
{
	if(m->next)
		m->next->prev = m->prev;
	else	*lp = m->prev;
	if(m->prev)
		m->prev->next = m->next;
	else	*fp = m->next;
}
