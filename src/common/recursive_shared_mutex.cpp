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

#include "recursive_shared_mutex.h"

#include <cassert>

namespace tools
{
// Initialize static member shared_slot_counter
std::atomic<recursive_shared_mutex::slot_t> recursive_shared_mutex::shared_slot_counter{0};

// Initialize static member tlocal_access_per_mutex
thread_local
    std::unordered_map<recursive_shared_mutex::slot_t, recursive_shared_mutex::access_counter_t>
        recursive_shared_mutex::tlocal_access_per_mutex{};


recursive_shared_mutex::recursive_shared_mutex(): m_rw_mutex{}, m_slot{shared_slot_counter++}
{}

void recursive_shared_mutex::lock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(0 == access || access & write_bit); // cannot upgrade from read->write

    if (!access)
        m_rw_mutex.lock();

    access = (access + 1) | write_bit;
}

bool recursive_shared_mutex::try_lock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(0 == access || access & write_bit); // cannot upgrade from read->write

    if (access || m_rw_mutex.try_lock())
    {
        access = (access + 1) | write_bit;
        return true;
    }

    if (!access)
        tlocal_access_per_mutex.erase(m_slot);

    return false;
}

void recursive_shared_mutex::unlock()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(access & depth_mask); // write depth must be non zero

    const bool still_held = --access & depth_mask;
    if (!still_held)
    {
        m_rw_mutex.unlock();
        tlocal_access_per_mutex.erase(m_slot);
    }
}

void recursive_shared_mutex::lock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];

    if (!(access++))
        m_rw_mutex.lock_shared();
}

bool recursive_shared_mutex::try_lock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];

    if (access || m_rw_mutex.try_lock_shared())
    {
        ++access;
        return true;
    }

    if (!access)
        tlocal_access_per_mutex.erase(m_slot);

    return false;
}

void recursive_shared_mutex::unlock_shared()
{
    access_counter_t &access = tlocal_access_per_mutex[m_slot];
    assert(access & depth_mask); // read depth must be non zero

    const bool still_held = --access & depth_mask;
    if (!still_held)
    {
        m_rw_mutex.unlock_shared();
        tlocal_access_per_mutex.erase(m_slot);
    }
}

} // namespace tools
