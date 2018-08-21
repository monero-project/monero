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
#include "common/is_hdd.h"
#include <string>
#include <gtest/gtest.h>

#if defined(__GLIBC__)
TEST(is_hdd, linux_os_root)
{
  bool result;
  std::string path = "/";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
}
#elif defined(_WIN32) and (_WIN32_WINNT >= 0x0601)
TEST(is_hdd, win_os_c)
{
  bool result;
  std::string path = "\\\\?\\C:\\Windows\\System32\\cmd.exe";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
  path = "\\\\?\\C:\\";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
  path = "C:\\";
  EXPECT_TRUE(tools::is_hdd(path.c_str(), result));
}
#else
TEST(is_hdd, unknown_os)
{
  bool result;
  std::string path = "";
  EXPECT_FALSE(tools::is_hdd(path.c_str(), result));
}
#endif
