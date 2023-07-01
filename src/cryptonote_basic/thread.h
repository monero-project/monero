// Copyright (c) 2014-2022, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <utility>

#include <boost/thread/thread.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif  // _WIN32

namespace cryptonote
{

// Creates and returns a thread running at an idle priority (if possible).
template<typename Callable>
boost::thread create_background_thread(boost::thread::attributes& attrs, Callable func);

#ifdef _WIN32

template<typename Callable>
boost::thread create_background_thread(boost::thread::attributes& attrs, Callable func) {
    return boost::thread(attrs, [func = std::move(func)]() mutable {
        auto handle = GetCurrentThread();
        if (!SetThreadPriority(handle, THREAD_MODE_BACKGROUND_BEGIN)) {
            if (!SetThreadPriority(handle, THREAD_PRIORITY_LOWEST)) {
                MWARNING("Can't set a background thread priority: "
                     << std::error_code(GetLastError(), std::system_category()));

            }
        }
        std::move(func)();
    });
}

#else

template<typename Callable>
boost::thread create_background_thread(boost::thread::attributes& attrs, Callable func) {
    auto thread = boost::thread(attrs, std::move(func));
    auto handle = thread.native_handle();
    int policy;
    struct sched_param param;
    int error;
    if ((error = pthread_getschedparam(handle, &policy, &param)) != 0) {
        MWARNING("Can't set a background thread priority: pthread_getschedparam "
                 "failed: " << std::strerror(error));
        return thread;
    }
#ifdef SCHED_IDLE
    policy = SCHED_IDLE;
    // In particular MacOS doesn't support `SCHED_IDLE`. In the future we might
    // consider using https://developer.apple.com/documentation/dispatch
    // instead.
#else
#warning SCHED_IDLE policy not available, falling back to minimum priority
    param.sched_priority = sched_get_priority_min(policy);
#endif
    if ((error = pthread_setschedparam(handle, policy, &param)) != 0) {
        MWARNING("Can't set a background thread priority: pthread_setschedparam "
                 "to SCHED_IDLE failed: " << std::strerror(error));
    }
    return thread;
}

#endif

}  // namespace cryptonote
