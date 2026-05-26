// Copyright (c) 2023-2026, The Monero Project
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

#include <boost/core/demangle.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <type_traits>

#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/cbor.h"
#include "serialization/wire/field.h"
#include "serialization/wire/traits.h"

#include "hex.h"
#include <iostream>

namespace wire
{
  struct small_blob { std::uint8_t data[4]; };
  WIRE_DECLARE_BLOB_NS(small_blob);
}

namespace
{
  template<typename T>
  using limits = std::numeric_limits<T>;

  constexpr const char basic_string[] = u8"my_string_data";
  constexpr const wire::small_blob basic_blob{{0x00, 0xff, 0x22, 0x11}};

  //  u8"{\"utf8\":\"my_string_data\",\"vec\":[0,127],\"data\":\"00ff2211\",\"integer\":-2,\"real\":3.5,\"choice\":true}";
  constexpr const std::uint8_t basic_cbor[] = {
    0xa6, 0x64, 0x75, 0x74, 0x66, 0x38, 0x6e, 0x6d, 0x79, 0x5f, 0x73, 0x74,
    0x72, 0x69, 0x6e, 0x67, 0x5f, 0x64, 0x61, 0x74, 0x61, 0x63, 0x76, 0x65,
    0x63, 0x82, 0x00, 0x18, 0x7f, 0x64, 0x64, 0x61, 0x74, 0x61, 0x44, 0x00,
    0xff, 0x22, 0x11, 0x67, 0x69, 0x6e, 0x74, 0x65, 0x67, 0x65, 0x72, 0x21,
    0x64, 0x72, 0x65, 0x61, 0x6c, 0xfb, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x66, 0x63, 0x68, 0x6f, 0x69, 0x63, 0x65, 0xf5
  };

  template<typename T>
  struct integer { T val; };

  template<typename T>
  struct object
  {
    std::string utf8;
    std::vector<T> vec;
    wire::small_blob data;
    std::int32_t integer;
    double real;
    bool choice;
  };

  template<typename F, typename T>
  void object_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(utf8),
      WIRE_FIELD(vec),
      WIRE_FIELD(data),
      WIRE_FIELD(integer),
      WIRE_FIELD(real),
      WIRE_FIELD(choice)
    );
  }

  template<typename T>
  void write_bytes(wire::writer& dest, const object<T>& src)
  {
    object_map(dest, src);
  }

  template<typename T>
  void write_bytes(wire::writer& dest, const integer<T>& src)
  {
    write_bytes(dest, src.val);
  }

  template<typename T>
  std::error_code convert_to_cbor(epee::byte_stream& dest, const T& src)
  {
    return wire_write::to_bytes<wire::cbor_slice_writer>(dest, src);
  }

  template<typename T>
  bool test_writing()
  {
    const object<T> val{basic_string, std::vector<T>{0, 127}, basic_blob, -2, 3.5, true};   
    epee::byte_stream result{};
    const std::error_code error = wire::cbor::to_bytes(result, val);
    return !error && boost::range::equal(epee::byte_slice{std::move(result)}, epee::byte_slice{basic_cbor});
  }

  template<typename T>
  bool test_integer(const T val, const epee::span<const std::uint8_t> expected)
  {
    epee::byte_stream result{};
    const std::error_code error = wire::cbor::to_bytes(result, integer<T>{val});
    return !error && boost::range::equal(epee::byte_slice{std::move(result)}, expected);
  }
}

TEST(wire, cbor_writer_object)
{
  EXPECT_TRUE(test_writing<std::int16_t>());
  EXPECT_TRUE(test_writing<std::int32_t>());
  EXPECT_TRUE(test_writing<std::int64_t>());
  EXPECT_TRUE(test_writing<std::intmax_t>());
  EXPECT_TRUE(test_writing<std::uint16_t>());
  EXPECT_TRUE(test_writing<std::uint32_t>());
  EXPECT_TRUE(test_writing<std::uint64_t>());
  EXPECT_TRUE(test_writing<std::uintmax_t>());
}

TEST(wire, cbor_writer_integers)
{
  const std::uint8_t v_0[] = {0x00};
  const std::uint8_t v_23[] = {23};
  const std::uint8_t v_24[] = {24, 24};
  const std::uint8_t v_uint8[] = {24, 0xff};
  const std::uint8_t v_uint16[] = {25, 0xff, 0xff};
  const std::uint8_t v_uint32[] = {26, 0xff, 0xff, 0xff, 0xff};
  const std::uint8_t v_uint64[] = {27, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  const std::uint8_t v_n1[] = {0x20};
  const std::uint8_t v_n24[] = {0x20 | 23};
  const std::uint8_t v_n25[] = {0x20 | 24, 24};
  const std::uint8_t v_int8[] = {0x20 | 24, 0x7f};
  const std::uint8_t v_int16[] = {0x20 | 25, 0x7f, 0xff};
  const std::uint8_t v_int32[] = {0x20 | 26, 0x7f, 0xff, 0xff, 0xff};
  const std::uint8_t v_int64[] = {0x20 | 27, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


  EXPECT_TRUE(test_integer(0, v_0));
  EXPECT_TRUE(test_integer(23, v_23));
  EXPECT_TRUE(test_integer(24, v_24));
  EXPECT_TRUE(test_integer(limits<std::uint8_t>::max(), v_uint8));
  EXPECT_TRUE(test_integer(limits<std::uint16_t>::max(), v_uint16));
  EXPECT_TRUE(test_integer(limits<std::uint32_t>::max(), v_uint32));
  EXPECT_TRUE(test_integer(limits<std::uint64_t>::max(), v_uint64));
  EXPECT_TRUE(test_integer(-1, v_n1));
  EXPECT_TRUE(test_integer(-24, v_n24));
  EXPECT_TRUE(test_integer(-25, v_n25));
  EXPECT_TRUE(test_integer(limits<std::int8_t>::min(), v_int8));
  EXPECT_TRUE(test_integer(limits<std::int16_t>::min(), v_int16));
  EXPECT_TRUE(test_integer(limits<std::int32_t>::min(), v_int32));
  EXPECT_TRUE(test_integer(limits<std::int64_t>::min(), v_int64));
}
