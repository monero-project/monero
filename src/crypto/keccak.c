// keccak.c
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>
// A baseline Keccak (3rd round) implementation.

#include "keccak.h"
#include "hash-ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void local_abort(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
	_exit(1);
#else
	abort();
#endif
}

const uint64_t keccakf_rndc[24] = {
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

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

void keccakf(uint64_t st[25])
{
	for (size_t round = 0; round < 24; round++)
	{
		uint64_t t0, t1, bc0, bc1, bc2, bc3, bc4;
		// Theta
		bc0 = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
		bc1 = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
		bc2 = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
		bc3 = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
		bc4 = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];

		t0 = bc0;
		t1 = bc1;
		bc0 ^= ROTL64(bc2, 1);
		bc1 ^= ROTL64(bc3, 1);
		bc2 ^= ROTL64(bc4, 1);
		bc3 ^= ROTL64(t0 , 1);
		bc4 ^= ROTL64(t1 , 1);

		// Rho Pi
		t0 = st[1] ^ bc0;
		st[ 0] ^= bc4;
		st[ 1] = ROTL64(st[ 6] ^ bc0, 44);
		st[ 6] = ROTL64(st[ 9] ^ bc3, 20);
		st[ 9] = ROTL64(st[22] ^ bc1, 61);
		st[22] = ROTL64(st[14] ^ bc3, 39);
		st[14] = ROTL64(st[20] ^ bc4, 18);
		st[20] = ROTL64(st[ 2] ^ bc1, 62);
		st[ 2] = ROTL64(st[12] ^ bc1, 43);
		st[12] = ROTL64(st[13] ^ bc2, 25);
		st[13] = ROTL64(st[19] ^ bc3,  8);
		st[19] = ROTL64(st[23] ^ bc2, 56);
		st[23] = ROTL64(st[15] ^ bc4, 41);
		st[15] = ROTL64(st[ 4] ^ bc3, 27);
		st[ 4] = ROTL64(st[24] ^ bc3, 14);
		st[24] = ROTL64(st[21] ^ bc0,  2);
		st[21] = ROTL64(st[ 8] ^ bc2, 55);
		st[ 8] = ROTL64(st[16] ^ bc0, 45);
		st[16] = ROTL64(st[ 5] ^ bc4, 36);
		st[ 5] = ROTL64(st[ 3] ^ bc2, 28);
		st[ 3] = ROTL64(st[18] ^ bc2, 21);
		st[18] = ROTL64(st[17] ^ bc1, 15);
		st[17] = ROTL64(st[11] ^ bc0, 10);
		st[11] = ROTL64(st[ 7] ^ bc1,  6);
		st[ 7] = ROTL64(st[10] ^ bc4,  3);
		st[10] = ROTL64(t0, 1);

		//  Chi
		bc0 = st[ 0];
		bc1 = st[ 1];
		st[ 0] ^= (~st[ 1]) & st[ 2];
		st[ 1] ^= (~st[ 2]) & st[ 3];
		st[ 2] ^= (~st[ 3]) & st[ 4];
		st[ 3] ^= (~st[ 4]) & bc0;
		st[ 4] ^= (~bc0) & bc1;

		bc0 = st[ 5];
		bc1 = st[ 6];
		st[ 5] ^= (~st[ 6]) & st[ 7];
		st[ 6] ^= (~st[ 7]) & st[ 8];
		st[ 7] ^= (~st[ 8]) & st[ 9];
		st[ 8] ^= (~st[ 9]) & bc0;
		st[ 9] ^= (~bc0) & bc1;

		bc0 = st[10];
		bc1 = st[11];
		st[10] ^= (~st[11]) & st[12];
		st[11] ^= (~st[12]) & st[13];
		st[12] ^= (~st[13]) & st[14];
		st[13] ^= (~st[14]) & bc0;
		st[14] ^= (~bc0) & bc1;

		bc0 = st[15];
		bc1 = st[16];
		st[15] ^= (~st[16]) & st[17];
		st[16] ^= (~st[17]) & st[18];
		st[17] ^= (~st[18]) & st[19];
		st[18] ^= (~st[19]) & bc0;
		st[19] ^= (~bc0) & bc1;

		bc0 = st[20];
		bc1 = st[21];
		bc2 = st[22];
		bc3 = st[23];
		bc4 = st[24];

		st[20] ^= (~bc1) & bc2;
		st[21] ^= (~bc2) & bc3;
		st[22] ^= (~bc3) & bc4;
		st[23] ^= (~bc4) & bc0;
		st[24] ^= (~bc0) & bc1;

		//  Iota
		st[0] ^= keccakf_rndc[round];
	}
}


void keccakf_2(uint64_t st[25], int rounds)
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
	if(mdlen <= 0 || (mdlen > 100 && sizeof(st) != (size_t)mdlen))
	{
		local_abort("Bad keccak use");
	}

	rsiz = sizeof(state_t) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
	rsizw = rsiz / 8;

	memset(st, 0, sizeof(st));

	for(; inlen >= rsiz; inlen -= rsiz, in += rsiz)
	{
		for(i = 0; i < rsizw; i++)
			st[i] ^= ((uint64_t *)in)[i];
		keccakf(st);
	}

	// last block and padding
	if(inlen + 1 >= sizeof(temp) || inlen > rsiz || rsiz - inlen + inlen + 1 >= sizeof(temp) || rsiz == 0 || rsiz - 1 >= sizeof(temp) || rsizw * 8 > sizeof(temp))
	{
		local_abort("Bad keccak use");
	}

	memcpy(temp, in, inlen);
	temp[inlen++] = 1;
	memset(temp + inlen, 0, rsiz - inlen);
	temp[rsiz - 1] |= 0x80;

	for(i = 0; i < rsizw; i++)
		st[i] ^= ((uint64_t *)temp)[i];

	keccakf(st);

	memcpy(md, st, mdlen);
}

#define KECCAK_FINALIZED 0x80000000
#define KECCAK_BLOCKLEN 136
#define KECCAK_WORDS 17
#define KECCAK_DIGESTSIZE 32
#define IS_ALIGNED_64(p) (0 == (7 & ((const char*)(p) - (const char*)0)))
#define KECCAK_PROCESS_BLOCK(st, block) { \
    for (int i_ = 0; i_ < KECCAK_WORDS; i_++){ \
        ((st))[i_] ^= ((block))[i_]; \
    }; \
    keccakf_2(st, KECCAK_ROUNDS); }


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

    const bool is_aligned = IS_ALIGNED_64(in);
    while (inlen >= KECCAK_BLOCKLEN) {
        const uint64_t* aligned_message_block;
        if (is_aligned) {
            aligned_message_block = (uint64_t*)in;
        } else {
            memcpy(ctx->message, in, KECCAK_BLOCKLEN);
            aligned_message_block = ctx->message;
        }

        KECCAK_PROCESS_BLOCK(ctx->hash, aligned_message_block);
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
    if (md) {
        memcpy(md, ctx->hash, KECCAK_DIGESTSIZE);
    }
}