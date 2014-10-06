# Copyright (c) 2014, The Monero Project
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

find_package(MiniUpnpc QUIET)

# FreeBSD doesn't play well with the local copy, so default to using shared
SET(USE_SHARED_MINIUPNPC true)

# If we have the correct shared version and we're not building static, use it
IF(!MINIUPNP_FOUND OR !MINIUPNPC_VERSION_1_7_OR_HIGHER OR STATIC)
 SET(USE_SHARED_MINIUPNPC false)
ENDIF()

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
  
  set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
  set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
  set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")
  add_subdirectory(miniupnpc)

  set_property(TARGET upnpc-static PROPERTY FOLDER "external")
  if(MSVC)
    set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -wd4244 -wd4267")
  elseif(NOT MSVC)
    set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value")
  endif()

  set(UPNP_STATIC true PARENT_SCOPE)
  set(UPNP_LIBRARIES "upnpc-static" PARENT_SCOPE)
endif()

find_package(Unbound)

IF(!UNBOUND_INCLUDE_DIR OR STATIC)
  if(STATIC)
    message(STATUS "Using libunbound from local source tree for static build")
  else()
    message(STATUS "Using libunbound from local source tree (/external/unbound)")
  endif()
  
  INCLUDE(ExternalProject)
  
  FIND_PACKAGE(OpenSSL QUIET)
  IF(!OPENSSL_LIBRARIES)
    MESSAGE(FATAL_ERROR "${BoldRed}Could not find the openssl library. Please make sure you have installed openssl or libssl-dev or the equivalent${ColourReset}")
  ELSE()
    MESSAGE(STATUS "Found openssl libraries")
  ENDIF()

  FIND_PACKAGE(Expat QUIET)
  IF(!EXPAT_LIBRARIES)
    MESSAGE(FATAL_ERROR "${BoldRed}Could not find the expat library. Please make sure you have installed libexpat or libexpat-dev or the equivalent${ColourReset}")
  ELSE()
    MESSAGE(STATUS "Found expat libraries")
  ENDIF()
  
  IF(MINGW)
    set(ENV{USE_WINSOCK} 1)
    set(ENV{CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    set(ENV{GCC_PREFIX} ${GCC_PREFIX})
    set(ENV{CMAKE_FIND_ROOT_PATH} ${CMAKE_FIND_ROOT_PATH})
    EXTERNALPROJECT_ADD(
      libunbound
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/unbound
      URL ${CMAKE_CURRENT_SOURCE_DIR}/unbound/
      CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/unbound/monero-config.sh
      BUILD_COMMAND $(MAKE)
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      INSTALL_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libtool --mode=install cp ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libunbound.la ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build
      LOG_DOWNLOAD 1
      LOG_UPDATE 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
      LOG_TEST 1
      LOG_INSTALL 1
    )
  ELSEIF(APPLE)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXTERNALPROJECT_ADD(
      libunbound
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/unbound
      URL ${CMAKE_CURRENT_SOURCE_DIR}/unbound/
      CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/unbound/configure --prefix=${BREW_PREFIX} --enable-shared --enable-static --disable-gost --disable-rpath --with-libevent=no --without-pyunbound --without-pythonmodule --without-pthreads --with-libunbound-only
      BUILD_COMMAND $(MAKE)
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      INSTALL_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libtool --mode=install cp ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libunbound.la ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build
      LOG_DOWNLOAD 1
      LOG_UPDATE 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
      LOG_TEST 1
      LOG_INSTALL 1
    )
  ELSE()
    EXTERNALPROJECT_ADD(
      libunbound
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/unbound
      URL ${CMAKE_CURRENT_SOURCE_DIR}/unbound/
      CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/unbound/configure --disable-shared --enable-static --disable-gost --disable-rpath --with-libevent=no --without-pyunbound --without-pythonmodule --without-pthreads --with-libunbound-only
      BUILD_COMMAND $(MAKE)
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      INSTALL_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libtool --mode=install cp ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libunbound.la ${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build
      LOG_DOWNLOAD 1
      LOG_UPDATE 1
      LOG_CONFIGURE 1
      LOG_BUILD 1
      LOG_TEST 1
      LOG_INSTALL 1
    )
  ENDIF()
  
  set(UNBOUND_STATIC true PARENT_SCOPE)
  set(UNBOUND_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build" PARENT_SCOPE)
  set(UNBOUND_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/unbound/src/libunbound-build/libunbound.a;${OPENSSL_LIBRARIES}" PARENT_SCOPE)
ELSE()
  MESSAGE(STATUS "Found libunbound include (unbound.h) in ${UNBOUND_INCLUDE_DIR}")
  IF(UNBOUND_LIBRARIES)
    MESSAGE(STATUS "Found libunbound shared library")
    set(UNBOUND_STATIC false PARENT_SCOPE)
    set(UNBOUND_INCLUDE ${UNBOUND_INCLUDE_DIR} PARENT_SCOPE)
    set(UNBOUND_LIBRARY ${UNBOUND_LIBRARIES} PARENT_SCOPE)
  ELSE()
    MESSAGE(FATAL_ERROR "${BoldRed}Found libunbound includes, but could not find libunbound library. Please make sure you have installed libunbound or libunbound-dev or the equivalent${ColourReset}")
  ENDIF()
ENDIF()
