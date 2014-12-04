/*
 * util/data/msgreply.c - store message and reply data. 
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
 * This file contains a data structure to store a message and its reply.
 */

#include "config.h"
#include "util/data/msgreply.h"
#include "util/storage/lookup3.h"
#include "util/log.h"
#include "util/alloc.h"
#include "util/netevent.h"
#include "util/net_help.h"
#include "util/data/dname.h"
#include "util/regional.h"
#include "util/data/msgparse.h"
#include "util/data/msgencode.h"
#include "ldns/sbuffer.h"
#include "ldns/wire2str.h"

/** MAX TTL default for messages and rrsets */
time_t MAX_TTL = 3600 * 24 * 10; /* ten days */
/** MIN TTL default for messages and rrsets */
time_t MIN_TTL = 0;

/** allocate qinfo, return 0 on error */
static int
parse_create_qinfo(sldns_buffer* pkt, struct msg_parse* msg, 
	struct query_info* qinf, struct regional* region)
{
	if(msg->qname) {
		if(region)
			qinf->qname = (uint8_t*)regional_alloc(region, 
				msg->qname_len);
		else	qinf->qname = (uint8_t*)malloc(msg->qname_len);
		if(!qinf->qname) return 0;
		dname_pkt_copy(pkt, qinf->qname, msg->qname);
	} else	qinf->qname = 0;
	qinf->qname_len = msg->qname_len;
	qinf->qtype = msg->qtype;
	qinf->qclass = msg->qclass;
	return 1;
}

/** constructor for replyinfo */
struct reply_info*
construct_reply_info_base(struct regional* region, uint16_t flags, size_t qd,
	time_t ttl, time_t prettl, size_t an, size_t ns, size_t ar, 
	size_t total, enum sec_status sec)
{
	struct reply_info* rep;
	/* rrset_count-1 because the first ref is part of the struct. */
	size_t s = sizeof(struct reply_info) - sizeof(struct rrset_ref) +
		sizeof(struct ub_packed_rrset_key*) * total;
	if(region)
		rep = (struct reply_info*)regional_alloc(region, s);
	else	rep = (struct reply_info*)malloc(s + 
			sizeof(struct rrset_ref) * (total));
	if(!rep) 
		return NULL;
	rep->flags = flags;
	rep->qdcount = qd;
	rep->ttl = ttl;
	rep->prefetch_ttl = prettl;
	rep->an_numrrsets = an;
	rep->ns_numrrsets = ns;
	rep->ar_numrrsets = ar;
	rep->rrset_count = total;
	rep->security = sec;
	rep->authoritative = 0;
	/* array starts after the refs */
	if(region)
		rep->rrsets = (struct ub_packed_rrset_key**)&(rep->ref[0]);
	else	rep->rrsets = (struct ub_packed_rrset_key**)&(rep->ref[total]);
	/* zero the arrays to assist cleanup in case of malloc failure */
	memset( rep->rrsets, 0, sizeof(struct ub_packed_rrset_key*) * total);
	if(!region)
		memset( &rep->ref[0], 0, sizeof(struct rrset_ref) * total);
	return rep;
}

/** allocate replyinfo, return 0 on error */
static int
parse_create_repinfo(struct msg_parse* msg, struct reply_info** rep,
	struct regional* region)
{
	*rep = construct_reply_info_base(region, msg->flags, msg->qdcount, 0, 
		0, msg->an_rrsets, msg->ns_rrsets, msg->ar_rrsets, 
		msg->rrset_count, sec_status_unchecked);
	if(!*rep)
		return 0;
	return 1;
}

/** allocate (special) rrset keys, return 0 on error */
static int
repinfo_alloc_rrset_keys(struct reply_info* rep, struct alloc_cache* alloc, 
	struct regional* region)
{
	size_t i;
	for(i=0; i<rep->rrset_count; i++) {
		if(region) {
			rep->rrsets[i] = (struct ub_packed_rrset_key*)
				regional_alloc(region, 
				sizeof(struct ub_packed_rrset_key));
			if(rep->rrsets[i]) {
				memset(rep->rrsets[i], 0, 
					sizeof(struct ub_packed_rrset_key));
				rep->rrsets[i]->entry.key = rep->rrsets[i];
			}
		}
		else	rep->rrsets[i] = alloc_special_obtain(alloc);
		if(!rep->rrsets[i])
			return 0;
		rep->rrsets[i]->entry.data = NULL;
	}
	return 1;
}

/** do the rdata copy */
static int
rdata_copy(sldns_buffer* pkt, struct packed_rrset_data* data, uint8_t* to, 
	struct rr_parse* rr, time_t* rr_ttl, uint16_t type)
{
	uint16_t pkt_len;
	const sldns_rr_descriptor* desc;

	*rr_ttl = sldns_read_uint32(rr->ttl_data);
	/* RFC 2181 Section 8. if msb of ttl is set treat as if zero. */
	if(*rr_ttl & 0x80000000U)
		*rr_ttl = 0;
	if(*rr_ttl < MIN_TTL)
		*rr_ttl = MIN_TTL;
	if(*rr_ttl < data->ttl)
		data->ttl = *rr_ttl;

	if(rr->outside_packet) {
		/* uncompressed already, only needs copy */
		memmove(to, rr->ttl_data+sizeof(uint32_t), rr->size);
		return 1;
	}

	sldns_buffer_set_position(pkt, (size_t)
		(rr->ttl_data - sldns_buffer_begin(pkt) + sizeof(uint32_t)));
	/* insert decompressed size into rdata len stored in memory */
	/* -2 because rdatalen bytes are not included. */
	pkt_len = htons(rr->size - 2);
	memmove(to, &pkt_len, sizeof(uint16_t));
	to += 2;
	/* read packet rdata len */
	pkt_len = sldns_buffer_read_u16(pkt);
	if(sldns_buffer_remaining(pkt) < pkt_len)
		return 0;
	desc = sldns_rr_descript(type);
	if(pkt_len > 0 && desc && desc->_dname_count > 0) {
		int count = (int)desc->_dname_count;
		int rdf = 0;
		size_t len;
		size_t oldpos;
		/* decompress dnames. */
		while(pkt_len > 0 && count) {
			switch(desc->_wireformat[rdf]) {
			case LDNS_RDF_TYPE_DNAME:
				oldpos = sldns_buffer_position(pkt);
				dname_pkt_copy(pkt, to, 
					sldns_buffer_current(pkt));
				to += pkt_dname_len(pkt);
				pkt_len -= sldns_buffer_position(pkt)-oldpos;
				count--;
				len = 0;
				break;
			case LDNS_RDF_TYPE_STR:
				len = sldns_buffer_current(pkt)[0] + 1;
				break;
			default:
				len = get_rdf_size(desc->_wireformat[rdf]);
				break;
			}
			if(len) {
				memmove(to, sldns_buffer_current(pkt), len);
				to += len;
				sldns_buffer_skip(pkt, (ssize_t)len);
				log_assert(len <= pkt_len);
				pkt_len -= len;
			}
			rdf++;
		}
	}
	/* copy remaining rdata */
	if(pkt_len >  0)
		memmove(to, sldns_buffer_current(pkt), pkt_len);
	
	return 1;
}

/** copy over the data into packed rrset */
static int
parse_rr_copy(sldns_buffer* pkt, struct rrset_parse* pset, 
	struct packed_rrset_data* data)
{
	size_t i;
	struct rr_parse* rr = pset->rr_first;
	uint8_t* nextrdata;
	size_t total = pset->rr_count + pset->rrsig_count;
	data->ttl = MAX_TTL;
	data->count = pset->rr_count;
	data->rrsig_count = pset->rrsig_count;
	data->trust = rrset_trust_none;
	data->security = sec_status_unchecked;
	/* layout: struct - rr_len - rr_data - rr_ttl - rdata - rrsig */
	data->rr_len = (size_t*)((uint8_t*)data + 
		sizeof(struct packed_rrset_data));
	data->rr_data = (uint8_t**)&(data->rr_len[total]);
	data->rr_ttl = (time_t*)&(data->rr_data[total]);
	nextrdata = (uint8_t*)&(data->rr_ttl[total]);
	for(i=0; i<data->count; i++) {
		data->rr_len[i] = rr->size;
		data->rr_data[i] = nextrdata;
		nextrdata += rr->size;
		if(!rdata_copy(pkt, data, data->rr_data[i], rr, 
			&data->rr_ttl[i], pset->type))
			return 0;
		rr = rr->next;
	}
	/* if rrsig, its rdata is at nextrdata */
	rr = pset->rrsig_first;
	for(i=data->count; i<total; i++) {
		data->rr_len[i] = rr->size;
		data->rr_data[i] = nextrdata;
		nextrdata += rr->size;
		if(!rdata_copy(pkt, data, data->rr_data[i], rr, 
			&data->rr_ttl[i], LDNS_RR_TYPE_RRSIG))
			return 0;
		rr = rr->next;
	}
	return 1;
}

/** create rrset return 0 on failure */
static int
parse_create_rrset(sldns_buffer* pkt, struct rrset_parse* pset,
	struct packed_rrset_data** data, struct regional* region)
{
	/* allocate */
	size_t s = sizeof(struct packed_rrset_data) + 
		(pset->rr_count + pset->rrsig_count) * 
		(sizeof(size_t)+sizeof(uint8_t*)+sizeof(time_t)) + 
		pset->size;
	if(region)
		*data = regional_alloc(region, s);
	else	*data = malloc(s);
	if(!*data)
		return 0;
	/* copy & decompress */
	if(!parse_rr_copy(pkt, pset, *data)) {
		if(!region) free(*data);
		return 0;
	}
	return 1;
}

/** get trust value for rrset */
static enum rrset_trust
get_rrset_trust(struct msg_parse* msg, struct rrset_parse* rrset)
{
	uint16_t AA = msg->flags & BIT_AA;
	if(rrset->section == LDNS_SECTION_ANSWER) {
		if(AA) {
			/* RFC2181 says remainder of CNAME chain is nonauth*/
			if(msg->rrset_first && 
				msg->rrset_first->section==LDNS_SECTION_ANSWER
				&& msg->rrset_first->type==LDNS_RR_TYPE_CNAME){
				if(rrset == msg->rrset_first)
					return rrset_trust_ans_AA;
				else 	return rrset_trust_ans_noAA;
			}
			if(msg->rrset_first && 
				msg->rrset_first->section==LDNS_SECTION_ANSWER
				&& msg->rrset_first->type==LDNS_RR_TYPE_DNAME){
				if(rrset == msg->rrset_first ||
				   rrset == msg->rrset_first->rrset_all_next)
					return rrset_trust_ans_AA;
				else 	return rrset_trust_ans_noAA;
			}
			return rrset_trust_ans_AA;
		}
		else	return rrset_trust_ans_noAA;
	} else if(rrset->section == LDNS_SECTION_AUTHORITY) {
		if(AA)	return rrset_trust_auth_AA;
		else	return rrset_trust_auth_noAA;
	} else {
		/* addit section */
		if(AA)	return rrset_trust_add_AA;
		else	return rrset_trust_add_noAA;
	}
	/* NOTREACHED */
	return rrset_trust_none;
}

int
parse_copy_decompress_rrset(sldns_buffer* pkt, struct msg_parse* msg,
	struct rrset_parse *pset, struct regional* region, 
	struct ub_packed_rrset_key* pk)
{
	struct packed_rrset_data* data;
	pk->rk.flags = pset->flags;
	pk->rk.dname_len = pset->dname_len;
	if(region)
		pk->rk.dname = (uint8_t*)regional_alloc(
			region, pset->dname_len);
	else	pk->rk.dname = 
			(uint8_t*)malloc(pset->dname_len);
	if(!pk->rk.dname)
		return 0;
	/** copy & decompress dname */
	dname_pkt_copy(pkt, pk->rk.dname, pset->dname);
	/** copy over type and class */
	pk->rk.type = htons(pset->type);
	pk->rk.rrset_class = pset->rrset_class;
	/** read data part. */
	if(!parse_create_rrset(pkt, pset, &data, region))
		return 0;
	pk->entry.data = (void*)data;
	pk->entry.key = (void*)pk;
	pk->entry.hash = pset->hash;
	data->trust = get_rrset_trust(msg, pset);
	return 1;
}

/** 
 * Copy and decompress rrs
 * @param pkt: the packet for compression pointer resolution.
 * @param msg: the parsed message
 * @param rep: reply info to put rrs into.
 * @param region: if not NULL, used for allocation.
 * @return 0 on failure.
 */
static int
parse_copy_decompress(sldns_buffer* pkt, struct msg_parse* msg,
	struct reply_info* rep, struct regional* region)
{
	size_t i;
	struct rrset_parse *pset = msg->rrset_first;
	struct packed_rrset_data* data;
	log_assert(rep);
	rep->ttl = MAX_TTL;
	rep->security = sec_status_unchecked;
	if(rep->rrset_count == 0)
		rep->ttl = NORR_TTL;

	for(i=0; i<rep->rrset_count; i++) {
		if(!parse_copy_decompress_rrset(pkt, msg, pset, region,
			rep->rrsets[i]))
			return 0;
		data = (struct packed_rrset_data*)rep->rrsets[i]->entry.data;
		if(data->ttl < rep->ttl)
			rep->ttl = data->ttl;

		pset = pset->rrset_all_next;
	}
	rep->prefetch_ttl = PREFETCH_TTL_CALC(rep->ttl);
	return 1;
}

int 
parse_create_msg(sldns_buffer* pkt, struct msg_parse* msg,
	struct alloc_cache* alloc, struct query_info* qinf, 
	struct reply_info** rep, struct regional* region)
{
	log_assert(pkt && msg);
	if(!parse_create_qinfo(pkt, msg, qinf, region))
		return 0;
	if(!parse_create_repinfo(msg, rep, region))
		return 0;
	if(!repinfo_alloc_rrset_keys(*rep, alloc, region))
		return 0;
	if(!parse_copy_decompress(pkt, msg, *rep, region))
		return 0;
	return 1;
}

int reply_info_parse(sldns_buffer* pkt, struct alloc_cache* alloc,
        struct query_info* qinf, struct reply_info** rep, 
	struct regional* region, struct edns_data* edns)
{
	/* use scratch pad region-allocator during parsing. */
	struct msg_parse* msg;
	int ret;
	
	qinf->qname = NULL;
	*rep = NULL;
	if(!(msg = regional_alloc(region, sizeof(*msg)))) {
		return LDNS_RCODE_SERVFAIL;
	}
	memset(msg, 0, sizeof(*msg));
	
	sldns_buffer_set_position(pkt, 0);
	if((ret = parse_packet(pkt, msg, region)) != 0) {
		return ret;
	}
	if((ret = parse_extract_edns(msg, edns)) != 0)
		return ret;

	/* parse OK, allocate return structures */
	/* this also performs dname decompression */
	if(!parse_create_msg(pkt, msg, alloc, qinf, rep, NULL)) {
		query_info_clear(qinf);
		reply_info_parsedelete(*rep, alloc);
		*rep = NULL;
		return LDNS_RCODE_SERVFAIL;
	}
	return 0;
}

/** helper compare function to sort in lock order */
static int
reply_info_sortref_cmp(const void* a, const void* b)
{
	struct rrset_ref* x = (struct rrset_ref*)a;
	struct rrset_ref* y = (struct rrset_ref*)b;
	if(x->key < y->key) return -1;
	if(x->key > y->key) return 1;
	return 0;
}

void 
reply_info_sortref(struct reply_info* rep)
{
	qsort(&rep->ref[0], rep->rrset_count, sizeof(struct rrset_ref),
		reply_info_sortref_cmp);
}

void 
reply_info_set_ttls(struct reply_info* rep, time_t timenow)
{
	size_t i, j;
	rep->ttl += timenow;
	rep->prefetch_ttl += timenow;
	for(i=0; i<rep->rrset_count; i++) {
		struct packed_rrset_data* data = (struct packed_rrset_data*)
			rep->ref[i].key->entry.data;
		if(i>0 && rep->ref[i].key == rep->ref[i-1].key)
			continue;
		data->ttl += timenow;
		for(j=0; j<data->count + data->rrsig_count; j++) {
			data->rr_ttl[j] += timenow;
		}
	}
}

void 
reply_info_parsedelete(struct reply_info* rep, struct alloc_cache* alloc)
{
	size_t i;
	if(!rep) 
		return;
	/* no need to lock, since not shared in hashtables. */
	for(i=0; i<rep->rrset_count; i++) {
		ub_packed_rrset_parsedelete(rep->rrsets[i], alloc);
	}
	free(rep);
}

int 
query_info_parse(struct query_info* m, sldns_buffer* query)
{
	uint8_t* q = sldns_buffer_begin(query);
	/* minimum size: header + \0 + qtype + qclass */
	if(sldns_buffer_limit(query) < LDNS_HEADER_SIZE + 5)
		return 0;
	if(LDNS_OPCODE_WIRE(q) != LDNS_PACKET_QUERY || 
		LDNS_QDCOUNT(q) != 1 || sldns_buffer_position(query) != 0)
		return 0;
	sldns_buffer_skip(query, LDNS_HEADER_SIZE);
	m->qname = sldns_buffer_current(query);
	if((m->qname_len = query_dname_len(query)) == 0)
		return 0; /* parse error */
	if(sldns_buffer_remaining(query) < 4)
		return 0; /* need qtype, qclass */
	m->qtype = sldns_buffer_read_u16(query);
	m->qclass = sldns_buffer_read_u16(query);
	return 1;
}

/** tiny subroutine for msgreply_compare */
#define COMPARE_IT(x, y) \
	if( (x) < (y) ) return -1; \
	else if( (x) > (y) ) return +1; \
	log_assert( (x) == (y) );

int 
query_info_compare(void* m1, void* m2)
{
	struct query_info* msg1 = (struct query_info*)m1;
	struct query_info* msg2 = (struct query_info*)m2;
	int mc;
	/* from most different to least different for speed */
	COMPARE_IT(msg1->qtype, msg2->qtype);
	if((mc = query_dname_compare(msg1->qname, msg2->qname)) != 0)
		return mc;
	log_assert(msg1->qname_len == msg2->qname_len);
	COMPARE_IT(msg1->qclass, msg2->qclass);
	return 0;
#undef COMPARE_IT
}

void 
query_info_clear(struct query_info* m)
{
	free(m->qname);
	m->qname = NULL;
}

size_t 
msgreply_sizefunc(void* k, void* d)
{
	struct msgreply_entry* q = (struct msgreply_entry*)k;
	struct reply_info* r = (struct reply_info*)d;
	size_t s = sizeof(struct msgreply_entry) + sizeof(struct reply_info)
		+ q->key.qname_len + lock_get_mem(&q->entry.lock)
		- sizeof(struct rrset_ref);
	s += r->rrset_count * sizeof(struct rrset_ref);
	s += r->rrset_count * sizeof(struct ub_packed_rrset_key*);
	return s;
}

void 
query_entry_delete(void *k, void* ATTR_UNUSED(arg))
{
	struct msgreply_entry* q = (struct msgreply_entry*)k;
	lock_rw_destroy(&q->entry.lock);
	query_info_clear(&q->key);
	free(q);
}

void 
reply_info_delete(void* d, void* ATTR_UNUSED(arg))
{
	struct reply_info* r = (struct reply_info*)d;
	free(r);
}

hashvalue_t 
query_info_hash(struct query_info *q, uint16_t flags)
{
	hashvalue_t h = 0xab;
	h = hashlittle(&q->qtype, sizeof(q->qtype), h);
	if(q->qtype == LDNS_RR_TYPE_AAAA && (flags&BIT_CD))
		h++;
	h = hashlittle(&q->qclass, sizeof(q->qclass), h);
	h = dname_query_hash(q->qname, h);
	return h;
}

struct msgreply_entry* 
query_info_entrysetup(struct query_info* q, struct reply_info* r, 
	hashvalue_t h)
{
	struct msgreply_entry* e = (struct msgreply_entry*)malloc( 
		sizeof(struct msgreply_entry));
	if(!e) return NULL;
	memcpy(&e->key, q, sizeof(*q));
	e->entry.hash = h;
	e->entry.key = e;
	e->entry.data = r;
	lock_rw_init(&e->entry.lock);
	lock_protect(&e->entry.lock, &e->key, sizeof(e->key));
	lock_protect(&e->entry.lock, &e->entry.hash, sizeof(e->entry.hash) +
		sizeof(e->entry.key) + sizeof(e->entry.data));
	lock_protect(&e->entry.lock, e->key.qname, e->key.qname_len);
	q->qname = NULL;
	return e;
}

/** copy rrsets from replyinfo to dest replyinfo */
static int
repinfo_copy_rrsets(struct reply_info* dest, struct reply_info* from, 
	struct regional* region)
{
	size_t i, s;
	struct packed_rrset_data* fd, *dd;
	struct ub_packed_rrset_key* fk, *dk;
	for(i=0; i<dest->rrset_count; i++) {
		fk = from->rrsets[i];
		dk = dest->rrsets[i];
		fd = (struct packed_rrset_data*)fk->entry.data;
		dk->entry.hash = fk->entry.hash;
		dk->rk = fk->rk;
		if(region) {
			dk->id = fk->id;
			dk->rk.dname = (uint8_t*)regional_alloc_init(region,
				fk->rk.dname, fk->rk.dname_len);
		} else	
			dk->rk.dname = (uint8_t*)memdup(fk->rk.dname, 
				fk->rk.dname_len);
		if(!dk->rk.dname)
			return 0;
		s = packed_rrset_sizeof(fd);
		if(region)
			dd = (struct packed_rrset_data*)regional_alloc_init(
				region, fd, s);
		else	dd = (struct packed_rrset_data*)memdup(fd, s);
		if(!dd) 
			return 0;
		packed_rrset_ptr_fixup(dd);
		dk->entry.data = (void*)dd;
	}
	return 1;
}

struct reply_info* 
reply_info_copy(struct reply_info* rep, struct alloc_cache* alloc, 
	struct regional* region)
{
	struct reply_info* cp;
	cp = construct_reply_info_base(region, rep->flags, rep->qdcount, 
		rep->ttl, rep->prefetch_ttl, rep->an_numrrsets, 
		rep->ns_numrrsets, rep->ar_numrrsets, rep->rrset_count, 
		rep->security);
	if(!cp)
		return NULL;
	/* allocate ub_key structures special or not */
	if(!repinfo_alloc_rrset_keys(cp, alloc, region)) {
		if(!region)
			reply_info_parsedelete(cp, alloc);
		return NULL;
	}
	if(!repinfo_copy_rrsets(cp, rep, region)) {
		if(!region)
			reply_info_parsedelete(cp, alloc);
		return NULL;
	}
	return cp;
}

uint8_t* 
reply_find_final_cname_target(struct query_info* qinfo, struct reply_info* rep)
{
	uint8_t* sname = qinfo->qname;
	size_t snamelen = qinfo->qname_len;
	size_t i;
	for(i=0; i<rep->an_numrrsets; i++) {
		struct ub_packed_rrset_key* s = rep->rrsets[i];
		/* follow CNAME chain (if any) */
		if(ntohs(s->rk.type) == LDNS_RR_TYPE_CNAME && 
			ntohs(s->rk.rrset_class) == qinfo->qclass && 
			snamelen == s->rk.dname_len &&
			query_dname_compare(sname, s->rk.dname) == 0) {
			get_cname_target(s, &sname, &snamelen);
		}
	}
	if(sname != qinfo->qname)
		return sname;
	return NULL;
}

struct ub_packed_rrset_key* 
reply_find_answer_rrset(struct query_info* qinfo, struct reply_info* rep)
{
	uint8_t* sname = qinfo->qname;
	size_t snamelen = qinfo->qname_len;
	size_t i;
	for(i=0; i<rep->an_numrrsets; i++) {
		struct ub_packed_rrset_key* s = rep->rrsets[i];
		/* first match type, for query of qtype cname */
		if(ntohs(s->rk.type) == qinfo->qtype && 
			ntohs(s->rk.rrset_class) == qinfo->qclass && 
			snamelen == s->rk.dname_len &&
			query_dname_compare(sname, s->rk.dname) == 0) {
			return s;
		}
		/* follow CNAME chain (if any) */
		if(ntohs(s->rk.type) == LDNS_RR_TYPE_CNAME && 
			ntohs(s->rk.rrset_class) == qinfo->qclass && 
			snamelen == s->rk.dname_len &&
			query_dname_compare(sname, s->rk.dname) == 0) {
			get_cname_target(s, &sname, &snamelen);
		}
	}
	return NULL;
}

struct ub_packed_rrset_key* reply_find_rrset_section_an(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass)
{
	size_t i;
	for(i=0; i<rep->an_numrrsets; i++) {
		struct ub_packed_rrset_key* s = rep->rrsets[i];
		if(ntohs(s->rk.type) == type && 
			ntohs(s->rk.rrset_class) == dclass && 
			namelen == s->rk.dname_len &&
			query_dname_compare(name, s->rk.dname) == 0) {
			return s;
		}
	}
	return NULL;
}

struct ub_packed_rrset_key* reply_find_rrset_section_ns(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass)
{
	size_t i;
	for(i=rep->an_numrrsets; i<rep->an_numrrsets+rep->ns_numrrsets; i++) {
		struct ub_packed_rrset_key* s = rep->rrsets[i];
		if(ntohs(s->rk.type) == type && 
			ntohs(s->rk.rrset_class) == dclass && 
			namelen == s->rk.dname_len &&
			query_dname_compare(name, s->rk.dname) == 0) {
			return s;
		}
	}
	return NULL;
}

struct ub_packed_rrset_key* reply_find_rrset(struct reply_info* rep,
	uint8_t* name, size_t namelen, uint16_t type, uint16_t dclass)
{
	size_t i;
	for(i=0; i<rep->rrset_count; i++) {
		struct ub_packed_rrset_key* s = rep->rrsets[i];
		if(ntohs(s->rk.type) == type && 
			ntohs(s->rk.rrset_class) == dclass && 
			namelen == s->rk.dname_len &&
			query_dname_compare(name, s->rk.dname) == 0) {
			return s;
		}
	}
	return NULL;
}

void 
log_dns_msg(const char* str, struct query_info* qinfo, struct reply_info* rep)
{
	/* not particularly fast but flexible, make wireformat and print */
	sldns_buffer* buf = sldns_buffer_new(65535);
	struct regional* region = regional_create();
	if(!reply_info_encode(qinfo, rep, 0, rep->flags, buf, 0, 
		region, 65535, 1)) {
		log_info("%s: log_dns_msg: out of memory", str);
	} else {
		char* s = sldns_wire2str_pkt(sldns_buffer_begin(buf),
			sldns_buffer_limit(buf));
		if(!s) {
			log_info("%s: log_dns_msg: ldns tostr failed", str);
		} else {
			log_info("%s %s", str, s);
		}
		free(s);
	}
	sldns_buffer_free(buf);
	regional_destroy(region);
}

void 
log_query_info(enum verbosity_value v, const char* str, 
	struct query_info* qinf)
{
	log_nametypeclass(v, str, qinf->qname, qinf->qtype, qinf->qclass);
}

int
reply_check_cname_chain(struct reply_info* rep) 
{
	/* check only answer section rrs for matching cname chain.
	 * the cache may return changed rdata, but owner names are untouched.*/
	size_t i;
	uint8_t* sname = rep->rrsets[0]->rk.dname;
	size_t snamelen = rep->rrsets[0]->rk.dname_len;
	for(i=0; i<rep->an_numrrsets; i++) {
		uint16_t t = ntohs(rep->rrsets[i]->rk.type);
		if(t == LDNS_RR_TYPE_DNAME)
			continue; /* skip dnames; note TTL 0 not cached */
		/* verify that owner matches current sname */
		if(query_dname_compare(sname, rep->rrsets[i]->rk.dname) != 0){
			/* cname chain broken */
			return 0;
		}
		/* if this is a cname; move on */
		if(t == LDNS_RR_TYPE_CNAME) {
			get_cname_target(rep->rrsets[i], &sname, &snamelen);
		}
	}
	return 1;
}

int
reply_all_rrsets_secure(struct reply_info* rep) 
{
	size_t i;
	for(i=0; i<rep->rrset_count; i++) {
		if( ((struct packed_rrset_data*)rep->rrsets[i]->entry.data)
			->security != sec_status_secure )
		return 0;
	}
	return 1;
}
