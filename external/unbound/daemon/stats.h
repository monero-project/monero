/*
 * daemon/stats.h - collect runtime performance indicators.
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
 * This file describes the data structure used to collect runtime performance
 * numbers. These 'statistics' may be of interest to the operator.
 */

#ifndef DAEMON_STATS_H
#define DAEMON_STATS_H
#include "util/timehist.h"
struct worker;
struct config_file;
struct comm_point;
struct comm_reply;
struct edns_data;
struct sldns_buffer;

/** number of qtype that is stored for in array */
#define STATS_QTYPE_NUM 256
/** number of qclass that is stored for in array */
#define STATS_QCLASS_NUM 256
/** number of rcodes in stats */
#define STATS_RCODE_NUM 16
/** number of opcodes in stats */
#define STATS_OPCODE_NUM 16

/** per worker statistics */
struct server_stats {
	/** number of queries from clients received. */
	size_t num_queries;
	/** number of queries that had a cache-miss. */
	size_t num_queries_missed_cache;
	/** number of prefetch queries - cachehits with prefetch */
	size_t num_queries_prefetch;

	/**
	 * Sum of the querylistsize of the worker for 
	 * every query that missed cache. To calculate average.
	 */
	size_t sum_query_list_size;
	/** max value of query list size reached. */
	size_t max_query_list_size;

	/** Extended stats below (bool) */
	int extended;

	/** qtype stats */
	size_t qtype[STATS_QTYPE_NUM];
	/** bigger qtype values not in array */
	size_t qtype_big;
	/** qclass stats */
	size_t qclass[STATS_QCLASS_NUM];
	/** bigger qclass values not in array */
	size_t qclass_big;
	/** query opcodes */
	size_t qopcode[STATS_OPCODE_NUM];
	/** number of queries over TCP */
	size_t qtcp;
	/** number of outgoing queries over TCP */
	size_t qtcp_outgoing;
	/** number of queries over IPv6 */
	size_t qipv6;
	/** number of queries with QR bit */
	size_t qbit_QR;
	/** number of queries with AA bit */
	size_t qbit_AA;
	/** number of queries with TC bit */
	size_t qbit_TC;
	/** number of queries with RD bit */
	size_t qbit_RD;
	/** number of queries with RA bit */
	size_t qbit_RA;
	/** number of queries with Z bit */
	size_t qbit_Z;
	/** number of queries with AD bit */
	size_t qbit_AD;
	/** number of queries with CD bit */
	size_t qbit_CD;
	/** number of queries with EDNS OPT record */
	size_t qEDNS;
	/** number of queries with EDNS with DO flag */
	size_t qEDNS_DO;
	/** answer rcodes */
	size_t ans_rcode[STATS_RCODE_NUM];
	/** answers with pseudo rcode 'nodata' */
	size_t ans_rcode_nodata;
	/** answers that were secure (AD) */
	size_t ans_secure;
	/** answers that were bogus (withheld as SERVFAIL) */
	size_t ans_bogus;
	/** rrsets marked bogus by validator */
	size_t rrset_bogus;
	/** unwanted traffic received on server-facing ports */
	size_t unwanted_replies;
	/** unwanted traffic received on client-facing ports */
	size_t unwanted_queries;
	/** usage of tcp accept list */
	size_t tcp_accept_usage;

	/** histogram data exported to array 
	 * if the array is the same size, no data is lost, and
	 * if all histograms are same size (is so by default) then
	 * adding up works well. */
	size_t hist[NUM_BUCKETS_HIST];
	
	/** number of message cache entries */
	size_t msg_cache_count;
	/** number of rrset cache entries */
	size_t rrset_cache_count;
	/** number of infra cache entries */
	size_t infra_cache_count;
	/** number of key cache entries */
	size_t key_cache_count;
};

/** 
 * Statistics to send over the control pipe when asked
 * This struct is made to be memcpied, sent in binary.
 */
struct stats_info {
	/** the thread stats */
	struct server_stats svr;

	/** mesh stats: current number of states */
	size_t mesh_num_states;
	/** mesh stats: current number of reply (user) states */
	size_t mesh_num_reply_states;
	/** mesh stats: number of reply states overwritten with a new one */
	size_t mesh_jostled;
	/** mesh stats: number of incoming queries dropped */
	size_t mesh_dropped;
	/** mesh stats: replies sent */
	size_t mesh_replies_sent;
	/** mesh stats: sum of waiting times for the replies */
	struct timeval mesh_replies_sum_wait;
	/** mesh stats: median of waiting times for replies (in sec) */
	double mesh_time_median;
};

/** 
 * Initialize server stats to 0.
 * @param stats: what to init (this is alloced by the caller).
 * @param cfg: with extended statistics option.
 */
void server_stats_init(struct server_stats* stats, struct config_file* cfg);

/** add query if it missed the cache */
void server_stats_querymiss(struct server_stats* stats, struct worker* worker);

/** add query if was cached and also resulted in a prefetch */
void server_stats_prefetch(struct server_stats* stats, struct worker* worker);

/** display the stats to the log */
void server_stats_log(struct server_stats* stats, struct worker* worker,
	int threadnum);

/**
 * Obtain the stats info for a given thread. Uses pipe to communicate.
 * @param worker: the worker that is executing (the first worker).
 * @param who: on who to get the statistics info.
 * @param s: the stats block to fill in.
 * @param reset: if stats can be reset.
 */
void server_stats_obtain(struct worker* worker, struct worker* who,
	struct stats_info* s, int reset);

/**
 * Compile stats into structure for this thread worker.
 * Also clears the statistics counters (if that is set by config file).
 * @param worker: the worker to compile stats for, also the executing worker.
 * @param s: stats block.
 * @param reset: if true, depending on config stats are reset.
 * 	if false, statistics are not reset.
 */
void server_stats_compile(struct worker* worker, struct stats_info* s, 
	int reset);

/**
 * Send stats over comm tube in reply to query cmd
 * @param worker: this worker.
 * @param reset: if true, depending on config stats are reset.
 * 	if false, statistics are not reset.
 */
void server_stats_reply(struct worker* worker, int reset);

/**
 * Addup stat blocks.
 * @param total: sum of the two entries.
 * @param a: to add to it.
 */
void server_stats_add(struct stats_info* total, struct stats_info* a);

/**
 * Add stats for this query
 * @param stats: the stats
 * @param c: commpoint with type and buffer.
 * @param qtype: query type
 * @param qclass: query class
 * @param edns: edns record
 * @param repinfo: reply info with remote address
 */
void server_stats_insquery(struct server_stats* stats, struct comm_point* c,
	uint16_t qtype, uint16_t qclass, struct edns_data* edns, 
	struct comm_reply* repinfo);

/**
 * Add rcode for this query.
 * @param stats: the stats
 * @param buf: buffer with rcode. If buffer is length0: not counted.
 */
void server_stats_insrcode(struct server_stats* stats, struct sldns_buffer* buf);

#endif /* DAEMON_STATS_H */
