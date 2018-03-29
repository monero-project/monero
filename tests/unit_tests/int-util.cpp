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

#include "common/int-util.h"

namespace
{
  TEST(int_util, int_log2)
  {
    for (uint64_t i = 0; i < 64; ++i)
      ASSERT_EQ(i, int_log2((uint64_t)1 << i));
  }

  TEST(int_util, rational_ceil)
  {
    ASSERT_EQ(0, rational_ceil(0, 1));
    ASSERT_EQ(1, rational_ceil(1, 1));
    ASSERT_EQ(2, rational_ceil(2, 1));
    ASSERT_EQ(3, rational_ceil(3, 1));

    ASSERT_EQ(0, rational_ceil(0, 4));
    ASSERT_EQ(1, rational_ceil(1, 4));
    ASSERT_EQ(1, rational_ceil(2, 4));
    ASSERT_EQ(1, rational_ceil(3, 4));
    ASSERT_EQ(1, rational_ceil(4, 4));
    ASSERT_EQ(2, rational_ceil(5, 4));
    ASSERT_EQ(2, rational_ceil(6, 4));
    ASSERT_EQ(2, rational_ceil(7, 4));
    ASSERT_EQ(2, rational_ceil(8, 4));
    ASSERT_EQ(3, rational_ceil(9, 4));
  }
}
