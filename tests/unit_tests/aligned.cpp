// Copyright (c) 2018-2022, The Monero Project

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

#include "common/aligned.h"

TEST(aligned, large_null) { ASSERT_TRUE(aligned_malloc((size_t)-1, 1) == NULL); }
TEST(aligned, free_null) { aligned_free(NULL); }
TEST(aligned, zero) { void *ptr = aligned_malloc(0, 1); ASSERT_TRUE(ptr); aligned_free(ptr); }
TEST(aligned, aligned1) { void *ptr = aligned_malloc(1, 1); ASSERT_TRUE(ptr); aligned_free(ptr); }
TEST(aligned, aligned4096) { void *ptr = aligned_malloc(1, 4096); ASSERT_TRUE(ptr && ((uintptr_t)ptr & 4095) == 0); aligned_free(ptr); }
TEST(aligned, aligned8) { void *ptr = aligned_malloc(1, 8); ASSERT_TRUE(ptr && ((uintptr_t)ptr & 7) == 0); aligned_free(ptr); }
TEST(aligned, realloc_null) { void *ptr = aligned_realloc(NULL, 1, 4096); ASSERT_TRUE(ptr && ((uintptr_t)ptr & 4095) == 0); aligned_free(ptr); }
TEST(aligned, realloc_diff_align) { void *ptr = aligned_malloc(1, 4096); ASSERT_TRUE(!aligned_realloc(ptr, 1, 2048)); aligned_free(ptr); }
TEST(aligned, realloc_same) { void *ptr = aligned_malloc(1, 4096), *ptr2 = aligned_realloc(ptr, 1, 4096); ASSERT_TRUE(ptr == ptr2); aligned_free(ptr2); }
TEST(aligned, realloc_larger) { void *ptr = aligned_malloc(1, 4096), *ptr2 = aligned_realloc(ptr, 2, 4096); ASSERT_TRUE(ptr != ptr2); aligned_free(ptr2); }
TEST(aligned, realloc_zero) { void *ptr = aligned_malloc(1, 4096), *ptr2 = aligned_realloc(ptr, 0, 4096); ASSERT_TRUE(ptr && !ptr2); }

TEST(aligned, contents_larger)
{
  unsigned char *ptr = (unsigned char*)aligned_malloc(50, 256);
  ASSERT_TRUE(ptr);
  for (int n = 0; n < 50; ++n)
    ptr[n] = n;
  unsigned char *ptr2 = (unsigned char*)aligned_realloc(ptr, 51, 256);
  for (int n = 0; n < 50; ++n)
  {
    ASSERT_TRUE(ptr2[n] == n);
  }
  aligned_free(ptr2);
}

TEST(aligned, contents_same)
{
  unsigned char *ptr = (unsigned char*)aligned_malloc(50, 256);
  ASSERT_TRUE(ptr);
  for (int n = 0; n < 50; ++n)
    ptr[n] = n;
  unsigned char *ptr2 = (unsigned char*)aligned_realloc(ptr, 50, 256);
  for (int n = 0; n < 50; ++n)
  {
    ASSERT_TRUE(ptr2[n] == n);
  }
  aligned_free(ptr2);
}

TEST(aligned, contents_smaller)
{
  unsigned char *ptr = (unsigned char*)aligned_malloc(50, 256);
  ASSERT_TRUE(ptr);
  for (int n = 0; n < 50; ++n)
    ptr[n] = n;
  unsigned char *ptr2 = (unsigned char*)aligned_realloc(ptr, 49, 256);
  for (int n = 0; n < 49; ++n)
  {
    ASSERT_TRUE(ptr2[n] == n);
  }
  aligned_free(ptr2);
}

TEST(aligned, alignment)
{
  static const size_t good_alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
  for (size_t a = 0; a <= 8192; ++a)
  {
    bool good = false;
    for (const auto t: good_alignments) if (a == t) good = true;
    void *ptr = aligned_malloc(1, a);
    if (good)
    {
      ASSERT_TRUE(ptr != NULL);
      aligned_free(ptr);
    }
    else
    {
      ASSERT_TRUE(ptr == NULL);
    }
  }

  ASSERT_TRUE(aligned_malloc(1, ~0) == NULL);
}
