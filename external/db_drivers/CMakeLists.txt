# Copyright (c) 2014-2015, The Monero Project
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

# We aren't even going to check the system for an installed LMDB driver, as it is too
# critical a consensus component to rely on dynamically linked libraries
message(STATUS "Using ${ARCH_WIDTH}-bit LMDB from source tree")
add_subdirectory(liblmdb${ARCH_WIDTH})
set(LMDB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/liblmdb${ARCH_WIDTH}" CACHE STRING "LMDB Include path")

set(LMDB_LIBRARY "lmdb" CACHE STRING "LMDB Library name")

if (NOT STATIC)
  find_package(BerkeleyDB)

  if(NOT BERKELEY_DB_LIBRARIES OR STATIC)
    add_subdirectory(libdb)
    message(STATUS "BerkeleyDB not found, building from src tree")

    set(BDB_STATIC true CACHE BOOL "BDB Static flag")
    set(BDB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/libdb" CACHE STRING "BDB include path")
    set(BDB_LIBRARY "db" CACHE STRING "BDB library name")
  else()
    message(STATUS "Found BerkeleyDB include (db.h) in ${BERKELEY_DB_INCLUDE_DIR}")
    if(BERKELEY_DB_LIBRARIES)
      message(STATUS "Found BerkeleyDB shared library")
      set(BDB_STATIC false CACHE BOOL "BDB Static flag")
      set(BDB_INCLUDE ${BERKELEY_DB_INCLUDE_DIR} CACHE STRING "BDB include path")
      set(BDB_LIBRARY ${BERKELEY_DB_LIBRARIES} CACHE STRING "BDB library name")
      set(BDB_LIBRARY_DIRS "" CACHE STRING "BDB Library dirs")
    else()
      die("Found BerkeleyDB includes, but could not find BerkeleyDB library. Please make sure you have installed libdb and libdb-dev or the equivalent")
    endif()
  endif()
endif()
