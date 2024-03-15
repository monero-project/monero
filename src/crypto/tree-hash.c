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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hash-ops.h"

/*** 
* Round to power of two, for count>=3 and for count being not too large (as reasonable for tree hash calculations)
*/
size_t tree_hash_cnt(size_t count) {
	// This algo has some bad history but all we are doing is 1 << floor(log2(count))
	// There are _many_ ways to do log2, for some reason the one selected was the most obscure one,
	// and fixing it made it even more obscure.
	//
	// Iterative method implemented below aims for clarity over speed, if performance is needed
	// then my advice is to use the BSR instruction on x86
	//
	// All the paranoid asserts have been removed since it is trivial to mathematically prove that
	// the return will always be a power of 2.
	// Problem space has been defined as 3 <= count <= 2^28. Of course quarter of a billion transactions
	// is not a sane upper limit for a block, so there will be tighter limits in other parts of the code

	assert( count >= 3 ); // cases for 0,1,2 are handled elsewhere
	assert( count <= 0x10000000 ); // sanity limit to 2^28, MSB=1 will cause an inf loop

	size_t pow = 2;
	while(pow < count) pow <<= 1;
	return pow >> 1;
}

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash) {
// The blockchain block at height 202612 https://moneroblocks.info/block/202612
// contained 514 transactions, that triggered bad calculation of variable "cnt" in the original version of this function
// as from CryptoNote code.
//
// This bug applies to all CN altcoins.
//
// Mathematical bug here was first published on 14:45:34 (GMT+2) 2014-09-04 by Rafal Freeman <rfree>
// https://github.com/rfree2monero/bitmonero/commit/b417abfb7a297d09f1bbb6de29030f8de9952ac8
// and soon also applied to CryptoNote (15:10 GMT+2), and BoolBerry used not fully correct work around:
// the work around of sizeof(size_t)*8 or <<3 as used before in 2 coins and in BBL later was blocking
// exploitation on normal platforms, how ever we strongly recommend the following fix because it removes
// mistake in mathematical formula.

  assert(count > 0);
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
  } else if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
  } else {
    size_t i, j;

    size_t cnt = tree_hash_cnt( count );

    char *ints = calloc(cnt, HASH_SIZE);  // zero out as extra protection for using uninitialized mem
    assert(ints);

    memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);

    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      cn_fast_hash(hashes[i], 64, ints + j * HASH_SIZE);
    }
    assert(i == count);

    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        cn_fast_hash(ints + i * HASH_SIZE, 64, ints + j * HASH_SIZE);
      }
    }

    cn_fast_hash(ints, 64, root_hash);
    free(ints);
  }
}

bool tree_path(size_t count, size_t idx, uint32_t *path)
{
  if (count == 0)
    return false;

  if (count == 1) {
    *path = 0;
  } else if (count == 2) {
    *path = idx == 0 ? 0 : 1;
  } else {
    size_t i, j;

    *path = 0;
    size_t cnt = tree_hash_cnt( count );

    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      if (idx == i || idx == i+1)
      {
        *path = (*path << 1) | (idx == i ? 0 : 1);
        idx = j;
      }
    }
    assert(i == count);

    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        if (idx == i || idx == i + 1)
        {
          *path = (*path << 1) | (idx == i ? 0 : 1);
          idx = j;
        }
      }
    }

    if (idx == 0 || idx == 1)
    {
      *path = (*path << 1) | (idx == 0 ? 0 : 1);
      idx = 0;
    }
  }
  return true;
}

bool tree_branch(const char (*hashes)[HASH_SIZE], size_t count, const char *hash, char (*branch)[HASH_SIZE], size_t *depth, uint32_t *path)
{
  size_t idx;

  if (count == 0)
    return false;

  for (idx = 0; idx < count; ++idx)
    if (!memcmp(hash, hashes[idx], HASH_SIZE))
      break;
  if (idx == count)
    return false;

  assert(count > 0);
  if (count == 1) {
    *depth = 0;
    *path = 0;
  } else if (count == 2) {
    *depth = 1;
    *path = idx == 0 ? 0 : 1;
    memcpy(branch[0], hashes[idx ^ 1], HASH_SIZE);
  } else {
    size_t i, j;

    *depth = 0;
    *path = 0;
    size_t cnt = tree_hash_cnt( count );

    char *ints = calloc(cnt, HASH_SIZE);  // zero out as extra protection for using uninitialized mem
    assert(ints);

    memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);

    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      if (idx == i || idx == i+1)
      {
        memcpy(branch[*depth], hashes[idx == i ? i + 1 : i], HASH_SIZE);
        ++*depth;
        *path = (*path << 1) | (idx == i ? 0 : 1);
        idx = j;
      }
      cn_fast_hash(hashes[i], 64, ints + j * HASH_SIZE);
    }
    assert(i == count);

    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        if (idx == i || idx == i + 1)
        {
          memcpy(branch[*depth], ints + (idx == i ? i + 1 : i) * HASH_SIZE, HASH_SIZE);
          ++*depth;
          *path = (*path << 1) | (idx == i ? 0 : 1);
          idx = j;
        }
        cn_fast_hash(ints + i * HASH_SIZE, 64, ints + j * HASH_SIZE);
      }
    }

    if (idx == 0 || idx == 1)
    {
      memcpy(branch[*depth], ints + (idx == 0 ? 1 : 0) * HASH_SIZE, HASH_SIZE);
      ++*depth;
      *path = (*path << 1) | (idx == 0 ? 0 : 1);
      idx = 0;
    }

    free(ints);
  }
  return true;
}

bool tree_branch_hash(const char hash[HASH_SIZE], const char (*branch)[HASH_SIZE], size_t depth, uint32_t path, char root[HASH_SIZE])
{
  size_t d;
  char partial[HASH_SIZE];

  memcpy(partial, hash, HASH_SIZE);

  for (d = 0; d < depth; ++d)
  {
    char buffer[2 * HASH_SIZE];
    if ((path >> (depth - d - 1)) & 1)
    {
      memcpy(buffer, branch[d], HASH_SIZE);
      memcpy(buffer + HASH_SIZE, partial, HASH_SIZE);
    }
    else
    {
      memcpy(buffer, partial, HASH_SIZE);
      memcpy(buffer + HASH_SIZE, branch[d], HASH_SIZE);
    }
    cn_fast_hash(buffer, 2 * HASH_SIZE, partial);
  }

  memcpy(root, partial, HASH_SIZE);
  return true;
}

bool is_branch_in_tree(const char hash[HASH_SIZE], const char root[HASH_SIZE], const char (*branch)[HASH_SIZE], size_t depth, uint32_t path)
{
  char res[HASH_SIZE];
  if (!tree_branch_hash(hash, branch, depth, path, res))
    return false;
  return memcmp(res, root, HASH_SIZE) == 0;
}
