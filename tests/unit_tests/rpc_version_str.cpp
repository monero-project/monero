// Copyright (c) 2019-2024, The Monero Project
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

#include "rpc/rpc_version_str.h"
#include "version.h"

TEST(rpc, is_version_string_valid)
{
  using namespace cryptonote::rpc;
  ASSERT_TRUE(is_version_string_valid(MONERO_VERSION));
  ASSERT_TRUE(is_version_string_valid("0.14.1.2"));
  ASSERT_TRUE(is_version_string_valid("0.15.0.0-release"));
  ASSERT_TRUE(is_version_string_valid("0.15.0.0-fe3f6a3e6"));

  ASSERT_FALSE(is_version_string_valid(""));
  ASSERT_FALSE(is_version_string_valid("invalid"));
  ASSERT_FALSE(is_version_string_valid("0.15.0.0-invalid"));
  ASSERT_FALSE(is_version_string_valid("0.15.0.0-release0"));
  ASSERT_FALSE(is_version_string_valid("0.15.0.0-release "));
  ASSERT_FALSE(is_version_string_valid("0.15.0.0-fe3f6a3e60"));
  ASSERT_FALSE(is_version_string_valid("0.15.0.0-fe3f6a3e6 "));
}
