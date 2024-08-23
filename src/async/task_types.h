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

/// Task types for a threadpool

#pragma once

//local headers
#include "common/variant.h"

//third-party headers
#include <boost/none.hpp>

//standard headers
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <type_traits>

//forward declarations


namespace async
{

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// waketime
/// - waketime = start time + duration
/// - if 'start time == 0' when a task is received, then the start time will be set to the time at that moment
///   - this allows task-makers to specify either a task's waketime or its sleep duration from the moment it is
///     submitted, e.g. for task continuations that are defined well in advance of when they are submitted
struct WakeTime final
{
    std::chrono::time_point<std::chrono::steady_clock> start_time{
            std::chrono::time_point<std::chrono::steady_clock>::min()
        };
    std::chrono::nanoseconds duration{0};
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// possible statuses of a sleepy task in a sleepy queue
enum class SleepingTaskStatus : unsigned char
{
    /// task is waiting for a worker
    UNCLAIMED,
    /// task is reserved by a worker
    RESERVED,
    /// task has been consumed by a worker
    DEAD
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

struct SimpleTask;
struct SleepyTask;

/// task
using TaskVariant = tools::optional_variant<SimpleTask, SleepyTask>;
using task_t      = std::function<TaskVariant()>;  //tasks auto-return their continuation (or an empty variant)

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// pending task
struct SimpleTask final
{
    unsigned char priority;
    task_t task;

    SimpleTask() = default;
    SimpleTask(const SimpleTask&) = delete;
    SimpleTask(SimpleTask&&) = default;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// sleepy task
struct SleepyTask final
{
    SimpleTask simple_task;
    WakeTime wake_time;

    SleepyTask() = default;
    SleepyTask(const SleepyTask&) = delete;
    SleepyTask(SleepyTask&&) = default;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// sleeping task
/// note: we need an extra type for sleeping tasks because SleepyTasks are not copy-constructible, and the atomic status
///       is not move-constructible, which means SleepingTasks are very hard to move around
struct SleepingTask final
{
    SleepyTask sleepy_task;
    std::atomic<SleepingTaskStatus> status{SleepingTaskStatus::UNCLAIMED};

    /// normal constructor (this struct is not movable or copyable, so it needs some help...)
    SleepingTask(SleepyTask &&sleepy_task, const SleepingTaskStatus status) :
        sleepy_task{std::move(sleepy_task)}, status{status}
    {}
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// copyable tasks
template <
    typename F,
    typename std::enable_if<
            std::is_same<TaskVariant, decltype(std::declval<F>()())>::value
            && std::is_constructible<std::function<TaskVariant()>, F>::value,
            bool
        >::type = true
>
std::function<TaskVariant()> as_task_function(F &&func)
{
    return std::move(func);
}

template <
    typename F,
    typename std::enable_if<
            std::is_same<void, decltype(std::declval<F>()())>::value
            && std::is_constructible<std::function<void()>, F>::value,
            bool
        >::type = true
>
std::function<TaskVariant()> as_task_function(F &&func)
{
    return [l_func = std::move(func)]() -> TaskVariant { l_func(); return boost::none; };
}

/// move-only tasks
//todo: std::packaged_task is inefficient for this use-case but std::move_only_function is C++23
template <
    typename F,
    typename std::enable_if<
            std::is_same<TaskVariant, decltype(std::declval<F>()())>::value
            && !std::is_constructible<std::function<TaskVariant()>, F>::value,
            bool
        >::type = true
>
std::function<TaskVariant()> as_task_function(F &&func)
{
    return
        [l_func = std::make_shared<std::packaged_task<TaskVariant()>>(std::move(func))]
        () -> TaskVariant
        {
            if (!l_func) return boost::none;
            try { (*l_func)(); return l_func->get_future().get(); } catch (...) {}
            return boost::none;
        };
}

template <
    typename F,
    typename std::enable_if<
            std::is_same<void, decltype(std::declval<F>()())>::value
            && !std::is_constructible<std::function<void()>, F>::value,
            bool
        >::type = true
>
std::function<TaskVariant()> as_task_function(F &&func)
{
    return
        [l_func = std::make_shared<std::packaged_task<void()>>(std::move(func))]
        () -> TaskVariant
        {
            if (!l_func) return boost::none;
            try { (*l_func)(); } catch (...) {}
            return boost::none;
        };
}

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// make simple task
template <typename F>
SimpleTask make_simple_task(const unsigned char priority, F &&func)
{
    static_assert(std::is_same<decltype(func()), TaskVariant>::value, "tasks must return task variants");
    return SimpleTask{
            .priority = priority,
            .task     = as_task_function(std::forward<F>(func))
        };
}

/// make sleepy task
template <typename F>
SleepyTask make_sleepy_task(const unsigned char priority, const WakeTime &waketime, F &&func)
{
    return {
            make_simple_task(priority, std::forward<F>(func)),
            waketime
        };
}
template <typename F>
SleepyTask make_sleepy_task(const unsigned char priority, const std::chrono::nanoseconds &duration, F &&func)
{
    // note: the start time is left undefined/zero until the task gets scheduled
    WakeTime waketime{};
    waketime.duration = duration;

    return {
            make_simple_task(priority, std::forward<F>(func)),
            waketime
        };
}
template <typename F>
SleepyTask make_sleepy_task(const unsigned char priority,
    const std::chrono::time_point<std::chrono::steady_clock> &waketime,
    F &&func)
{
    return {
            make_simple_task(priority, std::forward<F>(func)),
            WakeTime{ .start_time = waketime, .duration = std::chrono::nanoseconds{0} }
        };
}

//todo
std::chrono::time_point<std::chrono::steady_clock> wake_time(const WakeTime waketime);

//todo
bool sleepy_task_is_awake(const SleepyTask &task);
bool sleeping_task_is_unclaimed(const SleepingTask &task);
bool sleeping_task_is_dead(const SleepingTask &task);
void unclaim_sleeping_task(SleepingTask &sleeping_task_inout);
void reserve_sleeping_task(SleepingTask &sleeping_task_inout);
void kill_sleeping_task(SleepingTask &sleeping_task_inout);

} //namespace async
