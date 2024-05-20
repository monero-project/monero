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

/// threadpool

#pragma once

//local headers
#include "scoped_notification.h"
#include "sleepy_task_queue.h"
#include "task_types.h"
#include "token_queue.h"
#include "waiter_manager.h"

//third-party headers
#include <boost/container/map.hpp>
#include <boost/optional/optional.hpp>

//standard headers
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

//forward declarations


namespace async
{

/// join signal/token/condition helper types
using join_signal_t    = std::shared_ptr<std::atomic<bool>>;
using join_token_t     = std::shared_ptr<ScopedNotification>;
using join_condition_t = std::function<bool()>;

/// fanout token helper type
using fanout_token_t = std::unique_ptr<ScopedNotification>;

struct FanoutCondition final
{
    std::shared_ptr<std::atomic<std::uint64_t>> worker_index;
    std::function<bool()> condition;
};

/// thread pool
class Threadpool final
{
    /// clean up pass on the sleepy queues
    void perform_sleepy_queue_maintenance();

    /// submit task types
    void submit_simple_task(SimpleTask &&simple_task);
    void submit_sleepy_task(SleepyTask &&sleepy_task);

    /// get a task to run
    boost::optional<task_t> try_get_simple_task_to_run(const unsigned char max_task_priority,
        const std::uint16_t worker_index);
    boost::optional<task_t> try_wait_for_sleepy_task_to_run(const unsigned char max_task_priority,
        const std::uint16_t worker_index,
        const std::function<
                WaiterManager::Result(
                        const std::uint16_t,
                        const std::chrono::time_point<std::chrono::steady_clock>&,
                        const WaiterManager::ShutdownPolicy
                    )
            > &custom_wait_until);
    boost::optional<task_t> try_get_task_to_run(const unsigned char max_task_priority,
        const std::uint16_t worker_index,
        const std::function<
                WaiterManager::Result(
                        const std::uint16_t,
                        const std::chrono::time_point<std::chrono::steady_clock>&,
                        const WaiterManager::ShutdownPolicy
                    )
            > &custom_wait_until) noexcept;

public:
    /// run as a pool worker
    /// - this function is only invoked when launching pool workers and from within the pool destructor
    void run_as_worker_DONT_CALL_ME();
    /// run as a pool fanout worker
    /// - this function is only invoked when launching pool fanout workers
    void run_as_fanout_worker_DONT_CALL_ME();

//constructors
    /// default constructor: disabled
    Threadpool() = delete;
    /// normal constructor: from config
    Threadpool(const unsigned char max_priority_level,
        const std::uint16_t num_managed_workers,
        const unsigned char num_submit_cycle_attempts,
        const std::chrono::nanoseconds max_wait_duration);

    /// disable copy/moves so references to this object can't be invalidated until this object's lifetime ends
    Threadpool& operator=(Threadpool&&) = delete;

//destructor
    /// destroy the threadpool
    /// 1) shuts down the pool
    /// 2) joins all worker threads
    /// 3) clears out any remaining tasks
    ///    - note that this ensures any ScopedNotifications attached to tasks will be executed before the pool dies,
    ///      which ensures references in those notifications
    ~Threadpool();

//member functions
    /// submit a task
    /// - note: if submit() returns true, then it is guaranteed the submission succeeded; otherwise it is unspecified
    ///   what happened to the task (it may have been submitted, or an exception may have caused it to be dropped)
    bool submit(TaskVariant task) noexcept;

    /// toolkit for manually joining on a set of tasks
    /// - how to use this:
    ///   1) make a new join signal in the thread that will be joining on a set of tasks yet to be launched
    ///   2) [get_join_token()]: get a new join token using the join signal
    ///   3) save a copy of the token in the lambda capture of each task in the set of tasks that you want to join on
    ///   4) [get_join_condition()]: consume the joining thread's copy of the join token and the join signal to get the
    ///      join condition
    ///   5) call ThreadPool::work_while_waiting() from the joining thread, using that join condition
    ///
    /// - PRECONDITION: the thread that joins on a join token must be the same thread that created that token
    /// - PRECONDITION: there must be NO living copies of a join token after the corresponding threadpool has died
    static join_signal_t make_join_signal();
    join_token_t get_join_token(join_signal_t &join_signal_inout);
    static join_condition_t get_join_condition(join_signal_t &&join_signal_in, join_token_t &&join_token_in);

    void work_while_waiting(const std::chrono::time_point<std::chrono::steady_clock> &deadline,
        const unsigned char max_task_priority = 0);
    void work_while_waiting(const std::chrono::nanoseconds &duration, const unsigned char max_task_priority = 0);
    void work_while_waiting(const std::function<bool()> &wait_condition_func, const unsigned char max_task_priority = 0);

    /// launch a temporary worker
    fanout_token_t launch_temporary_worker();

    /// shut down the threadpool
    void shut_down() noexcept;

    /// id of this threadpool
    std::uint64_t threadpool_id()       const { return m_threadpool_id; }
    std::uint64_t threadpool_owner_id() const { return m_threadpool_owner_id; }

private:
//member variables
    /// config
    const std::uint64_t m_threadpool_id;  //unique identifier for this threadpool
    const std::uint64_t m_threadpool_owner_id;  //unique identifier for the thread that owns this threadpool
    const unsigned char m_max_priority_level;  //note: priority 0 is the 'highest' priority
    const std::uint16_t m_num_queues;  //num workers + 1 for the threadpool owner
    const unsigned char m_num_submit_cycle_attempts;
    const std::chrono::nanoseconds m_max_wait_duration;

    /// worker context
    std::vector<std::thread> m_workers;
    std::vector<std::thread> m_fanout_workers;  //extra workers that can be manually activated

    /// queues
    std::vector<std::vector<TokenQueue<task_t>>> m_task_queues;  //outer vector: priorities, inner vector: workers
    std::vector<SleepyTaskQueue> m_sleepy_task_queues;   //vector: workers
    TokenQueue<FanoutCondition> m_fanout_condition_queue;
    std::atomic<std::uint16_t> m_normal_queue_submission_counter{0};
    std::atomic<std::uint16_t> m_sleepy_queue_submission_counter{0};
    std::atomic<std::uint64_t> m_num_unclaimed_sleepy_tasks{0};

    // waiter manager
    WaiterManager m_waiter_manager;
};


/// default priorities
enum DefaultPriorityLevels : unsigned char
{
    MAX = 0,
    MEDIUM,
    LOW,
    MIN = LOW
};

/// default threadpool
inline Threadpool& get_default_threadpool()
{
    static Threadpool default_threadpool{
            static_cast<unsigned char>(DefaultPriorityLevels::MIN),
            static_cast<uint16_t>(std::max(2u, std::thread::hardware_concurrency()) - 1),
            20,
            std::chrono::milliseconds(500)
        };
    return default_threadpool;
}

} //namespace async
