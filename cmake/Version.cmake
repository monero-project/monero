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

function (write_static_version_header hash)
  set(VERSIONTAG "${hash}")
  configure_file("src/version.cpp.in" "version.cpp")
endfunction ()

find_package(Git QUIET)
if ("$Format:$" STREQUAL "")
  # We're in a tarball; use hard-coded variables.
  write_static_version_header("release")
elseif (GIT_FOUND OR Git_FOUND)
  message(STATUS "Found Git: ${GIT_EXECUTABLE}")
  add_custom_target(genversion ALL
    COMMAND           "${CMAKE_COMMAND}"
                      "-D" "GIT=${GIT_EXECUTABLE}"
                      "-D" "TO=${CMAKE_BINARY_DIR}/version.cpp"
                      "-P" "cmake/GenVersion.cmake"
    BYPRODUCTS        "${CMAKE_BINARY_DIR}/version.cpp"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
else()
  message(STATUS "WARNING: Git was not found!")
  write_static_version_header("unknown")
endif ()
