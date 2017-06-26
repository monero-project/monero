# Copyright (c) 2014-2017, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 2.8.7)

project(unbound C)

find_package(Threads)

include(configure_checks.cmake)

if (WIN32)
  set(USE_MINI_EVENT 1)
  set(USE_WINSOCK 1)
else ()
  find_package(PkgConfig REQUIRED)
  set(USE_MINI_EVENT 1)
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
  respip/respip.c
  services/listen_dnsport.c
  services/localzone.c
  services/mesh.c
  services/modstack.c
  services/outbound_list.c
  services/outside_network.c
  services/view.c
  util/alloc.c
  util/as112.c
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
  util/ub_event.c
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

foreach (symbol IN ITEMS ctime_r gmtime_r inet_aton inet_ntop inet_pton isblank malloc memmove snprintf strsep strlcat strlcpy strptime explicit_bzero arc4random arc4random_uniform reallocarray)
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
  sldns/keyraw.c
  sldns/sbuffer.c
  sldns/wire2str.c
  sldns/parse.c
  sldns/parseutil.c
  sldns/rrdef.c
  sldns/str2wire.c)

set(libunbound_src
  libunbound/context.c
  libunbound/libunbound.c
  libunbound/libworker.c)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
add_library(unbound
  ${common_src}
  ${sldns_src}
  ${compat_src}
  ${libunbound_src})
target_link_libraries(unbound
  PRIVATE
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
  target_link_libraries(unbound
    PRIVATE
      iphlpapi
      ws2_32)
endif ()

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


if (INSTALL_VENDORED_LIBUNBOUND)
    if(IOS)
        set(lib_folder lib-${ARCH})
    else()
        set(lib_folder lib)
    endif()
    install(TARGETS unbound
        ARCHIVE DESTINATION ${lib_folder})
endif()
