# Copyright (c) 2014-2019, The Monero Project
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

set(functional_tests_sources
  main.cpp
  transactions_flow_test.cpp
  transactions_generation_from_blockchain.cpp)

set(functional_tests_headers
  transactions_flow_test.h
  transactions_generation_from_blockchain.h)

add_executable(functional_tests
  ${functional_tests_sources}
  ${functional_tests_headers})
target_link_libraries(functional_tests
  PRIVATE
    cryptonote_core
    wallet
    common
    cncrypto
    epee
    ${Boost_REGEX_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

set(make_test_signature_sources
  make_test_signature.cc)

add_executable(make_test_signature
  ${make_test_signature_sources})

target_link_libraries(make_test_signature
  PRIVATE
    rpc_base
    cncrypto
    epee
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

execute_process(COMMAND ${PYTHON_EXECUTABLE} "-c" "import requests; print('OK')" OUTPUT_VARIABLE REQUESTS_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
if (REQUESTS_OUTPUT STREQUAL "OK")
  add_test(
    NAME    functional_tests_rpc
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/functional_tests_rpc.py" "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" all)
else()
  message(WARNING "functional_tests_rpc skipped, needs the 'requests' python module")
  set(CTEST_CUSTOM_TESTS_IGNORE ${CTEST_CUSTOM_TESTS_IGNORE} functional_tests_rpc)
endif()

add_test(
  NAME    check_missing_rpc_methods
  COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/check_missing_rpc_methods.py" "${CMAKE_SOURCE_DIR}")
