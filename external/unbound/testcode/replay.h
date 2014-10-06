/*
 * testcode/replay.h - store and use a replay of events for the DNS resolver.
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
 * Store and use a replay of events for the DNS resolver.
 * Used to test known scenarios to get known outcomes.
 *
 * <pre>
 * File format for replay files.
 *
 * ; unbound.conf options.
 * ; ...
 * ; additional commandline options to pass to unbound
 * COMMANDLINE cmdline_option
 * ; autotrust key file contents, also adds auto-trust-anchor-file: "x" to cfg
 * AUTOTRUST_FILE id
 * ; contents of that file
 * AUTOTRUST_END
 * CONFIG_END
 * ; comment line.
 * SCENARIO_BEGIN name_of_scenario
 * RANGE_BEGIN start_time end_time
 *    ; give ip of the virtual server, it matches any ip if not present.
 *    ADDRESS ip_address 
 *    match_entries
 * RANGE_END
 * ; more RANGE items.
 * ; go to the next moment
 * STEP time_step event_type [ADDRESS ip_address]
 * ; event_type can be:
 *	o NOTHING - nothing
 *	o QUERY - followed by entry
 *	o CHECK_ANSWER - followed by entry
 *	o CHECK_OUT_QUERY - followed by entry (if copy-id it is also reply).
 *	o REPLY - followed by entry
 *      o TIMEOUT
 *      o TIME_PASSES ELAPSE [seconds] - increase 'now' time counter, can be 
 *      			a floating point number.
 *        TIME_PASSES EVAL [macro] - expanded for seconds to move time.
 *      o TRAFFIC - like CHECK_ANSWER, causes traffic to flow.
 *		actually the traffic flows before this step is taken.
 *		the step waits for traffic to stop.
 *      o CHECK_AUTOTRUST [id] - followed by FILE_BEGIN [to match] FILE_END.
 *      	The file contents is macro expanded before match.
 *      o INFRA_RTT [ip] [dp] [rtt] - update infra cache entry with rtt.
 *      o ERROR
 * ; following entry starts on the next line, ENTRY_BEGIN.
 * ; more STEP items
 * SCENARIO_END
 *
 * Calculations, a macro-like system: ${$myvar + 3600}
 * STEP 10 ASSIGN myvar = 3600
 * 	; ASSIGN event. '=' is syntactic sugar here. 3600 is some expression.
 * ${..} is macro expanded from its expression.  Text substitution.
 * 	o $var replaced with its value.  var is identifier [azAZ09_]*
 * 	o number is that number.
 * 	o ${variables and arithmetic }
 * 	o +, -, / and *.  Note, evaluated left-to-right. Use ${} for brackets.
 * 	  So again, no precedence rules, so 2+3*4 == ${2+3}*4 = 20.
 * 	  Do 2+${3*4} to get 24.
 * 	o ${function params}
 *		o ${time} is the current time for the simulated unbound.
 *		o ${ctime value} is the text ctime(value), Fri 3 Aug 2009, ...
 *		o ${timeout} is the time until next timeout in comm_timer list.
 *		o ${range lower value upper} checks if lower<=value<=upper
 *			returns value if check succeeds.
 *
 * ; Example file
 * SCENARIO_BEGIN Example scenario
 * RANGE_BEGIN 0 100
 *   ENTRY_BEGIN
 *   ; precoded answers to queries.
 *   ENTRY_END
 * END_RANGE
 * STEP 0 QUERY
 *   ENTRY_BEGIN
 *   ; query
 *   ENTRY_END
 * ; a query is sent out to the network by resolver.
 * ; precoded answer from range is returned.
 * ; algorithm will do precoded answers from RANGE immediately, except if
 * ; the next step specifically checks for that OUT_QUERY.
 * ; or if none of the precoded answers match.
 * STEP 1 CHECK_ANSWER
 *   ENTRY_BEGIN
 *   ; what the reply should look like
 *   ENTRY_END
 * ; successful termination. (if the answer was OK).
 * ; also, all answers must have been checked with CHECK_ANSWER.
 * ; and, no more pending out_queries (that have not been checked).
 * SCENARIO_END
 * 
 * </pre>
 */

#ifndef TESTCODE_REPLAY_H
#define TESTCODE_REPLAY_H
#include "util/netevent.h"
#include "testcode/testpkts.h"
#include "util/rbtree.h"
struct replay_answer;
struct replay_moment;
struct replay_range;
struct fake_pending;
struct fake_timer;
struct replay_var;
struct infra_cache;
struct sldns_buffer;

/**
 * A replay scenario.
 */
struct replay_scenario {
	/** name of replay scenario. malloced string. */
	char* title;

	/** The list of replay moments. Linked list. Time increases in list. */
	struct replay_moment* mom_first;
	/** The last element in list of replay moments. */
	struct replay_moment* mom_last;

	/** 
	 * List of matching answers. This is to ease replay scenario
	 * creation. It lists queries (to the network) and what answer
	 * should be returned. The matching answers are valid for a range
	 * of time steps. 
	 * So: timestep, parts of query, destination --> answer.
	 */
	struct replay_range* range_list;
};

/**
 * A replay moment.
 * Basically, it consists of events to a fake select() call.
 * This is a recording of an event that happens.
 * And if output is presented, what is done with that.
 */
struct replay_moment {
	/** 
	 * The replay time step number. Starts at 0, time is incremented 
	 * every time the fake select() is run. 
	 */
	int time_step;
	/** Next replay moment in list of replay moments. */
	struct replay_moment* mom_next;

	/** what happens this moment? */
	enum replay_event_type {
		/** nothing happens, as if this event is not there. */
		repevt_nothing,
		/** incoming query */
		repevt_front_query,
		/** test fails if reply to query does not match */
		repevt_front_reply,
		/** timeout */
		repevt_timeout,
		/** time passes */
		repevt_time_passes,
		/** reply arrives from the network */
		repevt_back_reply,
		/** test fails if query to the network does not match */
		repevt_back_query,
		/** check autotrust key file */
		repevt_autotrust_check,
		/** an error happens to outbound query */
		repevt_error,
		/** assignment to a variable */
		repevt_assign,
		/** store infra rtt cache entry: addr and string (int) */
		repevt_infra_rtt,
		/** cause traffic to flow */
		repevt_traffic
	}
		/** variable with what is to happen this moment */
		evt_type;

	/** The sent packet must match this. Incoming events, the data. */
	struct entry* match;

	/** the amount of time that passes */
	struct timeval elapse;

	/** address that must be matched, or packet remote host address. */
	struct sockaddr_storage addr;
	/** length of addr, if 0, then any address will do */
	socklen_t addrlen;

	/** macro name, for assign. */
	char* variable;
	/** string argument, for assign. */
	char* string;

	/** the autotrust file id to check */
	char* autotrust_id;
	/** file contents to match, one string per line */
	struct config_strlist* file_content;
};

/**
 * Range of timesteps, and canned replies to matching queries.
 */
struct replay_range {
	/** time range when this is valid. Including start and end step. */
	int start_step;
	/** end step of time range. */
	int end_step;
	/** address of where this range is served. */
	struct sockaddr_storage addr;
	/** length of addr, if 0, then any address will do */
	socklen_t addrlen;

	/** Matching list */
	struct entry* match;

	/** next in list of time ranges. */
	struct replay_range* next_range;
};

/**
 * Replay storage of runtime information.
 */
struct replay_runtime {
	/**
	 * The scenario
	 */
	struct replay_scenario* scenario;
	/**
	 * Current moment.
	 */
	struct replay_moment* now;

	/** 
	 * List of pending queries in order they were sent out. First
	 * one has been sent out most recently. Last one in list is oldest. 
	 */
	struct fake_pending* pending_list;

	/**
	 * List of answers to queries from clients. These need to be checked.
	 */
	struct replay_answer* answer_list;
	
	/** last element in answer list. */
	struct replay_answer* answer_last;

	/** list of fake timer callbacks that are pending */
	struct fake_timer* timer_list;

	/** callback to call for incoming queries */
	comm_point_callback_t* callback_query;
	/** user argument for incoming query callback */
	void *cb_arg;

	/** ref the infra cache (was passed to outside_network_create) */
	struct infra_cache* infra;

	/** the current time in seconds */
	time_t now_secs;
	/** the current time in microseconds */
	struct timeval now_tv;

	/** signal handler callback */
	void (*sig_cb)(int, void*);
	/** signal handler user arg */
	void *sig_cb_arg;
	/** time to exit cleanly */
	int exit_cleanly;

	/** size of buffers */
	size_t bufsize;

	/**
	 * Tree of macro values. Of type replay_var
	 */
	rbtree_t* vars;
};

/**
 * Pending queries to network, fake replay version.
 */
struct fake_pending {
	/** what is important only that we remember the query, copied here. */
	struct sldns_buffer* buffer;
	/** and to what address this is sent to. */
	struct sockaddr_storage addr;
	/** len of addr */
	socklen_t addrlen;
	/** zone name, uncompressed wire format (as used when sent) */
	uint8_t* zone;
	/** length of zone name */
	size_t zonelen;
	/** qtype */
	int qtype;
	/** The callback function to call when answer arrives (or timeout) */
	comm_point_callback_t* callback;
	/** callback user argument */
	void* cb_arg;
	/** original timeout in seconds from 'then' */
	int timeout;

	/** next in pending list */
	struct fake_pending* next;
	/** the buffer parsed into a sldns_pkt */
	uint8_t* pkt;
	size_t pkt_len;
	/** by what transport was the query sent out */
	enum transport_type transport;
	/** if this is a serviced query */
	int serviced;
	/** the runtime structure this is part of */
	struct replay_runtime* runtime;
};

/**
 * An answer that is pending to happen.
 */
struct replay_answer {
	/** Next in list */
	struct replay_answer* next;
	/** reply information */
	struct comm_reply repinfo;
	/** the answer preparsed as ldns pkt */
	uint8_t* pkt;
	size_t pkt_len;
};

/**
 * Timers with callbacks, fake replay version.
 */
struct fake_timer {
	/** next in list */
	struct fake_timer* next;
	/** the runtime structure this is part of */
	struct replay_runtime* runtime;
	/** the callback to call */
	void (*cb)(void*);
	/** the callback user argument */
	void* cb_arg;
	/** if timer is enabled */
	int enabled;
	/** when the timer expires */
	struct timeval tv;
};

/**
 * Replay macro variable.  And its value.
 */
struct replay_var {
	/** rbtree node. Key is this structure. Sorted by name. */
	rbnode_t node;
	/** the variable name */
	char* name;
	/** the variable value */
	char* value;
};

/**
 * Read a replay scenario from the file.
 * @param in: file to read from.
 * @param name: name to print in errors.
 * @param lineno: incremented for every line read.
 * @return: Scenario. NULL if no scenario read.
 */
struct replay_scenario* replay_scenario_read(FILE* in, const char* name, 
	int* lineno);

/**
 * Delete scenario.
 * @param scen: to delete.
 */
void replay_scenario_delete(struct replay_scenario* scen);

/** compare two replay_vars */
int replay_var_compare(const void* a, const void* b);

/** get oldest enabled fake timer */
struct fake_timer* replay_get_oldest_timer(struct replay_runtime* runtime);

/**
 * Create variable storage
 * @return new or NULL on failure.
 */
rbtree_t* macro_store_create(void);

/**
 * Delete variable storage
 * @param store: the macro storage to free up.
 */
void macro_store_delete(rbtree_t* store);

/**
 * Apply macro substitution to string.
 * @param store: variable store.
 * @param runtime: the runtime to look up values as needed.
 * @param text: string to work on.
 * @return newly malloced string with result.
 */
char* macro_process(rbtree_t* store, struct replay_runtime* runtime, 
	char* text);

/**
 * Look up a macro value. Like calling ${$name}.
 * @param store: variable store
 * @param name: macro name
 * @return newly malloced string with result or strdup("") if not found.
 * 	or NULL on malloc failure.
 */
char* macro_lookup(rbtree_t* store, char* name);

/**
 * Set macro value.
 * @param store: variable store
 * @param name: macro name
 * @param value: text to set it to.  Not expanded.
 * @return false on failure.
 */
int macro_assign(rbtree_t* store, char* name, char* value);

/** Print macro variables stored as debug info */
void macro_print_debug(rbtree_t* store);

/** testbounds self test */
void testbound_selftest(void);

#endif /* TESTCODE_REPLAY_H */
