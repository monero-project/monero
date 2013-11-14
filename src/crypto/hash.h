#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "int-util.h"

static inline void *padd(void *p, size_t i) {
  return (char *) p + i;
}

static inline const void *cpadd(const void *p, size_t i) {
  return (const char *) p + i;
}

static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "size_t must be 4 or 8 bytes long");
static inline void place_length(uint8_t *buffer, size_t bufsize, size_t length) {
  if (sizeof(size_t) == 4) {
    *(uint32_t *) padd(buffer, bufsize - 4) = swap32be(length);
  } else {
    *(uint64_t *) padd(buffer, bufsize - 8) = swap64be(length);
  }
}

struct keccak_state {
  union {
    uint8_t b[200];
    uint64_t w[25];
  };
};

enum {
  HASH_SIZE = 32,
  HASH_DATA_AREA = 136
};

void keccak(const void *data, size_t length, char *hash);
void keccak_permutation(struct keccak_state *state);