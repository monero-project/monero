#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "int-util.h"
#include "hash.h"

static const uint64_t round_constants[] = {
  0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
  0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
  0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
  0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
  0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
  0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

static const uint8_t rho_offsets[] = {
  0, 1, 62, 28, 27,
  36, 44, 6, 55, 20,
  3, 10, 43, 25, 39,
  41, 45, 15, 21, 8,
  18, 2, 61, 56, 14
};

static void keccak_round(uint64_t *state, unsigned int i) {
  unsigned int j, k;
  uint64_t tmp[25];
  for(j = 0; j < 5; j++) {
    tmp[j] = state[j] ^ state[j + 5] ^ state[j + 10] ^ state[j + 15] ^ state[j + 20];
  }
  for(j = 0; j < 5; j++) {
    uint64_t t = rol64(tmp[(j + 1) % 5], 1) ^ tmp[(j + 4) % 5];
    state[j] ^= t;
    state[j + 5] ^= t;
    state[j + 10] ^= t;
    state[j + 15] ^= t;
    state[j + 20] ^= t;
  }
  for(j = 0; j < 25; j++) {
    state[j] = rol64(state[j], rho_offsets[j]);
  }
#define index(x, y) (((x) % 5) + 5 * ((y) % 5))
  for(j = 0; j < 5; j++) {
    for(k = 0; k < 5; k++) {
      tmp[index(0 * k + 1 * j, 2 * k + 3 * j)] = state[index(k, j)];
    }
  }
  for(j = 0; j < 5; j++) {
    for(k = 0; k < 5; k++) {
      state[index(k, j)] = tmp[index(k, j)] ^ ((~tmp[index(k + 1, j)]) & tmp[index(k + 2, j)]);
    }
  }
#undef index
  state[0] ^= round_constants[i];
}

void keccak_permutation(struct keccak_state *state) {
  unsigned int i;
  mem_inplace_swap64le(&state, 25);
  for (i = 0; i < 24; i++) {
    keccak_round(state->w, i);
  }
  mem_inplace_swap64le(&state, 25);
}

void keccak(const void *data, size_t length, char *hash) {
  struct keccak_state state;
  const void *limit = cpadd(data, length);
  size_t i;
  memset(&state, 0, sizeof(struct keccak_state));
  for (i = 0; data != limit; ++data) {
    state.b[i] ^= *(char *) data;
    if (++i == 136) {
      keccak_permutation(&state);
      i = 0;
    }
  }
  state.b[i] ^= 1;
  state.b[135] ^= 0x80;
  keccak_permutation(&state);
  memcpy(hash, &state, 32);
}