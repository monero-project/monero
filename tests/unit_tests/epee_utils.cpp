// Copyright (c) 2014-2019, The Monero Project
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

#include <array>
#include <boost/predef/other/endian.h>
#include <boost/endian/conversion.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm_ext/iota.hpp>
#include <cstdint>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#ifndef _WIN32
# include <arpa/inet.h>
#endif

#include "boost/archive/portable_binary_iarchive.hpp"
#include "boost/archive/portable_binary_oarchive.hpp"
#include "byte_slice.h"
#include "crypto/crypto.h"
#include "hex.h"
#include "net/net_utils_base.h"
#include "net/local_ip.h"
#include "net/buffer.h"
#include "p2p/net_peerlist_boost_serialization.h"
#include "span.h"
#include "string_tools.h"
#include "storages/parserse_base_utils.h"

namespace
{
  template<typename Destination, typename Source>
  bool can_construct()
  {
    const unsigned count =
      unsigned(std::is_constructible<Destination, Source>()) +
      unsigned(std::is_constructible<Destination, Source&>()) +
      unsigned(std::is_convertible<Source, Destination>()) +
      unsigned(std::is_convertible<Source&, Destination>()) +
      unsigned(std::is_assignable<Destination, Source>()) +
      unsigned(std::is_assignable<Destination, Source&>());
    EXPECT_TRUE(count == 6 || count == 0) <<
      "Mismatch on construction results - " << count << " were true";
    return count == 6; 
  }

  // This is probably stressing the compiler more than the implementation ...
  constexpr const epee::span<const char> test_string("a string");
  static_assert(!test_string.empty(), "test failure");
  static_assert(test_string.size() == 9, "test failure");
  static_assert(test_string.size_bytes() == 9, "test_failure");
  static_assert(test_string.begin() == test_string.cbegin(), "test failure");
  static_assert(test_string.end() == test_string.cend(), "test failure");
  static_assert(test_string.cend() - test_string.cbegin() == 9, "test failure");
  static_assert(*test_string.cbegin() == 'a', "test failure");
  static_assert(*(test_string.cend() - 2) == 'g', "test failure");
  static_assert(
    epee::span<const char>(test_string).cbegin() + 3 == test_string.cbegin() + 3,
    "test failure"
  );

  static_assert(epee::span<char>().empty(), "test failure");
  static_assert(epee::span<char>(nullptr).empty(), "test failure");
  static_assert(epee::span<const char>("foo", 2).size() == 2, "test failure");

  std::string std_to_hex(const std::vector<unsigned char>& source)
  {
    std::stringstream out;
    out << std::hex;
    for (const unsigned char byte : source)
    {
      out << std::setw(2) << std::setfill('0') << int(byte);
    }
    return out.str();
  }

  std::vector<unsigned char> get_all_bytes()
  {
    std::vector<unsigned char> out;
    out.resize(256);
    boost::range::iota(out, 0);
    return out;
  }

  #define CHECK_EQUAL(lhs, rhs) \
    EXPECT_TRUE( lhs == rhs );  \
    EXPECT_TRUE( rhs == lhs );  \
    EXPECT_FALSE( lhs != rhs ); \
    EXPECT_FALSE( rhs != lhs ); \
    EXPECT_FALSE( lhs < rhs );  \
    EXPECT_FALSE( rhs < lhs );  \
    EXPECT_TRUE( lhs <= rhs );  \
    EXPECT_TRUE( rhs <= lhs );  \
    EXPECT_FALSE( lhs > rhs );  \
    EXPECT_FALSE( rhs > lhs );  \
    EXPECT_TRUE( lhs >= rhs );  \
    EXPECT_TRUE( rhs >= lhs )

  #define CHECK_LESS(lhs, rhs)   \
    EXPECT_FALSE( lhs == rhs );  \
    EXPECT_FALSE( rhs == lhs );  \
    EXPECT_TRUE( lhs != rhs );   \
    EXPECT_TRUE( rhs != lhs );   \
    EXPECT_TRUE( lhs < rhs );    \
    EXPECT_FALSE( rhs < lhs );   \
    EXPECT_TRUE( lhs <= rhs );   \
    EXPECT_FALSE( rhs <= lhs );  \
    EXPECT_FALSE( lhs > rhs );   \
    EXPECT_TRUE( rhs > lhs );    \
    EXPECT_FALSE( lhs >= rhs );  \
    EXPECT_TRUE( rhs >= lhs )

  #if BOOST_ENDIAN_LITTLE_BYTE
    #define CHECK_LESS_ENDIAN(lhs, rhs) CHECK_LESS( rhs , lhs )
  #else
    #define CHECK_LESS_ENDIAN(lhs, rhs) CHECK_LESS( lhs , rhs )
  #endif
}

TEST(Span, Traits)
{
  EXPECT_TRUE((std::is_same<std::size_t, typename epee::span<char>::size_type>()));
  EXPECT_TRUE((std::is_same<std::ptrdiff_t, typename epee::span<char>::difference_type>()));
  EXPECT_TRUE((std::is_same<char, typename epee::span<char>::value_type>()));
  EXPECT_TRUE((std::is_same<char*, typename epee::span<char>::pointer>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<char>::const_pointer>()));
  EXPECT_TRUE((std::is_same<char*, typename epee::span<char>::iterator>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<char>::const_iterator>()));
  EXPECT_TRUE((std::is_same<char&, typename epee::span<char>::reference>()));
  EXPECT_TRUE((std::is_same<const char&, typename epee::span<char>::const_reference>()));

  EXPECT_TRUE((std::is_same<std::size_t, typename epee::span<const char>::size_type>()));
  EXPECT_TRUE((std::is_same<std::ptrdiff_t, typename epee::span<const char>::difference_type>()));
  EXPECT_TRUE((std::is_same<const char, typename epee::span<const char>::value_type>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<const char>::pointer>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<const char>::const_pointer>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<const char>::iterator>()));
  EXPECT_TRUE((std::is_same<const char*, typename epee::span<const char>::const_iterator>()));
  EXPECT_TRUE((std::is_same<const char&, typename epee::span<const char>::reference>()));
  EXPECT_TRUE((std::is_same<const char&, typename epee::span<const char>::const_reference>()));
}

TEST(Span, MutableConstruction)
{
  struct no_conversion{};
  struct inherited : no_conversion {};

  EXPECT_TRUE(std::is_constructible<epee::span<char>>());
  EXPECT_TRUE((std::is_constructible<epee::span<char>, char*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<char>, const char*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<char>, unsigned char*, std::size_t>()));

  EXPECT_TRUE(std::is_constructible<epee::span<no_conversion>>());
  EXPECT_TRUE((std::is_constructible<epee::span<no_conversion>, no_conversion*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<no_conversion>, inherited*, std::size_t>()));

  EXPECT_TRUE((can_construct<epee::span<char>, std::nullptr_t>()));
  EXPECT_TRUE((can_construct<epee::span<char>, char(&)[1]>()));

  EXPECT_FALSE((can_construct<epee::span<char>, std::vector<char>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, std::array<char, 1>>()));

  EXPECT_FALSE((can_construct<epee::span<char>, std::wstring>()));
  EXPECT_FALSE((can_construct<epee::span<char>, const std::vector<char>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, std::vector<unsigned char>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, const std::array<char, 1>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, std::array<unsigned char, 1>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, const char[1]>()));
  EXPECT_FALSE((can_construct<epee::span<char>, unsigned char[1]>()));
  EXPECT_FALSE((can_construct<epee::span<char>, epee::span<const char>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, epee::span<unsigned char>>()));
  EXPECT_FALSE((can_construct<epee::span<char>, no_conversion>()));
}

TEST(Span, ImmutableConstruction)
{
  struct no_conversion{};
  struct inherited : no_conversion {};

  EXPECT_TRUE(std::is_constructible<epee::span<const char>>());
  EXPECT_TRUE((std::is_constructible<epee::span<const char>, char*, std::size_t>()));
  EXPECT_TRUE((std::is_constructible<epee::span<const char>, const char*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<const char>, unsigned char*, std::size_t>()));

  EXPECT_TRUE(std::is_constructible<epee::span<const no_conversion>>());
  EXPECT_TRUE((std::is_constructible<epee::span<const no_conversion>, const no_conversion*, std::size_t>()));
  EXPECT_TRUE((std::is_constructible<epee::span<const no_conversion>, no_conversion*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<const no_conversion>, const inherited*, std::size_t>()));
  EXPECT_FALSE((std::is_constructible<epee::span<const no_conversion>, inherited*, std::size_t>()));

  EXPECT_FALSE((can_construct<epee::span<const char>, std::string>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, std::vector<char>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, const std::vector<char>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, std::array<char, 1>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, const std::array<char, 1>>()));

  EXPECT_TRUE((can_construct<epee::span<const char>, std::nullptr_t>()));
  EXPECT_TRUE((can_construct<epee::span<const char>, char[1]>()));
  EXPECT_TRUE((can_construct<epee::span<const char>, const char[1]>()));
  EXPECT_TRUE((can_construct<epee::span<const char>, epee::span<const char>>()));

  EXPECT_FALSE((can_construct<epee::span<const char>, std::wstring>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, std::vector<unsigned char>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, std::array<unsigned char, 1>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, unsigned char[1]>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, epee::span<unsigned char>>()));
  EXPECT_FALSE((can_construct<epee::span<const char>, no_conversion>()));
}

TEST(Span, NoExcept)
{
  EXPECT_TRUE(std::is_nothrow_default_constructible<epee::span<char>>());
  EXPECT_TRUE(std::is_nothrow_move_constructible<epee::span<char>>());
  EXPECT_TRUE(std::is_nothrow_copy_constructible<epee::span<char>>());
  EXPECT_TRUE(std::is_move_assignable<epee::span<char>>());
  EXPECT_TRUE(std::is_copy_assignable<epee::span<char>>());

  char data[10];
  epee::span<char> lvalue(data);
  const epee::span<char> clvalue(data);
  EXPECT_TRUE(noexcept(epee::span<char>()));
  EXPECT_TRUE(noexcept(epee::span<char>(nullptr)));
  EXPECT_TRUE(noexcept(epee::span<char>(data)));
  EXPECT_TRUE(noexcept(epee::span<char>(lvalue)));
  EXPECT_TRUE(noexcept(epee::span<char>(clvalue)));

  // conversion from mutable to immutable not yet implemented
  // EXPECT_TRUE(noexcept(epee::span<const char>(lvalue)));
  // EXPECT_TRUE(noexcept(epee::span<const char>(clvalue)));

  EXPECT_TRUE(noexcept(epee::span<char>(epee::span<char>(lvalue))));
  EXPECT_TRUE(noexcept(lvalue = lvalue));
  EXPECT_TRUE(noexcept(lvalue = clvalue));
  EXPECT_TRUE(noexcept(lvalue = epee::span<char>(lvalue)));
}

TEST(Span, Nullptr)
{
  const auto check_empty = [](epee::span<const char> data)
  {
    EXPECT_TRUE(data.empty());
    EXPECT_EQ(data.cbegin(), data.begin());
    EXPECT_EQ(data.cend(), data.end());
    EXPECT_EQ(data.cend(), data.cbegin());
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(0, data.size_bytes());
  };
  check_empty({});
  check_empty(nullptr); 
}

TEST(Span, Writing)
{
  const int expected[] = {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int> source;

  epee::span<int> span;
  EXPECT_TRUE(span.empty());
  EXPECT_EQ(0, span.size());
  EXPECT_EQ(0, span.size_bytes());

  source.resize(15);
  span = {source.data(), source.size()};
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(15, span.size());
  EXPECT_EQ(15 * 4, span.size_bytes());

  boost::range::iota(span, -5);
  EXPECT_EQ(span.begin(), span.cbegin());
  EXPECT_EQ(span.end(), span.cend());
  EXPECT_TRUE(boost::range::equal(expected, source));
  EXPECT_TRUE(boost::range::equal(expected, span));
}

TEST(Span, RemovePrefix)
{
  const  std::array<unsigned, 4> expected{0, 1, 2, 3};
  auto span = epee::to_span(expected);

  EXPECT_EQ(expected.begin(), span.begin());
  EXPECT_EQ(expected.end(), span.end());

  EXPECT_EQ(2u, span.remove_prefix(2));
  EXPECT_EQ(expected.begin() + 2, span.begin());
  EXPECT_EQ(expected.end(), span.end());

  EXPECT_EQ(2u, span.remove_prefix(3));
  EXPECT_EQ(span.begin(), span.end());
  EXPECT_EQ(expected.end(), span.begin());

  EXPECT_EQ(0u, span.remove_prefix(100));
}

TEST(Span, ToByteSpan)
{
  const char expected[] = {56, 44, 11, 5};
  EXPECT_TRUE(
    boost::range::equal(
      std::array<std::uint8_t, 4>{{56, 44, 11, 5}},
      epee::to_byte_span<char>(expected)
    )
  );
  EXPECT_TRUE(
    boost::range::equal(
      std::array<char, 4>{{56, 44, 11, 5}},
      epee::to_byte_span(epee::span<const char>{expected})
    )
  );
}

TEST(Span, AsByteSpan)
{
  struct some_pod { char value[4]; };
  const some_pod immutable {{ 5, 10, 12, 127 }};
  EXPECT_TRUE(
    boost::range::equal(
      std::array<unsigned char, 4>{{5, 10, 12, 127}},
      epee::as_byte_span(immutable)
    )
  );
  EXPECT_TRUE(
    boost::range::equal(
      std::array<std::uint8_t, 3>{{'a', 'y', 0x00}}, epee::as_byte_span("ay")
    )
  );
}

TEST(Span, AsMutByteSpan)
{
  struct some_pod { char value[4]; };
  some_pod actual {};

  auto span = epee::as_mut_byte_span(actual);
  boost::range::iota(span, 1);
  EXPECT_TRUE(
    boost::range::equal(
      std::array<unsigned char, 4>{{1, 2, 3, 4}}, actual.value
    )
  );
}

TEST(Span, ToMutSpan)
{
  std::vector<unsigned> mut;
  mut.resize(4);

  auto span = epee::to_mut_span(mut);
  boost::range::iota(span, 1);
  EXPECT_EQ((std::vector<unsigned>{1, 2, 3, 4}), mut);
}

TEST(ByteSlice, Construction)
{
  EXPECT_TRUE(std::is_default_constructible<epee::byte_slice>());
  EXPECT_TRUE(std::is_move_constructible<epee::byte_slice>());
  EXPECT_FALSE(std::is_copy_constructible<epee::byte_slice>());
  EXPECT_TRUE(std::is_move_assignable<epee::byte_slice>());
  EXPECT_FALSE(std::is_copy_assignable<epee::byte_slice>());
}

TEST(ByteSlice, NoExcept)
{
  EXPECT_TRUE(std::is_nothrow_default_constructible<epee::byte_slice>());
  EXPECT_TRUE(std::is_nothrow_move_constructible<epee::byte_slice>());
  EXPECT_TRUE(std::is_nothrow_move_assignable<epee::byte_slice>());

  epee::byte_slice lvalue{};
  const epee::byte_slice clvalue{};

  EXPECT_TRUE(noexcept(lvalue.clone()));
  EXPECT_TRUE(noexcept(clvalue.clone()));

  EXPECT_TRUE(noexcept(lvalue.begin()));
  EXPECT_TRUE(noexcept(clvalue.begin()));
  EXPECT_TRUE(noexcept(lvalue.end()));
  EXPECT_TRUE(noexcept(clvalue.end()));

  EXPECT_TRUE(noexcept(lvalue.cbegin()));
  EXPECT_TRUE(noexcept(clvalue.cbegin()));
  EXPECT_TRUE(noexcept(lvalue.cend()));
  EXPECT_TRUE(noexcept(clvalue.cend()));

  EXPECT_TRUE(noexcept(lvalue.empty()));
  EXPECT_TRUE(noexcept(clvalue.empty()));

  EXPECT_TRUE(noexcept(lvalue.data()));
  EXPECT_TRUE(noexcept(clvalue.data()));
  EXPECT_TRUE(noexcept(lvalue.size()));
  EXPECT_TRUE(noexcept(clvalue.size()));

  EXPECT_TRUE(noexcept(lvalue.remove_prefix(0)));
  EXPECT_TRUE(noexcept(lvalue.take_slice(0)));
}

TEST(ByteSlice, Empty)
{
  epee::byte_slice slice{};

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(0u, slice.size());
  EXPECT_EQ(slice.begin(), slice.data());

  EXPECT_EQ(0u, slice.get_slice(0, 0).size());
  EXPECT_THROW(slice.get_slice(0, 1), std::out_of_range);
  EXPECT_EQ(0u, slice.remove_prefix(1));
  EXPECT_EQ(0u, slice.take_slice(1).size());
}

TEST(ByteSlice, CopySpans)
{
  const epee::span<const std::uint8_t> part1 = epee::as_byte_span("this is part1");
  const epee::span<const std::uint8_t> part2 = epee::as_byte_span("then part2");
  const epee::span<const std::uint8_t> part3 = epee::as_byte_span("finally part3");

  const epee::byte_slice slice{part1, part2, part3};

  EXPECT_NE(nullptr, slice.begin());
  EXPECT_NE(nullptr, slice.end());
  EXPECT_NE(slice.begin(), slice.end());
  EXPECT_NE(slice.cbegin(), slice.cend());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());
  ASSERT_EQ(slice.size(), std::size_t(slice.end() - slice.begin()));

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(slice.begin(), slice.data());
  ASSERT_EQ(part1.size() + part2.size() + part3.size(), slice.size());
  EXPECT_TRUE(
    boost::range::equal(
      part1, boost::make_iterator_range(slice.begin(), slice.begin() + part1.size())
    )
  );
  EXPECT_TRUE(
    boost::range::equal(
      part2, boost::make_iterator_range(slice.begin() + part1.size(), slice.end() - part3.size())
    )
  );
  EXPECT_TRUE(
    boost::range::equal(
      part3, boost::make_iterator_range(slice.end() - part3.size(), slice.end())
    )
  );
}

TEST(ByteSlice, AdaptString)
{
  static constexpr const char base_string[] = "this is an example message";
  std::string adapted = base_string;

  const epee::span<const uint8_t> original = epee::to_byte_span(epee::to_span(adapted));
  const epee::byte_slice slice{std::move(adapted)};

  EXPECT_EQ(original.begin(), slice.begin());
  EXPECT_EQ(original.cbegin(), slice.cbegin());
  EXPECT_EQ(original.end(), slice.end());
  EXPECT_EQ(original.cend(), slice.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(original.data(), slice.data());
  EXPECT_EQ(original.size(), slice.size());
  EXPECT_TRUE(boost::range::equal(boost::string_ref{base_string}, slice));
}

TEST(ByteSlice, EmptyAdaptString)
{
  epee::byte_slice slice{std::string{}};

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(0u, slice.size());
  EXPECT_EQ(slice.begin(), slice.data());

  EXPECT_EQ(0u, slice.get_slice(0, 0).size());
  EXPECT_THROW(slice.get_slice(0, 1), std::out_of_range);
  EXPECT_EQ(0u, slice.remove_prefix(1));
  EXPECT_EQ(0u, slice.take_slice(1).size());
}

TEST(ByteSlice, AdaptVector)
{
  static constexpr const char base_string[] = "this is an example message";
  std::vector<std::uint8_t> adapted(sizeof(base_string));

  ASSERT_EQ(sizeof(base_string), adapted.size());
  std::memcpy(adapted.data(), base_string, sizeof(base_string));

  const epee::span<const uint8_t> original = epee::to_span(adapted);
  const epee::byte_slice slice{std::move(adapted)};

  EXPECT_EQ(sizeof(base_string), original.size());

  EXPECT_EQ(original.begin(), slice.begin());
  EXPECT_EQ(original.cbegin(), slice.cbegin());
  EXPECT_EQ(original.end(), slice.end());
  EXPECT_EQ(original.cend(), slice.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(original.data(), slice.data());
  EXPECT_EQ(original.size(), slice.size());
  EXPECT_TRUE(boost::range::equal(base_string, slice));
}

TEST(ByteSlice, EmptyAdaptVector)
{
  epee::byte_slice slice{std::vector<std::uint8_t>{}};

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(0u, slice.size());
  EXPECT_EQ(slice.begin(), slice.data());

  EXPECT_EQ(0u, slice.get_slice(0, 0).size());
  EXPECT_THROW(slice.get_slice(0, 1), std::out_of_range);
  EXPECT_EQ(0u, slice.remove_prefix(1));
  EXPECT_EQ(0u, slice.take_slice(1).size());
}

TEST(ByteSlice, Move)
{
  static constexpr const char base_string[] = "another example message";

  epee::byte_slice slice{epee::as_byte_span(base_string)};
  EXPECT_TRUE(boost::range::equal(base_string, slice));

  const epee::span<const std::uint8_t> original = epee::to_span(slice);
  epee::byte_slice moved{std::move(slice)};
  EXPECT_TRUE(boost::range::equal(base_string, moved));

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_EQ(original.begin(), moved.begin());
  EXPECT_EQ(moved.begin(), moved.cbegin());
  EXPECT_EQ(original.end(), moved.end());
  EXPECT_EQ(moved.end(), moved.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(slice.begin(), slice.data());
  EXPECT_EQ(0u, slice.size());

  EXPECT_FALSE(moved.empty());
  EXPECT_EQ(moved.begin(), moved.data());
  EXPECT_EQ(original.size(), moved.size());

  slice = std::move(moved);
  EXPECT_TRUE(boost::range::equal(base_string, slice));

  EXPECT_EQ(original.begin(), slice.begin());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(original.end(), slice.end());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(slice.begin(), slice.data());
  EXPECT_EQ(original.size(), slice.size());

  EXPECT_TRUE(moved.empty());
  EXPECT_EQ(moved.begin(), moved.data());
  EXPECT_EQ(0u, moved.size());
}

TEST(ByteSlice, Clone)
{
  static constexpr const char base_string[] = "another example message";

  const epee::byte_slice slice{epee::as_byte_span(base_string)};
  EXPECT_TRUE(boost::range::equal(base_string, slice));

  const epee::byte_slice clone{slice.clone()};
  EXPECT_TRUE(boost::range::equal(base_string, clone));

  EXPECT_EQ(slice.begin(), clone.begin());
  EXPECT_EQ(slice.cbegin(), clone.cbegin());
  EXPECT_EQ(slice.end(), clone.end());
  EXPECT_EQ(slice.cend(), clone.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_FALSE(clone.empty());
  EXPECT_EQ(slice.cbegin(), slice.data());
  EXPECT_EQ(slice.data(), clone.data());
  EXPECT_EQ(sizeof(base_string), slice.size());
  EXPECT_EQ(slice.size(), clone.size());
}

TEST(ByteSlice, RemovePrefix)
{
  static constexpr const char base_string[] = "another example message";
  static constexpr std::size_t remove_size = sizeof("another");
  static constexpr std::size_t remaining = sizeof(base_string) - remove_size;

  epee::byte_slice slice{epee::as_byte_span(base_string)};
  EXPECT_TRUE(boost::range::equal(base_string, slice));

  const epee::span<const std::uint8_t> original = epee::to_span(slice);
  EXPECT_EQ(remove_size, slice.remove_prefix(remove_size));

  EXPECT_EQ(original.begin() + remove_size, slice.begin());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(original.end(), slice.end());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(slice.cbegin(), slice.data());
  EXPECT_EQ(remaining, slice.size());

  // touch original pointers to check "free" status
  EXPECT_TRUE(boost::range::equal(base_string, original));

  EXPECT_EQ(remaining, slice.remove_prefix(remaining + 1));

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(slice.cbegin(), slice.data());
  EXPECT_EQ(0, slice.size());
}

TEST(ByteSlice, TakeSlice)
{
  static constexpr const char base_string[] = "another example message";
  static constexpr std::size_t remove_size = sizeof("another");
  static constexpr std::size_t remaining = sizeof(base_string) - remove_size;

  epee::byte_slice slice{epee::as_byte_span(base_string)};
  EXPECT_TRUE(boost::range::equal(base_string, slice));

  const epee::span<const std::uint8_t> original = epee::to_span(slice);
  const epee::byte_slice slice2 = slice.take_slice(remove_size);

  EXPECT_EQ(original.begin() + remove_size, slice.begin());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(original.end(), slice.end());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_EQ(original.begin(), slice2.begin());
  EXPECT_EQ(slice2.begin(), slice2.cbegin());
  EXPECT_EQ(original.begin() + remove_size, slice2.end());
  EXPECT_EQ(slice2.end(), slice2.cend());

  EXPECT_FALSE(slice.empty());
  EXPECT_EQ(slice.cbegin(), slice.data());
  EXPECT_EQ(remaining, slice.size());

  EXPECT_FALSE(slice2.empty());
  EXPECT_EQ(slice2.cbegin(), slice2.data());
  EXPECT_EQ(remove_size, slice2.size());

  // touch original pointers to check "free" status
  EXPECT_TRUE(boost::range::equal(base_string, original));

  const epee::byte_slice slice3 = slice.take_slice(remaining + 1);

  EXPECT_EQ(slice.begin(), slice.end());
  EXPECT_EQ(slice.begin(), slice.cbegin());
  EXPECT_EQ(slice.end(), slice.cend());

  EXPECT_EQ(original.begin(), slice2.begin());
  EXPECT_EQ(slice2.begin(), slice2.cbegin());
  EXPECT_EQ(original.begin() + remove_size, slice2.end());
  EXPECT_EQ(slice2.end(), slice2.cend());

  EXPECT_EQ(slice2.end(), slice3.begin());
  EXPECT_EQ(slice3.begin(), slice3.cbegin());
  EXPECT_EQ(original.end(), slice3.end());
  EXPECT_EQ(slice3.end(), slice3.cend());

  EXPECT_TRUE(slice.empty());
  EXPECT_EQ(slice.cbegin(), slice.data());
  EXPECT_EQ(0, slice.size());

  EXPECT_FALSE(slice2.empty());
  EXPECT_EQ(slice2.cbegin(), slice2.data());
  EXPECT_EQ(remove_size, slice2.size());

  EXPECT_FALSE(slice3.empty());
  EXPECT_EQ(slice3.cbegin(), slice3.data());
  EXPECT_EQ(remaining, slice3.size());

  // touch original pointers to check "free" status
  slice = nullptr;
  EXPECT_TRUE(boost::range::equal(base_string, original));
}

TEST(ByteSlice, GetSlice)
{
  static constexpr const char base_string[] = "another example message";
  static constexpr std::size_t get_size = sizeof("another");
  static constexpr std::size_t get2_size = sizeof(base_string) - get_size;

  epee::span<const std::uint8_t> original{};
  epee::byte_slice slice2{};
  epee::byte_slice slice3{};

  // make sure get_slice increments ref count
  {
    const epee::byte_slice slice{epee::as_byte_span(base_string)};
    EXPECT_TRUE(boost::range::equal(base_string, slice));

    original = epee::to_span(slice);
    slice2 = slice.get_slice(0, get_size);

    EXPECT_EQ(original.begin(), slice.begin());
    EXPECT_EQ(slice.begin(), slice.cbegin());
    EXPECT_EQ(original.end(), slice.end());
    EXPECT_EQ(slice.end(), slice.cend());

    EXPECT_EQ(original.begin(), slice2.begin());
    EXPECT_EQ(slice2.begin(), slice2.cbegin());
    EXPECT_EQ(original.begin() + get_size, slice2.end());
    EXPECT_EQ(slice2.end(), slice2.cend());

    EXPECT_FALSE(slice.empty());
    EXPECT_EQ(slice.cbegin(), slice.data());
    EXPECT_EQ(original.size(), slice.size());

    EXPECT_FALSE(slice2.empty());
    EXPECT_EQ(slice2.cbegin(), slice2.data());
    EXPECT_EQ(get_size, slice2.size());

    // touch original pointers to check "free" status
    EXPECT_TRUE(boost::range::equal(base_string, original));

    slice3 = slice.get_slice(get_size, sizeof(base_string));

    EXPECT_EQ(original.begin(), slice.begin());
    EXPECT_EQ(slice.begin(), slice.cbegin());
    EXPECT_EQ(original.end(), slice.end());
    EXPECT_EQ(slice.end(), slice.cend());

    EXPECT_EQ(original.begin(), slice2.begin());
    EXPECT_EQ(slice2.begin(), slice2.cbegin());
    EXPECT_EQ(original.begin() + get_size, slice2.end());
    EXPECT_EQ(slice2.end(), slice2.cend());

    EXPECT_EQ(slice2.end(), slice3.begin());
    EXPECT_EQ(slice3.begin(), slice3.cbegin());
    EXPECT_EQ(original.end(), slice3.end());
    EXPECT_EQ(slice3.end(), slice3.cend());

    EXPECT_FALSE(slice.empty());
    EXPECT_EQ(slice.cbegin(), slice.data());
    EXPECT_EQ(original.size(), slice.size());

    EXPECT_FALSE(slice2.empty());
    EXPECT_EQ(slice2.cbegin(), slice2.data());
    EXPECT_EQ(get_size, slice2.size());

    EXPECT_FALSE(slice3.empty());
    EXPECT_EQ(slice3.cbegin(), slice3.data());
    EXPECT_EQ(get2_size, slice3.size());

    EXPECT_THROW(slice.get_slice(1, 0), std::out_of_range);
    EXPECT_THROW(slice.get_slice(0, sizeof(base_string) + 1), std::out_of_range);
    EXPECT_THROW(slice.get_slice(sizeof(base_string) + 1, sizeof(base_string) + 1), std::out_of_range);
    EXPECT_TRUE(slice.get_slice(sizeof(base_string), sizeof(base_string)).empty());

    EXPECT_EQ(original.begin(), slice.begin());
    EXPECT_EQ(slice.begin(), slice.cbegin());
    EXPECT_EQ(original.end(), slice.end());
    EXPECT_EQ(slice.end(), slice.cend());

    EXPECT_FALSE(slice.empty());
    EXPECT_EQ(slice.cbegin(), slice.data());
    EXPECT_EQ(original.size(), slice.size());
  }

  // touch original pointers to check "free" status
  EXPECT_TRUE(boost::range::equal(base_string, original));
}

TEST(ToHex, String)
{
  EXPECT_TRUE(epee::to_hex::string(nullptr).empty());
  EXPECT_EQ(
    std::string{"ffab0100"},
    epee::to_hex::string(epee::as_byte_span("\xff\xab\x01"))
  );

  const std::vector<unsigned char> all_bytes = get_all_bytes();
  EXPECT_EQ(
    std_to_hex(all_bytes), epee::to_hex::string(epee::to_span(all_bytes))
  );

}

TEST(FromHex, String)
{
    // the source data to encode and decode
    std::vector<uint8_t> source{{ 0x00, 0xFF, 0x0F, 0xF0 }};

    // encode and decode the data
    auto hex = epee::to_hex::string({ source.data(), source.size() });
    auto decoded = epee::from_hex::vector(hex);

    // encoded should be twice the size and should decode to the exact same data
    EXPECT_EQ(source.size() * 2, hex.size());
    EXPECT_EQ(source, decoded);

    // we will now create a padded hex string, we want to explicitly allow
    // decoding it this way also, ignoring spaces and colons between the numbers
    hex.assign("00:ff 0f:f0");
    EXPECT_EQ(source, epee::from_hex::vector(hex));
}

TEST(ToHex, Array)
{
  EXPECT_EQ(
    (std::array<char, 8>{{'f', 'f', 'a', 'b', '0', '1', '0', '0'}}),
    (epee::to_hex::array(std::array<unsigned char, 4>{{0xFF, 0xAB, 0x01, 0x00}}))
  );
}

TEST(ToHex, ArrayFromPod)
{
  std::array<char, 64> expected{{'5', 'f', '2', 'b', '0', '1'}};
  std::fill(expected.begin() + 6, expected.end(), '0');

  EXPECT_EQ(
    expected,
    (epee::to_hex::array(crypto::ec_point{{0x5F, 0x2B, 0x01, 0x00}}))
  );
}

TEST(ToHex, Ostream)
{
  std::stringstream out;
  epee::to_hex::buffer(out, nullptr);
  EXPECT_TRUE(out.str().empty());

  {
    const std::uint8_t source[] = {0xff, 0xab, 0x01, 0x00};
    epee::to_hex::buffer(out, source);
  }

  std::string expected{"ffab0100"};
  EXPECT_EQ(expected, out.str());

  const std::vector<unsigned char> all_bytes = get_all_bytes();

  expected.append(std_to_hex(all_bytes));
  epee::to_hex::buffer(out, epee::to_span(all_bytes));
  EXPECT_EQ(expected, out.str());
}

TEST(ToHex, Formatted)
{
  std::stringstream out;
  std::string expected{"<>"};

  epee::to_hex::formatted(out, nullptr);
  EXPECT_EQ(expected, out.str());

  expected.append("<ffab0100>");
  epee::to_hex::formatted(out, epee::as_byte_span("\xFF\xAB\x01"));
  EXPECT_EQ(expected, out.str());

  const std::vector<unsigned char> all_bytes = get_all_bytes();

  expected.append("<").append(std_to_hex(all_bytes)).append(">");
  epee::to_hex::formatted(out, epee::to_span(all_bytes));
  EXPECT_EQ(expected, out.str());
}

TEST(StringTools, BuffToHex)
{
  const std::vector<unsigned char> all_bytes = get_all_bytes();

  EXPECT_EQ(
    std_to_hex(all_bytes),
    (epee::string_tools::buff_to_hex_nodelimer(
      std::string{reinterpret_cast<const char*>(all_bytes.data()), all_bytes.size()}
    ))
  );
}

TEST(StringTools, PodToHex)
{
  struct some_pod { unsigned char data[4]; };
  EXPECT_EQ(
    std::string{"ffab0100"},
    (epee::string_tools::pod_to_hex(some_pod{{0xFF, 0xAB, 0x01, 0x00}}))
  );
}

TEST(StringTools, ParseHex)
{
  static const char data[] = "a10b68c2";
  for (size_t i = 0; i < sizeof(data); i += 2)
  {
    std::string res;
    ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(std::string(data, i), res));
    std::string hex = epee::string_tools::buff_to_hex_nodelimer(res);
    ASSERT_EQ(hex.size(), i);
    ASSERT_EQ(memcmp(data, hex.data(), i), 0);
  }
}

TEST(StringTools, ParseNotHex)
{
  std::string res;
  for (size_t i = 0; i < 256; ++i)
  {
    std::string inputHexString = std::string(2, static_cast<char>(i));
    if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'F') || (i >= 'a' && i <= 'f')) {
      ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(inputHexString, res));
    } else {
      ASSERT_FALSE(epee::string_tools::parse_hexstr_to_binbuff(inputHexString, res));
    }
  }

  ASSERT_FALSE(epee::string_tools::parse_hexstr_to_binbuff(std::string("a"), res));
}

TEST(StringTools, GetIpString)
{
  EXPECT_EQ(
    std::string{"0.0.0.0"}, epee::string_tools::get_ip_string_from_int32(0)
  );
  EXPECT_EQ(
    std::string{"255.0.255.0"},
    epee::string_tools::get_ip_string_from_int32(htonl(0xff00ff00))
  );
  EXPECT_EQ(
    std::string{"255.255.255.255"},
    epee::string_tools::get_ip_string_from_int32(htonl(0xffffffff))
  );
}

TEST(StringTools, GetIpInt32)
{
  std::uint32_t ip = 0;
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, ""));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "ff.0.ff.0"));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1.256"));

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1"));
  EXPECT_EQ(htonl(1), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1"));
  EXPECT_EQ(htonl(0x1000001), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1"));
  EXPECT_EQ(htonl(0x1010001), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "0.0.0.0"));
  EXPECT_EQ(0, ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1.1"));
  EXPECT_EQ(htonl(0x01010101), ip);

/*
  The existing epee conversion function does not work with 255.255.255.255, for
  the reasons specified in the inet_addr documentation. Consider fixing in a
  future patch. This address is not likely to be used for purposes within
  monero.
  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "255.255.255.255"));
  EXPECT_EQ(htonl(0xffffffff), ip);
*/

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "10.0377.0.0377"));
  EXPECT_EQ(htonl(0xaff00ff), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "0xff.10.0xff.0"));
  EXPECT_EQ(htonl(0xff0aff00), ip);
}

TEST(NetUtils, IPv4NetworkAddress)
{
  static_assert(epee::net_utils::ipv4_network_address::get_type_id() == epee::net_utils::address_type::ipv4, "bad ipv4 type id");

  const auto ip1 = boost::endian::native_to_big(0x330012FFu);
  const auto ip_loopback = boost::endian::native_to_big(0x7F000001u);
  const auto ip_local = boost::endian::native_to_big(0x0A000000u);

  epee::net_utils::ipv4_network_address address1{ip1, 65535};
  CHECK_EQUAL(address1, address1);
  EXPECT_STREQ("51.0.18.255:65535", address1.str().c_str());
  EXPECT_STREQ("51.0.18.255", address1.host_str().c_str());
  EXPECT_FALSE(address1.is_loopback());
  EXPECT_FALSE(address1.is_local());
  EXPECT_EQ(epee::net_utils::ipv4_network_address::get_type_id(), address1.get_type_id());
  EXPECT_EQ(ip1, address1.ip());
  EXPECT_EQ(65535, address1.port());
  EXPECT_TRUE(epee::net_utils::ipv4_network_address{std::move(address1)} == address1);
  EXPECT_TRUE(epee::net_utils::ipv4_network_address{address1} == address1);

  const epee::net_utils::ipv4_network_address loopback{ip_loopback, 0};
  CHECK_EQUAL(loopback, loopback);
  CHECK_LESS_ENDIAN(address1, loopback);
  EXPECT_STREQ("127.0.0.1:0", loopback.str().c_str());
  EXPECT_STREQ("127.0.0.1", loopback.host_str().c_str());
  EXPECT_TRUE(loopback.is_loopback());
  EXPECT_FALSE(loopback.is_local());
  EXPECT_EQ(epee::net_utils::ipv4_network_address::get_type_id(), address1.get_type_id());
  EXPECT_EQ(ip_loopback, loopback.ip());
  EXPECT_EQ(0, loopback.port());

  const epee::net_utils::ipv4_network_address local{ip_local, 8080};
  CHECK_EQUAL(local, local);
  CHECK_LESS(local, address1);
  CHECK_LESS(local, loopback);
  EXPECT_FALSE(local.is_loopback());
  EXPECT_TRUE(local.is_local());

  epee::net_utils::ipv4_network_address address2{ip1, 55};
  CHECK_EQUAL(address2, address2);
  CHECK_LESS_ENDIAN(address2, loopback);
  CHECK_LESS(local, address2);
  EXPECT_STREQ("51.0.18.255:55", address2.str().c_str());
  EXPECT_STREQ("51.0.18.255", address2.host_str().c_str());


  address2 = std::move(address1);
  CHECK_EQUAL(address2, address1);

  address2 = local;
  CHECK_EQUAL(address2, local);
  CHECK_LESS(address2, address1);

  {
    std::stringstream stream;
    {
      boost::archive::portable_binary_oarchive ostream{stream};
      ostream << address1;
    }
    {
      boost::archive::portable_binary_iarchive istream{stream};
      istream >> address2;
    }
  }
  CHECK_EQUAL(address1, address2);
  EXPECT_EQ(ip1, address2.ip());
  EXPECT_EQ(65535, address2.port());
}

TEST(NetUtils, NetworkAddress)
{
  const auto ip1 = boost::endian::native_to_big(0x330012FFu);
  const auto ip_loopback = boost::endian::native_to_big(0x7F000001u);
  const auto ip_local = boost::endian::native_to_big(0x0A000000u);

  struct custom_address {
    constexpr static bool equal(const custom_address&) noexcept { return false; }
    constexpr static bool less(const custom_address&) noexcept { return false; }
    constexpr static bool is_same_host(const custom_address&) noexcept { return false; }
    constexpr static bool is_loopback() noexcept { return false; }
    constexpr static bool is_local() noexcept { return false; }
    static std::string str() { return {}; }
    static std::string host_str() { return {}; }
    constexpr static epee::net_utils::address_type get_type_id() noexcept { return epee::net_utils::address_type(-1); }
    constexpr static epee::net_utils::zone get_zone() noexcept { return epee::net_utils::zone::invalid; }
    constexpr static bool is_blockable() noexcept { return false; }
  };

  const epee::net_utils::network_address empty;
  CHECK_EQUAL(empty, empty);
  EXPECT_TRUE(empty.is_same_host(empty));
  EXPECT_STREQ("<none>", empty.str().c_str());
  EXPECT_STREQ("<none>", empty.host_str().c_str());
  EXPECT_FALSE(empty.is_loopback());
  EXPECT_FALSE(empty.is_local());
  EXPECT_EQ(epee::net_utils::address_type::invalid, empty.get_type_id());
  EXPECT_EQ(epee::net_utils::zone::invalid, empty.get_zone());
  EXPECT_FALSE(empty.is_blockable());
  EXPECT_THROW(empty.as<custom_address>(), std::bad_cast);

  epee::net_utils::network_address address1{
    epee::net_utils::ipv4_network_address{ip1, 65535}
  };
  CHECK_EQUAL(address1, address1);
  CHECK_EQUAL(epee::net_utils::network_address{address1}, address1);
  CHECK_LESS(empty, address1);
  EXPECT_TRUE(address1.is_same_host(address1));
  EXPECT_FALSE(empty.is_same_host(address1));
  EXPECT_FALSE(address1.is_same_host(empty));
  EXPECT_STREQ("51.0.18.255:65535", address1.str().c_str());
  EXPECT_STREQ("51.0.18.255", address1.host_str().c_str());
  EXPECT_FALSE(address1.is_loopback());
  EXPECT_FALSE(address1.is_local());
  EXPECT_EQ(epee::net_utils::ipv4_network_address::get_type_id(), address1.get_type_id());
  EXPECT_EQ(epee::net_utils::zone::public_, address1.get_zone());
  EXPECT_TRUE(address1.is_blockable());
  EXPECT_NO_THROW(address1.as<epee::net_utils::ipv4_network_address>());
  EXPECT_THROW(address1.as<custom_address>(), std::bad_cast);

  const epee::net_utils::network_address loopback{
    epee::net_utils::ipv4_network_address{ip_loopback, 0}
  };
  CHECK_EQUAL(loopback, loopback);
  CHECK_LESS(empty, loopback);
  CHECK_LESS_ENDIAN(address1, loopback);
  EXPECT_TRUE(loopback.is_same_host(loopback));
  EXPECT_FALSE(loopback.is_same_host(address1));
  EXPECT_FALSE(address1.is_same_host(loopback));
  EXPECT_STREQ("127.0.0.1:0", loopback.str().c_str());
  EXPECT_STREQ("127.0.0.1", loopback.host_str().c_str());
  EXPECT_TRUE(loopback.is_loopback());
  EXPECT_FALSE(loopback.is_local());
  EXPECT_EQ(epee::net_utils::ipv4_network_address::get_type_id(), address1.get_type_id());
  EXPECT_EQ(epee::net_utils::zone::public_, address1.get_zone());
  EXPECT_EQ(epee::net_utils::ipv4_network_address::get_type_id(), address1.get_type_id());

  const epee::net_utils::network_address local{
    epee::net_utils::ipv4_network_address{ip_local, 8080}
  };
  CHECK_EQUAL(local, local);
  CHECK_LESS(local, loopback);
  CHECK_LESS(local, address1);
  EXPECT_FALSE(local.is_loopback());
  EXPECT_TRUE(local.is_local());

  epee::net_utils::network_address address2{
    epee::net_utils::ipv4_network_address{ip1, 55}
  };
  CHECK_EQUAL(address2, address2);
  CHECK_LESS(address2, address1);
  CHECK_LESS(local, address2);
  CHECK_LESS_ENDIAN(address2, loopback);
  EXPECT_TRUE(address1.is_same_host(address2));
  EXPECT_TRUE(address2.is_same_host(address1));
  EXPECT_STREQ("51.0.18.255:55", address2.str().c_str());
  EXPECT_STREQ("51.0.18.255", address2.host_str().c_str());

  address2 = std::move(address1);
  CHECK_EQUAL(address1, address1);
  CHECK_EQUAL(empty, address1);
  CHECK_LESS(address1, address2);
  EXPECT_FALSE(address1.is_same_host(address2));
  EXPECT_FALSE(address2.is_same_host(address1));
  EXPECT_STREQ("51.0.18.255:65535", address2.str().c_str());
  EXPECT_STREQ("51.0.18.255", address2.host_str().c_str());
  EXPECT_FALSE(address1.is_loopback());
  EXPECT_FALSE(address1.is_local());
  EXPECT_THROW(address1.as<epee::net_utils::ipv4_network_address>(), std::bad_cast);
  EXPECT_NO_THROW(address2.as<epee::net_utils::ipv4_network_address>());

  address2 = local;
  CHECK_EQUAL(address2, local);
  CHECK_LESS(address1, address2);
  EXPECT_TRUE(address2.is_same_host(local));
  EXPECT_TRUE(local.is_same_host(address2));
  EXPECT_FALSE(address2.is_same_host(address1));
  EXPECT_FALSE(address1.is_same_host(address2));

  {
    std::stringstream stream;
    {
      boost::archive::portable_binary_oarchive ostream{stream};
      ostream << address2;
    }
    {
      boost::archive::portable_binary_iarchive istream{stream};
      istream >> address1;
    }
  }
  CHECK_EQUAL(address1, address2);
  EXPECT_TRUE(address1.is_same_host(address2));
  EXPECT_TRUE(address2.is_same_host(address1));
  EXPECT_NO_THROW(address1.as<epee::net_utils::ipv4_network_address>());

  address1 = custom_address{};
  CHECK_EQUAL(address1, address1);
  CHECK_LESS(address2, address1);
  EXPECT_FALSE(address1.is_same_host(loopback));
  EXPECT_FALSE(loopback.is_same_host(address1));
  EXPECT_THROW(address1.as<epee::net_utils::ipv4_network_address>(), std::bad_cast);
  EXPECT_NO_THROW(address1.as<custom_address>());
}

static bool is_local(const char *s)
{
  uint32_t ip;
  CHECK_AND_ASSERT_THROW_MES(epee::string_tools::get_ip_int32_from_string(ip, s), std::string("Invalid IP address: ") + s);
  return epee::net_utils::is_ip_local(ip);
}

TEST(NetUtils, PrivateRanges)
{
  ASSERT_EQ(is_local("10.0.0.0"), true);
  ASSERT_EQ(is_local("10.255.0.0"), true);
  ASSERT_EQ(is_local("127.0.0.0"), false); // loopback is not considered local
  ASSERT_EQ(is_local("192.167.255.255"), false);
  ASSERT_EQ(is_local("192.168.0.0"), true);
  ASSERT_EQ(is_local("192.168.255.255"), true);
  ASSERT_EQ(is_local("192.169.0.0"), false);
  ASSERT_EQ(is_local("172.0.0.0"), false);
  ASSERT_EQ(is_local("172.15.255.255"), false);
  ASSERT_EQ(is_local("172.16.0.0"), true);
  ASSERT_EQ(is_local("172.16.255.255"), true);
  ASSERT_EQ(is_local("172.31.255.255"), true);
  ASSERT_EQ(is_local("172.32.0.0"), false);
  ASSERT_EQ(is_local("0.0.0.0"), false);
  ASSERT_EQ(is_local("255.255.255.254"), false);
  ASSERT_EQ(is_local("11.255.255.255"), false);
  ASSERT_EQ(is_local("0.0.0.10"), false);
  ASSERT_EQ(is_local("0.0.168.192"), false);
  ASSERT_EQ(is_local("0.0.30.172"), false);
  ASSERT_EQ(is_local("0.0.30.127"), false);
}

TEST(net_buffer, basic)
{
  epee::net_utils::buffer buf;

  ASSERT_EQ(buf.size(), 0);
  EXPECT_THROW(buf.span(1), std::runtime_error);
  buf.append("a", 1);
  epee::span<const uint8_t> span = buf.span(1);
  ASSERT_EQ(span.size(), 1);
  ASSERT_EQ(span.data()[0], 'a');
  EXPECT_THROW(buf.span(2), std::runtime_error);
  buf.append("bc", 2);
  buf.erase(1);
  EXPECT_THROW(buf.span(3), std::runtime_error);
  span = buf.span(2);
  ASSERT_EQ(span.size(), 2);
  ASSERT_EQ(span.data()[0], 'b');
  ASSERT_EQ(span.data()[1], 'c');
  buf.erase(1);
  EXPECT_THROW(buf.span(2), std::runtime_error);
  span = buf.span(1);
  ASSERT_EQ(span.size(), 1);
  ASSERT_EQ(span.data()[0], 'c');
  EXPECT_THROW(buf.erase(2), std::runtime_error);
  buf.erase(1);
  EXPECT_EQ(buf.size(), 0);
  EXPECT_THROW(buf.span(1), std::runtime_error);
}

TEST(net_buffer, existing_capacity)
{
  epee::net_utils::buffer buf;

  buf.append("123456789", 9);
  buf.erase(9);
  buf.append("abc", 3);
  buf.append("def", 3);
  ASSERT_EQ(buf.size(), 6);
  epee::span<const uint8_t> span = buf.span(6);
  ASSERT_TRUE(!memcmp(span.data(), "abcdef", 6));
}

TEST(net_buffer, reallocate)
{
  epee::net_utils::buffer buf;

  buf.append(std::string(4000, ' ').c_str(), 4000);
  buf.append(std::string(8000, '0').c_str(), 8000);
  ASSERT_EQ(buf.size(), 12000);
  epee::span<const uint8_t> span = buf.span(12000);
  ASSERT_TRUE(!memcmp(span.data(), std::string(4000, ' ').c_str(), 4000));
  ASSERT_TRUE(!memcmp(span.data() + 4000, std::string(8000, '0').c_str(), 8000));
}

TEST(net_buffer, move)
{
  epee::net_utils::buffer buf;

  buf.append(std::string(400, ' ').c_str(), 400);
  buf.erase(399);
  buf.append(std::string(4000, '0').c_str(), 4000);
  ASSERT_EQ(buf.size(), 4001);
  epee::span<const uint8_t> span = buf.span(4001);
  ASSERT_TRUE(!memcmp(span.data(), std::string(1, ' ').c_str(), 1));
  ASSERT_TRUE(!memcmp(span.data() + 1, std::string(4000, '0').c_str(), 4000));
}

TEST(parsing, isspace)
{
  ASSERT_FALSE(epee::misc_utils::parse::isspace(0));
  for (int c = 1; c < 256; ++c)
  {
    ASSERT_EQ(epee::misc_utils::parse::isspace(c), strchr("\r\n\t\f\v ", c) != NULL);
  }
}

TEST(parsing, isdigit)
{
  ASSERT_FALSE(epee::misc_utils::parse::isdigit(0));
  for (int c = 1; c < 256; ++c)
  {
    ASSERT_EQ(epee::misc_utils::parse::isdigit(c), strchr("0123456789", c) != NULL);
  }
}

TEST(parsing, number)
{
  boost::string_ref val;
  std::string s;
  std::string::const_iterator i;

  // the parser expects another character to end the number, and accepts things
  // that aren't numbers, as it's meant as a pre-filter for strto* functions,
  // so we just check that numbers get accepted, but don't test non numbers

  s = "0 ";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "0");

  s = "000 ";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "000");

  s = "10x";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "10");

  s = "10.09/";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "10.09");

  s = "-1.r";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "-1.");

  s = "-49.;";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "-49.");

  s = "0.78/";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "0.78");

  s = "33E9$";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "33E9");

  s = ".34e2=";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, ".34e2");

  s = "-9.34e-2=";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "-9.34e-2");

  s = "+9.34e+03=";
  i = s.begin();
  epee::misc_utils::parse::match_number(i, s.end(), val);
  ASSERT_EQ(val, "+9.34e+03");
}

TEST(parsing, unicode)
{
  std::string bs;
  std::string s;
  std::string::const_iterator si;

  s = "\"\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, "");
  s = "\"\\u0000\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, std::string(1, '\0'));
  s = "\"\\u0020\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, " ");
  s = "\"\\u1\""; si = s.begin(); ASSERT_FALSE(epee::misc_utils::parse::match_string(si, s.end(), bs));
  s = "\"\\u12\""; si = s.begin(); ASSERT_FALSE(epee::misc_utils::parse::match_string(si, s.end(), bs));
  s = "\"\\u123\""; si = s.begin(); ASSERT_FALSE(epee::misc_utils::parse::match_string(si, s.end(), bs));
  s = "\"\\u1234\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, "ሴ");
  s = "\"foo\\u1234bar\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, "fooሴbar");
  s = "\"\\u3042\\u307e\\u3084\\u304b\\u3059\""; si = s.begin(); ASSERT_TRUE(epee::misc_utils::parse::match_string(si, s.end(), bs)); ASSERT_EQ(bs, "あまやかす");
}
