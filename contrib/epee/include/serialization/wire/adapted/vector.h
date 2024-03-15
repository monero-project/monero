// Copyright (c) 2022-2024-2023, The Monero Project
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
#include <cstring>
#include <type_traits>
#include <vector>

#include "byte_slice.h"
#include "serialization/wire/traits.h"

namespace wire
{
  template<typename T, typename A>
  struct is_array<std::vector<T, A>>
    : std::true_type
  {};
  template<typename A>
  struct is_array<std::vector<std::uint8_t, A>>
    : std::false_type
  {};

  template<typename R, typename A>
  inline void read_bytes(R& source, std::vector<std::uint8_t, A>& dest)
  {
    const epee::byte_slice bytes = source.binary();
    dest.resize(bytes.size());
    std::memcpy(dest.data(), bytes.data(), bytes.size());
  }
  template<typename W, typename A>
  inline void write_bytes(W& dest, const std::vector<std::uint8_t, A>& source)
  {
    dest.binary(epee::to_span(source));
  }
}
