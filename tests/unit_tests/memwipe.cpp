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

#include <stdint.h>
#include "misc_log_ex.h"
#include "memwipe.h"

// Probably won't catch the optimized out case, but at least we test
// it works in the normal case
static void test(bool wipe)
{
  char *foo = (char*)malloc(4);
  ASSERT_TRUE(foo != NULL);
  intptr_t foop = (intptr_t)foo;
  strcpy(foo, "bar");
  void *bar = wipe ? memwipe(foo, 3) : memset(foo, 0, 3);
  ASSERT_EQ(foo, bar);
  free(foo);
  char *quux = (char*)malloc(4); // same size, just after free, so we're likely to get the same, depending on the allocator
  if ((intptr_t)quux == foop)
  {
    MDEBUG(std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)quux);
    if (wipe) { ASSERT_TRUE(memcmp(quux, "bar", 3)); }
  }
  else MWARNING("We did not get the same location, cannot check");
  free(quux);
}

TEST(memwipe, control)
{
  test(false);
}

TEST(memwipe, works)
{
  test(true);
}
