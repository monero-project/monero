// Copyright (c) 2022, The Monero Project
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

//paired header
#include "task_types.h"

//local headers

//third party headers

//standard headers
#include <chrono>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "async"

namespace async
{
//-------------------------------------------------------------------------------------------------------------------
std::chrono::time_point<std::chrono::steady_clock> wake_time(const WakeTime waketime)
{
    return waketime.start_time + waketime.duration;
}
//-------------------------------------------------------------------------------------------------------------------
bool sleepy_task_is_awake(const SleepyTask &task)
{
    return wake_time(task.wake_time) <= std::chrono::steady_clock::now();
}
//-------------------------------------------------------------------------------------------------------------------
bool sleeping_task_is_unclaimed(const SleepingTask &task)
{
    return task.status.load(std::memory_order_acquire) == SleepingTaskStatus::UNCLAIMED;
}
//-------------------------------------------------------------------------------------------------------------------
bool sleeping_task_is_dead(const SleepingTask &task)
{
    return task.status.load(std::memory_order_acquire) == SleepingTaskStatus::DEAD;
}
//-------------------------------------------------------------------------------------------------------------------
void unclaim_sleeping_task(SleepingTask &sleeping_task_inout)
{
    sleeping_task_inout.status.store(SleepingTaskStatus::UNCLAIMED, std::memory_order_release);
}
//-------------------------------------------------------------------------------------------------------------------
void reserve_sleeping_task(SleepingTask &sleeping_task_inout)
{
    sleeping_task_inout.status.store(SleepingTaskStatus::RESERVED, std::memory_order_release);
}
//-------------------------------------------------------------------------------------------------------------------
void kill_sleeping_task(SleepingTask &sleeping_task_inout)
{
    sleeping_task_inout.status.store(SleepingTaskStatus::DEAD, std::memory_order_release);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace async
