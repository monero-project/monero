// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <alloca.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "hash-ops.h"

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash) {
  assert(count > 0);
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
  } else if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
  } else {
    size_t i, j;
    size_t cnt = count - 1;
    char (*ints)[HASH_SIZE];
    for (i = 1; i < sizeof(size_t); i <<= 1) {
      cnt |= cnt >> i;
    }
    cnt &= ~(cnt >> 1);
    ints = alloca(cnt * HASH_SIZE);
    memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);
    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      cn_fast_hash(hashes[i], 64, ints[j]);
    }
    assert(i == count);
    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        cn_fast_hash(ints[i], 64, ints[j]);
      }
    }
    cn_fast_hash(ints[0], 64, root_hash);
  }
}
