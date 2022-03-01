// Copyright (c) 2017-2022, The Monero Project
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
#include "common/apply_permutation.h"

TEST(apply_permutation, empty)
{
  std::vector<int> v = {};
  tools::apply_permutation({}, v);
  ASSERT_EQ(v, std::vector<int>({}));
}

TEST(apply_permutation, reorder)
{
  //                    0  1  2  3  4  5  6
  std::vector<int> v = {8, 4, 6, 1, 7, 2, 4};
  tools::apply_permutation({3, 5, 6, 1, 2, 4, 0}, v);
  ASSERT_EQ(v, std::vector<int>({1, 2, 4, 4, 6, 7, 8}));
}

TEST(apply_permutation, bad_size)
{
  std::vector<int> v_large = {8, 4, 6, 1, 7, 2, 4, 9};
  std::vector<int> v_small = {8, 4, 6, 1, 7, 2};
  try
  { 
    tools::apply_permutation({3, 5, 6, 1, 2, 4, 0}, v_large);
    ASSERT_FALSE(true);
  }
  catch (const std::exception &e) {}
  try
  { 
    tools::apply_permutation({3, 5, 6, 1, 2, 4, 0}, v_small);
    ASSERT_FALSE(true);
  }
  catch (const std::exception &e) {}
}

TEST(apply_permutation, bad_permutation)
{
  std::vector<int> v = {8, 4, 6, 1, 7, 2, 4};
  try
  { 
    tools::apply_permutation({3, 5, 6, 1, 2, 4, 1}, v);
    ASSERT_FALSE(true);
  }
  catch (const std::exception &e) {}
}
