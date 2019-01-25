# Copyright (c) 2014-2019, The Monero Project
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

# TODO(unassigned): the original copyright for this file actually extends from 2012-2014, not 2013

# The miniupnp project has, historically, be unreliable in providing timely releases after security patches.
#   Use this option to build your own miniupnpc (using Monero's mindful fork) to be used in monerod.
#   You may also need to keep this option off if your system *must* rely on using a system installation.
option(BUILD_MINIUPNPC "Build in-tree miniupnpc" OFF)

# Sekreta/Kovri provides miniupnpc from the same upstream source as Monero, and uses its own targets.
if (NOT BUILD_KOVRI)
  find_package(Miniupnpc REQUIRED)
  add_library(libminiupnpc INTERFACE)

  # In addition to allowing optional local building of miniupnpc, the following will
  #  require that the build is built locally if a system package only provides a shared
  #  library (like on Arch) even though we have chosen to build statically.
  if (NOT MINIUPNP_INCLUDE_DIR OR NOT MINIUPNP_LIBRARY)
    set(BUILD_MINIUPNPC ON)
  endif()

  if (BUILD_MINIUPNPC)
    if (NOT STATIC)
      set(UPNPC_BUILD_STATIC OFF CACHE BOOL "Disable static lib target" FORCE)
      set(UPNPC_BUILD_SHARED ON CACHE BOOL "Enable shared lib target" FORCE)
    endif()

    if (MINIUPNP_FOUND)
      message(WARNING "Ignoring system miniupnpc, forcing a local build")
    endif()

    message(STATUS "Using " ${CMAKE_CURRENT_SOURCE_DIR}/miniupnp/miniupnpc)
    add_subdirectory(miniupnp/miniupnpc)

    if (NOT STATIC)
      set(MINIUPNPC_TARGET libminiupnpc-shared)
    else()
      set(MINIUPNPC_TARGET libminiupnpc-static)
    endif()

    set(MINIUPNPC_FLAGS "")
    if(MSVC)
      list(APPEND MINIUPNPC_FLAGS "-wd4244 -wd4267")
    elseif(NOT MSVC)
      list(APPEND MINIUPNPC_FLAGS "-Wno-undef -Wno-unused-result -Wno-unused-value")
    endif()
    if(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
      list(APPEND MINIUPNPC_FLAGS "-D_NETBSD_SOURCE")
    endif()

    string(REPLACE ";" " " MINIUPNPC_FLAGS "${MINIUPNPC_FLAGS}" )
    set_property(TARGET ${MINIUPNPC_TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS "${MINIUPNPC_FLAGS}")
    set_property(TARGET ${MINIUPNPC_TARGET} PROPERTY FOLDER "external")

    target_link_libraries(libminiupnpc INTERFACE ${MINIUPNPC_TARGET})
  else()
    target_include_directories(libminiupnpc INTERFACE ${MINIUPNP_INCLUDE_DIR})
    target_link_libraries(libminiupnpc INTERFACE ${MINIUPNP_LIBRARY})
  endif()
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
add_subdirectory(randomx EXCLUDE_FROM_ALL)
