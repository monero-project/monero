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

#include <type_traits>
#include <utility>
#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"
#include "span.h"

//
// Also see `wire/traits.h` to register a type as a blob (no wrapper needed).
//


/*! A required field, where the wire content is expected to be a binary blob,
  and the C++ data is a pod type with no padding. */
#define WIRE_FIELD_BLOB(name)                                  \
  ::wire::field( #name , ::wire::blob(std::ref( self . name )))

namespace wire
{
  /*! A wrapper that tells `wire::writer`s` and `wire::reader`s to encode a
    type as a binary blob. If the encoded size on the wire is not exactly the
    size of the blob, it is considered an error.

    `value_type` is `T` with optional `std::reference_wrapper` removed.
    `value_type` concept requirements:
      * `epee::has_padding<value_type>()` must return false. */
  template<typename T>
  struct blob_
  {
    using value_type = unwrap_reference_t<T>;
    static_assert(!epee::has_padding<value_type>(), "expected safe pod type");

    T value;

    //! \return `value` with `std::reference_wrapper` removed.
    constexpr const value_type& get_value() const noexcept { return value; }

    //! \return `value` with `std::reference_wrapper` removed.
    value_type& get_value() noexcept { return value; }
  };

  template<typename T>
  constexpr inline blob_<T> blob(T value)
  {
    return {std::move(value)};
  }

  template<typename R, typename T>
  inline void read_bytes(R& source, blob_<T> dest)
  {
    source.binary(epee::as_mut_byte_span(dest.get_value()), /*exact=*/ true);
  }

  template<typename W, typename T>
  inline void write_bytes(W& dest, const blob_<T> source)
  {
    dest.binary(epee::as_byte_span(source.get_value()));
  }
} // wire
