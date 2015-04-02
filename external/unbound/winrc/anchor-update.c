/*
 * winrc/anchor-update.c - windows trust anchor update util
 *
 * Copyright (c) 2009, NLnet Labs. All rights reserved.
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
 * This file is made because contrib/update-anchor.sh does not work on
 * windows (no shell). 
 */
#include "config.h"
#include "libunbound/unbound.h"
#include "sldns/rrdef.h"
#include "sldns/pkthdr.h"
#include "sldns/wire2str.h"

/** usage */
static void
usage(void)
{
	printf("usage: { name-of-domain filename }+ \n");
	printf("exit codes: 0 anchors updated, 1 no changes, 2 errors.\n");
	exit(1);
}

/** fatal exit */
static void fatal(const char* str)
{
	printf("fatal error: %s\n", str);
	exit(2);
}

/** lookup data */
static struct ub_result*
do_lookup(struct ub_ctx* ctx, char* domain)
{
	struct ub_result* result = NULL;
	int r;
	r = ub_resolve(ctx, domain, LDNS_RR_TYPE_DNSKEY, LDNS_RR_CLASS_IN,
		&result);
	if(r) {
		printf("failed to lookup %s\n", ub_strerror(r));
		fatal("ub_resolve failed");
	}
	if(!result->havedata && (result->rcode == LDNS_RCODE_SERVFAIL ||
		result->rcode == LDNS_RCODE_REFUSED))
		return NULL; /* probably no internet connection */
	if(!result->havedata) fatal("result has no data");
	if(!result->secure) fatal("result is not secure");
	return result;
}

/** print result to file */
static void
do_print(struct ub_result* result, char* file)
{
	FILE* out = fopen(file, "w");
	char s[65535], t[32];
	int i;
	if(!out) {
		perror(file);
		fatal("fopen failed");
	}
	i = 0;
	if(result->havedata)
	  while(result->data[i]) {
		sldns_wire2str_rdata_buf((uint8_t*)result->data[i],
			(size_t)result->len[i], s, sizeof(s),
			(uint16_t)result->qtype);
		sldns_wire2str_type_buf((uint16_t)result->qtype, t, sizeof(t));
		fprintf(out, "%s\t%s\t%s\n", result->qname, t, s);
		i++;
	}
	fclose(out);
}

/** update domain to file */
static int
do_update(char* domain, char* file)
{
	struct ub_ctx* ctx;
	struct ub_result* result;
	int r;
	printf("updating %s to %s\n", domain, file);
	ctx = ub_ctx_create();
	if(!ctx) fatal("ub_ctx_create failed");

	if((r=ub_ctx_add_ta_file(ctx, file))) {
		printf("%s\n", ub_strerror(r));
		fatal("ub_ctx_add_ta_file failed");
	}

	if(!(result=do_lookup(ctx, domain))) {
		ub_ctx_delete(ctx);
		return 1;
	}
	ub_ctx_delete(ctx);
	do_print(result, file);
	ub_resolve_free(result);
	return 0;
}

/** anchor update main */
int main(int argc, char** argv)
{
	int retcode = 1;
	if(argc == 1) {
		usage();
	}
	argc--;
	argv++;
	while(argc > 0) {
		int r = do_update(argv[0], argv[1]);
		if(r == 0) retcode = 0;

		/* next */
		argc-=2;
		argv+=2;
	}
	return retcode;
}
