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

/// Scoped notification: calls a function when destroyed.

#pragma once

//local headers

//third-party headers

//standard headers
#include <functional>

//forward declarations


namespace async
{

/// scoped notification (notifies on destruction)
/// - only use this if you can GUARANTEE the lifetimes of any references in the notification function are longer
///   than the notification's lifetime
class ScopedNotification final
{
public:
//constructors
    /// normal constructor
    ScopedNotification(std::function<void()> notification_func) :
        m_notification_func{std::move(notification_func)}
    {}

    /// disable copies (this is a scoped manager)
    ScopedNotification(const ScopedNotification&)            = delete;
    ScopedNotification& operator=(const ScopedNotification&) = delete;

    /// moved-from notifications need empty notification functions so they are not called in the destructor
    ScopedNotification(ScopedNotification &&other)
    {
        *this = std::move(other);
    }
    ScopedNotification& operator=(ScopedNotification &&other)
    {
        this->notify();
        this->m_notification_func = std::move(other).m_notification_func;
        other.m_notification_func = nullptr;  //nullify the moved-from function
        return *this;
    }

//destructor
    ~ScopedNotification()
    {
        this->notify();
    }

private:
//member functions
    void notify() noexcept
    {
        if (m_notification_func)
        {
            try { m_notification_func(); } catch (...) {}
        }
    }

//member variables
    std::function<void()> m_notification_func;
};

} //namespace async
