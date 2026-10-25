// Copyright (c) 2022, The Monero Project
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

#include <boost/variant/get.hpp>
#include <boost/variant/variant_fwd.hpp>
#include <functional>
#include <utility>

#include "serialization/wire/error.h"
#include "serialization/wire/fwd.h"
#include "serialization/wire/field.h"

#define WIRE_OPTION(name, type, cpp_name)                               \
  wire::optional_field(name, wire::option<type>(std::ref(cpp_name)))

namespace wire
{
  [[noreturn]] void throw_variant_exception(error::schema type, const char* variant_name);

  /*! Wrapper for any C++ variant type that tracks if a `read_bytes` call has
    completed on wrapped `value`. This wrapper is not needed if the variant is
    being used for writes only - see `wire::option_` below for more information.

    `variant_type` is `T` with optional `std::reference_wrapper` removed. See
    `option_` for concept requirements of `variant_type`.

    Example usage:
    ```
    template<typename F, typename T>
    void type_map(F& format, T& self)
    {
      auto variant = wire::variant(std::ref(self.field3));
      wire::object(format,
        ...
        WIRE_OPTION("type1", type1, variant),
        WIRE_OPTION("type2", type2, variant)
      );
    }
    ``` */
  template<typename T>
  struct variant_
  {
    using variant_type = unwrap_reference_t<T>;

    //! \throw wire::exception with `type` and mangled C++ name of `variant_type`.
    [[noreturn]] static void throw_exception(const error::schema type)
    { throw_variant_exception(type, typeid(variant_type).name()); }

    constexpr variant_(T&& value)
      : value(std::move(value)), read(false)
    {}

    T value;
    bool read;

    constexpr const variant_type& get_variant() const noexcept { return value; }
    variant_type& get_variant() noexcept { return value; }

    //! Makes `variant_` compatible with `emplace()` in `option_`.
    template<typename U>
    variant_& operator=(U&& rhs)
    {
      get_variant() = std::forward<U>(rhs);
      return *this;
    }
  };

  template<typename T>
  inline constexpr variant_<T> variant(T value)
  { return {std::move(value)}; }

  namespace adapt
  {
    template<typename T>
    inline void throw_if_not_read(const T&)
    { throw_variant_exception(error::schema::missing_key, typeid(T).name()); }

    template<typename T>
    inline void throw_if_not_read(const variant_<T>& value)
    {
      if (!value.read)
        value.throw_exception(error::schema::missing_key);
    }


    // other variant overloads can be added here as needed

    template<typename U, typename... T>
    inline const U* get_if(const boost::variant<T...>& value)
    { return boost::get<U>(std::addressof(value)); }

    template<typename U, typename T>
    inline const U* get_if(const variant_<T>& value)
    { return adapt::get_if<U>(value.get_variant()); }
  }

  /*! Wrapper that makes a variant compatible with `wire::optional_field`.
    Currently `wire::variant_` and `boost::variant` are valid variant types
    for writing, and only `wire::variant_` is valid for reading.

   `variant_type` is `T` with optional `std::reference_wrapper` removed.
   `variant_type` concept requirements:
     * must have two overloads for `get<U>` function in `adapt` namespace - one
       `const` and one non-`const` that returns `const U&` and `U&` respectively
       iff `variant_type` is storing type `U`. Otherwise, the function should
       throw an exception.
     * must have overload for `get_if<U>` function in `adapt` namespace that
       returns `const U*` when `variant_type` is storing type `U`. Otherwise, the
       function should return `nullptr`.
     * must have a member function `operator=(U&&)` that changes the stored type
       to `U` (`get<U>` and `get_if<U>` will return `U` after `operator=`
       completion).

   The `wire::variant(std::ref(self.field3))` step in the example above can be
   omitted if only writing is needed. The `boost::variant` value should be
   given directly to `wire::option<U>(...)` or `WIRE_OPTION` macro - only one
   type is active so `wire::optional_field` will omit all other types/fields. */
  template<typename T, typename U>
  struct option_
  {
    using variant_type = unwrap_reference_t<T>;
    using option_type = U;

    T value;

    constexpr const variant_type& get_variant() const noexcept { return value; }
    variant_type& get_variant() noexcept { return value; }

    //! \return `true` iff `U` is active type in variant.
    bool is_active() const { return adapt::get_if<U>(get_variant()) != nullptr; }


    // concept requirements for optional fields

    explicit operator bool() const { return is_active(); }
    void emplace() { get_variant() = U{}; }

    const option_& operator*() const { return *this; }
    option_& operator*() { return *this; }

    //! \throw wire::exception iff no variant type was read.
    void reset() { adapt::throw_if_not_read(get_variant()); }
  };

  template<typename U, typename T>
  inline constexpr option_<T, U> option(T value)
  { return {std::move(value)}; }

  namespace adapt
  {
    // other variant overloads can be added here as needed

    template<typename U, typename... T>
    inline U& get(boost::variant<T...>& value)
    { return boost::get<U>(value); }

    template<typename U, typename... T>
    inline const U& get(const boost::variant<T...>& value)
    { return boost::get<U>(value); }

    template<typename U, typename T>
    inline const U& get(const variant_<T>& value)
    { return adapt::get<U>(value.get_variant()); }
  }

  //! \throw wire::exception if `dest.get_variant()` has already been used in `read_bytes`.
  template<typename R, typename T, typename U>
  inline void read_bytes(R& source, option_<std::reference_wrapper<variant_<T>>, U> dest)
  {
    if (dest.get_variant().read)
      dest.get_variant().throw_exception(error::schema::invalid_key);
    wire_read::bytes(source, adapt::get<U>(dest.get_variant().get_variant()));
    dest.get_variant().read = true;
  }

  template<typename W, typename T, typename U>
  inline void write_bytes(W& dest, const option_<T, U>& source)
  { wire_write::bytes(dest, adapt::get<U>(source.get_variant())); }
}
