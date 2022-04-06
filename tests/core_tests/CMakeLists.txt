# Copyright (c) 2014-2022, The Monero Project
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

set(core_tests_sources
  block_reward.cpp
  block_validation.cpp
  chain_split_1.cpp
  chain_switch_1.cpp
  chaingen.cpp
  chaingen001.cpp
  chaingen_main.cpp
  double_spend.cpp
  integer_overflow.cpp
  multisig.cpp
  ring_signature_1.cpp
  transaction_tests.cpp
  tx_validation.cpp
  tx_pool.cpp
  v2_tests.cpp
  rct.cpp
  bulletproofs.cpp
  bulletproof_plus.cpp
  rct2.cpp
  wallet_tools.cpp)

set(core_tests_headers
  block_reward.h
  block_validation.h
  chain_split_1.h
  chain_switch_1.h
  chaingen.h
  chaingen_tests_list.h
  double_spend.h
  double_spend.inl
  integer_overflow.h
  multisig.h
  ring_signature_1.h
  tx_pool.h
  transaction_tests.h
  tx_validation.h
  v2_tests.h
  rct.h
  bulletproofs.h
  bulletproof_plus.h
  rct2.h
  wallet_tools.h)

monero_add_minimal_executable(core_tests
  ${core_tests_sources}
  ${core_tests_headers})
target_link_libraries(core_tests
  PRIVATE
    multisig
    cryptonote_core
    p2p
    version
    epee
    device
    wallet
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
enable_stack_trace(core_tests)
set_property(TARGET core_tests
  PROPERTY
    FOLDER "tests")

add_test(
  NAME    core_tests
  COMMAND core_tests --generate_and_play_test_data)
