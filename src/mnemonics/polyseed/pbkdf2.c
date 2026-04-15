// Copyright (c) 2026, The Monero Project
// Copyright (c) 2021, tevador <tevador@gmail.com>
// Copyright (c) 2005,2007,2009 Colin Percival
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <string.h>

#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/utils.h>

static inline void
store32_be(uint8_t dst[4], uint32_t w)
{
    dst[3] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[0] = (uint8_t) w;
}

void
crypto_pbkdf2_sha256(const uint8_t* passwd, size_t passwdlen,
                     const uint8_t* salt, size_t saltlen, uint64_t c,
                     uint8_t* buf, size_t dkLen)
{
    crypto_auth_hmacsha256_state Phctx, PShctx, hctx;
    size_t                       i;
    uint8_t                      ivec[4];
    uint8_t                      U[32];
    uint8_t                      T[32];
    uint64_t                     j;
    int                          k;
    size_t                       clen;

    crypto_auth_hmacsha256_init(&Phctx, passwd, passwdlen);
    PShctx = Phctx;
    crypto_auth_hmacsha256_update(&PShctx, salt, saltlen);

    for (i = 0; i * 32 < dkLen; i++) {
        store32_be(ivec, (uint32_t)(i + 1));
        hctx = PShctx;
        crypto_auth_hmacsha256_update(&hctx, ivec, 4);
        crypto_auth_hmacsha256_final(&hctx, U);

        memcpy(T, U, 32);
        for (j = 2; j <= c; j++) {
            hctx = Phctx;
            crypto_auth_hmacsha256_update(&hctx, U, 32);
            crypto_auth_hmacsha256_final(&hctx, U);

            for (k = 0; k < 32; k++) {
                T[k] ^= U[k];
            }
        }

        clen = dkLen - i * 32;
        if (clen > 32) {
            clen = 32;
        }
        memcpy(&buf[i * 32], T, clen);
    }
    sodium_memzero((void*)&Phctx, sizeof Phctx);
    sodium_memzero((void*)&PShctx, sizeof PShctx);
}
