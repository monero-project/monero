/*
 * pythonmod_utils.c: utilities used by wrapper
 *
 * Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
 *                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 * 
 *    * Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 * 
 *    * Neither the name of the organization nor the names of its
 *      contributors may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file
 * Utility functions for the python module that perform stores and loads and
 * conversions.
 */
#include "config.h"
#include "util/module.h"
#include "util/netevent.h"
#include "util/net_help.h"
#include "services/cache/dns.h"
#include "services/cache/rrset.h"
#include "util/data/msgparse.h"
#include "util/data/msgreply.h"
#include "util/storage/slabhash.h"
#include "util/regional.h"
#include "iterator/iter_delegpt.h"
#include "sldns/sbuffer.h"

#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>

/* Store the reply_info and query_info pair in message cache (qstate->msg_cache) */
int storeQueryInCache(struct module_qstate* qstate, struct query_info* qinfo, struct reply_info* msgrep, int is_referral)
{
    if (!msgrep) 
       return 0;

    if (msgrep->authoritative) /*authoritative answer can't be stored in cache*/
    {
       PyErr_SetString(PyExc_ValueError, "Authoritative answer can't be stored");
       return 0;
    }

    return dns_cache_store(qstate->env, qinfo, msgrep, is_referral, 
	qstate->prefetch_leeway, 0, NULL, qstate->query_flags);
}

/*  Invalidate the message associated with query_info stored in message cache */
void invalidateQueryInCache(struct module_qstate* qstate, struct query_info* qinfo)
{ 
    hashvalue_t h;
    struct lruhash_entry* e;
    struct reply_info *r;
    size_t i, j;

    h = query_info_hash(qinfo, qstate->query_flags);
    if ((e=slabhash_lookup(qstate->env->msg_cache, h, qinfo, 0))) 
    {
	r = (struct reply_info*)(e->data);
	if (r) 
	{
	   r->ttl = 0;
	   if(rrset_array_lock(r->ref, r->rrset_count, *qstate->env->now)) {
		   for(i=0; i< r->rrset_count; i++) 
		   {
		       struct packed_rrset_data* data = 
		       	(struct packed_rrset_data*) r->ref[i].key->entry.data;
		       if(i>0 && r->ref[i].key == r->ref[i-1].key)
			   continue;
	      
		       data->ttl = r->ttl;
		       for(j=0; j<data->count + data->rrsig_count; j++)
			   data->rr_ttl[j] = r->ttl;
		   }
		   rrset_array_unlock(r->ref, r->rrset_count);
	   }
	}
	lock_rw_unlock(&e->lock);
    } else {
	log_info("invalidateQueryInCache: qinfo is not in cache");
    }
}

/* Create response according to the ldns packet content */
int createResponse(struct module_qstate* qstate, sldns_buffer* pkt)
{
    struct msg_parse* prs;
    struct edns_data edns;
    
    /* parse message */
    prs = (struct msg_parse*) regional_alloc(qstate->env->scratch, sizeof(struct msg_parse));
    if (!prs) {
	log_err("storeResponse: out of memory on incoming message");
	return 0;
    }

    memset(prs, 0, sizeof(*prs));
    memset(&edns, 0, sizeof(edns));

    sldns_buffer_set_position(pkt, 0);
    if (parse_packet(pkt, prs, qstate->env->scratch) != LDNS_RCODE_NOERROR) {
	verbose(VERB_ALGO, "storeResponse: parse error on reply packet");
	return 0;
    }
    /* edns is not examined, but removed from message to help cache */
    if(parse_extract_edns(prs, &edns) != LDNS_RCODE_NOERROR)
	return 0;

    /* remove CD-bit, we asked for in case we handle validation ourself */
    prs->flags &= ~BIT_CD;

    /* allocate response dns_msg in region */
    qstate->return_msg = (struct dns_msg*)regional_alloc(qstate->region, sizeof(struct dns_msg));
    if (!qstate->return_msg)
       return 0;

    memset(qstate->return_msg, 0, sizeof(*qstate->return_msg));
    if(!parse_create_msg(pkt, prs, NULL, &(qstate->return_msg)->qinfo, &(qstate->return_msg)->rep, qstate->region)) {
	log_err("storeResponse: malloc failure: allocating incoming dns_msg");
	return 0;
    }
    
    /* Make sure that the RA flag is set (since the presence of 
     * this module means that recursion is available) */
    /* qstate->return_msg->rep->flags |= BIT_RA; */

    /* Clear the AA flag */
    /* FIXME: does this action go here or in some other module? */
    /*qstate->return_msg->rep->flags &= ~BIT_AA; */

    /* make sure QR flag is on */
    /*qstate->return_msg->rep->flags |= BIT_QR; */

    if(verbosity >= VERB_ALGO)
	log_dns_msg("storeResponse: packet:", &qstate->return_msg->qinfo, qstate->return_msg->rep);

    return 1;
}


/* Convert reply->addr to string */
void reply_addr2str(struct comm_reply* reply, char* dest, int maxlen)
{
	int af = (int)((struct sockaddr_in*) &(reply->addr))->sin_family;
	void* sinaddr = &((struct sockaddr_in*) &(reply->addr))->sin_addr;

	if(af == AF_INET6)
		sinaddr = &((struct sockaddr_in6*)&(reply->addr))->sin6_addr;
	dest[0] = 0;
	if (inet_ntop(af, sinaddr, dest, (socklen_t)maxlen) == 0)
	   return;
	dest[maxlen-1] = 0;
}

/* Convert target->addr to string */
void delegpt_addr_addr2str(struct delegpt_addr* target, char *dest, int maxlen)
{
	int af = (int)((struct sockaddr_in*) &(target->addr))->sin_family;
	void* sinaddr = &((struct sockaddr_in*) &(target->addr))->sin_addr;

	if(af == AF_INET6)
		sinaddr = &((struct sockaddr_in6*)&(target->addr))->sin6_addr;
	dest[0] = 0;
	if (inet_ntop(af, sinaddr, dest, (socklen_t)maxlen) == 0)
	   return;
	dest[maxlen-1] = 0;
}
