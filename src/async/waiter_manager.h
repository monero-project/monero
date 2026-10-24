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

/// Coordinates async wait/notify for a threadpool.

#pragma once

//local headers

//third-party headers

//standard headers
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

//forward declarations


namespace async
{

/// WaiterManager
/// - performance will decrease significantly if multiple threads try to claim the same waiter index
/// - notify_one() prioritizes: primary waiters > secondary waiters > conditional waiters
///   - this function has several race conditions that can mean no worker gets notified even if there are several actually
///     waiting (these are non-critical race conditions that marginally reduce throughput under low to moderate load)
///   - there is also a race condition where a conditional waiter gets notified but ends up detecting its condition was
///     triggered, implying it will go down some custom upstream control path instead of the normal path that
///     'notify_one()' is aimed at (e.g. 'go find a task to work on'); (this marginally reduces throughput under moderate
///     to high load)
/// - conditional waiting is designed so a conditional waiter will never wait after its condition is set if a conditional
///   notify is used to set the condition
///   - COST: the condition setting/checking is protected by a unique lock, so any real system WILL waste time fighting
///     over those locks (to maximize throughput, consider using large task graphs to avoid manual joining and other
///     mechanisms that use conditional waits)
/// - 'shutting down' the manager means
///   A) existing waiters will all be woken up
///   B) future waiters using 'ShutdownPolicy::EXIT_EARLY' will simply exit without waiting
class WaiterManager final
{
public:
    enum class ShutdownPolicy : unsigned char
    {
        WAIT,
        EXIT_EARLY
    };

    enum class WaitPriority : unsigned char
    {
        HIGH,
        LOW
    };

    enum class Result : unsigned char
    {
        CONDITION_TRIGGERED,
        SHUTTING_DOWN,
        TIMEOUT,
        DONE_WAITING,
        CRITICAL_EXCEPTION
    };

private:
    struct ConditionalWaiterContext final
    {
        std::atomic<std::int32_t> num_waiting;
        std::condition_variable_any cond_var;
    };

    std::uint16_t clamp_waiter_index(const std::uint16_t nominal_index) noexcept;

    /// wait
    Result wait_impl(std::mutex &mutex_inout,
        std::condition_variable_any &condvar_inout,
        std::atomic<std::int32_t> &counter_inout,
        const std::function<bool()> &condition_checker_func,
        const std::function<std::cv_status(std::condition_variable_any&, std::unique_lock<std::mutex>&)> &wait_func,
        const ShutdownPolicy shutdown_policy) noexcept;

public:
//constructors
    WaiterManager(const std::uint16_t num_managed_waiters);

//overloaded operators
    /// disable copy/moves so references to this object can't be invalidated until this object's lifetime ends
    WaiterManager& operator=(WaiterManager&&) = delete;

//member functions
    Result wait(const std::uint16_t waiter_index,
        const ShutdownPolicy shutdown_policy,
        const WaitPriority wait_priority) noexcept;
    Result wait_for(const std::uint16_t waiter_index,
        const std::chrono::nanoseconds &duration,
        const ShutdownPolicy shutdown_policy,
        const WaitPriority wait_priority) noexcept;
    Result wait_until(const std::uint16_t waiter_index,
        const std::chrono::time_point<std::chrono::steady_clock> &timepoint,
        const ShutdownPolicy shutdown_policy,
        const WaitPriority wait_priority) noexcept;
    Result conditional_wait(const std::uint16_t waiter_index,
        const std::function<bool()> &condition_checker_func,
        const ShutdownPolicy shutdown_policy) noexcept;
    Result conditional_wait_for(const std::uint16_t waiter_index,
        const std::function<bool()> &condition_checker_func,
        const std::chrono::nanoseconds &duration,
        const ShutdownPolicy shutdown_policy) noexcept;
    Result conditional_wait_until(const std::uint16_t waiter_index,
        const std::function<bool()> &condition_checker_func,
        const std::chrono::time_point<std::chrono::steady_clock> &timepoint,
        const ShutdownPolicy shutdown_policy) noexcept;

    void notify_one() noexcept;
    void notify_all() noexcept;
    void notify_conditional_waiter(const std::uint16_t waiter_index,
        std::function<void()> condition_setter_func) noexcept;

    void shut_down() noexcept;

    bool is_shutting_down() const noexcept { return m_shutting_down.load(std::memory_order_relaxed); }

private:
//member variables
    /// config
    const std::uint16_t m_num_managed_waiters;

    /// manager status
    std::atomic<std::int32_t> m_num_primary_waiters{0};
    std::atomic<std::int32_t> m_num_secondary_waiters{0};
    std::atomic<bool> m_shutting_down{false};

    /// synchronization
    std::vector<std::mutex> m_waiter_mutexes;
    std::condition_variable_any m_primary_shared_cond_var;
    std::condition_variable_any m_secondary_shared_cond_var;

    /// conditional waiters
    std::vector<ConditionalWaiterContext> m_conditional_waiters;
};

} //namespace async
