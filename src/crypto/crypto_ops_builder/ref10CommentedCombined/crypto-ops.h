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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

/* From fe.h */

typedef int32_t fe[10];

/* From ge.h */

typedef struct {
  fe X;
  fe Y;
  fe Z;
} ge_p2;

typedef struct {
  fe X;
  fe Y;
  fe Z;
  fe T;
} ge_p3;

typedef struct {
  fe X;
  fe Y;
  fe Z;
  fe T;
} ge_p1p1;

typedef struct {
  fe yplusx;
  fe yminusx;
  fe xy2d;
} ge_precomp;

typedef struct {
  fe YplusX;
  fe YminusX;
  fe Z;
  fe T2d;
} ge_cached;

/* From ge_add.c */

void ge_add(ge_p1p1 *, const ge_p3 *, const ge_cached *);

/* From ge_double_scalarmult.c, modified */

typedef ge_cached ge_dsmp[8];
extern const ge_precomp ge_Bi[8];
void ge_dsm_precomp(ge_dsmp r, const ge_p3 *s);
void ge_double_scalarmult_base_vartime(ge_p2 *, const unsigned char *, const ge_p3 *, const unsigned char *);

/* From ge_frombytes.c, modified */

extern const fe fe_sqrtm1;
extern const fe fe_d;
int ge_frombytes_vartime(ge_p3 *, const unsigned char *);

/* From ge_p1p1_to_p2.c */

void ge_p1p1_to_p2(ge_p2 *, const ge_p1p1 *);

/* From ge_p1p1_to_p3.c */

void ge_p1p1_to_p3(ge_p3 *, const ge_p1p1 *);

/* From ge_p2_dbl.c */

void ge_p2_dbl(ge_p1p1 *, const ge_p2 *);

/* From ge_p3_to_cached.c */

extern const fe fe_d2;
void ge_p3_to_cached(ge_cached *, const ge_p3 *);

/* From ge_p3_to_p2.c */

void ge_p3_to_p2(ge_p2 *, const ge_p3 *);

/* From ge_p3_tobytes.c */

void ge_p3_tobytes(unsigned char *, const ge_p3 *);

/* From ge_scalarmult_base.c */

extern const ge_precomp ge_base[32][8];
void ge_scalarmult_base(ge_p3 *, const unsigned char *);

/* From ge_tobytes.c */

void ge_tobytes(unsigned char *, const ge_p2 *);

/* From sc_reduce.c */

void sc_reduce(unsigned char *);

/* New code */

void ge_scalarmult(ge_p2 *, const unsigned char *, const ge_p3 *);
void ge_double_scalarmult_precomp_vartime(ge_p2 *, const unsigned char *, const ge_p3 *, const unsigned char *, const ge_dsmp);
void ge_mul8(ge_p1p1 *, const ge_p2 *);
extern const fe fe_ma2;
extern const fe fe_ma;
extern const fe fe_fffb1;
extern const fe fe_fffb2;
extern const fe fe_fffb3;
extern const fe fe_fffb4;
void ge_fromfe_frombytes_vartime(ge_p2 *, const unsigned char *);
void sc_0(unsigned char *);
void sc_reduce32(unsigned char *);
void sc_add(unsigned char *, const unsigned char *, const unsigned char *);
void sc_sub(unsigned char *, const unsigned char *, const unsigned char *);
void sc_mulsub(unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *);
int sc_check(const unsigned char *);
int sc_isnonzero(const unsigned char *); /* Doesn't normalize */
