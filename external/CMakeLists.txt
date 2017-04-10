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
# 
# Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers


# This is broken up into two parts: first we check for miniupnp, compile it if we can't
# find it, and thereafter we check for libunbound, and compile it if we can't find it.
# We always compile if we are building statically to reduce static dependency issues...
# ...except for FreeBSD, because FreeBSD is a special case that doesn't play well with
# others.
if(NOT IOS)
    find_package(Miniupnpc QUIET)
endif()

# If we have the correct shared version and we're not building static, use it
if(STATIC OR IOS)
 set(USE_SHARED_MINIUPNPC false)
elseif(MINIUPNP_FOUND AND MINIUPNPC_VERSION_1_7_OR_HIGHER)
 set(USE_SHARED_MINIUPNPC true)
endif()

if(USE_SHARED_MINIUPNPC)
  message(STATUS "Using shared miniupnpc found at ${MINIUPNP_INCLUDE_DIR}")

  set(UPNP_STATIC false PARENT_SCOPE)
  set(UPNP_INCLUDE ${MINIUPNP_INCLUDE_DIR} PARENT_SCOPE)
  set(UPNP_LIBRARIES ${MINIUPNP_LIBRARY} PARENT_SCOPE)
else()
  if(STATIC)
    message(STATUS "Using miniupnpc from local source tree for static build")
  else()
    message(STATUS "Using miniupnpc from local source tree (/external/miniupnpc)")
  endif()

  add_subdirectory(miniupnpc)

  set_property(TARGET libminiupnpc-static PROPERTY FOLDER "external")
  if(MSVC)
    set_property(TARGET libminiupnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -wd4244 -wd4267")
  elseif(NOT MSVC)
    set_property(TARGET libminiupnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value")
  endif()

  set(UPNP_STATIC true PARENT_SCOPE)
  set(UPNP_LIBRARIES "libminiupnpc-static" PARENT_SCOPE)
endif()

find_package(Unbound)

if(NOT UNBOUND_INCLUDE_DIR OR STATIC)
  # NOTE: If STATIC is true, CMAKE_FIND_LIBRARY_SUFFIXES has been reordered.
  # unbound has config tests which used OpenSSL libraries, so -ldl may need to
  # be set in this case.
  # The unbound CMakeLists.txt can set it, since it's also needed for the
  # static OpenSSL libraries set up there after with target_link_libraries.
  add_subdirectory(unbound)

  set(UNBOUND_STATIC true PARENT_SCOPE)
  set(UNBOUND_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/unbound/libunbound" PARENT_SCOPE)
  set(UNBOUND_LIBRARY "unbound" PARENT_SCOPE)
  set(UNBOUND_LIBRARY_DIRS "${LIBEVENT2_LIBDIR}" PARENT_SCOPE)
else()
  message(STATUS "Found libunbound include (unbound.h) in ${UNBOUND_INCLUDE_DIR}")
  if(UNBOUND_LIBRARIES)
    message(STATUS "Found libunbound shared library")
    set(UNBOUND_STATIC false PARENT_SCOPE)
    set(UNBOUND_INCLUDE ${UNBOUND_INCLUDE_DIR} PARENT_SCOPE)
    set(UNBOUND_LIBRARY ${UNBOUND_LIBRARIES} PARENT_SCOPE)
    set(UNBOUND_LIBRARY_DIRS "" PARENT_SCOPE)
  else()
    die("Found libunbound includes, but could not find libunbound library. Please make sure you have installed libunbound or libunbound-dev or the equivalent")
  endif()
endif()

add_subdirectory(db_drivers)
add_subdirectory(easylogging++)
