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

if(ARCH_ID MATCHES "x86-64")
  set(RUST_ARCH "x86_64")
elseif(ARCH_ID MATCHES "(arm64|armv8a|armv8-a)")
  set(RUST_ARCH "aarch64")
elseif(ARCH_ID MATCHES "armv7-a")
  set(RUST_ARCH "armv7")
elseif(ARCH_ID MATCHES "i386")
  set(RUST_ARCH "i686")
elseif(ARCH_ID MATCHES "riscv64")
  set(RUST_ARCH "riscv64gc")
else()
  set(RUST_ARCH "${ARCH_ID}")
endif()

if(MINGW)
  set(RUST_PLATFORM "pc-windows")
  set(RUST_TOOLCHAIN "-gnu")
elseif(MSVC)
  set(RUST_PLATFORM "pc-windows")
  set(RUST_TOOLCHAIN "-msvc")
elseif(APPLE)
  set(RUST_PLATFORM "apple-darwin")
  set(RUST_TOOLCHAIN "")
elseif(FREEBSD)
  set(RUST_PLATFORM "unknown-freebsd")
  set(RUST_TOOLCHAIN "")
elseif(OPENBSD)
  set(RUST_PLATFORM "unknown-openbsd")
  set(RUST_TOOLCHAIN "")
elseif(ANDROID)
  set(RUST_PLATFORM "linux-android")
  if(RUST_ARCH MATCHES "(arm|armv7)")
    set(RUST_TOOLCHAIN "eabi")
  else()
    set(RUST_TOOLCHAIN "")
  endif()
elseif(DRAGONFLY)
  set(RUST_PLATFORM "unknown-dragonfly")
  set(RUST_TOOLCHAIN "")
elseif(CMAKE_SYSTEM_NAME MATCHES "(SunOS|Solaris)")
  if(RUST_ARCH MATCHES "x86_64")
    set(RUST_PLATFORM "pc-solaris")
    set(RUST_TOOLCHAIN "")
  elseif(RUST_ARCH MATCHES "sparcv9")
    set(RUST_PLATFORM "sun-solaris")
    set(RUST_TOOLCHAIN "")
  endif()
else()
  set(RUST_PLATFORM "unknown-linux")
  if(RUST_ARCH MATCHES "armv7")
    # Rust does support non-HF, yet Monero assumes HF for armv7
    set(RUST_TOOLCHAIN "-gnueabihf")
  else()
    set(RUST_TOOLCHAIN "-gnu")
  endif()
endif()

set(RUST_TARGET "${RUST_ARCH}-${RUST_PLATFORM}${RUST_TOOLCHAIN}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CARGO_CMD cargo build --target "${RUST_TARGET}")
  set(TARGET_DIR "debug")
else ()
  set(CARGO_CMD cargo build --target "${RUST_TARGET}" --release)
  set(TARGET_DIR "release")
endif ()

set(FCMP_PP_RUST_HEADER_DIR "${MONERO_GENERATED_HEADERS_DIR}/fcmp_pp_rust")
set(FCMP_PP_RUST_HEADER "${FCMP_PP_RUST_HEADER_DIR}/fcmp++.h")
set(FCMP_PP_RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/libfcmp_pp_rust.a")

# Removing OUTPUT files makes sure custom command runs every time
file(REMOVE_RECURSE "${FCMP_PP_RUST_HEADER_DIR}")
file(MAKE_DIRECTORY "${FCMP_PP_RUST_HEADER_DIR}")

file(REMOVE "${FCMP_PP_RUST_LIB}")

add_custom_command(
  COMMENT "Building fcmp++ rust lib"
  OUTPUT ${FCMP_PP_RUST_HEADER}
  OUTPUT ${FCMP_PP_RUST_LIB}
  COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD}
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/fcmp++.h ${FCMP_PP_RUST_HEADER}
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${RUST_TARGET}/${TARGET_DIR}/libfcmp_pp_rust.a ${FCMP_PP_RUST_LIB}
  COMMAND echo "Finished copying fcmp++ rust targets"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_custom_target(fcmp_pp_rust DEPENDS ${FCMP_PP_RUST_LIB})
