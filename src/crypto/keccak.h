// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

#ifndef KECCAK_H
#define KECCAK_H

#include <stdint.h>
#include <string.h>

#ifndef KECCAK_ROUNDS
#define KECCAK_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

// SHA3 Algorithm context.
typedef struct KECCAK_CTX
{
  // 1600 bits algorithm hashing state
  uint64_t hash[25];
  // 1088-bit buffer for leftovers, block size = 136 B for 256-bit keccak
  uint64_t message[17];
  // count of bytes in the message[] buffer
  size_t rest;
} KECCAK_CTX;

// compute a keccak hash (md) of given byte length from "in"
void keccak(const uint8_t *in, size_t inlen, uint8_t *md, int mdlen);

// update the state
void keccakf(uint64_t st[25], int norounds);

void keccak1600(const uint8_t *in, size_t inlen, uint8_t *md);

void keccak_init(KECCAK_CTX * ctx);
void keccak_update(KECCAK_CTX * ctx, const uint8_t *in, size_t inlen);
void keccak_finish(KECCAK_CTX * ctx, uint8_t *md);
#endif
