// Copyright (c) 2018, The Monero Project
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

#include <cassert>
#include <system_error>
#include <type_traits>
#include <utility>

#include "common/error.h"

//! If precondition fails, return `::error::kInvalidArgument` in current scope.
#define MONERO_PRECOND(...)                            \
    do                                                 \
    {                                                  \
        if (!( __VA_ARGS__ ))                          \
            return {::common_error::kInvalidArgument}; \
    } while (0)

//! Check `expect<void>` and return errors in current scope.
#define MONERO_CHECK(...)                           \
    do                                              \
    {                                               \
        const ::expect<void> result = __VA_ARGS__ ; \
        if (!result)                                \
            return result.error();                  \
    } while (0)

/*! Get `T` from `expect<T>` by `std::move` as-if by function call.
    `expect<void>` returns nothing.

    \throw std::system_error with `expect<T>::error()`, filename and line
        number when `expect<T>::has_error() == true`.*/
#define MONERO_UNWRAP(...)                                        \
    ::detail::expect::unwrap( __VA_ARGS__ , nullptr, __FILE__ , __LINE__ )

/* \throw std::system_error with `code` and `msg` as part of the details. The
filename and line number will automatically be injected into the explanation
string. `code` can be any enum convertible to `std::error_code`. */
#define MONERO_THROW(code, msg) \
    ::detail::expect::throw_( code , msg , __FILE__ , __LINE__ )


template<typename> class expect;

namespace detail
{
    // Shortens the characters in the places that `enable_if` is used below.
    template<bool C>
    using enable_if = typename std::enable_if<C>::type;
 
    struct expect
    {
        //! \throw std::system_error with `ec`, optional `msg` and/or optional `file` + `line`.
        static void throw_(std::error_code ec, const char* msg, const char* file, unsigned line);

        //! If `result.has_error()` call `throw_`. Otherwise, \return `*result` by move.
        template<typename T>
        static T unwrap(::expect<T>&& result, const char* error_msg, const char* file, unsigned line)
        {
            if (!result)
                throw_(result.error(), error_msg, file, line);
            return std::move(*result);
        }

        //! If `result.has_error()` call `throw_`.
        static void unwrap(::expect<void>&& result, const char* error_msg, const char* file, unsigned line);
    };
}

/*!
    `expect<T>` is a value or error implementation, similar to Rust std::result
    or various C++ proposals (boost::expected, boost::outcome). This
    implementation currently has a strict error type, `std::error_code`, and a
    templated value type `T`. `expect<T>` is implicitly convertible from `T`
    or `std::error_code`, and one `expect<T>` object type is implicitly
    convertible to another `expect<U>` object iff the destination value type
    can be implicitly constructed from the source value type (i.e.
    `struct U { ... U(T src) { ...} ... };`).

    `operator==` and `operator!=` are the only comparison operators provided;
    comparison between different value types is allowed provided the two values
    types have a `operator==` defined between them (i.e.
    `assert(expect<int>{100} == expect<short>{100});`). Comparisons can also be
    done against `std::error_code` objects or error code enums directly (i.e.
    `assert(expect<int>{make_error_code(common_error::kInvalidArgument)} == error::kInvalidArgument)`).
    Comparison of default constructed `std::error_code` will always fail.
    "Generic" comparisons can be done with `std::error_condition` via the `matches`
    method only (i.e.
    `assert(expect<int>{make_error_code{common_error::kInvalidErrorCode}.matches(std::errc::invalid_argument))`),
    `operator==` and `operator!=` will not work with `std::errc` or
    `std::error_condition`. A comparison with `matches` is more expensive
    because an equivalency between error categories is computed, but is
    recommended when an error can be one of several categories (this is going
    to be the case in nearly every situation when calling a function from
    another C++ struct/class).

    `expect<void>` is a special case with no stored value. It is used by
    functions that can fail, but otherwise would return `void`. It is useful
    for consistency; all macros, standalone functions, and comparison operators
    work with `expect<void>`.

    \note See `src/common/error.h` for creating a custom error enum.
 */
template<typename T>
class expect
{
    static_assert(std::is_nothrow_destructible<T>(), "T must have a nothrow destructor");

    template<typename U>
    static constexpr bool is_convertible() noexcept
    {
        return std::is_constructible<T, U>() &&
            std::is_convertible<U, T>();
    }

    // MEMBERS
    std::error_code code_;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    // MEMBERS

    T& get() noexcept
    {
        assert(has_value());
        return *reinterpret_cast<T*>(std::addressof(storage_));
    }

    T const& get() const noexcept
    {
        assert(has_value());
        return *reinterpret_cast<T const*>(std::addressof(storage_));
    }

    template<typename U>
    void store(U&& value) noexcept(std::is_nothrow_constructible<T, U>())
    {
        new (std::addressof(storage_)) T{std::forward<U>(value)};
        code_ = std::error_code{};
    }

    void maybe_throw() const
    {
        if (has_error())
            ::detail::expect::throw_(error(), nullptr, nullptr, 0);
    }

public:
    using value_type = T;
    using error_type = std::error_code;

    expect() = delete;

    /*! Store an error, `code`, in the `expect` object. If `code` creates a
    `std::error_code` object whose `.value() == 0`, then `error()` will be set
    to `::common_error::kInvalidErrorCode`. */
    expect(std::error_code const& code) noexcept
      : code_(code), storage_()
    {
        if (!has_error())
            code_ = ::common_error::kInvalidErrorCode;
    }

    //! Store a value, `val`, in the `expect` object.
    expect(T val) noexcept(std::is_nothrow_move_constructible<T>())
      : code_(), storage_()
    {
        store(std::move(val));
    }

    expect(expect const& src) noexcept(std::is_nothrow_copy_constructible<T>())
      : code_(src.error()), storage_()
    {
        if (src.has_value())
            store(src.get());
    }

    //! Copy conversion from `U` to `T`.
    template<typename U, typename = detail::enable_if<is_convertible<U const&>()>>
    expect(expect<U> const& src) noexcept(std::is_nothrow_constructible<T, U const&>())
      : code_(src.error()), storage_()
    {
        if (src.has_value())
            store(*src);
    }

    expect(expect&& src) noexcept(std::is_nothrow_move_constructible<T>())
      : code_(src.error()), storage_()
    {
        if (src.has_value())
            store(std::move(src.get()));
    }

    //! Move conversion from `U` to `T`.
    template<typename U, typename = detail::enable_if<is_convertible<U>()>>
    expect(expect<U>&& src) noexcept(std::is_nothrow_constructible<T, U>())
      : code_(src.error()), storage_()
    {
        if (src.has_value())
            store(std::move(*src));
    }

    ~expect() noexcept
    {
        if (has_value())
            get().~T();
    }

    expect& operator=(expect const& src) noexcept(std::is_nothrow_copy_constructible<T>() && std::is_nothrow_copy_assignable<T>())
    {
        if (this != std::addressof(src))
        {
            if (has_value() && src.has_value())
                get() = src.get();
            else if (has_value())
                get().~T();
            else if (src.has_value())
                store(src.get());
            code_ = src.error();
        }
        return *this;
    }

    /*! Move `src` into `this`. If `src.has_value() && addressof(src) != this`
    then `src.value() will be in a "moved from state". */
    expect& operator=(expect&& src) noexcept(std::is_nothrow_move_constructible<T>() && std::is_nothrow_move_assignable<T>())
    {
        if (this != std::addressof(src))
        {
            if (has_value() && src.has_value())
                get() = std::move(src.get());
            else if (has_value())
                get().~T();
            else if (src.has_value())
                store(std::move(src.get()));
            code_ = src.error();
        }
        return *this;
    }

    //! \return True if `this` is storing a value instead of an error.
    explicit operator bool() const noexcept { return has_value(); }

    //! \return True if `this` is storing an error instead of a value.
    bool has_error() const noexcept { return bool(code_); }

    //! \return True if `this` is storing a value instead of an error.
    bool has_value() const noexcept { return !has_error(); }

    //! \return Error - always safe to call. Empty when `!has_error()`.
    std::error_code error() const noexcept { return code_; }

    //! \return Value if `has_value()` otherwise \throw `std::system_error{error()}`.
    T& value() &
    {
        maybe_throw();
        return get();
    }

    //! \return Value if `has_value()` otherwise \throw `std::system_error{error()}`.
    T const& value() const &
    {
        maybe_throw();
        return get();
    }

    /*! Same as other overloads, but expressions such as `foo(bar().value())`
    will automatically perform moves with no copies. */
    T&& value() &&
    {
        maybe_throw();
        return std::move(get());
    }

    //! \return Value, \pre `has_value()`.
    T* operator->() noexcept { return std::addressof(get()); }
    //! \return Value, \pre `has_value()`.
    T const* operator->() const noexcept { return std::addressof(get()); }
    //! \return Value, \pre `has_value()`.
    T& operator*() noexcept { return get(); }
    //! \return Value, \pre `has_value()`.
    T const& operator*() const noexcept { return get(); }

    /*! 
        \note This function is `noexcept` when `U == T` is `noexcept`.
        \return True if `has_value() == rhs.has_value()` and if values or errors are equal.
    */
    template<typename U>
    bool equal(expect<U> const& rhs) const noexcept(noexcept(*std::declval<expect<T>>() == *rhs))
    {
        return has_value() && rhs.has_value() ?
            get() == *rhs : error() == rhs.error();
    }

    //! \return False if `has_value()`, otherwise `error() == rhs`.
    bool equal(std::error_code const& rhs) const noexcept
    {
        return has_error() && error() == rhs;
    }

    /*!
        \note This function is `noexcept` when `U == T` is `noexcept`.
        \return False if `has_error()`, otherwise `value() == rhs`.
    */
    template<typename U, typename = detail::enable_if<!std::is_constructible<std::error_code, U>::value>>
    bool equal(U const& rhs) const noexcept(noexcept(*std::declval<expect<T>>() == rhs))
    {
        return has_value() && get() == rhs;
    }

    //! \return False if `has_value()`, otherwise `error() == rhs`.
    bool matches(std::error_condition const& rhs) const noexcept
    {
        return has_error() && error() == rhs;
    }
};

template<>
class expect<void>
{
    std::error_code code_;

public:
    using value_type = void;
    using error_type = std::error_code;

    //! Create a successful object.
    expect() noexcept
      : code_()
    {}

    expect(std::error_code const& code) noexcept
      : code_(code)
    {
        if (!has_error())
            code_ = ::common_error::kInvalidErrorCode;
    }

    expect(expect const&) = default;
    ~expect() = default;
    expect& operator=(expect const&) = default;

    //! \return True if `this` is storing a value instead of an error.
    explicit operator bool() const noexcept { return !has_error(); }

    //! \return True if `this` is storing an error instead of a value.
    bool has_error() const noexcept { return bool(code_); }

    //! \return Error - alway
    std::error_code error() const noexcept { return code_; }

    //! \return `error() == rhs.error()`.
    bool equal(expect const& rhs) const noexcept
    {
        return error() == rhs.error();
    }

    //! \return `has_error() && error() == rhs`.
    bool equal(std::error_code const& rhs) const noexcept
    {
        return has_error() && error() == rhs;
    }

    //! \return False if `has_value()`, otherwise `error() == rhs`.
    bool matches(std::error_condition const& rhs) const noexcept
    {
        return has_error() && error() == rhs;
    }
};

//! \return An `expect<void>` object with `!has_error()`.
inline expect<void> success() noexcept { return expect<void>{}; }

template<typename T, typename U>
inline
bool operator==(expect<T> const& lhs, expect<U> const& rhs) noexcept(noexcept(lhs.equal(rhs)))
{
    return lhs.equal(rhs);
}

template<typename T, typename U>
inline
bool operator==(expect<T> const& lhs, U const& rhs) noexcept(noexcept(lhs.equal(rhs)))
{
    return lhs.equal(rhs);
}

template<typename T, typename U>
inline
bool operator==(T const& lhs, expect<U> const& rhs) noexcept(noexcept(rhs.equal(lhs)))
{
    return rhs.equal(lhs);
}

template<typename T, typename U>
inline
bool operator!=(expect<T> const& lhs, expect<U> const& rhs) noexcept(noexcept(lhs.equal(rhs)))
{
    return !lhs.equal(rhs);
}

template<typename T, typename U>
inline
bool operator!=(expect<T> const& lhs, U const& rhs) noexcept(noexcept(lhs.equal(rhs)))
{
    return !lhs.equal(rhs);
}

template<typename T, typename U>
inline
bool operator!=(T const& lhs, expect<U> const& rhs) noexcept(noexcept(rhs.equal(lhs)))
{
    return !rhs.equal(lhs);
}

namespace detail
{
    inline void expect::unwrap(::expect<void>&& result, const char* error_msg, const char* file, unsigned line)
    {
        if (!result)
            throw_(result.error(), error_msg, file, line);
    }
}

