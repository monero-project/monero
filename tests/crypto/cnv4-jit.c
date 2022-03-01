// Copyright (c) 2019-2022, The Monero Project
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

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "crypto/hash-ops.h"

extern volatile int use_v4_jit_flag;

static int test(const uint8_t *data, size_t len, uint64_t height)
{
  char hash0[32], hash1[32];
  use_v4_jit_flag = 0;
  cn_slow_hash(data, len, hash0, 4, 0, height);
  use_v4_jit_flag = 1;
  cn_slow_hash(data, len, hash1, 4, 0, height);
  return memcmp(hash0, hash1, 32);
}

int main(int argc, char **argv)
{
  uint8_t data[64];
  uint64_t start_height = 1788000;
  uint64_t end_height = 1788001;

  if (argc != 1 && argc != 2 && argc != 3)
  {
    fprintf(stderr, "usage: %s [<start_height> [<end_height>]]\n", argv[0]);
    return 1;
  }
  if (argc > 1)
  {
    errno = 0;
    start_height = strtoull(argv[1], NULL, 10);
    if ((start_height == 0 && errno) || start_height == ULLONG_MAX)
    {
      fprintf(stderr, "invalid start_height\n");
      return 1;
    }
    end_height = start_height;
    if (argc > 2)
    {
      errno = 0;
      end_height = strtoull(argv[2], NULL, 10);
      if ((end_height == 0 && errno) || end_height == ULLONG_MAX)
      {
        fprintf(stderr, "invalid end_height\n");
        return 1;
      }
    }
  }

  if (start_height == end_height)
  {
    uint64_t counter = 0;
    while (1)
    {
      printf("\r%llu", (unsigned long long)counter);
      fflush(stdout);
      size_t offset = 0;
      while (offset + 8 < sizeof(data))
      {
        memcpy(data + offset, &counter, sizeof(counter));
        offset += 8;
      }
      if (test(data, sizeof(data), start_height))
      {
        fprintf(stderr, "\nFailure at height %llu, counter %llu\n", (unsigned long long)start_height, (unsigned long long)counter);
        return 0;
      }
      ++counter;
    }
  }

  memset(data, 0x42, sizeof(data));
  for (uint64_t h = start_height; h < end_height; ++h)
  {
    printf("\r%llu/%llu", (unsigned long long)(h-start_height), (unsigned long long)(end_height-start_height));
    fflush(stdout);
    if (test(data, sizeof(data), h))
    {
      fprintf(stderr, "\nFailure at height %llu\n", (unsigned long long)h);
      return 0;
    }
  }

  printf("\r");

  return 0;
}
