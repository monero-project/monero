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

find_package(LMDB)

if(NOT LMDB_LIBRARIES OR STATIC)
  add_subdirectory(liblmdb)
  message(STATUS "lmdb not found, building from src tree")

  set(LMDB_STATIC true PARENT_SCOPE)
  set(LMDB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/liblmdb" PARENT_SCOPE)
  set(LMDB_LIBRARY "lmdb" PARENT_SCOPE)
else()
  message(STATUS "Found liblmdb include (lmdb.h) in ${LMDB_INCLUDE_DIR}")
  if(LMDB_LIBRARIES)
    message(STATUS "Found liblmdb shared library")
    set(LMDB_STATIC false PARENT_SCOPE)
    set(LMDB_INCLUDE ${LMDB_INCLUDE_DIR} PARENT_SCOPE)
    set(LMDB_LIBRARY ${LMDB_LIBRARIES} PARENT_SCOPE)
    set(LMDB_LIBRARY_DIRS "" PARENT_SCOPE)
  else()
    die("Found liblmdb includes, but could not find liblmdb library. Please make sure you have installed liblmdb and liblmdb-dev or the equivalent")
  endif()
endif()

find_package(BerkeleyDB)

if(NOT DB_LIBRARIES OR STATIC)
  add_subdirectory(libdb)
  message(STATUS "BerkeleyDB not found, building from src tree")

  set(BDB_STATIC true PARENT_SCOPE)
  set(BDB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/libdb" PARENT_SCOPE)
  set(BDB_LIBRARY "db" PARENT_SCOPE)
else()
  message(STATUS "Found BerkeleyDB include (db.h) in ${DB_INCLUDE_DIR}")
  if(DB_LIBRARIES)
    message(STATUS "Found BerkeleyDB shared library")
    set(BDB_STATIC false PARENT_SCOPE)
    set(BDB_INCLUDE ${DB_INCLUDE_DIR} PARENT_SCOPE)
    set(BDB_LIBRARY ${DB_LIBRARIES} PARENT_SCOPE)
    set(BDB_LIBRARY_DIRS "" PARENT_SCOPE)
  else()
    die("Found BerkeleyDB includes, but could not find BerkeleyDB library. Please make sure you have installed libdb and libdb-dev or the equivalent")
  endif()
endif()
