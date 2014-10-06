/*
 * services/cache/dns.h - Cache services for DNS using msg and rrset caches.
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
 * This file contains the DNS cache.
 */

#ifndef SERVICES_CACHE_DNS_H
#define SERVICES_CACHE_DNS_H
#include "util/storage/lruhash.h"
#include "util/data/msgreply.h"
struct module_env;
struct query_info;
struct reply_info;
struct regional;
struct delegpt;

/**
 * Region allocated message reply
 */
struct dns_msg {
	/** query info */
	struct query_info qinfo;
	/** reply info - ptr to packed repinfo structure */
	struct reply_info *rep;
};

/**
 * Allocate a dns_msg with malloc/alloc structure and store in dns cache.
 *
 * @param env: environment, with alloc structure and dns cache.
 * @param qinf: query info, the query for which answer is stored.
 * 	this is allocated in a region, and will be copied to malloc area
 * 	before insertion.
 * @param rep: reply in dns_msg from dns_alloc_msg for example.
 * 	this is allocated in a region, and will be copied to malloc area
 * 	before insertion.
 * @param is_referral: If true, then the given message to be stored is a
 *      referral. The cache implementation may use this as a hint.
 *      It will store only the RRsets, not the message.
 * @param leeway: TTL value, if not 0, other rrsets are considered expired
 *	that many seconds before actual TTL expiry.
 * @param pside: if true, information came from a server which was fetched
 * 	from the parentside of the zonecut.  This means that the type NS
 * 	can be updated to full TTL even in prefetch situations.
 * @param region: region to allocate better entries from cache into.
 *   (used when is_referral is false).
 * @return 0 on alloc error (out of memory).
 */
int dns_cache_store(struct module_env* env, struct query_info* qinf,
        struct reply_info* rep, int is_referral, time_t leeway, int pside,
	struct regional* region); 

/**
 * Store message in the cache. Stores in message cache and rrset cache.
 * Both qinfo and rep should be malloced and are put in the cache.
 * They should not be used after this call, as they are then in shared cache.
 * Does not return errors, they are logged and only lead to less cache.
 *
 * @param env: module environment with the DNS cache.
 * @param qinfo: query info
 * @param hash: hash over qinfo.
 * @param rep: reply info, together with qinfo makes up the message.
 *	Adjusts the reply info TTLs to absolute time.
 * @param leeway: TTL value, if not 0, other rrsets are considered expired
 *	that many seconds before actual TTL expiry.
 * @param pside: if true, information came from a server which was fetched
 * 	from the parentside of the zonecut.  This means that the type NS
 * 	can be updated to full TTL even in prefetch situations.
 * @param qrep: message that can be altered with better rrs from cache.
 * @param region: to allocate into for qmsg.
 */
void dns_cache_store_msg(struct module_env* env, struct query_info* qinfo,
	hashvalue_t hash, struct reply_info* rep, time_t leeway, int pside,
	struct reply_info* qrep, struct regional* region);

/**
 * Find a delegation from the cache.
 * @param env: module environment with the DNS cache.
 * @param qname: query name.
 * @param qnamelen: length of qname.
 * @param qtype: query type.
 * @param qclass: query class.
 * @param region: where to allocate result delegation.
 * @param msg: if not NULL, delegation message is returned here, synthesized
 *	from the cache.
 * @param timenow: the time now, for checking if TTL on cache entries is OK.
 * @return new delegation or NULL on error or if not found in cache.
 */
struct delegpt* dns_cache_find_delegation(struct module_env* env, 
	uint8_t* qname, size_t qnamelen, uint16_t qtype, uint16_t qclass, 
	struct regional* region, struct dns_msg** msg, time_t timenow);

/** 
 * Find cached message 
 * @param env: module environment with the DNS cache.
 * @param qname: query name.
 * @param qnamelen: length of qname.
 * @param qtype: query type.
 * @param qclass: query class.
 * @param region: where to allocate result.
 * @param scratch: where to allocate temporary data.
 * @return new response message (alloced in region, rrsets do not have IDs).
 * 	or NULL on error or if not found in cache.
 *	TTLs are made relative to the current time.
 */
struct dns_msg* dns_cache_lookup(struct module_env* env,
	uint8_t* qname, size_t qnamelen, uint16_t qtype, uint16_t qclass,
	struct regional* region, struct regional* scratch);

/** 
 * find and add A and AAAA records for missing nameservers in delegpt 
 * @param env: module environment with rrset cache
 * @param qclass: which class to look in.
 * @param region: where to store new dp info.
 * @param dp: delegation point to fill missing entries.
 * @return false on alloc failure.
 */
int cache_fill_missing(struct module_env* env, uint16_t qclass, 
	struct regional* region, struct delegpt* dp);

/**
 * Utility, create new, unpacked data structure for cache response.
 * QR bit set, no AA. Query set as indicated. Space for number of rrsets.
 * @param qname: query section name
 * @param qnamelen: len of qname
 * @param qtype: query section type
 * @param qclass: query section class
 * @param region: where to alloc.
 * @param capacity: number of rrsets space to create in the array.
 * @return new dns_msg struct or NULL on mem fail.
 */
struct dns_msg* dns_msg_create(uint8_t* qname, size_t qnamelen, uint16_t qtype, 
	uint16_t qclass, struct regional* region, size_t capacity);

/**
 * Add rrset to authority section in unpacked dns_msg message. Must have enough
 * space left, does not grow the array.
 * @param msg: msg to put it in.
 * @param region: region to alloc in
 * @param rrset: to add in authority section
 * @param now: now.
 * @return true if worked, false on fail
 */
int dns_msg_authadd(struct dns_msg* msg, struct regional* region, 
	struct ub_packed_rrset_key* rrset, time_t now);

/**
 * Adjust the prefetch_ttl for a cached message.  This adds a value to the
 * prefetch ttl - postponing the time when it will be prefetched for future
 * incoming queries.
 * @param env: module environment with caches and time.
 * @param qinfo: query info for the query that needs adjustment.
 * @param adjust: time in seconds to add to the prefetch_leeway.
 * @return false if not in cache. true if added.
 */
int dns_cache_prefetch_adjust(struct module_env* env, struct query_info* qinfo,
        time_t adjust);

#endif /* SERVICES_CACHE_DNS_H */
