// Copyright (c) 2021-2024, The Monero Project
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

#include <cstdint>
#include <type_traits>

#define WIRE_DECLARE_BLOB_NS(type) \
  template<>                       \
  struct is_blob<type>             \
    : std::true_type               \
  {}

#define WIRE_DECLARE_BLOB(type)                  \
  namespace wire { WIRE_DECLARE_BLOB_NS(type); }

#define WIRE_DECLARE_OPTIONAL_ROOT(type) \
  template<>                             \
  struct is_optional_root<type>          \
    : std::true_type                     \
  {}

namespace wire
{
  template<typename T>
  struct unwrap_reference
  {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
  };

  template<typename T>
  struct unwrap_reference<std::reference_wrapper<T>>
    : std::remove_cv<T>
  {};

  template<typename T>
  using unwrap_reference_t = typename unwrap_reference<T>::type;


  /*! Mark `T` as an array for writing, and reading when
    `default_min_element_size<T::value_type>::value != 0`. See `array_` in
    `wrapper/array.h`. */
  template<typename T>
  struct is_array : std::false_type
  {};

  /*! Mark `T` as fixed binary data for reading+writing. Concept requirements
      for reading:
        * `T` must be compatible with `epee::as_mut_byte_span` (`std::is_pod<T>`
          and no padding).
      Concept requirements for writing:
        * `T` must be compatible with `epee::as_byte_span` (std::is_pod<T>` and
          no padding). */
  template<typename T>
  struct is_blob : std::false_type
  {};

  /*! Forces field to be optional when empty. Concept requirements for `T` when
    `is_optional_on_empty<T>::value == true`:
      * must have an `empty()` method that toggles whether the associated
        `wire::field_<...>` is omitted by the `wire::writer`.
      * must have a `clear()` method where `empty() == true` upon completion,
        used by the `wire::reader` when the `wire::field_<...>` is omitted. */
  template<typename T>
  struct is_optional_on_empty
    : is_array<T> // all array types in old output engine were optional when empty
  {};

  //! When `T` is being read as root object, allow an empty read buffer.
  template<typename T>
  struct is_optional_root
    : std::is_empty<T>
  {};

  //! A constraint for `wire_read::array` where a max of `N` elements can be read.
  template<std::size_t N>
  struct max_element_count
    : std::integral_constant<std::size_t, N>
  {
    // The threshold is low - min_element_size is a better constraint metric
    static constexpr std::size_t max_bytes() noexcept { return 512 * 1024; } // 512 KiB

    //! \return True if `N` C++ objects of type `T` are below `max_bytes()` threshold.
    template<typename T>
    static constexpr bool check() noexcept
    {
      return N <= (max_bytes() / sizeof(T));
    }
  };

  //! A constraint for `wire_read::array` where each element must use at least `N` bytes on the wire.
  template<std::size_t N>
  struct min_element_size
    : std::integral_constant<std::size_t, N>
  {
    static constexpr std::size_t max_ratio() noexcept { return 4; }

    //! \return True if C++ object of type `T` with minimum wire size `N` is below `max_ratio()`.
    template<typename T>
    static constexpr bool check() noexcept
    {
      return N != 0 ? ((sizeof(T) / N) <= max_ratio()) : false;
    }
  };

  /*! Trait used in `wire/read.h` for default `min_element_size` behavior based
    on an array of `T` objects and `R` reader type. This trait can be used
    instead of the `wire::array(...)` (and associated macros) functionality, as
    it sets a global value. The last argument is for `enable_if`. */
  template<typename R, typename T, typename = void>
  struct default_min_element_size
    : std::integral_constant<std::size_t, 0>
  {};

  //! If `T` is a blob, a safe default for all formats is the size of the blob
  template<typename R, typename T>
  struct default_min_element_size<R, T, std::enable_if_t<is_blob<T>::value>>
    : std::integral_constant<std::size_t, sizeof(T)>
  {};

  // example usage : `wire::sum(std::size_t(wire::available(fields))...)`

  inline constexpr int sum() noexcept
  {
    return 0;
  }
  template<typename T, typename... U>
  inline constexpr T sum(const T head, const U... tail) noexcept
  {
    return head + sum(tail...);
  }

  template<typename... T>
  using min_element_sizeof = min_element_size<sum(sizeof(T)...)>;

  //! If container has no `reserve(0)` function, this function is used
  template<typename... T>
  inline void reserve(const T&...) noexcept
  {}

  //! Container has `reserve(std::size_t)` function, use it
  template<typename T>
  inline auto reserve(T& container, const std::size_t count) -> decltype(container.reserve(count))
  { return container.reserve(count); }

  //! If `T` has no `empty()` function, this function is used
  template<typename... T>
  inline constexpr bool empty(const T&...) noexcept
  {
    static_assert(sum(is_optional_on_empty<T>::value...) == 0, "type needs empty method");
    return false;
  }

  //! `T` has `empty()` function, use it
  template<typename T>
  inline auto empty(const T& container) -> decltype(container.empty())
  { return container.empty(); }

  //! If `T` has no `clear()` function, this function is used
  template<typename... T>
  inline void clear(const T&...) noexcept
  {
    static_assert(sum(is_optional_on_empty<T>::value...) == 0, "type needs clear method");
  }

  //! `T` has `clear()` function, use it
  template<typename T>
  inline auto clear(T& container) -> decltype(container.clear())
  { return container.clear(); }
} // wire
