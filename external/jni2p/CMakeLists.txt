# Copyright (c) 2014-2021, The Monero Project
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

cmake_minimum_required(VERSION 3.5)

project(jni2p)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.h)

find_package(JNI REQUIRED)

include_directories(${JNI_INCLUDE_DIRS})

add_library(jni2p STATIC
  ${SOURCES}
  ${HEADERS})

target_include_directories(jni2p
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src" ${JNI_INCLUDE_DIRS})

target_link_libraries(jni2p
  PUBLIC
    ${JNI_LIBRARIES})

set(JNI2P_LIBRARY "jni2p")
set(JNI2P_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE STRING "JNI2P Include path")

set(JNI2P_INCLUDE_DIRS "${JNI2P_INCLUDE};${JNI_INCLUDE_DIRS}" CACHE STRING "JNI2P Include paths")
set(JNI2P_LIBRARIES "${JNI2P_LIBRARY};${JNI_LIBRARIES}" CACHE STRING "JNI2P Libraries")

message(STATUS "JNI2P includes: ${JNI_INCLUDE_DIRS}")
message(STATUS "JNI2P library: ${JNI2P_LIBRARY};${JNI_LIBRARIES}")

set_property(TARGET jni2p PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET jni2p PROPERTY CXX_STANDARD 11)
set_property(TARGET jni2p PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET jni2p PROPERTY PUBLIC_HEADER src/jni2p.h)

include(GNUInstallDirs)
install(TARGETS jni2p
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
