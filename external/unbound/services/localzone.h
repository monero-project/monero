/*
 * services/localzone.h - local zones authority service.
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
 * This file contains functions to enable local zone authority service.
 */

#ifndef SERVICES_LOCALZONE_H
#define SERVICES_LOCALZONE_H
#include "util/rbtree.h"
#include "util/locks.h"
struct ub_packed_rrset_key;
struct regional;
struct config_file;
struct edns_data;
struct query_info;
struct sldns_buffer;
struct comm_reply;

/**
 * Local zone type
 * This type determines processing for queries that did not match
 * local-data directly.
 */
enum localzone_type {
	/** drop query */
	local_zone_deny = 0,
	/** answer with error */
	local_zone_refuse,
	/** answer nxdomain or nodata */
	local_zone_static,
	/** resolve normally */
	local_zone_transparent,
	/** do not block types at localdata names */
	local_zone_typetransparent,
	/** answer with data at zone apex */
	local_zone_redirect,
	/** remove default AS112 blocking contents for zone
	 * nodefault is used in config not during service. */
	local_zone_nodefault,
	/** log client address, but no block (transparent) */
	local_zone_inform,
	/** log client address, and block (drop) */
	local_zone_inform_deny
};

/**
 * Authoritative local zones storage, shared.
 */
struct local_zones {
	/** lock on the localzone tree */
	lock_rw_t lock;
	/** rbtree of struct local_zone */
	rbtree_t ztree;
};

/**
 * Local zone. A locally served authoritative zone.
 */
struct local_zone {
	/** rbtree node, key is name and class */
	rbnode_t node;
	/** parent zone, if any. */
	struct local_zone* parent;

	/** zone name, in uncompressed wireformat */
	uint8_t* name;
	/** length of zone name */
	size_t namelen;
	/** number of labels in zone name */
	int namelabs;
	/** the class of this zone. 
	 * uses 'dclass' to not conflict with c++ keyword class. */
	uint16_t dclass;

	/** lock on the data in the structure
	 * For the node, parent, name, namelen, namelabs, dclass, you
	 * need to also hold the zones_tree lock to change them (or to
	 * delete this zone) */
	lock_rw_t lock;

	/** how to process zone */
	enum localzone_type type;

	/** in this region the zone's data is allocated.
	 * the struct local_zone itself is malloced. */
	struct regional* region;
	/** local data for this zone
	 * rbtree of struct local_data */
	rbtree_t data;
	/** if data contains zone apex SOA data, this is a ptr to it. */
	struct ub_packed_rrset_key* soa;
};

/**
 * Local data. One domain name, and the RRs to go with it.
 */
struct local_data {
	/** rbtree node, key is name only */
	rbnode_t node;
	/** domain name */
	uint8_t* name;
	/** length of name */
	size_t namelen;
	/** number of labels in name */
	int namelabs;
	/** the data rrsets, with different types, linked list.
	 * If this list is NULL, the node is an empty non-terminal. */
	struct local_rrset* rrsets;
};

/**
 * A local data RRset
 */
struct local_rrset {
	/** next in list */
	struct local_rrset* next;
	/** RRset data item */
	struct ub_packed_rrset_key* rrset;
};

/**
 * Create local zones storage
 * @return new struct or NULL on error.
 */
struct local_zones* local_zones_create(void);

/**
 * Delete local zones storage
 * @param zones: to delete.
 */
void local_zones_delete(struct local_zones* zones);

/**
 * Apply config settings; setup the local authoritative data. 
 * Takes care of locking.
 * @param zones: is set up.
 * @param cfg: config data.
 * @return false on error.
 */
int local_zones_apply_cfg(struct local_zones* zones, struct config_file* cfg);

/**
 * Compare two local_zone entries in rbtree. Sort hierarchical but not
 * canonical
 * @param z1: zone 1
 * @param z2: zone 2
 * @return: -1, 0, +1 comparison value.
 */
int local_zone_cmp(const void* z1, const void* z2);

/**
 * Compare two local_data entries in rbtree. Sort canonical.
 * @param d1: data 1
 * @param d2: data 2
 * @return: -1, 0, +1 comparison value.
 */
int local_data_cmp(const void* d1, const void* d2);

/**
 * Delete one zone
 * @param z: to delete.
 */
void local_zone_delete(struct local_zone* z);

/**
 * Lookup zone that contains the given name, class.
 * User must lock the tree or result zone.
 * @param zones: the zones tree
 * @param name: dname to lookup
 * @param len: length of name.
 * @param labs: labelcount of name.
 * @param dclass: class to lookup.
 * @return closest local_zone or NULL if no covering zone is found.
 */
struct local_zone* local_zones_lookup(struct local_zones* zones, 
	uint8_t* name, size_t len, int labs, uint16_t dclass);

/**
 * Debug helper. Print all zones 
 * Takes care of locking.
 * @param zones: the zones tree
 */
void local_zones_print(struct local_zones* zones);

/**
 * Answer authoritatively for local zones.
 * Takes care of locking.
 * @param zones: the stored zones (shared, read only).
 * @param qinfo: query info (parsed).
 * @param edns: edns info (parsed).
 * @param buf: buffer with query ID and flags, also for reply.
 * @param temp: temporary storage region.
 * @param repinfo: source address for checks. may be NULL.
 * @return true if answer is in buffer. false if query is not answered 
 * by authority data. If the reply should be dropped altogether, the return 
 * value is true, but the buffer is cleared (empty).
 */
int local_zones_answer(struct local_zones* zones, struct query_info* qinfo,
	struct edns_data* edns, struct sldns_buffer* buf, struct regional* temp,
	struct comm_reply* repinfo);

/**
 * Parse the string into localzone type.
 *
 * @param str: string to parse
 * @param t: local zone type returned here.
 * @return 0 on parse error.
 */
int local_zone_str2type(const char* str, enum localzone_type* t);

/**
 * Print localzone type to a string.  Pointer to a constant string.
 *
 * @param t: local zone type.
 * @return constant string that describes type.
 */
const char* local_zone_type2str(enum localzone_type t);

/**
 * Find zone that with exactly given name, class.
 * User must lock the tree or result zone.
 * @param zones: the zones tree
 * @param name: dname to lookup
 * @param len: length of name.
 * @param labs: labelcount of name.
 * @param dclass: class to lookup.
 * @return the exact local_zone or NULL.
 */
struct local_zone* local_zones_find(struct local_zones* zones, 
	uint8_t* name, size_t len, int labs, uint16_t dclass);

/**
 * Add a new zone. Caller must hold the zones lock.
 * Adjusts the other zones as well (parent pointers) after insertion.
 * The zone must NOT exist (returns NULL and logs error).
 * @param zones: the zones tree
 * @param name: dname to add
 * @param len: length of name.
 * @param labs: labelcount of name.
 * @param dclass: class to add.
 * @param tp: type.
 * @return local_zone or NULL on error, caller must printout memory error.
 */
struct local_zone* local_zones_add_zone(struct local_zones* zones, 
	uint8_t* name, size_t len, int labs, uint16_t dclass, 
	enum localzone_type tp);

/**
 * Delete a zone. Caller must hold the zones lock.
 * Adjusts the other zones as well (parent pointers) after insertion.
 * @param zones: the zones tree
 * @param zone: the zone to delete from tree. Also deletes zone from memory.
 */
void local_zones_del_zone(struct local_zones* zones, struct local_zone* zone);

/**
 * Add RR data into the localzone data.
 * Looks up the zone, if no covering zone, a transparent zone with the
 * name of the RR is created.
 * @param zones: the zones tree. Not locked by caller.
 * @param rr: string with on RR.
 * @return false on failure.
 */
int local_zones_add_RR(struct local_zones* zones, const char* rr);

/**
 * Remove data from domain name in the tree.
 * All types are removed. No effect if zone or name does not exist.
 * @param zones: zones tree.
 * @param name: dname to remove
 * @param len: length of name.
 * @param labs: labelcount of name.
 * @param dclass: class to remove.
 */
void local_zones_del_data(struct local_zones* zones, 
	uint8_t* name, size_t len, int labs, uint16_t dclass);


/** 
 * Form wireformat from text format domain name. 
 * @param str: the domain name in text "www.example.com"
 * @param res: resulting wireformat is stored here with malloc.
 * @param len: length of resulting wireformat.
 * @param labs: number of labels in resulting wireformat.
 * @return false on error, syntax or memory. Also logged.
 */
int parse_dname(const char* str, uint8_t** res, size_t* len, int* labs);

#endif /* SERVICES_LOCALZONE_H */
