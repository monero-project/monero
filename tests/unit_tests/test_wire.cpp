// Copyright (c) 2022, The Monero Project
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

#include <gtest/gtest.h>

#include <algorithm>
#include <boost/core/demangle.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/variant/get.hpp>
#include <cstdint>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "common/expect.h"
#include "hex.h"
#include "serialization/wire.h"
#include "serialization/wire/adapted/static_vector.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/basic_value.h"
#include "serialization/wire/epee.h"
#include "serialization/wire/json.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrapper/array_blob.h"
#include "serialization/wire/wrapper/defaulted.h"
#include "serialization/wire/wrapper/variant.h"
#include "serialization/wire/wrappers_impl.h"
#include "span.h"

namespace
{
  template<typename Target>
  void test_unsigned_to_unsigned()
  {
    using limit = std::numeric_limits<Target>;
    static constexpr const auto max =
      std::numeric_limits<std::uintmax_t>::max();
    static_assert(limit::is_integer, "expected integer");
    static_assert(!limit::is_signed, "expected unsigned");

    SCOPED_TRACE("uintmax_t to " + boost::core::demangle(typeid(Target).name()));

    EXPECT_EQ(Target(0), wire::integer::cast_unsigned<Target>(std::uintmax_t(0)));
    EXPECT_EQ(limit::max(), wire::integer::cast_unsigned<Target>(std::uintmax_t(limit::max())));
    if (limit::max() < max)
    {
      EXPECT_THROW(wire::integer::cast_unsigned<Target>(std::uintmax_t(limit::max()) + 1), wire::exception);
      EXPECT_THROW(wire::integer::cast_unsigned<Target>(max), wire::exception);
    }
  }

  template<typename Target>
  void test_signed_to_signed()
  {
    using limit = std::numeric_limits<Target>;
    static constexpr const auto min =
      std::numeric_limits<std::intmax_t>::min();
    static constexpr const auto max =
      std::numeric_limits<std::intmax_t>::max();
    static_assert(limit::is_integer, "expected integer");
    static_assert(limit::is_signed, "expected signed");

    SCOPED_TRACE("intmax_t to " + boost::core::demangle(typeid(Target).name()));
    if (min < limit::min())
    {
      EXPECT_THROW(wire::integer::cast_signed<Target>(std::intmax_t(limit::min()) - 1), wire::exception);
      EXPECT_THROW(wire::integer::cast_signed<Target>(min), wire::exception);
    }
    EXPECT_EQ(limit::min(), wire::integer::cast_signed<Target>(std::intmax_t(limit::min())));
    EXPECT_EQ(Target(0), wire::integer::cast_signed<Target>(std::intmax_t(0)));
    EXPECT_EQ(limit::max(), wire::integer::cast_signed<Target>(std::intmax_t(limit::max())));
    if (limit::max() < max)
    {
      EXPECT_THROW(wire::integer::cast_signed<Target>(std::intmax_t(limit::max()) + 1), wire::exception);
      EXPECT_THROW(wire::integer::cast_signed<Target>(max), wire::exception);
    }
  }
}


TEST(wire, integer_convert_to)
{
  {
    SCOPED_TRACE("unsigned to unsigned");
    test_unsigned_to_unsigned<std::uint8_t>();
    test_unsigned_to_unsigned<std::uint16_t>();
    test_unsigned_to_unsigned<std::uint32_t>();
    test_unsigned_to_unsigned<std::uint64_t>();
    test_unsigned_to_unsigned<std::uintmax_t>();
  }
  {
    SCOPED_TRACE("signed to signed");
    test_signed_to_signed<std::int8_t>();
    test_signed_to_signed<std::int16_t>();
    test_signed_to_signed<std::int32_t>();
    test_signed_to_signed<std::int64_t>();
    test_signed_to_signed<std::intmax_t>();
  }
}

namespace
{
  template<typename T>
  using limit = std::numeric_limits<T>;

  struct small_blob { std::uint8_t buf[4]; };
  constexpr const small_blob blob_test1{{0x00, 0xFF, 0x22, 0x11}};
  constexpr const small_blob blob_test2{{0x11, 0x7F, 0x7E, 0x80}};
  constexpr const small_blob blob_test3{{0xDE, 0xAD, 0xBE, 0xEF}};

  inline bool operator==(const small_blob& lhs, const small_blob& rhs)
  {
    return std::memcmp(lhs.buf, rhs.buf, sizeof(lhs.buf)) == 0;
  }
  inline bool operator!=(const small_blob& lhs, const small_blob& rhs)
  {
    return !(lhs == rhs);
  }

  template<std::size_t N>
  inline bool operator==(const boost::container::static_vector<std::uint8_t, N>& lhs, const small_blob& rhs)
  {
    return boost::range::equal(lhs, rhs.buf);
  }

  struct inner
  {
    std::uint32_t left;
    std::uint32_t right;

    WIRE_BEGIN_MAP_ASM_SIZE(),
      WIRE_FIELD(left),
      WIRE_FIELD(right)
    WIRE_END_MAP()
  };

  struct variant
  {
    WIRE_DEFINE_CONVERSIONS()
    boost::variant<int, std::string> choice;
  };

  template<typename F, typename T>
  void variant_map(F& format, T& self)
  {
    auto wrapped = wire::variant(std::ref(self.choice));
    wire::object(format,
      WIRE_OPTION("a_string", std::string, wrapped),
      WIRE_OPTION("a_int",    int,         wrapped)
    );
  }
  WIRE_DEFINE_OBJECT(variant, variant_map);

  using max_1 = wire::max_element_count<1>;
  struct complex
  {
    using max_2 = wire::max_element_count<2>;
    using max_4 = wire::max_element_count<4>;

    std::vector<inner> objects;
    boost::container::static_vector<std::int16_t, 4> ints;
    std::vector<std::vector<std::vector<std::uint64_t>>> uints;
    std::vector<small_blob> blobs;
    std::vector<small_blob> vector_blobs;
    std::list<small_blob> list_blobs;
    std::vector<std::string> strings;
    std::string string;
    wire::basic_value any;
    variant string_or_int;
    double real;
    std::uint8_t uint8;
    std::int8_t int8;
    bool choice;

    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP(),
      WIRE_FIELD_ARRAY(objects, wire::max_element_count<3>),
      WIRE_FIELD(ints),
      wire::field("uints", wire::array<max_1>(wire::array<max_2>(wire::array<max_4>(std::ref(self.uints))))),
      WIRE_FIELD(blobs),
      WIRE_FIELD_ARRAY_AS_BLOB(vector_blobs),
      WIRE_FIELD_ARRAY_AS_BLOB(list_blobs),
      WIRE_FIELD_ARRAY(strings, wire::min_element_size<7>),
      WIRE_FIELD(string),
      WIRE_OPTIONAL_FIELD(any),
      WIRE_FIELD(string_or_int),
      WIRE_FIELD(real),
      WIRE_FIELD_DEFAULTED(uint8, 100u),
      WIRE_FIELD(int8),
      WIRE_FIELD(choice)
    WIRE_END_MAP()
  };

  template<std::size_t G, std::size_t H, std::size_t I, std::size_t J, std::size_t X, std::size_t Y, std::size_t Z>
  struct constraints
  {
    using max_b = wire::max_element_count<H>;
    using max_c = wire::max_element_count<I>;

    std::set<std::int16_t> ints;
    std::vector<std::vector<std::vector<std::uint64_t>>> uints;
    boost::container::static_vector<boost::container::static_vector<std::uint8_t, J>, X> blobs;
    std::vector<std::uint8_t> vector_blobs;
    boost::container::static_vector<boost::container::static_vector<char, Y>, Z> strings;

    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP_ASM_SIZE(),
      WIRE_FIELD_ARRAY(ints, wire::max_element_count<G>),
      wire::optional_field("uints", wire::array<max_1>(wire::array<max_b>(wire::array<max_c>(std::ref(self.uints))))),
      WIRE_FIELD(blobs),
      WIRE_FIELD_DEFAULTED(vector_blobs, std::vector<std::uint8_t>{}),
      WIRE_FIELD(strings)
    WIRE_END_MAP()
  };

  template<std::size_t N>
  struct min_size
  {
    std::vector<std::string> strings;

    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP(),
      WIRE_FIELD_ARRAY(strings, wire::min_element_size<N>)
    WIRE_END_MAP()
  };

  struct required_array
  {
    std::vector<std::int16_t> ints;

    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP(),
      wire::field("ints", wire::array<wire::max_element_count<4>>(std::ref(self.ints)))
    WIRE_END_MAP()
  };

  struct skipped { bool choice; };

  template<typename F, typename T>
  void skipped_map(F& format, T& self)
  { wire::object(format, WIRE_FIELD(choice)); }

  WIRE_DEFINE_OBJECT(skipped, skipped_map)
  WIRE_EPEE_DEFINE_CONVERSION(skipped)
  WIRE_JSON_DEFINE_CONVERSION(skipped)

  void verify_initial(const complex& self)
  {
    EXPECT_TRUE(self.objects.empty());
    EXPECT_TRUE(self.ints.empty());
    EXPECT_TRUE(self.uints.empty());
    EXPECT_TRUE(self.blobs.empty());
    EXPECT_TRUE(self.vector_blobs.empty());
    EXPECT_TRUE(self.list_blobs.empty());
    EXPECT_TRUE(self.strings.empty());
    EXPECT_TRUE(self.string.empty());
    EXPECT_FALSE(bool(self.any));
    EXPECT_EQ(0u, boost::get<int>(self.string_or_int.choice));
    EXPECT_DOUBLE_EQ(self.real, 0.0);
    EXPECT_EQ(self.uint8, 0);
    EXPECT_EQ(self.int8, 0);
    EXPECT_FALSE(self.choice);
  }

  template<typename T>
  void verify_initial(const T& self)
  {
    EXPECT_TRUE(self.ints.empty());
    EXPECT_TRUE(self.uints.empty());
    EXPECT_TRUE(self.blobs.empty());
    EXPECT_TRUE(self.strings.empty());
    EXPECT_TRUE(self.vector_blobs.empty());
  }

  void fill(complex& self)
  {
    self.objects = {inner{0, limit<std::uint32_t>::max()}, inner{100, 200}, inner{44444, 83434}};
    self.ints = {limit<std::int16_t>::min(), 0, 31234, limit<std::int16_t>::max()};
    self.uints = {{{0, limit<std::uint64_t>::max(), 34234234, 33}, {977}}};
    self.blobs = {blob_test1, blob_test2, blob_test3};
    self.vector_blobs = self.blobs;
    boost::range::copy(self.blobs, std::back_inserter(self.list_blobs));
    self.strings = {"string1", "string2", "string3", "string4"};
    self.string = "simple_string";
    self.any.value = limit<std::intmax_t>::min();
    self.string_or_int.choice = std::string{"variant_string"};
    self.real = 2.43;
    self.uint8 = limit<std::uint8_t>::max();
    self.int8 = limit<std::int8_t>::min();
    self.choice = true;
  }

  template<typename T>
  void verify_ints(const T& self)
  {
    ASSERT_EQ(self.size(), 4);
    auto it = self.begin();
    EXPECT_EQ(*it++, limit<std::int16_t>::min());
    EXPECT_EQ(*it++, 0);
    EXPECT_EQ(*it++, 31234);
    EXPECT_EQ(*it++, limit<std::int16_t>::max());
    EXPECT_EQ(it, self.end());
  }

  void verify_uints(const std::vector<std::vector<std::vector<std::uint64_t>>>& self)
  {
    ASSERT_EQ(self.size(), 1);
    ASSERT_EQ(self.at(0).size(), 2);
    ASSERT_EQ(self.at(0).at(0).size(), 4);
    EXPECT_EQ(self.at(0).at(0).at(0), 0);
    EXPECT_EQ(self.at(0).at(0).at(1), limit<std::uint64_t>::max());
    EXPECT_EQ(self.at(0).at(0).at(2), 34234234);
    EXPECT_EQ(self.at(0).at(0).at(3), 33);
    ASSERT_EQ(self.at(0).at(1).size(), 1);
    EXPECT_EQ(self.at(0).at(1).at(0), 977);
  }

  void verify_blobs(const std::vector<small_blob>& self)
  {
    ASSERT_EQ(self.size(), 3);
    EXPECT_EQ(self.at(0), blob_test1);
    EXPECT_EQ(self.at(1), blob_test2);
    EXPECT_EQ(self.at(2), blob_test3);
  }

  template<typename T>
  void verify_strings(const T& self)
  {
    ASSERT_EQ(self.size(), 4);
    EXPECT_TRUE(boost::range::equal(self.at(0), std::string{"string1"}));
    EXPECT_TRUE(boost::range::equal(self.at(1), std::string{"string2"}));
    EXPECT_TRUE(boost::range::equal(self.at(2), std::string{"string3"}));
    EXPECT_TRUE(boost::range::equal(self.at(3), std::string{"string4"}));
  }

  void verify_filled(const complex& self)
  {
    ASSERT_EQ(self.objects.size(), 3);
    EXPECT_EQ(self.objects.at(0).left, 0);
    EXPECT_EQ(self.objects.at(0).right, limit<std::uint32_t>::max());
    EXPECT_EQ(self.objects.at(1).left, 100);
    EXPECT_EQ(self.objects.at(1).right, 200);
    EXPECT_EQ(self.objects.at(2).left, 44444);
    EXPECT_EQ(self.objects.at(2).right, 83434);

    verify_ints(self.ints);
    verify_uints(self.uints);
    verify_blobs(self.blobs);
    verify_blobs(self.vector_blobs);

    ASSERT_EQ(self.list_blobs.size(), 3);
    EXPECT_TRUE(boost::range::equal(self.blobs, self.list_blobs));

    verify_strings(self.strings);

    EXPECT_EQ(self.string, "simple_string");
    EXPECT_TRUE(bool(self.any));
    EXPECT_EQ(limit<std::intmax_t>::min(), boost::get<std::intmax_t>(self.any.value));
    EXPECT_EQ("variant_string", boost::get<std::string>(self.string_or_int.choice));
    EXPECT_DOUBLE_EQ(self.real, 2.43);
    EXPECT_EQ(self.uint8, limit<std::uint8_t>::max());
    EXPECT_EQ(self.int8, limit<std::int8_t>::min());

    EXPECT_TRUE(self.choice);
  }

  template<typename T>
  void verify_filled(const T& self)
  {
    verify_ints(self.ints);
    verify_uints(self.uints);
    {
      std::vector<small_blob> vector_blobs;
      wire::read_as_blob(epee::to_span(self.vector_blobs), vector_blobs);
      verify_blobs(vector_blobs);
    }
    verify_strings(self.strings);
  }

  template<typename T, typename U, typename V>
  bool check_error(const V& source, const std::error_code expected)
  {
    SCOPED_TRACE("Constraint test for " + boost::core::demangle(typeid(U).name()));

    U dest{};
    const std::error_code error = T::from_bytes(epee::to_span(source), dest);
    EXPECT_EQ(error, expected);
    return true;
  }

  void copy_buffer(std::string& out, const std::string& in)
  { out = in; }

  void copy_buffer(epee::byte_stream& out, const epee::byte_stream& in)
  { out.write(in.data(), in.size()); }

  template<typename T, typename U>
  void run_complex(U& buffer)
  {
    SCOPED_TRACE("Complex test for " + boost::core::demangle(typeid(T).name()));
    complex base{};
    verify_initial(base);

    U initial_buffer;
    {
      SCOPED_TRACE("Empty complex data structure");

      buffer.clear();
      std::error_code error = T::to_bytes(buffer, base);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
      copy_buffer(initial_buffer, buffer);

      complex derived{};
      fill(derived);
      verify_filled(derived);

      error = T::from_bytes(epee::to_span(buffer), derived);
      ASSERT_FALSE(error);
      verify_initial(derived);
    }

    fill(base);
    verify_filled(base);

    {
      SCOPED_TRACE("Filled complex data structure");

      buffer.clear();
      std::error_code error = T::to_bytes(buffer, base);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
      const std::size_t buffer_size = buffer.size();

      complex derived{};
      error = T::from_bytes(epee::to_span(buffer), derived);
      ASSERT_FALSE(error);
      verify_filled(derived);

      skipped skip{};
      EXPECT_FALSE(skip.choice);
      error = T::from_bytes(epee::to_span(buffer), skip);
      ASSERT_FALSE(error);
      EXPECT_TRUE(skip.choice);

      {
        SCOPED_TRACE("Required field/array");

        check_error<T, required_array>(initial_buffer, wire::error::schema::missing_key);
        check_error<T, required_array>(buffer, std::error_code{});

        required_array array{};
        EXPECT_TRUE(array.ints.empty());
        error = T::from_bytes(epee::to_span(buffer), array);
        ASSERT_FALSE(error);
        verify_ints(array.ints);
      }
      {
        SCOPED_TRACE("Array constraints");

        constraints<4, 2, 4, 5, 4, 9, 5> base2{};
        verify_initial(base2);
        error = T::from_bytes(epee::to_span(buffer), base2);
        ASSERT_FALSE(error);
        verify_filled(base2);

        buffer.clear();
        error = T::to_bytes(buffer, base2);
        ASSERT_FALSE(error);
        EXPECT_LT(0u, buffer.size());

        constraints<4, 2, 4, 4, 3, 7, 4> derived2{};
        verify_initial(derived2);
        error = T::from_bytes(epee::to_span(buffer), derived2);
        ASSERT_FALSE(error);
        verify_filled(derived2);

        error = T::from_bytes(epee::to_span(initial_buffer), derived2);
        ASSERT_FALSE(error);
        verify_initial(derived2);

        static constexpr const std::size_t min = std::is_same<T, wire::json>::value ? 10 : 9;
        static constexpr const wire::error::schema string_error =
          std::is_same<T, wire::epee_bin>::value ?
          wire::error::schema::binary : wire::error::schema::string;

        check_error<T, min_size<min - 1>>(buffer, std::error_code{});
        check_error<T, min_size<min>>(initial_buffer, std::error_code{});
        check_error<T, min_size<min>>(buffer, wire::error::schema::array_min_size);

        check_error<T, constraints<4, 2, 4, 4, 3, 7, 4>>(buffer, std::error_code{});
        check_error<T, constraints<1, 1, 1, 1, 1, 1, 1>>(initial_buffer, std::error_code{});
        check_error<T, constraints<3, 2, 4, 4, 3, 7, 4>>(buffer, wire::error::schema::array_max_element);
        check_error<T, constraints<4, 1, 4, 4, 3, 7, 4>>(buffer, wire::error::schema::array_max_element);
        check_error<T, constraints<4, 2, 3, 4, 3, 7, 4>>(buffer, wire::error::schema::array_max_element);
        check_error<T, constraints<4, 2, 4, 3, 3, 7, 4>>(buffer, wire::error::schema::binary);
        check_error<T, constraints<4, 2, 4, 4, 2, 7, 4>>(buffer, wire::error::schema::array_max_element);
        check_error<T, constraints<4, 2, 4, 4, 3, 6, 4>>(buffer, string_error);
        check_error<T, constraints<4, 2, 4, 4, 3, 7, 3>>(buffer, wire::error::schema::array_max_element);
      }
      {
        SCOPED_TRACE("Testing defaulted value");

        buffer.clear();
        error = T::to_bytes(buffer, base);
        ASSERT_FALSE(error);
        EXPECT_EQ(buffer.size(), buffer_size);

        EXPECT_NE(100u, base.uint8);
        base.uint8 = 100u; // defaulted value
        buffer.clear();
        error = T::to_bytes(buffer, base);
        ASSERT_FALSE(error);
        EXPECT_LT(0u, buffer.size());
        EXPECT_LT(buffer.size(), buffer_size);

        EXPECT_NE(100u, derived.uint8);
        error = T::from_bytes(epee::to_span(buffer), derived);
        ASSERT_FALSE(error);
        EXPECT_EQ(100u, derived.uint8);
      }

      buffer.clear();
      error = T::to_bytes(buffer, skip);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
      EXPECT_LT(buffer.size(), buffer_size);
    }
  }

  struct duplicate_key
  {
    std::uint16_t value;
    std::uint16_t value2;
    
    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP(),
      WIRE_FIELD(value),
      wire::field("value", std::ref(self.value2))
    WIRE_END_MAP()
  };

  template<typename T, typename U>
  void run_duplicate_key(U& buffer)
  {
    SCOPED_TRACE("Duplicate key for " + boost::core::demangle(typeid(T).name()));

    buffer.clear();
    {
      const duplicate_key data{};
      const std::error_code error = T::to_bytes(buffer, data);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }

    check_error<T, duplicate_key>(buffer, wire::error::schema::invalid_key);
  }

  template<typename X>
  struct single
  {
    X value;

    WIRE_DEFINE_CONVERSIONS()
    WIRE_BEGIN_MAP(),
      WIRE_FIELD(value)
    WIRE_END_MAP()
  };

  template<typename T, typename U>
  void run_basic_value(U& buffer)
  {
    SCOPED_TRACE("basic_value test for " + boost::core::demangle(typeid(T).name()));

    buffer.clear();
    U duplicate;
    {
      buffer.clear();
      const single<bool> boolean{true};
      const std::error_code error = T::to_bytes(buffer, boolean);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }
    {
      single<wire::basic_value> any;
      std::error_code error = T::from_bytes(epee::to_span(buffer), any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(bool(any.value));
      EXPECT_TRUE(boost::get<bool>(any.value.value));

      duplicate.clear();
      error = T::to_bytes(duplicate, any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(boost::range::equal(epee::to_span(duplicate), epee::to_span(buffer)));
    }
    {
      buffer.clear();
      const single<std::uint64_t> number{limit<std::uint64_t>::max()};
      const std::error_code error = T::to_bytes(buffer, number);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }
    {
      single<wire::basic_value> any;
      std::error_code error = T::from_bytes(epee::to_span(buffer), any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(bool(any.value));
      EXPECT_EQ(limit<std::uintmax_t>::max(), boost::get<std::uintmax_t>(any.value.value));

      duplicate.clear();
      error = T::to_bytes(duplicate, any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(boost::range::equal(epee::to_span(duplicate), epee::to_span(buffer)));
    }
    {
      buffer.clear();
      const single<std::int64_t> number{limit<std::int64_t>::min()};
      const std::error_code error = T::to_bytes(buffer, number);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }
    {
      single<wire::basic_value> any;
      std::error_code error = T::from_bytes(epee::to_span(buffer), any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(bool(any.value));
      EXPECT_EQ(limit<std::intmax_t>::min(), boost::get<std::intmax_t>(any.value.value));

      duplicate.clear();
      error = T::to_bytes(duplicate, any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(boost::range::equal(epee::to_span(duplicate), epee::to_span(buffer)));
    }
    {
      buffer.clear();
      const single<double> number{5.433};
      const std::error_code error = T::to_bytes(buffer, number);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }
    {
      single<wire::basic_value> any;
      std::error_code error = T::from_bytes(epee::to_span(buffer), any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(bool(any.value));
      EXPECT_DOUBLE_EQ(5.433, boost::get<double>(any.value.value));

      duplicate.clear();
      error = T::to_bytes(duplicate, any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(boost::range::equal(epee::to_span(duplicate), epee::to_span(buffer)));
    }
    {
      buffer.clear();
      const single<std::string> string{"the string"};
      const std::error_code error = T::to_bytes(buffer, string);
      ASSERT_FALSE(error);
      EXPECT_LT(0u, buffer.size());
    }
    {
      single<wire::basic_value> any;
      std::error_code error = T::from_bytes(epee::to_span(buffer), any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(bool(any.value));
      EXPECT_EQ("the string", boost::get<std::string>(any.value.value));

      duplicate.clear();
      error = T::to_bytes(duplicate, any);
      ASSERT_FALSE(error);
      EXPECT_TRUE(boost::range::equal(epee::to_span(duplicate), epee::to_span(buffer)));
    }
  }

  template<typename T, typename U>
  expect<single<std::int32_t>> round_trip(U& buffer, const std::int64_t value)
  {
    single<std::int32_t> out{0};
    {
      SCOPED_TRACE("Testing round-trip with " + std::to_string(value));

      buffer.clear();
      std::error_code error = T::to_bytes(buffer, single<std::int64_t>{value});
      if (error)
        return error;
      error = T::from_bytes(epee::to_span(buffer), out);
      if (error)
        return error;
    }
    return out;
  }

  template<typename T, typename U>
  void not_overflow(U& buffer, const std::int64_t value)
  {
    const expect<single<std::int32_t>> result = round_trip<T>(buffer, value);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value, value);
  }

  template<typename T, typename U>
   void overflow(U& buffer, const std::int64_t value, const std::error_code error)
  {
    const expect<single<std::int32_t>> result = round_trip<T>(buffer, value);
    EXPECT_EQ(result, error);
  }

  template<typename T, typename U>
  void run_overflow(U& buffer)
  {
    SCOPED_TRACE("Overflow test for " + boost::core::demangle(typeid(T).name()));
    {
      not_overflow<T>(buffer, limit<std::int32_t>::min());
      not_overflow<T>(buffer, 0);
      not_overflow<T>(buffer, limit<std::int32_t>::max());

      overflow<T>(buffer, limit<std::int64_t>::min(), wire::error::schema::larger_integer);
      overflow<T>(buffer, std::int64_t(limit<std::int32_t>::min()) - 1, wire::error::schema::larger_integer);
      overflow<T>(buffer, limit<std::int64_t>::max(), wire::error::schema::smaller_integer);
      overflow<T>(buffer, std::int64_t(limit<std::int32_t>::max()) + 1, wire::error::schema::smaller_integer);
    }
  }

  template<typename T, typename U, std::size_t... I, typename V, typename... X>
  bool run_schema_check4(U& buffer, const std::index_sequence<I...> sequence, const boost::fusion::pair<V, wire::error::schema> base, const std::tuple<boost::fusion::pair<X, wire::error::schema>...> types)
  {
    SCOPED_TRACE("Schema check for " + boost::core::demangle(typeid(V).name()) + ": " + get_string(base.second));

    buffer.clear();
    const single<V> source{};
    const std::error_code error = T::to_bytes(buffer, source);
    EXPECT_FALSE(error);
    if (error)
      return false;
    EXPECT_LT(0u, buffer.size());

    const bool same[] = {std::get<I>(types).second == base.second...};
    const std::error_code errors[] =
      {same[I] ? std::error_code{} : std::error_code{std::get<I>(types).second}...};

    wire::sum(check_error<T, single<X>>(buffer, errors[I])...);
    return true;
  }

  template<typename T, typename U, std::size_t... I, typename... V>
  void run_schema_check3(U& buffer, const std::index_sequence<I...> sequence, const std::tuple<boost::fusion::pair<V, wire::error::schema>...> types)

  {
    wire::sum(run_schema_check4<T>(buffer, sequence, std::get<I>(types), types)...);
  }

  template<typename T, typename U, typename... V>
  void run_schema_check2(U& buffer, const boost::fusion::pair<V, wire::error::schema>... types)
  {
    run_schema_check3<T>(buffer, std::make_index_sequence<sizeof...(V)>{}, std::make_tuple(types...));
  }

  template<typename T, typename U>
  void run_schema_check(U& buffer)
  {
    SCOPED_TRACE("Schema check for " + boost::core::demangle(typeid(T).name()));

    run_schema_check2<T>(buffer,
      boost::fusion::make_pair<bool>(wire::error::schema::boolean),
      boost::fusion::make_pair<std::uint32_t>(wire::error::schema::integer),
      boost::fusion::make_pair<std::string>(wire::error::schema::string),
      boost::fusion::make_pair<wire::array_<std::vector<int>, wire::max_element_count<1>>>(wire::error::schema::array),
      boost::fusion::make_pair<inner>(wire::error::schema::object)
    );
  }
}

WIRE_DECLARE_BLOB(small_blob)

TEST(wire, readers_writers)
{
  {
    std::string buffer;
    run_complex<wire::json>(buffer);
    run_duplicate_key<wire::json>(buffer);
    run_basic_value<wire::json>(buffer);
    run_overflow<wire::json>(buffer);
    run_schema_check<wire::json>(buffer);
  }

  {
    epee::byte_stream buffer;
    run_complex<wire::epee_bin>(buffer);
    run_duplicate_key<wire::epee_bin>(buffer);
    run_basic_value<wire::epee_bin>(buffer);
    run_overflow<wire::epee_bin>(buffer);
    run_schema_check<wire::epee_bin>(buffer);
  }
}

namespace
{
  constexpr const char bad_variant_json[] = u8"{\"a_int\":0,\"a_string\":\"\"}";
  constexpr const char initial_json[] =
    u8"{\"uints\":[],\"string\":\"\",\"string_or_int\":{\"a_int\":0},\"real\":0.0,\"uint8\":0,\"int8\":0,\"choice\":false}";
  constexpr const char filled_json[] =
    u8"{"
        "\"objects\":[{\"left\":0,\"right\":4294967295},{\"left\":100,\"right\":200},{\"left\":44444,\"right\":83434}],"
        "\"ints\":[-32768,0,31234,32767],"
        "\"uints\":[[[0,18446744073709551615,34234234,33],[977]]],"
        "\"blobs\":[\"00ff2211\",\"117f7e80\",\"deadbeef\"],"
        "\"vector_blobs\":\"00ff2211117f7e80deadbeef\","
        "\"list_blobs\":\"00ff2211117f7e80deadbeef\","
        "\"strings\":[\"string1\",\"string2\",\"string3\",\"string4\"],"
        "\"string\":\"simple_string\","
        "\"any\":-9223372036854775808,"
        "\"string_or_int\":{\"a_string\":\"variant_string\"},"
        "\"real\":2.43,"
        "\"uint8\":255,"
        "\"int8\":-128,"
        "\"choice\":true"
      "}";
}

// Ensures correctness of all templated tests above
TEST(wire, json_writer)
{
  complex data{};
  verify_initial(data);

  std::string buffer;
  std::error_code error = wire::json::to_bytes(buffer, data);
  ASSERT_FALSE(error);
  EXPECT_EQ(initial_json, buffer);

  fill(data);
  verify_filled(data);

  buffer.clear();
  error = wire::json::to_bytes(buffer, data);
  ASSERT_FALSE(error);
  EXPECT_EQ(filled_json, buffer);

  check_error<wire::json, variant>(std::string{bad_variant_json}, wire::error::schema::invalid_key);
  check_error<wire::json, variant>(std::string{"{}"}, wire::error::schema::missing_key);
}

namespace
{
  constexpr const std::uint8_t compressed_epee[] =
    {0x01,0x11,0x01,0x01,0x01,0x01,0x02,0x01,0x01,0x04,0x04,'i','n','t','s',0x83,0x08,0x1,0x00,0x03,0x00};

  constexpr const std::uint8_t expanded_epee[] =
    {0x01,0x11,0x01,0x01,0x01,0x01,0x02,0x01,0x01,0x04,0x04,'i','n','t','s',
    0x81,0x08,0x1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00};
}

TEST(wire, epee_writer)
{
  const required_array data{{1, 3}};

  epee::byte_stream buffer;
  std::error_code error = wire::epee_bin::to_bytes(buffer, data);
  ASSERT_FALSE(error);

  // writing with derived class should yield exact size type
  epee::byte_slice slice{std::move(buffer)};
  EXPECT_TRUE(boost::range::equal(slice, compressed_epee))
    << epee::to_hex::string(epee::to_span(slice)) << " vs " << epee::to_hex::string(compressed_epee);

  buffer.clear();
  const constraints<2, 1, 1, 1, 1, 1, 1> data2{{1, 3}};
  error = wire::epee_bin::to_bytes(buffer, data2);
  ASSERT_FALSE(error);

  // writing with base class should yield int size type (expecting 64-bit)
  slice = epee::byte_slice{std::move(buffer)};
  EXPECT_TRUE(boost::range::equal(slice, expanded_epee))
    << epee::to_hex::string(epee::to_span(slice)) << " vs " << epee::to_hex::string(expanded_epee);
}
