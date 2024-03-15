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
#include "common/command_line.h"

TEST(CommandLine, IsYes)
{
  EXPECT_TRUE(command_line::is_yes("Y"));
  EXPECT_TRUE(command_line::is_yes("y"));
  EXPECT_TRUE(command_line::is_yes("YES"));
  EXPECT_TRUE(command_line::is_yes("YEs"));
  EXPECT_TRUE(command_line::is_yes("YeS"));
  EXPECT_TRUE(command_line::is_yes("yES"));
  EXPECT_TRUE(command_line::is_yes("Yes"));
  EXPECT_TRUE(command_line::is_yes("yeS"));
  EXPECT_TRUE(command_line::is_yes("yEs"));
  EXPECT_TRUE(command_line::is_yes("yes"));

  EXPECT_FALSE(command_line::is_yes(""));
  EXPECT_FALSE(command_line::is_yes("yes-"));
  EXPECT_FALSE(command_line::is_yes("NO"));
  EXPECT_FALSE(command_line::is_yes("No"));
  EXPECT_FALSE(command_line::is_yes("nO"));
  EXPECT_FALSE(command_line::is_yes("no"));
}
