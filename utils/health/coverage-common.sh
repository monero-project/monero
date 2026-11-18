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

# Code coverage's common functions. This script is meant to be sourced. 
# See examples starting with coverage-*.sh in this directory.

# Boilerplate starts
DIR_THIS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "$DIR_THIS/health-common.sh"
# Boilerplate ends

DIR_CWD=`pwd`
DIR_OUT_REL="build/coverage"
DIR_BUILD="$DIR_OUT_REL"

PROG_LCOV="lcov"
PROG_GENHTML="genhtml"
GEN="CodeBlocks - Unix Makefiles"

# Build with both the required hardcoded options, as well as optionals, provided by the caller.
build() {
	MAKE_OPTIONS="$1"
	CMAKE_OPTIONS="$2"
	if [ ! -z "$3" ]; then
		# A possibility to override the default build dir
		DIR_BUILD="$3"
	fi
	cmake -S "${DIR_CWD}" -B "${DIR_BUILD}" -G "${GEN}" $CMAKE_OPTIONS -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
	cd "${DIR_BUILD}"
	make $MAKE_OPTIONS
}

# Clean object files
clean_obj() {
	cd "${DIR_CWD}"
	cd "${DIR_BUILD}"
	echo "Cleaning objects in `pwd`"
	find . -name \*.o   -type f -delete
	find . -name \*.gc* -type f -delete
}

# Zero the stats for a more objective measurement. 
# Normally only meant to be used internally.
zero() {
	rm -f "$FBASE" "$FTEST" "$FFLTR" "$FTOTAL" || true

	$PROG_LCOV --zerocounters --directory "$DIR"
	$PROG_LCOV --capture --initial --directory "$DIR" --output-file "$FBASE"
}

# Generate and archive the report.
# Normally only meant to be used internally.
generate() {
	$PROG_LCOV --capture --directory "$DIR" --output-file "$FTEST"
	$PROG_LCOV --add-tracefile "$FBASE" --add-tracefile "$FTEST" --output-file "$FTOTAL"
	rm -rf "$DIR_REPORT"
	mkdir -p "$DIR_REPORT"
	$PROG_LCOV --remove "$FTOTAL" '/usr/include/*' '/usr/lib/*' '*/build/*' '*tests/*' '/home/enjo/devel/lib/tree/*' -o "$FFLTR"
	$PROG_GENHTML --ignore-errors source "$FFLTR" --legend --title $PROJ --output-directory="$DIR_REPORT" 2>&1 | tee "$LOG"
	KPI_LINES=$(grep "  lines" 	"$LOG" | awk '{print $2}' | sed 's/.$//')
	KPI_FUNCS=$(grep "  functions" 	"$LOG" | awk '{print $2}' | sed 's/.$//')
	KPI_FILE="kpis.txt"
	if [ -z "$var" ]; then
		echo "$KPI_LINES $KPI_FUNCS" > "$KPI_FILE"
	else
		echo "-3" > "$KPI_FILE"
	fi
	echo "Report written to: $DIR_REPORT/index.html"
	
	if [ ! -L "$PROJ" ]; then
		ln -s "$DIR_REPORT"
	fi
	echo "Archiving the report..."
	tar -cJhf "$ARCHIVE" "$PROJ"

	echo "Archive stored to: `pwd`/$ARCHIVE"

	FINAL_DEST="$DIR_CWD/$DIR_OUT_REL"

	if [ "`pwd`" != "$FINAL_DEST" ]; then
		mkdir -p "$FINAL_DEST"
		cp -v "$ARCHIVE" "$FINAL_DEST"
		cp -v "$KPI_FILE" "$FINAL_DEST"
	else
		echo "Artifacts '$ARCHIVE' and '$KPI_FILE' already in the final destination dir: '$FINAL_DEST'."
	fi
}

# General handling of the report.
# It will be generated if only the build succeeded, regardless of the tests' exit status.
report() {
	PROJ="$1-lcov"
	COMMAND=$2
	
	if [ -z "$1" ]; then
		echo "Please provide a project name as the 1st argument of the report function."
		exit 1
	fi
	if [ -z "$2" ]; then
		echo "Please provide a test command as the 2nd argument of the report function, for example:"
		echo  "\"ctest -R unit_tests\""
		echo ", or:"
		echo "\"tests/unit_tests/unit_tests --gtest_filter=logging*\""
		exit 1
	fi
	
	DIR="."
	DIR_OUT=/tmp/lcov
	PATH_OUT_BASE="$DIR_OUT/$PROJ"
	DIR_REPORT="$PATH_OUT_BASE"
	FBASE="$PATH_OUT_BASE"_base.info
	FTEST="$PATH_OUT_BASE"_test.info
	FFLTR="$PATH_OUT_BASE"_filtered.info
	FTOTAL="$PATH_OUT_BASE"_total.info
	LOG="$PATH_OUT_BASE"-log.txt
	ARCHIVE="$PROJ.tar.xz"
	
	mkdir -p "$DIR_OUT"
	
	zero
	$COMMAND || true # Failing tests aren't a reason to abort the report generation
	generate
}

# Make sure we've got everything
find_prog $PROG_LCOV
find_prog $PROG_GENHTML

