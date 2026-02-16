// Copyright (c) 2022-2024, The Monero Project
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
#include "common/container_helpers.h"

#include <string>
#include <vector>

// -- is_sorted_and_unique tests --

TEST(container_helpers, is_sorted_and_unique_empty)
{
  std::vector<int> v;
  EXPECT_TRUE(tools::is_sorted_and_unique(v));
}

TEST(container_helpers, is_sorted_and_unique_single)
{
  std::vector<int> v = {42};
  EXPECT_TRUE(tools::is_sorted_and_unique(v));
}

TEST(container_helpers, is_sorted_and_unique_sorted_unique)
{
  std::vector<int> v = {1, 2, 3, 4, 5};
  EXPECT_TRUE(tools::is_sorted_and_unique(v));
}

TEST(container_helpers, is_sorted_and_unique_unsorted)
{
  std::vector<int> v = {1, 3, 2};
  EXPECT_FALSE(tools::is_sorted_and_unique(v));
}

TEST(container_helpers, is_sorted_and_unique_duplicates)
{
  std::vector<int> v = {1, 2, 2, 3};
  EXPECT_FALSE(tools::is_sorted_and_unique(v));
}

TEST(container_helpers, is_sorted_and_unique_custom_comparator)
{
  // Descending order
  std::vector<int> v = {5, 4, 3, 2, 1};
  EXPECT_TRUE(tools::is_sorted_and_unique(v, std::greater<int>{}));
}

TEST(container_helpers, is_sorted_and_unique_custom_comparator_with_dups)
{
  std::vector<int> v = {5, 4, 4, 2, 1};
  EXPECT_FALSE(tools::is_sorted_and_unique(v, std::greater<int>{}));
}

TEST(container_helpers, is_sorted_and_unique_strings)
{
  std::vector<std::string> v = {"apple", "banana", "cherry"};
  EXPECT_TRUE(tools::is_sorted_and_unique(v));

  std::vector<std::string> v2 = {"apple", "apple", "cherry"};
  EXPECT_FALSE(tools::is_sorted_and_unique(v2));
}

// -- keys_match_internal_values tests --

TEST(container_helpers, keys_match_internal_values_all_match)
{
  std::unordered_map<int, int> m = {{1, 2}, {3, 6}, {5, 10}};
  // Check that value == key * 2
  EXPECT_TRUE(tools::keys_match_internal_values(m,
    [](const int &key, const int &val) { return val == key * 2; }));
}

TEST(container_helpers, keys_match_internal_values_mismatch)
{
  std::unordered_map<int, int> m = {{1, 2}, {3, 7}, {5, 10}};
  EXPECT_FALSE(tools::keys_match_internal_values(m,
    [](const int &key, const int &val) { return val == key * 2; }));
}

TEST(container_helpers, keys_match_internal_values_empty)
{
  std::unordered_map<int, int> m;
  EXPECT_TRUE(tools::keys_match_internal_values(m,
    [](const int &key, const int &val) { return false; }));
}

// -- for_all_in_map_erase_if tests --

TEST(container_helpers, for_all_in_map_erase_if_remove_evens)
{
  std::unordered_map<int, std::string> m = {
    {1, "a"}, {2, "b"}, {3, "c"}, {4, "d"}
  };
  tools::for_all_in_map_erase_if(m,
    [](const std::pair<const int, std::string> &entry) { return entry.first % 2 == 0; });
  EXPECT_EQ(2u, m.size());
  EXPECT_TRUE(m.count(1));
  EXPECT_TRUE(m.count(3));
  EXPECT_FALSE(m.count(2));
  EXPECT_FALSE(m.count(4));
}

TEST(container_helpers, for_all_in_map_erase_if_remove_none)
{
  std::unordered_map<int, int> m = {{1, 1}, {2, 2}};
  tools::for_all_in_map_erase_if(m,
    [](const std::pair<const int, int> &) { return false; });
  EXPECT_EQ(2u, m.size());
}

TEST(container_helpers, for_all_in_map_erase_if_remove_all)
{
  std::unordered_map<int, int> m = {{1, 1}, {2, 2}};
  tools::for_all_in_map_erase_if(m,
    [](const std::pair<const int, int> &) { return true; });
  EXPECT_TRUE(m.empty());
}

TEST(container_helpers, for_all_in_map_erase_if_empty_map)
{
  std::unordered_map<int, int> m;
  tools::for_all_in_map_erase_if(m,
    [](const std::pair<const int, int> &) { return true; });
  EXPECT_TRUE(m.empty());
}

// -- compare_func / as_functor tests --

TEST(container_helpers, compare_func_basic)
{
  auto cmp = tools::compare_func<int>(std::less<int>{});
  EXPECT_TRUE(cmp(1, 2));
  EXPECT_FALSE(cmp(2, 1));
  EXPECT_FALSE(cmp(1, 1));
}

TEST(container_helpers, is_sorted_and_unique_with_function_pointer)
{
  std::vector<int> v = {1, 2, 3, 4};
  // Use the function pointer overload
  EXPECT_TRUE(tools::is_sorted_and_unique(v,
    [](const int &a, const int &b) -> bool { return a < b; }));
}
