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

add_subdirectory(gtest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

file(GLOB_RECURSE CORE_TESTS core_tests/*)
file(GLOB_RECURSE CRYPTO_TESTS crypto/*)
file(GLOB_RECURSE FUNC_TESTS functional_tests/*)
file(GLOB_RECURSE PERFORMANCE_TESTS performance_tests/*)
file(GLOB_RECURSE CORE_PROXY core_proxy/*)
file(GLOB_RECURSE UNIT_TESTS unit_tests/*)

source_group(core_tests FILES ${CORE_TESTS})
source_group(crypto_tests FILES ${CRYPTO_TESTS})
source_group(functional_tests FILES ${FUNC_TESTS})
source_group(performance_tests FILES ${PERFORMANCE_TESTS})
source_group(core_proxy FILES ${CORE_PROXY})
source_group(unit_tests FILES ${UNIT_TESTS})

# add_subdirectory(daemon_tests)

add_executable(coretests ${CORE_TESTS})
add_executable(crypto-tests ${CRYPTO_TESTS})
add_executable(difficulty-tests difficulty/difficulty.cpp)
add_executable(hash-tests hash/main.cpp)
add_executable(hash-target-tests hash-target.cpp)
add_executable(functional_tests ${FUNC_TESTS})
add_executable(performance_tests ${PERFORMANCE_TESTS})
add_executable(core_proxy ${CORE_PROXY})
add_executable(unit_tests ${UNIT_TESTS})
add_executable(net_load_tests_clt net_load_tests/clt.cpp)
add_executable(net_load_tests_srv net_load_tests/srv.cpp)

target_link_libraries(core_proxy cryptonote_core common crypto ${UNBOUND_LIBRARY} ${UPNP_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(coretests cryptonote_core common crypto ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(difficulty-tests cryptonote_core)
target_link_libraries(functional_tests cryptonote_core wallet common crypto ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(hash-tests crypto)
target_link_libraries(hash-target-tests crypto cryptonote_core)
target_link_libraries(performance_tests cryptonote_core common crypto ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(unit_tests gtest_main cryptonote_core wallet crypto common ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(net_load_tests_clt cryptonote_core common crypto gtest_main ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(net_load_tests_srv cryptonote_core common crypto gtest_main ${UNBOUND_LIBRARY} ${Boost_LIBRARIES})

if(NOT MSVC)
  set_property(TARGET gtest gtest_main unit_tests net_load_tests_clt net_load_tests_srv APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-sign-compare")
endif()

add_custom_target(tests DEPENDS coretests difficulty hash performance_tests core_proxy unit_tests)
set_property(TARGET coretests crypto-tests functional_tests difficulty-tests gtest gtest_main hash-tests hash-target-tests performance_tests core_proxy unit_tests tests net_load_tests_clt net_load_tests_srv PROPERTY FOLDER "tests")

add_test(coretests coretests --generate_and_play_test_data)
add_test(crypto crypto-tests ${CMAKE_CURRENT_SOURCE_DIR}/crypto/tests.txt)
add_test(difficulty difficulty-tests ${CMAKE_CURRENT_SOURCE_DIR}/difficulty/data.txt)
foreach(hash IN ITEMS fast slow tree extra-blake extra-groestl extra-jh extra-skein)
  add_test(hash-${hash} hash-tests ${hash} ${CMAKE_CURRENT_SOURCE_DIR}/hash/tests-${hash}.txt)
endforeach(hash)
add_test(hash-target hash-target-tests)
add_test(unit_tests unit_tests)
