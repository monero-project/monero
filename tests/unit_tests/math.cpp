// Copyright (c) 2021, The Monero Project
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
#include <limits>

#include <boost/math/special_functions/binomial.hpp>
#include "gtest/gtest.h"

#include "math.h"
#include "misc_language.h"

TEST(misc_language, get_mid)
{
  ASSERT_EQ(epee::misc_utils::get_mid<int>(-1,-1) , 1);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(-1,0), 0);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(0,-1), 0);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(1,0), 0);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(0,1), 0);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(1,1), 1);

  ASSERT_EQ(epee::misc_utils::get_mid<int>(-4,-2) , 3);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(4,2), 3);
  ASSERT_EQ(epee::misc_utils::get_mid<int>(5,2), 3);

  EXPECT_FLOAT_EQ(epee::misc_utils::get_mid<double>(5.0,2.0), 3.5);

  // overflow test
  ASSERT_EQ(epee::misc_utils::get_mid<std::uint32_t>(
    std::numeric_limits<std::uint32_t>::max(),
    std::numeric_limits<std::uint32_t>::max() - 2)
    ,
    std::numeric_limits<std::uint32_t>::max() - 1);
}

TEST(math, median)
{
  std::vector<size_t> sz;
  size_t m = cryptonote::median(sz);
  ASSERT_EQ(m, 0);
  sz.push_back(1);
  m = cryptonote::median(sz);
  ASSERT_EQ(m, 1);
  sz.push_back(10);
  m = cryptonote::median(sz);
  ASSERT_EQ(m, 5);
  
  sz.clear();
  sz.resize(3);
  sz[0] = 0;
  sz[1] = 9;
  sz[2] = 3;
  m = cryptonote::median(sz);
  ASSERT_EQ(m, 3);

  sz.clear();
  sz.resize(4);
  sz[0] = 77;
  sz[1] = 9;
  sz[2] = 22;
  sz[3] = 60;
  m = cryptonote::median(sz);
  ASSERT_EQ(m, 41);

  sz.clear();
  sz.resize(5);
  sz[0] = 77;
  sz[1] = 9;
  sz[2] = 22;
  sz[3] = 60;
  sz[4] = 11;
  m = cryptonote::median(sz);
  ASSERT_EQ(m, 22);
}

TEST(math, n_choose_k)
{
  ASSERT_EQ(cryptonote::n_choose_k(0,0), 1);

  ASSERT_EQ(cryptonote::n_choose_k(1,0), 1);
  ASSERT_EQ(cryptonote::n_choose_k(1,1), 1);
  ASSERT_EQ(cryptonote::n_choose_k(1,2), 0);

  ASSERT_EQ(cryptonote::n_choose_k(2,0), 1);
  ASSERT_EQ(cryptonote::n_choose_k(2,1), 2);
  ASSERT_EQ(cryptonote::n_choose_k(2,2), 1);

  ASSERT_EQ(cryptonote::n_choose_k(10,1), 10);
  ASSERT_EQ(cryptonote::n_choose_k(10,2), 45);
  ASSERT_EQ(cryptonote::n_choose_k(10,3), 120);
  ASSERT_EQ(cryptonote::n_choose_k(10,4), 210);
  ASSERT_EQ(cryptonote::n_choose_k(10,5), 252);
  ASSERT_EQ(cryptonote::n_choose_k(10,6), 210);
  ASSERT_EQ(cryptonote::n_choose_k(10,7), 120);
  ASSERT_EQ(cryptonote::n_choose_k(10,8), 45);
  ASSERT_EQ(cryptonote::n_choose_k(10,9), 10);
  ASSERT_EQ(cryptonote::n_choose_k(10,10), 1);

  // numerical limits
  // a != b
  ASSERT_NE(cryptonote::n_choose_k(33,16), 0);
  ASSERT_EQ(cryptonote::n_choose_k(34,16), 0);

  double fp_result_good = boost::math::binomial_coefficient<double>(33, 16);
  double fp_result_overflow = boost::math::binomial_coefficient<double>(34, 16);

  // a <= b
  ASSERT_LE(fp_result_good, std::numeric_limits<std::int32_t>::max());
  // a > b
  ASSERT_GT(fp_result_overflow, std::numeric_limits<std::int32_t>::max());
}


