#!/bin/bash -e

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

# ClangBuildAnalyzer is able to analyze the aggregate build time of particular headers.

DIR_THIS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Build variables
PROG="ClangBuildAnalyzer"
PROG_PATH="$DIR_THIS/bin/$PROG"
DIR_BUILD="build/clang-build-analyser"

# ClangBuildAnalyzer variables
DIR_MONITORED="."
RESULT="cba-result.txt"
TRACE="cba-trace.txt"

if [ -f "$PROG_PATH" ]; then
	echo "Found: $PROG_PATH"
else
	echo "Couldn't find: $PROG_PATH"
	echo "Please run the below script to clone and build $PROG:"
	echo "$DIR_THIS/build-scripts/clang-build-time-analyzer-clone-build.sh"
	exit 1
fi

mkdir -p "$DIR_BUILD" && cd "$DIR_BUILD"

cmake ../.. \
-DCMAKE_C_COMPILER=clang \
-DCMAKE_CXX_COMPILER=clang++ \
-DUSE_CCACHE=OFF \
-DUSE_COMPILATION_TIME_PROFILER=ON \
-DBUILD_SHARED_LIBS=ON \
-DBUILD_TESTS=ON

make clean 				# Clean up, so that the trace can be regenerated from scratch
$PROG_PATH --start $DIR_MONITORED	# Start monitoring
time make 				# Build
#time make easylogging			# Quick testing: build a single target
$PROG_PATH --stop $DIR_MONITORED $TRACE	# Stop and output to trace file
$PROG_PATH --analyze $TRACE | tee $RESULT # Analyze the trace, and store it in a readable format
gzip -f $TRACE # Zip the trace, because it's huge. -f overwrites the previously generated trace

echo ""
echo "Readable result stored in: $DIR_BUILD/$RESULT"
echo "The trace (analyser's input data) in: $DIR_BUILD/$TRACE.gz"

