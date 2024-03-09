# Copyright (c) 2014-2023, The Monero Project
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

set(lock_tests_source
  lock_tests.cpp)

monero_add_minimal_executable(lock_tests
  ${lock_tests_source})
target_link_libraries(lock_tests
  PRIVATE
    epee
    common
    ${GTEST_LIBRARIES}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBARRY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
  # Due to a bug in upstream ubuntu-20.04
  # we have to disable thread sanitizer until the fix 
  # pushed. Once fixed we can remove this condition and 
  # set_target_properties unconditionaly.
  # https://bugs.launchpad.net/ubuntu/+source/gcc-9/+bug/2029910
  if(EXISTS "/usr/lib/x86_64-linux-gnu/libtsan_preinit.o")
    set_target_properties(lock_tests
    PROPERTIES
      COMPILE_FLAGS "-fsanitize=thread -Og -fno-omit-frame-pointer -g"
      LINK_FLAGS "-fsanitize=thread -Og -fno-omit-frame-pointer -g")
  endif()
endif()

set_property(TARGET lock_tests
  PROPERTY
    FOLDER "tests")

add_test(
  NAME    lock_tests
  COMMAND lock_tests)

# This is not computation intensive test.
# just locking and unlocking in multiple threads.
# if it fails to complete in 1500 seconds in slow machines.
# Then it means it in somekind of deadlock, and should fail.   
set_tests_properties(lock_tests PROPERTIES TIMEOUT 1500) 
