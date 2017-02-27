// Copyright (c) 2017, The Monero Project
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

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace epee
{
  /*!
    \brief Non-owning sequence of data. Does not deep copy

    Inspired by `gsl::span`. This class is intended to be used as a parameter
    type for functions that need to take a writable or read-only sequence of
    data. Most common cases are `span<char>` and `span<std::uint8_t>`. Using as
    a class member is only recommended if clearly documented as not doing a
    deep-copy. `std::vector`, `std::string`, `std::array`, and C-arrays are all
    easily convertible to this type.

    \note Conversion from C string literal to `view<char>` will include the
      NULL-terminator.
    \note `view<T>` is an alias for `span<const T>` which is read-only.
    \note Never allows derived-to-base pointer conversion; an array of derived
      types is not an array of base types.
   */
  template<typename T>
  class span
  {
    /* Supporting class types is tricky - the {ptr,len} and {container}
       constructors will allow derived-to-base conversions. This is NOT
       desireable because an array of derived types is not an array of base
       types. It is possible to handle this case, implement when/if needed. */
    static_assert(std::is_integral<T>::value, "only integral values currently for span");

    // Disables ADL for these calls.
    struct get {
      // we only accept pointer and size_t return types, so use noexcept aggressively
      template<std::size_t N>
      static constexpr T* data(T (&src)[N]) noexcept { return src; }

      template<typename C>
      static constexpr auto
      data(C& src) noexcept(noexcept(src.data())) -> decltype(src.data()) {
        return src.data();
      }


      template<std::size_t N>
      static constexpr std::size_t size(T (&)[N]) noexcept { return N; }

      template<typename C>
      static constexpr auto
      size(const C& src) noexcept(noexcept(src.size())) -> decltype(src.size()) {
        return src.size();
      }
    };

  public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator = pointer;
    using const_iterator = typename std::add_const<value_type>::type *;

    constexpr span() noexcept : ptr(nullptr), len(0) {}
    constexpr span(std::nullptr_t) noexcept : span() {}

    constexpr span(T* const src_ptr, const std::size_t src_len) noexcept
      : ptr(src_ptr), len(src_len) {
    }

    /*!
       A `span` is implicitly convertible from any type where `get::data(src)`
       returns a pointer convertible to T* and `get::size(src)` returns a
       `std::size_t`. This constructor is disabled for all other types.
     */
    template<
      typename C,
      typename U = decltype(get::data(std::declval<C&>())),
      typename S = decltype(get::size(std::declval<C&>())),
      typename = typename std::enable_if<
        // Do not allow proxy iterators, but allow valid cv conversions
        std::is_pointer<U>::value && std::is_convertible<U, pointer>::value &&
        // Prevent underflow/overflow on size storage
        std::is_same<S, std::size_t>::value
      >::type
    >
    constexpr span(C&& src) noexcept(noexcept(get::data(src), get::size(src)))
      : span(get::data(src), get::size(src)) {
    }

    constexpr span(span&&) noexcept = default;
    constexpr span(span&) noexcept = default;
    constexpr span(const span&) noexcept = default;
    span& operator=(const span&) noexcept = default;

    constexpr iterator begin() const noexcept { return ptr; }
    constexpr const_iterator cbegin() const noexcept { return ptr; }

    constexpr iterator end() const noexcept { return begin() + size(); }
    constexpr const_iterator cend() const noexcept { return cbegin() + size(); }

    constexpr bool empty() const noexcept { return size() == 0; }
    constexpr pointer data() const noexcept { return ptr; }
    constexpr std::size_t size() const noexcept { return len; }
    constexpr std::size_t size_bytes() const noexcept { return size() * sizeof(value_type); }

  private:
    T* ptr;
    std::size_t len;
  };

  //! A `view` is a `span` over immutable data. See `span` for more info.
  template<typename T>
  using view = span<typename std::add_const<T>::type>;

  //! \return `src` aliased as a view of type `T`.
  template<typename T, typename U>
  view<T> view_cast(const span<U>& src) noexcept
  {
    using normalized = typename std::remove_cv<T>::type;
    static_assert(
      std::is_same<normalized, char>::value || std::is_same<normalized, std::uint8_t>::value,
      "only valid aliasing allowed - char or unsigned char target types"
    );

    static_assert(std::is_integral<U>::value, "only integral source types");
    return {reinterpret_cast<typename std::add_const<T>::type *>(src.data()), src.size_bytes()};
  }

  //! \return `src` aliased as a span of type `T`.
  template<typename T, typename U>
  span<T> pod_cast(U&& src) noexcept
  {
    using normalized = typename std::remove_cv<T>::type;
    static_assert(
      std::is_same<normalized, char>::value || std::is_same<normalized, std::uint8_t>::value,
      "only valid aliasing allowed - char or unsigned char target types"
    );

    using decayed = typename std::decay<U>::type;
    static_assert(std::is_pod<decayed>::value, "only POD types allowed for conversion");
    static_assert(alignof(decayed) == 1, "only types with 1 byte alignment allowed for conversion");
    return {reinterpret_cast<T*>(std::addressof(src)), sizeof(decayed)};
  }
}
