# Copyright (c) 2014-2016, The Monero Project
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

# The docs say this only affects grouping in IDEs
set(folder "tests")

if (WIN32 AND STATIC)
  add_definitions(-DSTATICLIB)
  # miniupnp changed their static define
  add_definitions(-DMINIUPNP_STATICLIB)
endif ()

find_package(GTest)

if (GTest_FOUND)
  include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
else ()
  add_subdirectory(gtest)
  include_directories(SYSTEM "${gtest_SOURCE_DIR}/include" "${gtest_SOURCE_DIR}")

  # Emulate the FindGTest module's variable.
  set(GTEST_MAIN_LIBRARIES gtest_main)

  # Ignore some warnings when building gtest binaries.
  if(NOT MSVC)
    set_property(TARGET gtest gtest_main
      APPEND_STRING
      PROPERTY
        COMPILE_FLAGS " -Wno-undef -Wno-sign-compare")
  endif()

  set_property(TARGET gtest gtest_main
      PROPERTY
        FOLDER "${folder}")
endif ()

if (NOT DEFINED ENV{TRAVIS})
  add_subdirectory(core_tests)
endif ()
add_subdirectory(crypto)
add_subdirectory(functional_tests)
add_subdirectory(performance_tests)
add_subdirectory(core_proxy)
add_subdirectory(unit_tests)
add_subdirectory(difficulty)
add_subdirectory(hash)
add_subdirectory(net_load_tests)

# Disabled until issue #895 is resolved
#add_subdirectory(libwallet_api_tests)

# add_subdirectory(daemon_tests)

set(hash_targets_sources
  hash-target.cpp)

set(hash_targets_headers)

add_executable(hash-target-tests
  ${hash_targets_sources}
  ${hash_targets_headers})
target_link_libraries(hash-target-tests
  LINK_PRIVATE
    cryptonote_core)
set_property(TARGET hash-target-tests
  PROPERTY
    FOLDER "${folder}")

add_test(
  NAME    hash-target
  COMMAND hash-target-tests)

# Skip the core_tests if we are running in Travis-CI because they will take too long
if (DEFINED ENV{TRAVIS})
  add_custom_target(tests DEPENDS difficulty hash performance_tests core_proxy unit_tests)
else ()
  add_custom_target(tests DEPENDS coretests difficulty hash performance_tests core_proxy unit_tests)
endif ()

set_property(TARGET tests PROPERTY FOLDER "${folder}")
