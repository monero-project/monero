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

#include "int-util.h"

namespace
{
  TEST(mul128, handles_zero)
  {
    uint64_t hi, lo;

    lo = mul128(0, 0, &hi);
    ASSERT_EQ(lo, 0);
    ASSERT_EQ(hi, 0);

    lo = mul128(7, 0, &hi);
    ASSERT_EQ(lo, 0);
    ASSERT_EQ(hi, 0);

    lo = mul128(0, 7, &hi);
    ASSERT_EQ(lo, 0);
    ASSERT_EQ(hi, 0);
  }

  TEST(mul128, handles_one)
  {
    uint64_t hi, lo;

    lo = mul128(1, 1, &hi);
    ASSERT_EQ(lo, 1);
    ASSERT_EQ(hi, 0);

    lo = mul128(7, 1, &hi);
    ASSERT_EQ(lo, 7);
    ASSERT_EQ(hi, 0);

    lo = mul128(1, 7, &hi);
    ASSERT_EQ(lo, 7);
    ASSERT_EQ(hi, 0);
  }

  TEST(mul128_without_carry, multiplies_correctly)
  {
    uint64_t hi, lo;

    lo = mul128(0x3333333333333333, 5, &hi);
    ASSERT_EQ(lo, 0xffffffffffffffff);
    ASSERT_EQ(hi, 0);

    lo = mul128(5, 0x3333333333333333, &hi);
    ASSERT_EQ(lo, 0xffffffffffffffff);
    ASSERT_EQ(hi, 0);

    lo = mul128(0x1111111111111111, 0x1111111111111111, &hi);
    ASSERT_EQ(lo, 0x0fedcba987654321);
    ASSERT_EQ(hi, 0x0123456789abcdf0);;
  }

  TEST(mul128_with_carry_1_only, multiplies_correctly)
  {
    uint64_t hi, lo;

    lo = mul128(0xe0000000e0000000, 0xe0000000e0000000, &hi);
    ASSERT_EQ(lo, 0xc400000000000000);
    ASSERT_EQ(hi, 0xc400000188000000);
  }

  TEST(mul128_with_carry_2_only, multiplies_correctly)
  {
    uint64_t hi, lo;

    lo = mul128(0x10000000ffffffff, 0x10000000ffffffff, &hi);
    ASSERT_EQ(lo, 0xdffffffe00000001);
    ASSERT_EQ(hi, 0x0100000020000000);
  }

  TEST(mul128_with_carry_1_and_2, multiplies_correctly)
  {
    uint64_t hi, lo;

    lo = mul128(0xf1f2f3f4f5f6f7f8, 0xf9f0fafbfcfdfeff, &hi);
    ASSERT_EQ(lo, 0x52118e5f3b211008);
    ASSERT_EQ(hi, 0xec39104363716e59);

    lo = mul128(0xffffffffffffffff, 0xffffffffffffffff, &hi);
    ASSERT_EQ(lo, 0x0000000000000001);
    ASSERT_EQ(hi, 0xfffffffffffffffe);
  }

  TEST(div128_32, handles_zero)
  {
    uint32_t reminder;
    uint64_t hi;
    uint64_t lo;

    reminder = div128_32(0, 0, 7, &hi, &lo);
    ASSERT_EQ(reminder, 0);
    ASSERT_EQ(hi, 0);
    ASSERT_EQ(lo, 0);

    // Division by zero is UB, so can be tested correctly
  }

  TEST(div128_32, handles_one)
  {
    uint32_t reminder;
    uint64_t hi;
    uint64_t lo;

    reminder = div128_32(0, 7, 1, &hi, &lo);
    ASSERT_EQ(reminder, 0);
    ASSERT_EQ(hi, 0);
    ASSERT_EQ(lo, 7);

    reminder = div128_32(7, 0, 1, &hi, &lo);
    ASSERT_EQ(reminder, 0);
    ASSERT_EQ(hi, 7);
    ASSERT_EQ(lo, 0);
  }

  TEST(div128_32, handles_if_dividend_less_divider)
  {
    uint32_t reminder;
    uint64_t hi;
    uint64_t lo;

    reminder = div128_32(0, 1383746, 1645825, &hi, &lo);
    ASSERT_EQ(reminder, 1383746);
    ASSERT_EQ(hi, 0);
    ASSERT_EQ(lo, 0);
  }

  TEST(div128_32, handles_if_dividend_dwords_less_divider)
  {
    uint32_t reminder;
    uint64_t hi;
    uint64_t lo;

    reminder = div128_32(0x5AD629E441074F28, 0x0DBCAB2B231081F1, 0xFE735CD6, &hi, &lo);
    ASSERT_EQ(reminder, 0xB9C924E9);
    ASSERT_EQ(hi, 0x000000005B63C274);
    ASSERT_EQ(lo, 0x9084FC024383E48C);
  }

  TEST(div128_32, works_correctly)
  {
    uint32_t reminder;
    uint64_t hi;
    uint64_t lo;

    reminder = div128_32(2, 0, 2, &hi, &lo);
    ASSERT_EQ(reminder, 0);
    ASSERT_EQ(hi, 1);
    ASSERT_EQ(lo, 0);

    reminder = div128_32(0xffffffffffffffff, 0, 0xffffffff, &hi, &lo);
    ASSERT_EQ(reminder, 0);
    ASSERT_EQ(hi, 0x0000000100000001);
    ASSERT_EQ(lo, 0);

    reminder = div128_32(0xffffffffffffffff, 5846, 0xffffffff, &hi, &lo);
    ASSERT_EQ(reminder, 5846);
    ASSERT_EQ(hi, 0x0000000100000001);
    ASSERT_EQ(lo, 0);

    reminder = div128_32(0xffffffffffffffff - 1, 0, 0xffffffff, &hi, &lo);
    ASSERT_EQ(reminder, 0xfffffffe);
    ASSERT_EQ(hi, 0x0000000100000000);
    ASSERT_EQ(lo, 0xfffffffefffffffe);

    reminder = div128_32(0x2649372534875028, 0xaedbfedc5adbc739, 0x27826534, &hi, &lo);
    ASSERT_EQ(reminder, 0x1a6dc2e5);
    ASSERT_EQ(hi, 0x00000000f812c1f8);
    ASSERT_EQ(lo, 0xddf2fdb09bc2e2e9);
  }
}
