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

#include <boost/variant/variant.hpp>
#include <cstddef>
#include <cstdint>
#include <string>

#include "serialization/wire/fwd.h"

namespace wire
{
  /*! Can hold any non-recursive value. Implements optional field concept
    requirements. If used in a `optional_field`, the `nullptr` type/value
    determines whether the field name is omitted or present in an object. If used
    in a `field` (required), the field name is always present in the object, and
    the value could be `null`/`nil`. */
  struct basic_value
  {
    using variant_type =
      boost::variant<std::nullptr_t, bool, std::uintmax_t, std::intmax_t, double, std::string>;
    
    variant_type value;

    // concept requirements for optional fields

    explicit operator bool() const noexcept { return value != variant_type{nullptr}; }
    basic_value& emplace() noexcept { return *this; }

    basic_value& operator*() noexcept { return *this; }
    const basic_value& operator*() const noexcept { return *this; }

    void reset();
  };

  void read_bytes(reader& source, basic_value& dest);
  void write_bytes(writer& dest, const basic_value& source);
} // wire
