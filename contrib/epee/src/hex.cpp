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

#include "hex.h"

#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace epee
{
  namespace
  {
    template<typename T>
    void write_hex(T&& out, const span<const std::uint8_t> src)
    {
      static constexpr const char hex[] = u8"0123456789abcdef";
      static_assert(sizeof(hex) == 17, "bad string size");
      for (const std::uint8_t byte : src)
      {
        *out = hex[byte >> 4];
        ++out;
        *out = hex[byte & 0x0F];
        ++out;
      }
    }
  }

  std::string to_hex::string(const span<const std::uint8_t> src)
  {
    if (std::numeric_limits<std::size_t>::max() / 2 < src.size())
      throw std::range_error("hex_view::to_string exceeded maximum size");

    std::string out{};
    out.resize(src.size() * 2);
    buffer_unchecked(std::addressof(out[0]), src);
    return out;
  }

  void to_hex::buffer(std::ostream& out, const span<const std::uint8_t> src)
  {
    write_hex(std::ostreambuf_iterator<char>{out}, src);
  }

  void to_hex::formatted(std::ostream& out, const span<const std::uint8_t> src)
  {
    out.put('<');
    buffer(out, src);
    out.put('>');
  }

  void to_hex::buffer_unchecked(char* out, const span<const std::uint8_t> src) noexcept
  {
    return write_hex(out, src);
  }
}
