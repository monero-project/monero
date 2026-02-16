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
#include "common/combinator.h"

#include <algorithm>
#include <set>
#include <string>

TEST(combinator, combinations_count_basic)
{
  // C(5,2) = 10
  EXPECT_EQ(10u, tools::combinations_count(2, 5));
  // C(4,4) = 1
  EXPECT_EQ(1u, tools::combinations_count(4, 4));
  // C(1,10) = 10
  EXPECT_EQ(10u, tools::combinations_count(1, 10));
  // C(0,5) = 1
  EXPECT_EQ(1u, tools::combinations_count(0, 5));
  // C(0,0) = 1
  EXPECT_EQ(1u, tools::combinations_count(0, 0));
  // C(6,6) = 1
  EXPECT_EQ(1u, tools::combinations_count(6, 6));
  // C(3,7) = 35
  EXPECT_EQ(35u, tools::combinations_count(3, 7));
}

TEST(combinator, combinations_count_k_greater_than_n_throws)
{
  EXPECT_THROW(tools::combinations_count(5, 3), std::runtime_error);
}

TEST(combinator, combine_choose_1_from_3)
{
  std::vector<int> v = {10, 20, 30};
  tools::Combinator<int> comb(v);
  auto result = comb.combine(1);
  EXPECT_EQ(3u, result.size());
  // Each combination should have exactly 1 element
  for (const auto &c : result)
    EXPECT_EQ(1u, c.size());
}

TEST(combinator, combine_choose_2_from_4)
{
  std::vector<int> v = {1, 2, 3, 4};
  tools::Combinator<int> comb(v);
  auto result = comb.combine(2);
  // C(4,2) = 6
  EXPECT_EQ(6u, result.size());
  // Verify all combinations are unique
  std::set<std::vector<int>> unique_combos(result.begin(), result.end());
  EXPECT_EQ(6u, unique_combos.size());
  // Each should have 2 elements
  for (const auto &c : result)
    EXPECT_EQ(2u, c.size());
}

TEST(combinator, combine_choose_all)
{
  std::vector<int> v = {1, 2, 3};
  tools::Combinator<int> comb(v);
  auto result = comb.combine(3);
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ(v, result[0]);
}

TEST(combinator, combine_k_zero_throws)
{
  std::vector<int> v = {1, 2};
  tools::Combinator<int> comb(v);
  EXPECT_THROW(comb.combine(0), std::runtime_error);
}

TEST(combinator, combine_k_too_large_throws)
{
  std::vector<int> v = {1, 2};
  tools::Combinator<int> comb(v);
  EXPECT_THROW(comb.combine(3), std::runtime_error);
}

TEST(combinator, combine_preserves_element_order)
{
  std::vector<std::string> v = {"a", "b", "c", "d"};
  tools::Combinator<std::string> comb(v);
  auto result = comb.combine(2);
  // Each combination should have elements in their original relative order
  for (const auto &c : result)
  {
    ASSERT_EQ(2u, c.size());
    auto it1 = std::find(v.begin(), v.end(), c[0]);
    auto it2 = std::find(v.begin(), v.end(), c[1]);
    EXPECT_LT(it1, it2);
  }
}

TEST(combinator, combine_count_matches_combinations_count)
{
  std::vector<int> v = {1, 2, 3, 4, 5, 6};
  tools::Combinator<int> comb(v);
  for (size_t k = 1; k <= v.size(); ++k)
  {
    auto result = comb.combine(k);
    EXPECT_EQ(tools::combinations_count(k, v.size()), result.size());
  }
}

TEST(combinator, combine_single_element_vector)
{
  std::vector<int> v = {42};
  tools::Combinator<int> comb(v);
  auto result = comb.combine(1);
  ASSERT_EQ(1u, result.size());
  ASSERT_EQ(1u, result[0].size());
  EXPECT_EQ(42, result[0][0]);
}
