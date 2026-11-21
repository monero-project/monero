// Copyright (c) 2026, The Monero Project
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

#include "simplewallet/string_width.h"

TEST(SplitStringByWidth, ascii_wraps_on_columns)
{
  const auto lines = tools::split_string_by_width("the quick brown fox", 10);
  ASSERT_EQ(lines.size(), 2);
  ASSERT_EQ(lines[0].first, "the quick");
  ASSERT_EQ(lines[0].second, 9);
  ASSERT_EQ(lines[1].first, "brown fox");
  ASSERT_EQ(lines[1].second, 9);
}

TEST(SplitStringByWidth, narrow_multibyte_counts_as_one_column)
{
  // "héllo" has 5 codepoints but 6 bytes (é is 2 bytes), and should occupy 5 columns, not 6
  const auto lines = tools::split_string_by_width("h\xc3\xa9llo", 80);
  ASSERT_EQ(lines.size(), 1);
  ASSERT_EQ(lines[0].first, "h\xc3\xa9llo");
  ASSERT_EQ(lines[0].second, 5);
}

TEST(SplitStringByWidth, wide_cjk_counts_as_two_columns)
{
  // Three CJK codepoints (U+4E2D each encoded as 3 bytes in UTF-8), each 2 columns wide
  const std::string cjk3 = "\xe4\xb8\xad\xe4\xb8\xad\xe4\xb8\xad";
  ASSERT_EQ(tools::get_string_width(cjk3), 6);

  // With a 4 column budget, only two wide characters (4 columns) fit on the first line
  const auto lines = tools::split_string_by_width(cjk3, 4);
  ASSERT_EQ(lines.size(), 2);
  ASSERT_EQ(lines[0].first, "\xe4\xb8\xad\xe4\xb8\xad");
  ASSERT_EQ(lines[0].second, 4);
  ASSERT_EQ(lines[1].first, "\xe4\xb8\xad");
  ASSERT_EQ(lines[1].second, 2);
}

TEST(SplitStringByWidth, invalid_utf8_falls_back_to_byte_length)
{
  // a lone continuation byte is not valid UTF-8 on its own
  const std::string invalid = "\x80\x80";
  const auto lines = tools::split_string_by_width(invalid, 80);
  ASSERT_EQ(lines.size(), 1);
  ASSERT_EQ(lines[0].first, invalid);
  ASSERT_EQ(lines[0].second, invalid.size());
}
