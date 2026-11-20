// Copyright (c) 2021, The Monero Project
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
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"

/*! An array field with read constraint. See `array_` for more info. All (empty)
  arrays were "optional" (omitted) historically in epee, so this matches prior
  behavior. */
#define WIRE_FIELD_ARRAY(name, read_constraint)         \
  ::wire::optional_field( #name , ::wire::array< read_constraint >(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that ensures `T` is written as an array, with `C` constraints
    when reading (`max_element_count` or `min_element_size`). `C` can be `void`
    if write-only.

    This wrapper meets the requirements for an optional field; `wire::field`
    and `wire::optional_field` determine whether an empty array must be
    encoded on the wire. Historically, empty arrays were always omitted on
    the wire (a defacto optional field).

    The `is_array` trait can also be used, but is default treated as an optional
    field. The trait `is_optional_on_empty` traits can be specialized to disable
    the optional on empty behavior. See `wire/traits.h` for more ifnormation
    on the `is_optional_on_empty` trait.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
      * must have `size()` method that returns number of elements.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` method that removes all elements (`size() == 0`).
      * must have either: (1) `end()` and `emplace_hint(iterator, value_type&&)`
        or (2) `emplace_back()` and `back()`:
        * `end()` method that returns one-past the last element.
        * `emplace_hint(iterator, value_type&&)` method that move constructs a new
           element.
        * `emplace_back()` method that default initializes new element
        * `back()` method that retrieves last element by reference.
    Additional concept requirements for `container_type` when writing:
      * must work with foreach loop (`std::begin` and `std::end`).
      * must work with `boost::size` (from the `boost::range` library). */
  template<typename T, typename C = void>
  struct array_
  {
    using constraint = C;
    using container_type = unwrap_reference_t<T>;
    using value_type = typename container_type::value_type;

    // See nested `array_` overload below
    using inner_array = std::reference_wrapper<value_type>;
    using inner_array_const = std::reference_wrapper<const value_type>;

    T container;

    constexpr const container_type& get_container() const noexcept { return container; }
    container_type& get_container() noexcept { return container; }

    //! Read directly into the non-nested array
    container_type& get_read_object() noexcept { return get_container(); }


    // concept requirements for optional fields

    explicit operator bool() const noexcept { return !get_container().empty(); }
    array_& emplace() noexcept { return *this; }

    array_& operator*() noexcept { return *this; }
    const array_& operator*() const noexcept { return *this; }

    void reset() { get_container().clear(); }
  };

  //! Nested array case
  template<typename T, typename C, typename D>
  struct array_<array_<T, C>, D>
  {
    // compute `container_type` and `value_type` recursively
    using constraint = D;
    using container_type = typename array_<T, C>::container_type;
    using value_type = typename container_type::value_type;

    // Re-compute `array_` for inner values
    using inner_array = array_<typename array_<T, C>::inner_array, C>;
    using inner_array_const = array_<typename array_<T, C>::inner_array_const, C>;

    array_<T, C> nested;

    const container_type& get_container() const noexcept { return nested.get_container(); }
    container_type& get_container() noexcept { return nested.get_container(); }

    //! Read through this proxy to track nested array
    array_& get_read_object() noexcept { return *this; }


    // concept requirements for optional fields

    explicit operator bool() const noexcept { return !empty(); }
    array_& emplace() noexcept { return *this; }

    array_& operator*() noexcept { return *this; }
    const array_& operator*() const noexcept { return *this; }

    void reset() { clear(); }


    /* For reading nested arrays. writing nested arrays is handled in
       `wrappers_impl.h` with range transform. */

    void clear() { get_container().clear(); }
    bool empty() const noexcept { return get_container().empty(); }
    std::size_t size() const noexcept { return get_container().size(); }

    void emplace_back() { get_container().emplace_back(); }

    //! \return A proxy object for tracking inner-array constraints
    inner_array back() noexcept { return {std::ref(get_container().back())}; }
  };

  //! Treat `value` as an array when reading/writing, and constrain reading with `C`.
  template<typename C = void, typename T = void>
  inline constexpr array_<T, C> array(T value)
  {
    return {std::move(value)};
  }

  /* Do not register with `is_optional_on_empty` trait, this allows selection
     on whether an array is mandatory on wire. */

} // wire
