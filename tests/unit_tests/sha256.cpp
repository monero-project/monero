// Copyright (c) 2017-2024, The Monero Project
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

#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

#include "common/util.h"
#include "string_tools.h"
#include "unit_tests_utils.h"

static bool check(const std::string &data, const char *expected_hash_hex)
{
  crypto::hash hash, expected_hash;
  if (!epee::string_tools::hex_to_pod(expected_hash_hex, expected_hash))
    return false;
  return tools::sha256sum((const uint8_t*)data.data(), data.size(), hash) && hash == expected_hash;
}

static std::string file_to_hex_hash(const std::string& filename)
 {
   const boost::filesystem::path full_path = unit_test::data_dir / "sha256sum" / filename;

   crypto::hash hash;
   if (!tools::sha256sum(full_path.string(), hash)) {
     throw std::runtime_error("sha256sum failed");
   }

   const std::string data_cstr(hash.data, sizeof(hash.data));
   const std::string hex_hash = epee::string_tools::buff_to_hex_nodelimer(data_cstr);

   return hex_hash;
 }

TEST(sha256, empty) { ASSERT_TRUE(check(std::string(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")); }
TEST(sha256, small) { ASSERT_TRUE(check("0123456789", "84d89877f0d4041efb6bf91a16f0248f2fd573e6af05c19f96bedb9f882f7882")); }
TEST(sha256, large) { ASSERT_TRUE(check(std::string(65536*256, 0), "080acf35a507ac9849cfcba47dc2ad83e01b75663a516279c8b9d243b719643e")); }

TEST(sha256, emptyfile) { EXPECT_EQ(file_to_hex_hash("empty.txt"), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"); }
TEST(sha256, smallfile) { EXPECT_EQ(file_to_hex_hash("small_file.txt"), "91c60f6d9ad0235306115913febccb93a5014bf4cea1ecd1fa33f3cf07ad9e8d"); }
TEST(sha256, largefile) { EXPECT_EQ(file_to_hex_hash("CLSAG.pdf"), "c38699c9a235a70285165ff8cce0bf3e48989de8092c15514116ca4c95d41e3f"); }
TEST(sha256, noexist) { crypto::hash hash; EXPECT_FALSE(tools::sha256sum("this_file_does_not_exist.exe", hash)); }
