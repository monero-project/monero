// Copyright (c) 2017-2022, The Monero Project
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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace epee
{
  /*!
    \brief Non-owning sequence of data. Does not deep copy

    Inspired by `gsl::span` and/or `boost::iterator_range`. This class is
    intended to be used as a parameter type for functions that need to take a
    writable or read-only sequence of data. Most common cases are `span<char>`
    and `span<std::uint8_t>`. Using as a class member is only recommended if
    clearly documented as not doing a deep-copy. C-arrays are easily convertible
    to this type.

    \note Conversion from C string literal to `span<const char>` will include
      the NULL-terminator.
    \note Never allows derived-to-base pointer conversion; an array of derived
      types is not an array of base types.
   */
  template<typename T>
  class span
  {
    template<typename U>
    static constexpr bool safe_conversion() noexcept
    {
      // Allow exact matches or `T*` -> `const T*`.
      using with_const = typename std::add_const<U>::type;
      return std::is_same<T, U>() ||
        (std::is_const<T>() && std::is_same<T, with_const>());
    }

  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr span() noexcept : ptr(nullptr), len(0) {}
    constexpr span(std::nullptr_t) noexcept : span() {}

    //! Prevent derived-to-base conversions; invalid in this context.
    template<typename U, typename = typename std::enable_if<safe_conversion<U>()>::type>
    constexpr span(U* const src_ptr, const std::size_t count) noexcept
      : ptr(src_ptr), len(count) {}

    //! Conversion from C-array. Prevents common bugs with sizeof + arrays.
    template<std::size_t N>
    constexpr span(T (&src)[N]) noexcept : span(src, N) {}

    constexpr span(const span&) noexcept = default;
    span& operator=(const span&) noexcept = default;

    /*! Try to remove `amount` elements from beginning of span.
    \return Number of elements removed. */
    std::size_t remove_prefix(std::size_t amount) noexcept
    {
        amount = std::min(len, amount);
        ptr += amount;
        len -= amount;
        return amount;
    }

    constexpr iterator begin() const noexcept { return ptr; }
    constexpr const_iterator cbegin() const noexcept { return ptr; }

    constexpr iterator end() const noexcept { return begin() + size(); }
    constexpr const_iterator cend() const noexcept { return cbegin() + size(); }

    constexpr bool empty() const noexcept { return size() == 0; }
    constexpr pointer data() const noexcept { return ptr; }
    constexpr std::size_t size() const noexcept { return len; }
    constexpr std::size_t size_bytes() const noexcept { return size() * sizeof(value_type); }

    T &operator[](size_t idx) noexcept { return ptr[idx]; }
    const T &operator[](size_t idx) const noexcept { return ptr[idx]; }

  private:
    T* ptr;
    std::size_t len;
  };

  //! \return `span<const T::value_type>` from a STL compatible `src`.
  template<typename T>
  constexpr span<const typename T::value_type> to_span(const T& src)
  {
    // compiler provides diagnostic if size() is not size_t.
    return {src.data(), src.size()};
  }

  //! \return `span<T::value_type>` from a STL compatible `src`.
  template<typename T>
  constexpr span<typename T::value_type> to_mut_span(T& src)
  {
    // compiler provides diagnostic if size() is not size_t.
    return {src.data(), src.size()};
  }

  template<typename T>
  constexpr bool has_padding() noexcept
  {
    return !std::is_standard_layout<T>() || alignof(T) != 1;
  }

  //! \return Cast data from `src` as `span<const std::uint8_t>`.
  template<typename T>
  span<const std::uint8_t> to_byte_span(const span<const T> src) noexcept
  {
    static_assert(!has_padding<T>(), "source type may have padding");
    return {reinterpret_cast<const std::uint8_t*>(src.data()), src.size_bytes()}; 
  }

  //! \return `span<const std::uint8_t>` which represents the bytes at `&src`.
  template<typename T>
  span<const std::uint8_t> as_byte_span(const T& src) noexcept
  {
    static_assert(!std::is_empty<T>(), "empty types will not work -> sizeof == 1");
    static_assert(!has_padding<T>(), "source type may have padding");
    return {reinterpret_cast<const std::uint8_t*>(std::addressof(src)), sizeof(T)};
  }

  //! \return `span<std::uint8_t>` which represents the bytes at `&src`.
  template<typename T>
  span<std::uint8_t> as_mut_byte_span(T& src) noexcept
  {
    static_assert(!std::is_empty<T>(), "empty types will not work -> sizeof == 1");
    static_assert(!has_padding<T>(), "source type may have padding");
    return {reinterpret_cast<std::uint8_t*>(std::addressof(src)), sizeof(T)};
  }

  //! make a span from a std::string
  template<typename T, typename U>
  span<const T> strspan(const U&s) noexcept
  {
    static_assert(std::is_same<typename U::value_type, char>(), "unexpected source type");
    static_assert(std::is_same<T, char>() || std::is_same<T, unsigned char>() || std::is_same<T, int8_t>() || std::is_same<T, uint8_t>(), "Unexpected destination type");
    return {reinterpret_cast<const T*>(s.data()), s.size()};
  }
}
