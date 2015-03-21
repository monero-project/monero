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

set(server_ipc_sources
  wap_server/wap_server.c
  wap_proto.c
  daemon_ipc_handlers.cpp)

set(client_ipc_sources
  wap_client/wap_client.c
  wap_proto.c)

set_source_files_properties(${server_ipc_sources} ${client_ipc_sources} PROPERTIES LANGUAGE CXX)
set_source_files_properties(${server_ipc_sources} ${client_ipc_sources} PROPERTIES COMPILE_FLAGS "-Wno-write-strings -Wno-error -fpermissive")

set(server_ipc_headers)
set(client_ipc_headers)

bitmonero_add_library(server_ipc
  ${server_ipc_sources}
  ${server_ipc_headers})
target_link_libraries(server_ipc
  LINK_PRIVATE
    cryptonote_core
    ${ZMQ_LIB}
    ${CZMQ_LIB})

bitmonero_add_library(client_ipc
  ${client_ipc_sources}
  ${client_ipc_headers})
target_link_libraries(client_ipc
  LINK_PRIVATE
    cryptonote_core
    ${ZMQ_LIB}
    ${CZMQ_LIB})
