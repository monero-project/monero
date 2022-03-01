// Copyright (c) 2017-2022, The Monero Project
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

#include "storages/parserse_base_utils.h"

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

  template<typename T>
  T to_hex::convert(const span<const std::uint8_t> src)
  {
    if (std::numeric_limits<std::size_t>::max() / 2 < src.size())
      throw std::range_error("hex_view::to_string exceeded maximum size");

    T out{};
    out.resize(src.size() * 2);
    to_hex::buffer_unchecked((char*)out.data(), src); // can't see the non const version in wipeable_string??
    return out;
  }

  std::string to_hex::string(const span<const std::uint8_t> src) { return convert<std::string>(src); }
  epee::wipeable_string to_hex::wipeable_string(const span<const std::uint8_t> src) { return convert<epee::wipeable_string>(src); }

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


  bool from_hex::to_string(std::string& out, const boost::string_ref src)
  {
    out.resize(src.size() / 2);
    return to_buffer_unchecked(reinterpret_cast<std::uint8_t*>(&out[0]), src);
  }

  bool from_hex::to_buffer(span<std::uint8_t> out, const boost::string_ref src) noexcept
  {
    if (src.size() / 2 != out.size())
      return false;
    return to_buffer_unchecked(out.data(), src);
  }

  bool from_hex::to_buffer_unchecked(std::uint8_t* dst, const boost::string_ref s) noexcept
  {
      if (s.size() % 2 != 0)
        return false;

      const unsigned char *src = (const unsigned char *)s.data();
      for(size_t i = 0; i < s.size(); i += 2)
      {
        int tmp = *src++;
        tmp = epee::misc_utils::parse::isx[tmp];
        if (tmp == 0xff) return false;
        int t2 = *src++;
        t2 = epee::misc_utils::parse::isx[t2];
        if (t2 == 0xff) return false;
        *dst++ = (tmp << 4) | t2;
      }

      return true;
  }


  std::vector<uint8_t> from_hex_locale::to_vector(const boost::string_ref src)
  {
    // should we include a specific character
    auto include = [](char input) {
        // we ignore spaces and colons
        return !std::isspace(input) && input != ':';
    };

    // the number of relevant characters to decode
    auto count = std::count_if(src.begin(), src.end(), include);

    // this must be a multiple of two, otherwise we have a truncated input
    if (count % 2) {
      throw std::length_error{ "Invalid hexadecimal input length" };
    }

    std::vector<uint8_t> result;
    result.reserve(count / 2);

    // the data to work with (std::string is always null-terminated)
    auto data = src.begin();

    // convert a single hex character to an unsigned integer
    auto char_to_int = [](const char *input) {
      switch (std::tolower(*input)) {
        case '0': return  0;
        case '1': return  1;
        case '2': return  2;
        case '3': return  3;
        case '4': return  4;
        case '5': return  5;
        case '6': return  6;
        case '7': return  7;
        case '8': return  8;
        case '9': return  9;
        case 'a': return 10;
        case 'b': return 11;
        case 'c': return 12;
        case 'd': return 13;
        case 'e': return 14;
        case 'f': return 15;
        default:  throw std::range_error{ "Invalid hexadecimal input" };
      }
    };

    // keep going until we reach the end
    while (data != src.end()) {
      // skip unwanted characters
      if (!include(*data)) {
        ++data;
        continue;
      }

      // convert two matching characters to int
      auto high = char_to_int(data++);
      auto low  = char_to_int(data++);

      result.push_back(high << 4 | low);
    }

    return result;
  }
}
