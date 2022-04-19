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

# This script is able to run valgrind's callgrind, cachegrind and memcheck for a given set of executables.
# It expects ONE PARAMETER, which points to a file with paths to executables and their arguments, written line by line.

if [ "$#" -ne 1 ]; then
    echo "Please provide an argument, which points to a file with paths to executables and their arguments, written line by line. For example:"
    echo ""
    echo "ls -l -h"
    echo "build/tests/unit_tests/unit_tests"
    exit 1
fi

FILE_IN="$1"
DIR_OUT="build/valgrind-output" # Using build as the base output directory, as it's ignored in .gitignore

function is_file_or_exit {
	FILE="${1}"
	if [ -f $FILE ]; then
		echo "The input file $FILE exists. Can proceed."
	else
		echo "The input file $FILE doesn't exist."
		exit 1
	fi
	return 0
}

function is_tool_or_exit {
	TOOL="${1}"
	if $(hash ${TOOL}); then
		echo "${TOOL} is installed. Can proceed."
	else
		echo "Please install ${TOOL} to continue."
		exit 1
	fi
	return 0
}

function get_tool_out_file_base {
	EXE="${1}"
	TOOL="${2}"

	EXE_NAME=$(basename $EXE)
	local retval="${DIR_OUT}/${EXE_NAME}-${TOOL}"
    	echo "$retval"
}

function get_tool_out_file {
	EXE="${1}"
	TOOL="${2}"

	FILE_OUT_BASE=$(get_tool_out_file_base ${EXE} ${TOOL})
	local retval="--${TOOL}-out-file=${FILE_OUT_BASE}.out"
    	echo "$retval"
}

function run_valgrind_4_executable {
	EXE="${1}"
	ARGS="${2}"
	TOOL="${3}"
	EXTRA_OPTS="${4}"
	FILE_OUT_TOOL="${5}"
	FILE_OUT_BASE=$(get_tool_out_file_base ${EXE} ${TOOL})

	echo "Runnig '${TOOL}' for '${EXE}' with args '${ARGS}'"
	echo "EXTRA_OPTS = ${EXTRA_OPTS}"
	echo "FILE_OUT_TOOL = ${FILE_OUT_TOOL}"
	if ! valgrind --tool=${TOOL} ${FILE_OUT_TOOL} --log-file="${FILE_OUT_BASE}.log" ${EXTRA_OPTS} ${EXE} ${ARGS}; then
		echo "FAILED in runnig ${TOOL} for ${EXE} !"
	fi
}

function run_valgrind_4_executable_callgrind {
	EXE="${1}"
	ARGS="${2}"
	TOOL="callgrind"
	EXTRA_OPTS="--dump-instr=yes --simulate-cache=yes --collect-jumps=yes"
	FILE_OUT_TOOL=$(get_tool_out_file ${EXE} ${TOOL})

	run_valgrind_4_executable ${EXE} "${ARGS}" ${TOOL} "${EXTRA_OPTS}" ${FILE_OUT_TOOL}
}

function run_valgrind_4_executable_cachegrind {
	EXE="${1}"
	ARGS="${2}"
	TOOL="cachegrind"
	EXTRA_OPTS=""
	FILE_OUT_TOOL=$(get_tool_out_file ${EXE} ${TOOL})

	run_valgrind_4_executable ${EXE} "${ARGS}" ${TOOL} "${EXTRA_OPTS}" ${FILE_OUT_TOOL}
}

function run_valgrind_4_executable_memcheck {
	EXE="${1}"
	ARGS="${2}"
	TOOL="memcheck"
	#EXTRA_OPTS="--leak-check=yes" # Minimalistic
	EXTRA_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes"
	FILE_OUT_TOOL="" # memcheck has no special out file, only the log

	run_valgrind_4_executable ${EXE} "${ARGS}" ${TOOL} "${EXTRA_OPTS}" ${FILE_OUT_TOOL}
}

function run_valgrind_4_executable_all {
	EXE_ARGS_ARR=(${1})
	EXE=${EXE_ARGS_ARR[0]} # First element of the array
	ARGS=${EXE_ARGS_ARR[@]:1} # Every next element

	#EXE="ls"  	# A quick check of the happy path
	#EXE="nothere"	# A quick check of error handling - no such executable
	#EXE=/bin/false	# A quick check of error handling - executable returned != 0

	run_valgrind_4_executable_memcheck   ${EXE} "${ARGS}"
	run_valgrind_4_executable_cachegrind ${EXE} "${ARGS}"
	run_valgrind_4_executable_callgrind  ${EXE} "${ARGS}"
}

is_tool_or_exit valgrind
is_file_or_exit "$FILE_IN"
echo "All OK."
echo "Will perform checks for the following executables and their arguments:"
while IFS= read -r line; do
    echo "$line"
done < "$FILE_IN"

mkdir -p "$DIR_OUT"
while IFS= read -r line; do
    echo "$line"
    run_valgrind_4_executable_all "$line"
done < "$FILE_IN"

echo "Done. All data saved in ${DIR_OUT}"

