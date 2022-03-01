# Copyright (c) 2014-2022, The Monero Project
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

set(GENERATED_SOURCES "")

set(GENERATOR "${CMAKE_CURRENT_BINARY_DIR}/blocks_generator.cmake")
file(GENERATE OUTPUT ${GENERATOR} CONTENT [=[
file(READ "${INPUT_DAT_FILE}" DATA HEX)
string(REGEX REPLACE "[0-9a-fA-F][0-9a-fA-F]" "0x\\0," DATA "${DATA}")
file(WRITE "${OUTPUT_C_SOURCE}" "
#include <stddef.h>
const unsigned char ${BLOB_NAME}[]={
  ${DATA}
};
const size_t ${BLOB_NAME}_len = sizeof(${BLOB_NAME});
"
)
]=])
foreach(BLOB_NAME checkpoints testnet_blocks stagenet_blocks)
    set(OUTPUT_C_SOURCE "generated_${BLOB_NAME}.c")
    list(APPEND GENERATED_SOURCES ${OUTPUT_C_SOURCE})
    set(INPUT_DAT_FILE "${BLOB_NAME}.dat")
    add_custom_command(
      OUTPUT ${OUTPUT_C_SOURCE}
      MAIN_DEPENDENCY ${INPUT_DAT_FILE}
      DEPENDS ${GENERATOR}
      COMMAND ${CMAKE_COMMAND}
        "-DINPUT_DAT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DAT_FILE}"
        "-DBLOB_NAME=${BLOB_NAME}"
        "-DOUTPUT_C_SOURCE=${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_C_SOURCE}"
        -P "${GENERATOR}"
    )
endforeach()

monero_add_library(blocks blocks.cpp ${GENERATED_SOURCES})
