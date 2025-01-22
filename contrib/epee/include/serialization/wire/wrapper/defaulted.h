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

#include <functional>
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"

//! An optional field that is omitted when a default value is used
#define WIRE_FIELD_DEFAULTED(name, default_)                            \
  ::wire::optional_field( #name , ::wire::defaulted(std::ref( self . name ), default_ ))

namespace wire
{
  /*! A wrapper that tells `wire::writer`s to skip field generation when default
    value, and tells `wire::reader`s to use default value when field not present. */
  template<typename T, typename U>
  struct defaulted_
  {
    using value_type = unwrap_reference_t<T>;

    T value;
    U default_;

    constexpr const value_type& get_value() const noexcept { return value; }
    value_type& get_value() noexcept { return value; }

    // concept requirements for optional fields

    constexpr explicit operator bool() const { return get_value() != default_; }
    value_type& emplace() noexcept { return get_value(); }

    constexpr const value_type& operator*() const noexcept { return get_value(); }
    value_type& operator*() noexcept { return get_value(); }

    void reset() { get_value() = default_; }
  };

  //! Links `value` with `default_`.
  template<typename T, typename U>
  inline constexpr defaulted_<T, U> defaulted(T value, U default_)
  {
    return {std::move(value), std::move(default_)};
  }

  /* read/write functions not needed since `defaulted_` meets the concept
     requirements for an optional type (optional fields are handled
     directly by the generic read/write code because the field name is omitted
     entirely when the value is "empty"). */
} // wire
