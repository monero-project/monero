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
#include <type_traits>
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"

/*! A required field, where the array contents are written as a single binary
  blob. All (empty) arrays-as-blobs were "optional" (omitted) historically in
  epee, so this matches prior behavior. */
#define WIRE_FIELD_ARRAY_AS_BLOB(name)                                  \
  ::wire::optional_field( #name , ::wire::array_as_blob(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that tells `wire::writer`s` and `wire::reader`s to encode a
    container as a single binary blob. This wrapper meets the requirements for
    an optional field - currently the type is always considered optional by
    the input/output system to match the old input/output engine.

    `container_type` is `T` with optional `std::reference_wrapper` removed.
    `container_type` concept requirements:
      * `typedef` `value_type` that specifies inner type.
      * `std::is_pod<value_type>::value` must be true.
    Additional concept requirements for `container_type` when reading:
      * must have `clear()` method that removes all elements (`size() == 0`).
      * must have `emplace_back()` method that default initializes new element
      * must have `back()` method that retrieves last element by reference.
    Additional concept requirements for `container_type` when writing:
      * must work with foreach loop (`std::begin` and `std::end`).
      * must have `size()` method that returns number of elements. */
  template<typename T>
  struct array_as_blob_
  {
    using container_type = unwrap_reference_t<T>;
    using value_type = typename container_type::value_type;
    static constexpr std::size_t value_size() noexcept { return sizeof(value_type); }
    static_assert(std::is_pod<value_type>::value, "container value must be POD");

    T container;

    constexpr const container_type& get_container() const noexcept { return container; }
    container_type& get_container() noexcept { return container; }

    // concept requirements for optional fields

    explicit operator bool() const noexcept { return !get_container().empty(); }
    container_type& emplace() noexcept { return get_container(); }

    array_as_blob_& operator*() noexcept { return *this; }
    const array_as_blob_& operator*() const noexcept { return *this; }

    void reset() { get_container().clear(); }
  };

  template<typename T>
  inline array_as_blob_<T> array_as_blob(T value)
  {
    return {std::move(value)};
  }

  // `read_bytes` / `write_bytes` in `wire/wrappers_impl.h`

  // Do not specialize `is_optional_on_empty`; allow selection

} // wire
