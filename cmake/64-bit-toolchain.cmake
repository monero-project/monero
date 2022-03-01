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

if (NOT CMAKE_HOST_WIN32)
  set (CMAKE_SYSTEM_NAME Windows)
endif()

set (GCC_PREFIX x86_64-w64-mingw32)
set (CMAKE_C_COMPILER ${GCC_PREFIX}-gcc)
set (CMAKE_CXX_COMPILER ${GCC_PREFIX}-g++)
set (CMAKE_AR ar CACHE FILEPATH "" FORCE)
set (CMAKE_NM nm CACHE FILEPATH "" FORCE)
set (CMAKE_LINKER ld CACHE FILEPATH "" FORCE)
#set (CMAKE_RANLIB ${GCC_PREFIX}-gcc-ranlib CACHE FILEPATH "" FORCE)
set (CMAKE_RC_COMPILER windres)

set (CMAKE_FIND_ROOT_PATH "${MSYS2_FOLDER}/mingw64")

# Ensure cmake doesn't find things in the wrong places
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # Find programs on host
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) # Find libs in target
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY) # Find includes in target

set (MINGW_FLAG "-m64")
set (USE_LTO_DEFAULT false)
