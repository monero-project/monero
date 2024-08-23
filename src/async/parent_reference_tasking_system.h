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

/// Sean Parent's reference thread pool
/// ref: https://github.com/stlab/libraries/blob/main/stlab/concurrency/default_executor.hpp

#pragma once

//local headers

//third-party headers

//standard headers
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

//forward declarations


namespace parent
{

enum class TokenQueueResult : unsigned char
{
    SUCCESS,
    SHUTTING_DOWN,
    QUEUE_EMPTY,
    TRY_LOCK_FAIL
};

/// async token queue
template <typename TokenT>
class TokenQueue final
{
    struct element_t final
    {
        std::size_t index;
        TokenT token;

        template <class T>
        element_t(const std::size_t index, T&& new_element) :
            index{index},
            token{std::forward<T>(new_element)}
        {}

        bool operator<(const element_t& other) const
        {
            return this->index < other.index;
        };
    };

    // must be called under lock with non-empty queue
    TokenT pop_not_empty()
    {
        TokenT temp{std::move(m_queue.front()).token};
        std::pop_heap(std::begin(m_queue), std::end(m_queue));
        m_queue.pop_back();
        return temp;
    }

public:
//constructors
    /// resurrect default constructor
    TokenQueue() = default;

//member functions
    /// try to add an element to the top
    template <typename T>
    TokenQueueResult try_push(T &&new_element_in)
    {
        {
            std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
            if (!lock.owns_lock())
                return TokenQueueResult::TRY_LOCK_FAIL;

            m_queue.emplace_back(m_index_counter++, std::forward<T>(new_element_in));
            std::push_heap(std::begin(m_queue), std::end(m_queue));
        }
        m_condvar.notify_one();
        return TokenQueueResult::SUCCESS;
    }
    /// add an element to the top (always succeeds)
    template <typename T>
    void force_push(T &&new_element_in)
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_queue.emplace_back(m_index_counter++, std::forward<T>(new_element_in));
            std::push_heap(std::begin(m_queue), std::end(m_queue));
        }
        m_condvar.notify_one();
    }

    /// try to remove an element from the bottom
    TokenQueueResult try_pop(TokenT &token_out)
    {
        // try to lock the queue, then check if there are any elements
        std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
        if (!lock.owns_lock())
            return TokenQueueResult::TRY_LOCK_FAIL;
        if (m_queue.size() == 0)
            return TokenQueueResult::QUEUE_EMPTY;

        // pop the bottom element
        token_out = pop_not_empty();
        return TokenQueueResult::SUCCESS;
    }
    /// wait until you can remove an element from the bottom
    TokenQueueResult force_pop(TokenT &token_out)
    {
        // lock the queue then wait until there is an element or the queue is shutting down
        std::unique_lock<std::mutex> lock{m_mutex};
        while (m_queue.size() == 0 && !m_shutting_down)
            m_condvar.wait(lock);
        if (m_queue.size() == 0 && m_shutting_down)
            return TokenQueueResult::SHUTTING_DOWN;

        // pop the bottom element
        token_out = pop_not_empty();
        return TokenQueueResult::SUCCESS;
    }

    void shut_down()
    {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_shutting_down = true;
        }
        m_condvar.notify_all();
    }

private:
//member variables
    /// queue status
    bool m_shutting_down{false};

    /// queue context
    std::vector<element_t> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::size_t m_index_counter{0};
};


/// Sean Parent's reference threadpool
class ThreadPool final
{
    using queue_t = TokenQueue<std::function<void()>>;
public:
    ThreadPool(const std::uint16_t num_workers) :
        m_queues{static_cast<std::size_t>(num_workers > 0 ? num_workers : 1)}
    {
        m_workers.reserve(m_queues.size());
        for (std::uint16_t worker_index{0}; worker_index < m_queues.size(); ++worker_index)
        {
            try
            {
                m_workers.emplace_back(
                        [this, worker_index]() mutable
                        {
                            try { this->run(worker_index); } catch (...) { /* can't do anything */ }
                        }
                    );
            }
            catch (...) { /* can't do anything */ }
        }
    }

    ~ThreadPool()
    {
        this->shut_down();
    }

    void run(const std::uint16_t worker_index)
    {
        while (true)
        {
            // cycle all the queues
            std::function<void()> task;
            for (std::uint16_t i{0}; i < m_queues.size(); ++i)
            {
                if (m_queues[(i + worker_index) % m_queues.size()].try_pop(task) == TokenQueueResult::SUCCESS)
                    break;
            }

            // fallback: force pop
            if (!task &&
                m_queues[worker_index].force_pop(task) == TokenQueueResult::SHUTTING_DOWN)
                break;

            // run the task
            try { task(); } catch (...) {}
        } 
    }

    template <typename F>
    void submit(F &&function)
    {
        std::uint32_t start_counter{m_submit_rotation_counter.fetch_add(1, std::memory_order_relaxed)};

        // cycle all the queues
        for (std::uint32_t i{0}; i < m_queues.size() * 40; ++i)
        {
            if (m_queues[(i + start_counter) % m_queues.size()].try_push(std::forward<F>(function)) ==
                    TokenQueueResult::SUCCESS)
                return;
        }

        // fallback: force push
        m_queues[start_counter % m_queues.size()].force_push(std::forward<F>(function));
    }

    void shut_down()
    {
        for (queue_t &queue : m_queues)
            try { queue.shut_down(); } catch (...) {}

        for (std::thread &worker : m_workers)
            try { if (worker.joinable()) worker.join(); } catch (...) {}
    }

private:
//member variables
    std::vector<queue_t> m_queues;
    std::vector<std::thread> m_workers;
    std::atomic<std::uint16_t> m_submit_rotation_counter{0};
};

} //namespace parent
