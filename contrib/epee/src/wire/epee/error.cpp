// Copyright (c) 2023, The Monero Project
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

#include "serialization/wire/epee/error.h"

#include <string>

namespace wire
{
namespace error
{
  const char* get_string(const epee value) noexcept
  {
    switch (value)
    {
    default:
      break;
    case epee::invalid_tag:
      return "Found an unknown epee type tag";
    case epee::invalid_varint_type:
      return "Found an unknown epee varint type tag";
    case epee::key_size:
      return "Epee writer encountered a name field that was too long";
    case epee::not_enough_bytes:
      return "Invalid epee encoding; not enough bytes";
    case epee::signature:
      return "Invalid epee binary header signature";
    case epee::varint_size:
      return "Epee writer encountered a length value that was too big";
    case epee::version:
      return "Invalid epee binary version";
    }

    return "Unknown epee parser error";
  }

  const std::error_category& epee_category() noexcept
  {
    struct category final : std::error_category
    {
      virtual const char* name() const noexcept override final
      {
        return "wire::error::epee_category()";
      }

      virtual std::string message(int value) const override final
      {
        return get_string(epee(value));
      }
    };
    static const category instance{};
    return instance;
  }
} // error
} // wire
