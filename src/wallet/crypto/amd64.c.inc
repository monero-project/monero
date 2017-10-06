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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers
//
// Parts of this file from biench.cr.yp.to/supercop.html (2017-02-25):
//     Daniel J. Bernstein
//     Niels Duif
//     Tanja Lange
//     lead: Peter Schwabe
//     Bo-Yin Yang

#include <string.h>
#include "ge25519.h"

extern void choose_tp(ge25519_pniels *t, unsigned long long pos, signed long long b, const ge25519_pniels *base_multiples);

/* return 0 on success, -1 otherwise. Taken from
ed25519/amd64-51-30k/ge25519_unpackneg.c - the negation is removed. */
static int unpack_vartime(ge25519_p3 *r, const unsigned char p[32]) {
  fe25519 t, chk, num, den, den2, den4, den6;
  unsigned char par = p[31] >> 7;

  fe25519_setint(&r->z,1);
  fe25519_unpack(&r->y, p); 
  fe25519_square(&num, &r->y); /* x = y^2 */
  fe25519_mul(&den, &num, &ecd); /* den = dy^2 */
  fe25519_sub(&num, &num, &r->z); /* x = y^2-1 */
  fe25519_add(&den, &r->z, &den); /* den = dy^2+1 */

  /* Computation of sqrt(num/den)
     1.: computation of num^((p-5)/8)*den^((7p-35)/8) = (num*den^7)^((p-5)/8)
  */
  fe25519_square(&den2, &den);
  fe25519_square(&den4, &den2);
  fe25519_mul(&den6, &den4, &den2);
  fe25519_mul(&t, &den6, &num);
  fe25519_mul(&t, &t, &den);

  fe25519_pow2523(&t, &t);
  /* 2. computation of r->x = t * num * den^3
  */
  fe25519_mul(&t, &t, &num);
  fe25519_mul(&t, &t, &den);
  fe25519_mul(&t, &t, &den);
  fe25519_mul(&r->x, &t, &den);

  /* 3. Check whether sqrt computation gave correct result, multiply by sqrt(-1) if not:
  */
  fe25519_square(&chk, &r->x);
  fe25519_mul(&chk, &chk, &den);
  if (!fe25519_iseq_vartime(&chk, &num))
    fe25519_mul(&r->x, &r->x, &sqrtm1);

  /* 4. Now we have one of the two square roots, except if input was not a square
  */
  fe25519_square(&chk, &r->x);
  fe25519_mul(&chk, &chk, &den);
  if (!fe25519_iseq_vartime(&chk, &num))
    return -1;

  /* 5. Choose the desired square root according to parity:
  */
  if(fe25519_getparity(&r->x) == (1-par))
    fe25519_neg(&r->x, &r->x);

  fe25519_mul(&r->t, &r->x, &r->y);
  return 0;
}

static void p3_to_pniels(ge25519_pniels* out, ge25519_p3 const* src) {
  fe25519_sub(&out->ysubx, &src->y, &src->x);
  fe25519_add(&out->xaddy, &src->x, &src->y);
  fe25519_mul(&out->t2d, &src->t, &ec2d);
  out->z = src->z;
}

/* _similar_ to ge_scalarmult in src/crypto/crypto-ops.c */
static void monero_scalarmult(ge25519_p3* r, char const* sec) {
  signed char b[64];
  ge25519_pniels base_multiples[8];

  ge25519_pniels t;
  ge25519_p1p1 tp1p1;

  sc25519 s;
  memcpy(s.v, sec, sizeof(s));
  sc25519_window4(b, &s);

  p3_to_pniels(&base_multiples[0], r);
  for (int i = 0; i < 7; ++i) {
    ge25519_pnielsadd_p1p1(&tp1p1, r, &base_multiples[i]);
    ge25519_p1p1_to_pniels(&base_multiples[i + 1], &tp1p1);
  }

  // set neutral
  fe25519_setint(&r->x, 0);
  fe25519_setint(&r->y, 1);
  fe25519_setint(&r->t, 0);
  fe25519_setint(&r->z, 1);
  // end set neutral

  for (int i = 63; /* break below*/ ; --i) {
    choose_tp(&t, (unsigned long long) 0, (signed long long) b[i], base_multiples);
    ge25519_pnielsadd_p1p1(&tp1p1, r, &t);
    ge25519_p1p1_to_p2((ge25519_p2*)r, &tp1p1);

    if (i == 0) break;

    ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
    ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
    ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
    ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
    ge25519_p1p1_to_p3(r, &tp1p1);
  }

  // non-standard, monero specific - guarantees point is in ed25519 group
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
  ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
  ge25519_p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
  ge25519_dbl_p1p1(&tp1p1,(ge25519_p2 *)r);
  ge25519_p1p1_to_p3(r, &tp1p1);
}

static int get_tx_key(char* out, char const* pub, char const* sec) {
  ge25519 unpacked;
  if (unpack_vartime(&unpacked, (unsigned char const*)pub) != 0)
    return -1;

  monero_scalarmult(&unpacked, sec);
  ge25519_pack((unsigned char*)out, &unpacked);
  return 0;
}

static int get_tx_output_public(char* out, char const* base, char const* sec) {
  ge25519 unpacked1;
  ge25519 unpacked2;
  sc25519 unpacked3;

  if (unpack_vartime(&unpacked1, (unsigned char const*)base) != 0)
    return -1;

  memcpy(unpacked3.v, sec, sizeof(unpacked3.v));
  ge25519_scalarmult_base(&unpacked2, &unpacked3);
  ge25519_add(&unpacked1, &unpacked1, &unpacked2);
  ge25519_pack((unsigned char*)out, &unpacked1);
  return 0;
}

