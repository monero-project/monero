/*
 * unbound-event.h - unbound validating resolver public API with events
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
 * This file contains the unbound interface for use with libevent.
 * You have to use the same libevent that unbound was compiled with,
 * otherwise it wouldn't work, the event and event_base structures would
 * be different.  If unbound is compiled without libevent support then
 * this header file is not supposed to be installed on the system.
 *
 * Use ub_ctx_create_event_base() to create an unbound context that uses
 * the event base that you have made.  Then, use the ub_resolve_event call
 * to add DNS resolve queries to the context.  Those then run when you
 * call event_dispatch() on your event_base, and when they are done you
 * get a function callback.
 *
 * This method does not fork another process or create a thread, the effort
 * is done by the unbound state machines that are connected to the event_base.
 */
#ifndef _UB_UNBOUND_EVENT_H
#define _UB_UNBOUND_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

struct ub_ctx;
struct ub_result;
struct event_base;

typedef void (*ub_event_callback_t)(void*, int, void*, int, int, char*);

/**
 * Create a resolving and validation context.
 * The information from /etc/resolv.conf and /etc/hosts is not utilised by
 * default. Use ub_ctx_resolvconf and ub_ctx_hosts to read them.
 * @param base: the event base that the caller has created.  The unbound
 *	context uses this event base.
 * @return a new context. default initialisation.
 * 	returns NULL on error.
 * You must use ub_resolve_event with this context.
 * Do not call ub_ctx_async, ub_poll, ub_wait, ub_process, this is all done
 * with the event_base.  Setup the options you like with the other functions.
 */
struct ub_ctx* ub_ctx_create_event(struct event_base* base);

/**
 * Set a new event_base on a context created with ub_ctx_create_event.
 * Any outbound queries will be canceled.
 * @param ctx the ub_ctx to update.  Must have been created with ub_ctx_create_event
 * @param base the new event_base to attach to the ctx
 * @return 0 if OK, else error
 */
int ub_ctx_set_event(struct ub_ctx* ctx, struct event_base* base); 

/**
 * Perform resolution and validation of the target name.
 * Asynchronous, after a while, the callback will be called with your
 * data and the result.  Uses the event_base user installed by creating the
 * context with ub_ctx_create_event().
 * @param ctx: context with event_base in it.
 *	The context is finalized, and can no longer accept all config changes.
 * @param name: domain name in text format (a string).
 * @param rrtype: type of RR in host order, 1 is A.
 * @param rrclass: class of RR in host order, 1 is IN (for internet).
 * @param mydata: this data is your own data (you can pass NULL),
 * 	and is passed on to the callback function.
 * @param callback: this is called on completion of the resolution.
 * 	It is called as:
 * 	void callback(void* mydata, int rcode, void* packet, int packet_len,
 * 		int sec, char* why_bogus)
 * 	with mydata: the same as passed here, you may pass NULL,
 * 	with rcode: 0 on no error, nonzero for mostly SERVFAIL situations,
 *		this is a DNS rcode.
 *	with packet: a buffer with DNS wireformat packet with the answer.
 *		do not inspect if rcode != 0.
 *		do not write or free the packet buffer, it is used internally
 *		in unbound (for other callbacks that want the same data).
 *	with packet_len: length in bytes of the packet buffer.
 *	with sec: 0 if insecure, 1 if bogus, 2 if DNSSEC secure.
 *	with why_bogus: text string explaining why it is bogus (or NULL).
 *	These point to buffers inside unbound; do not deallocate the packet or
 *	error string.
 *
 * 	If an error happens during processing, your callback will be called
 * 	with error set to a nonzero value (and result==NULL).
 * 	For localdata (etc/hosts) the callback is called immediately, before
 * 	resolve_event returns, async_id=0 is returned.
 * @param async_id: if you pass a non-NULL value, an identifier number is
 *	returned for the query as it is in progress. It can be used to 
 *	cancel the query.
 * @return 0 if OK, else error.
 */
int ub_resolve_event(struct ub_ctx* ctx, const char* name, int rrtype, 
	int rrclass, void* mydata, ub_event_callback_t callback, int* async_id);

#ifdef __cplusplus
}
#endif

#endif /* _UB_UNBOUND_H */
