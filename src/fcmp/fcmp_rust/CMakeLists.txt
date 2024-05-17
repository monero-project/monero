# Copyright (c) 2016-2024, The Monero Project
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

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CARGO_CMD cargo build)
  set(TARGET_DIR "debug")
else ()
  set(CARGO_CMD cargo build --release)
  set(TARGET_DIR "release")
endif ()

set(FCMP_RUST_CXX "${CMAKE_CURRENT_BINARY_DIR}/fcmp_rust.cc")
set(FCMP_RUST_HEADER_DIR "${MONERO_GENERATED_HEADERS_DIR}/fcmp_rust")
set(FCMP_RUST_HEADER "${FCMP_RUST_HEADER_DIR}/fcmp_rust.h")
set(CXX_HEADER "${FCMP_RUST_HEADER_DIR}/cxx.h")

# Removing OUTPUT files makes sure custom command runs every time
file(REMOVE_RECURSE "${FCMP_RUST_CXX}")
file(REMOVE_RECURSE "${FCMP_RUST_HEADER_DIR}")
file(MAKE_DIRECTORY "${FCMP_RUST_HEADER_DIR}")

add_custom_command(
  COMMENT "Building rust fcmp lib"
  OUTPUT ${FCMP_RUST_CXX} ${FCMP_RUST_HEADER} ${CXX_HEADER}
  COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD}
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fcmp_rust/src/lib.rs.cc ${FCMP_RUST_CXX}
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fcmp_rust/src/lib.rs.h ${FCMP_RUST_HEADER}
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/rust/cxx.h ${CXX_HEADER}
  COMMAND echo "Finished copying fcmp rust targets"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_custom_target(rust_cxx ALL DEPENDS ${CXX_HEADER})

set(fcmp_rust_sources ${FCMP_RUST_CXX})

monero_find_all_headers(fcmp_rust_headers "${FCMP_RUST_HEAfDER_DIR}")

monero_add_library(fcmp_rust
  ${fcmp_rust_sources}
  ${fcmp_rust_headers})

set(FCMP_RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libfcmp_rust.a")
target_link_libraries(fcmp_rust dl ${FCMP_RUST_LIB})
