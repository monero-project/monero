// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include <string_tools.h>
#include <string_tools_lexical.h>

using namespace epee::string_tools;

namespace
{
  template<typename T>
  void do_pos_test(T expected, const std::string& str)
  {
    T val;
    ASSERT_TRUE(get_xtype_from_string(val, str));
    ASSERT_EQ(expected, val);
  }

  template<typename T>
  void do_neg_test(const std::string& str)
  {
    T val;
    ASSERT_FALSE(get_xtype_from_string(val, str));
  }
}

#define TEST_pos(int_type, expected, str)                                \
  TEST(get_xtype_from_string, handles_pos_ ## int_type ## _ ## expected) \
  {                                                                      \
    do_pos_test<int_type>(expected, str);                                \
  }

#define DO_MAKE_NEG_TEST_NAME(prefix, int_type, ln) prefix ## int_type ## _ ## ln
#define MAKE_NEG_TEST_NAME(prefix, int_type, ln) DO_MAKE_NEG_TEST_NAME(prefix, int_type, ln)

#define TEST_neg(int_type, str)                                                    \
  TEST(get_xtype_from_string, MAKE_NEG_TEST_NAME(handles_neg, int_type, __LINE__)) \
  {                                                                                \
    do_neg_test<int_type>(str);                                                    \
  }

TEST_pos(uint16_t,     0,     "0");
TEST_pos(uint16_t,     1,     "1");
TEST_pos(uint16_t, 65535, "65535");

TEST_neg(uint16_t, "");
TEST_neg(uint16_t, "+0");
TEST_neg(uint16_t, "+1");
TEST_neg(uint16_t, "+65535");
TEST_neg(uint16_t, "+65536");

TEST_neg(uint16_t, "-0");
TEST_neg(uint16_t, "-1");
TEST_neg(uint16_t, "-65535");
TEST_neg(uint16_t, "-65536");

TEST_neg(uint16_t, ".0");
TEST_neg(uint16_t, ".1");
TEST_neg(uint16_t, "0.0");
TEST_neg(uint16_t, "0.1");
TEST_neg(uint16_t, "1.0");
TEST_neg(uint16_t, "1.1");

TEST_neg(uint16_t, "w");
TEST_neg(uint16_t, "0w");
TEST_neg(uint16_t, "1w");
TEST_neg(uint16_t, "1w1");
TEST_neg(uint16_t, "65535w");

TEST_neg(uint16_t, "65536");
TEST_neg(uint16_t, "4294967296");
TEST_neg(uint16_t, "18446744073709551616");


TEST_pos(uint32_t,          0,          "0");
TEST_pos(uint32_t,          1,          "1");
TEST_pos(uint32_t, 4294967295, "4294967295");

TEST_neg(uint32_t, "");
TEST_neg(uint32_t, "+0");
TEST_neg(uint32_t, "+1");
TEST_neg(uint32_t, "+4294967295");
TEST_neg(uint32_t, "+4294967296");

TEST_neg(uint32_t, "-0");
TEST_neg(uint32_t, "-1");
TEST_neg(uint32_t, "-4294967295");
TEST_neg(uint32_t, "-4294967296");

TEST_neg(uint32_t, ".0");
TEST_neg(uint32_t, ".1");
TEST_neg(uint32_t, "0.0");
TEST_neg(uint32_t, "0.1");
TEST_neg(uint32_t, "1.0");
TEST_neg(uint32_t, "1.1");

TEST_neg(uint32_t, "w");
TEST_neg(uint32_t, "0w");
TEST_neg(uint32_t, "1w");
TEST_neg(uint32_t, "1w1");
TEST_neg(uint32_t, "4294967295w");

TEST_neg(uint32_t, "4294967296");
TEST_neg(uint32_t, "18446744073709551616");

TEST_pos(uint64_t, 0, "0");
TEST_pos(uint64_t, 1, "1");
TEST_pos(uint64_t, 18446744073709551615ULL, "18446744073709551615");

TEST_neg(uint64_t, "");
TEST_neg(uint64_t, "+0");
TEST_neg(uint64_t, "+1");
TEST_neg(uint64_t, "+18446744073709551615");
TEST_neg(uint64_t, "+18446744073709551616");

TEST_neg(uint64_t, "-0");
TEST_neg(uint64_t, "-1");
TEST_neg(uint64_t, "-18446744073709551615");
TEST_neg(uint64_t, "-18446744073709551616");

TEST_neg(uint64_t, ".0");
TEST_neg(uint64_t, ".1");
TEST_neg(uint64_t, "0.0");
TEST_neg(uint64_t, "0.1");
TEST_neg(uint64_t, "1.0");
TEST_neg(uint64_t, "1.1");

TEST_neg(uint64_t, "w");
TEST_neg(uint64_t, "0w");
TEST_neg(uint64_t, "1w");
TEST_neg(uint64_t, "1w1");
TEST_neg(uint64_t, "18446744073709551615w");

TEST_neg(uint64_t, "18446744073709551616");
