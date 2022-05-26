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

// update the state with given number of rounds

void keccakf(uint64_t st[25], int rounds)
{
    int round;
    uint64_t t, bc[5];

    for (round = 0; round < rounds; ++round) {
        // Theta
        bc[0] = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
        bc[1] = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
        bc[2] = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
        bc[3] = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
        bc[4] = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];

#define THETA(i) { \
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1); \
            st[i     ] ^= t; \
            st[i +  5] ^= t; \
            st[i + 10] ^= t; \
            st[i + 15] ^= t; \
            st[i + 20] ^= t; \
        }

        THETA(0);
        THETA(1);
        THETA(2);
        THETA(3);
        THETA(4);

        // Rho Pi
        t = st[1];
        st[ 1] = ROTL64(st[ 6], 44);
        st[ 6] = ROTL64(st[ 9], 20);
        st[ 9] = ROTL64(st[22], 61);
        st[22] = ROTL64(st[14], 39);
        st[14] = ROTL64(st[20], 18);
        st[20] = ROTL64(st[ 2], 62);
        st[ 2] = ROTL64(st[12], 43);
        st[12] = ROTL64(st[13], 25);
        st[13] = ROTL64(st[19],  8);
        st[19] = ROTL64(st[23], 56);
        st[23] = ROTL64(st[15], 41);
        st[15] = ROTL64(st[ 4], 27);
        st[ 4] = ROTL64(st[24], 14);
        st[24] = ROTL64(st[21],  2);
        st[21] = ROTL64(st[ 8], 55);
        st[ 8] = ROTL64(st[16], 45);
        st[16] = ROTL64(st[ 5], 36);
        st[ 5] = ROTL64(st[ 3], 28);
        st[ 3] = ROTL64(st[18], 21);
        st[18] = ROTL64(st[17], 15);
        st[17] = ROTL64(st[11], 10);
        st[11] = ROTL64(st[ 7],  6);
        st[ 7] = ROTL64(st[10],  3);
        st[10] = ROTL64(t, 1);

        //  Chi
#define CHI(j) { \
            const uint64_t st0 = st[j    ]; \
            const uint64_t st1 = st[j + 1]; \
            const uint64_t st2 = st[j + 2]; \
            const uint64_t st3 = st[j + 3]; \
            const uint64_t st4 = st[j + 4]; \
            st[j    ] ^= ~st1 & st2; \
            st[j + 1] ^= ~st2 & st3; \
            st[j + 2] ^= ~st3 & st4; \
            st[j + 3] ^= ~st4 & st0; \
            st[j + 4] ^= ~st0 & st1; \
        }

        CHI( 0);
        CHI( 5);
        CHI(10);
        CHI(15);
        CHI(20);

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
