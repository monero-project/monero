// Copyright (c) 2025, The Monero Project
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

#include <type_traits>
#include <utility>

#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"
#include "serialization/wire.h"

namespace wire
{
  //! A wrapper that tells `wire::writer`s to write the inner type as an array.
  template<typename U>
  struct range_
  {
    WIRE_DEFINE_CONVERSIONS()

    using value_type = unwrap_reference_t<U>;

    U value;

    constexpr const value_type& get_value() const noexcept { return value; }
    value_type& get_value() noexcept { return value; }

    // concept requirements for `is_optional_on_empty`    
    bool empty() const { return get_value().empty(); }
  };

  template<typename F, typename T>
  void write_bytes(F& format, const range_<T> self)
  {
    wire_write::array(format, self.get_value());
  }

  //! Links `value` with `range_`.
  template<typename T>
  inline constexpr range_<T> range(T value)
  {
    return {std::move(value)};
  }

  template<typename T>
  struct is_optional_on_empty<range_<T>>
    : std::true_type
  {};
} // wire
