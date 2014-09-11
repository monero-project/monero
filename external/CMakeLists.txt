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

# --------------------------------- FindMiniupnpc Start ---------------------------------
# Locate miniupnp library
# This module defines
#  MINIUPNP_FOUND, if false, do not try to link to miniupnp
#  MINIUPNP_LIBRARY, the miniupnp variant
#  MINIUPNP_INCLUDE_DIR, where to find miniupnpc.h and family)
#  MINIUPNPC_VERSION_PRE1_6 --> set if we detect the version of miniupnpc is
#                               pre 1.6
#  MINIUPNPC_VERSION_PRE1_5 --> set if we detect the version of miniupnpc is
#                               pre 1.5
#
# Note that the expected include convention is
#  #include "miniupnpc.h"
# and not
#  #include <miniupnpc/miniupnpc.h>
# This is because, the miniupnpc location is not standardized and may exist
# in locations other than miniupnpc/

#=============================================================================
# Copyright 2011 Mark Vejvoda
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

if (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)
	# Already in cache, be silent
	set(MINIUPNP_FIND_QUIETLY TRUE)
endif (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)

find_path(MINIUPNP_INCLUDE_DIR miniupnpc.h
	PATH_SUFFIXES miniupnpc)
find_library(MINIUPNP_LIBRARY miniupnpc)

if (MINIUPNP_INCLUDE_DIR AND MINIUPNP_LIBRARY)
	set (MINIUPNP_FOUND TRUE)
endif ()

if (MINIUPNP_FOUND)
	include(CheckCXXSourceRuns)
	if (NOT MINIUPNP_FIND_QUIETLY)
		message (STATUS "Found the miniupnpc libraries at ${MINIUPNP_LIBRARY}")
		message (STATUS "Found the miniupnpc headers at ${MINIUPNP_INCLUDE_DIR}")
	endif (NOT MINIUPNP_FIND_QUIETLY)

	message(STATUS "Detecting version of miniupnpc in path: ${MINIUPNP_INCLUDE_DIR}")

	set(CMAKE_REQUIRED_INCLUDES ${MINIUPNP_INCLUDE_DIR})
	set(CMAKE_REQUIRED_LIBRARIES ${MINIUPNP_LIBRARY})
	check_cxx_source_runs("
	#include <miniwget.h>
	#include <miniupnpc.h>
	#include <upnpcommands.h>
	#include <stdio.h>
	int main()
	{
	static struct UPNPUrls urls;
	static struct IGDdatas data;

	GetUPNPUrls (&urls, &data, \"myurl\",0);

	return 0;
	}"
	MINIUPNPC_VERSION_1_7_OR_HIGHER)

IF (NOT MINIUPNPC_VERSION_1_7_OR_HIGHER)
	set(CMAKE_REQUIRED_INCLUDES ${MINIUPNP_INCLUDE_DIR})
	set(CMAKE_REQUIRED_LIBRARIES ${MINIUPNP_LIBRARY})
	check_cxx_source_runs("
	#include <miniwget.h>
	#include <miniupnpc.h>
	#include <upnpcommands.h>
	#include <stdio.h>
	int main()
	{
	struct UPNPDev *devlist = NULL;
	int upnp_delay = 5000;
	const char *upnp_multicastif = NULL;
	const char *upnp_minissdpdsock = NULL;
	int upnp_sameport = 0;
	int upnp_ipv6 = 0;
	int upnp_error = 0;
	devlist = upnpDiscover(upnp_delay, upnp_multicastif, upnp_minissdpdsock, upnp_sameport, upnp_ipv6, &upnp_error);

	return 0;
	}"
	MINIUPNPC_VERSION_PRE1_7)
   ENDIF()

   IF (NOT MINIUPNPC_VERSION_PRE1_7 AND NOT MINIUPNPC_VERSION_1_7_OR_HIGHER)
	   set(CMAKE_REQUIRED_INCLUDES ${MINIUPNP_INCLUDE_DIR})
	   set(CMAKE_REQUIRED_LIBRARIES ${MINIUPNP_LIBRARY})
	   check_cxx_source_runs("
	   #include <miniwget.h>
	   #include <miniupnpc.h>
	   #include <upnpcommands.h>
	   #include <stdio.h>
	   int main()
	   {
	   struct UPNPDev *devlist = NULL;
	   int upnp_delay = 5000;
	   const char *upnp_multicastif = NULL;
	   const char *upnp_minissdpdsock = NULL;
	   int upnp_sameport = 0;
	   int upnp_ipv6 = 0;
	   int upnp_error = 0;
	   devlist = upnpDiscover(upnp_delay, upnp_multicastif, upnp_minissdpdsock, upnp_sameport);

	   return 0;
	   }"
	   MINIUPNPC_VERSION_PRE1_6)

   ENDIF()

   IF (NOT MINIUPNPC_VERSION_PRE1_6 AND NOT MINIUPNPC_VERSION_PRE1_7 AND NOT MINIUPNPC_VERSION_1_7_OR_HIGHER)
	   set(CMAKE_REQUIRED_INCLUDES ${MINIUPNP_INCLUDE_DIR})
	   set(CMAKE_REQUIRED_LIBRARIES ${MINIUPNP_LIBRARY})
	   check_cxx_source_runs("
	   #include <miniwget.h>
	   #include <miniupnpc.h>
	   #include <upnpcommands.h>
	   #include <stdio.h>
	   static struct UPNPUrls urls;
	   static struct IGDdatas data;
	   int main()
	   {
	   char externalIP[16]     = \"\";
	   UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIP);

	   return 0;
	   }"
	   MINIUPNPC_VERSION_1_5_OR_HIGHER)
	ENDIF()

	IF (NOT MINIUPNPC_VERSION_1_5_OR_HIGHER AND NOT MINIUPNPC_VERSION_PRE1_6 AND NOT MINIUPNPC_VERSION_PRE1_7 AND NOT MINIUPNPC_VERSION_1_7_OR_HIGHER)
		set(CMAKE_REQUIRED_INCLUDES ${MINIUPNP_INCLUDE_DIR})
		set(CMAKE_REQUIRED_LIBRARIES ${MINIUPNP_LIBRARY})
		check_cxx_source_runs("
		#include <miniwget.h>
		#include <miniupnpc.h>
		#include <upnpcommands.h>
		#include <stdio.h>
		static struct UPNPUrls urls;
		static struct IGDdatas data;
		int main()
		{
		char externalIP[16]     = \"\";
		UPNP_GetExternalIPAddress(urls.controlURL, data.servicetype, externalIP);

		return 0;
		}"
		MINIUPNPC_VERSION_PRE1_5)

ENDIF()

IF(MINIUPNPC_VERSION_PRE1_5)
	message(STATUS "Found miniupnpc version is pre v1.5")
ENDIF()
IF(MINIUPNPC_VERSION_PRE1_6)
	message(STATUS "Found miniupnpc version is pre v1.6")
ENDIF()
IF(MINIUPNPC_VERSION_PRE1_7)
	message(STATUS "Found miniupnpc version is pre v1.7")
ENDIF()

IF(NOT MINIUPNPC_VERSION_PRE1_5 AND NOT MINIUPNPC_VERSION_PRE1_6 AND NOT MINIUPNPC_VERSION_PRE1_7)
	IF(MINIUPNPC_VERSION_1_5_OR_HIGHER)
		message(STATUS "Found miniupnpc version is v1.5 or higher")
	ELSE()
		message(STATUS "Found miniupnpc version is v1.7 or higher")
	ENDIF()
ENDIF()

else ()
	message (STATUS "Could not find miniupnp")
endif ()

MARK_AS_ADVANCED(MINIUPNP_INCLUDE_DIR MINIUPNP_LIBRARY)
# --------------------------------- FindMiniupnpc End ---------------------------------

# And now on to the Monero part of things

if(MINIUPNP_FOUND AND MINIUPNPC_VERSION_1_7_OR_HIGHER)
  message(STATUS "Using shared miniupnpc found at ${MINIUPNP_INCLUDE_DIR}")

  set(UPNP_STATIC false PARENT_SCOPE)
  set(UPNP_INCLUDE ${MINIUPNP_INCLUDE_DIR} PARENT_SCOPE)
  set(UPNP_LIBRARIES ${MINIUPNP_LIBRARY} PARENT_SCOPE)
else()
  message(STATUS "Using static miniupnpc from external")

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
