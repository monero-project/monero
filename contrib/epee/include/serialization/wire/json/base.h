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

#include <string>
#include <system_error>
#include "serialization/wire/json/fwd.h"
#include "span.h"

//! Declare functions that convert `type` to/from json bytes
#define WIRE_JSON_DECLARE_CONVERSION(type)                              \
  std::error_code convert_from_json(::epee::span<const char>, type&);   \
  std::error_code convert_to_json(std::string&, const type&)            \

//! Declare functions that convert `type::request` and `type::response` to/from json bytes
#define WIRE_JSON_DECLARE_COMMAND(type)	       \
  WIRE_JSON_DECLARE_CONVERSION(type::request); \
  WIRE_JSON_DECLARE_CONVERSION(type::response)

namespace epee { class byte_stream; }
namespace wire
{
  struct json
  {
    using input_type = json_reader;
    using output_type = json_string_writer;

    //! Only enabled if `T` has templated method `from_bytes`.
    template<typename T>
    static auto from_bytes(const epee::span<const char> source, T& dest) -> decltype(dest.template from_bytes<input_type>(source))
    {
      return dest.template from_bytes<input_type>(source);
    }

    //! Only enabled if `T` has templated method `to_bytes`.
    template<typename T>
    static auto to_bytes(std::string& dest, const T& source) -> decltype(source.template to_bytes<output_type>(dest))
    {
      return source.template to_bytes<output_type>(dest);
    }

    // Parameters packs have lower precedence; above functions are preferred.

    template<typename... T>
    static std::error_code from_bytes(const epee::span<const char> source, T&... dest)
    {
      return convert_from_json(source, dest...); // ADL (searches every associated namespace)
    }

    template<typename... T>
    static std::error_code to_bytes(std::string& dest, const T&... source)
    {
      return convert_to_json(dest, source...); // ADL (searches every associated namespace)
    }
  };
}

