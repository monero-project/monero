// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

#ifndef KECCAK_H
#define KECCAK_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// compute a keccak hash (md) of given byte length from "in"
void keccak(const uint8_t *in, size_t inlen, uint8_t *md, int mdlen);

// update the state
void keccakf(uint64_t st[25]);

#ifdef __cplusplus
}
#endif

#endif
