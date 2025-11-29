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

#include "serialization/wire/error.h"

namespace wire
{
  namespace error
  {
    const char* get_string(const schema value) noexcept
    {
      switch (value)
      {
      default:
        break;

      case schema::none:
        return "No schema errors";
      case schema::array:
        return "Schema expected array";
      case schema::array_max_element:
        return "Schema expected array size to be smaller";
      case schema::array_min_size:
        return "Schema expected minimum wire size per array element to be larger";
      case schema::binary:
        return "Schema expected binary value of variable size";
      case schema::boolean:
        return "Schema expected boolean value";
      case schema::enumeration:
        return "Schema expected a specific of enumeration value(s)";
      case schema::fixed_binary:
        return "Schema expected binary of fixed size";
      case schema::integer:
        return "Schema expected integer value";
      case schema::invalid_key:
        return "Schema does not allow object field key";
      case schema::larger_integer:
        return "Schema expected a larger integer value";
      case schema::maximum_depth:
        return "Schema hit maximum array+object depth tracking";
      case schema::missing_key:
        return "Schema missing required field key";
      case schema::number:
        return "Schema expected number (integer or float) value";
      case schema::object:
        return "Schema expected object";
      case schema::smaller_integer:
        return "Schema expected a smaller integer value";
      case schema::string:
        return "Schema expected string";
      }
      return "Unknown schema error";
    }

    const std::error_category& schema_category() noexcept
    {
      struct category final : std::error_category
      {
        virtual const char* name() const noexcept override final
          {
            return "wire::error::schema_category()";
          }

          virtual std::string message(int value) const override final
          {
            return get_string(schema(value));
          }
      };
      static const category instance{};
      return instance;
    }
  }
}
