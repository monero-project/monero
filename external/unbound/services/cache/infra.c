/*
 * services/cache/infra.c - infrastructure cache, server rtt and capabilities
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
 * This file contains the infrastructure cache.
 */
#include "config.h"
#include "ldns/rrdef.h"
#include "services/cache/infra.h"
#include "util/storage/slabhash.h"
#include "util/storage/lookup3.h"
#include "util/data/dname.h"
#include "util/log.h"
#include "util/net_help.h"
#include "util/config_file.h"
#include "iterator/iterator.h"

/** Timeout when only a single probe query per IP is allowed. */
#define PROBE_MAXRTO 12000 /* in msec */

/** number of timeouts for a type when the domain can be blocked ;
 * even if another type has completely rtt maxed it, the different type
 * can do this number of packets (until those all timeout too) */
#define TIMEOUT_COUNT_MAX 3

size_t 
infra_sizefunc(void* k, void* ATTR_UNUSED(d))
{
	struct infra_key* key = (struct infra_key*)k;
	return sizeof(*key) + sizeof(struct infra_data) + key->namelen
		+ lock_get_mem(&key->entry.lock);
}

int 
infra_compfunc(void* key1, void* key2)
{
	struct infra_key* k1 = (struct infra_key*)key1;
	struct infra_key* k2 = (struct infra_key*)key2;
	int r = sockaddr_cmp(&k1->addr, k1->addrlen, &k2->addr, k2->addrlen);
	if(r != 0)
		return r;
	if(k1->namelen != k2->namelen) {
		if(k1->namelen < k2->namelen)
			return -1;
		return 1;
	}
	return query_dname_compare(k1->zonename, k2->zonename);
}

void 
infra_delkeyfunc(void* k, void* ATTR_UNUSED(arg))
{
	struct infra_key* key = (struct infra_key*)k;
	if(!key)
		return;
	lock_rw_destroy(&key->entry.lock);
	free(key->zonename);
	free(key);
}

void 
infra_deldatafunc(void* d, void* ATTR_UNUSED(arg))
{
	struct infra_data* data = (struct infra_data*)d;
	free(data);
}

struct infra_cache* 
infra_create(struct config_file* cfg)
{
	struct infra_cache* infra = (struct infra_cache*)calloc(1, 
		sizeof(struct infra_cache));
	size_t maxmem = cfg->infra_cache_numhosts * (sizeof(struct infra_key)+
		sizeof(struct infra_data)+INFRA_BYTES_NAME);
	infra->hosts = slabhash_create(cfg->infra_cache_slabs,
		INFRA_HOST_STARTSIZE, maxmem, &infra_sizefunc, &infra_compfunc,
		&infra_delkeyfunc, &infra_deldatafunc, NULL);
	if(!infra->hosts) {
		free(infra);
		return NULL;
	}
	infra->host_ttl = cfg->host_ttl;
	return infra;
}

void 
infra_delete(struct infra_cache* infra)
{
	if(!infra)
		return;
	slabhash_delete(infra->hosts);
	free(infra);
}

struct infra_cache* 
infra_adjust(struct infra_cache* infra, struct config_file* cfg)
{
	size_t maxmem;
	if(!infra)
		return infra_create(cfg);
	infra->host_ttl = cfg->host_ttl;
	maxmem = cfg->infra_cache_numhosts * (sizeof(struct infra_key)+
		sizeof(struct infra_data)+INFRA_BYTES_NAME);
	if(maxmem != slabhash_get_size(infra->hosts) ||
		cfg->infra_cache_slabs != infra->hosts->size) {
		infra_delete(infra);
		infra = infra_create(cfg);
	}
	return infra;
}

/** calculate the hash value for a host key */
static hashvalue_t
hash_addr(struct sockaddr_storage* addr, socklen_t addrlen)
{
	hashvalue_t h = 0xab;
	/* select the pieces to hash, some OS have changing data inside */
	if(addr_is_ip6(addr, addrlen)) {
		struct sockaddr_in6* in6 = (struct sockaddr_in6*)addr;
		h = hashlittle(&in6->sin6_family, sizeof(in6->sin6_family), h);
		h = hashlittle(&in6->sin6_port, sizeof(in6->sin6_port), h);
		h = hashlittle(&in6->sin6_addr, INET6_SIZE, h);
	} else {
		struct sockaddr_in* in = (struct sockaddr_in*)addr;
		h = hashlittle(&in->sin_family, sizeof(in->sin_family), h);
		h = hashlittle(&in->sin_port, sizeof(in->sin_port), h);
		h = hashlittle(&in->sin_addr, INET_SIZE, h);
	}
	return h;
}

/** calculate infra hash for a key */
static hashvalue_t
hash_infra(struct sockaddr_storage* addr, socklen_t addrlen, uint8_t* name)
{
	return dname_query_hash(name, hash_addr(addr, addrlen));
}

/** lookup version that does not check host ttl (you check it) */
struct lruhash_entry* 
infra_lookup_nottl(struct infra_cache* infra, struct sockaddr_storage* addr,
	socklen_t addrlen, uint8_t* name, size_t namelen, int wr)
{
	struct infra_key k;
	k.addrlen = addrlen;
	memcpy(&k.addr, addr, addrlen);
	k.namelen = namelen;
	k.zonename = name;
	k.entry.hash = hash_infra(addr, addrlen, name);
	k.entry.key = (void*)&k;
	k.entry.data = NULL;
	return slabhash_lookup(infra->hosts, k.entry.hash, &k, wr);
}

/** init the data elements */
static void
data_entry_init(struct infra_cache* infra, struct lruhash_entry* e, 
	time_t timenow)
{
	struct infra_data* data = (struct infra_data*)e->data;
	data->ttl = timenow + infra->host_ttl;
	rtt_init(&data->rtt);
	data->edns_version = 0;
	data->edns_lame_known = 0;
	data->probedelay = 0;
	data->isdnsseclame = 0;
	data->rec_lame = 0;
	data->lame_type_A = 0;
	data->lame_other = 0;
	data->timeout_A = 0;
	data->timeout_AAAA = 0;
	data->timeout_other = 0;
}

/** 
 * Create and init a new entry for a host 
 * @param infra: infra structure with config parameters.
 * @param addr: host address.
 * @param addrlen: length of addr.
 * @param name: name of zone
 * @param namelen: length of name.
 * @param tm: time now.
 * @return: the new entry or NULL on malloc failure.
 */
static struct lruhash_entry*
new_entry(struct infra_cache* infra, struct sockaddr_storage* addr, 
	socklen_t addrlen, uint8_t* name, size_t namelen, time_t tm)
{
	struct infra_data* data;
	struct infra_key* key = (struct infra_key*)malloc(sizeof(*key));
	if(!key)
		return NULL;
	data = (struct infra_data*)malloc(sizeof(struct infra_data));
	if(!data) {
		free(key);
		return NULL;
	}
	key->zonename = memdup(name, namelen);
	if(!key->zonename) {
		free(key);
		free(data);
		return NULL;
	}
	key->namelen = namelen;
	lock_rw_init(&key->entry.lock);
	key->entry.hash = hash_infra(addr, addrlen, name);
	key->entry.key = (void*)key;
	key->entry.data = (void*)data;
	key->addrlen = addrlen;
	memcpy(&key->addr, addr, addrlen);
	data_entry_init(infra, &key->entry, tm);
	return &key->entry;
}

int 
infra_host(struct infra_cache* infra, struct sockaddr_storage* addr,
        socklen_t addrlen, uint8_t* nm, size_t nmlen, time_t timenow,
	int* edns_vs, uint8_t* edns_lame_known, int* to)
{
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		nm, nmlen, 0);
	struct infra_data* data;
	int wr = 0;
	if(e && ((struct infra_data*)e->data)->ttl < timenow) {
		/* it expired, try to reuse existing entry */
		int old = ((struct infra_data*)e->data)->rtt.rto;
		uint8_t tA = ((struct infra_data*)e->data)->timeout_A;
		uint8_t tAAAA = ((struct infra_data*)e->data)->timeout_AAAA;
		uint8_t tother = ((struct infra_data*)e->data)->timeout_other;
		lock_rw_unlock(&e->lock);
		e = infra_lookup_nottl(infra, addr, addrlen, nm, nmlen, 1);
		if(e) {
			/* if its still there we have a writelock, init */
			/* re-initialise */
			/* do not touch lameness, it may be valid still */
			data_entry_init(infra, e, timenow);
			wr = 1;
			/* TOP_TIMEOUT remains on reuse */
			if(old >= USEFUL_SERVER_TOP_TIMEOUT) {
				((struct infra_data*)e->data)->rtt.rto
					= USEFUL_SERVER_TOP_TIMEOUT;
				((struct infra_data*)e->data)->timeout_A = tA;
				((struct infra_data*)e->data)->timeout_AAAA = tAAAA;
				((struct infra_data*)e->data)->timeout_other = tother;
			}
		}
	}
	if(!e) {
		/* insert new entry */
		if(!(e = new_entry(infra, addr, addrlen, nm, nmlen, timenow)))
			return 0;
		data = (struct infra_data*)e->data;
		*edns_vs = data->edns_version;
		*edns_lame_known = data->edns_lame_known;
		*to = rtt_timeout(&data->rtt);
		slabhash_insert(infra->hosts, e->hash, e, data, NULL);
		return 1;
	}
	/* use existing entry */
	data = (struct infra_data*)e->data;
	*edns_vs = data->edns_version;
	*edns_lame_known = data->edns_lame_known;
	*to = rtt_timeout(&data->rtt);
	if(*to >= PROBE_MAXRTO && rtt_notimeout(&data->rtt)*4 <= *to) {
		/* delay other queries, this is the probe query */
		if(!wr) {
			lock_rw_unlock(&e->lock);
			e = infra_lookup_nottl(infra, addr,addrlen,nm,nmlen, 1);
			if(!e) { /* flushed from cache real fast, no use to
				allocate just for the probedelay */
				return 1;
			}
			data = (struct infra_data*)e->data;
		}
		/* add 999 to round up the timeout value from msec to sec,
		 * then add a whole second so it is certain that this probe
		 * has timed out before the next is allowed */
		data->probedelay = timenow + ((*to)+1999)/1000;
	}
	lock_rw_unlock(&e->lock);
	return 1;
}

int 
infra_set_lame(struct infra_cache* infra, struct sockaddr_storage* addr,
	socklen_t addrlen, uint8_t* nm, size_t nmlen, time_t timenow,
	int dnsseclame, int reclame, uint16_t qtype)
{
	struct infra_data* data;
	struct lruhash_entry* e;
	int needtoinsert = 0;
	e = infra_lookup_nottl(infra, addr, addrlen, nm, nmlen, 1);
	if(!e) {
		/* insert it */
		if(!(e = new_entry(infra, addr, addrlen, nm, nmlen, timenow))) {
			log_err("set_lame: malloc failure");
			return 0;
		}
		needtoinsert = 1;
	} else if( ((struct infra_data*)e->data)->ttl < timenow) {
		/* expired, reuse existing entry */
		data_entry_init(infra, e, timenow);
	}
	/* got an entry, now set the zone lame */
	data = (struct infra_data*)e->data;
	/* merge data (if any) */
	if(dnsseclame)
		data->isdnsseclame = 1;
	if(reclame)
		data->rec_lame = 1;
	if(!dnsseclame && !reclame && qtype == LDNS_RR_TYPE_A)
		data->lame_type_A = 1;
	if(!dnsseclame  && !reclame && qtype != LDNS_RR_TYPE_A)
		data->lame_other = 1;
	/* done */
	if(needtoinsert)
		slabhash_insert(infra->hosts, e->hash, e, e->data, NULL);
	else 	{ lock_rw_unlock(&e->lock); }
	return 1;
}

void 
infra_update_tcp_works(struct infra_cache* infra,
        struct sockaddr_storage* addr, socklen_t addrlen, uint8_t* nm,
	size_t nmlen)
{
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		nm, nmlen, 1);
	struct infra_data* data;
	if(!e)
		return; /* doesn't exist */
	data = (struct infra_data*)e->data;
	if(data->rtt.rto >= RTT_MAX_TIMEOUT)
		/* do not disqualify this server altogether, it is better
		 * than nothing */
		data->rtt.rto = RTT_MAX_TIMEOUT-1000;
	lock_rw_unlock(&e->lock);
}

int 
infra_rtt_update(struct infra_cache* infra, struct sockaddr_storage* addr,
	socklen_t addrlen, uint8_t* nm, size_t nmlen, int qtype,
	int roundtrip, int orig_rtt, time_t timenow)
{
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		nm, nmlen, 1);
	struct infra_data* data;
	int needtoinsert = 0;
	int rto = 1;
	if(!e) {
		if(!(e = new_entry(infra, addr, addrlen, nm, nmlen, timenow)))
			return 0;
		needtoinsert = 1;
	} else if(((struct infra_data*)e->data)->ttl < timenow) {
		data_entry_init(infra, e, timenow);
	}
	/* have an entry, update the rtt */
	data = (struct infra_data*)e->data;
	if(roundtrip == -1) {
		rtt_lost(&data->rtt, orig_rtt);
		if(qtype == LDNS_RR_TYPE_A) {
			if(data->timeout_A < TIMEOUT_COUNT_MAX)
				data->timeout_A++;
		} else if(qtype == LDNS_RR_TYPE_AAAA) {
			if(data->timeout_AAAA < TIMEOUT_COUNT_MAX)
				data->timeout_AAAA++;
		} else {
			if(data->timeout_other < TIMEOUT_COUNT_MAX)
				data->timeout_other++;
		}
	} else {
		/* if we got a reply, but the old timeout was above server
		 * selection height, delete the timeout so the server is
		 * fully available again */
		if(rtt_unclamped(&data->rtt) >= USEFUL_SERVER_TOP_TIMEOUT)
			rtt_init(&data->rtt);
		rtt_update(&data->rtt, roundtrip);
		data->probedelay = 0;
		if(qtype == LDNS_RR_TYPE_A)
			data->timeout_A = 0;
		else if(qtype == LDNS_RR_TYPE_AAAA)
			data->timeout_AAAA = 0;
		else	data->timeout_other = 0;
	}
	if(data->rtt.rto > 0)
		rto = data->rtt.rto;

	if(needtoinsert)
		slabhash_insert(infra->hosts, e->hash, e, e->data, NULL);
	else 	{ lock_rw_unlock(&e->lock); }
	return rto;
}

long long infra_get_host_rto(struct infra_cache* infra,
        struct sockaddr_storage* addr, socklen_t addrlen, uint8_t* nm,
	size_t nmlen, struct rtt_info* rtt, int* delay, time_t timenow,
	int* tA, int* tAAAA, int* tother)
{
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		nm, nmlen, 0);
	struct infra_data* data;
	long long ttl = -2;
	if(!e) return -1;
	data = (struct infra_data*)e->data;
	if(data->ttl >= timenow) {
		ttl = (long long)(data->ttl - timenow);
		memmove(rtt, &data->rtt, sizeof(*rtt));
		if(timenow < data->probedelay)
			*delay = (int)(data->probedelay - timenow);
		else	*delay = 0;
	}
	*tA = (int)data->timeout_A;
	*tAAAA = (int)data->timeout_AAAA;
	*tother = (int)data->timeout_other;
	lock_rw_unlock(&e->lock);
	return ttl;
}

int 
infra_edns_update(struct infra_cache* infra, struct sockaddr_storage* addr,
	socklen_t addrlen, uint8_t* nm, size_t nmlen, int edns_version,
	time_t timenow)
{
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		nm, nmlen, 1);
	struct infra_data* data;
	int needtoinsert = 0;
	if(!e) {
		if(!(e = new_entry(infra, addr, addrlen, nm, nmlen, timenow)))
			return 0;
		needtoinsert = 1;
	} else if(((struct infra_data*)e->data)->ttl < timenow) {
		data_entry_init(infra, e, timenow);
	}
	/* have an entry, update the rtt, and the ttl */
	data = (struct infra_data*)e->data;
	/* do not update if noEDNS and stored is yesEDNS */
	if(!(edns_version == -1 && (data->edns_version != -1 &&
		data->edns_lame_known))) {
		data->edns_version = edns_version;
		data->edns_lame_known = 1;
	}

	if(needtoinsert)
		slabhash_insert(infra->hosts, e->hash, e, e->data, NULL);
	else 	{ lock_rw_unlock(&e->lock); }
	return 1;
}

int
infra_get_lame_rtt(struct infra_cache* infra,
        struct sockaddr_storage* addr, socklen_t addrlen,
        uint8_t* name, size_t namelen, uint16_t qtype, 
	int* lame, int* dnsseclame, int* reclame, int* rtt, time_t timenow)
{
	struct infra_data* host;
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		name, namelen, 0);
	if(!e) 
		return 0;
	host = (struct infra_data*)e->data;
	*rtt = rtt_unclamped(&host->rtt);
	if(host->rtt.rto >= PROBE_MAXRTO && timenow < host->probedelay
		&& rtt_notimeout(&host->rtt)*4 <= host->rtt.rto) {
		/* single probe for this domain, and we are not probing */
		/* unless the query type allows a probe to happen */
		if(qtype == LDNS_RR_TYPE_A) {
			if(host->timeout_A >= TIMEOUT_COUNT_MAX)
				*rtt = USEFUL_SERVER_TOP_TIMEOUT;
			else	*rtt = USEFUL_SERVER_TOP_TIMEOUT-1000;
		} else if(qtype == LDNS_RR_TYPE_AAAA) {
			if(host->timeout_AAAA >= TIMEOUT_COUNT_MAX)
				*rtt = USEFUL_SERVER_TOP_TIMEOUT;
			else	*rtt = USEFUL_SERVER_TOP_TIMEOUT-1000;
		} else {
			if(host->timeout_other >= TIMEOUT_COUNT_MAX)
				*rtt = USEFUL_SERVER_TOP_TIMEOUT;
			else	*rtt = USEFUL_SERVER_TOP_TIMEOUT-1000;
		}
	}
	if(timenow > host->ttl) {
		/* expired entry */
		/* see if this can be a re-probe of an unresponsive server */
		/* minus 1000 because that is outside of the RTTBAND, so
		 * blacklisted servers stay blacklisted if this is chosen */
		if(host->rtt.rto >= USEFUL_SERVER_TOP_TIMEOUT) {
			lock_rw_unlock(&e->lock);
			*rtt = USEFUL_SERVER_TOP_TIMEOUT-1000;
			*lame = 0;
			*dnsseclame = 0;
			*reclame = 0;
			return 1;
		}
		lock_rw_unlock(&e->lock);
		return 0;
	}
	/* check lameness first */
	if(host->lame_type_A && qtype == LDNS_RR_TYPE_A) {
		lock_rw_unlock(&e->lock);
		*lame = 1;
		*dnsseclame = 0;
		*reclame = 0;
		return 1;
	} else if(host->lame_other && qtype != LDNS_RR_TYPE_A) {
		lock_rw_unlock(&e->lock);
		*lame = 1;
		*dnsseclame = 0;
		*reclame = 0;
		return 1;
	} else if(host->isdnsseclame) {
		lock_rw_unlock(&e->lock);
		*lame = 0;
		*dnsseclame = 1;
		*reclame = 0;
		return 1;
	} else if(host->rec_lame) {
		lock_rw_unlock(&e->lock);
		*lame = 0;
		*dnsseclame = 0;
		*reclame = 1;
		return 1;
	}
	/* no lameness for this type of query */
	lock_rw_unlock(&e->lock);
	*lame = 0;
	*dnsseclame = 0;
	*reclame = 0;
	return 1;
}

size_t 
infra_get_mem(struct infra_cache* infra)
{
	return sizeof(*infra) + slabhash_get_mem(infra->hosts);
}
