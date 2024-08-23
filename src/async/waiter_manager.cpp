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
#include "waiter_manager.h"

//local headers

//third party headers

//standard headers
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "async"

namespace async
{
//-------------------------------------------------------------------------------------------------------------------
// WaiterManager INTERNAL
//-------------------------------------------------------------------------------------------------------------------
std::uint16_t WaiterManager::clamp_waiter_index(const std::uint16_t nominal_index) noexcept
{
    assert(m_num_managed_waiters > 0);
    if (nominal_index >= m_num_managed_waiters)
        return m_num_managed_waiters - 1;
    return nominal_index;
}
//-------------------------------------------------------------------------------------------------------------------
// WaiterManager INTERNAL
// - note: the order of result checks is intentional based on their assumed importance to the caller
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::wait_impl(std::mutex &mutex_inout,
    std::condition_variable_any &condvar_inout,
    std::atomic<std::int32_t> &counter_inout,
    const std::function<bool()> &condition_checker_func,
    const std::function<std::cv_status(std::condition_variable_any&, std::unique_lock<std::mutex>&)> &wait_func,
    const WaiterManager::ShutdownPolicy shutdown_policy) noexcept
{
    try
    {
        std::unique_lock<std::mutex> lock{mutex_inout};

        // pre-wait checks
        if (condition_checker_func)
        {
            try { if (condition_checker_func()) return Result::CONDITION_TRIGGERED; }
            catch (...)                       { return Result::CONDITION_TRIGGERED; }
        }
        if (shutdown_policy == WaiterManager::ShutdownPolicy::EXIT_EARLY && this->is_shutting_down())
            return Result::SHUTTING_DOWN;

        // wait
        // note: using a signed int for counters means underflow due to reordering of the decrement won't yield a value > 0
        counter_inout.fetch_add(1, std::memory_order_relaxed);
        const std::cv_status wait_status{wait_func(condvar_inout, lock)};
        counter_inout.fetch_sub(1, std::memory_order_relaxed);

        // post-wait checks
        if (condition_checker_func)
        {
            try { if (condition_checker_func()) return Result::CONDITION_TRIGGERED; }
            catch (...)                       { return Result::CONDITION_TRIGGERED; }
        }
        if (this->is_shutting_down())               return Result::SHUTTING_DOWN;
        if (wait_status == std::cv_status::timeout) return Result::TIMEOUT;

        return Result::DONE_WAITING;
    }
    catch (...) { return Result::CRITICAL_EXCEPTION; }
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
WaiterManager::WaiterManager(const std::uint16_t num_managed_waiters) :
    //we always want at least one waiter slot to avoid UB
    m_num_managed_waiters{static_cast<uint16_t>(num_managed_waiters > 0 ? num_managed_waiters : 1)}
{
    m_waiter_mutexes      = std::vector<std::mutex>{m_num_managed_waiters};
    m_conditional_waiters = std::vector<ConditionalWaiterContext>{m_num_managed_waiters};
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::wait(const std::uint16_t waiter_index,
    const WaiterManager::ShutdownPolicy shutdown_policy,
    const WaitPriority wait_priority) noexcept
{
    return this->wait_impl(m_waiter_mutexes[this->clamp_waiter_index(waiter_index)],
            (wait_priority == WaitPriority::HIGH) ? m_primary_shared_cond_var : m_secondary_shared_cond_var,
            (wait_priority == WaitPriority::HIGH) ? m_num_primary_waiters     : m_num_secondary_waiters,
            nullptr,
            [](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock) -> std::cv_status
            {
                cv_inout.wait(lock);
                return std::cv_status::no_timeout;
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::wait_for(const std::uint16_t waiter_index,
    const std::chrono::nanoseconds &duration,
    const WaiterManager::ShutdownPolicy shutdown_policy,
    const WaitPriority wait_priority) noexcept
{
    return this->wait_impl(m_waiter_mutexes[this->clamp_waiter_index(waiter_index)],
            (wait_priority == WaitPriority::HIGH) ? m_primary_shared_cond_var : m_secondary_shared_cond_var,
            (wait_priority == WaitPriority::HIGH) ? m_num_primary_waiters     : m_num_secondary_waiters,
            nullptr,
            [&duration](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock_inout) -> std::cv_status
            {
                return cv_inout.wait_for(lock_inout, duration);
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::wait_until(const std::uint16_t waiter_index,
    const std::chrono::time_point<std::chrono::steady_clock> &timepoint,
    const WaiterManager::ShutdownPolicy shutdown_policy,
    const WaitPriority wait_priority) noexcept
{
    return this->wait_impl(m_waiter_mutexes[this->clamp_waiter_index(waiter_index)],
            (wait_priority == WaitPriority::HIGH) ? m_primary_shared_cond_var : m_secondary_shared_cond_var,
            (wait_priority == WaitPriority::HIGH) ? m_num_primary_waiters     : m_num_secondary_waiters,
            nullptr,
            [&timepoint](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock_inout) -> std::cv_status
            {
                return cv_inout.wait_until(lock_inout, timepoint);
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::conditional_wait(const std::uint16_t waiter_index,
    const std::function<bool()> &condition_checker_func,
    const WaiterManager::ShutdownPolicy shutdown_policy) noexcept
{
    const std::uint16_t clamped_waiter_index{this->clamp_waiter_index(waiter_index)};
    return this->wait_impl(m_waiter_mutexes[clamped_waiter_index],
            m_conditional_waiters[clamped_waiter_index].cond_var,
            m_conditional_waiters[clamped_waiter_index].num_waiting,
            condition_checker_func,
            [](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock_inout) -> std::cv_status
            {
                cv_inout.wait(lock_inout);
                return std::cv_status::no_timeout;
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::conditional_wait_for(const std::uint16_t waiter_index,
    const std::function<bool()> &condition_checker_func,
    const std::chrono::nanoseconds &duration,
    const WaiterManager::ShutdownPolicy shutdown_policy) noexcept
{
    const std::uint16_t clamped_waiter_index{this->clamp_waiter_index(waiter_index)};
    return this->wait_impl(m_waiter_mutexes[clamped_waiter_index],
            m_conditional_waiters[clamped_waiter_index].cond_var,
            m_conditional_waiters[clamped_waiter_index].num_waiting,
            condition_checker_func,
            [&duration](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock_inout) -> std::cv_status
            {
                return cv_inout.wait_for(lock_inout, duration);
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
WaiterManager::Result WaiterManager::conditional_wait_until(const std::uint16_t waiter_index,
    const std::function<bool()> &condition_checker_func,
    const std::chrono::time_point<std::chrono::steady_clock> &timepoint,
    const WaiterManager::ShutdownPolicy shutdown_policy) noexcept
{
    const std::uint16_t clamped_waiter_index{this->clamp_waiter_index(waiter_index)};
    return this->wait_impl(m_waiter_mutexes[clamped_waiter_index],
            m_conditional_waiters[clamped_waiter_index].cond_var,
            m_conditional_waiters[clamped_waiter_index].num_waiting,
            condition_checker_func,
            [&timepoint](std::condition_variable_any &cv_inout, std::unique_lock<std::mutex> &lock_inout)
            -> std::cv_status
            {
                return cv_inout.wait_until(lock_inout, timepoint);
            },
            shutdown_policy
        );
}
//-------------------------------------------------------------------------------------------------------------------
void WaiterManager::notify_one() noexcept
{
    // try to notify a normal waiter
    if (m_num_primary_waiters.load(std::memory_order_relaxed) > 0)
    {
        m_primary_shared_cond_var.notify_one();
        return;
    }

    // try to notify a sleepy waiter
    if (m_num_secondary_waiters.load(std::memory_order_relaxed) > 0)
    {
        m_secondary_shared_cond_var.notify_one();
        return;
    }

    // find a conditional waiter to notify
    for (ConditionalWaiterContext &conditional_waiter : m_conditional_waiters)
    {
        if (conditional_waiter.num_waiting.load(std::memory_order_relaxed) > 0)
        {
            conditional_waiter.cond_var.notify_one();
            break;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WaiterManager::notify_all() noexcept
{
    m_primary_shared_cond_var.notify_all();
    m_secondary_shared_cond_var.notify_all();
    for (ConditionalWaiterContext &conditional_waiter : m_conditional_waiters)
        conditional_waiter.cond_var.notify_all();
}
//-------------------------------------------------------------------------------------------------------------------
void WaiterManager::notify_conditional_waiter(const std::uint16_t waiter_index,
    std::function<void()> condition_setter_func) noexcept
{
    const std::uint16_t clamped_waiter_index{this->clamp_waiter_index(waiter_index)};
    if (condition_setter_func) try { condition_setter_func(); } catch (...) {}
    // tap the mutex here to synchronize with conditional waiters
    { const std::lock_guard<std::mutex> lock{m_waiter_mutexes[clamped_waiter_index]}; };
    // notify all because if there are multiple threads waiting on this index (not recommended, but possible),
    //   we don't know which one actually cares about this condition function
    m_conditional_waiters[clamped_waiter_index].cond_var.notify_all();
}
//-------------------------------------------------------------------------------------------------------------------
void WaiterManager::shut_down() noexcept
{
    // shut down
    m_shutting_down.store(true, std::memory_order_relaxed);

    // tap all the mutexes to synchronize with waiters
    for (std::mutex &mutex : m_waiter_mutexes)
        try { const std::lock_guard<std::mutex> lock{mutex}; } catch (...) {}

    // notify all waiters
    this->notify_all();
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace async
