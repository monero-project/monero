include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)

# Need expat.

check_include_file(arpa/inet.h HAVE_ARPA_INET_H)
check_include_file(endian.h HAVE_ENDIAN_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(event.h HAVE_EVENT_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(glob.h HAVE_GLOB_H)
check_include_file(grp.h HAVE_GRP_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(iphlpapi.h HAVE_IPHLPAPI_H)
check_include_file(login_cap.h HAVE_LOGIN_CAP_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(pthread.h HAVE_PTHREAD)
check_include_file(pwd.h HAVE_PWD_H)
check_include_file(stdarg.h HAVE_STDARG_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/sha2.h HAVE_SYS_SHA2_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/sysctl.h HAVE_SYS_SYSCTL_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/uio.h HAVE_SYS_UIO_H)
check_include_file(sys/un.h HAVE_SYS_UN_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(syslog.h HAVE_SYSLOG_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(vfork.h HAVE_VFORK_H)
check_include_file(windows.h HAVE_WINDOWS_H)
check_include_file(winsock2.h HAVE_WINSOCK2_H)
check_include_file(ws2tcpip.h HAVE_WS2TCPIP_H)

if (WIN32)
  set(CMAKE_REQUIRED_LIBRARIES
    iphlpapi
    ws2_32)
endif ()
if (CMAKE_SYSTEM_NAME MATCHES "(SunOS|Solaris)")
  set(CMAKE_REQUIRED_LIBRARIES
    socket
    nsl)
endif ()

check_function_exists(_beginthreadex HAVE__BEGINTHREADEX)
check_function_exists(arc4random HAVE_ARC4RANDOM)
check_function_exists(arc4random_uniform HAVE_ARC4RANDOM_UNIFORM)
check_function_exists(chown HAVE_CHOWN)
check_function_exists(chroot HAVE_CHROOT)
check_function_exists(ctime_r HAVE_CTIME_R)
check_function_exists(daemon HAVE_DAEMON)
check_function_exists(endprotoent HAVE_ENDPROTOENT)
check_function_exists(endservent HAVE_ENDSERVENT)
check_function_exists(fork HAVE_FORK)
check_function_exists(fseeko HAVE_FSEEKO)
check_function_exists(fsync HAVE_FSYNC)
check_function_exists(getauxval HAVE_GETAUXVAL)
check_function_exists(getentropy HAVE_GETENTROPY)
check_function_exists(getpwnam HAVE_GETPWNAM)
check_function_exists(getrlimit HAVE_GETRLIMIT)
check_function_exists(glob HAVE_GLOB)
check_function_exists(gmtime_r HAVE_GMTIME_R)
check_function_exists(fcntl HAVE_FCNTL)
check_function_exists(inet_aton HAVE_INET_ATON)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_function_exists(inet_pton HAVE_INET_PTON)
check_function_exists(initgroups HAVE_INITGROUPS)
check_function_exists(ioctlsocket HAVE_IOCTLSOCKET)
check_function_exists(isblank HAVE_ISBLANK)
check_function_exists(kill HAVE_KILL)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(random HAVE_RANDOM)
check_function_exists(reallocarray HAVE_DECL_REALLOCARRAY)
check_function_exists(recvmsg HAVE_RECVMSG)
check_function_exists(sbrk HAVE_SBRK)
check_function_exists(sendmsg HAVE_SENDMSG)
check_function_exists(setregid HAVE_SETREGID)
check_function_exists(setresgid HAVE_SETRESGID)
check_function_exists(setresuid HAVE_SETRESUID)
check_function_exists(setreuid HAVE_SETREUID)
check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(setsid HAVE_SETSID)
check_function_exists(setusercontent HAVE_SETUSERCONTENT)
check_function_exists(sigprocmask HAVE_SIGPROCMASK)
check_function_exists(sleep HAVE_SLEEP)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(socketpair HAVE_SOCKETPAIR)
check_function_exists(srandom HAVE_SRANDOM)
check_function_exists(strsep HAVE_STRSEP)
check_function_exists(strftime HAVE_STRFTIME)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(strptime HAVE_STRPTIME)
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(tzset HAVE_TZSET)
check_function_exists(usleep HAVE_USLEEP)
check_function_exists(writev HAVE_WRITEV)
check_function_exists(_beginthreadex HAVE__BEGINTHREADEX)

set(getaddrinfo_headers)
if (HAVE_NETDB_H)
  list(APPEND getaddrinfo_headers "netdb.h")
endif ()
if (HAVE_WS2TCPIP_H)
  list(APPEND getaddrinfo_headers "ws2tcpip.h")
endif ()
check_symbol_exists(getaddrinfo "${getaddrinfo_headers}" HAVE_GETADDRINFO)

check_function_exists(getaddrinfo HAVE_GETADDRINFO)

function (check_type_exists type variable header default)
  set(CMAKE_EXTRA_INCLUDE_FILES "${header}")
  check_type_size("${type}" "${variable}")

  if (NOT HAVE_${type})
    set("${variable}" "${default}" PARENT_SCOPE)
  else ()
    set("${variable}" "FALSE" PARENT_SCOPE)
  endif ()
endfunction ()

set(CMAKE_EXTRA_INCLUDE_FILES "time.h")
check_type_size(time_t SIZEOF_TIME_T)
set(CMAKE_EXTRA_INCLUDE_FILES)

check_type_exists(gid_t gid_t "sys/types.h" int)
check_type_exists(in_addr_t in_addr_t "netinet/in.h" uint32_t)
check_type_exists(in_port_t in_port_t "netinet/in.h" uint16_t)
check_type_exists(int16_t int16_t "sys/types.h" short)
check_type_exists(int32_t int32_t "sys/types.h" int)
check_type_exists(int64_t int64_t "sys/types.h" __int64)
check_type_exists(int8_t int8_t "sys/types.h" char)
check_type_exists(pid_t pid_t "sys/types.h" int)
check_type_exists(rlim_t rlim_t "sys/resource.h" "unsigned long")
check_type_exists(ssize_t ssize_t "sys/types.h" int)
check_type_exists(uid_t uid_t "sys/types.h" int)
check_type_exists(uint16_t uint16_t "sys/types.h" "unsigned short")
check_type_exists(uint32_t uint32_t "sys/types.h" "unsigned int")
check_type_exists(uint64_t uint64_t "sys/types.h" "unsigned long long")
check_type_exists(uint8_t uint8_t "sys/types.h" "unsigned char")

if (WIN32)
  set(UB_ON_WINDOWS 1)
endif ()

if (MSVC)
  set(inline __inline)
  set(__func__ __FUNCTION__)
endif ()

if (NOT HAVE_VFORK)
  set(vfork fork)
endif ()

# XXX: Check for broken malloc()?
# XXX: Check for broken memcmp()?
# XXX: Check for broken vfork()?
# XXX: Check for one-arg mkdir?

check_symbol_exists(inet_pton "arpa/inet.h" HAVE_INET_PTON)
check_symbol_exists(inet_ntop "arpa/inet.h" HAVE_INET_NTOP)

check_symbol_exists(strsep "string.h" HAVE_STRSEP)

check_symbol_exists(PTHREAD_PRIO_INHERIT "pthread.h" HAVE_PTHREAD_PRIO_INHERIT)
check_symbol_exists(pthread_rwlock_t "pthread.h" HAVE_PTHREAD_RWLOCK_T)
check_symbol_exists(pthread_spinlock_t "pthread.h" HAVE_PTHREAD_SPINLOCK_T)

# openssl
set(CMAKE_REQUIRED_INCLUDES
  ${OPENSSL_INCLUDE_DIR})

check_include_file(openssl/conf.h HAVE_OPENSSL_CONF_H)
check_include_file(openssl/engine.h HAVE_OPENSSL_ENGINE_H)
check_include_file(openssl/err.h HAVE_OPENSSL_ERR_H)
check_include_file(openssl/rand.h HAVE_OPENSSL_RAND_H)
check_include_file(openssl/ssl.h HAVE_OPENSSL_SSL_H)

set(CMAKE_REQUIRED_INCLUDES)

check_symbol_exists(NID_secp384r1 "openssl/evp.h" HAVE_DECL_NID_SECP384R1)
check_symbol_exists(NID_X9_62_prime256v1 "openssl/evp.h" HAVE_DECL_NID_X9_62_PRIME256V1)
check_symbol_exists(sk_SSL_COMP_pop_free "openssl/ssl.h" HAVE_DECL_SK_SSL_COMP_POP_FREE)
check_symbol_exists(SSL_COMP_get_compression_methods "openssl/ssl.h" HAVE_DECL_SSL_COMP_GET_COMPRESSION_METHODS)

set(CMAKE_REQUIRED_LIBRARIES
  ${OPENSSL_LIBRARIES})

check_function_exists(EVP_MD_CTX_new HAVE_EVP_MD_CTX_NEW)
check_function_exists(EVP_sha1 HAVE_EVP_SHA1)
check_function_exists(EVP_sha256 HAVE_EVP_SHA256)
check_function_exists(EVP_sha512 HAVE_EVP_SHA512)
check_function_exists(FIPS_mode HAVE_FIPS_MODE)
check_function_exists(HMAC_Update HAVE_HMAC_UPDATE)
check_function_exists(OPENSSL_config HAVE_OPENSSL_CONFIG)
check_function_exists(SHA512_Update HAVE_SHA512_UPDATE)

set(CMAKE_REQUIRED_LIBRARIES)

set(UNBOUND_CONFIGFILE "${CMAKE_INSTALL_PREFIX}/etc/unbound/unbound.conf"
 CACHE STRING "default configuration file")
set(UNBOUND_USERNAME "unbound"
 CACHE STRING "default user that unbound changes to")
set(UNBOUND_CHROOT_DIR "${CMAKE_INSTALL_PREFIX}/etc/unbound"
 CACHE STRING "default directory to chroot to")
set(UNBOUND_RUN_DIR "${CMAKE_INSTALL_PREFIX}/etc/unbound"
 CACHE STRING "default directory to chroot to")
set(UNBOUND_SHARE_DIR "${CMAKE_INSTALL_PREFIX}/etc/unbound"
 CACHE STRING "default directory with shared data")
set(UNBOUND_PIDFILE "${CMAKE_INSTALL_PREFIX}/etc/unbound/unbound.pid"
 CACHE STRING "default pathname to the pidfile")

# Copied from configure.ac.
set(WINVER 0x0502)
set(PACKAGE_VERSION "1.5.8")
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(MAXSYSLOGMSGLEN 10240)

# Make assumptions.
set(HAVE_WORKING_FORK ${HAVE_FORK})
set(HAVE_WORKING_VFORK ${HAVE_VFORK})
