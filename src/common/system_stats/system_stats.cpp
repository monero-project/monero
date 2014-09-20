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

#ifdef __linux__

#include "system_stats.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>
#include <cinttypes>

const int CPU_USAGE_CHECK_WAIT_DURATION = 1000000;

namespace
{
  /* Gets a snapshot of CPU times till this point */
  void get_cpu_snapshot(uint64_t &total_cpu_user_before, uint64_t &total_cpu_user_low_before,
    uint64_t &total_cpu_sys_before, uint64_t &total_cpu_idle_before)
  {
    // Get a snapshot of the how much CPU time has been spent for each type.
    FILE *file = fopen("/proc/stat", "r");
    if (!fscanf(file, "cpu %" SCNu64 "%" SCNu64 "%" SCNu64 "%" SCNu64, &total_cpu_user_before, &total_cpu_user_low_before,
      &total_cpu_sys_before, &total_cpu_idle_before))
    {
      throw std::runtime_error("Couldn't read /proc/stat");
    }
    fclose(file);
  }

  /* Find CPU usage given two snapshots */
  double calculate_cpu_load(uint64_t total_cpu_user_before, uint64_t total_cpu_user_low_before,
    uint64_t total_cpu_sys_before, uint64_t total_cpu_idle_before,
    uint64_t total_cpu_user_after, uint64_t total_cpu_user_low_after,
    uint64_t total_cpu_sys_after, uint64_t total_cpu_idle_after)
  {
    uint64_t total;
    double percent;
    if (total_cpu_user_after < total_cpu_user_before || total_cpu_user_low_after < total_cpu_user_low_before ||
      total_cpu_sys_after < total_cpu_sys_before || total_cpu_idle_after < total_cpu_idle_before)
    {
      // Overflow detected.
      throw std::runtime_error("Rare CPU time integer overflow occured. Try again.");
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

namespace system_stats
{
  /* Returns the total amount of main memory in the system (in Bytes) */
  uint64_t get_total_system_memory()
  {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    uint64_t total_mem = mem_info.totalram;
    total_mem *= mem_info.mem_unit;
    return total_mem;
  }

  /* Returns the total amount of the memory that is currently being used (in Bytes) */
  uint64_t get_used_system_memory()
  {
    struct sysinfo mem_info;
    sysinfo(&mem_info);
    uint64_t used_mem = mem_info.totalram - mem_info.freeram;
    used_mem *= mem_info.mem_unit;
    return used_mem;
  }

  /* Returns CPU usage percentage */
  double get_cpu_usage()
  {
    double percent;
    uint64_t total_cpu_user_before, total_cpu_user_low_before, total_cpu_sys_before,
      total_cpu_idle_before;
    uint64_t total_cpu_user_after, total_cpu_user_low_after, total_cpu_sys_after,
      total_cpu_idle_after;

    try
    {
      // Get a CPU usage snapshot
      get_cpu_snapshot(total_cpu_user_before, total_cpu_user_low_before, total_cpu_sys_before,
        total_cpu_idle_before);

      // Wait for sometime and get another snapshot to compare against.
      usleep(CPU_USAGE_CHECK_WAIT_DURATION);

      get_cpu_snapshot(total_cpu_user_after, total_cpu_user_low_after, total_cpu_sys_after,
        total_cpu_idle_after);
    }
    catch (std::runtime_error &e)
    {
      throw e;
    }
    try
    {
      percent = calculate_cpu_load(total_cpu_user_before, total_cpu_user_low_before, total_cpu_sys_before,
        total_cpu_idle_before, total_cpu_user_after, total_cpu_user_low_after, total_cpu_sys_after,
        total_cpu_idle_after);
    }
    catch (std::runtime_error &e)
    {
      throw e;
    }
    return percent;
  }
};

#elif _WIN32

#include "windows.h"
#include <pdh.h>
#include <pdhmsg.h>
#include <stdexcept>
#include <string>
#include <cinttypes>

CONST PWSTR COUNTER_PATH    = L"\\Processor(0)\\% Processor Time";

namespace system_stats
{
  /* Returns the total amount of main memory in the system (in Bytes) */
  uint64_t get_total_system_memory()
  {
    DWORDLONG w_total_mem = memInfo.ullTotalPhys;
    uint64_t total_mem = static_cast<uint64_t>(w_total_mem);
    return total_mem;
  }

  /* Returns the total amount of the memory that is currently being used (in Bytes) */
  uint64_t get_used_system_memory()
  {
    DWORDLONG w_used_mem = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    uint64_t used_mem = static_cast<uint64_t>(w_used_mem);
    return used_mem;
  }

  /* Returns CPU usage percentage */
  double get_cpu_usage()
  {
    HQUERY h_query = NULL;
    HCOUNTER h_counter = NULL;
    DWORD dw_format = PDH_FMT_DOUBLE;
    PDH_STATUS status = PdhOpenQuery(NULL, NULL, &h_query);
    PDH_FMT_COUNTERVALUE item_buffer;
    if (status != ERROR_SUCCESS)
    {
      std::string msg = "Failure while getting CPU usage. `PdhOpenQuery` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    status = PdhAddCounter(h_query, COUNTER_PATH, 0, &h_counter);
    if (status != ERROR_SUCCESS)
    {
      std::string msg = "Failure while getting CPU usage. `PdhAddCounter` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    status = PdhCollectQueryData(h_query);
    if (status != ERROR_SUCCESS)
    {
      std::string msg = "Failure while getting CPU usage. `PdhCollectQueryData` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    status = PdhGetFormattedCounterValue(h_counter, dw_format, (LPDWORD)NULL, &item_buffer);
    if (status != ERROR_SUCCESS)
    {
      std::string msg = "Failure while getting CPU usage. `PdhGetFormattedCounterValue` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    return item_buffer.doubleValue;
  }
};

#elif __APPLE__

#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cinttypes>

const int CPU_USAGE_CHECK_WAIT_DURATION = 1000000;

namespace
{
  /* Gets a snapshot of the number of CPU ticks (idle and total) at this point */
  void get_cpu_snapshot(uint64_t &idle_ticks, uint64_t &total_ticks)
  {
    mach_port_t mach_port = mach_host_self();
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_cpu_load_info_data_t cpu_stats;

    int status = host_statistics(mach_port, HOST_CPU_LOAD_INFO, (host_info_t)&cpu_stats, &count);
    if (status != KERN_SUCCESS)
    {
      std::string msg = "Failure while getting CPU usage. `host_statistics` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    for (int i = 0; i < CPU_STATE_MAX; i++)
    {
      total_ticks += cpu_stats.cpu_ticks[i];
    }
    idle_ticks = cpu_stats.cpu_ticks[CPU_STATE_IDLE];
  }

  /* Find CPU usage given two snapshots */
  double calculate_cpu_load(uint64_t idle_ticks_1, uint64_t total_ticks_1,
    uint64_t idle_ticks_2, uint64_t total_ticks_2)
  {
    long long total_ticks_diff = total_ticks_2 - total_ticks_1;
    long long idle_ticks_diff  = idle_ticks_2 -idle_ticks_1;
    if (total_ticks_diff < 0 ||idle_ticks_diff < 0)
    {
      // Overflow detected.
      throw std::runtime_error("Rare CPU time integer overflow occured. Try again.");
    }
    return 100 * (1.0 - (static_cast<double>(idle_ticks_diff) / total_ticks_diff));
  }
};

namespace system_stats
{
  /* Returns the total amount of main memory in the system (in Bytes) */
  uint64_t get_total_system_memory()
  {
    int mib[] = {CTL_HW, HW_MEMSIZE};
    int64_t physical_memory;
    size_t length = sizeof(int64_t);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    return static_cast<uint64_t>(physical_memory);
  }

  /* Returns the total amount of the memory that is currently being used (in Bytes) */
  uint64_t get_used_system_memory()
  {
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics_data_t vm_stats;

    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    int status = host_page_size(mach_port, &page_size);
    if (status != KERN_SUCCESS)
    {
      std::string msg = "Failure while getting used system memory. `host_page_size` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    status = host_statistics(mach_port, HOST_VM_INFO, (host_info_t)&vm_stats, &count);
    if (status != KERN_SUCCESS)
    {
      std::string msg = "Failure while getting used system memory. `host_statistics` \
        failed with error code: " + std::to_string(status);
      throw std::runtime_error(msg);
    }
    uint64_t used_mem = (static_cast<uint64_t>(vm_stats.active_count) + 
      static_cast<uint64_t>(vm_stats.inactive_count) + 
      static_cast<uint64_t>(vm_stats.wire_count)) * page_size;
    return used_mem;
  }

  /* Returns CPU usage percentage */
  double get_cpu_usage()
  {
    uint64_t total_ticks_1 = 0;
    uint64_t total_ticks_2 = 0;
    uint64_t idle_ticks_1 = 0;
    uint64_t idle_ticks_2 = 0;
    double percent;

    try
    {
      // Get two CPU snapshots separated by some time.
      get_cpu_snapshot(idle_ticks_1, total_ticks_1);
      usleep(CPU_USAGE_CHECK_WAIT_DURATION);
      get_cpu_snapshot(idle_ticks_2, total_ticks_2);
    }
    catch (std::runtime_error &e)
    {
      throw e;
    }
    try
    {
      // Calculate CPU usage based on the two snapshots.
      percent = calculate_cpu_load(idle_ticks_1, total_ticks_1, idle_ticks_2, total_ticks_2);
    }
    catch (std::runtime_error &e)
    {
      throw e;
    }
    return percent;
  }
};

#endif
