# Copyright (c) 2014-2024, The Monero Project
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
# Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

# Check what commit we're on

function (get_version_tag_from_git GIT)
    execute_process(COMMAND "${GIT}" rev-parse --short=9 HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                    RESULT_VARIABLE RET
                    OUTPUT_VARIABLE COMMIT
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(RET)
        # Something went wrong, set the version tag to -unknown

        message(WARNING "Cannot determine current commit. Make sure that you are building either from a Git working tree or from a source archive.")
        set(VERSIONTAG "unknown")
        set(VERSION_IS_RELEASE "false")
    else()
        string(SUBSTRING ${COMMIT} 0 9 COMMIT)
        message(STATUS "You are currently on commit ${COMMIT}")

        # Get all the tags
        execute_process(COMMAND "${GIT}" tag -l --points-at HEAD
                        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                        RESULT_VARIABLE RET
                        OUTPUT_VARIABLE TAG
                        OUTPUT_STRIP_TRAILING_WHITESPACE)

        # Check if we're building that tagged commit or a different one
        if(TAG)
            message(STATUS "You are building a tagged release")
            set(VERSIONTAG "release")
            set(VERSION_IS_RELEASE "true")
        else()
            message(STATUS "You are ahead of or behind a tagged release")
            set(VERSIONTAG "${COMMIT}")
            set(VERSION_IS_RELEASE "false")
        endif()	    
    endif()

    set(VERSIONTAG "${VERSIONTAG}" PARENT_SCOPE)
    set(VERSION_IS_RELEASE "${VERSION_IS_RELEASE}" PARENT_SCOPE)
endfunction()
