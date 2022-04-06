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

#include "common/util.h"

TEST(vercmp, empty) { ASSERT_TRUE(tools::vercmp("", "") == 0); }
TEST(vercmp, empty0) { ASSERT_TRUE(tools::vercmp("", "0") == 0); }
TEST(vercmp, empty1) { ASSERT_TRUE(tools::vercmp("0", "") == 0); }
TEST(vercmp, zero_zero) { ASSERT_TRUE(tools::vercmp("0", "0") == 0); }
TEST(vercmp, one_one) { ASSERT_TRUE(tools::vercmp("1", "1") == 0); }
TEST(vercmp, one_two) { ASSERT_TRUE(tools::vercmp("1", "2") < 0); }
TEST(vercmp, two_one) { ASSERT_TRUE(tools::vercmp("2", "1") > 0); }
TEST(vercmp, ten_nine) { ASSERT_TRUE(tools::vercmp("10", "9") > 0); }
TEST(vercmp, one_dot_ten_one_dot_nine) { ASSERT_TRUE(tools::vercmp("1.10", "1.9") > 0); }
TEST(vercmp, one_one_dot_nine) { ASSERT_TRUE(tools::vercmp("1", "1.9") < 0); }
TEST(vercmp, to_master) { ASSERT_TRUE(tools::vercmp("1.0", "1.0-master") < 0); }
TEST(vercmp, from_master) { ASSERT_TRUE(tools::vercmp("1.0-master", "1.1") < 0); }

