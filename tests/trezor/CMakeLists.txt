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

set(trezor_tests_sources
        tools.cpp
        daemon.cpp
        trezor_tests.cpp
        ../core_tests/chaingen.cpp
        ../core_tests/wallet_tools.cpp)

set(trezor_tests_headers
        tools.h
        daemon.h
        trezor_tests.h
        ../core_tests/chaingen.h
        ../core_tests/wallet_tools.h)

monero_add_minimal_executable(trezor_tests
  ${trezor_tests_sources}
  ${trezor_tests_headers})

target_link_libraries(trezor_tests
  PRIVATE
    multisig
    cryptonote_core
    p2p
    version
    epee
    device
    device_trezor
    wallet
    wallet_api
    rpc
    cryptonote_protocol
    daemon_rpc_server
    ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${ZMQ_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

enable_stack_trace(trezor_tests)
set_property(TARGET trezor_tests
  PROPERTY
    FOLDER "tests")

add_test(
  NAME    trezor_tests
  COMMAND trezor_tests)
