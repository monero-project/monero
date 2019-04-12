// Copyright (c) 2014-2019, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __hash_h
#define __hash_h
/*
#include "crypto_uint8.h"
#include "crypto_uint32.h"
#include "crypto_uint64.h"
#include "crypto_hash.h" 

typedef crypto_uint8 uint8_t; 
typedef crypto_uint32 uint32_t; 
typedef crypto_uint64 uint64_t;
*/
#include <stdint.h>

/* some sizes (number of bytes) */
#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8

#define SIZE512 (ROWS*COLS512)

#define ROUNDS512 10
#define HASH_BIT_LEN 256

#define ROTL32(v, n) ((((v)<<(n))|((v)>>(32-(n))))&li_32(ffffffff))


#define li_32(h) 0x##h##u
#define EXT_BYTE(var,n) ((uint8_t)((uint32_t)(var) >> (8*n)))
#define u32BIG(a)				\
  ((ROTL32(a,8) & li_32(00FF00FF)) |		\
   (ROTL32(a,24) & li_32(FF00FF00)))


/* NIST API begin */
typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef struct {
  uint32_t chaining[SIZE512/sizeof(uint32_t)];            /* actual state */
  uint32_t block_counter1,
  block_counter2;         /* message block counter(s) */
  BitSequence buffer[SIZE512];      /* data buffer */
  int buf_ptr;              /* data buffer pointer */
  int bits_in_last_byte;    /* no. of message bits in last byte of
			       data buffer */
} hashState;

/*void Init(hashState*);
void Update(hashState*, const BitSequence*, DataLength);
void Final(hashState*, BitSequence*); */
void groestl(const BitSequence*, DataLength, BitSequence*);
/* NIST API end   */

/*
int crypto_hash(unsigned char *out,
		const unsigned char *in,
		unsigned long long len);
*/

#endif /* __hash_h */
