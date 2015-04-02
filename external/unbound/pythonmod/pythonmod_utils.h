/*
 * pythonmod_utils.h: utils header file
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
#ifndef PYTHONMOD_UTILS_H
#define PYTHONMOD_UTILS_H

#include "util/module.h"
struct delegpt_addr;

/**
 *  Store the reply_info and query_info pair in message cache (qstate->msg_cache)
 *
 * @param qstate: module environment
 * @param qinfo: query info, the query for which answer is stored.
 * @param msgrep: reply in dns_msg
 * @param is_referral: If true, then the given message to be stored is a
 *      referral. The cache implementation may use this as a hint.
 *      It will store only the RRsets, not the message.
 * @return 0 on alloc error (out of memory).
 */
int storeQueryInCache(struct module_qstate* qstate, struct query_info* qinfo, struct reply_info* msgrep, int is_referral);


/**
 *  Invalidate the message associated with query_info stored in message cache.
 *
 *  This function invalidates the record in message cache associated with the given query only if such a record exists.
 *
 * @param qstate: module environment
 * @param qinfo: query info, the query for which answer is stored.
 */
void invalidateQueryInCache(struct module_qstate* qstate, struct query_info* qinfo);

/**
 *  Create response according to the ldns packet content
 *
 *  This function fills qstate.return_msg up with data of a given packet
 * 
 * @param qstate: module environment
 * @param pkt: a sldns_buffer which contains sldns_packet data
 * @return 0 on failure, out of memory or parse error.
 */
int createResponse(struct module_qstate* qstate, sldns_buffer* pkt);

/**
 *  Convert reply->addr to string
 *  @param reply: comm reply with address in it.
 *  @param dest: destination string.
 *  @param maxlen: length of string buffer.
 */
void reply_addr2str(struct comm_reply* reply, char* dest, int maxlen);

/* Convert target->addr to string */
void delegpt_addr_addr2str(struct delegpt_addr* target, char *dest, int maxlen);

#endif /* PYTHONMOD_UTILS_H */
