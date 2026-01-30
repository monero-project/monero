#!/bin/bash -e

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

# An example script, explaining how to customize the code coverage generation,
# for instance for Test Driven Development (TDD).
# Execute from Monero's source dir.

# Boilerplate starts
DIR_THIS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "$DIR_THIS/coverage-common.sh"
# Boilerplate ends

FILTER="logging*" # An example of a filter to run only the logging tests,
# The filters help with:
#
# 1) Reducing the run time of a long lasting process (from TDD perspective),
#    thus allowing for a much quicker feedback loop, and
#
# 2) Identifying, whether the responsible test group 
#    has actually touched the lines, that you'd expect the group to touch, 
#    or another, unrelated group has done it, which is a kind of a mistake.


# Build with optional, but positional arguments (use empty "" to leave any out):
# 1) Make options
# 2) CMake options
# 3) Custom build directory
build \
"-j`nproc` unit_tests" \
"-DUSE_UNITY=ON -DBoost_INCLUDE_DIR='$HOME/devel/lib/tree/include'" \
"/tmp/monero/coverage"

# Report with required arguments:
# 1) Report name  (required)
# 2) Test command (required). Could also be: "ctest -R unit_tests", etc.
report "unit-tests" "tests/unit_tests/unit_tests --gtest_filter=$FILTER"

