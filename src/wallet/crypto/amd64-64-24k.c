// Copyright (c) 2017, The Monero Project
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
//
// Parts of this file from bench.cr.yp.to/supercop.html (2017-07-25):
//     Daniel J. Bernstein
//     Niels Duif
//     Tanja Lange
//     lead: Peter Schwabe
//     Bo-Yin Yang

#include <assert.h>
#include <stddef.h>
#include "fe25519.h"
#include "ge25519.h"

/* constants below can be found in various fles in ed25519/amd64-64-24k */

/* d */
static const fe25519 ecd = {{0x75EB4DCA135978A3, 0x00700A4D4141D8AB, 0x8CC740797779E898, 0x52036CEE2B6FFE73}};
/* 2*d */
static const fe25519 ec2d = {{0xEBD69B9426B2F146, 0x00E0149A8283B156, 0x198E80F2EEF3D130, 0xA406D9DC56DFFCE7}};
/* sqrt(-1) */
static const fe25519 sqrtm1 = {{0xC4EE1B274A0EA0B0, 0x2F431806AD2FE478, 0x2B4D00993DFBD7A7, 0x2B8324804FC1DF0B}};

/* taken from loop in ed25519/amd64-64-24k/ge25519_double_scalarmult.c */
static void ge25519_p1p1_to_pniels(ge25519_pniels* out, ge25519_p1p1 const* in) {
    assert(out && in);
    ge25519_p1p1_to_p3((ge25519_p3*)out, in);
    const fe25519 d = out->ysubx;
    fe25519_sub(&(out->ysubx), &(out->xaddy), &(out->ysubx));
    fe25519_add(&(out->xaddy), &(out->xaddy), &d);
    fe25519_mul(&(out->t2d), &(out->t2d), &ec2d);
}

#define choose_tp crypto_sign_ed25519_amd64_64_choose_tp
#define crypto_scalarmult crypto_scalarmult_curve25519_amd64_64_24k
#include "amd64.c.inc"

int monero_wallet_amd64_64_24k_get_tx_key(char* out, char const* pub, char const* sec) {
    return get_tx_key(out, pub, sec);
}
int monero_wallet_amd64_64_24k_get_tx_output_public(char* out, char const* pub, char const* sec) {
    return get_tx_output_public(out, pub, sec);
}

