# Copyright (c) 2017-2018, The Monero Project
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

cmake_minimum_required(VERSION 2.8.7)

project(translations)

add_executable(generate_translations_header generate_translations_header.c)

find_program(LRELEASE lrelease)
if(LRELEASE STREQUAL "LRELEASE-NOTFOUND")
  set(ts_files "")
  message(WARNING "lrelease program not found, translation files not built")
else()
  execute_process(COMMAND ${LRELEASE} -version
    RESULT_VARIABLE lrelease_ret)
  if(NOT lrelease_ret EQUAL "0")
    set(ts_files "")
    message(WARNING "lrelease program not working, translation files not built")
  else()
    file(GLOB ts_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.ts)
    foreach(ts_file ${ts_files})
      string(REPLACE ".ts" ".qm" qm_file "${ts_file}")
      add_custom_command(TARGET generate_translations_header
        PRE_BUILD
        COMMAND ${LRELEASE} "${CMAKE_CURRENT_SOURCE_DIR}/${ts_file}" -qm "${qm_file}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BIN_DIR}")
    endforeach()
  endif()
endif()

string(REPLACE ".ts" ".qm" qm_files "${ts_files}")

add_custom_command(TARGET generate_translations_header
  POST_BUILD
  COMMAND generate_translations_header ${qm_files}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BIN_DIR}"
  COMMENT "Generating embedded translations header")
