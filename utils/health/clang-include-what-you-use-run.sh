#!/bin/bash -e

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

# Include What You Use analyses the complexity of your header hierarchy and proposes optimisations.
# User documentation:
# https://github.com/include-what-you-use/include-what-you-use/blob/master/README.md

# Build variables
PROG="include-what-you-use"
PROG_SHORT="iwyu"
DIR_BUILD="build/clang-$PROG_SHORT"

RESULT="$PROG_SHORT-result.txt"

if hash "$PROG"; then
	echo "Found: $PROG"
else
	echo "Couldn't find: $PROG"
	echo "Please run the below command to install $PROG:"
	echo "sudo apt install $PROG_SHORT"
	exit 1
fi

mkdir -p "$DIR_BUILD" && cd "$DIR_BUILD"
rm `find . -name "CMakeCache.txt"` || true

UWYU_COMMAND="$PROG;-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;args" # Copy-pasted from the user docs.
	
cmake ../.. \
-DCMAKE_C_COMPILER=clang \
-DCMAKE_CXX_COMPILER=clang++ \
-DUSE_CCACHE=ON \
-DCMAKE_C_INCLUDE_WHAT_YOU_USE="$UWYU_COMMAND" \
-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE="$UWYU_COMMAND" \
-DBUILD_SHARED_LIBS=ON \
-DBUILD_TESTS=ON

make clean					# Clean up to generate the full report
time make -k 2>&1 | tee "$RESULT"		# Run the scan. -k means: ignore errors
#time make -k easylogging 2>&1 | tee $RESULT	# Quick testing: build a single target
KPI=$(cat "$RESULT" | wc -l)
tar -cJvf "$RESULT.txz" "$RESULT"		# Zip the result, because it's huge.
rm -v "$RESULT"

echo ""
echo "Readable result stored in: $DIR_BUILD/$RESULT.gz"

echo "$KPI" > "kpis.txt"
