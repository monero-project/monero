// Copyright (c) 2014-2024, The Monero Project
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

#include "hmac-keccak.h"
#include "memwipe.h"

#define KECCAK_BLOCKLEN 136
#define HASH_SIZE 32

void hmac_keccak_init(hmac_keccak_state *S, const uint8_t *_key, size_t keylen) {
  const uint8_t *key = _key;
  uint8_t keyhash[HASH_SIZE];
  uint8_t pad[KECCAK_BLOCKLEN];
  uint64_t i;

  if (keylen > KECCAK_BLOCKLEN) {
    keccak(key, keylen, keyhash, HASH_SIZE);
    key = keyhash;
    keylen = HASH_SIZE;
  }

  keccak_init(&S->inner);
  memset(pad, 0x36, KECCAK_BLOCKLEN);
  for (i = 0; i < keylen; ++i) {
    pad[i] ^= key[i];
  }
  keccak_update(&S->inner, pad, KECCAK_BLOCKLEN);

  keccak_init(&S->outer);
  memset(pad, 0x5c, KECCAK_BLOCKLEN);
  for (i = 0; i < keylen; ++i) {
    pad[i] ^= key[i];
  }
  keccak_update(&S->outer, pad, KECCAK_BLOCKLEN);

  memwipe(keyhash, HASH_SIZE);
}

void hmac_keccak_update(hmac_keccak_state *S, const uint8_t *data, size_t datalen) {
  keccak_update(&S->inner, data, datalen);
}

void hmac_keccak_finish(hmac_keccak_state *S, uint8_t *digest) {
  uint8_t ihash[HASH_SIZE];
  keccak_finish(&S->inner, ihash);
  keccak_update(&S->outer, ihash, HASH_SIZE);
  keccak_finish(&S->outer, digest);
  memwipe(ihash, HASH_SIZE);
}

void hmac_keccak_hash(uint8_t *out, const uint8_t *key, size_t keylen, const uint8_t *in, size_t inlen) {
  hmac_keccak_state S;
  hmac_keccak_init(&S, key, keylen);
  hmac_keccak_update(&S, in, inlen);
  hmac_keccak_finish(&S, out);
}
