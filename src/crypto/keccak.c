// keccak.c
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>
// A baseline Keccak (3rd round) implementation.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "int-util.h"
#include "hash-ops.h"
#include "keccak.h"

static void local_abort(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
  _exit(1);
#else
  abort();
#endif
}

const uint64_t keccakf_rndc[24] = 
{
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

const int keccakf_rotc[24] = 
{
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14, 
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

const int keccakf_piln[24] = 
{
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4, 
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1 
};

// update the state with given number of rounds

void keccakf(uint64_t st[25], int rounds)
{
    int i, j, round;
    uint64_t t, bc[5];

    for (round = 0; round < rounds; round++) {

        // Theta
        for (i = 0; i < 5; i++)     
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        //  Chi
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        //  Iota
        st[0] ^= keccakf_rndc[round];
    }
}

// compute a keccak hash (md) of given byte length from "in"
typedef uint64_t state_t[25];

void keccak(const uint8_t *in, size_t inlen, uint8_t *md, int mdlen)
{
    state_t st;
    uint8_t temp[144];
    size_t i, rsiz, rsizw;

    static_assert(HASH_DATA_AREA <= sizeof(temp), "Bad keccak preconditions");
    if (mdlen <= 0 || (mdlen > 100 && sizeof(st) != (size_t)mdlen))
    {
      local_abort("Bad keccak use");
    }

    rsiz = sizeof(state_t) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;
    
    memset(st, 0, sizeof(st));

    for ( ; inlen >= rsiz; inlen -= rsiz, in += rsiz) {
      for (i = 0; i < rsizw; i++) {
        uint64_t ina;
        memcpy(&ina, in + i * 8, 8);
        st[i] ^= swap64le(ina);
      }
      keccakf(st, KECCAK_ROUNDS);
    }
    
    // last block and padding
    if (inlen + 1 >= sizeof(temp) || inlen > rsiz || rsiz - inlen + inlen + 1 >= sizeof(temp) || rsiz == 0 || rsiz - 1 >= sizeof(temp) || rsizw * 8 > sizeof(temp))
    {
      local_abort("Bad keccak use");
    }

    if (inlen > 0)
      memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (i = 0; i < rsizw; i++)
        st[i] ^= swap64le(((uint64_t *) temp)[i]);

    keccakf(st, KECCAK_ROUNDS);

    if (((size_t)mdlen % sizeof(uint64_t)) != 0)
    {
      local_abort("Bad keccak use");
    }
    memcpy_swap64le(md, st, mdlen/sizeof(uint64_t));
}

void keccak1600(const uint8_t *in, size_t inlen, uint8_t *md)
{
    keccak(in, inlen, md, sizeof(state_t));
}

#define KECCAK_FINALIZED 0x80000000
#define KECCAK_BLOCKLEN 136
#define KECCAK_WORDS 17
#define KECCAK_DIGESTSIZE 32
#define KECCAK_PROCESS_BLOCK(st, block) { \
    for (int i_ = 0; i_ < KECCAK_WORDS; i_++){ \
        ((st))[i_] ^= swap64le(((block))[i_]); \
    }; \
    keccakf(st, KECCAK_ROUNDS); }


void keccak_init(KECCAK_CTX * ctx){
    memset(ctx, 0, sizeof(KECCAK_CTX));
}

void keccak_update(KECCAK_CTX * ctx, const uint8_t *in, size_t inlen){
    if (ctx->rest & KECCAK_FINALIZED) {
        local_abort("Bad keccak use");
    }

    const size_t idx = ctx->rest;
    ctx->rest = (ctx->rest + inlen) % KECCAK_BLOCKLEN;

    // fill partial block
    if (idx) {
        size_t left = KECCAK_BLOCKLEN - idx;
        memcpy((char*)ctx->message + idx, in, (inlen < left ? inlen : left));
        if (inlen < left) return;

        KECCAK_PROCESS_BLOCK(ctx->hash, ctx->message);

        in  += left;
        inlen -= left;
    }

    while (inlen >= KECCAK_BLOCKLEN) {
        memcpy(ctx->message, in, KECCAK_BLOCKLEN);

        KECCAK_PROCESS_BLOCK(ctx->hash, ctx->message);
        in  += KECCAK_BLOCKLEN;
        inlen -= KECCAK_BLOCKLEN;
    }
    if (inlen) {
        memcpy(ctx->message, in, inlen);
    }
}

void keccak_finish(KECCAK_CTX * ctx, uint8_t *md){
    if (!(ctx->rest & KECCAK_FINALIZED))
    {
        // clear the rest of the data queue
        memset((char*)ctx->message + ctx->rest, 0, KECCAK_BLOCKLEN - ctx->rest);
        ((char*)ctx->message)[ctx->rest] |= 0x01;
        ((char*)ctx->message)[KECCAK_BLOCKLEN - 1] |= 0x80;

        // process final block
        KECCAK_PROCESS_BLOCK(ctx->hash, ctx->message);
        ctx->rest = KECCAK_FINALIZED; // mark context as finalized
    }

    static_assert(KECCAK_BLOCKLEN > KECCAK_DIGESTSIZE, "");
    static_assert(KECCAK_DIGESTSIZE % sizeof(uint64_t) == 0, "");
    if (md) {
        memcpy_swap64le(md, ctx->hash, KECCAK_DIGESTSIZE / sizeof(uint64_t));
    }
}
