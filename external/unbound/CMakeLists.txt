cmake_minimum_required(VERSION 2.8.7)

project(unbound C)

find_package(OpenSSL REQUIRED)
find_package(Threads)

include(configure_checks.cmake)

if (WIN32)
  set(USE_MINI_EVENT 1)
  set(USE_WINSOCK 1)
else ()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBEVENT2 REQUIRED libevent)
endif ()

set(RETSIGTYPE void)

add_definitions(-D_GNU_SOURCE)

option(USE_ECDSA "Use ECDSA algorithms" ON)
option(USE_SHA2 "Enable SHA2 support" ON)
set(ENABLE_DNSTAP 0)
set(HAVE_SSL 1)
if (CMAKE_USE_PTHREADS_INIT AND NOT CMAKE_USE_WIN32_THREADS_INIT)
  set(HAVE_PTHREAD 1)
else ()
  set(HAVE_PTHREAD 0)
endif ()
if (CMAKE_USE_WIN32_THREADS_INIT)
  set(HAVE_WINDOWS_THREADS 1)
else ()
  set(HAVE_WINDOWS_THREADS 0)
endif ()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/dnstap/dnstap_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dnstap/dnstap_config.h")

set(common_src
  services/cache/dns.c
  services/cache/infra.c
  services/cache/rrset.c
  util/data/dname.c
  util/data/msgencode.c
  util/data/msgparse.c
  util/data/msgreply.c
  util/data/packed_rrset.c
  iterator/iterator.c
  iterator/iter_delegpt.c
  iterator/iter_donotq.c
  iterator/iter_fwd.c
  iterator/iter_hints.c
  iterator/iter_priv.c
  iterator/iter_resptype.c
  iterator/iter_scrub.c
  iterator/iter_utils.c
  services/listen_dnsport.c
  services/localzone.c
  services/mesh.c
  services/modstack.c
  services/outbound_list.c
  services/outside_network.c
  util/alloc.c
  util/config_file.c
  util/configlexer.c
  util/configparser.c
  util/fptr_wlist.c
  util/locks.c
  util/log.c
  util/mini_event.c
  util/module.c
  util/netevent.c
  util/net_help.c
  util/random.c
  util/rbtree.c
  util/regional.c
  util/rtt.c
  util/storage/dnstree.c
  util/storage/lookup3.c
  util/storage/lruhash.c
  util/storage/slabhash.c
  util/timehist.c
  util/tube.c
  util/winsock_event.c
  validator/autotrust.c
  validator/val_anchor.c
  validator/validator.c
  validator/val_kcache.c
  validator/val_kentry.c
  validator/val_neg.c
  validator/val_nsec3.c
  validator/val_nsec.c
  validator/val_secalgo.c
  validator/val_sigcrypt.c
  validator/val_utils.c
  dns64/dns64.c

  #$(CHECKLOCK_SRC)
  testcode/checklocks.c)

set(compat_src)

foreach (symbol IN ITEMS ctime_r gmtime_r inet_aton inet_ntop inet_pton malloc memcmp memmove snprintf strlcat strlcpy strptime explicit_bzero arc4random arc4random_uniform sha512)
  string(TOUPPER "${symbol}" upper_sym)
  if (NOT HAVE_${upper_sym})
    list(APPEND compat_src
      compat/${symbol}.c)
  endif ()
endforeach ()

if (NOT HAVE_ARC4RANDOM)
  list(APPEND compat_src
    compat/arc4_lock.c)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND compat_src
    compat/getentropy_linux.c)
elseif (APPLE)
  list(APPEND compat_src
    compat/getentropy_osx.c)
#elseif (SunOS)
#  list(APPEND compat_src
#    compat/getentropy_solaris.c)
elseif (WIN32)
  list(APPEND compat_src
    compat/getentropy_win.c)
endif ()

if (NOT HAVE_GETADDRINFO)
  list(APPEND compat_src
    compat/fake-rfc2553.c)
endif ()

set(sldns_src
  ldns/keyraw.c
  ldns/sbuffer.c
  ldns/wire2str.c
  ldns/parse.c
  ldns/parseutil.c
  ldns/rrdef.c
  ldns/str2wire.c)

set(libunbound_src
  libunbound/context.c
  libunbound/libunbound.c
  libunbound/libworker.c)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
if (LIBEVENT2_FOUND)
  include_directories(SYSTEM ${LIBEVENT2_INCLUDE_DIRS})
  link_directories(${LIBEVENT2_LIBRARY_DIRS})
endif ()
add_library(unbound
  ${common_src}
  ${sldns_src}
  ${compat_src}
  ${libunbound_src})
target_link_libraries(unbound
  LINK_PRIVATE
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
if (LIBEVENT2_FOUND)
  target_link_libraries(unbound
    LINK_PRIVATE
      ${LIBEVENT2_LIBRARIES})
endif ()

if (WIN32)
  target_link_libraries(unbound
    LINK_PRIVATE
      iphlpapi
      ws2_32)
endif ()

# XXX: Hack for static builds.
set(LIBEVENT2_LIBDIR
  "${LIBEVENT2_LIBDIR}"
  PARENT_SCOPE)

if (MINGW)
  # There is no variable for this (probably due to the fact that the pthread
  # library is implicit with a link in msys).
  find_library(win32pthread
    NAMES libwinpthread-1.dll)
  foreach (input IN LISTS win32pthread OPENSSL_LIBRARIES)
    # Copy shared libraries into the build tree so that no PATH manipulation is
    # necessary.
    get_filename_component(name "${input}" NAME)
    configure_file(
      "${input}"
      "${CMAKE_BINARY_DIR}/bin/${name}"
      COPYONLY)
  endforeach ()
endif ()
