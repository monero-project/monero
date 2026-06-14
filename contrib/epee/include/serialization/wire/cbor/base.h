// Copyright (c) 2026, The Monero Project
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

#include <cstdint>
#include <limits>
#include <system_error>
#include <tuple>

#include "byte_slice.h"
#include "serialization/wire/cbor/fwd.h"

namespace wire
{
  struct cbor
  {
    using input_type = cbor_reader;
    using output_type = cbor_slice_writer;

    // Major type defined in RFC
    enum class major : std::uint8_t
    {
      positive_int = 0,
      negative_int = 1 << 5,
      bytes = 2 << 5,
      string = 3 << 5,
      array = 4 << 5,
      map = 5 << 5,
      //tagged = 6 << 5, not supported
      real_and_special = 7 << 5
    };

    //! If the unsigned value is less equal than this, store in minor type
    static constexpr std::uint8_t inline_max() { return 23; }

    template<std::uint8_t M>
    struct simple
    {
      static constexpr std::uint8_t tag() noexcept { return M; }
      static constexpr std::uint8_t value() noexcept
      { return std::uint8_t(major::real_and_special) | tag(); }
    };

    using false_ = simple<20>;
    using true_ = simple<21>;
    using null = simple<22>;
    using undefined = simple<23>;
    using stop = simple<31>;

    template<typename T, std::uint8_t M>
    struct minor
    {
      using type = T;
      static constexpr T min() noexcept { return std::numeric_limits<T>::min(); }
      static constexpr T max() noexcept { return std::numeric_limits<T>::max(); }
      static constexpr std::uint8_t value() noexcept { return M; }
    };

    using uint8_t = minor<std::uint8_t, 24>;
    using uint16_t = minor<std::uint16_t, 25>;
    using uint32_t = minor<std::uint32_t, 26>;
    using uint64_t = minor<std::uint64_t, 27>;

    using unsigned_types =
      std::tuple<cbor::uint8_t, cbor::uint16_t, cbor::uint32_t, cbor::uint64_t>;

    using real16 = minor<std::uint16_t, 25>;
    using real32 = minor<std::uint32_t, 26>;
    using real64 = minor<std::uint64_t, 27>;

    //! Only enabled if `T` has templated method `from_bytes`.
    template<typename T>
    static auto from_bytes(const epee::span<const char> source, T& dest) -> decltype(dest.template from_bytes<input_type>(source))
    {
      return dest.template from_bytes<input_type>(source);
    }

    //! Only enabled if `T` has templated method `to_bytes`.
    template<typename T>
    static auto to_bytes(epee::byte_stream& dest, const T& source) -> decltype(source.template to_bytes<output_type>(dest))
    {
      return source.template to_bytes<output_type>(dest);
    }


    // Parameters packs have lower precedence; above functions are preferred.

    template<typename... T>
    static std::error_code from_bytes(const epee::span<const char> source, T&... dest)
    {
      return convert_from_cbor(source, dest...); // ADL (searches every associated namespace)
    }

    template<typename... T>
    static std::error_code to_bytes(epee::byte_stream& dest, const T&... source)
    {
      return convert_to_cbor(dest, source...); // ADL (searches every associated namespace)
    }
  };
}

