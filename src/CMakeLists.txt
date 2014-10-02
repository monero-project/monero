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

add_definitions(-DSTATICLIB)

file(GLOB_RECURSE COMMON common/*)
file(GLOB_RECURSE CRYPTO crypto/*)
file(GLOB_RECURSE CRYPTONOTE_CORE cryptonote_core/*)
file(GLOB_RECURSE CRYPTONOTE_PROTOCOL cryptonote_protocol/*)
file(GLOB_RECURSE DAEMON daemon/*)
file(GLOB_RECURSE P2P p2p/*)
file(GLOB_RECURSE RPC rpc/*)
file(GLOB_RECURSE SIMPLEWALLET simplewallet/*)
file(GLOB_RECURSE CONN_TOOL connectivity_tool/*)
file(GLOB_RECURSE WALLET wallet/*)
file(GLOB_RECURSE MINER miner/*)
file(GLOB MNEMONICS mnemonics/*)

source_group(common FILES ${COMMON})
source_group(crypto FILES ${CRYPTO})
source_group(cryptonote_core FILES ${CRYPTONOTE_CORE})
source_group(cryptonote_protocol FILES ${CRYPTONOTE_PROTOCOL})
source_group(daemon FILES ${DAEMON})
source_group(p2p FILES ${P2P})
source_group(rpc FILES ${RPC})
source_group(simplewallet FILES ${SIMPLEWALLET})
source_group(connectivity-tool FILES ${CONN_TOOL})
source_group(wallet FILES ${WALLET})
source_group(simpleminer FILES ${MINER})
source_group(mnemonics FILES ${MNEMONICS})

add_library(common ${COMMON})
add_library(crypto ${CRYPTO})
add_library(cryptonote_core ${CRYPTONOTE_CORE})
add_library(mnemonics ${MNEMONICS})
add_executable(daemon ${DAEMON} ${P2P} ${CRYPTONOTE_PROTOCOL})
add_executable(connectivity_tool ${CONN_TOOL})
add_executable(simpleminer ${MINER})
target_link_libraries(daemon rpc cryptonote_core crypto common ${UNBOUND_LIBRARIES} ${UPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries(connectivity_tool cryptonote_core crypto common ${UNBOUND_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
target_link_libraries(simpleminer cryptonote_core crypto common ${UNBOUND_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_library(rpc ${RPC})
add_library(wallet ${WALLET})
target_link_libraries(wallet mnemonics)
add_executable(simplewallet ${SIMPLEWALLET} )
target_link_libraries(simplewallet wallet rpc cryptonote_core crypto common mnemonics ${UNBOUND_LIBRARIES} ${UPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
add_dependencies(daemon version)
add_dependencies(rpc version)
add_dependencies(simplewallet version)

set_property(TARGET common crypto cryptonote_core rpc wallet PROPERTY FOLDER "libs")
set_property(TARGET daemon simplewallet connectivity_tool simpleminer PROPERTY FOLDER "prog")
if (STATIC)
  set_property(TARGET daemon simplewallet connectivity_tool simpleminer PROPERTY LINK_SEARCH_START_STATIC 1)
  set_property(TARGET daemon simplewallet connectivity_tool simpleminer PROPERTY LINK_SEARCH_END_STATIC 1)
endif()
set_property(TARGET daemon PROPERTY OUTPUT_NAME "bitmonerod")
