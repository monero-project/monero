// Copyright (c) 2020-2022, The Monero Project

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
#include <sstream>

#include "memwipe.h"
#include "mlocker.h"
#include "serde/epee_compat/keyvalue.h"
#include "serde/json/value.h"
#include "storages/serde_template_helper.h"
#include "span.h"

namespace {
  struct Data1
  {
    int16_t val;

    Data1(): val() {}
    Data1(int64_t val): val(val) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(val)
    END_KV_SERIALIZE_MAP()

    bool operator==(const Data1& other) const { return val == other.val; }
  }; // struct Data1

  struct StringData
  {
    std::string str;

    StringData(): str() {}
    StringData(std::string str): str(str) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(str)
    END_KV_SERIALIZE_MAP()

    bool operator==(const StringData& other) const { return str == other.str; }
  }; // struct StringData

  struct UnsignedData
  {
    uint64_t u64;
    uint32_t u32;
    uint16_t u16;
    uint8_t u8;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(u64)
      KV_SERIALIZE(u32)
      KV_SERIALIZE(u16)
      KV_SERIALIZE(u8)
    END_KV_SERIALIZE_MAP()

    bool operator==(const UnsignedData& other) const
    {
      return u64 == other.u64 && u32 == other.u32 && u16 == other.u16 && u8 == other.u8;
    }
  }; // struct UnsignedData

  struct Data2
  {
    int64_t i64;
    int32_t i32;
    int16_t i16;
    int8_t i8;
    UnsignedData unsign;
    double triple;
    StringData sd;
    std::vector<bool> booleans;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(i64)
      KV_SERIALIZE(i32)
      KV_SERIALIZE(i16)
      KV_SERIALIZE(i8)
      KV_SERIALIZE(unsign)
      KV_SERIALIZE(triple)
      KV_SERIALIZE(sd)
      KV_SERIALIZE(booleans)
    END_KV_SERIALIZE_MAP()

    bool operator==(const Data2& other) const {
      return i64 == other.i64 && i32 == other.i32 && i16 == other.i16 && i8 == other.i8 &&
        unsign == other.unsign && triple == other.triple && sd == other.sd && booleans == other.booleans;
    }
  }; // struct Data2

  struct int_blob
  {
    int16_t v;

    bool operator==(const int_blob& other) const {
      return v == other.v;
    }
  };

  template <size_t N>
  struct byte_blob
  {
    char buf[N];

    bool operator==(const byte_blob& other) const {
      return buf == other.buf;
    }
  };

  struct BlobData
  {
    std::string s;
    byte_blob<16> buf;
    epee::mlocked<tools::scrubbed<int_blob>> delegated;

    bool operator==(const BlobData& other) const {
      return s == other.s && buf == other.buf && (const int_blob&) delegated == (const int_blob&) other.delegated;
    }

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB(s)
      KV_SERIALIZE_VAL_POD_AS_BLOB(buf)
      KV_SERIALIZE_VAL_POD_AS_BLOB(delegated)
    END_KV_SERIALIZE_MAP()
  };

  struct DumpVisitor: serde::model::BasicVisitor
  {
    DumpVisitor(): serde::model::BasicVisitor(), depth() {}

    std::string expecting() const override final
    {
      return "anything, i'll dump it";
    }

    #define DEF_DUMP_VISITOR_METHOD(mname, tyname)                    \
      void visit_##mname(tyname value) override final                 \
      { std::cout << "visit_" #mname "(): " << value << std::endl; } \

    DEF_DUMP_VISITOR_METHOD(int64, int64_t)
    DEF_DUMP_VISITOR_METHOD(int32, int32_t)
    DEF_DUMP_VISITOR_METHOD(int16, int16_t)
    DEF_DUMP_VISITOR_METHOD(int8, int8_t)
    DEF_DUMP_VISITOR_METHOD(uint64, uint64_t)
    DEF_DUMP_VISITOR_METHOD(uint32, uint32_t)
    DEF_DUMP_VISITOR_METHOD(uint16, uint16_t)
    DEF_DUMP_VISITOR_METHOD(uint8, uint8_t)
    DEF_DUMP_VISITOR_METHOD(float64, double)
    DEF_DUMP_VISITOR_METHOD(boolean, bool)

    void visit_bytes(const serde::const_byte_span& value) override final
    {
      std::cout << "visit_bytes(): " << serde::internal::byte_span_to_string(value) << std::endl;
    }

    void visit_array(serde::optional<size_t> size_hint) override final
    {
      if (size_hint) { std::cout << "visit_array(): " << *size_hint << std::endl; }
      else { std::cout << "visit_array(): no size hint" << std::endl; }
      depth++;
    }

    void visit_end_array() override final
    {
      std::cout << "visit_end_array()" << std::endl;
      depth--;
    }

    void visit_object(serde::optional<size_t> size_hint) override final
    {
      if (size_hint) { std::cout << "visit_object(): " << *size_hint << std::endl; }
      else { std::cout << "visit_object(): no size hint" << std::endl; }
      depth++;
    }

    void visit_key(const serde::const_byte_span& value) override final
    {
      std::cout << "visit_key(): " << serde::internal::byte_span_to_string(value) << std::endl;
    }

    void visit_end_object() override final
    {
      std::cout << "visit_end_object()" << std::endl;
      depth--;
    }

    static void dump_deserializer(serde::model::Deserializer& deserializer)
    {
      DumpVisitor dv;
      do
      {
        deserializer.deserialize_any(dv);
      } while (dv.depth != 0);
    }

    size_t depth;
  }; // struct DumpVisitor
} // anonymous namespace 

#define ARRAY_STR(a) std::string(reinterpret_cast<const char*>(a), sizeof(a))

TEST(epee_serialization, bin_serialize_1)
{
  using namespace serde::epee_binary;

  static constexpr const std::uint8_t expected_binary[] =
  {
    0x01, 0x11, 0x01, 0x01, // Signature A
    0x01, 0x01, 0x02, 0x01, // Signature B
    0x01,                   // Format Version
    0x04,                   // Varint number of entries
    0x03, 'v','a', 'l',     // entry key
    0x03,                   // entry type
    0xe7, 0x07              // INT16 value of 'val'
  };

  const Data1 data = { 2023 };
  const epee::byte_slice actual_slice = epee::serialization::store_t_to_binary(data);
  const std::string expected = ARRAY_STR(expected_binary);
  const std::string actual{reinterpret_cast<const char*>(actual_slice.data()), actual_slice.size()};

  EXPECT_EQ(expected, actual);
}

TEST (epee_serialization, json_serialize_1)
{
  using namespace serde::json;

  const std::string expected_json("{\"val\":2023}");

  const Data1 data = { 2023 };
  const std::string result = epee::serialization::store_t_to_json(data);

  EXPECT_EQ(expected_json, result);
}

TEST(epee_serialization, json_escape)
{
  static const std::pair<StringData, std::string> test_cases[] =
  {
    { { "Howdy, World!" }, R"({"str":"Howdy, World!"})" },
    { { "New\nline"     }, R"({"str":"New\nline"})" },
    { { "\b\ruh"        }, R"({"str":"\b\ruh"})" },
    { { "\u1234"        }, "{\"str\":\"\u1234\"}" }, // not raw
  };

  for (const auto& test_case : test_cases) {
    const auto& input_instance = test_case.first;
    const auto& expected_json = test_case.second;

    const std::string actual_json = epee::serialization::store_t_to_json(input_instance);

    EXPECT_EQ(expected_json, actual_json);
  }
}

TEST(epee_serialization, bin_deserialize_1)
{
  static constexpr const std::uint8_t source_binary[] =
  {
    0x01, 0x11, 0x01, 0x01, // Signature A
    0x01, 0x01, 0x02, 0x01, // Signature B
    0x01,                   // Format Version
    0x04,                   // Varint number of entries
    0x03, 'v','a', 'l',     // entry key
    0x03,                   // entry type
    0xe7, 0x07              // INT16 value of 'val'
  };

  Data1 deserialized_data;
  EXPECT_TRUE(epee::serialization::load_t_from_binary(deserialized_data, source_binary));
  const Data1 expected_data = { 2023 };
  EXPECT_EQ(expected_data, deserialized_data);
}

TEST(epee_serialization, json_deserialize_1)
{
  using namespace serde::json;

  Data1 deserialized_data;
  std::string json_src = "{\"val\":7777}";
  EXPECT_TRUE(epee::serialization::load_t_from_json(deserialized_data, json_src));
  const Data1 expected_data = { 7777 };
  EXPECT_EQ(expected_data, deserialized_data);
}

TEST(epee_serialization, json_deserialize_2)
{
  std::string json_data = R"({
    "i8": -5, "i16": -6, "i32": -7, "i64": -8,
    "unsign": { "u64": 1, "u32": 2, "u16": 3, "u8": 4 },
    "triple": 20.23,
    "sd": { "str": "meep meep"},
    "booleans": [true, false, true, true, false, true, false, false]
  })";

  const Data2 expected = { -8, -7, -6, -5, { 1, 2, 3, 4 }, 20.23, { "meep meep" }, { true, false, true, true, false, true, false, false } };

  Data2 actual;
  EXPECT_TRUE(epee::serialization::load_t_from_json(actual, json_data));
  EXPECT_EQ(expected, actual);
}

TEST(epee_serialization, json_value_deserializer)
{
  using namespace serde::json;

  std::string json_data = R"({
    "i8": -5, "i16": -6, "i32": -7, "i64": -8,
    "unsign": { "u64": 1, "u32": 2, "u16": 3, "u8": 4 },
    "triple": 20.23,
    "sd": { "str": "meep meep"},
    "booleans": [true, false, true, true, false, true, false, false]
  })";

  const Data2 expected = { -8, -7, -6, -5, { 1, 2, 3, 4 }, 20.23, { "meep meep" }, { true, false, true, true, false, true, false, false } };

  const Document d = parse_borrowed_document_from_cstr(&json_data.front());
  
  // dump ValueDeserializer trace
  //ValueDeserializer vd1(d);
  //dump_deserializer(vd1);

  Data2 actual;
  ValueDeserializer vd2(d);
  deserialize_default(vd2, actual);
  EXPECT_EQ(expected, actual);
}

TEST(epee_serialization, blob_binary_serialize)
{
  tools::scrubbed<int_blob> scrubbed_data;
  ((int_blob&) scrubbed_data) = { 7784 };

  const BlobData blob_src =
  {
    "Jeffro",
    { "123456789ABCDEF" },
    { scrubbed_data }
  };

  static constexpr uint8_t expected_binary[] =
  {
    0x01, 0x11, 0x01, 0x01,
    0x01, 0x01, 0x02, 0x01,
    0x01,
    0x0C,
    0x01, 's',
    0X0A,
    0x18, 'J', 'e', 'f', 'f', 'r', 'o',
    0x03, 'b', 'u', 'f',
    0x0A, 
    0x40, '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '\0',
    0x09, 'd', 'e', 'l', 'e', 'g', 'a', 't', 'e', 'd',
    0x0A,
    0x08, 0x68, 0x1E
  };
  const std::string expected{reinterpret_cast<const char*>(expected_binary), sizeof(expected_binary)};

  const epee::byte_slice actual_slice = epee::serialization::store_t_to_binary(blob_src);
  std::string actual;
  actual.assign(reinterpret_cast<const char*>(actual_slice.begin()), actual_slice.size());
  EXPECT_EQ(expected, actual);
}
