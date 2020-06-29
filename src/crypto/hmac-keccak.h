// Copyright (c) 2014-2018, The Monero Project
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

#ifndef HMAC_KECCAK_H
#define HMAC_KECCAK_H

#include "keccak.h"

// HMAC RFC 2104 with Keccak-256 base hash function
//
// B = KECCAK_BLOCKLEN = 136 B
// L = HASH_SIZE = 32 B
//
// Note this is not HMAC-SHA3 as SHA3 and Keccak differs in
// the padding constant.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  KECCAK_CTX inner;
  KECCAK_CTX outer;
} hmac_keccak_state;

void hmac_keccak_init(hmac_keccak_state *S, const uint8_t *_key, size_t keylen);
void hmac_keccak_update(hmac_keccak_state *S, const uint8_t *data, size_t datalen);
void hmac_keccak_finish(hmac_keccak_state *S, uint8_t *digest);
void hmac_keccak_hash(uint8_t *out, const uint8_t *key, size_t keylen, const uint8_t *in, size_t inlen);

#ifdef __cplusplus
}
#endif
#endif //HMAC_KECCAK_H
