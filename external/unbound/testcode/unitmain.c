/*
 * testcode/unitmain.c - unit test main program for unbound.
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
 *
 */
/**
 * \file
 * Unit test main program. Calls all the other unit tests.
 * Exits with code 1 on a failure. 0 if all unit tests are successful.
 */

#include "config.h"
#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif

#ifdef HAVE_OPENSSL_RAND_H
#include <openssl/rand.h>
#endif

#ifdef HAVE_OPENSSL_CONF_H
#include <openssl/conf.h>
#endif

#ifdef HAVE_OPENSSL_ENGINE_H
#include <openssl/engine.h>
#endif

#ifdef HAVE_NSS
/* nss3 */
#include "nss.h"
#endif

#include "sldns/rrdef.h"
#include "sldns/keyraw.h"
#include "util/log.h"
#include "testcode/unitmain.h"

/** number of tests done */
int testcount = 0;

#include "util/alloc.h"
/** test alloc code */
static void
alloc_test(void) {
	alloc_special_t *t1, *t2;
	struct alloc_cache major, minor1, minor2;
	int i;

	unit_show_feature("alloc_special_obtain");
	alloc_init(&major, NULL, 0);
	alloc_init(&minor1, &major, 0);
	alloc_init(&minor2, &major, 1);

	t1 = alloc_special_obtain(&minor1);
	alloc_clear(&minor1);

	alloc_special_release(&minor2, t1);
	t2 = alloc_special_obtain(&minor2);
	unit_assert( t1 == t2 ); /* reused */
	alloc_special_release(&minor2, t1);

	for(i=0; i<100; i++) {
		t1 = alloc_special_obtain(&minor1);
		alloc_special_release(&minor2, t1);
	}
	if(0) {
		alloc_stats(&minor1);
		alloc_stats(&minor2);
		alloc_stats(&major);
	}
	/* reuse happened */
	unit_assert(minor1.num_quar + minor2.num_quar + major.num_quar == 11);

	alloc_clear(&minor1);
	alloc_clear(&minor2);
	unit_assert(major.num_quar == 11);
	alloc_clear(&major);
}

#include "util/net_help.h"
/** test net code */
static void 
net_test(void)
{
	const char* t4[] = {"\000\000\000\000",
		"\200\000\000\000",
		"\300\000\000\000",
		"\340\000\000\000",
		"\360\000\000\000",
		"\370\000\000\000",
		"\374\000\000\000",
		"\376\000\000\000",
		"\377\000\000\000",
		"\377\200\000\000",
		"\377\300\000\000",
		"\377\340\000\000",
		"\377\360\000\000",
		"\377\370\000\000",
		"\377\374\000\000",
		"\377\376\000\000",
		"\377\377\000\000",
		"\377\377\200\000",
		"\377\377\300\000",
		"\377\377\340\000",
		"\377\377\360\000",
		"\377\377\370\000",
		"\377\377\374\000",
		"\377\377\376\000",
		"\377\377\377\000",
		"\377\377\377\200",
		"\377\377\377\300",
		"\377\377\377\340",
		"\377\377\377\360",
		"\377\377\377\370",
		"\377\377\377\374",
		"\377\377\377\376",
		"\377\377\377\377",
		"\377\377\377\377",
		"\377\377\377\377",
	};
	unit_show_func("util/net_help.c", "str_is_ip6");
	unit_assert( str_is_ip6("::") );
	unit_assert( str_is_ip6("::1") );
	unit_assert( str_is_ip6("2001:7b8:206:1:240:f4ff:fe37:8810") );
	unit_assert( str_is_ip6("fe80::240:f4ff:fe37:8810") );
	unit_assert( !str_is_ip6("0.0.0.0") );
	unit_assert( !str_is_ip6("213.154.224.12") );
	unit_assert( !str_is_ip6("213.154.224.255") );
	unit_assert( !str_is_ip6("255.255.255.0") );
	unit_show_func("util/net_help.c", "is_pow2");
	unit_assert( is_pow2(0) );
	unit_assert( is_pow2(1) );
	unit_assert( is_pow2(2) );
	unit_assert( is_pow2(4) );
	unit_assert( is_pow2(8) );
	unit_assert( is_pow2(16) );
	unit_assert( is_pow2(1024) );
	unit_assert( is_pow2(1024*1024) );
	unit_assert( is_pow2(1024*1024*1024) );
	unit_assert( !is_pow2(3) );
	unit_assert( !is_pow2(5) );
	unit_assert( !is_pow2(6) );
	unit_assert( !is_pow2(7) );
	unit_assert( !is_pow2(9) );
	unit_assert( !is_pow2(10) );
	unit_assert( !is_pow2(11) );
	unit_assert( !is_pow2(17) );
	unit_assert( !is_pow2(23) );
	unit_assert( !is_pow2(257) );
	unit_assert( !is_pow2(259) );

	/* test addr_mask */
	unit_show_func("util/net_help.c", "addr_mask");
	if(1) {
		struct sockaddr_in a4;
		struct sockaddr_in6 a6;
		socklen_t l4 = (socklen_t)sizeof(a4);
		socklen_t l6 = (socklen_t)sizeof(a6);
		int i;
		a4.sin_family = AF_INET;
		a6.sin6_family = AF_INET6;
		for(i=0; i<35; i++) {
			/* address 255.255.255.255 */
			memcpy(&a4.sin_addr, "\377\377\377\377", 4);
			addr_mask((struct sockaddr_storage*)&a4, l4, i);
			unit_assert(memcmp(&a4.sin_addr, t4[i], 4) == 0);
		}
		memcpy(&a6.sin6_addr, "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", 16);
		addr_mask((struct sockaddr_storage*)&a6, l6, 128);
		unit_assert(memcmp(&a6.sin6_addr, "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", 16) == 0);
		addr_mask((struct sockaddr_storage*)&a6, l6, 122);
		unit_assert(memcmp(&a6.sin6_addr, "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\300", 16) == 0);
		addr_mask((struct sockaddr_storage*)&a6, l6, 120);
		unit_assert(memcmp(&a6.sin6_addr, "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000", 16) == 0);
		addr_mask((struct sockaddr_storage*)&a6, l6, 64);
		unit_assert(memcmp(&a6.sin6_addr, "\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000", 16) == 0);
		addr_mask((struct sockaddr_storage*)&a6, l6, 0);
		unit_assert(memcmp(&a6.sin6_addr, "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000", 16) == 0);
	}

	/* test addr_in_common */
	unit_show_func("util/net_help.c", "addr_in_common");
	if(1) {
		struct sockaddr_in a4, b4;
		struct sockaddr_in6 a6, b6;
		socklen_t l4 = (socklen_t)sizeof(a4);
		socklen_t l6 = (socklen_t)sizeof(a6);
		int i;
		a4.sin_family = AF_INET;
		b4.sin_family = AF_INET;
		a6.sin6_family = AF_INET6;
		b6.sin6_family = AF_INET6;
		memcpy(&a4.sin_addr, "abcd", 4);
		memcpy(&b4.sin_addr, "abcd", 4);
		unit_assert(addr_in_common((struct sockaddr_storage*)&a4, 32,
			(struct sockaddr_storage*)&b4, 32, l4) == 32);
		unit_assert(addr_in_common((struct sockaddr_storage*)&a4, 34,
			(struct sockaddr_storage*)&b4, 32, l4) == 32);
		for(i=0; i<=32; i++) {
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a4, 32,
				(struct sockaddr_storage*)&b4, i, l4) == i);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a4, i,
				(struct sockaddr_storage*)&b4, 32, l4) == i);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a4, i,
				(struct sockaddr_storage*)&b4, i, l4) == i);
		}
		for(i=0; i<=32; i++) {
			memcpy(&a4.sin_addr, "\377\377\377\377", 4);
			memcpy(&b4.sin_addr, t4[i], 4);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a4, 32,
				(struct sockaddr_storage*)&b4, 32, l4) == i);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&b4, 32,
				(struct sockaddr_storage*)&a4, 32, l4) == i);
		}
		memcpy(&a6.sin6_addr, "abcdefghabcdefgh", 16);
		memcpy(&b6.sin6_addr, "abcdefghabcdefgh", 16);
		unit_assert(addr_in_common((struct sockaddr_storage*)&a6, 128,
			(struct sockaddr_storage*)&b6, 128, l6) == 128);
		unit_assert(addr_in_common((struct sockaddr_storage*)&a6, 129,
			(struct sockaddr_storage*)&b6, 128, l6) == 128);
		for(i=0; i<=128; i++) {
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a6, 128,
				(struct sockaddr_storage*)&b6, i, l6) == i);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a6, i,
				(struct sockaddr_storage*)&b6, 128, l6) == i);
			unit_assert(addr_in_common(
				(struct sockaddr_storage*)&a6, i,
				(struct sockaddr_storage*)&b6, i, l6) == i);
		}
	}
	/* test sockaddr_cmp_addr */
	unit_show_func("util/net_help.c", "sockaddr_cmp_addr");
	if(1) {
		struct sockaddr_storage a, b;
		socklen_t alen = (socklen_t)sizeof(a);
		socklen_t blen = (socklen_t)sizeof(b);
		unit_assert(ipstrtoaddr("127.0.0.0", 53, &a, &alen));
		unit_assert(ipstrtoaddr("127.255.255.255", 53, &b, &blen));
		unit_assert(sockaddr_cmp_addr(&a, alen, &b, blen) < 0);
		unit_assert(sockaddr_cmp_addr(&b, blen, &a, alen) > 0);
		unit_assert(sockaddr_cmp_addr(&a, alen, &a, alen) == 0);
		unit_assert(sockaddr_cmp_addr(&b, blen, &b, blen) == 0);
		unit_assert(ipstrtoaddr("192.168.121.5", 53, &a, &alen));
		unit_assert(sockaddr_cmp_addr(&a, alen, &b, blen) > 0);
		unit_assert(sockaddr_cmp_addr(&b, blen, &a, alen) < 0);
		unit_assert(sockaddr_cmp_addr(&a, alen, &a, alen) == 0);
		unit_assert(ipstrtoaddr("2001:3578:ffeb::99", 53, &b, &blen));
		unit_assert(sockaddr_cmp_addr(&b, blen, &b, blen) == 0);
		unit_assert(sockaddr_cmp_addr(&a, alen, &b, blen) < 0);
		unit_assert(sockaddr_cmp_addr(&b, blen, &a, alen) > 0);
	}
	/* test addr_is_ip4mapped */
	unit_show_func("util/net_help.c", "addr_is_ip4mapped");
	if(1) {
		struct sockaddr_storage a;
		socklen_t l = (socklen_t)sizeof(a);
		unit_assert(ipstrtoaddr("12.13.14.15", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("fe80::217:31ff:fe91:df", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("ffff::217:31ff:fe91:df", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("::ffff:31ff:fe91:df", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("::fffe:fe91:df", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("::ffff:127.0.0.1", 53, &a, &l));
		unit_assert(addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("::ffff:127.0.0.2", 53, &a, &l));
		unit_assert(addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("::ffff:192.168.0.2", 53, &a, &l));
		unit_assert(addr_is_ip4mapped(&a, l));
		unit_assert(ipstrtoaddr("2::ffff:192.168.0.2", 53, &a, &l));
		unit_assert(!addr_is_ip4mapped(&a, l));
	}
	/* test addr_is_any */
	unit_show_func("util/net_help.c", "addr_is_any");
	if(1) {
		struct sockaddr_storage a;
		socklen_t l = (socklen_t)sizeof(a);
		unit_assert(ipstrtoaddr("0.0.0.0", 53, &a, &l));
		unit_assert(addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("0.0.0.0", 10053, &a, &l));
		unit_assert(addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("0.0.0.0", 0, &a, &l));
		unit_assert(addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("::0", 0, &a, &l));
		unit_assert(addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("::0", 53, &a, &l));
		unit_assert(addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("::1", 53, &a, &l));
		unit_assert(!addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("2001:1667::1", 0, &a, &l));
		unit_assert(!addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("2001::0", 0, &a, &l));
		unit_assert(!addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("10.0.0.0", 0, &a, &l));
		unit_assert(!addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("0.0.0.10", 0, &a, &l));
		unit_assert(!addr_is_any(&a, l));
		unit_assert(ipstrtoaddr("192.0.2.1", 0, &a, &l));
		unit_assert(!addr_is_any(&a, l));
	}
}

#include "util/config_file.h"
/** test config_file: cfg_parse_memsize */
static void
config_memsize_test(void) 
{
	size_t v = 0;
	unit_show_func("util/config_file.c", "cfg_parse_memsize");
	if(0) {
		/* these emit errors */
		unit_assert( cfg_parse_memsize("", &v) == 0);
		unit_assert( cfg_parse_memsize("bla", &v) == 0);
		unit_assert( cfg_parse_memsize("nop", &v) == 0);
		unit_assert( cfg_parse_memsize("n0b", &v) == 0);
		unit_assert( cfg_parse_memsize("gb", &v) == 0);
		unit_assert( cfg_parse_memsize("b", &v) == 0);
		unit_assert( cfg_parse_memsize("kb", &v) == 0);
		unit_assert( cfg_parse_memsize("kk kb", &v) == 0);
	}
	unit_assert( cfg_parse_memsize("0", &v) && v==0);
	unit_assert( cfg_parse_memsize("1", &v) && v==1);
	unit_assert( cfg_parse_memsize("10", &v) && v==10);
	unit_assert( cfg_parse_memsize("10b", &v) && v==10);
	unit_assert( cfg_parse_memsize("5b", &v) && v==5);
	unit_assert( cfg_parse_memsize("1024", &v) && v==1024);
	unit_assert( cfg_parse_memsize("1k", &v) && v==1024);
	unit_assert( cfg_parse_memsize("1K", &v) && v==1024);
	unit_assert( cfg_parse_memsize("1Kb", &v) && v==1024);
	unit_assert( cfg_parse_memsize("1kb", &v) && v==1024);
	unit_assert( cfg_parse_memsize("1 kb", &v) && v==1024);
	unit_assert( cfg_parse_memsize("10 kb", &v) && v==10240);
	unit_assert( cfg_parse_memsize("2k", &v) && v==2048);
	unit_assert( cfg_parse_memsize("2m", &v) && v==2048*1024);
	unit_assert( cfg_parse_memsize("3M", &v) && v==3072*1024);
	unit_assert( cfg_parse_memsize("40m", &v) && v==40960*1024);
	unit_assert( cfg_parse_memsize("1G", &v) && v==1024*1024*1024);
	unit_assert( cfg_parse_memsize("1 Gb", &v) && v==1024*1024*1024);
	unit_assert( cfg_parse_memsize("0 Gb", &v) && v==0*1024*1024);
}

#include "util/rtt.h"
/** test RTT code */
static void
rtt_test(void)
{
	int init = 376;
	int i;
	struct rtt_info r;
	unit_show_func("util/rtt.c", "rtt_timeout");
	rtt_init(&r);
	/* initial value sensible */
	unit_assert( rtt_timeout(&r) == init );
	rtt_lost(&r, init);
	unit_assert( rtt_timeout(&r) == init*2 );
	rtt_lost(&r, init*2);
	unit_assert( rtt_timeout(&r) == init*4 );
	rtt_update(&r, 4000);
	unit_assert( rtt_timeout(&r) >= 2000 );
	rtt_lost(&r, rtt_timeout(&r) );
	for(i=0; i<100; i++) {
		rtt_lost(&r, rtt_timeout(&r) ); 
		unit_assert( rtt_timeout(&r) > RTT_MIN_TIMEOUT-1);
		unit_assert( rtt_timeout(&r) < RTT_MAX_TIMEOUT+1);
	}
}

#include "services/cache/infra.h"
#include "util/config_file.h"

/* lookup and get key and data structs easily */
static struct infra_data* infra_lookup_host(struct infra_cache* infra,
	struct sockaddr_storage* addr, socklen_t addrlen, uint8_t* zone,
	size_t zonelen, int wr, time_t now, struct infra_key** k)
{
	struct infra_data* d;
	struct lruhash_entry* e = infra_lookup_nottl(infra, addr, addrlen,
		zone, zonelen, wr);
	if(!e) return NULL;
	d = (struct infra_data*)e->data;
	if(d->ttl < now) {
		lock_rw_unlock(&e->lock);
		return NULL;
	}
	*k = (struct infra_key*)e->key;
	return d;
}

/** test host cache */
static void
infra_test(void)
{
	struct sockaddr_storage one;
	socklen_t onelen;
	uint8_t* zone = (uint8_t*)"\007example\003com\000";
	size_t zonelen = 13;
	struct infra_cache* slab;
	struct config_file* cfg = config_create();
	time_t now = 0;
	uint8_t edns_lame;
	int vs, to;
	struct infra_key* k;
	struct infra_data* d;
	int init = 376;

	unit_show_feature("infra cache");
	unit_assert(ipstrtoaddr("127.0.0.1", 53, &one, &onelen));

	slab = infra_create(cfg);
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, now,
		&vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init && edns_lame == 0 );

	unit_assert( infra_rtt_update(slab, &one, onelen, zone, zonelen, LDNS_RR_TYPE_A, -1, init, now) );
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init*2 && edns_lame == 0 );

	unit_assert( infra_edns_update(slab, &one, onelen, zone, zonelen, -1, now) );
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == -1 && to == init*2  && edns_lame == 1);

	now += cfg->host_ttl + 10;
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init && edns_lame == 0 );
	
	unit_assert( infra_set_lame(slab, &one, onelen,
		zone, zonelen,  now, 0, 0, LDNS_RR_TYPE_A) );
	unit_assert( (d=infra_lookup_host(slab, &one, onelen, zone, zonelen, 0, now, &k)) );
	unit_assert( d->ttl == now+cfg->host_ttl );
	unit_assert( d->edns_version == 0 );
	unit_assert(!d->isdnsseclame && !d->rec_lame && d->lame_type_A &&
		!d->lame_other);
	lock_rw_unlock(&k->entry.lock);

	/* test merge of data */
	unit_assert( infra_set_lame(slab, &one, onelen,
		zone, zonelen,  now, 0, 0, LDNS_RR_TYPE_AAAA) );
	unit_assert( (d=infra_lookup_host(slab, &one, onelen, zone, zonelen, 0, now, &k)) );
	unit_assert(!d->isdnsseclame && !d->rec_lame && d->lame_type_A &&
		d->lame_other);
	lock_rw_unlock(&k->entry.lock);

	/* test that noEDNS cannot overwrite known-yesEDNS */
	now += cfg->host_ttl + 10;
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init && edns_lame == 0 );

	unit_assert( infra_edns_update(slab, &one, onelen, zone, zonelen, 0, now) );
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init && edns_lame == 1 );

	unit_assert( infra_edns_update(slab, &one, onelen, zone, zonelen, -1, now) );
	unit_assert( infra_host(slab, &one, onelen, zone, zonelen, 
			now, &vs, &edns_lame, &to) );
	unit_assert( vs == 0 && to == init && edns_lame == 1 );

	infra_delete(slab);
	config_delete(cfg);
}

#include "util/random.h"
/** test randomness */
static void
rnd_test(void)
{
	struct ub_randstate* r;
	int num = 1000, i;
	long int a[1000];
	unsigned int seed = (unsigned)time(NULL);
	unit_show_feature("ub_random");
	printf("ub_random seed is %u\n", seed);
	unit_assert( (r = ub_initstate(seed, NULL)) );
	for(i=0; i<num; i++) {
		a[i] = ub_random(r);
		unit_assert(a[i] >= 0);
		unit_assert((size_t)a[i] <= (size_t)0x7fffffff);
		if(i > 5)
			unit_assert(a[i] != a[i-1] || a[i] != a[i-2] ||
				a[i] != a[i-3] || a[i] != a[i-4] ||
				a[i] != a[i-5] || a[i] != a[i-6]);
	}
	a[0] = ub_random_max(r, 1);
	unit_assert(a[0] >= 0 && a[0] < 1);
	a[0] = ub_random_max(r, 10000);
	unit_assert(a[0] >= 0 && a[0] < 10000);
	for(i=0; i<num; i++) {
		a[i] = ub_random_max(r, 10);
		unit_assert(a[i] >= 0 && a[i] < 10);
	}
	ub_randfree(r);
}

void unit_show_func(const char* file, const char* func)
{
	printf("test %s:%s\n", file, func);
}

void unit_show_feature(const char* feature)
{
	printf("test %s functions\n", feature);
}

/**
 * Main unit test program. Setup, teardown and report errors.
 * @param argc: arg count.
 * @param argv: array of commandline arguments.
 * @return program failure if test fails.
 */
int 
main(int argc, char* argv[])
{
	log_init(NULL, 0, NULL);
	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		printf("\tperforms unit tests.\n");
		return 1;
	}
	printf("Start of %s unit test.\n", PACKAGE_STRING);
#ifdef HAVE_SSL
	ERR_load_crypto_strings();
#  ifdef HAVE_OPENSSL_CONFIG
	OPENSSL_config("unbound");
#  endif
#  ifdef USE_GOST
	(void)sldns_key_EVP_load_gost_id();
#  endif
#elif defined(HAVE_NSS)
	if(NSS_NoDB_Init(".") != SECSuccess)
		fatal_exit("could not init NSS");
#endif /* HAVE_SSL or HAVE_NSS*/
	checklock_start();
	neg_test();
	rnd_test();
	verify_test();
	net_test();
	config_memsize_test();
	dname_test();
	rtt_test();
	anchors_test();
	alloc_test();
	regional_test();
	lruhash_test();
	slabhash_test();
	infra_test();
	ldns_test();
	msgparse_test();
	checklock_stop();
	printf("%d checks ok.\n", testcount);
#ifdef HAVE_SSL
#  if defined(USE_GOST) && defined(HAVE_LDNS_KEY_EVP_UNLOAD_GOST)
	sldns_key_EVP_unload_gost();
#  endif
#  ifdef HAVE_OPENSSL_CONFIG
	EVP_cleanup();
	ENGINE_cleanup();
	CONF_modules_free();
#  endif
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();
	RAND_cleanup();
#elif defined(HAVE_NSS)
	if(NSS_Shutdown() != SECSuccess)
		fatal_exit("could not shutdown NSS");
#endif /* HAVE_SSL or HAVE_NSS */
#ifdef HAVE_PTHREAD
	/* dlopen frees its thread specific state */
	pthread_exit(NULL);
#endif
	return 0;
}
