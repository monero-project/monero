# Copyright (c) 2014, The Monero Project
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

include (CheckIncludeFiles)
include (CheckLibraryExists)
include (CheckSymbolExists)

set (Unbound_FOUND FALSE)
MESSAGE("Attempting to find libunbound")

#FIND_PATH("unbound.h" CMAKE_HAVE_UNBOUND_H)
MESSAGE("CMAKE_INCLUDE_PATH: ${CMAKE_INCLUDE_PATH}")
MESSAGE("CMAKE_SYSTEM_INCLUDE_PATH: ${CMAKE_SYSTEM_INCLUDE_PATH}")
CHECK_INCLUDE_FILES("unbound.h" CMAKE_HAVE_UNBOUND_H)
MESSAGE("CMAKE_HAVE_UNBOUND_H: ${CMAKE_HAVE_UNBOUND_H}")

if(CMAKE_HAVE_UNBOUND_H)

  MESSAGE("unbound.h found")

  CHECK_LIBRARY_EXISTS(unbound ub_ctx_create "" CMAKE_HAVE_UNBOUND)

  if(CMAKE_HAVE_UNBOUND)
    MESSAGE("-lunbound works?")
    set(CMAKE_UNBOUND_LIB "-lunbound")
    set(Unbound_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Unbound DEFAULT_MSG Unbound_FOUND)
