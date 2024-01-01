// Copyright (c) 2023-2024, The Monero Project
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

#include <boost/utility/string_ref.hpp>
#include <boost/range/size.hpp>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "byte_slice.h"
#include "serialization/wire/error.h"
#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"
#include "span.h"

/*
  Custom types (e.g type `type` in namespace `ns`) can define an output function by:
    * `namespace wire { template<> struct is_array<ns::type> : std::true_type {}; }`
    * `namespace wire { template<> struct is_blob<ns::type> : std::true_type {}; }`
    * `namespace wire { void write_bytes(writer&, const ns::type&); }`
    * `namespace ns { void write_bytes(wire::writer&, const type&); }`

  See `wrappers.h` for `is_array` requirements, and `traits.h` for `is_blob`
  requirements. `write_bytes` function can also specify derived type for faster
  output (i.e.  `namespace ns { void write_bytes(wire::epee_writer&, type&); }`).
  Using the derived type allows the compiler to de-virtualize and allows for
  custom functions not defined by base interface. Using base interface allows
  for multiple formats with minimal instruction count. */

namespace wire
{
  //! Interface for converting C/C++ objects to "wire" (byte) formats.
  struct writer
  {
    writer() = default;

    virtual ~writer() noexcept;

    //! By default, insist on retrieving array size before writing array
    static constexpr std::true_type need_array_size() noexcept { return{}; }

    virtual void boolean(bool) = 0;

    virtual void integer(std::intmax_t) = 0;
    virtual void unsigned_integer(std::uintmax_t) = 0;

    virtual void real(double) = 0;

    virtual void string(boost::string_ref) = 0;
    virtual void binary(epee::span<const std::uint8_t>) = 0;

    virtual void start_array(std::size_t) = 0;
    virtual void end_array() = 0;

    virtual void start_object(std::size_t) = 0;
    virtual void key(boost::string_ref) = 0;
    virtual void binary_key(epee::span<const std::uint8_t>) = 0;
    virtual void end_object() = 0;

  protected:
    writer(const writer&) = default;
    writer(writer&&) = default;
    writer& operator=(const writer&) = default;
    writer& operator=(writer&&) = default;
  };

  template<typename W>
  inline void write_arithmetic(W& dest, const bool source)
  { dest.boolean(source); }

  template<typename W>
  inline void write_arithmetic(W& dest, const int source)
  { dest.integer(source); }

  template<typename W>
  inline void write_arithmetic(W& dest, const long source)
  { dest.integer(std::intmax_t(source)); }

  template<typename W>
  inline void write_arithmetic(W& dest, const long long source)
  { dest.integer(std::intmax_t(source)); }

  template<typename W>
  inline void write_arithmetic(W& dest, const unsigned source)
  { dest.unsigned_integer(source); }

  template<typename W>
  inline void write_arithmetic(W& dest, const unsigned long source)
  { dest.unsigned_integer(std::uintmax_t(source)); }

  template<typename W>
  inline void write_arithmetic(W& dest, const unsigned long long source)
  { dest.unsigned_integer(std::uintmax_t(source));}

  template<typename W>
  inline void write_arithmetic(W& dest, const double source)
  { dest.real(source); }

  // Template both arguments to allow derived writer specializations
  template<typename W, typename T>
  inline std::enable_if_t<std::is_arithmetic<T>::value> write_bytes(W& dest, const T source)
  { write_arithmetic(dest, source); }

  template<typename W>
  inline void write_bytes(W& dest, const boost::string_ref source)
  { dest.string(source); }

  template<typename W, typename T>
  inline std::enable_if_t<is_blob<T>::value> write_bytes(W& dest, const T& source)
  { dest.binary(epee::as_byte_span(source)); }

  template<typename W>
  inline void write_bytes(W& dest, const epee::span<const std::uint8_t> source)
  { dest.binary(source); }

  template<typename W>
  inline void write_bytes(W& dest, const epee::byte_slice& source)
  { write_bytes(dest, epee::to_span(source)); }

  //! Use `write_bytes(...)` method if available for `T`.
  template<typename W, typename T>
  inline auto write_bytes(W& dest, const T& source) -> decltype(source.write_bytes(dest))
  { return source.write_bytes(dest); }
}

namespace wire_write
{
  /*! Don't add a function called `write_bytes` to this namespace, it will
      prevent ADL lookup. ADL lookup delays the function searching until the
      template is used instead of when its defined. This allows the unqualified
      calls to `write_bytes` in this namespace to "find" user functions that are
      declared after these functions. */

  template<typename W, typename T>
  inline void bytes(W& dest, const T& source)
  {
    write_bytes(dest, source); // ADL (searches every associated namespace)
  }

  //! Use writer `W` to convert `source` into bytes appended to `dest`.
  template<typename W, typename T, typename U>
  inline std::error_code to_bytes(T& dest, const U& source)
  {
    try
    {
      W out{std::move(dest)};
      bytes(out, source);
      dest = out.take_buffer();
    }
    catch (const wire::exception& e)
    {
      dest.clear();
      return e.code();
    }
    catch (...)
    {
      dest.clear();
      throw;
    }
    return {};
  }

  template<typename T>
  inline std::size_t array_size(std::true_type, const T& source)
  { return boost::size(source); }

  template<typename T>
  inline constexpr std::size_t array_size(std::false_type, const T&) noexcept
  { return 0; }

  template<typename W, typename T>
  inline void array(W& dest, const T& source)
  {
    using value_type = typename T::value_type;
    static_assert(!std::is_same<value_type, char>::value, "write array of chars as string");
    static_assert(!std::is_same<value_type, std::int8_t>::value, "write array of signed chars as binary");
    static_assert(!std::is_same<value_type, std::uint8_t>::value, "write array of unsigned chars as binary");

    dest.start_array(array_size(dest.need_array_size(), source));
    for (const auto& elem : source)
      bytes(dest, elem);
    dest.end_array();
  }

  template<typename W, typename T>
  inline bool field(W& dest, const wire::field_<T, true>& field)
  {
    // Arrays always optional, see `wire/field.h`
    if (wire::available(field))
    {
      dest.key(field.name);
      bytes(dest, field.get_value());
    }
    return true;
  }

  template<typename W, typename T>
  inline bool field(W& dest, const wire::field_<T, false>& field)
  {
    if (wire::available(field))
    {
      dest.key(field.name);
      bytes(dest, *field.get_value());
    }
    return true;
  }

  template<typename W, typename T>
  inline std::enable_if_t<std::is_pod<T>::value> dynamic_object_key(W& dest, const T& source)
  {
    dest.binary_key(epee::as_byte_span(source));
  }

  template<typename W>
  inline void dynamic_object_key(W& dest, const boost::string_ref source)
  {
    dest.key(source);
  }

  template<typename W, typename T>
  inline void dynamic_object(W& dest, const T& source)
  {
    dest.start_object(source.size());
    for (const auto& elem : source)
    {
      dynamic_object_key(dest, elem.first);
      bytes(dest, elem.second);
    }
    dest.end_object();
  }

  template<typename W, typename... T>
  inline void object(W& dest, T&&... fields)
  {
    dest.start_object(wire::sum(std::size_t(wire::available(fields))...));
    const bool dummy[] = {field(dest, std::forward<T>(fields))...};
    dest.end_object();
    (void)dummy; // expand into array to get 0,1,2,etc order
  }
} // wire_write

namespace wire
{
  template<typename W, typename T>
  inline std::enable_if_t<is_array<T>::value> write_bytes(W& dest, const T& source)
  {
    wire_write::array(dest, source);
  }

  template<typename W, typename... T>
  inline std::enable_if_t<std::is_base_of<writer, W>::value> object(W& dest, T... fields)
  {
    wire_write::object(dest, std::move(fields)...);
  }

  template<typename W, typename... T>
  inline void object_fwd(const std::false_type /* is_read */, W& dest, T... fields)
  {
    wire::object(dest, std::move(fields)...);
  }
}
