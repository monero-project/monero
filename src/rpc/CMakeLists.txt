# Copyright (c) 2014-2024, The Monero Project
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

include_directories(SYSTEM ${ZMQ_INCLUDE_PATH})

set(rpc_base_sources
  rpc_args.cpp
  rpc_payment_signature.cpp
  rpc_handler.cpp)

set(rpc_sources
  bootstrap_daemon.cpp
  bootstrap_node_selector.cpp
  core_rpc_server.cpp
  rpc_payment.cpp
  rpc_version_str.cpp
  instanciations.cpp)

set(daemon_messages_sources
  message.cpp
  daemon_messages.cpp)

set(rpc_pub_sources zmq_pub.cpp)

set(daemon_rpc_server_sources
  daemon_handler.cpp
  zmq_pub.cpp
  zmq_server.cpp)


set(rpc_base_headers
  rpc_args.h
  rpc_payment_signature.h
  rpc_handler.h)

set(rpc_headers
  rpc_version_str.h
  rpc_handler.h)

set(rpc_pub_headers zmq_pub.h)

set(daemon_rpc_server_headers)

set(rpc_private_headers
  bootstrap_daemon.h
  core_rpc_server.h
  rpc_payment.h
  core_rpc_server_commands_defs.h
  core_rpc_server_error_codes.h)

set(daemon_messages_private_headers
  message.h
  daemon_messages.h)

set(daemon_rpc_server_private_headers
  message.h
  daemon_messages.h
  daemon_handler.h
  zmq_server.h)


monero_private_headers(rpc
  ${rpc_private_headers})

set(rpc_pub_private_headers)

monero_private_headers(daemon_rpc_server
  ${daemon_rpc_server_private_headers})


monero_add_library(rpc_base
  ${rpc_base_sources}
  ${rpc_base_headers}
  ${rpc_base_private_headers})

monero_add_library(rpc
  ${rpc_sources}
  ${rpc_headers}
  ${rpc_private_headers})

monero_add_library(rpc_pub
  ${rpc_pub_sources}
  ${rpc_pub_headers}
  ${rpc_pub_private_headers})

monero_add_library(daemon_messages
  ${daemon_messages_sources}
  ${daemon_messages_headers}
  ${daemon_messages_private_headers})

monero_add_library(daemon_rpc_server
  ${daemon_rpc_server_sources}
  ${daemon_rpc_server_headers}
  ${daemon_rpc_server_private_headers})


target_link_libraries(rpc_base
  PUBLIC
    common
    epee
    ${Boost_REGEX_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})

target_link_libraries(rpc
  PUBLIC
    rpc_base
    common
    cryptonote_core
    cryptonote_protocol
    net
    version
    ${Boost_REGEX_LIBRARY}
    ${Boost_THREAD_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})

target_link_libraries(rpc_pub
  PUBLIC
    epee
    net
    cryptonote_basic
    serialization
    ${Boost_THREAD_LIBRARY})

target_link_libraries(daemon_messages
  LINK_PRIVATE
    cryptonote_core
    cryptonote_protocol
    version
    serialization
    ${EXTRA_LIBRARIES})

target_link_libraries(daemon_rpc_server
  LINK_PRIVATE
    rpc
    rpc_pub
    cryptonote_core
    cryptonote_protocol
    version
    daemon_messages
    serialization
    ${Boost_CHRONO_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${ZMQ_LIB}
    ${EXTRA_LIBRARIES})
target_include_directories(daemon_rpc_server PUBLIC ${ZMQ_INCLUDE_PATH})
target_include_directories(obj_daemon_rpc_server PUBLIC ${ZMQ_INCLUDE_PATH})
