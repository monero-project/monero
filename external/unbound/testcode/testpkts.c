/*
 * testpkts. Data file parse for test packets, and query matching.
 *
 * Data storage for specially crafted replies for testing purposes.
 *
 * (c) NLnet Labs, 2005, 2006, 2007, 2008
 * See the file LICENSE for the license
 */

/**
 * \file
 * This is a debugging aid. It is not efficient, especially
 * with a long config file, but it can give any reply to any query.
 * This can help the developer pre-script replies for queries.
 *
 * You can specify a packet RR by RR with header flags to return.
 *
 * Missing features:
 *		- matching content different from reply content.
 *		- find way to adjust mangled packets?
 */

#include "config.h"
struct sockaddr_storage;
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include "testcode/testpkts.h"
#include "util/net_help.h"
#include "sldns/sbuffer.h"
#include "sldns/rrdef.h"
#include "sldns/pkthdr.h"
#include "sldns/str2wire.h"
#include "sldns/wire2str.h"

/** max size of a packet */
#define MAX_PACKETLEN 65536
/** max line length */
#define MAX_LINE   10240	
/** string to show in warnings and errors */
static const char* prog_name = "testpkts";

#ifndef UTIL_LOG_H
/** verbosity definition for compat */
enum verbosity_value { NO_VERBOSE=0 };
#endif
/** logging routine, provided by caller */
void verbose(enum verbosity_value lvl, const char* msg, ...) ATTR_FORMAT(printf, 2, 3);

/** print error and exit */
static void error(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s error: ", prog_name);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}

/** return if string is empty or comment */
static int isendline(char c)
{
	if(c == ';' || c == '#' 
		|| c == '\n' || c == 0)
		return 1;
	return 0;
}

/** true if the string starts with the keyword given. Moves the str ahead. 
 * @param str: before keyword, afterwards after keyword and spaces.
 * @param keyword: the keyword to match
 * @return: true if keyword present. False otherwise, and str unchanged.
*/
static int str_keyword(char** str, const char* keyword)
{
	size_t len = strlen(keyword);
	assert(str && keyword);
	if(strncmp(*str, keyword, len) != 0)
		return 0;
	*str += len;
	while(isspace((unsigned char)**str))
		(*str)++;
	return 1;
}

/** Add reply packet to entry */
static struct reply_packet*
entry_add_reply(struct entry* entry) 
{
	struct reply_packet* pkt = (struct reply_packet*)malloc(
		sizeof(struct reply_packet));
	struct reply_packet ** p = &entry->reply_list;
	if(!pkt) error("out of memory");
	pkt->next = NULL;
	pkt->packet_sleep = 0;
	pkt->reply_pkt = NULL;
	pkt->reply_from_hex = NULL;
	/* link at end */
	while(*p)
		p = &((*p)->next);
	*p = pkt;
	return pkt;
}

/** parse MATCH line */
static void matchline(char* line, struct entry* e)
{
	char* parse = line;
	while(*parse) {
		if(isendline(*parse)) 
			return;
		if(str_keyword(&parse, "opcode")) {
			e->match_opcode = 1;
		} else if(str_keyword(&parse, "qtype")) {
			e->match_qtype = 1;
		} else if(str_keyword(&parse, "qname")) {
			e->match_qname = 1;
		} else if(str_keyword(&parse, "subdomain")) {
			e->match_subdomain = 1;
		} else if(str_keyword(&parse, "all")) {
			e->match_all = 1;
		} else if(str_keyword(&parse, "ttl")) {
			e->match_ttl = 1;
		} else if(str_keyword(&parse, "DO")) {
			e->match_do = 1;
		} else if(str_keyword(&parse, "noedns")) {
			e->match_noedns = 1;
		} else if(str_keyword(&parse, "UDP")) {
			e->match_transport = transport_udp;
		} else if(str_keyword(&parse, "TCP")) {
			e->match_transport = transport_tcp;
		} else if(str_keyword(&parse, "serial")) {
			e->match_serial = 1;
			if(*parse != '=' && *parse != ':')
				error("expected = or : in MATCH: %s", line);
			parse++;
			e->ixfr_soa_serial = (uint32_t)strtol(parse, (char**)&parse, 10);
			while(isspace((unsigned char)*parse)) 
				parse++;
		} else {
			error("could not parse MATCH: '%s'", parse);
		}
	}
}

/** parse REPLY line */
static void replyline(char* line, uint8_t* reply, size_t reply_len,
	int* do_flag)
{
	char* parse = line;
	if(reply_len < LDNS_HEADER_SIZE) error("packet too short for header");
	while(*parse) {
		if(isendline(*parse)) 
			return;
			/* opcodes */
		if(str_keyword(&parse, "QUERY")) {
			LDNS_OPCODE_SET(reply, LDNS_PACKET_QUERY);
		} else if(str_keyword(&parse, "IQUERY")) {
			LDNS_OPCODE_SET(reply, LDNS_PACKET_IQUERY);
		} else if(str_keyword(&parse, "STATUS")) {
			LDNS_OPCODE_SET(reply, LDNS_PACKET_STATUS);
		} else if(str_keyword(&parse, "NOTIFY")) {
			LDNS_OPCODE_SET(reply, LDNS_PACKET_NOTIFY);
		} else if(str_keyword(&parse, "UPDATE")) {
			LDNS_OPCODE_SET(reply, LDNS_PACKET_UPDATE);
			/* rcodes */
		} else if(str_keyword(&parse, "NOERROR")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NOERROR);
		} else if(str_keyword(&parse, "FORMERR")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_FORMERR);
		} else if(str_keyword(&parse, "SERVFAIL")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_SERVFAIL);
		} else if(str_keyword(&parse, "NXDOMAIN")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NXDOMAIN);
		} else if(str_keyword(&parse, "NOTIMPL")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NOTIMPL);
		} else if(str_keyword(&parse, "REFUSED")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_REFUSED);
		} else if(str_keyword(&parse, "YXDOMAIN")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_YXDOMAIN);
		} else if(str_keyword(&parse, "YXRRSET")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_YXRRSET);
		} else if(str_keyword(&parse, "NXRRSET")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NXRRSET);
		} else if(str_keyword(&parse, "NOTAUTH")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NOTAUTH);
		} else if(str_keyword(&parse, "NOTZONE")) {
			LDNS_RCODE_SET(reply, LDNS_RCODE_NOTZONE);
			/* flags */
		} else if(str_keyword(&parse, "QR")) {
			LDNS_QR_SET(reply);
		} else if(str_keyword(&parse, "AA")) {
			LDNS_AA_SET(reply);
		} else if(str_keyword(&parse, "TC")) {
			LDNS_TC_SET(reply);
		} else if(str_keyword(&parse, "RD")) {
			LDNS_RD_SET(reply);
		} else if(str_keyword(&parse, "CD")) {
			LDNS_CD_SET(reply);
		} else if(str_keyword(&parse, "RA")) {
			LDNS_RA_SET(reply);
		} else if(str_keyword(&parse, "AD")) {
			LDNS_AD_SET(reply);
		} else if(str_keyword(&parse, "DO")) {
			*do_flag = 1;
		} else {
			error("could not parse REPLY: '%s'", parse);
		}
	}
}

/** parse ADJUST line */
static void adjustline(char* line, struct entry* e, 
	struct reply_packet* pkt)
{
	char* parse = line;
	while(*parse) {
		if(isendline(*parse)) 
			return;
		if(str_keyword(&parse, "copy_id")) {
			e->copy_id = 1;
		} else if(str_keyword(&parse, "copy_query")) {
			e->copy_query = 1;
		} else if(str_keyword(&parse, "sleep=")) {
			e->sleeptime = (unsigned int) strtol(parse, (char**)&parse, 10);
			while(isspace((unsigned char)*parse)) 
				parse++;
		} else if(str_keyword(&parse, "packet_sleep=")) {
			pkt->packet_sleep = (unsigned int) strtol(parse, (char**)&parse, 10);
			while(isspace((unsigned char)*parse)) 
				parse++;
		} else {
			error("could not parse ADJUST: '%s'", parse);
		}
	}
}

/** create new entry */
static struct entry* new_entry()
{
	struct entry* e = (struct entry*)malloc(sizeof(struct entry));
	if(!e) error("out of memory");
	memset(e, 0, sizeof(*e));
	e->match_opcode = 0;
	e->match_qtype = 0;
	e->match_qname = 0;
	e->match_subdomain = 0;
	e->match_all = 0;
	e->match_ttl = 0;
	e->match_do = 0;
	e->match_noedns = 0;
	e->match_serial = 0;
	e->ixfr_soa_serial = 0;
	e->match_transport = transport_any;
	e->reply_list = NULL;
	e->copy_id = 0;
	e->copy_query = 0;
	e->sleeptime = 0;
	e->next = NULL;
	return e;
}

/**
 * Converts a hex string to binary data
 * @param hexstr: string of hex.
 * @param len: is the length of the string
 * @param buf: is the buffer to store the result in
 * @param offset: is the starting position in the result buffer
 * @param buf_len: is the length of buf.
 * @return This function returns the length of the result
 */
static size_t
hexstr2bin(char *hexstr, int len, uint8_t *buf, size_t offset, size_t buf_len)
{
	char c;
	int i; 
	uint8_t int8 = 0;
	int sec = 0;
	size_t bufpos = 0;
	
	if (len % 2 != 0) {
		return 0;
	}

	for (i=0; i<len; i++) {
		c = hexstr[i];

		/* case insensitive, skip spaces */
		if (c != ' ') {
			if (c >= '0' && c <= '9') {
				int8 += c & 0x0f;  
			} else if (c >= 'a' && c <= 'z') {
				int8 += (c & 0x0f) + 9;   
			} else if (c >= 'A' && c <= 'Z') {
				int8 += (c & 0x0f) + 9;   
			} else {
				return 0;
			}
			 
			if (sec == 0) {
				int8 = int8 << 4;
				sec = 1;
			} else {
				if (bufpos + offset + 1 <= buf_len) {
					buf[bufpos+offset] = int8;
					int8 = 0;
					sec = 0; 
					bufpos++;
				} else {
					fprintf(stderr, "Buffer too small in hexstr2bin");
				}
			}
		}
        }
        return bufpos;
}

/** convert hex buffer to binary buffer */
static sldns_buffer *
hex_buffer2wire(sldns_buffer *data_buffer)
{
	sldns_buffer *wire_buffer = NULL;
	int c;
	
	/* stat hack
	 * 0 = normal
	 * 1 = comment (skip to end of line)
	 * 2 = unprintable character found, read binary data directly
	 */
	size_t data_buf_pos = 0;
	int state = 0;
	uint8_t *hexbuf;
	int hexbufpos = 0;
	size_t wirelen;
	uint8_t *data_wire = (uint8_t *) sldns_buffer_begin(data_buffer);
	uint8_t *wire = (uint8_t*)malloc(MAX_PACKETLEN);
	if(!wire) error("out of memory");
	
	hexbuf = (uint8_t*)malloc(MAX_PACKETLEN);
	if(!hexbuf) error("out of memory");
	for (data_buf_pos = 0; data_buf_pos < sldns_buffer_position(data_buffer); data_buf_pos++) {
		c = (int) data_wire[data_buf_pos];
		
		if (state < 2 && !isascii((unsigned char)c)) {
			/*verbose("non ascii character found in file: (%d) switching to raw mode\n", c);*/
			state = 2;
		}
		switch (state) {
			case 0:
				if (	(c >= '0' && c <= '9') ||
					(c >= 'a' && c <= 'f') ||
					(c >= 'A' && c <= 'F') )
				{
					if (hexbufpos >= MAX_PACKETLEN) {
						error("buffer overflow");
						free(hexbuf);
						return 0;

					}
					hexbuf[hexbufpos] = (uint8_t) c;
					hexbufpos++;
				} else if (c == ';') {
					state = 1;
				} else if (c == ' ' || c == '\t' || c == '\n') {
					/* skip whitespace */
				} 
				break;
			case 1:
				if (c == '\n' || c == EOF) {
					state = 0;
				}
				break;
			case 2:
				if (hexbufpos >= MAX_PACKETLEN) {
					error("buffer overflow");
					free(hexbuf);
					return 0;
				}
				hexbuf[hexbufpos] = (uint8_t) c;
				hexbufpos++;
				break;
		}
	}

	if (hexbufpos >= MAX_PACKETLEN) {
		/*verbose("packet size reached\n");*/
	}
	
	/* lenient mode: length must be multiple of 2 */
	if (hexbufpos % 2 != 0) {
		if (hexbufpos >= MAX_PACKETLEN) {
			error("buffer overflow");
			free(hexbuf);
			return 0;
		}
		hexbuf[hexbufpos] = (uint8_t) '0';
		hexbufpos++;
	}

	if (state < 2) {
		wirelen = hexstr2bin((char *) hexbuf, hexbufpos, wire, 0, MAX_PACKETLEN);
		wire_buffer = sldns_buffer_new(wirelen);
		sldns_buffer_new_frm_data(wire_buffer, wire, wirelen);
	} else {
		error("Incomplete hex data, not at byte boundary\n");
	}
	free(wire);
	free(hexbuf);
	return wire_buffer;
}	

/** parse ORIGIN */
static void 
get_origin(const char* name, struct sldns_file_parse_state* pstate, char* parse)
{
	/* snip off rest of the text so as to make the parse work in ldns */
	char* end;
	char store;
	int status;

	end=parse;
	while(!isspace((unsigned char)*end) && !isendline(*end))
		end++;
	store = *end;
	*end = 0;
	verbose(3, "parsing '%s'\n", parse);
	status = sldns_str2wire_dname_buf(parse, pstate->origin,
		&pstate->origin_len);
	*end = store;
	if(status != 0)
		error("%s line %d:\n\t%s: %s", name, pstate->lineno,
			sldns_get_errorstr_parse(status), parse);
}

/** add RR to packet */
static void add_rr(char* rrstr, uint8_t* pktbuf, size_t pktsize,
	size_t* pktlen, struct sldns_file_parse_state* pstate,
	sldns_pkt_section add_section, const char* fname)
{
	/* it must be a RR, parse and add to packet. */
	size_t rr_len = pktsize - *pktlen;
	size_t dname_len = 0;
	int status;
	uint8_t* origin = pstate->origin_len?pstate->origin:0;
	uint8_t* prev = pstate->prev_rr_len?pstate->prev_rr:0;
	if(*pktlen > pktsize || *pktlen < LDNS_HEADER_SIZE)
		error("packet overflow");

	/* parse RR */
	if(add_section == LDNS_SECTION_QUESTION)
		status = sldns_str2wire_rr_question_buf(rrstr, pktbuf+*pktlen,
			&rr_len, &dname_len, origin, pstate->origin_len,
			prev, pstate->prev_rr_len);
	else status = sldns_str2wire_rr_buf(rrstr, pktbuf+*pktlen, &rr_len,
			&dname_len, pstate->default_ttl, origin,
			pstate->origin_len, prev, pstate->prev_rr_len);
	if(status != 0)
		error("%s line %d:%d %s\n\t%s", fname, pstate->lineno,
			LDNS_WIREPARSE_OFFSET(status),
			sldns_get_errorstr_parse(status), rrstr);
	*pktlen += rr_len;

	/* increase RR count */
	if(add_section == LDNS_SECTION_QUESTION)
		sldns_write_uint16(pktbuf+4, LDNS_QDCOUNT(pktbuf)+1);
	else if(add_section == LDNS_SECTION_ANSWER)
		sldns_write_uint16(pktbuf+6, LDNS_ANCOUNT(pktbuf)+1);
	else if(add_section == LDNS_SECTION_AUTHORITY)
		sldns_write_uint16(pktbuf+8, LDNS_NSCOUNT(pktbuf)+1);
	else if(add_section == LDNS_SECTION_ADDITIONAL)
		sldns_write_uint16(pktbuf+10, LDNS_ARCOUNT(pktbuf)+1);
	else error("internal error bad section %d", (int)add_section);
}

/* add EDNS 4096 DO opt record */
static void
add_do_flag(uint8_t* pktbuf, size_t pktsize, size_t* pktlen)
{
	uint8_t edns[] = {0x00, /* root label */
		0x00, LDNS_RR_TYPE_OPT, /* type */
		0x10, 0x00, /* class is UDPSIZE 4096 */
		0x00, /* TTL[0] is ext rcode */
		0x00, /* TTL[1] is edns version */
		0x80, 0x00, /* TTL[2-3] is edns flags, DO */
		0x00, 0x00 /* rdatalength (0 options) */
	};
	if(*pktlen < LDNS_HEADER_SIZE)
		return;
	if(*pktlen + sizeof(edns) > pktsize)
		error("not enough space for EDNS OPT record");
	memmove(pktbuf+*pktlen, edns, sizeof(edns));
	sldns_write_uint16(pktbuf+10, LDNS_ARCOUNT(pktbuf)+1);
	*pktlen += sizeof(edns);
}

/* Reads one entry from file. Returns entry or NULL on error. */
struct entry*
read_entry(FILE* in, const char* name, struct sldns_file_parse_state* pstate,
	int skip_whitespace)
{
	struct entry* current = NULL;
	char line[MAX_LINE];
	char* parse;
	sldns_pkt_section add_section = LDNS_SECTION_QUESTION;
	struct reply_packet *cur_reply = NULL;
	int reading_hex = 0;
	sldns_buffer* hex_data_buffer = NULL;
	uint8_t pktbuf[MAX_PACKETLEN];
	size_t pktlen = LDNS_HEADER_SIZE;
	int do_flag = 0; /* DO flag in EDNS */
	memset(pktbuf, 0, pktlen); /* ID = 0, FLAGS="", and rr counts 0 */

	while(fgets(line, (int)sizeof(line), in) != NULL) {
		line[MAX_LINE-1] = 0;
		parse = line;
		pstate->lineno++;

		while(isspace((unsigned char)*parse))
			parse++;
		/* test for keywords */
		if(isendline(*parse))
			continue; /* skip comment and empty lines */
		if(str_keyword(&parse, "ENTRY_BEGIN")) {
			if(current) {
				error("%s line %d: previous entry does not ENTRY_END", 
					name, pstate->lineno);
			}
			current = new_entry();
			current->lineno = pstate->lineno;
			cur_reply = entry_add_reply(current);
			continue;
		} else if(str_keyword(&parse, "$ORIGIN")) {
			get_origin(name, pstate, parse);
			continue;
		} else if(str_keyword(&parse, "$TTL")) {
			pstate->default_ttl = (uint32_t)atoi(parse);
			continue;
		}

		/* working inside an entry */
		if(!current) {
			error("%s line %d: expected ENTRY_BEGIN but got %s", 
				name, pstate->lineno, line);
		}
		if(str_keyword(&parse, "MATCH")) {
			matchline(parse, current);
		} else if(str_keyword(&parse, "REPLY")) {
			replyline(parse, pktbuf, pktlen, &do_flag);
		} else if(str_keyword(&parse, "ADJUST")) {
			adjustline(parse, current, cur_reply);
		} else if(str_keyword(&parse, "EXTRA_PACKET")) {
			cur_reply = entry_add_reply(current);
		} else if(str_keyword(&parse, "SECTION")) {
			if(str_keyword(&parse, "QUESTION"))
				add_section = LDNS_SECTION_QUESTION;
			else if(str_keyword(&parse, "ANSWER"))
				add_section = LDNS_SECTION_ANSWER;
			else if(str_keyword(&parse, "AUTHORITY"))
				add_section = LDNS_SECTION_AUTHORITY;
			else if(str_keyword(&parse, "ADDITIONAL"))
				add_section = LDNS_SECTION_ADDITIONAL;
			else error("%s line %d: bad section %s", name, pstate->lineno, parse);
		} else if(str_keyword(&parse, "HEX_ANSWER_BEGIN")) {
			hex_data_buffer = sldns_buffer_new(MAX_PACKETLEN);
			reading_hex = 1;
		} else if(str_keyword(&parse, "HEX_ANSWER_END")) {
			if(!reading_hex) {
				error("%s line %d: HEX_ANSWER_END read but no HEX_ANSWER_BEGIN keyword seen", name, pstate->lineno);
			}
			reading_hex = 0;
			cur_reply->reply_from_hex = hex_buffer2wire(hex_data_buffer);
			sldns_buffer_free(hex_data_buffer);
			hex_data_buffer = NULL;
		} else if(str_keyword(&parse, "ENTRY_END")) {
			if(hex_data_buffer)
				sldns_buffer_free(hex_data_buffer);
			if(pktlen != 0) {
				if(do_flag)
					add_do_flag(pktbuf, sizeof(pktbuf),
						&pktlen);
				cur_reply->reply_pkt = memdup(pktbuf, pktlen);
				cur_reply->reply_len = pktlen;
				if(!cur_reply->reply_pkt)
					error("out of memory");
			}
			return current;
		} else if(reading_hex) {
			sldns_buffer_printf(hex_data_buffer, "%s", line);
		} else {
			add_rr(skip_whitespace?parse:line, pktbuf,
				sizeof(pktbuf), &pktlen, pstate, add_section,
				name);
		}

	}
	if (reading_hex) {
		error("%s: End of file reached while still reading hex, "
			"missing HEX_ANSWER_END\n", name);
	}
	if(current) {
		error("%s: End of file reached while reading entry. "
			"missing ENTRY_END\n", name);
	}
	return 0;
}

/* reads the canned reply file and returns a list of structs */
struct entry* 
read_datafile(const char* name, int skip_whitespace)
{
	struct entry* list = NULL;
	struct entry* last = NULL;
	struct entry* current = NULL;
	FILE *in;
	struct sldns_file_parse_state pstate;
	int entry_num = 0;
	memset(&pstate, 0, sizeof(pstate));

	if((in=fopen(name, "r")) == NULL) {
		error("could not open file %s: %s", name, strerror(errno));
	}

	while((current = read_entry(in, name, &pstate, skip_whitespace)))
	{
		if(last)
			last->next = current;
		else	list = current;
		last = current;
		entry_num ++;
	}
	verbose(1, "%s: Read %d entries\n", prog_name, entry_num);

	fclose(in);
	return list;
}

/** get qtype from packet */
static sldns_rr_type get_qtype(uint8_t* pkt, size_t pktlen)
{
	uint8_t* d;
	size_t dl, sl=0;
	char* snull = NULL;
	if(pktlen < LDNS_HEADER_SIZE)
		return 0;
	if(LDNS_QDCOUNT(pkt) == 0)
		return 0;
	/* skip over dname with dname-scan routine */
	d = pkt+LDNS_HEADER_SIZE;
	dl = pktlen-LDNS_HEADER_SIZE;
	(void)sldns_wire2str_dname_scan(&d, &dl, &snull, &sl, pkt, pktlen);
	if(dl < 2)
		return 0;
	return sldns_read_uint16(d);
}

/** get qtype from packet */
static size_t get_qname_len(uint8_t* pkt, size_t pktlen)
{
	uint8_t* d;
	size_t dl, sl=0;
	char* snull = NULL;
	if(pktlen < LDNS_HEADER_SIZE)
		return 0;
	if(LDNS_QDCOUNT(pkt) == 0)
		return 0;
	/* skip over dname with dname-scan routine */
	d = pkt+LDNS_HEADER_SIZE;
	dl = pktlen-LDNS_HEADER_SIZE;
	(void)sldns_wire2str_dname_scan(&d, &dl, &snull, &sl, pkt, pktlen);
	return pktlen-dl-LDNS_HEADER_SIZE;
}

/** returns owner from packet */
static uint8_t* get_qname(uint8_t* pkt, size_t pktlen)
{
	if(pktlen < LDNS_HEADER_SIZE)
		return NULL;
	if(LDNS_QDCOUNT(pkt) == 0)
		return NULL;
	return pkt+LDNS_HEADER_SIZE;
}

/** returns opcode from packet */
static int get_opcode(uint8_t* pkt, size_t pktlen)
{
	if(pktlen < LDNS_HEADER_SIZE)
		return 0;
	return (int)LDNS_OPCODE_WIRE(pkt);
}

/** get authority section SOA serial value */
static uint32_t get_serial(uint8_t* p, size_t plen)
{
	uint8_t* walk = p;
	size_t walk_len = plen, sl=0;
	char* snull = NULL;
	uint16_t i;

	if(walk_len < LDNS_HEADER_SIZE)
		return 0;
	walk += LDNS_HEADER_SIZE;
	walk_len -= LDNS_HEADER_SIZE;

	/* skip other records with wire2str_scan */
	for(i=0; i < LDNS_QDCOUNT(p); i++)
		(void)sldns_wire2str_rrquestion_scan(&walk, &walk_len,
			&snull, &sl, p, plen);
	for(i=0; i < LDNS_ANCOUNT(p); i++)
		(void)sldns_wire2str_rr_scan(&walk, &walk_len, &snull, &sl,
			p, plen);

	/* walk through authority section */
	for(i=0; i < LDNS_NSCOUNT(p); i++) {
		/* if this is SOA then get serial, skip compressed dname */
		uint8_t* dstart = walk;
		size_t dlen = walk_len;
		(void)sldns_wire2str_dname_scan(&dstart, &dlen, &snull, &sl,
			p, plen);
		if(dlen >= 2 && sldns_read_uint16(dstart) == LDNS_RR_TYPE_SOA) {
			/* skip type, class, TTL, rdatalen */
			if(dlen < 10)
				return 0;
			if(dlen < 10 + (size_t)sldns_read_uint16(dstart+8))
				return 0;
			dstart += 10;
			dlen -= 10;
			/* check third rdf */
			(void)sldns_wire2str_dname_scan(&dstart, &dlen, &snull,
				&sl, p, plen);
			(void)sldns_wire2str_dname_scan(&dstart, &dlen, &snull,
				&sl, p, plen);
			if(dlen < 4)
				return 0;
			verbose(3, "found serial %u in msg. ",
				(int)sldns_read_uint32(dstart));
			return sldns_read_uint32(dstart);
		}
		/* move to next RR */
		(void)sldns_wire2str_rr_scan(&walk, &walk_len, &snull, &sl,
			p, plen);
	}
	return 0;
}

/** get ptr to EDNS OPT record (and remaining length); behind the type u16 */
static int
pkt_find_edns_opt(uint8_t** p, size_t* plen)
{
	/* walk over the packet with scan routines */
	uint8_t* w = *p;
	size_t wlen = *plen, sl=0;
	char* snull = NULL;
	uint16_t i;

	if(wlen < LDNS_HEADER_SIZE)
		return 0;
	w += LDNS_HEADER_SIZE;
	wlen -= LDNS_HEADER_SIZE;

	/* skip other records with wire2str_scan */
	for(i=0; i < LDNS_QDCOUNT(p); i++)
		(void)sldns_wire2str_rrquestion_scan(&w, &wlen, &snull, &sl,
			*p, *plen);
	for(i=0; i < LDNS_ANCOUNT(p); i++)
		(void)sldns_wire2str_rr_scan(&w, &wlen, &snull, &sl, *p, *plen);
	for(i=0; i < LDNS_NSCOUNT(p); i++)
		(void)sldns_wire2str_rr_scan(&w, &wlen, &snull, &sl, *p, *plen);

	/* walk through additional section */
	for(i=0; i < LDNS_ARCOUNT(p); i++) {
		/* if this is OPT then done */
		uint8_t* dstart = w;
		size_t dlen = wlen;
		(void)sldns_wire2str_dname_scan(&dstart, &dlen, &snull, &sl,
			*p, *plen);
		if(dlen >= 2 && sldns_read_uint16(dstart) == LDNS_RR_TYPE_OPT) {
			*p = dstart+2;
			*plen = dlen-2;
			return 1;
		}
		/* move to next RR */
		(void)sldns_wire2str_rr_scan(&w, &wlen, &snull, &sl, *p, *plen);
	}
	return 0;
}

/** return true if the packet has EDNS OPT record */
static int
get_has_edns(uint8_t* pkt, size_t len)
{
	/* use arguments as temporary variables */
	return pkt_find_edns_opt(&pkt, &len);
}

/** return true if the DO flag is set */
static int
get_do_flag(uint8_t* pkt, size_t len)
{
	uint16_t edns_bits;
	uint8_t* walk = pkt;
	size_t walk_len = len;
	if(pkt_find_edns_opt(&walk, &walk_len)) {
		return 1;
	}
	if(walk_len < 6)
		return 0; /* malformed */
	edns_bits = sldns_read_uint16(walk+4);
	return (int)(edns_bits&LDNS_EDNS_MASK_DO_BIT);
}

/** zero TTLs in packet */
static void
zerottls(uint8_t* pkt, size_t pktlen)
{
	uint8_t* walk = pkt;
	size_t walk_len = pktlen, sl=0;
	char* snull = NULL;
	uint16_t i;
	uint16_t num = LDNS_ANCOUNT(pkt)+LDNS_NSCOUNT(pkt)+LDNS_ARCOUNT(pkt);
	if(walk_len < LDNS_HEADER_SIZE)
		return;
	walk += LDNS_HEADER_SIZE;
	walk_len -= LDNS_HEADER_SIZE;
	for(i=0; i < LDNS_QDCOUNT(pkt); i++)
		(void)sldns_wire2str_rrquestion_scan(&walk, &walk_len,
			&snull, &sl, pkt, pktlen);
	for(i=0; i < num; i++) {
		/* wipe TTL */
		uint8_t* dstart = walk;
		size_t dlen = walk_len;
		(void)sldns_wire2str_dname_scan(&dstart, &dlen, &snull, &sl,
			pkt, pktlen);
		if(dlen < 8)
			return;
		sldns_write_uint32(dstart+4, 0);
		/* go to next RR */
		(void)sldns_wire2str_rr_scan(&walk, &walk_len, &snull, &sl,
			pkt, pktlen);
	}
}

/** get one line (\n) from a string, move next to after the \n, zero \n */
static int
get_line(char** s, char** n)
{
	/* at end of string? end */
	if(*n == NULL || **n == 0)
		return 0;
	/* result starts at next string */
	*s = *n;
	/* find \n after that */
	*n = strchr(*s, '\n');
	if(*n && **n != 0) {
		/* terminate line */
		(*n)[0] = 0;
		(*n)++;
	}
	return 1;
}

/** match two RR sections without ordering */
static int
match_noloc_section(char** q, char** nq, char** p, char** np, uint16_t num)
{
	/* for max number of RRs in packet */
	const uint16_t numarray = 3000;
	char* qlines[numarray], *plines[numarray];
	uint16_t i, j, numq=0, nump=0;
	if(num > numarray) fatal_exit("too many RRs");
	/* gather lines */
	for(i=0; i<num; i++) {
		get_line(q, nq);
		get_line(p, np);
		qlines[numq++] = *q;
		plines[nump++] = *p;
	}
	/* see if they are all present in the other */
	for(i=0; i<num; i++) {
		int found = 0;
		for(j=0; j<num; j++) {
			if(strcmp(qlines[i], plines[j]) == 0) {
				found = 1;
				break;
			}
		}
		if(!found) {
			verbose(3, "comparenoloc: failed for %s", qlines[i]);
			return 0;
		}
	}
	return 1;
}

/** match two strings for unordered equality of RRs and everything else */
static int
match_noloc(char* q, char* p, uint8_t* q_pkt, size_t q_pkt_len,
	uint8_t* p_pkt, size_t p_pkt_len)
{
	char* nq = q, *np = p;
	/* if no header, compare bytes */
	if(p_pkt_len < LDNS_HEADER_SIZE || q_pkt_len < LDNS_HEADER_SIZE) {
		if(p_pkt_len != q_pkt_len) return 0;
		return memcmp(p, q, p_pkt_len);
	}
	/* compare RR counts */
	if(LDNS_QDCOUNT(p_pkt) != LDNS_QDCOUNT(q_pkt))
		return 0;
	if(LDNS_ANCOUNT(p_pkt) != LDNS_ANCOUNT(q_pkt))
		return 0;
	if(LDNS_NSCOUNT(p_pkt) != LDNS_NSCOUNT(q_pkt))
		return 0;
	if(LDNS_ARCOUNT(p_pkt) != LDNS_ARCOUNT(q_pkt))
		return 0;
	/* get a line from both; compare; at sections do section */
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) {
		/* header line opcode, rcode, id */
		return 0;
	}
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) {
		/* header flags, rr counts */
		return 0;
	}
	/* ;; QUESTION SECTION */
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	if(!match_noloc_section(&q, &nq, &p, &np, LDNS_QDCOUNT(p_pkt)))
		return 0;

	/* empty line and ;; ANSWER SECTION */
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	if(!match_noloc_section(&q, &nq, &p, &np, LDNS_ANCOUNT(p_pkt)))
		return 0;

	/* empty line and ;; AUTHORITY SECTION */
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	if(!match_noloc_section(&q, &nq, &p, &np, LDNS_NSCOUNT(p_pkt)))
		return 0;

	/* empty line and ;; ADDITIONAL SECTION */
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	get_line(&q, &nq);
	get_line(&p, &np);
	if(strcmp(q, p) != 0) return 0;
	if(!match_noloc_section(&q, &nq, &p, &np, LDNS_ARCOUNT(p_pkt)))
		return 0;

	return 1;
}

/** lowercase domain name - does not follow compression pointers */
static void lowercase_dname(uint8_t** p, size_t* remain)
{
	unsigned i, llen;
	if(*remain == 0) return;
	while(**p != 0) {
		/* compressed? */
		if((**p & 0xc0) == 0xc0) {
			*p += 2;
			*remain -= 2;
			return;
		}
		llen = (unsigned int)**p;
		*p += 1;
		*remain -= 1;
		if(*remain < llen)
			llen = (unsigned int)*remain;
		for(i=0; i<llen; i++) {
			(*p)[i] = (uint8_t)tolower((int)(*p)[i]);
		}
		*p += llen;
		*remain -= llen;
		if(*remain == 0) return;
	}
	/* skip root label */
	*p += 1;
	*remain -= 1;
}

/** lowercase rdata of type */
static void lowercase_rdata(uint8_t** p, size_t* remain,
	uint16_t rdatalen, uint16_t t)
{
	const sldns_rr_descriptor *desc = sldns_rr_descript(t);
	uint8_t dname_count = 0;
	size_t i = 0;
	size_t rdataremain = rdatalen;
	if(!desc) {
		/* unknown type */
		*p += rdatalen;
		*remain -= rdatalen;
		return;
	}
	while(dname_count < desc->_dname_count) {
		sldns_rdf_type f = sldns_rr_descriptor_field_type(desc, i++);
		if(f == LDNS_RDF_TYPE_DNAME) {
			lowercase_dname(p, &rdataremain);
			dname_count++;
		} else if(f == LDNS_RDF_TYPE_STR) {
			uint8_t len;
			if(rdataremain == 0) return;
			len = **p;
			*p += len+1;
			rdataremain -= len+1;
		} else {
			int len = 0;
			switch(f) {
			case LDNS_RDF_TYPE_CLASS:
			case LDNS_RDF_TYPE_ALG:
			case LDNS_RDF_TYPE_INT8:
				len = 1;
				break;
			case LDNS_RDF_TYPE_INT16:
			case LDNS_RDF_TYPE_TYPE:
			case LDNS_RDF_TYPE_CERT_ALG:
				len = 2;
				break;
			case LDNS_RDF_TYPE_INT32:
			case LDNS_RDF_TYPE_TIME:
			case LDNS_RDF_TYPE_A:
			case LDNS_RDF_TYPE_PERIOD:
				len = 4;
				break;
			case LDNS_RDF_TYPE_TSIGTIME:
				len = 6;
				break;
			case LDNS_RDF_TYPE_AAAA:
				len = 16;
				break;
			default: error("bad rdf type in lowercase %d", (int)f);
			}
			*p += len;
			rdataremain -= len;
		}
	}
	/* skip remainder of rdata */
	*p += rdataremain;
	*remain -= rdatalen;
}

/** lowercase all names in the message */
static void lowercase_pkt(uint8_t* pkt, size_t pktlen)
{
	uint16_t i;
	uint8_t* p = pkt;
	size_t remain = pktlen;
	uint16_t t, rdatalen;
	if(pktlen < LDNS_HEADER_SIZE)
		return;
	p += LDNS_HEADER_SIZE;
	remain -= LDNS_HEADER_SIZE;
	for(i=0; i<LDNS_QDCOUNT(pkt); i++) {
		lowercase_dname(&p, &remain);
		if(remain < 4) return;
		p += 4;
		remain -= 4;
	}
	for(i=0; i<LDNS_ANCOUNT(pkt)+LDNS_NSCOUNT(pkt)+LDNS_ARCOUNT(pkt); i++) {
		lowercase_dname(&p, &remain);
		if(remain < 10) return;
		t = sldns_read_uint16(p);
		rdatalen = sldns_read_uint16(p+8);
		p += 10;
		remain -= 10;
		if(remain < rdatalen) return;
		lowercase_rdata(&p, &remain, rdatalen, t);
	}
}

/** match all of the packet */
int
match_all(uint8_t* q, size_t qlen, uint8_t* p, size_t plen, int mttl,
	int noloc)
{
	char* qstr, *pstr;
	uint8_t* qb = q, *pb = p;
	int r;
	/* zero TTLs */
	qb = memdup(q, qlen);
	pb = memdup(p, plen);
	if(!qb || !pb) error("out of memory");
	if(!mttl) {
		zerottls(qb, qlen);
		zerottls(pb, plen);
	}
	lowercase_pkt(qb, qlen);
	lowercase_pkt(pb, plen);
	qstr = sldns_wire2str_pkt(qb, qlen);
	pstr = sldns_wire2str_pkt(pb, plen);
	if(!qstr || !pstr) error("cannot pkt2string");
	r = (strcmp(qstr, pstr) == 0);
	if(!r) {
		/* remove ;; MSG SIZE (at end of string) */
		char* s = strstr(qstr, ";; MSG SIZE");
		if(s) *s=0;
		s = strstr(pstr, ";; MSG SIZE");
		if(s) *s=0;
		r = (strcmp(qstr, pstr) == 0);
		if(!r && !noloc) {
			/* we are going to fail see if it is because of EDNS */
			char* a = strstr(qstr, "; EDNS");
			char* b = strstr(pstr, "; EDNS");
			if( (a&&!b) || (b&&!a) ) {
				verbose(3, "mismatch in EDNS\n");
			}
		}
	}
	if(!r && noloc) {
		/* check for reordered sections */
		r = match_noloc(qstr, pstr, q, qlen, p, plen);
	}
	free(qstr);
	free(pstr);
	free(qb);
	free(pb);
	return r;
}

/** see if domain names are equal */
static int equal_dname(uint8_t* q, size_t qlen, uint8_t* p, size_t plen)
{
	uint8_t* qn = get_qname(q, qlen);
	uint8_t* pn = get_qname(p, plen);
	char qs[512], ps[512];
	size_t qslen = sizeof(qs), pslen = sizeof(ps);
	char* qss = qs, *pss = ps;
	if(!qn || !pn)
		return 0;
	(void)sldns_wire2str_dname_scan(&qn, &qlen, &qss, &qslen, q, qlen);
	(void)sldns_wire2str_dname_scan(&pn, &plen, &pss, &pslen, p, plen);
	return (strcmp(qs, ps) == 0);
}

/** see if domain names are subdomain q of p */
static int subdomain_dname(uint8_t* q, size_t qlen, uint8_t* p, size_t plen)
{
	/* we use the tostring routines so as to test unbound's routines
	 * with something else */
	uint8_t* qn = get_qname(q, qlen);
	uint8_t* pn = get_qname(p, plen);
	char qs[5120], ps[5120];
	size_t qslen = sizeof(qs), pslen = sizeof(ps);
	char* qss = qs, *pss = ps;
	if(!qn || !pn)
		return 0;
	/* decompresses domain names */
	(void)sldns_wire2str_dname_scan(&qn, &qlen, &qss, &qslen, q, qlen);
	(void)sldns_wire2str_dname_scan(&pn, &plen, &pss, &pslen, p, plen);
	/* same: false, (strict subdomain check)??? */
	if(strcmp(qs, ps) == 0)
		return 1;
	/* qs must end in ps, at a dot, without \ in front */
	qslen = strlen(qs);
	pslen = strlen(ps);
	if(qslen > pslen && strcmp(qs + (qslen-pslen), ps) == 0 &&
		qslen + 2 >= pslen && /* space for label and dot */
		qs[qslen-pslen-1] == '.') {
		unsigned int slashcount = 0;
		size_t i = qslen-pslen-2;
		while(i>0 && qs[i]=='\\') {
			i++;
			slashcount++;
		}
		if(slashcount%1 == 1) return 0; /* . preceded by \ */
		return 1;
	}
	return 0;
}

/* finds entry in list, or returns NULL */
struct entry* 
find_match(struct entry* entries, uint8_t* query_pkt, size_t len,
	enum transport_type transport)
{
	struct entry* p = entries;
	uint8_t* reply;
	size_t rlen;
	for(p=entries; p; p=p->next) {
		verbose(3, "comparepkt: ");
		reply = p->reply_list->reply_pkt;
		rlen = p->reply_list->reply_len;
		if(p->match_opcode && get_opcode(query_pkt, len) != 
			get_opcode(reply, rlen)) {
			verbose(3, "bad opcode\n");
			continue;
		}
		if(p->match_qtype && get_qtype(query_pkt, len) !=
			get_qtype(reply, rlen)) {
			verbose(3, "bad qtype %d %d\n", get_qtype(query_pkt, len), get_qtype(reply, rlen));
			continue;
		}
		if(p->match_qname) {
			if(!equal_dname(query_pkt, len, reply, rlen)) {
				verbose(3, "bad qname\n");
				continue;
			}
		}
		if(p->match_subdomain) {
			if(!subdomain_dname(query_pkt, len, reply, rlen)) {
				verbose(3, "bad subdomain\n");
				continue;
			}
		}
		if(p->match_serial && get_serial(query_pkt, len) != p->ixfr_soa_serial) {
				verbose(3, "bad serial\n");
				continue;
		}
		if(p->match_do && !get_do_flag(query_pkt, len)) {
			verbose(3, "no DO bit set\n");
			continue;
		}
		if(p->match_noedns && get_has_edns(query_pkt, len)) {
			verbose(3, "bad; EDNS OPT present\n");
			continue;
		}
		if(p->match_transport != transport_any && p->match_transport != transport) {
			verbose(3, "bad transport\n");
			continue;
		}
		if(p->match_all && !match_all(query_pkt, len, reply, rlen,
			(int)p->match_ttl, 0)) {
			verbose(3, "bad allmatch\n");
			continue;
		}
		verbose(3, "match!\n");
		return p;
	}
	return NULL;
}

void
adjust_packet(struct entry* match, uint8_t** answer_pkt, size_t *answer_len,
	uint8_t* query_pkt, size_t query_len)
{
	uint8_t* orig = *answer_pkt;
	size_t origlen = *answer_len;
	uint8_t* res;
	size_t reslen;

	/* perform the copy; if possible; must be uncompressed */
	if(match->copy_query && origlen >= LDNS_HEADER_SIZE &&
		query_len >= LDNS_HEADER_SIZE && LDNS_QDCOUNT(query_pkt)!=0
		&& LDNS_QDCOUNT(orig)==0) {
		/* no qname in output packet, insert it */
		size_t dlen = get_qname_len(query_pkt, query_len);
		reslen = origlen + dlen + 4;
		res = (uint8_t*)malloc(reslen);
		if(!res) {
			verbose(1, "out of memory; send without adjust\n");
			return;
		}
		/* copy the header, query, remainder */
		memcpy(res, orig, LDNS_HEADER_SIZE);
		memmove(res+LDNS_HEADER_SIZE, query_pkt+LDNS_HEADER_SIZE,
			dlen+4);
		memmove(res+LDNS_HEADER_SIZE+dlen+4, orig+LDNS_HEADER_SIZE,
			reslen-(LDNS_HEADER_SIZE+dlen+4));
		/* set QDCOUNT */
		sldns_write_uint16(res+4, 1);
	} else if(match->copy_query && origlen >= LDNS_HEADER_SIZE &&
		query_len >= LDNS_HEADER_SIZE && LDNS_QDCOUNT(query_pkt)!=0
		&& get_qname_len(orig, origlen) == 0) {
		/* QDCOUNT(orig)!=0 but qlen == 0, therefore, an error */
		verbose(1, "error: malformed qname; send without adjust\n");
		res = memdup(orig, origlen);
		reslen = origlen;
	} else if(match->copy_query && origlen >= LDNS_HEADER_SIZE &&
		query_len >= LDNS_HEADER_SIZE && LDNS_QDCOUNT(query_pkt)!=0
		&& LDNS_QDCOUNT(orig)!=0) {
		/* in this case olen != 0 and QDCOUNT(orig)!=0 */
		/* copy query section */
		size_t dlen = get_qname_len(query_pkt, query_len);
		size_t olen = get_qname_len(orig, origlen);
		reslen = origlen + dlen - olen;
		res = (uint8_t*)malloc(reslen);
		if(!res) {
			verbose(1, "out of memory; send without adjust\n");
			return;
		}
		/* copy the header, query, remainder */
		memcpy(res, orig, LDNS_HEADER_SIZE);
		memmove(res+LDNS_HEADER_SIZE, query_pkt+LDNS_HEADER_SIZE,
			dlen+4);
		memmove(res+LDNS_HEADER_SIZE+dlen+4,
			orig+LDNS_HEADER_SIZE+olen+4,
			reslen-(LDNS_HEADER_SIZE+dlen+4));
	} else {
		res = memdup(orig, origlen);
		reslen = origlen;
	}
	if(!res) {
		verbose(1, "out of memory; send without adjust\n");
		return;
	}
	/* copy the ID */
	if(match->copy_id && reslen >= 2)
		res[1] = orig[1];
	if(match->copy_id && reslen >= 1)
		res[0] = orig[0];

	if(match->sleeptime > 0) {
		verbose(3, "sleeping for %d seconds\n", match->sleeptime);
#ifdef HAVE_SLEEP
		sleep(match->sleeptime);
#else
		Sleep(match->sleeptime * 1000);
#endif
	}
	*answer_pkt = res;
	*answer_len = reslen;
}

/*
 * Parses data buffer to a query, finds the correct answer 
 * and calls the given function for every packet to send.
 */
void
handle_query(uint8_t* inbuf, ssize_t inlen, struct entry* entries, int* count,
	enum transport_type transport, void (*sendfunc)(uint8_t*, size_t, void*),
	void* userdata, FILE* verbose_out)
{
	struct reply_packet *p;
	uint8_t *outbuf = NULL;
	size_t outlen = 0;
	struct entry* entry = NULL;

	verbose(1, "query %d: id %d: %s %d bytes: ", ++(*count),
		(int)(inlen>=2?LDNS_ID_WIRE(inbuf):0), 
		(transport==transport_tcp)?"TCP":"UDP", (int)inlen);
	if(verbose_out) {
		char* out = sldns_wire2str_pkt(inbuf, (size_t)inlen);
		printf("%s\n", out);
		free(out);
	}

	/* fill up answer packet */
	entry = find_match(entries, inbuf, (size_t)inlen, transport);
	if(!entry || !entry->reply_list) {
		verbose(1, "no answer packet for this query, no reply.\n");
		return;
	}
	for(p = entry->reply_list; p; p = p->next)
	{
		verbose(3, "Answer pkt:\n");
		if (p->reply_from_hex) {
			/* try to adjust the hex packet, if it can be
			 * parsed, we can use adjust rules. if not,
			 * send packet literally */
			/* still try to adjust ID if others fail */
			outlen = sldns_buffer_limit(p->reply_from_hex);
			outbuf = sldns_buffer_begin(p->reply_from_hex);
		} else {
			outbuf = p->reply_pkt;
			outlen = p->reply_len;
		}
		if(!outbuf) {
			verbose(1, "out of memory\n");
			return;
		}
		/* copies outbuf in memory allocation */
		adjust_packet(entry, &outbuf, &outlen, inbuf, (size_t)inlen);
		verbose(1, "Answer packet size: %u bytes.\n", (unsigned int)outlen);
		if(verbose_out) {
			char* out = sldns_wire2str_pkt(outbuf, outlen);
			printf("%s\n", out);
			free(out);
		}
		if(p->packet_sleep) {
			verbose(3, "sleeping for next packet %d secs\n", 
				p->packet_sleep);
#ifdef HAVE_SLEEP
			sleep(p->packet_sleep);
#else
			Sleep(p->packet_sleep * 1000);
#endif
			verbose(3, "wakeup for next packet "
				"(slept %d secs)\n", p->packet_sleep);
		}
		sendfunc(outbuf, outlen, userdata);
		free(outbuf);
		outbuf = NULL;
		outlen = 0;
	}
}

/** delete the list of reply packets */
void delete_replylist(struct reply_packet* replist)
{
	struct reply_packet *p=replist, *np;
	while(p) {
		np = p->next;
		free(p->reply_pkt);
		sldns_buffer_free(p->reply_from_hex);
		free(p);
		p=np;
	}
}

void delete_entry(struct entry* list)
{
	struct entry *p=list, *np;
	while(p) {
		np = p->next;
		delete_replylist(p->reply_list);
		free(p);
		p = np;
	}
}
