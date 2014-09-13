// Copyright (c) 2014, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "system_stats.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>

const int CPU_USAGE_WAIT_DURATION = 1000000;

namespace system_stats
{
  /* Returns the total amount of main memory in the system */
  long long get_total_system_memory()
  {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long long total_mem = mem_info.totalram;
    total_mem *= mem_info.mem_unit;
    return total_mem;
  }

  /* Returns the total amount of the memory that is currently being used */
  long long get_used_system_memory()
  {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    long long used_mem = mem_info.totalram - mem_info.freeram;
    used_mem *= mem_info.mem_unit;
    return used_mem;
  }

  /* Returns CPU usage percentage */
  double get_cpu_usage()
  {
    double percent;
    FILE* file;
    unsigned long long total_cpu_user_before, total_cpu_user_low_before, total_cpu_sys_before,
      total_cpu_idle_before;
    unsigned long long total;
    unsigned long long total_cpu_user_after, total_cpu_user_low_after, total_cpu_sys_after,
      total_cpu_idle_after;

    // Get a snapshot of the how much CPU time has been spent for each type.
    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %Ld %Ld %Ld %Ld", &total_cpu_user_before, &total_cpu_user_low_before,
      &total_cpu_sys_before, &total_cpu_idle_before);
    fclose(file);

    // Wait for sometime and get another snapshot to compare against.
    usleep(CPU_USAGE_WAIT_DURATION);

    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %Ld %Ld %Ld %Ld", &total_cpu_user_after, &total_cpu_user_low_after,
      &total_cpu_sys_after, &total_cpu_idle_after);
    fclose(file);

    if (total_cpu_user_after < total_cpu_user_before || total_cpu_user_low_after < total_cpu_user_low_before ||
      total_cpu_sys_after < total_cpu_sys_before || total_cpu_idle_after < total_cpu_idle_before)
    {
      // Overflow detected.
      percent = -1.0;
    }
    else
    {
      total = (total_cpu_user_after - total_cpu_user_before) + (total_cpu_user_low_after - total_cpu_user_low_before) +
        (total_cpu_sys_after - total_cpu_sys_before);
      percent = total;
      total += (total_cpu_idle_after - total_cpu_idle_before);
      percent /= total;
      percent *= 100;
    }
    return percent;
  }
};
