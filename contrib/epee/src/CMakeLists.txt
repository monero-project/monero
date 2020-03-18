# Copyright (c) 2014-2019, The Monero Project
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


add_library(epee STATIC byte_slice.cpp byte_stream.cpp hex.cpp abstract_http_client.cpp http_auth.cpp mlog.cpp net_helper.cpp net_utils_base.cpp string_tools.cpp
    wipeable_string.cpp levin_base.cpp memwipe.c connection_basic.cpp network_throttle.cpp network_throttle-detail.cpp mlocker.cpp buffer.cpp net_ssl.cpp
    int-util.cpp)

if (USE_READLINE AND (GNU_READLINE_FOUND OR (DEPENDS AND NOT MINGW)))
  add_library(epee_readline STATIC readline_buffer.cpp)
endif()

if(HAVE_C11)
SET_PROPERTY(SOURCE memwipe.c PROPERTY COMPILE_FLAGS -std=c11)
endif()

# Build and install libepee if we're building for GUI
if (BUILD_GUI_DEPS)
    if(IOS)
        set(lib_folder lib-${ARCH})
    else()
        set(lib_folder lib)
    endif()
    install(TARGETS epee
        ARCHIVE DESTINATION ${lib_folder})
    if (USE_READLINE AND GNU_READLINE_FOUND)
      install(TARGETS epee_readline
          ARCHIVE DESTINATION ${lib_folder})
    endif()
endif()

target_link_libraries(epee
  PUBLIC
    easylogging
    ${Boost_CHRONO_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
  PRIVATE
    ${OPENSSL_LIBRARIES}
    ${EXTRA_LIBRARIES})

if (USE_READLINE AND (GNU_READLINE_FOUND OR (DEPENDS AND NOT MINGW)))
  target_link_libraries(epee_readline
    PUBLIC
      easylogging
    PRIVATE
    ${GNU_READLINE_LIBRARY})
endif()
