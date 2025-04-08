// Copyright (c) 2025, The Monero Project
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

#include <utility>

namespace epee
{

/**
 * @brief: scope_guard - RAII pattern to call action on scope leave
 */
template <class F>
class scope_guard final
{
public:
    // no copy
    scope_guard(const scope_guard&) = delete;
    scope_guard& operator=(const scope_guard&) = delete;

    // constructor with CTAD
    scope_guard(F &&f): m_f(std::forward<F>(f)) {}

    // call action
    ~scope_guard() noexcept { try { m_f(); } catch (...) { /* ignore exception in destructor */ } }

private:
    F m_f;
};

/**
 * @brief: unique_scope_guard - RAII pattern to call action on scope leave and/or at explicit call
 */
template <class F>
class unique_scope_guard final
{
public:
    // no copy
    unique_scope_guard(const unique_scope_guard&) = delete;
    unique_scope_guard& operator=(const unique_scope_guard&) = delete;

    // move resets current state and releases other instance
    unique_scope_guard(unique_scope_guard &&other): m_f(std::move(other.m_f)), m_do(other.m_do)
    {
        other.release();
    }
    unique_scope_guard& operator=(unique_scope_guard &&other)
    {
        if (this != &other)
        {
            reset();
            m_f = std::move(other.m_f);
            m_do = other.m_do;
            other.release();
        }
        return *this;
    }

    // constructor with CTAD
    unique_scope_guard(F &&f): m_f(std::forward<F>(f)), m_do(true) {}

    void reset() { if (m_do) { m_do = false; m_f(); } }
    void release() { m_do = false; }

    // call action
    ~unique_scope_guard() noexcept { try { reset(); } catch (...) { /* ignore exception in destructor */ } }

private:
    F m_f;
    bool m_do;
};

} //namespace epee
