#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
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

# The parallelism celing can be controlled with MONERO_PARALLEL_TEST_JOBS env variable,
# for example:
#
# MONERO_PARALLEL_TEST_JOBS=1     tests/run-tests-parallel.py
# MONERO_PARALLEL_TEST_JOBS=nproc tests/run-tests-parallel.py
#
# The minimum between the ceiling and a currently selected reasonable number of threads is used in the end.
# The reasonable number is selected as a number, that delivers the solution in the shortest time.
"""

import os
import sys
import subprocess
import multiprocessing
from timeit import default_timer as timer

NUM_PROC_MAX_REASONABLE = 2 # This might be increased, once the core_tests are divided into many more independent pieces or simply sped up
TESTS_EXCLUDED_REGEX = "unit_tests|functional_tests_rpc" # These tests collide with core_tests
ERR_CODE = 1

def get_forced_job_ceiling():
    try:
        ceiling_str = os.environ['MONERO_PARALLEL_TEST_JOBS']
    except KeyError:
        ceiling = multiprocessing.cpu_count()
        print("No parallelism ceiling selected. Defaulting to nproc, so", ceiling)
    else:
        if ceiling_str == "nproc":
            ceiling = multiprocessing.cpu_count()
        else:
            ceiling = int(ceiling_str)

        print("Parallelism ceiling selected. Using", ceiling, "jobs.")

    return ceiling


def run(num_proc_final, fail_fast=False):
    """
    Run the excluded tests first, then everything but excluded.
    In the current situation, the excluded tests are the most probable and quickest to fail,
    giving an early feedback when fail_fast is set to true.
    """
    cmds = []
    cmds.append(get_ctest_command(num_proc_final, '-R'))
    cmds.append(get_ctest_command(num_proc_final, '-E'))
    
    error = False
    for cmd in cmds:
        status = run_cmd(cmd)
        if status != 0:
            error = True
            if fail_fast:
                return status

    return ERR_CODE if error else 0

def get_ctest_command(num_proc_final, option):
    cmd = ['ctest', '-j{}'.format(num_proc_final), option, TESTS_EXCLUDED_REGEX]
    return cmd

def run_cmd(cmd):
    result = subprocess.run(cmd, stderr=sys.stderr, stdout=sys.stdout)
    return result.returncode


job_ceiling = get_forced_job_ceiling()
num_proc_final = min(NUM_PROC_MAX_REASONABLE, job_ceiling)
print("Job number ceiling is" ,job_ceiling, "and the reasonable maximum is currently", NUM_PROC_MAX_REASONABLE, ".")
print("Using the minimum of the two, so", num_proc_final, "jobs.")

start = timer()
ret = run(num_proc_final, fail_fast=False)
tdiff = timer() - start
print("Testing took:", round(tdiff / 60), "minutes.")
print("Status:", ret)

exit(ret)
