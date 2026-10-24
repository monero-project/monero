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

/// Single-writer/multi-reader value containers.
/// - Accessing a moved-from container will throw.
/// - We do not guarantee that accessing the same container from multiple threads is not UB.
/// - The containers use a shared_ptr internally, so misuse WILL cause reference cycles.
/// 
/// Implementation notes:
/// - We use a shared_mutex for the context mutex since after a write_lock is released multiple waiting readers may
///   concurrently acquire a shared lock on the value.

#pragma once

//local headers

//third-party headers
#include <boost/optional/optional.hpp>
#include <boost/thread/shared_mutex.hpp>

//standard headers
#include <atomic>
#include <condition_variable>
#include <memory>
#include <stdexcept>
#include <type_traits>

//forward declarations


namespace async
{

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

namespace detail
{

/// enable if nonconst
template <typename, typename = void>
struct enable_if_nonconst;
template <typename T>
struct enable_if_nonconst<T, std::enable_if_t<!std::is_const<T>::value>> {};
template <typename T>
struct enable_if_nonconst<T, std::enable_if_t<std::is_const<T>::value>> final { enable_if_nonconst() = delete; };

/// test a rw_lock pointer
[[noreturn]] inline void rw_lock_ptr_access_error() { throw std::runtime_error{"rw_lock invalid ptr access."}; }
inline void test_rw_ptr(const void *ptr) { if (ptr == nullptr) rw_lock_ptr_access_error(); }

/// value context
template <typename value_t>
struct rw_context final
{
    value_t value;
    boost::shared_mutex value_mutex;
    boost::shared_mutex ctx_mutex;
    std::condition_variable_any ctx_condvar;
    std::atomic<int> num_readers{0};
};

} //namespace detail

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// declarations
template <typename>
class read_lock;
template <typename>
class write_lock;
template <typename>
class readable;
template <typename>
class writable;

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// READ LOCK (can read the locked value concurrently with other read_locks)
template <typename value_t>
class read_lock final : public detail::enable_if_nonconst<value_t>
{
    friend class readable<value_t>;
    using ctx_t = detail::rw_context<value_t>;

protected:
//constructors
    /// default constructor: disabled
    read_lock() = delete;
    /// normal constructor: only callable by readable and writable
    read_lock(boost::shared_lock<boost::shared_mutex> lock, std::shared_ptr<ctx_t> context) :
        m_lock{std::move(lock)},
        m_context{std::move(context)}
    {}
    /// copies: disabled
    read_lock(const read_lock<value_t>&)            = delete;
    read_lock& operator=(const read_lock<value_t>&) = delete;

public:
    /// moves: default
    read_lock(read_lock<value_t>&&)            = default;
    read_lock& operator=(read_lock<value_t>&&) = default;

//destructor
    ~read_lock()
    {
        if (m_context &&
            m_lock.mutex() != nullptr &&
            m_lock.owns_lock())
        {
            {
                boost::shared_lock<boost::shared_mutex> ctx_lock{m_context->ctx_mutex};
                m_lock.unlock();
            }

            // if there seem to be no existing readers, notify one waiting writer
            // NOTE: this is only an optimization
            // - there is a race condition where a new reader is being added/gets added concurrently and the notified
            //   writer ends up failing to get a lock
            // - there is also a race conditon where a writer is added after our .unlock() call above, then a reader gets
            //   stuck on ctx_condvar, then our notify_one() call here causes that reader to needlessly try to get a lock
            if (m_context->num_readers.fetch_sub(1, std::memory_order_relaxed) <= 0)
                m_context->ctx_condvar.notify_one();  //notify one waiting writer
        }
    }

//member functions
    /// access the value
    const value_t& value() const { detail::test_rw_ptr(m_context.get()); return m_context->value; }

private:
//member variables
    boost::shared_lock<boost::shared_mutex> m_lock;
    std::shared_ptr<ctx_t> m_context;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// WRITE LOCK (can mutate the locked value)
template <typename value_t>
class write_lock final : public detail::enable_if_nonconst<value_t>
{
    friend class writable<value_t>;
    using ctx_t = detail::rw_context<value_t>;

protected:
//constructors
    /// default constructor: disabled
    write_lock() = delete;
    /// normal constructor: only callable by writable
    write_lock(boost::unique_lock<boost::shared_mutex> lock, std::shared_ptr<ctx_t> context) :
        m_lock{std::move(lock)},
        m_context{std::move(context)}
    {}
    /// copies: disabled
    write_lock(const write_lock<value_t>&)            = delete;
    write_lock& operator=(const write_lock<value_t>&) = delete;

public:
    /// moves: default
    write_lock(write_lock<value_t>&&)            = default;
    write_lock& operator=(write_lock<value_t>&&) = default;

//destructor
    ~write_lock()
    {
        if (m_context &&
            m_lock.mutex() != nullptr &&
            m_lock.owns_lock())
        {
            {
                boost::unique_lock<boost::shared_mutex> ctx_lock{m_context->ctx_mutex};
                m_lock.unlock();
            }
            m_context->ctx_condvar.notify_all();  //notify all waiting
        }
    }

//member functions
    /// access the value
    value_t& value() { detail::test_rw_ptr(m_context.get()); return m_context->value; }

private:
//member variables
    boost::unique_lock<boost::shared_mutex> m_lock;
    std::shared_ptr<ctx_t> m_context;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// READ LOCKABLE (can be copied and spawn read_locks)
template <typename value_t>
class readable final : public detail::enable_if_nonconst<value_t>
{
    friend class writable<value_t>;
    using ctx_t = detail::rw_context<value_t>;

protected:
//constructors
    /// default constructor: disabled
    readable() = delete;
    /// normal constructor: only callable by writable
    readable(std::shared_ptr<ctx_t> context) :
        m_context{std::move(context)}
    {}

public:
    /// normal constructor: from value
    readable(const value_t &raw_value) :
        m_context{std::make_shared<ctx_t>()}
    {
        m_context->value = raw_value;
    }
    readable(value_t &&raw_value) :
        m_context{std::make_shared<ctx_t>()}
    {
        m_context->value = std::move(raw_value);
    }

    /// moves and copies: default

//member functions
    /// try to get a write lock
    /// FAILS IF THERE IS A CONCURRENT WRITE LOCK
    boost::optional<read_lock<value_t>> try_lock()
    {
        detail::test_rw_ptr(m_context.get());
        boost::shared_lock<boost::shared_mutex> lock{m_context->value_mutex, boost::try_to_lock};
        if (!lock.owns_lock()) return boost::none;
        else
        {
            m_context->num_readers.fetch_add(1, std::memory_order_relaxed);
            return read_lock<value_t>{std::move(lock), m_context};
        }
    }

    /// get a read lock
    /// BLOCKS IF THERE IS A CONCURRENT WRITE LOCK
    read_lock<value_t> lock()
    {
        // cheap attempt
        boost::optional<read_lock<value_t>> lock;
        if ((lock = this->try_lock()))
            return std::move(*lock);

        // blocking attempt
        detail::test_rw_ptr(m_context.get());
        boost::shared_lock<boost::shared_mutex> ctx_lock{m_context->ctx_mutex};
        m_context->ctx_condvar.wait(ctx_lock,
                [&]() -> bool
                {
                    return (lock = this->try_lock()) != boost::none;
                }
            );

        return std::move(*lock);
    }

private:
//member variables
    std::shared_ptr<ctx_t> m_context;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

/// WRITE LOCKABLE (can spawn readables and write_locks)
template <typename value_t>
class writable final : public detail::enable_if_nonconst<value_t>
{
    using ctx_t = detail::rw_context<value_t>;

public:
//constructors
    /// default constructor: disabled
    writable() = delete;
    /// normal constructor: from value
    writable(const value_t &raw_value) :
        m_context{std::make_shared<ctx_t>()}
    {
        m_context->value = raw_value;
    }
    writable(value_t &&raw_value) :
        m_context{std::make_shared<ctx_t>()}
    {
        m_context->value = std::move(raw_value);
    }

    /// copies: disabled
    writable(const writable<value_t>&)            = delete;
    writable& operator=(const writable<value_t>&) = delete;
    /// moves: default
    writable(writable<value_t>&&)            = default;
    writable& operator=(writable<value_t>&&) = default;

//member functions
    /// get a readable
    readable<value_t> get_readable()
    {
        detail::test_rw_ptr(m_context.get());
        return readable<value_t>{m_context};
    }

    /// try to get a write lock
    /// FAILS IF THERE ARE ANY CONCURRENT WRITE OR READ LOCKS
    boost::optional<write_lock<value_t>> try_lock()
    {
        detail::test_rw_ptr(m_context.get());
        boost::unique_lock<boost::shared_mutex> lock{m_context->value_mutex, boost::try_to_lock};
        if (!lock.owns_lock()) return boost::none;
        else                   return write_lock<value_t>{std::move(lock), m_context};
    }

    /// get a write lock
    /// BLOCKS IF THERE ARE ANY CONCURRENT WRITE OR READ LOCKS
    write_lock<value_t> lock()
    {
        // cheap attempt
        boost::optional<write_lock<value_t>> lock;
        if ((lock = this->try_lock()))
            return std::move(*lock);

        // blocking attempt
        detail::test_rw_ptr(m_context.get());
        boost::unique_lock<boost::shared_mutex> ctx_lock{m_context->ctx_mutex};
        m_context->ctx_condvar.wait(ctx_lock,
                [&]() -> bool
                {
                    return (lock = this->try_lock()) != boost::none;
                }
            );

        return std::move(*lock);
    }

private:
//member variables
    std::shared_ptr<ctx_t> m_context;
};

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

} //namespace async
