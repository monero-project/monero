# Copyright (c) 2014-2017, The Monero Project
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

add_executable(block_fuzz_tests block.cpp fuzzer.cpp)
target_link_libraries(block_fuzz_tests
  PRIVATE
    cryptonote_core
    common
    blockchain_db
    p2p
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET block_fuzz_tests
  PROPERTY
    FOLDER "tests")

add_executable(transaction_fuzz_tests transaction.cpp fuzzer.cpp)
target_link_libraries(transaction_fuzz_tests
  PRIVATE
    cryptonote_core
    common
    blockchain_db
    p2p
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET transaction_fuzz_tests
  PROPERTY
    FOLDER "tests")

add_executable(signature_fuzz_tests signature.cpp fuzzer.cpp)
target_link_libraries(signature_fuzz_tests
  PRIVATE
    wallet
    cryptonote_core
    common
    blockchain_db
    p2p
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET signature_fuzz_tests
  PROPERTY
    FOLDER "tests")

add_executable(cold-outputs_fuzz_tests cold-outputs.cpp fuzzer.cpp)
target_link_libraries(cold-outputs_fuzz_tests
  PRIVATE
    wallet
    cryptonote_core
    common
    blockchain_db
    p2p
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET cold-outputs_fuzz_tests
  PROPERTY
    FOLDER "tests")

add_executable(cold-transaction_fuzz_tests cold-transaction.cpp fuzzer.cpp)
target_link_libraries(cold-transaction_fuzz_tests
  PRIVATE
    wallet
    cryptonote_core
    common
    blockchain_db
    p2p
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET cold-transaction_fuzz_tests
  PROPERTY
    FOLDER "tests")

