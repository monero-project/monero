/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 38 "./util/configparser.y"

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util/configyyrename.h"
#include "util/config_file.h"
#include "util/net_help.h"

int ub_c_lex(void);
void ub_c_error(const char *message);

/* these need to be global, otherwise they cannot be used inside yacc */
extern struct config_parser_state* cfg_parser;

#if 0
#define OUTYY(s)  printf s /* used ONLY when debugging */
#else
#define OUTYY(s)
#endif


/* Line 371 of yacc.c  */
#line 95 "util/configparser.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "configparser.h".  */
#ifndef YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
# define YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SPACE = 258,
     LETTER = 259,
     NEWLINE = 260,
     COMMENT = 261,
     COLON = 262,
     ANY = 263,
     ZONESTR = 264,
     STRING_ARG = 265,
     VAR_SERVER = 266,
     VAR_VERBOSITY = 267,
     VAR_NUM_THREADS = 268,
     VAR_PORT = 269,
     VAR_OUTGOING_RANGE = 270,
     VAR_INTERFACE = 271,
     VAR_DO_IP4 = 272,
     VAR_DO_IP6 = 273,
     VAR_DO_UDP = 274,
     VAR_DO_TCP = 275,
     VAR_CHROOT = 276,
     VAR_USERNAME = 277,
     VAR_DIRECTORY = 278,
     VAR_LOGFILE = 279,
     VAR_PIDFILE = 280,
     VAR_MSG_CACHE_SIZE = 281,
     VAR_MSG_CACHE_SLABS = 282,
     VAR_NUM_QUERIES_PER_THREAD = 283,
     VAR_RRSET_CACHE_SIZE = 284,
     VAR_RRSET_CACHE_SLABS = 285,
     VAR_OUTGOING_NUM_TCP = 286,
     VAR_INFRA_HOST_TTL = 287,
     VAR_INFRA_LAME_TTL = 288,
     VAR_INFRA_CACHE_SLABS = 289,
     VAR_INFRA_CACHE_NUMHOSTS = 290,
     VAR_INFRA_CACHE_LAME_SIZE = 291,
     VAR_NAME = 292,
     VAR_STUB_ZONE = 293,
     VAR_STUB_HOST = 294,
     VAR_STUB_ADDR = 295,
     VAR_TARGET_FETCH_POLICY = 296,
     VAR_HARDEN_SHORT_BUFSIZE = 297,
     VAR_HARDEN_LARGE_QUERIES = 298,
     VAR_FORWARD_ZONE = 299,
     VAR_FORWARD_HOST = 300,
     VAR_FORWARD_ADDR = 301,
     VAR_DO_NOT_QUERY_ADDRESS = 302,
     VAR_HIDE_IDENTITY = 303,
     VAR_HIDE_VERSION = 304,
     VAR_IDENTITY = 305,
     VAR_VERSION = 306,
     VAR_HARDEN_GLUE = 307,
     VAR_MODULE_CONF = 308,
     VAR_TRUST_ANCHOR_FILE = 309,
     VAR_TRUST_ANCHOR = 310,
     VAR_VAL_OVERRIDE_DATE = 311,
     VAR_BOGUS_TTL = 312,
     VAR_VAL_CLEAN_ADDITIONAL = 313,
     VAR_VAL_PERMISSIVE_MODE = 314,
     VAR_INCOMING_NUM_TCP = 315,
     VAR_MSG_BUFFER_SIZE = 316,
     VAR_KEY_CACHE_SIZE = 317,
     VAR_KEY_CACHE_SLABS = 318,
     VAR_TRUSTED_KEYS_FILE = 319,
     VAR_VAL_NSEC3_KEYSIZE_ITERATIONS = 320,
     VAR_USE_SYSLOG = 321,
     VAR_OUTGOING_INTERFACE = 322,
     VAR_ROOT_HINTS = 323,
     VAR_DO_NOT_QUERY_LOCALHOST = 324,
     VAR_CACHE_MAX_TTL = 325,
     VAR_HARDEN_DNSSEC_STRIPPED = 326,
     VAR_ACCESS_CONTROL = 327,
     VAR_LOCAL_ZONE = 328,
     VAR_LOCAL_DATA = 329,
     VAR_INTERFACE_AUTOMATIC = 330,
     VAR_STATISTICS_INTERVAL = 331,
     VAR_DO_DAEMONIZE = 332,
     VAR_USE_CAPS_FOR_ID = 333,
     VAR_STATISTICS_CUMULATIVE = 334,
     VAR_OUTGOING_PORT_PERMIT = 335,
     VAR_OUTGOING_PORT_AVOID = 336,
     VAR_DLV_ANCHOR_FILE = 337,
     VAR_DLV_ANCHOR = 338,
     VAR_NEG_CACHE_SIZE = 339,
     VAR_HARDEN_REFERRAL_PATH = 340,
     VAR_PRIVATE_ADDRESS = 341,
     VAR_PRIVATE_DOMAIN = 342,
     VAR_REMOTE_CONTROL = 343,
     VAR_CONTROL_ENABLE = 344,
     VAR_CONTROL_INTERFACE = 345,
     VAR_CONTROL_PORT = 346,
     VAR_SERVER_KEY_FILE = 347,
     VAR_SERVER_CERT_FILE = 348,
     VAR_CONTROL_KEY_FILE = 349,
     VAR_CONTROL_CERT_FILE = 350,
     VAR_CONTROL_USE_CERT = 351,
     VAR_EXTENDED_STATISTICS = 352,
     VAR_LOCAL_DATA_PTR = 353,
     VAR_JOSTLE_TIMEOUT = 354,
     VAR_STUB_PRIME = 355,
     VAR_UNWANTED_REPLY_THRESHOLD = 356,
     VAR_LOG_TIME_ASCII = 357,
     VAR_DOMAIN_INSECURE = 358,
     VAR_PYTHON = 359,
     VAR_PYTHON_SCRIPT = 360,
     VAR_VAL_SIG_SKEW_MIN = 361,
     VAR_VAL_SIG_SKEW_MAX = 362,
     VAR_CACHE_MIN_TTL = 363,
     VAR_VAL_LOG_LEVEL = 364,
     VAR_AUTO_TRUST_ANCHOR_FILE = 365,
     VAR_KEEP_MISSING = 366,
     VAR_ADD_HOLDDOWN = 367,
     VAR_DEL_HOLDDOWN = 368,
     VAR_SO_RCVBUF = 369,
     VAR_EDNS_BUFFER_SIZE = 370,
     VAR_PREFETCH = 371,
     VAR_PREFETCH_KEY = 372,
     VAR_SO_SNDBUF = 373,
     VAR_SO_REUSEPORT = 374,
     VAR_HARDEN_BELOW_NXDOMAIN = 375,
     VAR_IGNORE_CD_FLAG = 376,
     VAR_LOG_QUERIES = 377,
     VAR_TCP_UPSTREAM = 378,
     VAR_SSL_UPSTREAM = 379,
     VAR_SSL_SERVICE_KEY = 380,
     VAR_SSL_SERVICE_PEM = 381,
     VAR_SSL_PORT = 382,
     VAR_FORWARD_FIRST = 383,
     VAR_STUB_FIRST = 384,
     VAR_MINIMAL_RESPONSES = 385,
     VAR_RRSET_ROUNDROBIN = 386,
     VAR_MAX_UDP_SIZE = 387,
     VAR_DELAY_CLOSE = 388,
     VAR_UNBLOCK_LAN_ZONES = 389,
     VAR_INFRA_CACHE_MIN_RTT = 390,
     VAR_DNS64_PREFIX = 391,
     VAR_DNS64_SYNTHALL = 392,
     VAR_DNSTAP = 393,
     VAR_DNSTAP_ENABLE = 394,
     VAR_DNSTAP_SOCKET_PATH = 395,
     VAR_DNSTAP_SEND_IDENTITY = 396,
     VAR_DNSTAP_SEND_VERSION = 397,
     VAR_DNSTAP_IDENTITY = 398,
     VAR_DNSTAP_VERSION = 399,
     VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES = 400,
     VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES = 401,
     VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES = 402,
     VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES = 403,
     VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES = 404,
     VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES = 405,
     VAR_HARDEN_ALGO_DOWNGRADE = 406,
     VAR_IP_TRANSPARENT = 407,
     VAR_RATELIMIT = 408,
     VAR_RATELIMIT_SLABS = 409,
     VAR_RATELIMIT_SIZE = 410,
     VAR_RATELIMIT_FOR_DOMAIN = 411,
     VAR_RATELIMIT_BELOW_DOMAIN = 412,
     VAR_RATELIMIT_FACTOR = 413,
     VAR_CAPS_WHITELIST = 414,
     VAR_CACHE_MAX_NEGATIVE_TTL = 415,
     VAR_PERMIT_SMALL_HOLDDOWN = 416,
     VAR_QNAME_MINIMISATION = 417
   };
#endif
/* Tokens.  */
#define SPACE 258
#define LETTER 259
#define NEWLINE 260
#define COMMENT 261
#define COLON 262
#define ANY 263
#define ZONESTR 264
#define STRING_ARG 265
#define VAR_SERVER 266
#define VAR_VERBOSITY 267
#define VAR_NUM_THREADS 268
#define VAR_PORT 269
#define VAR_OUTGOING_RANGE 270
#define VAR_INTERFACE 271
#define VAR_DO_IP4 272
#define VAR_DO_IP6 273
#define VAR_DO_UDP 274
#define VAR_DO_TCP 275
#define VAR_CHROOT 276
#define VAR_USERNAME 277
#define VAR_DIRECTORY 278
#define VAR_LOGFILE 279
#define VAR_PIDFILE 280
#define VAR_MSG_CACHE_SIZE 281
#define VAR_MSG_CACHE_SLABS 282
#define VAR_NUM_QUERIES_PER_THREAD 283
#define VAR_RRSET_CACHE_SIZE 284
#define VAR_RRSET_CACHE_SLABS 285
#define VAR_OUTGOING_NUM_TCP 286
#define VAR_INFRA_HOST_TTL 287
#define VAR_INFRA_LAME_TTL 288
#define VAR_INFRA_CACHE_SLABS 289
#define VAR_INFRA_CACHE_NUMHOSTS 290
#define VAR_INFRA_CACHE_LAME_SIZE 291
#define VAR_NAME 292
#define VAR_STUB_ZONE 293
#define VAR_STUB_HOST 294
#define VAR_STUB_ADDR 295
#define VAR_TARGET_FETCH_POLICY 296
#define VAR_HARDEN_SHORT_BUFSIZE 297
#define VAR_HARDEN_LARGE_QUERIES 298
#define VAR_FORWARD_ZONE 299
#define VAR_FORWARD_HOST 300
#define VAR_FORWARD_ADDR 301
#define VAR_DO_NOT_QUERY_ADDRESS 302
#define VAR_HIDE_IDENTITY 303
#define VAR_HIDE_VERSION 304
#define VAR_IDENTITY 305
#define VAR_VERSION 306
#define VAR_HARDEN_GLUE 307
#define VAR_MODULE_CONF 308
#define VAR_TRUST_ANCHOR_FILE 309
#define VAR_TRUST_ANCHOR 310
#define VAR_VAL_OVERRIDE_DATE 311
#define VAR_BOGUS_TTL 312
#define VAR_VAL_CLEAN_ADDITIONAL 313
#define VAR_VAL_PERMISSIVE_MODE 314
#define VAR_INCOMING_NUM_TCP 315
#define VAR_MSG_BUFFER_SIZE 316
#define VAR_KEY_CACHE_SIZE 317
#define VAR_KEY_CACHE_SLABS 318
#define VAR_TRUSTED_KEYS_FILE 319
#define VAR_VAL_NSEC3_KEYSIZE_ITERATIONS 320
#define VAR_USE_SYSLOG 321
#define VAR_OUTGOING_INTERFACE 322
#define VAR_ROOT_HINTS 323
#define VAR_DO_NOT_QUERY_LOCALHOST 324
#define VAR_CACHE_MAX_TTL 325
#define VAR_HARDEN_DNSSEC_STRIPPED 326
#define VAR_ACCESS_CONTROL 327
#define VAR_LOCAL_ZONE 328
#define VAR_LOCAL_DATA 329
#define VAR_INTERFACE_AUTOMATIC 330
#define VAR_STATISTICS_INTERVAL 331
#define VAR_DO_DAEMONIZE 332
#define VAR_USE_CAPS_FOR_ID 333
#define VAR_STATISTICS_CUMULATIVE 334
#define VAR_OUTGOING_PORT_PERMIT 335
#define VAR_OUTGOING_PORT_AVOID 336
#define VAR_DLV_ANCHOR_FILE 337
#define VAR_DLV_ANCHOR 338
#define VAR_NEG_CACHE_SIZE 339
#define VAR_HARDEN_REFERRAL_PATH 340
#define VAR_PRIVATE_ADDRESS 341
#define VAR_PRIVATE_DOMAIN 342
#define VAR_REMOTE_CONTROL 343
#define VAR_CONTROL_ENABLE 344
#define VAR_CONTROL_INTERFACE 345
#define VAR_CONTROL_PORT 346
#define VAR_SERVER_KEY_FILE 347
#define VAR_SERVER_CERT_FILE 348
#define VAR_CONTROL_KEY_FILE 349
#define VAR_CONTROL_CERT_FILE 350
#define VAR_CONTROL_USE_CERT 351
#define VAR_EXTENDED_STATISTICS 352
#define VAR_LOCAL_DATA_PTR 353
#define VAR_JOSTLE_TIMEOUT 354
#define VAR_STUB_PRIME 355
#define VAR_UNWANTED_REPLY_THRESHOLD 356
#define VAR_LOG_TIME_ASCII 357
#define VAR_DOMAIN_INSECURE 358
#define VAR_PYTHON 359
#define VAR_PYTHON_SCRIPT 360
#define VAR_VAL_SIG_SKEW_MIN 361
#define VAR_VAL_SIG_SKEW_MAX 362
#define VAR_CACHE_MIN_TTL 363
#define VAR_VAL_LOG_LEVEL 364
#define VAR_AUTO_TRUST_ANCHOR_FILE 365
#define VAR_KEEP_MISSING 366
#define VAR_ADD_HOLDDOWN 367
#define VAR_DEL_HOLDDOWN 368
#define VAR_SO_RCVBUF 369
#define VAR_EDNS_BUFFER_SIZE 370
#define VAR_PREFETCH 371
#define VAR_PREFETCH_KEY 372
#define VAR_SO_SNDBUF 373
#define VAR_SO_REUSEPORT 374
#define VAR_HARDEN_BELOW_NXDOMAIN 375
#define VAR_IGNORE_CD_FLAG 376
#define VAR_LOG_QUERIES 377
#define VAR_TCP_UPSTREAM 378
#define VAR_SSL_UPSTREAM 379
#define VAR_SSL_SERVICE_KEY 380
#define VAR_SSL_SERVICE_PEM 381
#define VAR_SSL_PORT 382
#define VAR_FORWARD_FIRST 383
#define VAR_STUB_FIRST 384
#define VAR_MINIMAL_RESPONSES 385
#define VAR_RRSET_ROUNDROBIN 386
#define VAR_MAX_UDP_SIZE 387
#define VAR_DELAY_CLOSE 388
#define VAR_UNBLOCK_LAN_ZONES 389
#define VAR_INFRA_CACHE_MIN_RTT 390
#define VAR_DNS64_PREFIX 391
#define VAR_DNS64_SYNTHALL 392
#define VAR_DNSTAP 393
#define VAR_DNSTAP_ENABLE 394
#define VAR_DNSTAP_SOCKET_PATH 395
#define VAR_DNSTAP_SEND_IDENTITY 396
#define VAR_DNSTAP_SEND_VERSION 397
#define VAR_DNSTAP_IDENTITY 398
#define VAR_DNSTAP_VERSION 399
#define VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES 400
#define VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES 401
#define VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES 402
#define VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES 403
#define VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES 404
#define VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES 405
#define VAR_HARDEN_ALGO_DOWNGRADE 406
#define VAR_IP_TRANSPARENT 407
#define VAR_RATELIMIT 408
#define VAR_RATELIMIT_SLABS 409
#define VAR_RATELIMIT_SIZE 410
#define VAR_RATELIMIT_FOR_DOMAIN 411
#define VAR_RATELIMIT_BELOW_DOMAIN 412
#define VAR_RATELIMIT_FACTOR 413
#define VAR_CAPS_WHITELIST 414
#define VAR_CACHE_MAX_NEGATIVE_TTL 415
#define VAR_PERMIT_SMALL_HOLDDOWN 416
#define VAR_QNAME_MINIMISATION 417



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 64 "./util/configparser.y"

	char*	str;


/* Line 387 of yacc.c  */
#line 467 "util/configparser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_UTIL_CONFIGPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 495 "util/configparser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   305

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  163
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  168
/* YYNRULES -- Number of rules.  */
#define YYNRULES  321
/* YYNRULES -- Number of states.  */
#define YYNSTATES  473

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   417

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    13,    16,    19,    22,
      25,    27,    30,    31,    33,    35,    37,    39,    41,    43,
      45,    47,    49,    51,    53,    55,    57,    59,    61,    63,
      65,    67,    69,    71,    73,    75,    77,    79,    81,    83,
      85,    87,    89,    91,    93,    95,    97,    99,   101,   103,
     105,   107,   109,   111,   113,   115,   117,   119,   121,   123,
     125,   127,   129,   131,   133,   135,   137,   139,   141,   143,
     145,   147,   149,   151,   153,   155,   157,   159,   161,   163,
     165,   167,   169,   171,   173,   175,   177,   179,   181,   183,
     185,   187,   189,   191,   193,   195,   197,   199,   201,   203,
     205,   207,   209,   211,   213,   215,   217,   219,   221,   223,
     225,   227,   229,   231,   233,   235,   237,   239,   241,   243,
     245,   247,   249,   251,   253,   255,   257,   259,   261,   263,
     265,   267,   270,   271,   273,   275,   277,   279,   281,   283,
     286,   287,   289,   291,   293,   295,   298,   301,   304,   307,
     310,   313,   316,   319,   322,   325,   328,   331,   334,   337,
     340,   343,   346,   349,   352,   355,   358,   361,   364,   367,
     370,   373,   376,   379,   382,   385,   388,   391,   394,   397,
     400,   403,   406,   409,   412,   415,   418,   421,   424,   427,
     430,   433,   436,   439,   442,   445,   448,   451,   454,   457,
     460,   463,   466,   469,   472,   475,   478,   481,   484,   487,
     490,   493,   496,   499,   502,   505,   508,   511,   514,   517,
     520,   523,   526,   529,   532,   535,   538,   542,   545,   548,
     551,   554,   557,   560,   563,   566,   569,   572,   575,   578,
     581,   584,   587,   590,   593,   596,   599,   602,   606,   609,
     612,   615,   618,   621,   624,   627,   630,   633,   636,   640,
     644,   647,   650,   653,   656,   659,   662,   665,   668,   671,
     674,   677,   679,   682,   683,   685,   687,   689,   691,   693,
     695,   697,   699,   702,   705,   708,   711,   714,   717,   720,
     723,   725,   728,   729,   731,   733,   735,   737,   739,   741,
     743,   745,   747,   749,   751,   753,   756,   759,   762,   765,
     768,   771,   774,   777,   780,   783,   786,   789,   791,   794,
     795,   797
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     164,     0,    -1,    -1,   164,   165,    -1,   166,   167,    -1,
     169,   170,    -1,   172,   173,    -1,   327,   328,    -1,   301,
     302,    -1,   312,   313,    -1,    11,    -1,   167,   168,    -1,
      -1,   175,    -1,   176,    -1,   180,    -1,   183,    -1,   189,
      -1,   190,    -1,   191,    -1,   192,    -1,   181,    -1,   202,
      -1,   203,    -1,   204,    -1,   205,    -1,   206,    -1,   225,
      -1,   226,    -1,   227,    -1,   231,    -1,   232,    -1,   186,
      -1,   233,    -1,   234,    -1,   237,    -1,   235,    -1,   236,
      -1,   239,    -1,   240,    -1,   241,    -1,   254,    -1,   215,
      -1,   216,    -1,   217,    -1,   218,    -1,   242,    -1,   257,
      -1,   211,    -1,   213,    -1,   258,    -1,   264,    -1,   265,
      -1,   266,    -1,   187,    -1,   224,    -1,   274,    -1,   275,
      -1,   212,    -1,   269,    -1,   199,    -1,   182,    -1,   207,
      -1,   255,    -1,   261,    -1,   243,    -1,   256,    -1,   277,
      -1,   278,    -1,   188,    -1,   177,    -1,   198,    -1,   247,
      -1,   178,    -1,   184,    -1,   185,    -1,   208,    -1,   209,
      -1,   276,    -1,   245,    -1,   249,    -1,   250,    -1,   179,
      -1,   279,    -1,   228,    -1,   253,    -1,   200,    -1,   214,
      -1,   259,    -1,   260,    -1,   263,    -1,   268,    -1,   210,
      -1,   270,    -1,   271,    -1,   272,    -1,   219,    -1,   223,
      -1,   251,    -1,   252,    -1,   220,    -1,   244,    -1,   267,
      -1,   201,    -1,   193,    -1,   194,    -1,   195,    -1,   196,
      -1,   197,    -1,   280,    -1,   281,    -1,   282,    -1,   221,
      -1,   229,    -1,   230,    -1,   283,    -1,   284,    -1,   238,
      -1,   246,    -1,   222,    -1,   285,    -1,   287,    -1,   286,
      -1,   288,    -1,   289,    -1,   290,    -1,   248,    -1,   262,
      -1,   273,    -1,   291,    -1,    38,    -1,   170,   171,    -1,
      -1,   292,    -1,   293,    -1,   294,    -1,   296,    -1,   295,
      -1,    44,    -1,   173,   174,    -1,    -1,   297,    -1,   298,
      -1,   299,    -1,   300,    -1,    13,    10,    -1,    12,    10,
      -1,    76,    10,    -1,    79,    10,    -1,    97,    10,    -1,
      14,    10,    -1,    16,    10,    -1,    67,    10,    -1,    15,
      10,    -1,    80,    10,    -1,    81,    10,    -1,    31,    10,
      -1,    60,    10,    -1,    75,    10,    -1,    17,    10,    -1,
      18,    10,    -1,    19,    10,    -1,    20,    10,    -1,   123,
      10,    -1,   124,    10,    -1,   125,    10,    -1,   126,    10,
      -1,   127,    10,    -1,    77,    10,    -1,    66,    10,    -1,
     102,    10,    -1,   122,    10,    -1,    21,    10,    -1,    22,
      10,    -1,    23,    10,    -1,    24,    10,    -1,    25,    10,
      -1,    68,    10,    -1,    82,    10,    -1,    83,    10,    -1,
     110,    10,    -1,    54,    10,    -1,    64,    10,    -1,    55,
      10,    -1,   103,    10,    -1,    48,    10,    -1,    49,    10,
      -1,    50,    10,    -1,    51,    10,    -1,   114,    10,    -1,
     118,    10,    -1,   119,    10,    -1,   152,    10,    -1,   115,
      10,    -1,    61,    10,    -1,    26,    10,    -1,    27,    10,
      -1,    28,    10,    -1,    99,    10,    -1,   133,    10,    -1,
     134,    10,    -1,    29,    10,    -1,    30,    10,    -1,    32,
      10,    -1,    33,    10,    -1,    35,    10,    -1,    36,    10,
      -1,    34,    10,    -1,   135,    10,    -1,    41,    10,    -1,
      42,    10,    -1,    43,    10,    -1,    52,    10,    -1,    71,
      10,    -1,   120,    10,    -1,    85,    10,    -1,   151,    10,
      -1,    78,    10,    -1,   159,    10,    -1,    86,    10,    -1,
      87,    10,    -1,   116,    10,    -1,   117,    10,    -1,   101,
      10,    -1,    47,    10,    -1,    69,    10,    -1,    72,    10,
      10,    -1,    53,    10,    -1,    56,    10,    -1,   106,    10,
      -1,   107,    10,    -1,    70,    10,    -1,   160,    10,    -1,
     108,    10,    -1,    57,    10,    -1,    58,    10,    -1,    59,
      10,    -1,   121,    10,    -1,   109,    10,    -1,    65,    10,
      -1,   112,    10,    -1,   113,    10,    -1,   111,    10,    -1,
     161,    10,    -1,    62,    10,    -1,    63,    10,    -1,    84,
      10,    -1,    73,    10,    10,    -1,    74,    10,    -1,    98,
      10,    -1,   130,    10,    -1,   131,    10,    -1,   132,    10,
      -1,   136,    10,    -1,   137,    10,    -1,   153,    10,    -1,
     155,    10,    -1,   154,    10,    -1,   156,    10,    10,    -1,
     157,    10,    10,    -1,   158,    10,    -1,   162,    10,    -1,
      37,    10,    -1,    39,    10,    -1,    40,    10,    -1,   129,
      10,    -1,   100,    10,    -1,    37,    10,    -1,    45,    10,
      -1,    46,    10,    -1,   128,    10,    -1,    88,    -1,   302,
     303,    -1,    -1,   304,    -1,   306,    -1,   305,    -1,   308,
      -1,   309,    -1,   310,    -1,   311,    -1,   307,    -1,    89,
      10,    -1,    91,    10,    -1,    90,    10,    -1,    96,    10,
      -1,    92,    10,    -1,    93,    10,    -1,    94,    10,    -1,
      95,    10,    -1,   138,    -1,   313,   314,    -1,    -1,   315,
      -1,   316,    -1,   317,    -1,   318,    -1,   319,    -1,   320,
      -1,   321,    -1,   322,    -1,   323,    -1,   324,    -1,   325,
      -1,   326,    -1,   139,    10,    -1,   140,    10,    -1,   141,
      10,    -1,   142,    10,    -1,   143,    10,    -1,   144,    10,
      -1,   145,    10,    -1,   146,    10,    -1,   147,    10,    -1,
     148,    10,    -1,   149,    10,    -1,   150,    10,    -1,   104,
      -1,   328,   329,    -1,    -1,   330,    -1,   105,    10,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   128,   128,   128,   129,   129,   130,   130,   131,   131,
     135,   140,   141,   142,   142,   142,   143,   143,   144,   144,
     144,   145,   145,   145,   146,   146,   146,   147,   147,   148,
     148,   149,   149,   150,   150,   151,   151,   152,   152,   153,
     153,   154,   154,   155,   155,   155,   156,   156,   156,   157,
     157,   157,   158,   158,   159,   159,   160,   160,   161,   161,
     162,   162,   162,   163,   163,   164,   164,   165,   165,   165,
     166,   166,   167,   167,   168,   168,   169,   169,   169,   170,
     170,   171,   171,   172,   172,   173,   173,   174,   174,   175,
     175,   175,   176,   176,   177,   177,   177,   178,   178,   178,
     179,   179,   179,   180,   180,   180,   181,   181,   181,   182,
     182,   182,   183,   183,   183,   184,   184,   185,   185,   186,
     186,   186,   187,   187,   188,   188,   189,   189,   190,   190,
     192,   204,   205,   206,   206,   206,   206,   206,   208,   220,
     221,   222,   222,   222,   222,   224,   233,   242,   253,   262,
     271,   280,   293,   308,   317,   326,   335,   344,   353,   362,
     371,   380,   389,   398,   407,   416,   423,   430,   439,   448,
     462,   471,   480,   487,   494,   501,   509,   516,   523,   530,
     537,   545,   553,   561,   568,   575,   584,   593,   600,   607,
     615,   623,   633,   643,   656,   667,   675,   688,   697,   706,
     715,   725,   733,   746,   755,   763,   772,   780,   793,   802,
     809,   819,   829,   839,   849,   859,   869,   879,   889,   896,
     903,   910,   919,   928,   937,   944,   954,   971,   978,   996,
    1009,  1022,  1031,  1040,  1049,  1058,  1068,  1078,  1087,  1096,
    1103,  1112,  1121,  1130,  1139,  1147,  1160,  1168,  1192,  1199,
    1214,  1224,  1234,  1241,  1248,  1257,  1266,  1274,  1287,  1300,
    1313,  1322,  1332,  1342,  1349,  1356,  1365,  1375,  1385,  1392,
    1399,  1408,  1413,  1414,  1415,  1415,  1415,  1416,  1416,  1416,
    1417,  1417,  1419,  1429,  1438,  1445,  1455,  1462,  1469,  1476,
    1483,  1488,  1489,  1490,  1490,  1491,  1491,  1492,  1492,  1493,
    1494,  1495,  1496,  1497,  1498,  1500,  1508,  1515,  1523,  1531,
    1538,  1545,  1554,  1563,  1572,  1581,  1590,  1599,  1604,  1605,
    1606,  1608
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SPACE", "LETTER", "NEWLINE", "COMMENT",
  "COLON", "ANY", "ZONESTR", "STRING_ARG", "VAR_SERVER", "VAR_VERBOSITY",
  "VAR_NUM_THREADS", "VAR_PORT", "VAR_OUTGOING_RANGE", "VAR_INTERFACE",
  "VAR_DO_IP4", "VAR_DO_IP6", "VAR_DO_UDP", "VAR_DO_TCP", "VAR_CHROOT",
  "VAR_USERNAME", "VAR_DIRECTORY", "VAR_LOGFILE", "VAR_PIDFILE",
  "VAR_MSG_CACHE_SIZE", "VAR_MSG_CACHE_SLABS",
  "VAR_NUM_QUERIES_PER_THREAD", "VAR_RRSET_CACHE_SIZE",
  "VAR_RRSET_CACHE_SLABS", "VAR_OUTGOING_NUM_TCP", "VAR_INFRA_HOST_TTL",
  "VAR_INFRA_LAME_TTL", "VAR_INFRA_CACHE_SLABS",
  "VAR_INFRA_CACHE_NUMHOSTS", "VAR_INFRA_CACHE_LAME_SIZE", "VAR_NAME",
  "VAR_STUB_ZONE", "VAR_STUB_HOST", "VAR_STUB_ADDR",
  "VAR_TARGET_FETCH_POLICY", "VAR_HARDEN_SHORT_BUFSIZE",
  "VAR_HARDEN_LARGE_QUERIES", "VAR_FORWARD_ZONE", "VAR_FORWARD_HOST",
  "VAR_FORWARD_ADDR", "VAR_DO_NOT_QUERY_ADDRESS", "VAR_HIDE_IDENTITY",
  "VAR_HIDE_VERSION", "VAR_IDENTITY", "VAR_VERSION", "VAR_HARDEN_GLUE",
  "VAR_MODULE_CONF", "VAR_TRUST_ANCHOR_FILE", "VAR_TRUST_ANCHOR",
  "VAR_VAL_OVERRIDE_DATE", "VAR_BOGUS_TTL", "VAR_VAL_CLEAN_ADDITIONAL",
  "VAR_VAL_PERMISSIVE_MODE", "VAR_INCOMING_NUM_TCP", "VAR_MSG_BUFFER_SIZE",
  "VAR_KEY_CACHE_SIZE", "VAR_KEY_CACHE_SLABS", "VAR_TRUSTED_KEYS_FILE",
  "VAR_VAL_NSEC3_KEYSIZE_ITERATIONS", "VAR_USE_SYSLOG",
  "VAR_OUTGOING_INTERFACE", "VAR_ROOT_HINTS", "VAR_DO_NOT_QUERY_LOCALHOST",
  "VAR_CACHE_MAX_TTL", "VAR_HARDEN_DNSSEC_STRIPPED", "VAR_ACCESS_CONTROL",
  "VAR_LOCAL_ZONE", "VAR_LOCAL_DATA", "VAR_INTERFACE_AUTOMATIC",
  "VAR_STATISTICS_INTERVAL", "VAR_DO_DAEMONIZE", "VAR_USE_CAPS_FOR_ID",
  "VAR_STATISTICS_CUMULATIVE", "VAR_OUTGOING_PORT_PERMIT",
  "VAR_OUTGOING_PORT_AVOID", "VAR_DLV_ANCHOR_FILE", "VAR_DLV_ANCHOR",
  "VAR_NEG_CACHE_SIZE", "VAR_HARDEN_REFERRAL_PATH", "VAR_PRIVATE_ADDRESS",
  "VAR_PRIVATE_DOMAIN", "VAR_REMOTE_CONTROL", "VAR_CONTROL_ENABLE",
  "VAR_CONTROL_INTERFACE", "VAR_CONTROL_PORT", "VAR_SERVER_KEY_FILE",
  "VAR_SERVER_CERT_FILE", "VAR_CONTROL_KEY_FILE", "VAR_CONTROL_CERT_FILE",
  "VAR_CONTROL_USE_CERT", "VAR_EXTENDED_STATISTICS", "VAR_LOCAL_DATA_PTR",
  "VAR_JOSTLE_TIMEOUT", "VAR_STUB_PRIME", "VAR_UNWANTED_REPLY_THRESHOLD",
  "VAR_LOG_TIME_ASCII", "VAR_DOMAIN_INSECURE", "VAR_PYTHON",
  "VAR_PYTHON_SCRIPT", "VAR_VAL_SIG_SKEW_MIN", "VAR_VAL_SIG_SKEW_MAX",
  "VAR_CACHE_MIN_TTL", "VAR_VAL_LOG_LEVEL", "VAR_AUTO_TRUST_ANCHOR_FILE",
  "VAR_KEEP_MISSING", "VAR_ADD_HOLDDOWN", "VAR_DEL_HOLDDOWN",
  "VAR_SO_RCVBUF", "VAR_EDNS_BUFFER_SIZE", "VAR_PREFETCH",
  "VAR_PREFETCH_KEY", "VAR_SO_SNDBUF", "VAR_SO_REUSEPORT",
  "VAR_HARDEN_BELOW_NXDOMAIN", "VAR_IGNORE_CD_FLAG", "VAR_LOG_QUERIES",
  "VAR_TCP_UPSTREAM", "VAR_SSL_UPSTREAM", "VAR_SSL_SERVICE_KEY",
  "VAR_SSL_SERVICE_PEM", "VAR_SSL_PORT", "VAR_FORWARD_FIRST",
  "VAR_STUB_FIRST", "VAR_MINIMAL_RESPONSES", "VAR_RRSET_ROUNDROBIN",
  "VAR_MAX_UDP_SIZE", "VAR_DELAY_CLOSE", "VAR_UNBLOCK_LAN_ZONES",
  "VAR_INFRA_CACHE_MIN_RTT", "VAR_DNS64_PREFIX", "VAR_DNS64_SYNTHALL",
  "VAR_DNSTAP", "VAR_DNSTAP_ENABLE", "VAR_DNSTAP_SOCKET_PATH",
  "VAR_DNSTAP_SEND_IDENTITY", "VAR_DNSTAP_SEND_VERSION",
  "VAR_DNSTAP_IDENTITY", "VAR_DNSTAP_VERSION",
  "VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES",
  "VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES",
  "VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES",
  "VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES",
  "VAR_HARDEN_ALGO_DOWNGRADE", "VAR_IP_TRANSPARENT", "VAR_RATELIMIT",
  "VAR_RATELIMIT_SLABS", "VAR_RATELIMIT_SIZE", "VAR_RATELIMIT_FOR_DOMAIN",
  "VAR_RATELIMIT_BELOW_DOMAIN", "VAR_RATELIMIT_FACTOR",
  "VAR_CAPS_WHITELIST", "VAR_CACHE_MAX_NEGATIVE_TTL",
  "VAR_PERMIT_SMALL_HOLDDOWN", "VAR_QNAME_MINIMISATION", "$accept",
  "toplevelvars", "toplevelvar", "serverstart", "contents_server",
  "content_server", "stubstart", "contents_stub", "content_stub",
  "forwardstart", "contents_forward", "content_forward",
  "server_num_threads", "server_verbosity", "server_statistics_interval",
  "server_statistics_cumulative", "server_extended_statistics",
  "server_port", "server_interface", "server_outgoing_interface",
  "server_outgoing_range", "server_outgoing_port_permit",
  "server_outgoing_port_avoid", "server_outgoing_num_tcp",
  "server_incoming_num_tcp", "server_interface_automatic", "server_do_ip4",
  "server_do_ip6", "server_do_udp", "server_do_tcp", "server_tcp_upstream",
  "server_ssl_upstream", "server_ssl_service_key",
  "server_ssl_service_pem", "server_ssl_port", "server_do_daemonize",
  "server_use_syslog", "server_log_time_ascii", "server_log_queries",
  "server_chroot", "server_username", "server_directory", "server_logfile",
  "server_pidfile", "server_root_hints", "server_dlv_anchor_file",
  "server_dlv_anchor", "server_auto_trust_anchor_file",
  "server_trust_anchor_file", "server_trusted_keys_file",
  "server_trust_anchor", "server_domain_insecure", "server_hide_identity",
  "server_hide_version", "server_identity", "server_version",
  "server_so_rcvbuf", "server_so_sndbuf", "server_so_reuseport",
  "server_ip_transparent", "server_edns_buffer_size",
  "server_msg_buffer_size", "server_msg_cache_size",
  "server_msg_cache_slabs", "server_num_queries_per_thread",
  "server_jostle_timeout", "server_delay_close",
  "server_unblock_lan_zones", "server_rrset_cache_size",
  "server_rrset_cache_slabs", "server_infra_host_ttl",
  "server_infra_lame_ttl", "server_infra_cache_numhosts",
  "server_infra_cache_lame_size", "server_infra_cache_slabs",
  "server_infra_cache_min_rtt", "server_target_fetch_policy",
  "server_harden_short_bufsize", "server_harden_large_queries",
  "server_harden_glue", "server_harden_dnssec_stripped",
  "server_harden_below_nxdomain", "server_harden_referral_path",
  "server_harden_algo_downgrade", "server_use_caps_for_id",
  "server_caps_whitelist", "server_private_address",
  "server_private_domain", "server_prefetch", "server_prefetch_key",
  "server_unwanted_reply_threshold", "server_do_not_query_address",
  "server_do_not_query_localhost", "server_access_control",
  "server_module_conf", "server_val_override_date",
  "server_val_sig_skew_min", "server_val_sig_skew_max",
  "server_cache_max_ttl", "server_cache_max_negative_ttl",
  "server_cache_min_ttl", "server_bogus_ttl",
  "server_val_clean_additional", "server_val_permissive_mode",
  "server_ignore_cd_flag", "server_val_log_level",
  "server_val_nsec3_keysize_iterations", "server_add_holddown",
  "server_del_holddown", "server_keep_missing",
  "server_permit_small_holddown", "server_key_cache_size",
  "server_key_cache_slabs", "server_neg_cache_size", "server_local_zone",
  "server_local_data", "server_local_data_ptr", "server_minimal_responses",
  "server_rrset_roundrobin", "server_max_udp_size", "server_dns64_prefix",
  "server_dns64_synthall", "server_ratelimit", "server_ratelimit_size",
  "server_ratelimit_slabs", "server_ratelimit_for_domain",
  "server_ratelimit_below_domain", "server_ratelimit_factor",
  "server_qname_minimisation", "stub_name", "stub_host", "stub_addr",
  "stub_first", "stub_prime", "forward_name", "forward_host",
  "forward_addr", "forward_first", "rcstart", "contents_rc", "content_rc",
  "rc_control_enable", "rc_control_port", "rc_control_interface",
  "rc_control_use_cert", "rc_server_key_file", "rc_server_cert_file",
  "rc_control_key_file", "rc_control_cert_file", "dtstart", "contents_dt",
  "content_dt", "dt_dnstap_enable", "dt_dnstap_socket_path",
  "dt_dnstap_send_identity", "dt_dnstap_send_version",
  "dt_dnstap_identity", "dt_dnstap_version",
  "dt_dnstap_log_resolver_query_messages",
  "dt_dnstap_log_resolver_response_messages",
  "dt_dnstap_log_client_query_messages",
  "dt_dnstap_log_client_response_messages",
  "dt_dnstap_log_forwarder_query_messages",
  "dt_dnstap_log_forwarder_response_messages", "pythonstart",
  "contents_py", "content_py", "py_script", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   163,   164,   164,   165,   165,   165,   165,   165,   165,
     166,   167,   167,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   168,   168,   168,
     169,   170,   170,   171,   171,   171,   171,   171,   172,   173,
     173,   174,   174,   174,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   302,   303,   303,   303,   303,   303,   303,
     303,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   313,   314,   314,   314,   314,   314,   314,   314,
     314,   314,   314,   314,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   328,
     329,   330
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       1,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     0,     1,     1,     1,     1,     1,     1,     2,
       0,     1,     1,     1,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     3,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     3,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     3,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     2,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       1,     2,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     2,     0,
       1,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,    10,   130,   138,   271,   317,   290,     3,
      12,   132,   140,   273,   292,   319,     4,     5,     6,     8,
       9,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    11,
      13,    14,    70,    73,    82,    15,    21,    61,    16,    74,
      75,    32,    54,    69,    17,    18,    19,    20,   104,   105,
     106,   107,   108,    71,    60,    86,   103,    22,    23,    24,
      25,    26,    62,    76,    77,    92,    48,    58,    49,    87,
      42,    43,    44,    45,    96,   100,   112,   119,    97,    55,
      27,    28,    29,    84,   113,   114,    30,    31,    33,    34,
      36,    37,    35,   117,    38,    39,    40,    46,    65,   101,
      79,   118,    72,   126,    80,    81,    98,    99,    85,    41,
      63,    66,    47,    50,    88,    89,    64,   127,    90,    51,
      52,    53,   102,    91,    59,    93,    94,    95,   128,    56,
      57,    78,    67,    68,    83,   109,   110,   111,   115,   116,
     120,   122,   121,   123,   124,   125,   129,     0,     0,     0,
       0,     0,   131,   133,   134,   135,   137,   136,     0,     0,
       0,     0,   139,   141,   142,   143,   144,     0,     0,     0,
       0,     0,     0,     0,     0,   272,   274,   276,   275,   281,
     277,   278,   279,   280,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   291,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,     0,
     318,   320,   146,   145,   150,   153,   151,   159,   160,   161,
     162,   172,   173,   174,   175,   176,   195,   196,   197,   201,
     202,   156,   203,   204,   207,   205,   206,   209,   210,   211,
     224,   185,   186,   187,   188,   212,   227,   181,   183,   228,
     234,   235,   236,   157,   194,   244,   245,   182,   239,   169,
     152,   177,   225,   231,   213,     0,     0,   248,   158,   147,
     168,   217,   148,   154,   155,   178,   179,   246,   215,   219,
     220,   149,   249,   198,   223,   170,   184,   229,   230,   233,
     238,   180,   242,   240,   241,   189,   193,   221,   222,   190,
     191,   214,   237,   171,   163,   164,   165,   166,   167,   250,
     251,   252,   199,   200,   208,   253,   254,   216,   192,   255,
     257,   256,     0,     0,   260,   218,   232,   243,   261,   262,
     263,   264,   266,   265,   267,   268,   269,   270,   282,   284,
     283,   286,   287,   288,   289,   285,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   321,   226,
     247,   258,   259
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     9,    10,    16,   139,    11,    17,   262,    12,
      18,   272,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   263,
     264,   265,   266,   267,   273,   274,   275,   276,    13,    19,
     285,   286,   287,   288,   289,   290,   291,   292,   293,    14,
      20,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,    15,    21,   320,   321
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -81
static const yytype_int16 yypact[] =
{
     -81,   116,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -12,    89,    47,   -13,
      22,   -80,    16,    17,    18,    23,    24,    78,   107,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   141,   142,
     143,   145,   146,   147,   148,   149,   163,   164,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   209,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   262,   263,   264,
     265,   266,   -81,   -81,   -81,   -81,   -81,   -81,   267,   268,
     269,   270,   -81,   -81,   -81,   -81,   -81,   271,   272,   273,
     274,   275,   276,   277,   278,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   291,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   292,   293,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   294,   295,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,   -81,   -81,   -81,   -81,   -81
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,   319,   322,   323,   324,    47,
      48,    49,     0,   325,   326,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,   277,   278,   279,   280,
     281,   282,   283,   284,   268,    91,    92,    93,   327,    94,
      95,    96,   269,   270,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,     2,   328,   119,   120,
     121,   122,   123,   124,   125,   126,   257,     3,   258,   259,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   338,   339,   340,     4,   341,   342,   343,   344,   345,
       5,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,   305,   346,   347,   271,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   260,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,     6,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   261,   388,
       7,   389,   390,   391,   392,   393,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,     8,   422,   423,   424,   425,   426,
     427,   428,   429,   430,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,   456,
     457,   458,   459,   460,   461,   462,   463,   464,   465,   466,
     467,   468,   469,   470,   471,   472
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-81)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,   105,    10,    10,    10,    41,
      42,    43,    -1,    10,    10,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    89,    90,    91,    92,
      93,    94,    95,    96,    37,    97,    98,    99,    10,   101,
     102,   103,    45,    46,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,     0,    10,   130,   131,
     132,   133,   134,   135,   136,   137,    37,    11,    39,    40,
      10,    10,    10,    10,    10,    10,    10,    10,    10,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,    10,    10,    10,    38,    10,    10,    10,    10,    10,
      44,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,    10,    10,   128,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,   100,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    88,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,   129,    10,
     104,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,   138,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   164,     0,    11,    38,    44,    88,   104,   138,   165,
     166,   169,   172,   301,   312,   327,   167,   170,   173,   302,
     313,   328,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    41,    42,    43,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    97,    98,    99,   101,   102,   103,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   130,
     131,   132,   133,   134,   135,   136,   137,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   168,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,    37,    39,    40,
     100,   129,   171,   292,   293,   294,   295,   296,    37,    45,
      46,   128,   174,   297,   298,   299,   300,    89,    90,    91,
      92,    93,    94,    95,    96,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,   105,
     329,   330,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 10:
/* Line 1792 of yacc.c  */
#line 136 "./util/configparser.y"
    { 
		OUTYY(("\nP(server:)\n")); 
	}
    break;

  case 130:
/* Line 1792 of yacc.c  */
#line 193 "./util/configparser.y"
    {
		struct config_stub* s;
		OUTYY(("\nP(stub_zone:)\n")); 
		s = (struct config_stub*)calloc(1, sizeof(struct config_stub));
		if(s) {
			s->next = cfg_parser->cfg->stubs;
			cfg_parser->cfg->stubs = s;
		} else 
			yyerror("out of memory");
	}
    break;

  case 138:
/* Line 1792 of yacc.c  */
#line 209 "./util/configparser.y"
    {
		struct config_stub* s;
		OUTYY(("\nP(forward_zone:)\n")); 
		s = (struct config_stub*)calloc(1, sizeof(struct config_stub));
		if(s) {
			s->next = cfg_parser->cfg->forwards;
			cfg_parser->cfg->forwards = s;
		} else 
			yyerror("out of memory");
	}
    break;

  case 145:
/* Line 1792 of yacc.c  */
#line 225 "./util/configparser.y"
    { 
		OUTYY(("P(server_num_threads:%s)\n", (yyvsp[(2) - (2)].str))); 
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->num_threads = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 146:
/* Line 1792 of yacc.c  */
#line 234 "./util/configparser.y"
    { 
		OUTYY(("P(server_verbosity:%s)\n", (yyvsp[(2) - (2)].str))); 
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->verbosity = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 147:
/* Line 1792 of yacc.c  */
#line 243 "./util/configparser.y"
    { 
		OUTYY(("P(server_statistics_interval:%s)\n", (yyvsp[(2) - (2)].str))); 
		if(strcmp((yyvsp[(2) - (2)].str), "") == 0 || strcmp((yyvsp[(2) - (2)].str), "0") == 0)
			cfg_parser->cfg->stat_interval = 0;
		else if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else cfg_parser->cfg->stat_interval = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 148:
/* Line 1792 of yacc.c  */
#line 254 "./util/configparser.y"
    {
		OUTYY(("P(server_statistics_cumulative:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->stat_cumulative = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 149:
/* Line 1792 of yacc.c  */
#line 263 "./util/configparser.y"
    {
		OUTYY(("P(server_extended_statistics:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->stat_extended = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 150:
/* Line 1792 of yacc.c  */
#line 272 "./util/configparser.y"
    {
		OUTYY(("P(server_port:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("port number expected");
		else cfg_parser->cfg->port = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 151:
/* Line 1792 of yacc.c  */
#line 281 "./util/configparser.y"
    {
		OUTYY(("P(server_interface:%s)\n", (yyvsp[(2) - (2)].str)));
		if(cfg_parser->cfg->num_ifs == 0)
			cfg_parser->cfg->ifs = calloc(1, sizeof(char*));
		else 	cfg_parser->cfg->ifs = realloc(cfg_parser->cfg->ifs,
				(cfg_parser->cfg->num_ifs+1)*sizeof(char*));
		if(!cfg_parser->cfg->ifs)
			yyerror("out of memory");
		else
			cfg_parser->cfg->ifs[cfg_parser->cfg->num_ifs++] = (yyvsp[(2) - (2)].str);
	}
    break;

  case 152:
/* Line 1792 of yacc.c  */
#line 294 "./util/configparser.y"
    {
		OUTYY(("P(server_outgoing_interface:%s)\n", (yyvsp[(2) - (2)].str)));
		if(cfg_parser->cfg->num_out_ifs == 0)
			cfg_parser->cfg->out_ifs = calloc(1, sizeof(char*));
		else 	cfg_parser->cfg->out_ifs = realloc(
			cfg_parser->cfg->out_ifs, 
			(cfg_parser->cfg->num_out_ifs+1)*sizeof(char*));
		if(!cfg_parser->cfg->out_ifs)
			yyerror("out of memory");
		else
			cfg_parser->cfg->out_ifs[
				cfg_parser->cfg->num_out_ifs++] = (yyvsp[(2) - (2)].str);
	}
    break;

  case 153:
/* Line 1792 of yacc.c  */
#line 309 "./util/configparser.y"
    {
		OUTYY(("P(server_outgoing_range:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else cfg_parser->cfg->outgoing_num_ports = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 154:
/* Line 1792 of yacc.c  */
#line 318 "./util/configparser.y"
    {
		OUTYY(("P(server_outgoing_port_permit:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_mark_ports((yyvsp[(2) - (2)].str), 1, 
			cfg_parser->cfg->outgoing_avail_ports, 65536))
			yyerror("port number or range (\"low-high\") expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 155:
/* Line 1792 of yacc.c  */
#line 327 "./util/configparser.y"
    {
		OUTYY(("P(server_outgoing_port_avoid:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_mark_ports((yyvsp[(2) - (2)].str), 0, 
			cfg_parser->cfg->outgoing_avail_ports, 65536))
			yyerror("port number or range (\"low-high\") expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 156:
/* Line 1792 of yacc.c  */
#line 336 "./util/configparser.y"
    {
		OUTYY(("P(server_outgoing_num_tcp:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->outgoing_num_tcp = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 157:
/* Line 1792 of yacc.c  */
#line 345 "./util/configparser.y"
    {
		OUTYY(("P(server_incoming_num_tcp:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->incoming_num_tcp = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 158:
/* Line 1792 of yacc.c  */
#line 354 "./util/configparser.y"
    {
		OUTYY(("P(server_interface_automatic:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->if_automatic = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 159:
/* Line 1792 of yacc.c  */
#line 363 "./util/configparser.y"
    {
		OUTYY(("P(server_do_ip4:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->do_ip4 = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 160:
/* Line 1792 of yacc.c  */
#line 372 "./util/configparser.y"
    {
		OUTYY(("P(server_do_ip6:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->do_ip6 = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 161:
/* Line 1792 of yacc.c  */
#line 381 "./util/configparser.y"
    {
		OUTYY(("P(server_do_udp:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->do_udp = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 162:
/* Line 1792 of yacc.c  */
#line 390 "./util/configparser.y"
    {
		OUTYY(("P(server_do_tcp:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->do_tcp = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 163:
/* Line 1792 of yacc.c  */
#line 399 "./util/configparser.y"
    {
		OUTYY(("P(server_tcp_upstream:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->tcp_upstream = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 164:
/* Line 1792 of yacc.c  */
#line 408 "./util/configparser.y"
    {
		OUTYY(("P(server_ssl_upstream:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->ssl_upstream = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 165:
/* Line 1792 of yacc.c  */
#line 417 "./util/configparser.y"
    {
		OUTYY(("P(server_ssl_service_key:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->ssl_service_key);
		cfg_parser->cfg->ssl_service_key = (yyvsp[(2) - (2)].str);
	}
    break;

  case 166:
/* Line 1792 of yacc.c  */
#line 424 "./util/configparser.y"
    {
		OUTYY(("P(server_ssl_service_pem:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->ssl_service_pem);
		cfg_parser->cfg->ssl_service_pem = (yyvsp[(2) - (2)].str);
	}
    break;

  case 167:
/* Line 1792 of yacc.c  */
#line 431 "./util/configparser.y"
    {
		OUTYY(("P(server_ssl_port:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("port number expected");
		else cfg_parser->cfg->ssl_port = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 168:
/* Line 1792 of yacc.c  */
#line 440 "./util/configparser.y"
    {
		OUTYY(("P(server_do_daemonize:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->do_daemonize = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 169:
/* Line 1792 of yacc.c  */
#line 449 "./util/configparser.y"
    {
		OUTYY(("P(server_use_syslog:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->use_syslog = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
#if !defined(HAVE_SYSLOG_H) && !defined(UB_ON_WINDOWS)
		if(strcmp((yyvsp[(2) - (2)].str), "yes") == 0)
			yyerror("no syslog services are available. "
				"(reconfigure and compile to add)");
#endif
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 170:
/* Line 1792 of yacc.c  */
#line 463 "./util/configparser.y"
    {
		OUTYY(("P(server_log_time_ascii:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->log_time_ascii = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 171:
/* Line 1792 of yacc.c  */
#line 472 "./util/configparser.y"
    {
		OUTYY(("P(server_log_queries:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->log_queries = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 172:
/* Line 1792 of yacc.c  */
#line 481 "./util/configparser.y"
    {
		OUTYY(("P(server_chroot:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->chrootdir);
		cfg_parser->cfg->chrootdir = (yyvsp[(2) - (2)].str);
	}
    break;

  case 173:
/* Line 1792 of yacc.c  */
#line 488 "./util/configparser.y"
    {
		OUTYY(("P(server_username:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->username);
		cfg_parser->cfg->username = (yyvsp[(2) - (2)].str);
	}
    break;

  case 174:
/* Line 1792 of yacc.c  */
#line 495 "./util/configparser.y"
    {
		OUTYY(("P(server_directory:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->directory);
		cfg_parser->cfg->directory = (yyvsp[(2) - (2)].str);
	}
    break;

  case 175:
/* Line 1792 of yacc.c  */
#line 502 "./util/configparser.y"
    {
		OUTYY(("P(server_logfile:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->logfile);
		cfg_parser->cfg->logfile = (yyvsp[(2) - (2)].str);
		cfg_parser->cfg->use_syslog = 0;
	}
    break;

  case 176:
/* Line 1792 of yacc.c  */
#line 510 "./util/configparser.y"
    {
		OUTYY(("P(server_pidfile:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->pidfile);
		cfg_parser->cfg->pidfile = (yyvsp[(2) - (2)].str);
	}
    break;

  case 177:
/* Line 1792 of yacc.c  */
#line 517 "./util/configparser.y"
    {
		OUTYY(("P(server_root_hints:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->root_hints, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 178:
/* Line 1792 of yacc.c  */
#line 524 "./util/configparser.y"
    {
		OUTYY(("P(server_dlv_anchor_file:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->dlv_anchor_file);
		cfg_parser->cfg->dlv_anchor_file = (yyvsp[(2) - (2)].str);
	}
    break;

  case 179:
/* Line 1792 of yacc.c  */
#line 531 "./util/configparser.y"
    {
		OUTYY(("P(server_dlv_anchor:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->dlv_anchor_list, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 180:
/* Line 1792 of yacc.c  */
#line 538 "./util/configparser.y"
    {
		OUTYY(("P(server_auto_trust_anchor_file:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->
			auto_trust_anchor_file_list, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 181:
/* Line 1792 of yacc.c  */
#line 546 "./util/configparser.y"
    {
		OUTYY(("P(server_trust_anchor_file:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->
			trust_anchor_file_list, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 182:
/* Line 1792 of yacc.c  */
#line 554 "./util/configparser.y"
    {
		OUTYY(("P(server_trusted_keys_file:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->
			trusted_keys_file_list, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 183:
/* Line 1792 of yacc.c  */
#line 562 "./util/configparser.y"
    {
		OUTYY(("P(server_trust_anchor:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->trust_anchor_list, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 184:
/* Line 1792 of yacc.c  */
#line 569 "./util/configparser.y"
    {
		OUTYY(("P(server_domain_insecure:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->domain_insecure, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 185:
/* Line 1792 of yacc.c  */
#line 576 "./util/configparser.y"
    {
		OUTYY(("P(server_hide_identity:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->hide_identity = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 186:
/* Line 1792 of yacc.c  */
#line 585 "./util/configparser.y"
    {
		OUTYY(("P(server_hide_version:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->hide_version = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 187:
/* Line 1792 of yacc.c  */
#line 594 "./util/configparser.y"
    {
		OUTYY(("P(server_identity:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->identity);
		cfg_parser->cfg->identity = (yyvsp[(2) - (2)].str);
	}
    break;

  case 188:
/* Line 1792 of yacc.c  */
#line 601 "./util/configparser.y"
    {
		OUTYY(("P(server_version:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->version);
		cfg_parser->cfg->version = (yyvsp[(2) - (2)].str);
	}
    break;

  case 189:
/* Line 1792 of yacc.c  */
#line 608 "./util/configparser.y"
    {
		OUTYY(("P(server_so_rcvbuf:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->so_rcvbuf))
			yyerror("buffer size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 190:
/* Line 1792 of yacc.c  */
#line 616 "./util/configparser.y"
    {
		OUTYY(("P(server_so_sndbuf:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->so_sndbuf))
			yyerror("buffer size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 191:
/* Line 1792 of yacc.c  */
#line 624 "./util/configparser.y"
    {
        OUTYY(("P(server_so_reuseport:%s)\n", (yyvsp[(2) - (2)].str)));
        if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
            yyerror("expected yes or no.");
        else cfg_parser->cfg->so_reuseport =
            (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
        free((yyvsp[(2) - (2)].str));
    }
    break;

  case 192:
/* Line 1792 of yacc.c  */
#line 634 "./util/configparser.y"
    {
        OUTYY(("P(server_ip_transparent:%s)\n", (yyvsp[(2) - (2)].str)));
        if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
            yyerror("expected yes or no.");
        else cfg_parser->cfg->ip_transparent =
            (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
        free((yyvsp[(2) - (2)].str));
    }
    break;

  case 193:
/* Line 1792 of yacc.c  */
#line 644 "./util/configparser.y"
    {
		OUTYY(("P(server_edns_buffer_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else if (atoi((yyvsp[(2) - (2)].str)) < 12)
			yyerror("edns buffer size too small");
		else if (atoi((yyvsp[(2) - (2)].str)) > 65535)
			cfg_parser->cfg->edns_buffer_size = 65535;
		else cfg_parser->cfg->edns_buffer_size = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 194:
/* Line 1792 of yacc.c  */
#line 657 "./util/configparser.y"
    {
		OUTYY(("P(server_msg_buffer_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else if (atoi((yyvsp[(2) - (2)].str)) < 4096)
			yyerror("message buffer size too small (use 4096)");
		else cfg_parser->cfg->msg_buffer_size = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 195:
/* Line 1792 of yacc.c  */
#line 668 "./util/configparser.y"
    {
		OUTYY(("P(server_msg_cache_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->msg_cache_size))
			yyerror("memory size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 196:
/* Line 1792 of yacc.c  */
#line 676 "./util/configparser.y"
    {
		OUTYY(("P(server_msg_cache_slabs:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else {
			cfg_parser->cfg->msg_cache_slabs = atoi((yyvsp[(2) - (2)].str));
			if(!is_pow2(cfg_parser->cfg->msg_cache_slabs))
				yyerror("must be a power of 2");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 197:
/* Line 1792 of yacc.c  */
#line 689 "./util/configparser.y"
    {
		OUTYY(("P(server_num_queries_per_thread:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else cfg_parser->cfg->num_queries_per_thread = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 198:
/* Line 1792 of yacc.c  */
#line 698 "./util/configparser.y"
    {
		OUTYY(("P(server_jostle_timeout:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->jostle_time = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 199:
/* Line 1792 of yacc.c  */
#line 707 "./util/configparser.y"
    {
		OUTYY(("P(server_delay_close:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->delay_close = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 200:
/* Line 1792 of yacc.c  */
#line 716 "./util/configparser.y"
    {
		OUTYY(("P(server_unblock_lan_zones:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->unblock_lan_zones = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 201:
/* Line 1792 of yacc.c  */
#line 726 "./util/configparser.y"
    {
		OUTYY(("P(server_rrset_cache_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->rrset_cache_size))
			yyerror("memory size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 202:
/* Line 1792 of yacc.c  */
#line 734 "./util/configparser.y"
    {
		OUTYY(("P(server_rrset_cache_slabs:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else {
			cfg_parser->cfg->rrset_cache_slabs = atoi((yyvsp[(2) - (2)].str));
			if(!is_pow2(cfg_parser->cfg->rrset_cache_slabs))
				yyerror("must be a power of 2");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 203:
/* Line 1792 of yacc.c  */
#line 747 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_host_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->host_ttl = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 204:
/* Line 1792 of yacc.c  */
#line 756 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_lame_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		verbose(VERB_DETAIL, "ignored infra-lame-ttl: %s (option "
			"removed, use infra-host-ttl)", (yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 205:
/* Line 1792 of yacc.c  */
#line 764 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_cache_numhosts:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else cfg_parser->cfg->infra_cache_numhosts = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 206:
/* Line 1792 of yacc.c  */
#line 773 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_cache_lame_size:%s)\n", (yyvsp[(2) - (2)].str)));
		verbose(VERB_DETAIL, "ignored infra-cache-lame-size: %s "
			"(option removed, use infra-cache-numhosts)", (yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 207:
/* Line 1792 of yacc.c  */
#line 781 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_cache_slabs:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else {
			cfg_parser->cfg->infra_cache_slabs = atoi((yyvsp[(2) - (2)].str));
			if(!is_pow2(cfg_parser->cfg->infra_cache_slabs))
				yyerror("must be a power of 2");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 208:
/* Line 1792 of yacc.c  */
#line 794 "./util/configparser.y"
    {
		OUTYY(("P(server_infra_cache_min_rtt:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->infra_cache_min_rtt = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 209:
/* Line 1792 of yacc.c  */
#line 803 "./util/configparser.y"
    {
		OUTYY(("P(server_target_fetch_policy:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->target_fetch_policy);
		cfg_parser->cfg->target_fetch_policy = (yyvsp[(2) - (2)].str);
	}
    break;

  case 210:
/* Line 1792 of yacc.c  */
#line 810 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_short_bufsize:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_short_bufsize = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 211:
/* Line 1792 of yacc.c  */
#line 820 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_large_queries:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_large_queries = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 212:
/* Line 1792 of yacc.c  */
#line 830 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_glue:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_glue = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 213:
/* Line 1792 of yacc.c  */
#line 840 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_dnssec_stripped:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_dnssec_stripped = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 214:
/* Line 1792 of yacc.c  */
#line 850 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_below_nxdomain:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_below_nxdomain = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 215:
/* Line 1792 of yacc.c  */
#line 860 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_referral_path:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_referral_path = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 216:
/* Line 1792 of yacc.c  */
#line 870 "./util/configparser.y"
    {
		OUTYY(("P(server_harden_algo_downgrade:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->harden_algo_downgrade = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 217:
/* Line 1792 of yacc.c  */
#line 880 "./util/configparser.y"
    {
		OUTYY(("P(server_use_caps_for_id:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->use_caps_bits_for_id = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 218:
/* Line 1792 of yacc.c  */
#line 890 "./util/configparser.y"
    {
		OUTYY(("P(server_caps_whitelist:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->caps_whitelist, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 219:
/* Line 1792 of yacc.c  */
#line 897 "./util/configparser.y"
    {
		OUTYY(("P(server_private_address:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->private_address, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 220:
/* Line 1792 of yacc.c  */
#line 904 "./util/configparser.y"
    {
		OUTYY(("P(server_private_domain:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->private_domain, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 221:
/* Line 1792 of yacc.c  */
#line 911 "./util/configparser.y"
    {
		OUTYY(("P(server_prefetch:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->prefetch = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 222:
/* Line 1792 of yacc.c  */
#line 920 "./util/configparser.y"
    {
		OUTYY(("P(server_prefetch_key:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->prefetch_key = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 223:
/* Line 1792 of yacc.c  */
#line 929 "./util/configparser.y"
    {
		OUTYY(("P(server_unwanted_reply_threshold:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->unwanted_threshold = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 224:
/* Line 1792 of yacc.c  */
#line 938 "./util/configparser.y"
    {
		OUTYY(("P(server_do_not_query_address:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->donotqueryaddrs, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 225:
/* Line 1792 of yacc.c  */
#line 945 "./util/configparser.y"
    {
		OUTYY(("P(server_do_not_query_localhost:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->donotquery_localhost = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 226:
/* Line 1792 of yacc.c  */
#line 955 "./util/configparser.y"
    {
		OUTYY(("P(server_access_control:%s %s)\n", (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)));
		if(strcmp((yyvsp[(3) - (3)].str), "deny")!=0 && strcmp((yyvsp[(3) - (3)].str), "refuse")!=0 &&
			strcmp((yyvsp[(3) - (3)].str), "deny_non_local")!=0 &&
			strcmp((yyvsp[(3) - (3)].str), "refuse_non_local")!=0 &&
			strcmp((yyvsp[(3) - (3)].str), "allow")!=0 && 
			strcmp((yyvsp[(3) - (3)].str), "allow_snoop")!=0) {
			yyerror("expected deny, refuse, deny_non_local, "
				"refuse_non_local, allow or allow_snoop "
				"in access control action");
		} else {
			if(!cfg_str2list_insert(&cfg_parser->cfg->acls, (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)))
				fatal_exit("out of memory adding acl");
		}
	}
    break;

  case 227:
/* Line 1792 of yacc.c  */
#line 972 "./util/configparser.y"
    {
		OUTYY(("P(server_module_conf:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->module_conf);
		cfg_parser->cfg->module_conf = (yyvsp[(2) - (2)].str);
	}
    break;

  case 228:
/* Line 1792 of yacc.c  */
#line 979 "./util/configparser.y"
    {
		OUTYY(("P(server_val_override_date:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strlen((yyvsp[(2) - (2)].str)) == 0 || strcmp((yyvsp[(2) - (2)].str), "0") == 0) {
			cfg_parser->cfg->val_date_override = 0;
		} else if(strlen((yyvsp[(2) - (2)].str)) == 14) {
			cfg_parser->cfg->val_date_override = 
				cfg_convert_timeval((yyvsp[(2) - (2)].str));
			if(!cfg_parser->cfg->val_date_override)
				yyerror("bad date/time specification");
		} else {
			if(atoi((yyvsp[(2) - (2)].str)) == 0)
				yyerror("number expected");
			cfg_parser->cfg->val_date_override = atoi((yyvsp[(2) - (2)].str));
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 229:
/* Line 1792 of yacc.c  */
#line 997 "./util/configparser.y"
    {
		OUTYY(("P(server_val_sig_skew_min:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strlen((yyvsp[(2) - (2)].str)) == 0 || strcmp((yyvsp[(2) - (2)].str), "0") == 0) {
			cfg_parser->cfg->val_sig_skew_min = 0;
		} else {
			cfg_parser->cfg->val_sig_skew_min = atoi((yyvsp[(2) - (2)].str));
			if(!cfg_parser->cfg->val_sig_skew_min)
				yyerror("number expected");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 230:
/* Line 1792 of yacc.c  */
#line 1010 "./util/configparser.y"
    {
		OUTYY(("P(server_val_sig_skew_max:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strlen((yyvsp[(2) - (2)].str)) == 0 || strcmp((yyvsp[(2) - (2)].str), "0") == 0) {
			cfg_parser->cfg->val_sig_skew_max = 0;
		} else {
			cfg_parser->cfg->val_sig_skew_max = atoi((yyvsp[(2) - (2)].str));
			if(!cfg_parser->cfg->val_sig_skew_max)
				yyerror("number expected");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 231:
/* Line 1792 of yacc.c  */
#line 1023 "./util/configparser.y"
    {
		OUTYY(("P(server_cache_max_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->max_ttl = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 232:
/* Line 1792 of yacc.c  */
#line 1032 "./util/configparser.y"
    {
		OUTYY(("P(server_cache_max_negative_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->max_negative_ttl = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 233:
/* Line 1792 of yacc.c  */
#line 1041 "./util/configparser.y"
    {
		OUTYY(("P(server_cache_min_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->min_ttl = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 234:
/* Line 1792 of yacc.c  */
#line 1050 "./util/configparser.y"
    {
		OUTYY(("P(server_bogus_ttl:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->bogus_ttl = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 235:
/* Line 1792 of yacc.c  */
#line 1059 "./util/configparser.y"
    {
		OUTYY(("P(server_val_clean_additional:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->val_clean_additional = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 236:
/* Line 1792 of yacc.c  */
#line 1069 "./util/configparser.y"
    {
		OUTYY(("P(server_val_permissive_mode:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->val_permissive_mode = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 237:
/* Line 1792 of yacc.c  */
#line 1079 "./util/configparser.y"
    {
		OUTYY(("P(server_ignore_cd_flag:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->ignore_cd = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 238:
/* Line 1792 of yacc.c  */
#line 1088 "./util/configparser.y"
    {
		OUTYY(("P(server_val_log_level:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->val_log_level = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 239:
/* Line 1792 of yacc.c  */
#line 1097 "./util/configparser.y"
    {
		OUTYY(("P(server_val_nsec3_keysize_iterations:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->val_nsec3_key_iterations);
		cfg_parser->cfg->val_nsec3_key_iterations = (yyvsp[(2) - (2)].str);
	}
    break;

  case 240:
/* Line 1792 of yacc.c  */
#line 1104 "./util/configparser.y"
    {
		OUTYY(("P(server_add_holddown:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->add_holddown = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 241:
/* Line 1792 of yacc.c  */
#line 1113 "./util/configparser.y"
    {
		OUTYY(("P(server_del_holddown:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->del_holddown = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 242:
/* Line 1792 of yacc.c  */
#line 1122 "./util/configparser.y"
    {
		OUTYY(("P(server_keep_missing:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->keep_missing = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 243:
/* Line 1792 of yacc.c  */
#line 1131 "./util/configparser.y"
    {
		OUTYY(("P(server_permit_small_holddown:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->permit_small_holddown =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 244:
/* Line 1792 of yacc.c  */
#line 1140 "./util/configparser.y"
    {
		OUTYY(("P(server_key_cache_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->key_cache_size))
			yyerror("memory size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 245:
/* Line 1792 of yacc.c  */
#line 1148 "./util/configparser.y"
    {
		OUTYY(("P(server_key_cache_slabs:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else {
			cfg_parser->cfg->key_cache_slabs = atoi((yyvsp[(2) - (2)].str));
			if(!is_pow2(cfg_parser->cfg->key_cache_slabs))
				yyerror("must be a power of 2");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 246:
/* Line 1792 of yacc.c  */
#line 1161 "./util/configparser.y"
    {
		OUTYY(("P(server_neg_cache_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->neg_cache_size))
			yyerror("memory size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 247:
/* Line 1792 of yacc.c  */
#line 1169 "./util/configparser.y"
    {
		OUTYY(("P(server_local_zone:%s %s)\n", (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)));
		if(strcmp((yyvsp[(3) - (3)].str), "static")!=0 && strcmp((yyvsp[(3) - (3)].str), "deny")!=0 &&
		   strcmp((yyvsp[(3) - (3)].str), "refuse")!=0 && strcmp((yyvsp[(3) - (3)].str), "redirect")!=0 &&
		   strcmp((yyvsp[(3) - (3)].str), "transparent")!=0 && strcmp((yyvsp[(3) - (3)].str), "nodefault")!=0
		   && strcmp((yyvsp[(3) - (3)].str), "typetransparent")!=0 &&
		   strcmp((yyvsp[(3) - (3)].str), "inform")!=0 && strcmp((yyvsp[(3) - (3)].str), "inform_deny")!=0)
			yyerror("local-zone type: expected static, deny, "
				"refuse, redirect, transparent, "
				"typetransparent, inform, inform_deny "
				"or nodefault");
		else if(strcmp((yyvsp[(3) - (3)].str), "nodefault")==0) {
			if(!cfg_strlist_insert(&cfg_parser->cfg->
				local_zones_nodefault, (yyvsp[(2) - (3)].str)))
				fatal_exit("out of memory adding local-zone");
			free((yyvsp[(3) - (3)].str));
		} else {
			if(!cfg_str2list_insert(&cfg_parser->cfg->local_zones, 
				(yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)))
				fatal_exit("out of memory adding local-zone");
		}
	}
    break;

  case 248:
/* Line 1792 of yacc.c  */
#line 1193 "./util/configparser.y"
    {
		OUTYY(("P(server_local_data:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->local_data, (yyvsp[(2) - (2)].str)))
			fatal_exit("out of memory adding local-data");
	}
    break;

  case 249:
/* Line 1792 of yacc.c  */
#line 1200 "./util/configparser.y"
    {
		char* ptr;
		OUTYY(("P(server_local_data_ptr:%s)\n", (yyvsp[(2) - (2)].str)));
		ptr = cfg_ptr_reverse((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
		if(ptr) {
			if(!cfg_strlist_insert(&cfg_parser->cfg->
				local_data, ptr))
				fatal_exit("out of memory adding local-data");
		} else {
			yyerror("local-data-ptr could not be reversed");
		}
	}
    break;

  case 250:
/* Line 1792 of yacc.c  */
#line 1215 "./util/configparser.y"
    {
		OUTYY(("P(server_minimal_responses:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->minimal_responses =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 251:
/* Line 1792 of yacc.c  */
#line 1225 "./util/configparser.y"
    {
		OUTYY(("P(server_rrset_roundrobin:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->rrset_roundrobin =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 252:
/* Line 1792 of yacc.c  */
#line 1235 "./util/configparser.y"
    {
		OUTYY(("P(server_max_udp_size:%s)\n", (yyvsp[(2) - (2)].str)));
		cfg_parser->cfg->max_udp_size = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 253:
/* Line 1792 of yacc.c  */
#line 1242 "./util/configparser.y"
    {
		OUTYY(("P(dns64_prefix:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->dns64_prefix);
		cfg_parser->cfg->dns64_prefix = (yyvsp[(2) - (2)].str);
	}
    break;

  case 254:
/* Line 1792 of yacc.c  */
#line 1249 "./util/configparser.y"
    {
		OUTYY(("P(server_dns64_synthall:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dns64_synthall = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 255:
/* Line 1792 of yacc.c  */
#line 1258 "./util/configparser.y"
    { 
		OUTYY(("P(server_ratelimit:%s)\n", (yyvsp[(2) - (2)].str))); 
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->ratelimit = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 256:
/* Line 1792 of yacc.c  */
#line 1267 "./util/configparser.y"
    {
		OUTYY(("P(server_ratelimit_size:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_parse_memsize((yyvsp[(2) - (2)].str), &cfg_parser->cfg->ratelimit_size))
			yyerror("memory size expected");
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 257:
/* Line 1792 of yacc.c  */
#line 1275 "./util/configparser.y"
    {
		OUTYY(("P(server_ratelimit_slabs:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("number expected");
		else {
			cfg_parser->cfg->ratelimit_slabs = atoi((yyvsp[(2) - (2)].str));
			if(!is_pow2(cfg_parser->cfg->ratelimit_slabs))
				yyerror("must be a power of 2");
		}
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 258:
/* Line 1792 of yacc.c  */
#line 1288 "./util/configparser.y"
    {
		OUTYY(("P(server_ratelimit_for_domain:%s %s)\n", (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)));
		if(atoi((yyvsp[(3) - (3)].str)) == 0 && strcmp((yyvsp[(3) - (3)].str), "0") != 0) {
			yyerror("number expected");
		} else {
			if(!cfg_str2list_insert(&cfg_parser->cfg->
				ratelimit_for_domain, (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)))
				fatal_exit("out of memory adding "
					"ratelimit-for-domain");
		}
	}
    break;

  case 259:
/* Line 1792 of yacc.c  */
#line 1301 "./util/configparser.y"
    {
		OUTYY(("P(server_ratelimit_below_domain:%s %s)\n", (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)));
		if(atoi((yyvsp[(3) - (3)].str)) == 0 && strcmp((yyvsp[(3) - (3)].str), "0") != 0) {
			yyerror("number expected");
		} else {
			if(!cfg_str2list_insert(&cfg_parser->cfg->
				ratelimit_below_domain, (yyvsp[(2) - (3)].str), (yyvsp[(3) - (3)].str)))
				fatal_exit("out of memory adding "
					"ratelimit-below-domain");
		}
	}
    break;

  case 260:
/* Line 1792 of yacc.c  */
#line 1314 "./util/configparser.y"
    { 
		OUTYY(("P(server_ratelimit_factor:%s)\n", (yyvsp[(2) - (2)].str))); 
		if(atoi((yyvsp[(2) - (2)].str)) == 0 && strcmp((yyvsp[(2) - (2)].str), "0") != 0)
			yyerror("number expected");
		else cfg_parser->cfg->ratelimit_factor = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 261:
/* Line 1792 of yacc.c  */
#line 1323 "./util/configparser.y"
    {
		OUTYY(("P(server_qname_minimisation:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->qname_minimisation = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 262:
/* Line 1792 of yacc.c  */
#line 1333 "./util/configparser.y"
    {
		OUTYY(("P(name:%s)\n", (yyvsp[(2) - (2)].str)));
		if(cfg_parser->cfg->stubs->name)
			yyerror("stub name override, there must be one name "
				"for one stub-zone");
		free(cfg_parser->cfg->stubs->name);
		cfg_parser->cfg->stubs->name = (yyvsp[(2) - (2)].str);
	}
    break;

  case 263:
/* Line 1792 of yacc.c  */
#line 1343 "./util/configparser.y"
    {
		OUTYY(("P(stub-host:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->stubs->hosts, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 264:
/* Line 1792 of yacc.c  */
#line 1350 "./util/configparser.y"
    {
		OUTYY(("P(stub-addr:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->stubs->addrs, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 265:
/* Line 1792 of yacc.c  */
#line 1357 "./util/configparser.y"
    {
		OUTYY(("P(stub-first:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->stubs->isfirst=(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 266:
/* Line 1792 of yacc.c  */
#line 1366 "./util/configparser.y"
    {
		OUTYY(("P(stub-prime:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->stubs->isprime = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 267:
/* Line 1792 of yacc.c  */
#line 1376 "./util/configparser.y"
    {
		OUTYY(("P(name:%s)\n", (yyvsp[(2) - (2)].str)));
		if(cfg_parser->cfg->forwards->name)
			yyerror("forward name override, there must be one "
				"name for one forward-zone");
		free(cfg_parser->cfg->forwards->name);
		cfg_parser->cfg->forwards->name = (yyvsp[(2) - (2)].str);
	}
    break;

  case 268:
/* Line 1792 of yacc.c  */
#line 1386 "./util/configparser.y"
    {
		OUTYY(("P(forward-host:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->forwards->hosts, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 269:
/* Line 1792 of yacc.c  */
#line 1393 "./util/configparser.y"
    {
		OUTYY(("P(forward-addr:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->forwards->addrs, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 270:
/* Line 1792 of yacc.c  */
#line 1400 "./util/configparser.y"
    {
		OUTYY(("P(forward-first:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->forwards->isfirst=(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 271:
/* Line 1792 of yacc.c  */
#line 1409 "./util/configparser.y"
    { 
		OUTYY(("\nP(remote-control:)\n")); 
	}
    break;

  case 282:
/* Line 1792 of yacc.c  */
#line 1420 "./util/configparser.y"
    {
		OUTYY(("P(control_enable:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->remote_control_enable = 
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 283:
/* Line 1792 of yacc.c  */
#line 1430 "./util/configparser.y"
    {
		OUTYY(("P(control_port:%s)\n", (yyvsp[(2) - (2)].str)));
		if(atoi((yyvsp[(2) - (2)].str)) == 0)
			yyerror("control port number expected");
		else cfg_parser->cfg->control_port = atoi((yyvsp[(2) - (2)].str));
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 284:
/* Line 1792 of yacc.c  */
#line 1439 "./util/configparser.y"
    {
		OUTYY(("P(control_interface:%s)\n", (yyvsp[(2) - (2)].str)));
		if(!cfg_strlist_insert(&cfg_parser->cfg->control_ifs, (yyvsp[(2) - (2)].str)))
			yyerror("out of memory");
	}
    break;

  case 285:
/* Line 1792 of yacc.c  */
#line 1446 "./util/configparser.y"
    {
		OUTYY(("P(control_use_cert:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->remote_control_use_cert =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
		free((yyvsp[(2) - (2)].str));
	}
    break;

  case 286:
/* Line 1792 of yacc.c  */
#line 1456 "./util/configparser.y"
    {
		OUTYY(("P(rc_server_key_file:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->server_key_file);
		cfg_parser->cfg->server_key_file = (yyvsp[(2) - (2)].str);
	}
    break;

  case 287:
/* Line 1792 of yacc.c  */
#line 1463 "./util/configparser.y"
    {
		OUTYY(("P(rc_server_cert_file:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->server_cert_file);
		cfg_parser->cfg->server_cert_file = (yyvsp[(2) - (2)].str);
	}
    break;

  case 288:
/* Line 1792 of yacc.c  */
#line 1470 "./util/configparser.y"
    {
		OUTYY(("P(rc_control_key_file:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->control_key_file);
		cfg_parser->cfg->control_key_file = (yyvsp[(2) - (2)].str);
	}
    break;

  case 289:
/* Line 1792 of yacc.c  */
#line 1477 "./util/configparser.y"
    {
		OUTYY(("P(rc_control_cert_file:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->control_cert_file);
		cfg_parser->cfg->control_cert_file = (yyvsp[(2) - (2)].str);
	}
    break;

  case 290:
/* Line 1792 of yacc.c  */
#line 1484 "./util/configparser.y"
    {
		OUTYY(("\nP(dnstap:)\n"));
	}
    break;

  case 305:
/* Line 1792 of yacc.c  */
#line 1501 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_enable:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 306:
/* Line 1792 of yacc.c  */
#line 1509 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_socket_path:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->dnstap_socket_path);
		cfg_parser->cfg->dnstap_socket_path = (yyvsp[(2) - (2)].str);
	}
    break;

  case 307:
/* Line 1792 of yacc.c  */
#line 1516 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_send_identity:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_send_identity = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 308:
/* Line 1792 of yacc.c  */
#line 1524 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_send_version:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_send_version = (strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 309:
/* Line 1792 of yacc.c  */
#line 1532 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_identity:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->dnstap_identity);
		cfg_parser->cfg->dnstap_identity = (yyvsp[(2) - (2)].str);
	}
    break;

  case 310:
/* Line 1792 of yacc.c  */
#line 1539 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_version:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->dnstap_version);
		cfg_parser->cfg->dnstap_version = (yyvsp[(2) - (2)].str);
	}
    break;

  case 311:
/* Line 1792 of yacc.c  */
#line 1546 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_resolver_query_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_resolver_query_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 312:
/* Line 1792 of yacc.c  */
#line 1555 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_resolver_response_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_resolver_response_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 313:
/* Line 1792 of yacc.c  */
#line 1564 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_client_query_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_client_query_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 314:
/* Line 1792 of yacc.c  */
#line 1573 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_client_response_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_client_response_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 315:
/* Line 1792 of yacc.c  */
#line 1582 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_forwarder_query_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_forwarder_query_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 316:
/* Line 1792 of yacc.c  */
#line 1591 "./util/configparser.y"
    {
		OUTYY(("P(dt_dnstap_log_forwarder_response_messages:%s)\n", (yyvsp[(2) - (2)].str)));
		if(strcmp((yyvsp[(2) - (2)].str), "yes") != 0 && strcmp((yyvsp[(2) - (2)].str), "no") != 0)
			yyerror("expected yes or no.");
		else cfg_parser->cfg->dnstap_log_forwarder_response_messages =
			(strcmp((yyvsp[(2) - (2)].str), "yes")==0);
	}
    break;

  case 317:
/* Line 1792 of yacc.c  */
#line 1600 "./util/configparser.y"
    { 
		OUTYY(("\nP(python:)\n")); 
	}
    break;

  case 321:
/* Line 1792 of yacc.c  */
#line 1609 "./util/configparser.y"
    {
		OUTYY(("P(python-script:%s)\n", (yyvsp[(2) - (2)].str)));
		free(cfg_parser->cfg->python_script);
		cfg_parser->cfg->python_script = (yyvsp[(2) - (2)].str);
	}
    break;


/* Line 1792 of yacc.c  */
#line 4124 "util/configparser.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2055 of yacc.c  */
#line 1614 "./util/configparser.y"


/* parse helper routines could be here */
