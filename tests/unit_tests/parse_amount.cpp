// Copyright (c) 2014-2018, The Monero Project
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

#include "cryptonote_basic/cryptonote_format_utils.h"

using namespace cryptonote;

namespace
{
  void do_pos_test(uint64_t expected, const std::string& str)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_TRUE(parse_amount(val, number_str));
    ASSERT_EQ(expected, val);
  }

  void do_neg_test(const std::string& str)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_FALSE(parse_amount(val, number_str));
  }
}

#define TEST_pos(expected, str)            \
  TEST(parse_amount, handles_pos_ ## str)  \
  {                                        \
    do_pos_test(UINT64_C(expected), #str); \
  }

#define TEST_neg(str)                      \
  TEST(parse_amount, handles_neg_ ## str)  \
  {                                        \
    do_neg_test(#str);                     \
  }

#define TEST_neg_n(str, name)              \
  TEST(parse_amount, handles_neg_ ## name) \
  {                                        \
    do_neg_test(#str);                     \
  }


TEST_pos(0, 0);
TEST_pos(0, 00);
TEST_pos(0, 00000000);
TEST_pos(0, 000000000);
TEST_pos(0, 00000000000000000000000000000000);

TEST_pos(0, _0);
TEST_pos(0, _00);
TEST_pos(0, _00000000);
TEST_pos(0, _000000000);
TEST_pos(0, _00000000000000000000000000000000);

TEST_pos(0, 00000000_);
TEST_pos(0, 000000000_);
TEST_pos(0, 00000000000000000000000000000000_);

TEST_pos(0, 0_);
TEST_pos(0, 0_0);
TEST_pos(0, 0_00);
TEST_pos(0, 0_00000000);
TEST_pos(0, 0_000000000);
TEST_pos(0, 0_00000000000000000000000000000000);

TEST_pos(0, 00_);
TEST_pos(0, 00_0);
TEST_pos(0, 00_00);
TEST_pos(0, 00_00000000);
TEST_pos(0, 00_000000000);
TEST_pos(0, 00_00000000000000000000000000000000);

TEST_pos(1, 0_000000000001);
TEST_pos(1, 0_0000000000010);
TEST_pos(1, 0_0000000000010000000000000000000000000);
TEST_pos(9, 0_000000000009);
TEST_pos(9, 0_0000000000090);
TEST_pos(9, 0_0000000000090000000000000000000000000);

TEST_pos(1000000000000,        1);
TEST_pos(10000000000000,       10);
TEST_pos(100000000000000,      100);
TEST_pos(1000000000000000,     1000);
TEST_pos(6553500000000000,     6553_5);
TEST_pos(429496729500000000,   429496_7295);
TEST_pos(18446744073700000000, 18446744_0737);
TEST_pos(18446744073700000000, 18446744_0737000);
TEST_pos(18446744073700000000, 18446744_07370000);
TEST_pos(18446744073700000000, 18446744_073700000);
TEST_pos(18446744073700000000, 18446744_0737000000000000000);

/* Max supply */
TEST_pos(18446744073709551615, 18446744_073709551615);

// Invalid numbers
TEST_neg_n(~, empty_string);
TEST_neg_n(-0, minus_0);
TEST_neg_n(+0, plus_0);
TEST_neg_n(-1, minus_1);
TEST_neg_n(+1, plus_1);
TEST_neg_n(_, only_point);

// Don't go below 10^-12
TEST_neg(0_0000000000001);
TEST_neg(0_0000000000009);
TEST_neg(184467440737_000000001);

// Overflow
TEST_neg(184467440737_09551616);
TEST_neg(184467440738);
TEST_neg(18446744073709551616);

// Two or more points
TEST_neg(__);
TEST_neg(0__);
TEST_neg(__0);
TEST_neg(0__0);
TEST_neg(0_0_);
TEST_neg(_0_0);
TEST_neg(0_0_0);
