/*
 * checkconf/unbound-control.c - remote control utility for unbound.
 *
 * Copyright (c) 2008, NLnet Labs. All rights reserved.
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
 * The remote control utility contacts the unbound server over ssl and
 * sends the command, receives the answer, and displays the result
 * from the commandline.
 */

#include "config.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_OPENSSL_SSL_H
#include <openssl/ssl.h>
#endif
#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif
#ifdef HAVE_OPENSSL_RAND_H
#include <openssl/rand.h>
#endif
#include "util/log.h"
#include "util/config_file.h"
#include "util/locks.h"
#include "util/net_help.h"

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/** Give unbound-control usage, and exit (1). */
static void
usage()
{
	printf("Usage:	unbound-control [options] command\n");
	printf("	Remote control utility for unbound server.\n");
	printf("Options:\n");
	printf("  -c file	config file, default is %s\n", CONFIGFILE);
	printf("  -s ip[@port]	server address, if omitted config is used.\n");
	printf("  -q		quiet (don't print anything if it works ok).\n");
	printf("  -h		show this usage help.\n");
	printf("Commands:\n");
	printf("  start				start server; runs unbound(8)\n");
	printf("  stop				stops the server\n");
	printf("  reload			reloads the server\n");
	printf("  				(this flushes data, stats, requestlist)\n");
	printf("  stats				print statistics\n");
	printf("  stats_noreset			peek at statistics\n");
	printf("  status			display status of server\n");
	printf("  verbosity <number>		change logging detail\n");
	printf("  log_reopen			close and open the logfile\n");
	printf("  local_zone <name> <type>	add new local zone\n");
	printf("  local_zone_remove <name>	remove local zone and its contents\n");
	printf("  local_data <RR data...>	add local data, for example\n");
	printf("				local_data www.example.com A 192.0.2.1\n");
	printf("  local_data_remove <name>	remove local RR data from name\n");
	printf("  dump_cache			print cache to stdout\n");
	printf("  load_cache			load cache from stdin\n");
	printf("  lookup <name>			print nameservers for name\n");
	printf("  flush <name>			flushes common types for name from cache\n");
	printf("  				types:  A, AAAA, MX, PTR, NS,\n");
	printf("					SOA, CNAME, DNAME, SRV, NAPTR\n");
	printf("  flush_type <name> <type>	flush name, type from cache\n");
	printf("  flush_zone <name>		flush everything at or under name\n");
	printf("  				from rr and dnssec caches\n");
	printf("  flush_bogus			flush all bogus data\n");
	printf("  flush_negative		flush all negative data\n");
	printf("  flush_stats 			flush statistics, make zero\n");
	printf("  flush_requestlist 		drop queries that are worked on\n");
	printf("  dump_requestlist		show what is worked on\n");
	printf("  flush_infra [all | ip] 	remove ping, edns for one IP or all\n");
	printf("  dump_infra			show ping and edns entries\n");
	printf("  set_option opt: val		set option to value, no reload\n");
	printf("  get_option opt		get option value\n");
	printf("  list_stubs			list stub-zones and root hints in use\n");
	printf("  list_forwards			list forward-zones in use\n");
	printf("  list_local_zones		list local-zones in use\n");
	printf("  list_local_data		list local-data RRs in use\n");
	printf("  insecure_add zone 		add domain-insecure zone\n");
	printf("  insecure_remove zone		remove domain-insecure zone\n");
	printf("  forward_add [+i] zone addr..	add forward-zone with servers\n");
	printf("  forward_remove [+i] zone	remove forward zone\n");
	printf("  stub_add [+ip] zone addr..	add stub-zone with servers\n");
	printf("  stub_remove [+i] zone		remove stub zone\n");
	printf("		+i		also do dnssec insecure point\n");
	printf("		+p		set stub to use priming\n");
	printf("  forward [off | addr ...]	without arg show forward setup\n");
	printf("				or off to turn off root forwarding\n");
	printf("				or give list of ip addresses\n");
	printf("Version %s\n", PACKAGE_VERSION);
	printf("BSD licensed, see LICENSE in source package for details.\n");
	printf("Report bugs to %s\n", PACKAGE_BUGREPORT);
	exit(1);
}

/** exit with ssl error */
static void ssl_err(const char* s)
{
	fprintf(stderr, "error: %s\n", s);
	ERR_print_errors_fp(stderr);
	exit(1);
}

/** setup SSL context */
static SSL_CTX*
setup_ctx(struct config_file* cfg)
{
	char* s_cert=NULL, *c_key=NULL, *c_cert=NULL;
	SSL_CTX* ctx;

	if(cfg->remote_control_use_cert) {
		s_cert = fname_after_chroot(cfg->server_cert_file, cfg, 1);
		c_key = fname_after_chroot(cfg->control_key_file, cfg, 1);
		c_cert = fname_after_chroot(cfg->control_cert_file, cfg, 1);
		if(!s_cert || !c_key || !c_cert)
			fatal_exit("out of memory");
	}
        ctx = SSL_CTX_new(SSLv23_client_method());
	if(!ctx)
		ssl_err("could not allocate SSL_CTX pointer");
        if(!(SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2) & SSL_OP_NO_SSLv2))
		ssl_err("could not set SSL_OP_NO_SSLv2");
        if(cfg->remote_control_use_cert) {
		if(!(SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3) & SSL_OP_NO_SSLv3))
			ssl_err("could not set SSL_OP_NO_SSLv3");
		if(!SSL_CTX_use_certificate_file(ctx,c_cert,SSL_FILETYPE_PEM) ||
		    !SSL_CTX_use_PrivateKey_file(ctx,c_key,SSL_FILETYPE_PEM)
		    || !SSL_CTX_check_private_key(ctx))
			ssl_err("Error setting up SSL_CTX client key and cert");
		if (SSL_CTX_load_verify_locations(ctx, s_cert, NULL) != 1)
			ssl_err("Error setting up SSL_CTX verify, server cert");
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

		free(s_cert);
		free(c_key);
		free(c_cert);
	} else {
		/* Use ciphers that don't require authentication  */
		if(!SSL_CTX_set_cipher_list(ctx, "aNULL"))
			ssl_err("Error setting NULL cipher!");
	}
	return ctx;
}

/** contact the server with TCP connect */
static int
contact_server(const char* svr, struct config_file* cfg, int statuscmd)
{
	struct sockaddr_storage addr;
	socklen_t addrlen;
	int addrfamily = 0;
	int fd;
	/* use svr or the first config entry */
	if(!svr) {
		if(cfg->control_ifs)
			svr = cfg->control_ifs->str;
		else	svr = "127.0.0.1";
		/* config 0 addr (everything), means ask localhost */
		if(strcmp(svr, "0.0.0.0") == 0)
			svr = "127.0.0.1";
		else if(strcmp(svr, "::0") == 0 ||
			strcmp(svr, "0::0") == 0 ||
			strcmp(svr, "0::") == 0 ||
			strcmp(svr, "::") == 0)
			svr = "::1";
	}
	if(strchr(svr, '@')) {
		if(!extstrtoaddr(svr, &addr, &addrlen))
			fatal_exit("could not parse IP@port: %s", svr);
#ifdef HAVE_SYS_UN_H
	} else if(svr[0] == '/') {
		struct sockaddr_un* usock = (struct sockaddr_un *) &addr;
		usock->sun_family = AF_LOCAL;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
		usock->sun_len = (socklen_t)sizeof(usock);
#endif
		(void)strlcpy(usock->sun_path, svr, sizeof(usock->sun_path));
		addrlen = (socklen_t)sizeof(struct sockaddr_un);
		addrfamily = AF_LOCAL;
#endif
	} else {
		if(!ipstrtoaddr(svr, cfg->control_port, &addr, &addrlen))
			fatal_exit("could not parse IP: %s", svr);
	}

	if(addrfamily == 0)
		addrfamily = addr_is_ip6(&addr, addrlen)?AF_INET6:AF_INET;
	fd = socket(addrfamily, SOCK_STREAM, 0);
	if(fd == -1) {
#ifndef USE_WINSOCK
		fatal_exit("socket: %s", strerror(errno));
#else
		fatal_exit("socket: %s", wsa_strerror(WSAGetLastError()));
#endif
	}
	if(connect(fd, (struct sockaddr*)&addr, addrlen) < 0) {
#ifndef USE_WINSOCK
		log_err_addr("connect", strerror(errno), &addr, addrlen);
		if(errno == ECONNREFUSED && statuscmd) {
			printf("unbound is stopped\n");
			exit(3);
		}
#else
		log_err_addr("connect", wsa_strerror(WSAGetLastError()), &addr, addrlen);
		if(WSAGetLastError() == WSAECONNREFUSED && statuscmd) {
			printf("unbound is stopped\n");
			exit(3);
		}
#endif
		exit(1);
	}
	return fd;
}

/** setup SSL on the connection */
static SSL*
setup_ssl(SSL_CTX* ctx, int fd, struct config_file* cfg)
{
	SSL* ssl;
	X509* x;
	int r;

	ssl = SSL_new(ctx);
	if(!ssl)
		ssl_err("could not SSL_new");
	SSL_set_connect_state(ssl);
	(void)SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	if(!SSL_set_fd(ssl, fd))
		ssl_err("could not SSL_set_fd");
	while(1) {
		ERR_clear_error();
		if( (r=SSL_do_handshake(ssl)) == 1)
			break;
		r = SSL_get_error(ssl, r);
		if(r != SSL_ERROR_WANT_READ && r != SSL_ERROR_WANT_WRITE)
			ssl_err("SSL handshake failed");
		/* wants to be called again */
	}

	/* check authenticity of server */
	if(SSL_get_verify_result(ssl) != X509_V_OK)
		ssl_err("SSL verification failed");
	if(cfg->remote_control_use_cert) {
		x = SSL_get_peer_certificate(ssl);
		if(!x)
			ssl_err("Server presented no peer certificate");
		X509_free(x);
	}

	return ssl;
}

/** send stdin to server */
static void
send_file(SSL* ssl, FILE* in, char* buf, size_t sz)
{
	while(fgets(buf, (int)sz, in)) {
		if(SSL_write(ssl, buf, (int)strlen(buf)) <= 0)
			ssl_err("could not SSL_write contents");
	}
}

/** send command and display result */
static int
go_cmd(SSL* ssl, int quiet, int argc, char* argv[])
{
	char pre[10];
	const char* space=" ";
	const char* newline="\n";
	int was_error = 0, first_line = 1;
	int r, i;
	char buf[1024];
	snprintf(pre, sizeof(pre), "UBCT%d ", UNBOUND_CONTROL_VERSION);
	if(SSL_write(ssl, pre, (int)strlen(pre)) <= 0)
		ssl_err("could not SSL_write");
	for(i=0; i<argc; i++) {
		if(SSL_write(ssl, space, (int)strlen(space)) <= 0)
			ssl_err("could not SSL_write");
		if(SSL_write(ssl, argv[i], (int)strlen(argv[i])) <= 0)
			ssl_err("could not SSL_write");
	}
	if(SSL_write(ssl, newline, (int)strlen(newline)) <= 0)
		ssl_err("could not SSL_write");

	if(argc == 1 && strcmp(argv[0], "load_cache") == 0) {
		send_file(ssl, stdin, buf, sizeof(buf));
	}

	while(1) {
		ERR_clear_error();
		if((r = SSL_read(ssl, buf, (int)sizeof(buf)-1)) <= 0) {
			if(SSL_get_error(ssl, r) == SSL_ERROR_ZERO_RETURN) {
				/* EOF */
				break;
			}
			ssl_err("could not SSL_read");
		}
		buf[r] = 0;
		if(first_line && strncmp(buf, "error", 5) == 0) {
			printf("%s", buf);
			was_error = 1;
		} else if (!quiet)
			printf("%s", buf);

		first_line = 0;
	}
	return was_error;
}

/** go ahead and read config, contact server and perform command and display */
static int
go(const char* cfgfile, char* svr, int quiet, int argc, char* argv[])
{
	struct config_file* cfg;
	int fd, ret;
	SSL_CTX* ctx;
	SSL* ssl;

	/* read config */
	if(!(cfg = config_create()))
		fatal_exit("out of memory");
	if(!config_read(cfg, cfgfile, NULL))
		fatal_exit("could not read config file");
	if(!cfg->remote_control_enable)
		log_warn("control-enable is 'no' in the config file.");
	ctx = setup_ctx(cfg);

	/* contact server */
	fd = contact_server(svr, cfg, argc>0&&strcmp(argv[0],"status")==0);
	ssl = setup_ssl(ctx, fd, cfg);

	/* send command */
	ret = go_cmd(ssl, quiet, argc, argv);

	SSL_free(ssl);
#ifndef USE_WINSOCK
	close(fd);
#else
	closesocket(fd);
#endif
	SSL_CTX_free(ctx);
	config_delete(cfg);
	return ret;
}

/** getopt global, in case header files fail to declare it. */
extern int optind;
/** getopt global, in case header files fail to declare it. */
extern char* optarg;

/** Main routine for unbound-control */
int main(int argc, char* argv[])
{
	int c, ret;
	int quiet = 0;
	const char* cfgfile = CONFIGFILE;
	char* svr = NULL;
#ifdef USE_WINSOCK
	int r;
	WSADATA wsa_data;
#endif
#ifdef USE_THREAD_DEBUG
	/* stop the file output from unbound-control, overwites the servers */
	extern int check_locking_order;
	check_locking_order = 0;
#endif /* USE_THREAD_DEBUG */
	log_ident_set("unbound-control");
	log_init(NULL, 0, NULL);
	checklock_start();
#ifdef USE_WINSOCK
	if((r = WSAStartup(MAKEWORD(2,2), &wsa_data)) != 0)
		fatal_exit("WSAStartup failed: %s", wsa_strerror(r));
	/* use registry config file in preference to compiletime location */
	if(!(cfgfile=w_lookup_reg_str("Software\\Unbound", "ConfigFile")))
		cfgfile = CONFIGFILE;
#endif

	ERR_load_crypto_strings();
	ERR_load_SSL_strings();
	OpenSSL_add_all_algorithms();
	(void)SSL_library_init();

	if(!RAND_status()) {
                /* try to seed it */
                unsigned char buf[256];
                unsigned int seed=(unsigned)time(NULL) ^ (unsigned)getpid();
		unsigned int v = seed;
                size_t i;
                for(i=0; i<256/sizeof(v); i++) {
                        memmove(buf+i*sizeof(v), &v, sizeof(v));
                        v = v*seed + (unsigned int)i;
                }
                RAND_seed(buf, 256);
		log_warn("no entropy, seeding openssl PRNG with time\n");
	}

	/* parse the options */
	while( (c=getopt(argc, argv, "c:s:qh")) != -1) {
		switch(c) {
		case 'c':
			cfgfile = optarg;
			break;
		case 's':
			svr = optarg;
			break;
		case 'q':
			quiet = 1;
			break;
		case '?':
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if(argc == 0)
		usage();
	if(argc >= 1 && strcmp(argv[0], "start")==0) {
		if(execlp("unbound", "unbound", "-c", cfgfile, 
			(char*)NULL) < 0) {
			fatal_exit("could not exec unbound: %s",
				strerror(errno));
		}
	}

	ret = go(cfgfile, svr, quiet, argc, argv);

#ifdef USE_WINSOCK
        WSACleanup();
#endif
	checklock_stop();
	return ret;
}
