// Copyright (c) 2024, The Monero Project
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

/// mutex

#pragma once

//local headers
#include "misc_language.h"
#include "misc_log_ex.h"

//third-party headers

//standard headers
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

//forward declarations


// Lock the mutex and then unlock it at the end of the local scope
// - if it fails to unlock (either it's already unlocked or owned by a different thread), the exception is ignored
#define SCOPE_LOCK_MUTEX(mutex)                                                                \
    mutex.lock();                                                                              \
    auto scope_exit_handler_##mutex = epee::misc_utils::create_scope_leave_handler([this](){   \
        CHECK_AND_ASSERT_THROW_MES(mutex.unlock(), "failed to unlock " + std::string(#mutex)); \
    })

namespace async
{

/// mutex
class Mutex final
{
public:
    /// disable copy/move
    Mutex& operator=(Mutex&&) = delete;

//member functions
    /// Lock the mutex and claim ownership
    void lock()
    {
        m_mutex.lock();
        m_mutex_owner.store(std::this_thread::get_id(), std::memory_order_relaxed);
    }

    /// Release ownership and unlock the mutex. If this thread does not own the lock already, returns false.
    bool unlock()
    {
        if (!thread_owns_lock())
            return false;
        m_mutex_owner.store(std::thread::id{}, std::memory_order_relaxed);
        m_mutex.unlock();
        return true;
    }

    /// Check if the given thread owns the mutex
    bool thread_owns_lock(const std::thread::id thread_id = std::this_thread::get_id()) const
    {
        return thread_id == m_mutex_owner.load(std::memory_order_relaxed);
    }

private:
//member variables
    std::mutex m_mutex;
    std::atomic<std::thread::id> m_mutex_owner{std::thread::id{}};
};

} //namespace async
