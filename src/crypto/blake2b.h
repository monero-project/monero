/*
Copyright (c) 2018-2019, tevador <tevador@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the copyright holder nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Original code from Argon2 reference source code package used under CC0 Licence
 * https://github.com/P-H-C/phc-winner-argon2
 * Copyright 2015
 * Daniel Dinu, Dmitry Khovratovich, Jean-Philippe Aumasson, and Samuel Neves
*/

/// NOTE: implementation lifted out of randomx package

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if defined(__cplusplus)
extern "C" {
#endif

	enum blake2b_constant {
		BLAKE2B_BLOCKBYTES = 128,
		BLAKE2B_OUTBYTES = 64,
		BLAKE2B_KEYBYTES = 64,
		BLAKE2B_SALTBYTES = 16,
		BLAKE2B_PERSONALBYTES = 16
	};

#pragma pack(push, 1)
	typedef struct __blake2b_param {
		uint8_t digest_length;                   /* 1 */
		uint8_t key_length;                      /* 2 */
		uint8_t fanout;                          /* 3 */
		uint8_t depth;                           /* 4 */
		uint32_t leaf_length;                    /* 8 */
		uint64_t node_offset;                    /* 16 */
		uint8_t node_depth;                      /* 17 */
		uint8_t inner_length;                    /* 18 */
		uint8_t reserved[14];                    /* 32 */
		uint8_t salt[BLAKE2B_SALTBYTES];         /* 48 */
		uint8_t personal[BLAKE2B_PERSONALBYTES]; /* 64 */
	} blake2b_param;
#pragma pack(pop)

	typedef struct __blake2b_state {
		uint64_t h[8];
		uint64_t t[2];
		uint64_t f[2];
		uint8_t buf[BLAKE2B_BLOCKBYTES];
		unsigned buflen;
		unsigned outlen;
		uint8_t last_node;
	} blake2b_state;

	/* Ensure param structs have not been wrongly padded */
	/* Poor man's static_assert */
	enum {
		blake2_size_check_0 = 1 / !!(CHAR_BIT == 8),
		blake2_size_check_2 =
		1 / !!(sizeof(blake2b_param) == sizeof(uint64_t) * CHAR_BIT)
	};

	// crypto namespace (fixes naming collisions with libsodium)
#define blake2b_init        crypto_blake2b_init
#define blake2b_init_key    crypto_blake2b_init_key
#define blake2b_init_param  crypto_blake2b_init_param
#define blake2b_update      crypto_blake2b_update
#define blake2b_final       crypto_blake2b_final
#define blake2b             crypto_blake2b
#define blake2b_long        crypto_blake2b_long

	/* Streaming API */
	int blake2b_init(blake2b_state *S, size_t outlen);
	int blake2b_init_key(blake2b_state *S, size_t outlen, const void *key,
		size_t keylen);
	int blake2b_init_param(blake2b_state *S, const blake2b_param *P);
	int blake2b_update(blake2b_state *S, const void *in, size_t inlen);
	int blake2b_final(blake2b_state *S, void *out, size_t outlen);

	/* Simple API */
	int blake2b(void *out, size_t outlen, const void *in, size_t inlen,
		const void *key, size_t keylen);

	/* Argon2 Team - Begin Code */
	int blake2b_long(void *out, size_t outlen, const void *in, size_t inlen);
	/* Argon2 Team - End Code */

#if defined(__cplusplus)
}
#endif
