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

/// Simple token queue.

#pragma once

//local headers

//third-party headers

//standard headers
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>

//forward declarations


namespace async
{

enum class TokenQueueResult : unsigned char
{
    SUCCESS,
    QUEUE_EMPTY,
    TRY_LOCK_FAIL,
    SHUTTING_DOWN,
    QUEUE_NOT_EMPTY
};

/// async token queue
template <typename TokenT>
class TokenQueue final
{
public:
//member functions
    /// try to add an element to the top
    template <typename T>
    TokenQueueResult try_push(T &&new_element_in)
    {
        {
            std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
            if (!lock.owns_lock())
                return TokenQueueResult::TRY_LOCK_FAIL;

            m_queue.emplace_back(std::forward<T>(new_element_in));
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
            m_queue.emplace_back(std::forward<T>(new_element_in));
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
        token_out = std::move(m_queue.front());
        m_queue.pop_front();
        return TokenQueueResult::SUCCESS;
    }
    /// remove an element from the bottom (always succeeds)
    TokenQueueResult force_pop(TokenT &token_out)
    {
        // lock and wait until the queue is not empty or the queue is shutting down
        std::unique_lock<std::mutex> lock{m_mutex};
        m_condvar.wait(lock, [this]() -> bool { return m_queue.size() > 0 || m_is_shutting_down; });
        if (m_queue.size() == 0 && m_is_shutting_down)
            return TokenQueueResult::SHUTTING_DOWN;
        else if (m_queue.size() == 0)
            return TokenQueueResult::QUEUE_EMPTY;

        // pop the bottom element
        token_out = std::move(m_queue.front());
        m_queue.pop_front();
        return TokenQueueResult::SUCCESS;
    }
    /// try to remove the minimum element
    TokenQueueResult try_remove_min(TokenT &token_out)
    {
        // try to lock the queue, then check if there are any elements
        std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
        if (!lock.owns_lock())
            return TokenQueueResult::TRY_LOCK_FAIL;
        if (m_queue.size() == 0)
            return TokenQueueResult::QUEUE_EMPTY;

        // find the min element
        auto min_elem = m_queue.begin();
        for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
        {
            if (*it < *min_elem)
                min_elem = it;
        }
        token_out = std::move(*min_elem);
        m_queue.erase(min_elem);
        return TokenQueueResult::SUCCESS;
    }
    /// shut down the queue
    void shut_down()
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_is_shutting_down = true;
        }
        m_condvar.notify_all();
    }
    /// reset the queue (queue must already be empty)
    TokenQueueResult reset()
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!m_queue.empty())
            return TokenQueueResult::QUEUE_NOT_EMPTY;
        m_is_shutting_down = false;
        return TokenQueueResult::SUCCESS;
    }

private:
//member variables
    /// queue context
    std::list<TokenT> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    bool m_is_shutting_down{false};
};

} //namespace async
