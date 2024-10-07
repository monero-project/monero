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


#pragma once

#include <atomic>
#include <cstdint>
#include <limits>
#include <unordered_map>

#include <boost/thread/shared_mutex.hpp>

namespace tools
{
// Implements Lockable & SharedLockable C++14 concepts. Allows for recursion of all calls when the
// first acquisition was exclusive, and recursion of shared access calls whens first acquisition was
// shared. Attempting to acquire exclusive ownership of the mutex when this thread already has
// shared ownership is undefined behavior. Maximum recursion depth is 2^31. Maxium number of
// instances per program is 2^32. This implementation only requires one boost::shared_mutex and a
// per-thread map of access counters, and so will be relatively performant for most use cases.
// Because a boost::shared_mutex is used internally, access to this lock is guaranteed to be fair.
class recursive_shared_mutex
{
public:
    recursive_shared_mutex();

    // delete copy/move construct/assign operations
    recursive_shared_mutex(const recursive_shared_mutex&)            = delete;
    recursive_shared_mutex(recursive_shared_mutex&&)                 = delete;
    recursive_shared_mutex& operator=(const recursive_shared_mutex&) = delete;
    recursive_shared_mutex& operator=(recursive_shared_mutex&&)      = delete;

    // Lockable
    void lock();
    bool try_lock();
    void unlock();

    // SharedLockable
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

private:
    using slot_t = std::uint32_t;
    using access_counter_t = std::uint32_t;

    static constexpr access_counter_t access_counter_max = std::numeric_limits<access_counter_t>::max();
    static constexpr access_counter_t depth_mask = access_counter_max >> 1;
    static constexpr access_counter_t write_bit = access_counter_max - depth_mask;

    // what the slot id will be for the next instance of recursive_shared_mutex
    static std::atomic<slot_t> shared_slot_counter;

    // keeps track of read/write depth and write mode per instance of recursive_shared_mutex
    static thread_local std::unordered_map<slot_t, access_counter_t> tlocal_access_per_mutex;

    boost::shared_mutex m_rw_mutex; // we use boost::shared_mutex since it is guaranteed fair, unlike std::shared_mutex
    const slot_t m_slot; // this is an ID number used per-thread to identify whether already held (enables recursion)
};
} // namespace tools
