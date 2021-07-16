# Copyright (c) 2014-2020, The Monero Project
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
# - Try to find readline include dirs and libraries 
#
# Automatically finds ccache build accelerator, if it's found in PATH.
#
# Usage of this module as follows:
#
#     project(monero)
#     include(FindCcache) # Include AFTER the project() macro to be able to reach the CMAKE_CXX_COMPILER variable
#
# Properties modified by this module:
#
#  GLOBAL PROPERTY RULE_LAUNCH_COMPILE     set to ccache, when ccache found
#  GLOBAL PROPERTY RULE_LAUNCH_LINK        set to ccache, when ccache found

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
	string(REGEX MATCH ".*depends/x86_64-unknown-freebsd" IS_FREEBSD_XCOMPILING "${CMAKE_CXX_COMPILER}")
	if (IS_FREEBSD_XCOMPILING)
		# During FreeBSD cross compilation the ccache fails, due to the "CMAKE_CXX_COMPILER" pointing actually to a script,
		# rather than to an executable, which is not accepted by ccache.
		# The solutiuon is to extract the contents of the script and strip it off its unnecessary (for ccache) commands.
		message(STATUS "Cross compiling for FreeBSD")
		file(READ "${CMAKE_CXX_COMPILER}" SCRIPT_CONTENT)

		string(REGEX MATCH "clang.*\-[0-9]+" CLANG_COMPILER ${SCRIPT_CONTENT}) # Extract compiler
		string(REGEX MATCH          "[0-9]+" CLANG_VER      ${CLANG_COMPILER}) # Extract compiler's version

		# Prepare the compiler options by extracting them from the "compiler" script
		string(REPLACE "/usr/bin/clang++-${CLANG_VER}" "" OPTS "${SCRIPT_CONTENT}") # Remove the compiler command
		string(REPLACE "exec" "" OPTS "${OPTS}") # Remove the leading script command
		string(REPLACE "$@"   "" OPTS "${OPTS}") # Remove the trailing redirection argument
		string(STRIP "${OPTS}" OPTS)

		set(CMAKE_C_FLAGS      "${CMAKE_C_FLAGS}   ${OPTS}")
		set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} ${OPTS}")
		set(CMAKE_C_COMPILER   "/usr/bin/clang-${CLANG_VER}"  )
		set(CMAKE_CXX_COMPILER "/usr/bin/clang++-${CLANG_VER}")
	endif()

	# Try to compile a test program with ccache, in order to verify if it really works. (needed on exotic setups)
	set(TEST_PROJECT "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/CMakeTmp")
	file(WRITE "${TEST_PROJECT}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.5)
project(test)
option (CCACHE "")
file(WRITE "${CMAKE_SOURCE_DIR}/test.cpp" "int main() { return 0; }")
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCACHE}")

add_executable(test test.cpp)
]=])
	# Tried:
	#try_compile(RET "${TEST_PROJECT}/build" "${TEST_PROJECT}" "test" CMAKE_FLAGS -DCCACHE="${CCACHE_FOUND}" -DCMAKE_C_COMPILER="${CMAKE_C_COMPILER}" -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}" -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}" -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS}")
	try_compile(RET "${TEST_PROJECT}/build" "${TEST_PROJECT}" "test" CMAKE_FLAGS -DCCACHE="${CCACHE_FOUND}")
	unset(TEST_PROJECT)
	if (${RET} OR IS_FREEBSD_XCOMPILING)
		# Success
		message(STATUS "Found usable ccache: ${CCACHE_FOUND}")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_FOUND}")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCACHE_FOUND}")
	else()
		message(STATUS "Found ccache ${CCACHE_FOUND}, but is UNUSABLE! Return code: ${RET}")
	endif()
else()
	message(STATUS "ccache NOT found! Please install it for faster rebuilds.")
endif()
