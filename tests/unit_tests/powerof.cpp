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

#include "gtest/gtest.h"
#include "common/powerof.h"

TEST(powerof, base_cases)
{
  // Anything to the power of 0 is 1
  EXPECT_EQ(1u, (tools::PowerOf<2, 0>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<10, 0>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<1, 0>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<100, 0>::Value));
}

TEST(powerof, powers_of_two)
{
  EXPECT_EQ(2u, (tools::PowerOf<2, 1>::Value));
  EXPECT_EQ(4u, (tools::PowerOf<2, 2>::Value));
  EXPECT_EQ(8u, (tools::PowerOf<2, 3>::Value));
  EXPECT_EQ(16u, (tools::PowerOf<2, 4>::Value));
  EXPECT_EQ(1024u, (tools::PowerOf<2, 10>::Value));
  EXPECT_EQ(1048576u, (tools::PowerOf<2, 20>::Value));
}

TEST(powerof, powers_of_ten)
{
  EXPECT_EQ(10u, (tools::PowerOf<10, 1>::Value));
  EXPECT_EQ(100u, (tools::PowerOf<10, 2>::Value));
  EXPECT_EQ(1000u, (tools::PowerOf<10, 3>::Value));
  EXPECT_EQ(1000000u, (tools::PowerOf<10, 6>::Value));
  EXPECT_EQ(1000000000000u, (tools::PowerOf<10, 12>::Value));
}

TEST(powerof, one_to_any_power)
{
  EXPECT_EQ(1u, (tools::PowerOf<1, 0>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<1, 1>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<1, 10>::Value));
  EXPECT_EQ(1u, (tools::PowerOf<1, 50>::Value));
}

TEST(powerof, miscellaneous)
{
  EXPECT_EQ(27u, (tools::PowerOf<3, 3>::Value));
  EXPECT_EQ(625u, (tools::PowerOf<5, 4>::Value));
  EXPECT_EQ(7776u, (tools::PowerOf<6, 5>::Value));
  EXPECT_EQ(256u, (tools::PowerOf<4, 4>::Value));
}

TEST(powerof, compile_time_constexpr)
{
  // Verify these are usable as compile-time constants
  static_assert(tools::PowerOf<2, 8>::Value == 256, "2^8 should be 256");
  static_assert(tools::PowerOf<10, 3>::Value == 1000, "10^3 should be 1000");
  static_assert(tools::PowerOf<3, 0>::Value == 1, "3^0 should be 1");
}
