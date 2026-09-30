// Copyright (c) 2023, The Monero Project
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

/// Queue of sleepy tasks.

#pragma once

//local headers
#include "task_types.h"

//third-party headers
#include <boost/container/map.hpp>

//standard headers
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <type_traits>

//forward declarations


namespace async
{

/// SleepyTaskQueue
/// - PRECONDITION: a user of a sleepy task queue with a pointer/reference to a task in that queue should ONLY change
///   the task's status from RESERVED to UNCLAIMED/DEAD (and not any other direction)
///   - once a RESERVED task's status has been changed, the user should assume they no longer have valid access to the
///     task
///   - only change a task's status from RESERVED -> UNCLAIMED if its contents will be left in a valid state after the
///     change (e.g. the internal task shouldn't be in a moved-from state)
class SleepyTaskQueue final
{
public:
//overloaded operators
    /// disable copy/move since this may be accessed concurrently from multiple threads
    SleepyTaskQueue& operator=(SleepyTaskQueue&&) = delete;

//member functions
    /// force push a sleepy task into the queue
    void force_push(SleepyTask &&task);
    /// try to push a sleepy task into the queue
    bool try_push(SleepyTask &&task);

    /// try to swap an existing sleepy task with a task that wakes up sooner
    /// - this function does not add/remove elements from the queue; instead, it simply adjusts task statuses then
    ///   swaps pointers
    /// - if 'task_inout == nullptr', then we set it to the unclaimed task with the lowest waketime
    /// - the cost of this function may be higher than expected if there are many tasks with higher priority than our
    ///   allowed max priority
    bool try_swap(const unsigned char max_task_priority, SleepingTask* &task_inout);

    /// try to clean up the queue
    /// - remove dead tasks
    /// - extract awake unclaimed tasks
    std::list<std::unique_ptr<SleepingTask>> try_perform_maintenance(
        const std::chrono::time_point<std::chrono::steady_clock> &current_time);

private:
//member variables
    /// queue context (sorted by waketime)
    boost::container::multimap<std::chrono::time_point<std::chrono::steady_clock>::rep,
        std::unique_ptr<SleepingTask>> m_queue;
    std::mutex m_mutex;
};

} //namespace async
