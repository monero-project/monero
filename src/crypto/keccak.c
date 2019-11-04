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
