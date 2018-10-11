// Copyright (c) 2018, The Monero Project
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

extern "C" {
#include "crypto/keccak.h"
}

#define KECCAK_BLOCKLEN 136

#define TEST_KECCAK(sz, chunks) \
  std::string data; \
  data.resize(sz); \
  for (size_t i = 0; i < sz; ++i) \
    data[i] = i * 17; \
  uint8_t md0[32], md1[32]; \
  keccak((const uint8_t*)data.data(), data.size(), md0, 32); \
  KECCAK_CTX ctx; \
  keccak_init(&ctx); \
  size_t offset = 0; \
  for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); ++i) \
  { \
    ASSERT_TRUE(offset + chunks[i] <= data.size()); \
    keccak_update(&ctx, (const uint8_t*)data.data() + offset, chunks[i]); \
    offset += chunks[i]; \
  } \
  ASSERT_TRUE(offset == data.size()); \
  keccak_finish(&ctx, md1); \
  ASSERT_EQ(memcmp(md0, md1, 32), 0);

TEST(keccak, )
{
}

TEST(keccak, 0_and_0)
{
  static const size_t chunks[] = {0};
  TEST_KECCAK(0, chunks);
}

TEST(keccak, 1_and_1)
{
  static const size_t chunks[] = {1};
  TEST_KECCAK(1, chunks);
}

TEST(keccak, 1_and_0_1_0)
{
  static const size_t chunks[] = {0, 1, 0};
  TEST_KECCAK(1, chunks);
}

TEST(keccak, 2_and_1_1)
{
  static const size_t chunks[] = {1, 1};
  TEST_KECCAK(2, chunks);
}

TEST(keccak, 4_and_0_0_1_0_2_1_0)
{
  static const size_t chunks[] = {0, 0, 1, 0, 2, 1, 0};
  TEST_KECCAK(4, chunks);
}

TEST(keccak, 15_and_1_14)
{
  static const size_t chunks[] = {1, 14};
  TEST_KECCAK(15, chunks);
}

TEST(keccak, 135_and_134_1)
{
  static const size_t chunks[] = {134, 1};
  TEST_KECCAK(135, chunks);
}

TEST(keccak, 135_and_135_0)
{
  static const size_t chunks[] = {135, 0};
  TEST_KECCAK(135, chunks);
}

TEST(keccak, 135_and_0_135)
{
  static const size_t chunks[] = {0, 135};
  TEST_KECCAK(135, chunks);
}

TEST(keccak, 136_and_135_1)
{
  static const size_t chunks[] = {135, 1};
  TEST_KECCAK(136, chunks);
}

TEST(keccak, 136_and_136_0)
{
  static const size_t chunks[] = {136, 0};
  TEST_KECCAK(136, chunks);
}

TEST(keccak, 136_and_0_136)
{
  static const size_t chunks[] = {0, 136};
  TEST_KECCAK(136, chunks);
}

TEST(keccak, 136_and_136)
{
  static const size_t chunks[] = {136};
  TEST_KECCAK(136, chunks);
}

TEST(keccak, 137_and_136_1)
{
  static const size_t chunks[] = {136, 1};
  TEST_KECCAK(137, chunks);
}

TEST(keccak, 137_and_1_136)
{
  static const size_t chunks[] = {1, 136};
  TEST_KECCAK(137, chunks);
}

