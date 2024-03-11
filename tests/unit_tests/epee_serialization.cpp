// Copyright (c) 2020-2023, The Monero Project

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

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_template_helper.h"
#include "span.h"

TEST(epee_binary, two_keys)
{
  static constexpr const std::uint8_t data[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x08, 0x01, 'a',
    0x0B, 0x00, 0x01, 'b', 0x0B, 0x00
  };

  epee::serialization::portable_storage storage{};
  EXPECT_TRUE(storage.load_from_binary(data));
}

TEST(epee_binary, duplicate_key)
{
  static constexpr const std::uint8_t data[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x08, 0x01, 'a',
    0x0B, 0x00, 0x01, 'a', 0x0B, 0x00
  };

  epee::serialization::portable_storage storage{};
  EXPECT_FALSE(storage.load_from_binary(data));
}

namespace
{
struct ObjOfObjs
{
  std::vector<ObjOfObjs> x;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(x)
  END_KV_SERIALIZE_MAP()
};

struct ObjOfInts
{
  std::list<int> x;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(x)
  END_KV_SERIALIZE_MAP()
};
}

TEST(epee_binary, any_empty_seq)
{
  // Test that any c++ sequence type (std::vector, std::list, etc) can deserialize without error
  // from an *empty* epee binary array of any type. This property is useful for other projects who
  // maintain code which serializes epee binary but don't know the type of arrays until runtime.
  // Without any elements to actually serialize, they don't know what type code to use for arrays
  // and should be able to choose a default typecode.

  static constexpr const std::uint8_t data_empty_bool[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x04, 0x01, 'x', 0x8B /*array of bools*/, 0x00 /*length 0*/
  };

  static constexpr const std::uint8_t data_empty_double[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x04, 0x01, 'x', 0x89 /*array of doubles*/, 0x00 /*length 0*/
  };

  static constexpr const std::uint8_t data_empty_string[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x04, 0x01, 'x', 0x8A /*array of strings*/, 0x00 /*length 0*/
  };

  static constexpr const std::uint8_t data_empty_int64[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x04, 0x01, 'x', 0x81 /*array of int64s*/, 0x00 /*length 0*/
  };

  static constexpr const std::uint8_t data_empty_object[] = {
    0x01, 0x11, 0x01, 0x1, 0x01, 0x01, 0x02, 0x1, 0x1, 0x04, 0x01, 'x', 0x8C /*array of objects*/, 0x00 /*length 0*/
  };

  ObjOfObjs o;

  EXPECT_TRUE(epee::serialization::load_t_from_binary(o, epee::span<const std::uint8_t>(data_empty_bool)));
  EXPECT_EQ(0, o.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(o, epee::span<const std::uint8_t>(data_empty_double)));
  EXPECT_EQ(0, o.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(o, epee::span<const std::uint8_t>(data_empty_string)));
  EXPECT_EQ(0, o.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(o, epee::span<const std::uint8_t>(data_empty_int64)));
  EXPECT_EQ(0, o.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(o, epee::span<const std::uint8_t>(data_empty_object)));
  EXPECT_EQ(0, o.x.size());

  ObjOfInts i;

  EXPECT_TRUE(epee::serialization::load_t_from_binary(i, epee::span<const std::uint8_t>(data_empty_bool)));
  EXPECT_EQ(0, i.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(i, epee::span<const std::uint8_t>(data_empty_double)));
  EXPECT_EQ(0, i.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(i, epee::span<const std::uint8_t>(data_empty_string)));
  EXPECT_EQ(0, i.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(i, epee::span<const std::uint8_t>(data_empty_int64)));
  EXPECT_EQ(0, i.x.size());

  EXPECT_TRUE(epee::serialization::load_t_from_binary(i, epee::span<const std::uint8_t>(data_empty_object)));
  EXPECT_EQ(0, i.x.size());
}
