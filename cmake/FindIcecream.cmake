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
#
# Automatically uses icecream network compiler, if it's found, else aborts.
#
# Usage of this module as follows:
#
#     include(FindIcecream) # Include before project() macro to preset the C and CXX vars
#     project(monero)
#
# Properties modified by this module:
#
#  CMAKE_C_COMPILER     set to ICECC_C_COMPILER,   if compilation with icecc succeeds
#  CMAKE_CXX_COMPILER   set to ICECC_CXX_COMPILER, if compilation with icecc succeeds

find_program(ICECC_FOUND icecc)
if (ICECC_FOUND)
	set(ICECC_C_COMPILER   /usr/lib/icecc/bin/cc  ) # Typical installation dir
	set(ICECC_CXX_COMPILER /usr/lib/icecc/bin/c++ )
	# Try to compile a test program with icecc, in order to verify if it really works.
	# Create a temporary file with a simple program.
	set(TEMP_CPP_FILE "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/CMakeTmp/test-program.cpp")
	file(WRITE "${TEMP_CPP_FILE}" "int main() { return 0; }")
	# And run the found ccache on it.
	execute_process(COMMAND "${ICECC_CXX_COMPILER}" "${TEMP_CPP_FILE}"  RESULT_VARIABLE RET)
	if (${RET} EQUAL 0)
		# Success
		message(STATUS "Found usable icecc: ${ICECC_CXX_COMPILER}")
		set(CMAKE_C_COMPILER   "${ICECC_C_COMPILER}")
		set(CMAKE_CXX_COMPILER "${ICECC_CXX_COMPILER}")
	else()
		message(FATAL_ERROR "Found icecc ${ICECC_CXX_COMPILER}, but is UNUSABLE! Return code: ${RET}")
	endif()
else()
	message(FATAL_ERROR "icecc NOT found! Please install it for parallel builds. sudo apt install icecc icecc-monitor")
endif()

