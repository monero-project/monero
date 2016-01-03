%{
/*
 * configlexer.lex - lexical analyzer for unbound config file
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved
 *
 * See LICENSE for the license.
 *
 */
#include <ctype.h>
#include <string.h>
#include <strings.h>
#ifdef HAVE_GLOB_H
# include <glob.h>
#endif

#include "util/config_file.h"
#include "util/configparser.h"
void ub_c_error(const char *message);

#if 0
#define LEXOUT(s)  printf s /* used ONLY when debugging */
#else
#define LEXOUT(s)
#endif

/** avoid warning in about fwrite return value */
#define ECHO ub_c_error_msg("syntax error at text: %s", yytext)

/** A parser variable, this is a statement in the config file which is
 * of the form variable: value1 value2 ...  nargs is the number of values. */
#define YDVAR(nargs, var) \
	num_args=(nargs); \
	LEXOUT(("v(%s%d) ", yytext, num_args)); \
	if(num_args > 0) { BEGIN(val); } \
	return (var);

struct inc_state {
	char* filename;
	int line;
	YY_BUFFER_STATE buffer;
	struct inc_state* next;
};
static struct inc_state* config_include_stack = NULL;
static int inc_depth = 0;
static int inc_prev = 0;
static int num_args = 0;

void init_cfg_parse(void)
{
	config_include_stack = NULL;
	inc_depth = 0;
	inc_prev = 0;
	num_args = 0;
}

static void config_start_include(const char* filename)
{
	FILE *input;
	struct inc_state* s;
	char* nm;
	if(inc_depth++ > 100000) {
		ub_c_error_msg("too many include files");
		return;
	}
	if(strlen(filename) == 0) {
		ub_c_error_msg("empty include file name");
		return;
	}
	s = (struct inc_state*)malloc(sizeof(*s));
	if(!s) {
		ub_c_error_msg("include %s: malloc failure", filename);
		return;
	}
	if(cfg_parser->chroot && strncmp(filename, cfg_parser->chroot,
		strlen(cfg_parser->chroot)) == 0) {
		filename += strlen(cfg_parser->chroot);
	}
	nm = strdup(filename);
	if(!nm) {
		ub_c_error_msg("include %s: strdup failure", filename);
		free(s);
		return;
	}
	input = fopen(filename, "r");
	if(!input) {
		ub_c_error_msg("cannot open include file '%s': %s",
			filename, strerror(errno));
		free(s);
		free(nm);
		return;
	}
	LEXOUT(("switch_to_include_file(%s)\n", filename));
	s->filename = cfg_parser->filename;
	s->line = cfg_parser->line;
	s->buffer = YY_CURRENT_BUFFER;
	s->next = config_include_stack;
	config_include_stack = s;
	cfg_parser->filename = nm;
	cfg_parser->line = 1;
	yy_switch_to_buffer(yy_create_buffer(input, YY_BUF_SIZE));
}

static void config_start_include_glob(const char* filename)
{

	/* check for wildcards */
#ifdef HAVE_GLOB
	glob_t g;
	size_t i;
	int r, flags;
	if(!(!strchr(filename, '*') && !strchr(filename, '?') && !strchr(filename, '[') &&
		!strchr(filename, '{') && !strchr(filename, '~'))) {
		flags = 0
#ifdef GLOB_ERR
			| GLOB_ERR
#endif
#ifdef GLOB_NOSORT
			| GLOB_NOSORT
#endif
#ifdef GLOB_BRACE
			| GLOB_BRACE
#endif
#ifdef GLOB_TILDE
			| GLOB_TILDE
#endif
		;
		memset(&g, 0, sizeof(g));
		if(cfg_parser->chroot && strncmp(filename, cfg_parser->chroot,
			strlen(cfg_parser->chroot)) == 0) {
			filename += strlen(cfg_parser->chroot);
		}
		r = glob(filename, flags, NULL, &g);
		if(r) {
			/* some error */
			globfree(&g);
			if(r == GLOB_NOMATCH)
				return; /* no matches for pattern */
			config_start_include(filename); /* let original deal with it */
			return;
		}
		/* process files found, if any */
		for(i=0; i<(size_t)g.gl_pathc; i++) {
			config_start_include(g.gl_pathv[i]);
		}
		globfree(&g);
		return;
	}
#endif /* HAVE_GLOB */

	config_start_include(filename);
}

static void config_end_include(void)
{
	struct inc_state* s = config_include_stack;
	--inc_depth;
	if(!s) return;
	free(cfg_parser->filename);
	cfg_parser->filename = s->filename;
	cfg_parser->line = s->line;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(s->buffer);
	config_include_stack = s->next;
	free(s);
}

#ifndef yy_set_bol /* compat definition, for flex 2.4.6 */
#define yy_set_bol(at_bol) \
        { \
	        if ( ! yy_current_buffer ) \
	                yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	        yy_current_buffer->yy_ch_buf[0] = ((at_bol)?'\n':' '); \
        }
#endif

%}
%option noinput
%option nounput
%{
#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT 1
#endif
#ifndef YY_NO_INPUT
#define YY_NO_INPUT 1
#endif
%}

SPACE   [ \t]
LETTER  [a-zA-Z]
UNQUOTEDLETTER [^\'\"\n\r \t\\]|\\.
UNQUOTEDLETTER_NOCOLON [^\:\'\"\n\r \t\\]|\\.
NEWLINE [\r\n]
COMMENT \#
COLON 	\:
DQANY     [^\"\n\r\\]|\\.
SQANY     [^\'\n\r\\]|\\.

%x	quotedstring singlequotedstr include include_quoted val

%%
<INITIAL,val>{SPACE}*	{ 
	LEXOUT(("SP ")); /* ignore */ }
<INITIAL,val>{SPACE}*{COMMENT}.*	{ 
	/* note that flex makes the longest match and '.' is any but not nl */
	LEXOUT(("comment(%s) ", yytext)); /* ignore */ }
server{COLON}			{ YDVAR(0, VAR_SERVER) }
qname-minimisation{COLON}	{ YDVAR(1, VAR_QNAME_MINIMISATION) }
num-threads{COLON}		{ YDVAR(1, VAR_NUM_THREADS) }
verbosity{COLON}		{ YDVAR(1, VAR_VERBOSITY) }
port{COLON}			{ YDVAR(1, VAR_PORT) }
outgoing-range{COLON}		{ YDVAR(1, VAR_OUTGOING_RANGE) }
outgoing-port-permit{COLON}	{ YDVAR(1, VAR_OUTGOING_PORT_PERMIT) }
outgoing-port-avoid{COLON}	{ YDVAR(1, VAR_OUTGOING_PORT_AVOID) }
outgoing-num-tcp{COLON}		{ YDVAR(1, VAR_OUTGOING_NUM_TCP) }
incoming-num-tcp{COLON}		{ YDVAR(1, VAR_INCOMING_NUM_TCP) }
do-ip4{COLON}			{ YDVAR(1, VAR_DO_IP4) }
do-ip6{COLON}			{ YDVAR(1, VAR_DO_IP6) }
do-udp{COLON}			{ YDVAR(1, VAR_DO_UDP) }
do-tcp{COLON}			{ YDVAR(1, VAR_DO_TCP) }
tcp-upstream{COLON}		{ YDVAR(1, VAR_TCP_UPSTREAM) }
ssl-upstream{COLON}		{ YDVAR(1, VAR_SSL_UPSTREAM) }
ssl-service-key{COLON}		{ YDVAR(1, VAR_SSL_SERVICE_KEY) }
ssl-service-pem{COLON}		{ YDVAR(1, VAR_SSL_SERVICE_PEM) }
ssl-port{COLON}			{ YDVAR(1, VAR_SSL_PORT) }
do-daemonize{COLON}		{ YDVAR(1, VAR_DO_DAEMONIZE) }
interface{COLON}		{ YDVAR(1, VAR_INTERFACE) }
ip-address{COLON}		{ YDVAR(1, VAR_INTERFACE) }
outgoing-interface{COLON}	{ YDVAR(1, VAR_OUTGOING_INTERFACE) }
interface-automatic{COLON}	{ YDVAR(1, VAR_INTERFACE_AUTOMATIC) }
so-rcvbuf{COLON}		{ YDVAR(1, VAR_SO_RCVBUF) }
so-sndbuf{COLON}		{ YDVAR(1, VAR_SO_SNDBUF) }
so-reuseport{COLON}		{ YDVAR(1, VAR_SO_REUSEPORT) }
ip-transparent{COLON}		{ YDVAR(1, VAR_IP_TRANSPARENT) }
chroot{COLON}			{ YDVAR(1, VAR_CHROOT) }
username{COLON}			{ YDVAR(1, VAR_USERNAME) }
directory{COLON}		{ YDVAR(1, VAR_DIRECTORY) }
logfile{COLON}			{ YDVAR(1, VAR_LOGFILE) }
pidfile{COLON}			{ YDVAR(1, VAR_PIDFILE) }
root-hints{COLON}		{ YDVAR(1, VAR_ROOT_HINTS) }
edns-buffer-size{COLON}		{ YDVAR(1, VAR_EDNS_BUFFER_SIZE) }
msg-buffer-size{COLON}		{ YDVAR(1, VAR_MSG_BUFFER_SIZE) }
msg-cache-size{COLON}		{ YDVAR(1, VAR_MSG_CACHE_SIZE) }
msg-cache-slabs{COLON}		{ YDVAR(1, VAR_MSG_CACHE_SLABS) }
rrset-cache-size{COLON}		{ YDVAR(1, VAR_RRSET_CACHE_SIZE) }
rrset-cache-slabs{COLON}	{ YDVAR(1, VAR_RRSET_CACHE_SLABS) }
cache-max-ttl{COLON}     	{ YDVAR(1, VAR_CACHE_MAX_TTL) }
cache-max-negative-ttl{COLON}   { YDVAR(1, VAR_CACHE_MAX_NEGATIVE_TTL) }
cache-min-ttl{COLON}     	{ YDVAR(1, VAR_CACHE_MIN_TTL) }
infra-host-ttl{COLON}		{ YDVAR(1, VAR_INFRA_HOST_TTL) }
infra-lame-ttl{COLON}		{ YDVAR(1, VAR_INFRA_LAME_TTL) }
infra-cache-slabs{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_SLABS) }
infra-cache-numhosts{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_NUMHOSTS) }
infra-cache-lame-size{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_LAME_SIZE) }
infra-cache-min-rtt{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_MIN_RTT) }
num-queries-per-thread{COLON}	{ YDVAR(1, VAR_NUM_QUERIES_PER_THREAD) }
jostle-timeout{COLON}		{ YDVAR(1, VAR_JOSTLE_TIMEOUT) }
delay-close{COLON}		{ YDVAR(1, VAR_DELAY_CLOSE) }
target-fetch-policy{COLON}	{ YDVAR(1, VAR_TARGET_FETCH_POLICY) }
harden-short-bufsize{COLON}	{ YDVAR(1, VAR_HARDEN_SHORT_BUFSIZE) }
harden-large-queries{COLON}	{ YDVAR(1, VAR_HARDEN_LARGE_QUERIES) }
harden-glue{COLON}		{ YDVAR(1, VAR_HARDEN_GLUE) }
harden-dnssec-stripped{COLON}	{ YDVAR(1, VAR_HARDEN_DNSSEC_STRIPPED) }
harden-below-nxdomain{COLON}	{ YDVAR(1, VAR_HARDEN_BELOW_NXDOMAIN) }
harden-referral-path{COLON}	{ YDVAR(1, VAR_HARDEN_REFERRAL_PATH) }
harden-algo-downgrade{COLON}	{ YDVAR(1, VAR_HARDEN_ALGO_DOWNGRADE) }
use-caps-for-id{COLON}		{ YDVAR(1, VAR_USE_CAPS_FOR_ID) }
caps-whitelist{COLON}		{ YDVAR(1, VAR_CAPS_WHITELIST) }
unwanted-reply-threshold{COLON}	{ YDVAR(1, VAR_UNWANTED_REPLY_THRESHOLD) }
private-address{COLON}		{ YDVAR(1, VAR_PRIVATE_ADDRESS) }
private-domain{COLON}		{ YDVAR(1, VAR_PRIVATE_DOMAIN) }
prefetch-key{COLON}		{ YDVAR(1, VAR_PREFETCH_KEY) }
prefetch{COLON}			{ YDVAR(1, VAR_PREFETCH) }
stub-zone{COLON}		{ YDVAR(0, VAR_STUB_ZONE) }
name{COLON}			{ YDVAR(1, VAR_NAME) }
stub-addr{COLON}		{ YDVAR(1, VAR_STUB_ADDR) }
stub-host{COLON}		{ YDVAR(1, VAR_STUB_HOST) }
stub-prime{COLON}		{ YDVAR(1, VAR_STUB_PRIME) }
stub-first{COLON}		{ YDVAR(1, VAR_STUB_FIRST) }
forward-zone{COLON}		{ YDVAR(0, VAR_FORWARD_ZONE) }
forward-addr{COLON}		{ YDVAR(1, VAR_FORWARD_ADDR) }
forward-host{COLON}		{ YDVAR(1, VAR_FORWARD_HOST) }
forward-first{COLON}		{ YDVAR(1, VAR_FORWARD_FIRST) }
do-not-query-address{COLON}	{ YDVAR(1, VAR_DO_NOT_QUERY_ADDRESS) }
do-not-query-localhost{COLON}	{ YDVAR(1, VAR_DO_NOT_QUERY_LOCALHOST) }
access-control{COLON}		{ YDVAR(2, VAR_ACCESS_CONTROL) }
hide-identity{COLON}		{ YDVAR(1, VAR_HIDE_IDENTITY) }
hide-version{COLON}		{ YDVAR(1, VAR_HIDE_VERSION) }
identity{COLON}			{ YDVAR(1, VAR_IDENTITY) }
version{COLON}			{ YDVAR(1, VAR_VERSION) }
module-config{COLON}     	{ YDVAR(1, VAR_MODULE_CONF) }
dlv-anchor{COLON}		{ YDVAR(1, VAR_DLV_ANCHOR) }
dlv-anchor-file{COLON}		{ YDVAR(1, VAR_DLV_ANCHOR_FILE) }
trust-anchor-file{COLON}	{ YDVAR(1, VAR_TRUST_ANCHOR_FILE) }
auto-trust-anchor-file{COLON}	{ YDVAR(1, VAR_AUTO_TRUST_ANCHOR_FILE) }
trusted-keys-file{COLON}	{ YDVAR(1, VAR_TRUSTED_KEYS_FILE) }
trust-anchor{COLON}		{ YDVAR(1, VAR_TRUST_ANCHOR) }
val-override-date{COLON}	{ YDVAR(1, VAR_VAL_OVERRIDE_DATE) }
val-sig-skew-min{COLON}		{ YDVAR(1, VAR_VAL_SIG_SKEW_MIN) }
val-sig-skew-max{COLON}		{ YDVAR(1, VAR_VAL_SIG_SKEW_MAX) }
val-bogus-ttl{COLON}		{ YDVAR(1, VAR_BOGUS_TTL) }
val-clean-additional{COLON}	{ YDVAR(1, VAR_VAL_CLEAN_ADDITIONAL) }
val-permissive-mode{COLON}	{ YDVAR(1, VAR_VAL_PERMISSIVE_MODE) }
ignore-cd-flag{COLON}		{ YDVAR(1, VAR_IGNORE_CD_FLAG) }
val-log-level{COLON}		{ YDVAR(1, VAR_VAL_LOG_LEVEL) }
key-cache-size{COLON}		{ YDVAR(1, VAR_KEY_CACHE_SIZE) }
key-cache-slabs{COLON}		{ YDVAR(1, VAR_KEY_CACHE_SLABS) }
neg-cache-size{COLON}		{ YDVAR(1, VAR_NEG_CACHE_SIZE) }
val-nsec3-keysize-iterations{COLON}	{ 
				  YDVAR(1, VAR_VAL_NSEC3_KEYSIZE_ITERATIONS) }
add-holddown{COLON}		{ YDVAR(1, VAR_ADD_HOLDDOWN) }
del-holddown{COLON}		{ YDVAR(1, VAR_DEL_HOLDDOWN) }
keep-missing{COLON}		{ YDVAR(1, VAR_KEEP_MISSING) }
permit-small-holddown{COLON}	{ YDVAR(1, VAR_PERMIT_SMALL_HOLDDOWN) }
use-syslog{COLON}		{ YDVAR(1, VAR_USE_SYSLOG) }
log-time-ascii{COLON}		{ YDVAR(1, VAR_LOG_TIME_ASCII) }
log-queries{COLON}		{ YDVAR(1, VAR_LOG_QUERIES) }
local-zone{COLON}		{ YDVAR(2, VAR_LOCAL_ZONE) }
local-data{COLON}		{ YDVAR(1, VAR_LOCAL_DATA) }
local-data-ptr{COLON}		{ YDVAR(1, VAR_LOCAL_DATA_PTR) }
unblock-lan-zones{COLON}	{ YDVAR(1, VAR_UNBLOCK_LAN_ZONES) }
statistics-interval{COLON}	{ YDVAR(1, VAR_STATISTICS_INTERVAL) }
statistics-cumulative{COLON}	{ YDVAR(1, VAR_STATISTICS_CUMULATIVE) }
extended-statistics{COLON}	{ YDVAR(1, VAR_EXTENDED_STATISTICS) }
remote-control{COLON}		{ YDVAR(0, VAR_REMOTE_CONTROL) }
control-enable{COLON}		{ YDVAR(1, VAR_CONTROL_ENABLE) }
control-interface{COLON}	{ YDVAR(1, VAR_CONTROL_INTERFACE) }
control-port{COLON}		{ YDVAR(1, VAR_CONTROL_PORT) }
control-use-cert{COLON}		{ YDVAR(1, VAR_CONTROL_USE_CERT) }
server-key-file{COLON}		{ YDVAR(1, VAR_SERVER_KEY_FILE) }
server-cert-file{COLON}		{ YDVAR(1, VAR_SERVER_CERT_FILE) }
control-key-file{COLON}		{ YDVAR(1, VAR_CONTROL_KEY_FILE) }
control-cert-file{COLON}	{ YDVAR(1, VAR_CONTROL_CERT_FILE) }
python-script{COLON}		{ YDVAR(1, VAR_PYTHON_SCRIPT) }
python{COLON}			{ YDVAR(0, VAR_PYTHON) }
domain-insecure{COLON}		{ YDVAR(1, VAR_DOMAIN_INSECURE) }
minimal-responses{COLON}	{ YDVAR(1, VAR_MINIMAL_RESPONSES) }
rrset-roundrobin{COLON}		{ YDVAR(1, VAR_RRSET_ROUNDROBIN) }
max-udp-size{COLON}		{ YDVAR(1, VAR_MAX_UDP_SIZE) }
dns64-prefix{COLON}		{ YDVAR(1, VAR_DNS64_PREFIX) }
dns64-synthall{COLON}		{ YDVAR(1, VAR_DNS64_SYNTHALL) }
dnstap{COLON}			{ YDVAR(0, VAR_DNSTAP) }
dnstap-enable{COLON}		{ YDVAR(1, VAR_DNSTAP_ENABLE) }
dnstap-socket-path{COLON}	{ YDVAR(1, VAR_DNSTAP_SOCKET_PATH) }
dnstap-send-identity{COLON}	{ YDVAR(1, VAR_DNSTAP_SEND_IDENTITY) }
dnstap-send-version{COLON}	{ YDVAR(1, VAR_DNSTAP_SEND_VERSION) }
dnstap-identity{COLON}		{ YDVAR(1, VAR_DNSTAP_IDENTITY) }
dnstap-version{COLON}		{ YDVAR(1, VAR_DNSTAP_VERSION) }
dnstap-log-resolver-query-messages{COLON}	{
		YDVAR(1, VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES) }
dnstap-log-resolver-response-messages{COLON}	{
		YDVAR(1, VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES) }
dnstap-log-client-query-messages{COLON}		{
		YDVAR(1, VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES) }
dnstap-log-client-response-messages{COLON}	{
		YDVAR(1, VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES) }
dnstap-log-forwarder-query-messages{COLON}	{
		YDVAR(1, VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES) }
dnstap-log-forwarder-response-messages{COLON}	{
		YDVAR(1, VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES) }
ratelimit{COLON}		{ YDVAR(1, VAR_RATELIMIT) }
ratelimit-slabs{COLON}		{ YDVAR(1, VAR_RATELIMIT_SLABS) }
ratelimit-size{COLON}		{ YDVAR(1, VAR_RATELIMIT_SIZE) }
ratelimit-for-domain{COLON}	{ YDVAR(2, VAR_RATELIMIT_FOR_DOMAIN) }
ratelimit-below-domain{COLON}	{ YDVAR(2, VAR_RATELIMIT_BELOW_DOMAIN) }
ratelimit-factor{COLON}		{ YDVAR(1, VAR_RATELIMIT_FACTOR) }
<INITIAL,val>{NEWLINE}		{ LEXOUT(("NL\n")); cfg_parser->line++; }

	/* Quoted strings. Strip leading and ending quotes */
<val>\"			{ BEGIN(quotedstring); LEXOUT(("QS ")); }
<quotedstring><<EOF>>   {
        yyerror("EOF inside quoted string");
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
}
<quotedstring>{DQANY}*  { LEXOUT(("STR(%s) ", yytext)); yymore(); }
<quotedstring>{NEWLINE} { yyerror("newline inside quoted string, no end \""); 
			  cfg_parser->line++; BEGIN(INITIAL); }
<quotedstring>\" {
        LEXOUT(("QE "));
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
        yytext[yyleng - 1] = '\0';
	yylval.str = strdup(yytext);
	if(!yylval.str)
		yyerror("out of memory");
        return STRING_ARG;
}

	/* Single Quoted strings. Strip leading and ending quotes */
<val>\'			{ BEGIN(singlequotedstr); LEXOUT(("SQS ")); }
<singlequotedstr><<EOF>>   {
        yyerror("EOF inside quoted string");
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
}
<singlequotedstr>{SQANY}*  { LEXOUT(("STR(%s) ", yytext)); yymore(); }
<singlequotedstr>{NEWLINE} { yyerror("newline inside quoted string, no end '"); 
			     cfg_parser->line++; BEGIN(INITIAL); }
<singlequotedstr>\' {
        LEXOUT(("SQE "));
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
        yytext[yyleng - 1] = '\0';
	yylval.str = strdup(yytext);
	if(!yylval.str)
		yyerror("out of memory");
        return STRING_ARG;
}

	/* include: directive */
<INITIAL,val>include{COLON}	{ 
	LEXOUT(("v(%s) ", yytext)); inc_prev = YYSTATE; BEGIN(include); }
<include><<EOF>>	{
        yyerror("EOF inside include directive");
        BEGIN(inc_prev);
}
<include>{SPACE}*	{ LEXOUT(("ISP ")); /* ignore */ }
<include>{NEWLINE}	{ LEXOUT(("NL\n")); cfg_parser->line++;}
<include>\"		{ LEXOUT(("IQS ")); BEGIN(include_quoted); }
<include>{UNQUOTEDLETTER}*	{
	LEXOUT(("Iunquotedstr(%s) ", yytext));
	config_start_include_glob(yytext);
	BEGIN(inc_prev);
}
<include_quoted><<EOF>>	{
        yyerror("EOF inside quoted string");
        BEGIN(inc_prev);
}
<include_quoted>{DQANY}*	{ LEXOUT(("ISTR(%s) ", yytext)); yymore(); }
<include_quoted>{NEWLINE}	{ yyerror("newline before \" in include name"); 
				  cfg_parser->line++; BEGIN(inc_prev); }
<include_quoted>\"	{
	LEXOUT(("IQE "));
	yytext[yyleng - 1] = '\0';
	config_start_include_glob(yytext);
	BEGIN(inc_prev);
}
<INITIAL,val><<EOF>>	{
	LEXOUT(("LEXEOF "));
	yy_set_bol(1); /* Set beginning of line, so "^" rules match.  */
	if (!config_include_stack) {
		yyterminate();
	} else {
		fclose(yyin);
		config_end_include();
	}
}

<val>{UNQUOTEDLETTER}*	{ LEXOUT(("unquotedstr(%s) ", yytext)); 
			if(--num_args == 0) { BEGIN(INITIAL); }
			yylval.str = strdup(yytext); return STRING_ARG; }

{UNQUOTEDLETTER_NOCOLON}*	{
	ub_c_error_msg("unknown keyword '%s'", yytext);
	}

<*>.	{
	ub_c_error_msg("stray '%s'", yytext);
	}

%%
