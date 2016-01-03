/*
 * services/cache/rrset.c - Resource record set cache.
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
 * This file contains the rrset cache.
 */
#include "config.h"
#include "services/cache/rrset.h"
#include "sldns/rrdef.h"
#include "util/storage/slabhash.h"
#include "util/config_file.h"
#include "util/data/packed_rrset.h"
#include "util/data/msgreply.h"
#include "util/regional.h"
#include "util/alloc.h"

void
rrset_markdel(void* key)
{
	struct ub_packed_rrset_key* r = (struct ub_packed_rrset_key*)key;
	r->id = 0;
}

struct rrset_cache* rrset_cache_create(struct config_file* cfg, 
	struct alloc_cache* alloc)
{
	size_t slabs = (cfg?cfg->rrset_cache_slabs:HASH_DEFAULT_SLABS);
	size_t startarray = HASH_DEFAULT_STARTARRAY;
	size_t maxmem = (cfg?cfg->rrset_cache_size:HASH_DEFAULT_MAXMEM);

	struct rrset_cache *r = (struct rrset_cache*)slabhash_create(slabs,
		startarray, maxmem, ub_rrset_sizefunc, ub_rrset_compare,
		ub_rrset_key_delete, rrset_data_delete, alloc);
	slabhash_setmarkdel(&r->table, &rrset_markdel);
	return r;
}

void rrset_cache_delete(struct rrset_cache* r)
{
	if(!r) 
		return;
	slabhash_delete(&r->table);
	/* slabhash delete also does free(r), since table is first in struct*/
}

struct rrset_cache* rrset_cache_adjust(struct rrset_cache *r, 
	struct config_file* cfg, struct alloc_cache* alloc)
{
	if(!r || !cfg || cfg->rrset_cache_slabs != r->table.size ||
		cfg->rrset_cache_size != slabhash_get_size(&r->table))
	{
		rrset_cache_delete(r);
		r = rrset_cache_create(cfg, alloc);
	}
	return r;
}

void 
rrset_cache_touch(struct rrset_cache* r, struct ub_packed_rrset_key* key,
        hashvalue_t hash, rrset_id_t id)
{
	struct lruhash* table = slabhash_gettable(&r->table, hash);
	/* 
	 * This leads to locking problems, deadlocks, if the caller is 
	 * holding any other rrset lock.
	 * Because a lookup through the hashtable does:
	 *	tablelock -> entrylock  (for that entry caller holds)
	 * And this would do
	 *	entrylock(already held) -> tablelock
	 * And if two threads do this, it results in deadlock.
	 * So, the caller must not hold entrylock.
	 */
	lock_quick_lock(&table->lock);
	/* we have locked the hash table, the item can still be deleted.
	 * because it could already have been reclaimed, but not yet set id=0.
	 * This is because some lruhash routines have lazy deletion.
	 * so, we must acquire a lock on the item to verify the id != 0.
	 * also, with hash not changed, we are using the right slab.
	 */
	lock_rw_rdlock(&key->entry.lock);
	if(key->id == id && key->entry.hash == hash) {
		lru_touch(table, &key->entry);
	}
	lock_rw_unlock(&key->entry.lock);
	lock_quick_unlock(&table->lock);
}

/** see if rrset needs to be updated in the cache */
static int
need_to_update_rrset(void* nd, void* cd, time_t timenow, int equal, int ns)
{
	struct packed_rrset_data* newd = (struct packed_rrset_data*)nd;
	struct packed_rrset_data* cached = (struct packed_rrset_data*)cd;
	/* 	o store if rrset has been validated 
	 *  		everything better than bogus data 
	 *  		secure is preferred */
	if( newd->security == sec_status_secure &&
		cached->security != sec_status_secure)
		return 1;
	if( cached->security == sec_status_bogus && 
		newd->security != sec_status_bogus && !equal)
		return 1;
        /*      o if current RRset is more trustworthy - insert it */
        if( newd->trust > cached->trust ) {
		/* if the cached rrset is bogus, and this one equal,
		 * do not update the TTL - let it expire. */
		if(equal && cached->ttl >= timenow && 
			cached->security == sec_status_bogus)
			return 0;
                return 1;
	}
	/*	o item in cache has expired */
	if( cached->ttl < timenow )
		return 1;
	/*  o same trust, but different in data - insert it */
	if( newd->trust == cached->trust && !equal ) {
		/* if this is type NS, do not 'stick' to owner that changes
		 * the NS RRset, but use the old TTL for the new data, and
		 * update to fetch the latest data. ttl is not expired, because
		 * that check was before this one. */
		if(ns) {
			size_t i;
			newd->ttl = cached->ttl;
			for(i=0; i<(newd->count+newd->rrsig_count); i++)
				if(newd->rr_ttl[i] > newd->ttl)
					newd->rr_ttl[i] = newd->ttl;
		}
		return 1;
	}
	return 0;
}

/** Update RRSet special key ID */
static void
rrset_update_id(struct rrset_ref* ref, struct alloc_cache* alloc)
{
	/* this may clear the cache and invalidate lock below */
	uint64_t newid = alloc_get_id(alloc);
	/* obtain writelock */
	lock_rw_wrlock(&ref->key->entry.lock);
	/* check if it was deleted in the meantime, if so, skip update */
	if(ref->key->id == ref->id) {
		ref->key->id = newid;
		ref->id = newid;
	}
	lock_rw_unlock(&ref->key->entry.lock);
}

int 
rrset_cache_update(struct rrset_cache* r, struct rrset_ref* ref,
	struct alloc_cache* alloc, time_t timenow)
{
	struct lruhash_entry* e;
	struct ub_packed_rrset_key* k = ref->key;
	hashvalue_t h = k->entry.hash;
	uint16_t rrset_type = ntohs(k->rk.type);
	int equal = 0;
	log_assert(ref->id != 0 && k->id != 0);
	log_assert(k->rk.dname != NULL);
	/* looks up item with a readlock - no editing! */
	if((e=slabhash_lookup(&r->table, h, k, 0)) != 0) {
		/* return id and key as they will be used in the cache
		 * since the lruhash_insert, if item already exists, deallocs
		 * the passed key in favor of the already stored key.
		 * because of the small gap (see below) this key ptr and id
		 * may prove later to be already deleted, which is no problem
		 * as it only makes a cache miss. 
		 */
		ref->key = (struct ub_packed_rrset_key*)e->key;
		ref->id = ref->key->id;
		equal = rrsetdata_equal((struct packed_rrset_data*)k->entry.
			data, (struct packed_rrset_data*)e->data);
		if(!need_to_update_rrset(k->entry.data, e->data, timenow,
			equal, (rrset_type==LDNS_RR_TYPE_NS))) {
			/* cache is superior, return that value */
			lock_rw_unlock(&e->lock);
			ub_packed_rrset_parsedelete(k, alloc);
			if(equal) return 2;
			return 1;
		}
		lock_rw_unlock(&e->lock);
		/* Go on and insert the passed item.
		 * small gap here, where entry is not locked.
		 * possibly entry is updated with something else.
		 * we then overwrite that with our data.
		 * this is just too bad, its cache anyway. */
		/* use insert to update entry to manage lruhash
		 * cache size values nicely. */
	}
	log_assert(ref->key->id != 0);
	slabhash_insert(&r->table, h, &k->entry, k->entry.data, alloc);
	if(e) {
		/* For NSEC, NSEC3, DNAME, when rdata is updated, update 
		 * the ID number so that proofs in message cache are 
		 * invalidated */
		if((rrset_type == LDNS_RR_TYPE_NSEC 
			|| rrset_type == LDNS_RR_TYPE_NSEC3
			|| rrset_type == LDNS_RR_TYPE_DNAME) && !equal) {
			rrset_update_id(ref, alloc);
		}
		return 1;
	}
	return 0;
}

struct ub_packed_rrset_key* 
rrset_cache_lookup(struct rrset_cache* r, uint8_t* qname, size_t qnamelen, 
	uint16_t qtype, uint16_t qclass, uint32_t flags, time_t timenow,
	int wr)
{
	struct lruhash_entry* e;
	struct ub_packed_rrset_key key;
	
	key.entry.key = &key;
	key.entry.data = NULL;
	key.rk.dname = qname;
	key.rk.dname_len = qnamelen;
	key.rk.type = htons(qtype);
	key.rk.rrset_class = htons(qclass);
	key.rk.flags = flags;

	key.entry.hash = rrset_key_hash(&key.rk);

	if((e = slabhash_lookup(&r->table, key.entry.hash, &key, wr))) {
		/* check TTL */
		struct packed_rrset_data* data = 
			(struct packed_rrset_data*)e->data;
		if(timenow > data->ttl) {
			lock_rw_unlock(&e->lock);
			return NULL;
		}
		/* we're done */
		return (struct ub_packed_rrset_key*)e->key;
	}
	return NULL;
}

int 
rrset_array_lock(struct rrset_ref* ref, size_t count, time_t timenow)
{
	size_t i;
	for(i=0; i<count; i++) {
		if(i>0 && ref[i].key == ref[i-1].key)
			continue; /* only lock items once */
		lock_rw_rdlock(&ref[i].key->entry.lock);
		if(ref[i].id != ref[i].key->id || timenow >
			((struct packed_rrset_data*)(ref[i].key->entry.data))
			->ttl) {
			/* failure! rollback our readlocks */
			rrset_array_unlock(ref, i+1);
			return 0;
		}
	}
	return 1;
}

void 
rrset_array_unlock(struct rrset_ref* ref, size_t count)
{
	size_t i;
	for(i=0; i<count; i++) {
		if(i>0 && ref[i].key == ref[i-1].key)
			continue; /* only unlock items once */
		lock_rw_unlock(&ref[i].key->entry.lock);
	}
}

void 
rrset_array_unlock_touch(struct rrset_cache* r, struct regional* scratch,
	struct rrset_ref* ref, size_t count)
{
	hashvalue_t* h;
	size_t i;
	if(count > RR_COUNT_MAX || !(h = (hashvalue_t*)regional_alloc(scratch, 
		sizeof(hashvalue_t)*count))) {
		log_warn("rrset LRU: memory allocation failed");
		h = NULL;
	} else 	/* store hash values */
		for(i=0; i<count; i++)
			h[i] = ref[i].key->entry.hash;
	/* unlock */
	for(i=0; i<count; i++) {
		if(i>0 && ref[i].key == ref[i-1].key)
			continue; /* only unlock items once */
		lock_rw_unlock(&ref[i].key->entry.lock);
	}
	if(h) {
		/* LRU touch, with no rrset locks held */
		for(i=0; i<count; i++) {
			if(i>0 && ref[i].key == ref[i-1].key)
				continue; /* only touch items once */
			rrset_cache_touch(r, ref[i].key, h[i], ref[i].id);
		}
	}
}

void 
rrset_update_sec_status(struct rrset_cache* r, 
	struct ub_packed_rrset_key* rrset, time_t now)
{
	struct packed_rrset_data* updata = 
		(struct packed_rrset_data*)rrset->entry.data;
	struct lruhash_entry* e;
	struct packed_rrset_data* cachedata;

	/* hash it again to make sure it has a hash */
	rrset->entry.hash = rrset_key_hash(&rrset->rk);

	e = slabhash_lookup(&r->table, rrset->entry.hash, rrset, 1);
	if(!e)
		return; /* not in the cache anymore */
	cachedata = (struct packed_rrset_data*)e->data;
	if(!rrsetdata_equal(updata, cachedata)) {
		lock_rw_unlock(&e->lock);
		return; /* rrset has changed in the meantime */
	}
	/* update the cached rrset */
	if(updata->security > cachedata->security) {
		size_t i;
		if(updata->trust > cachedata->trust)
			cachedata->trust = updata->trust;
		cachedata->security = updata->security;
		/* for NS records only shorter TTLs, other types: update it */
		if(ntohs(rrset->rk.type) != LDNS_RR_TYPE_NS ||
			updata->ttl+now < cachedata->ttl ||
			cachedata->ttl < now ||
			updata->security == sec_status_bogus) {
			cachedata->ttl = updata->ttl + now;
			for(i=0; i<cachedata->count+cachedata->rrsig_count; i++)
				cachedata->rr_ttl[i] = updata->rr_ttl[i]+now;
		}
	}
	lock_rw_unlock(&e->lock);
}

void 
rrset_check_sec_status(struct rrset_cache* r, 
	struct ub_packed_rrset_key* rrset, time_t now)
{
	struct packed_rrset_data* updata = 
		(struct packed_rrset_data*)rrset->entry.data;
	struct lruhash_entry* e;
	struct packed_rrset_data* cachedata;

	/* hash it again to make sure it has a hash */
	rrset->entry.hash = rrset_key_hash(&rrset->rk);

	e = slabhash_lookup(&r->table, rrset->entry.hash, rrset, 0);
	if(!e)
		return; /* not in the cache anymore */
	cachedata = (struct packed_rrset_data*)e->data;
	if(now > cachedata->ttl || !rrsetdata_equal(updata, cachedata)) {
		lock_rw_unlock(&e->lock);
		return; /* expired, or rrset has changed in the meantime */
	}
	if(cachedata->security > updata->security) {
		updata->security = cachedata->security;
		if(cachedata->security == sec_status_bogus) {
			size_t i;
			updata->ttl = cachedata->ttl - now;
			for(i=0; i<cachedata->count+cachedata->rrsig_count; i++)
				if(cachedata->rr_ttl[i] < now)
					updata->rr_ttl[i] = 0;
				else updata->rr_ttl[i] = 
					cachedata->rr_ttl[i]-now;
		}
		if(cachedata->trust > updata->trust)
			updata->trust = cachedata->trust;
	}
	lock_rw_unlock(&e->lock);
}

void rrset_cache_remove(struct rrset_cache* r, uint8_t* nm, size_t nmlen,
	uint16_t type, uint16_t dclass, uint32_t flags)
{
	struct ub_packed_rrset_key key;
	key.entry.key = &key;
	key.rk.dname = nm;
	key.rk.dname_len = nmlen;
	key.rk.rrset_class = htons(dclass);
	key.rk.type = htons(type);
	key.rk.flags = flags;
	key.entry.hash = rrset_key_hash(&key.rk);
	slabhash_remove(&r->table, key.entry.hash, &key);
}
