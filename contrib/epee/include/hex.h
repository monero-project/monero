// Copyright (c) 2017-2020, The Monero Project
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

#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <boost/utility/string_ref.hpp>

#include "wipeable_string.h"
#include "span.h"

namespace epee
{
  struct to_hex
  {
    //! \return A std::string containing hex of `src`.
    static std::string string(const span<const std::uint8_t> src);
    //! \return A epee::wipeable_string containing hex of `src`.
    static epee::wipeable_string wipeable_string(const span<const std::uint8_t> src);
    template<typename T> static epee::wipeable_string wipeable_string(const T &pod) { return wipeable_string(span<const uint8_t>((const uint8_t*)&pod, sizeof(pod))); }

    //! \return An array containing hex of `src`.
    template<std::size_t N>
    static std::array<char, N * 2> array(const std::array<std::uint8_t, N>& src) noexcept
    {
      std::array<char, N * 2> out;
      static_assert(N <= 128, "keep the stack size down");
      buffer_unchecked(out.data(), {src.data(), src.size()});
      return out;
    }

    //! \return An array containing hex of `src`.
    template<typename T>
    static std::array<char, sizeof(T) * 2> array(const T& src) noexcept
    {
      std::array<char, sizeof(T) * 2> out;
      static_assert(sizeof(T) <= 128, "keep the stack size down");
      buffer_unchecked(out.data(), as_byte_span(src));
      return out;
    }

    //! Append `src` as hex to `out`.
    static void buffer(std::ostream& out, const span<const std::uint8_t> src);

    //! Append `< + src + >` as hex to `out`.
    static void formatted(std::ostream& out, const span<const std::uint8_t> src);

  private:
    template<typename T> T static convert(const span<const std::uint8_t> src);

    //! Write `src` bytes as hex to `out`. `out` must be twice the length
    static void buffer_unchecked(char* out, const span<const std::uint8_t> src) noexcept;
  };

  //! Convert hex in UTF8 encoding to binary
  struct from_hex
  {
    static bool to_string(std::string& out, boost::string_ref src);

    static bool to_buffer(span<std::uint8_t> out, boost::string_ref src) noexcept;

  private:
    static bool to_buffer_unchecked(std::uint8_t* out, boost::string_ref src) noexcept;
  };

  //! Convert hex in current C locale encoding to binary
  struct from_hex_locale
  {
      static std::vector<uint8_t> to_vector(boost::string_ref src);
  };
}
