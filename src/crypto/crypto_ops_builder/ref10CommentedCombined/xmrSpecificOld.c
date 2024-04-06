/*Code inherited from the original cryptonote (I didn't write these) */
/*These functions either don't exist in, or are modified from ref10*/


/* From fe_isnonzero.c, modified */

//x
static int fe_isnonzero(const fe f) {
  unsigned char s[32];
  fe_tobytes(s, f);
  return (((int) (s[0] | s[1] | s[2] | s[3] | s[4] | s[5] | s[6] | s[7] | s[8] |
    s[9] | s[10] | s[11] | s[12] | s[13] | s[14] | s[15] | s[16] | s[17] |
    s[18] | s[19] | s[20] | s[21] | s[22] | s[23] | s[24] | s[25] | s[26] |
    s[27] | s[28] | s[29] | s[30] | s[31]) - 1) >> 8) + 1;
}

//double scalar mult precomp
void ge_dsm_precomp(ge_dsmp r, const ge_p3 *s) {
  ge_p1p1 t;
  ge_p3 s2, u;
  ge_p3_to_cached(&r[0], s);
  ge_p3_dbl(&t, s); ge_p1p1_to_p3(&s2, &t);
  ge_add(&t, &s2, &r[0]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[1], &u);
  ge_add(&t, &s2, &r[1]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[2], &u);
  ge_add(&t, &s2, &r[2]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[3], &u);
  ge_add(&t, &s2, &r[3]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[4], &u);
  ge_add(&t, &s2, &r[4]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[5], &u);
  ge_add(&t, &s2, &r[5]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[6], &u);
  ge_add(&t, &s2, &r[6]); ge_p1p1_to_p3(&u, &t); ge_p3_to_cached(&r[7], &u);
}

/*
r = a * A + b * B
where a = a[0]+256*a[1]+...+256^31 a[31].
and b = b[0]+256*b[1]+...+256^31 b[31].
B is the Ed25519 base point (x,4/5) with x positive.
*/

void ge_double_scalarmult_base_vartime(ge_p2 *r, const unsigned char *a, const ge_p3 *A, const unsigned char *b) {
  signed char aslide[256];
  signed char bslide[256];
  ge_dsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */
  ge_p1p1 t;
  ge_p3 u;
  int i;

  slide(aslide, a);
  slide(bslide, b);
  ge_dsm_precomp(Ai, A);/* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */

  ge_p2_0(r);

  for (i = 255; i >= 0; --i) {
      //so this gets us to the highest i such that both have a nonzero
    if (aslide[i] || bslide[i]) break;
  }

  for (; i >= 0; --i) {
    ge_p2_dbl(&t, r);//simple double and add

    if (aslide[i] > 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_add(&t, &u, &Ai[aslide[i]/2]);
    } else if (aslide[i] < 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_sub(&t, &u, &Ai[(-aslide[i])/2]);
    }

    if (bslide[i] > 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_madd(&t, &u, &ge_Bi[bslide[i]/2]);
    } else if (bslide[i] < 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_msub(&t, &u, &ge_Bi[(-bslide[i])/2]);
    }

    ge_p1p1_to_p2(r, &t);
  }
}


/* From ge_frombytes.c, modified */
//this is like xrecover .. 
//x
int ge_frombytes_vartime(ge_p3 *h, const unsigned char *s) {
  fe u;
  fe v;
  fe vxx;
  fe check;

  /* From fe_frombytes.c */

  int64_t h0 = load_4(s);
  int64_t h1 = load_3(s + 4) << 6;
  int64_t h2 = load_3(s + 7) << 5;
  int64_t h3 = load_3(s + 10) << 3;
  int64_t h4 = load_3(s + 13) << 2;
  int64_t h5 = load_4(s + 16);
  int64_t h6 = load_3(s + 20) << 7;
  int64_t h7 = load_3(s + 23) << 5;
  int64_t h8 = load_3(s + 26) << 4;
  int64_t h9 = (load_3(s + 29) & 8388607) << 2;
  int64_t carry0;
  int64_t carry1;
  int64_t carry2;
  int64_t carry3;
  int64_t carry4;
  int64_t carry5;
  int64_t carry6;
  int64_t carry7;
  int64_t carry8;
  int64_t carry9;

  /* Validate the number to be canonical */
  if (h9 == 33554428 && h8 == 268435440 && h7 == 536870880 && h6 == 2147483520 &&
    h5 == 4294967295 && h4 == 67108860 && h3 == 134217720 && h2 == 536870880 &&
    h1 == 1073741760 && h0 >= 4294967277) {
    return -1;
  }

  carry9 = (h9 + (int64_t) (1<<24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  carry1 = (h1 + (int64_t) (1<<24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry3 = (h3 + (int64_t) (1<<24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry5 = (h5 + (int64_t) (1<<24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry7 = (h7 + (int64_t) (1<<24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

  carry0 = (h0 + (int64_t) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry2 = (h2 + (int64_t) (1<<25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry4 = (h4 + (int64_t) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry6 = (h6 + (int64_t) (1<<25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry8 = (h8 + (int64_t) (1<<25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

  h->Y[0] = h0;
  h->Y[1] = h1;
  h->Y[2] = h2;
  h->Y[3] = h3;
  h->Y[4] = h4;
  h->Y[5] = h5;
  h->Y[6] = h6;
  h->Y[7] = h7;
  h->Y[8] = h8;
  h->Y[9] = h9;

  /* End fe_frombytes.c */

  fe_1(h->Z);
  fe_sq(u, h->Y);
  fe_mul(v, u, fe_d);
  fe_sub(u, u, h->Z);       /* u = y^2-1 */
  fe_add(v, v, h->Z);       /* v = dy^2+1 */

  fe_divpowm1(h->X, u, v); /* x = uv^3(uv^7)^((q-5)/8) */

  fe_sq(vxx, h->X);
  fe_mul(vxx, vxx, v);
  fe_sub(check, vxx, u);    /* vx^2-u */
  if (fe_isnonzero(check)) {
    fe_add(check, vxx, u);  /* vx^2+u */
    if (fe_isnonzero(check)) {
      return -1;
    }
    fe_mul(h->X, h->X, fe_sqrtm1); //this is mapping X to X * sqrt(-1) c.f. 3.1 in hisil, dong, etc. 
  }

  if (fe_isnegative(h->X) != (s[31] >> 7)) {
    /* If x = 0, the sign must be positive */
    if (!fe_isnonzero(h->X)) {
      return -1;
    }
    fe_neg(h->X, h->X);
  }

  fe_mul(h->T, h->X, h->Y);
  return 0;
}

/* New code */
/* (u / v)^(m + 1) */
static void fe_divpowm1(fe r, const fe u, const fe v) {
  fe v3, uv7, t0, t1, t2;
  int i;

  fe_sq(v3, v);
  fe_mul(v3, v3, v); /* v3 = v^3 */
  fe_sq(uv7, v3);
  fe_mul(uv7, uv7, v);
  fe_mul(uv7, uv7, u); /* uv7 = uv^7 */

  /*fe_pow22523(uv7, uv7);*/

  /* From fe_pow22523.c */

  fe_sq(t0, uv7);
  fe_sq(t1, t0);
  fe_sq(t1, t1);
  fe_mul(t1, uv7, t1);
  fe_mul(t0, t0, t1);
  fe_sq(t0, t0);
  fe_mul(t0, t1, t0);
  fe_sq(t1, t0);
  for (i = 0; i < 4; ++i) {
    fe_sq(t1, t1);
  }
  fe_mul(t0, t1, t0);
  fe_sq(t1, t0);
  for (i = 0; i < 9; ++i) {
    fe_sq(t1, t1);
  }
  fe_mul(t1, t1, t0);
  fe_sq(t2, t1);
  for (i = 0; i < 19; ++i) {
    fe_sq(t2, t2);
  }
  fe_mul(t1, t2, t1);
  for (i = 0; i < 10; ++i) {
    fe_sq(t1, t1);
  }
  fe_mul(t0, t1, t0);
  fe_sq(t1, t0);
  for (i = 0; i < 49; ++i) {
    fe_sq(t1, t1);
  }
  fe_mul(t1, t1, t0);
  fe_sq(t2, t1);
  for (i = 0; i < 99; ++i) {
    fe_sq(t2, t2);
  }
  fe_mul(t1, t2, t1);
  for (i = 0; i < 50; ++i) {
    fe_sq(t1, t1);
  }
  fe_mul(t0, t1, t0);
  fe_sq(t0, t0);
  fe_sq(t0, t0);
  fe_mul(t0, t0, uv7);

  /* End fe_pow22523.c */
  /* t0 = (uv^7)^((q-5)/8) */
  fe_mul(t0, t0, v3);
  fe_mul(r, t0, u); /* u^(m+1)v^(-(m+1)) */
}

static void ge_cached_0(ge_cached *r) {
  fe_1(r->YplusX);
  fe_1(r->YminusX);
  fe_1(r->Z);
  fe_0(r->T2d);
}

static void ge_cached_cmov(ge_cached *t, const ge_cached *u, unsigned char b) {
  fe_cmov(t->YplusX, u->YplusX, b);
  fe_cmov(t->YminusX, u->YminusX, b);
  fe_cmov(t->Z, u->Z, b);
  fe_cmov(t->T2d, u->T2d, b);
}

/* Assumes that a[31] <= 127 */
void ge_scalarmult(ge_p2 *r, const unsigned char *a, const ge_p3 *A) {
  signed char e[64];
  int carry, carry2, i;
  ge_cached Ai[8]; /* 1 * A, 2 * A, ..., 8 * A */
  ge_p1p1 t;
  ge_p3 u;

  carry = 0; /* 0..1 */
  for (i = 0; i < 31; i++) {
    carry += a[i]; /* 0..256 */
    carry2 = (carry + 8) >> 4; /* 0..16 */
    e[2 * i] = carry - (carry2 << 4); /* -8..7 */
    carry = (carry2 + 8) >> 4; /* 0..1 */
    e[2 * i + 1] = carry2 - (carry << 4); /* -8..7 */
  }
  carry += a[31]; /* 0..128 */
  carry2 = (carry + 8) >> 4; /* 0..8 */
  e[62] = carry - (carry2 << 4); /* -8..7 */
  e[63] = carry2; /* 0..8 */

  ge_p3_to_cached(&Ai[0], A);
  for (i = 0; i < 7; i++) {
    ge_add(&t, A, &Ai[i]);
    ge_p1p1_to_p3(&u, &t);
    ge_p3_to_cached(&Ai[i + 1], &u);
  }

  ge_p2_0(r);
  for (i = 63; i >= 0; i--) {
    signed char b = e[i];
    unsigned char bnegative = negative(b);
    unsigned char babs = b - (((-bnegative) & b) << 1);
    ge_cached cur, minuscur;
    ge_p2_dbl(&t, r);
    ge_p1p1_to_p2(r, &t);
    ge_p2_dbl(&t, r);
    ge_p1p1_to_p2(r, &t);
    ge_p2_dbl(&t, r);
    ge_p1p1_to_p2(r, &t);
    ge_p2_dbl(&t, r);
    ge_p1p1_to_p3(&u, &t);
    ge_cached_0(&cur);
    ge_cached_cmov(&cur, &Ai[0], equal(babs, 1));
    ge_cached_cmov(&cur, &Ai[1], equal(babs, 2));
    ge_cached_cmov(&cur, &Ai[2], equal(babs, 3));
    ge_cached_cmov(&cur, &Ai[3], equal(babs, 4));
    ge_cached_cmov(&cur, &Ai[4], equal(babs, 5));
    ge_cached_cmov(&cur, &Ai[5], equal(babs, 6));
    ge_cached_cmov(&cur, &Ai[6], equal(babs, 7));
    ge_cached_cmov(&cur, &Ai[7], equal(babs, 8));
    fe_copy(minuscur.YplusX, cur.YminusX);
    fe_copy(minuscur.YminusX, cur.YplusX);
    fe_copy(minuscur.Z, cur.Z);
    fe_neg(minuscur.T2d, cur.T2d);
    ge_cached_cmov(&cur, &minuscur, bnegative);
    ge_add(&t, &u, &cur);
    ge_p1p1_to_p2(r, &t);
  }
}

//assume Bi has precomputed multiples
void ge_double_scalarmult_precomp_vartime(ge_p2 *r, const unsigned char *a, const ge_p3 *A, const unsigned char *b, const ge_dsmp Bii) {
  signed char aslide[256];
  signed char bslide[256];
  ge_dsmp Ai; /* A, 3A, 5A, 7A, 9A, 11A, 13A, 15A */ //precomps
  ge_p1p1 t;
  ge_p3 u;
  int i;

  slide(aslide, a);
  slide(bslide, b);
  ge_dsm_precomp(Ai, A);

  ge_p2_0(r);

  for (i = 255; i >= 0; --i) {
    if (aslide[i] || bslide[i]) break;
  }

  for (; i >= 0; --i) {
    ge_p2_dbl(&t, r);

    if (aslide[i] > 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_add(&t, &u, &Ai[aslide[i]/2]);
    } else if (aslide[i] < 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_sub(&t, &u, &Ai[(-aslide[i])/2]);
    }

    if (bslide[i] > 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_add(&t, &u, &Bii[bslide[i]/2]);
    } else if (bslide[i] < 0) {
      ge_p1p1_to_p3(&u, &t);
      ge_sub(&t, &u, &Bii[(-bslide[i])/2]);
    }

    ge_p1p1_to_p2(r, &t);
  }
}

//x
void ge_mul8(ge_p1p1 *r, const ge_p2 *t) {
  ge_p2 u;
  ge_p2_dbl(r, t);
  ge_p1p1_to_p2(&u, r);
  ge_p2_dbl(r, &u);
  ge_p1p1_to_p2(&u, r);
  ge_p2_dbl(r, &u);
}

//x
void ge_fromfe_frombytes_vartime(ge_p2 *r, const unsigned char *s) {
  fe u, v, w, x, y, z;
  unsigned char sign;

  /* From fe_frombytes.c */

  int64_t h0 = load_4(s);
  int64_t h1 = load_3(s + 4) << 6;
  int64_t h2 = load_3(s + 7) << 5;
  int64_t h3 = load_3(s + 10) << 3;
  int64_t h4 = load_3(s + 13) << 2;
  int64_t h5 = load_4(s + 16);
  int64_t h6 = load_3(s + 20) << 7;
  int64_t h7 = load_3(s + 23) << 5;
  int64_t h8 = load_3(s + 26) << 4;
  int64_t h9 = load_3(s + 29) << 2;
  int64_t carry0;
  int64_t carry1;
  int64_t carry2;
  int64_t carry3;
  int64_t carry4;
  int64_t carry5;
  int64_t carry6;
  int64_t carry7;
  int64_t carry8;
  int64_t carry9;

  carry9 = (h9 + (int64_t) (1<<24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  carry1 = (h1 + (int64_t) (1<<24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry3 = (h3 + (int64_t) (1<<24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry5 = (h5 + (int64_t) (1<<24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry7 = (h7 + (int64_t) (1<<24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

  carry0 = (h0 + (int64_t) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry2 = (h2 + (int64_t) (1<<25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry4 = (h4 + (int64_t) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry6 = (h6 + (int64_t) (1<<25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry8 = (h8 + (int64_t) (1<<25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

  u[0] = h0;
  u[1] = h1;
  u[2] = h2;
  u[3] = h3;
  u[4] = h4;
  u[5] = h5;
  u[6] = h6;
  u[7] = h7;
  u[8] = h8;
  u[9] = h9;

  /* End fe_frombytes.c */

  fe_sq2(v, u); /* 2 * u^2 */
  fe_1(w);
  fe_add(w, v, w); /* w = 2 * u^2 + 1 */
  fe_sq(x, w); /* w^2 */
  fe_mul(y, fe_ma2, v); /* -2 * A^2 * u^2 */
  fe_add(x, x, y); /* x = w^2 - 2 * A^2 * u^2 */
  fe_divpowm1(r->X, w, x); /* (w / x)^(m + 1) */
  fe_sq(y, r->X);
  fe_mul(x, y, x);
  fe_sub(y, w, x);
  fe_copy(z, fe_ma);
  if (fe_isnonzero(y)) {
    fe_add(y, w, x);
    if (fe_isnonzero(y)) {
      goto negative;
    } else {
      fe_mul(r->X, r->X, fe_fffb1);
    }
  } else {
    fe_mul(r->X, r->X, fe_fffb2);
  }
  fe_mul(r->X, r->X, u); /* u * sqrt(2 * A * (A + 2) * w / x) */
  fe_mul(z, z, v); /* -2 * A * u^2 */
  sign = 0;
  goto setsign;
negative:
  fe_mul(x, x, fe_sqrtm1);
  fe_sub(y, w, x);
  if (fe_isnonzero(y)) {
    assert((fe_add(y, w, x), !fe_isnonzero(y)));
    fe_mul(r->X, r->X, fe_fffb3);
  } else {
    fe_mul(r->X, r->X, fe_fffb4);
  }
  /* r->X = sqrt(A * (A + 2) * w / x) */
  /* z = -A */
  sign = 1;
setsign:
  if (fe_isnegative(r->X) != sign) {
    assert(fe_isnonzero(r->X));
    fe_neg(r->X, r->X);
  }
  fe_add(r->Z, z, w);
  fe_sub(r->Y, z, w);
  fe_mul(r->X, r->X, r->Z);
#if !defined(NDEBUG)
  {
    fe check_x, check_y, check_iz, check_v;
    fe_invert(check_iz, r->Z);
    fe_mul(check_x, r->X, check_iz);
    fe_mul(check_y, r->Y, check_iz);
    fe_sq(check_x, check_x);
    fe_sq(check_y, check_y);
    fe_mul(check_v, check_x, check_y);
    fe_mul(check_v, fe_d, check_v);
    fe_add(check_v, check_v, check_x);
    fe_sub(check_v, check_v, check_y);
    fe_1(check_x);
    fe_add(check_v, check_v, check_x);
    assert(!fe_isnonzero(check_v));
  }
#endif
}

//x
void sc_0(unsigned char *s) {
  int i;
  for (i = 0; i < 32; i++) {
    s[i] = 0;
  }
}

//x
void sc_reduce32(unsigned char *s) {
  int64_t s0 = 2097151 & load_3(s);
  int64_t s1 = 2097151 & (load_4(s + 2) >> 5);
  int64_t s2 = 2097151 & (load_3(s + 5) >> 2);
  int64_t s3 = 2097151 & (load_4(s + 7) >> 7);
  int64_t s4 = 2097151 & (load_4(s + 10) >> 4);
  int64_t s5 = 2097151 & (load_3(s + 13) >> 1);
  int64_t s6 = 2097151 & (load_4(s + 15) >> 6);
  int64_t s7 = 2097151 & (load_3(s + 18) >> 3);
  int64_t s8 = 2097151 & load_3(s + 21);
  int64_t s9 = 2097151 & (load_4(s + 23) >> 5);
  int64_t s10 = 2097151 & (load_3(s + 26) >> 2);
  int64_t s11 = (load_4(s + 28) >> 7);
  int64_t s12 = 0;
  int64_t carry0;
  int64_t carry1;
  int64_t carry2;
  int64_t carry3;
  int64_t carry4;
  int64_t carry5;
  int64_t carry6;
  int64_t carry7;
  int64_t carry8;
  int64_t carry9;
  int64_t carry10;
  int64_t carry11;

  carry0 = (s0 + (1<<20)) >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry2 = (s2 + (1<<20)) >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry4 = (s4 + (1<<20)) >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry6 = (s6 + (1<<20)) >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry8 = (s8 + (1<<20)) >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry10 = (s10 + (1<<20)) >> 21; s11 += carry10; s10 -= carry10 << 21;

  carry1 = (s1 + (1<<20)) >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry3 = (s3 + (1<<20)) >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry5 = (s5 + (1<<20)) >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry7 = (s7 + (1<<20)) >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry9 = (s9 + (1<<20)) >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry11 = (s11 + (1<<20)) >> 21; s12 += carry11; s11 -= carry11 << 21;

  s0 += s12 * 666643;
  s1 += s12 * 470296;
  s2 += s12 * 654183;
  s3 -= s12 * 997805;
  s4 += s12 * 136657;
  s5 -= s12 * 683901;
  s12 = 0;

  carry0 = s0 >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry1 = s1 >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry2 = s2 >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry3 = s3 >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry4 = s4 >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry5 = s5 >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry6 = s6 >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry7 = s7 >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry8 = s8 >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry9 = s9 >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry10 = s10 >> 21; s11 += carry10; s10 -= carry10 << 21;
  carry11 = s11 >> 21; s12 += carry11; s11 -= carry11 << 21;

  s0 += s12 * 666643;
  s1 += s12 * 470296;
  s2 += s12 * 654183;
  s3 -= s12 * 997805;
  s4 += s12 * 136657;
  s5 -= s12 * 683901;

  carry0 = s0 >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry1 = s1 >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry2 = s2 >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry3 = s3 >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry4 = s4 >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry5 = s5 >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry6 = s6 >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry7 = s7 >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry8 = s8 >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry9 = s9 >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry10 = s10 >> 21; s11 += carry10; s10 -= carry10 << 21;

  s[0] = s0 >> 0;
  s[1] = s0 >> 8;
  s[2] = (s0 >> 16) | (s1 << 5);
  s[3] = s1 >> 3;
  s[4] = s1 >> 11;
  s[5] = (s1 >> 19) | (s2 << 2);
  s[6] = s2 >> 6;
  s[7] = (s2 >> 14) | (s3 << 7);
  s[8] = s3 >> 1;
  s[9] = s3 >> 9;
  s[10] = (s3 >> 17) | (s4 << 4);
  s[11] = s4 >> 4;
  s[12] = s4 >> 12;
  s[13] = (s4 >> 20) | (s5 << 1);
  s[14] = s5 >> 7;
  s[15] = (s5 >> 15) | (s6 << 6);
  s[16] = s6 >> 2;
  s[17] = s6 >> 10;
  s[18] = (s6 >> 18) | (s7 << 3);
  s[19] = s7 >> 5;
  s[20] = s7 >> 13;
  s[21] = s8 >> 0;
  s[22] = s8 >> 8;
  s[23] = (s8 >> 16) | (s9 << 5);
  s[24] = s9 >> 3;
  s[25] = s9 >> 11;
  s[26] = (s9 >> 19) | (s10 << 2);
  s[27] = s10 >> 6;
  s[28] = (s10 >> 14) | (s11 << 7);
  s[29] = s11 >> 1;
  s[30] = s11 >> 9;
  s[31] = s11 >> 17;
}

//x
void sc_add(unsigned char *s, const unsigned char *a, const unsigned char *b) {
  int64_t a0 = 2097151 & load_3(a);
  int64_t a1 = 2097151 & (load_4(a + 2) >> 5);
  int64_t a2 = 2097151 & (load_3(a + 5) >> 2);
  int64_t a3 = 2097151 & (load_4(a + 7) >> 7);
  int64_t a4 = 2097151 & (load_4(a + 10) >> 4);
  int64_t a5 = 2097151 & (load_3(a + 13) >> 1);
  int64_t a6 = 2097151 & (load_4(a + 15) >> 6);
  int64_t a7 = 2097151 & (load_3(a + 18) >> 3);
  int64_t a8 = 2097151 & load_3(a + 21);
  int64_t a9 = 2097151 & (load_4(a + 23) >> 5);
  int64_t a10 = 2097151 & (load_3(a + 26) >> 2);
  int64_t a11 = (load_4(a + 28) >> 7);
  int64_t b0 = 2097151 & load_3(b);
  int64_t b1 = 2097151 & (load_4(b + 2) >> 5);
  int64_t b2 = 2097151 & (load_3(b + 5) >> 2);
  int64_t b3 = 2097151 & (load_4(b + 7) >> 7);
  int64_t b4 = 2097151 & (load_4(b + 10) >> 4);
  int64_t b5 = 2097151 & (load_3(b + 13) >> 1);
  int64_t b6 = 2097151 & (load_4(b + 15) >> 6);
  int64_t b7 = 2097151 & (load_3(b + 18) >> 3);
  int64_t b8 = 2097151 & load_3(b + 21);
  int64_t b9 = 2097151 & (load_4(b + 23) >> 5);
  int64_t b10 = 2097151 & (load_3(b + 26) >> 2);
  int64_t b11 = (load_4(b + 28) >> 7);
  int64_t s0 = a0 + b0;
  int64_t s1 = a1 + b1;
  int64_t s2 = a2 + b2;
  int64_t s3 = a3 + b3;
  int64_t s4 = a4 + b4;
  int64_t s5 = a5 + b5;
  int64_t s6 = a6 + b6;
  int64_t s7 = a7 + b7;
  int64_t s8 = a8 + b8;
  int64_t s9 = a9 + b9;
  int64_t s10 = a10 + b10;
  int64_t s11 = a11 + b11;
  int64_t s12 = 0;
  int64_t carry0;
  int64_t carry1;
  int64_t carry2;
  int64_t carry3;
  int64_t carry4;
  int64_t carry5;
  int64_t carry6;
  int64_t carry7;
  int64_t carry8;
  int64_t carry9;
  int64_t carry10;
  int64_t carry11;

  carry0 = (s0 + (1<<20)) >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry2 = (s2 + (1<<20)) >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry4 = (s4 + (1<<20)) >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry6 = (s6 + (1<<20)) >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry8 = (s8 + (1<<20)) >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry10 = (s10 + (1<<20)) >> 21; s11 += carry10; s10 -= carry10 << 21;

  carry1 = (s1 + (1<<20)) >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry3 = (s3 + (1<<20)) >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry5 = (s5 + (1<<20)) >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry7 = (s7 + (1<<20)) >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry9 = (s9 + (1<<20)) >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry11 = (s11 + (1<<20)) >> 21; s12 += carry11; s11 -= carry11 << 21;

  s0 += s12 * 666643;
  s1 += s12 * 470296;
  s2 += s12 * 654183;
  s3 -= s12 * 997805;
  s4 += s12 * 136657;
  s5 -= s12 * 683901;
  s12 = 0;

  carry0 = s0 >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry1 = s1 >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry2 = s2 >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry3 = s3 >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry4 = s4 >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry5 = s5 >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry6 = s6 >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry7 = s7 >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry8 = s8 >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry9 = s9 >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry10 = s10 >> 21; s11 += carry10; s10 -= carry10 << 21;
  carry11 = s11 >> 21; s12 += carry11; s11 -= carry11 << 21;

  s0 += s12 * 666643;
  s1 += s12 * 470296;
  s2 += s12 * 654183;
  s3 -= s12 * 997805;
  s4 += s12 * 136657;
  s5 -= s12 * 683901;

  carry0 = s0 >> 21; s1 += carry0; s0 -= carry0 << 21;
  carry1 = s1 >> 21; s2 += carry1; s1 -= carry1 << 21;
  carry2 = s2 >> 21; s3 += carry2; s2 -= carry2 << 21;
  carry3 = s3 >> 21; s4 += carry3; s3 -= carry3 << 21;
  carry4 = s4 >> 21; s5 += carry4; s4 -= carry4 << 21;
  carry5 = s5 >> 21; s6 += carry5; s5 -= carry5 << 21;
  carry6 = s6 >> 21; s7 += carry6; s6 -= carry6 << 21;
  carry7 = s7 >> 21; s8 += carry7; s7 -= carry7 << 21;
  carry8 = s8 >> 21; s9 += carry8; s8 -= carry8 << 21;
  carry9 = s9 >> 21; s10 += carry9; s9 -= carry9 << 21;
  carry10 = s10 >> 21; s11 += carry10; s10 -= carry10 << 21;

  s[0] = s0 >> 0;
  s[1] = s0 >> 8;
  s[2] = (s0 >> 16) | (s1 << 5);
  s[3] = s1 >> 3;
  s[4] = s1 >> 11;
  s[5] = (s1 >> 19) | (s2 << 2);
  s[6] = s2 >> 6;
  s[7] = (s2 >> 14) | (s3 << 7);
  s[8] = s3 >> 1;
  s[9] = s3 >> 9;
  s[10] = (s3 >> 17) | (s4 << 4);
  s[11] = s4 >> 4;
  s[12] = s4 >> 12;
  s[13] = (s4 >> 20) | (s5 << 1);
  s[14] = s5 >> 7;
  s[15] = (s5 >> 15) | (s6 << 6);
  s[16] = s6 >> 2;
  s[17] = s6 >> 10;
  s[18] = (s6 >> 18) | (s7 << 3);
  s[19] = s7 >> 5;
  s[20] = s7 >> 13;
  s[21] = s8 >> 0;
  s[22] = s8 >> 8;
  s[23] = (s8 >> 16) | (s9 << 5);
  s[24] = s9 >> 3;
  s[25] = s9 >> 11;
  s[26] = (s9 >> 19) | (s10 << 2);
  s[27] = s10 >> 6;
  s[28] = (s10 >> 14) | (s11 << 7);
  s[29] = s11 >> 1;
  s[30] = s11 >> 9;
  s[31] = s11 >> 17;
}

/* Assumes that a != INT64_MIN */
//x c.f. lt in 32 bit ref
static int64_t signum(int64_t a) {
  return (a >> 63) - ((-a) >> 63);
}

//x
int sc_check(const unsigned char *s) {
  int64_t s0 = load_4(s);
  int64_t s1 = load_4(s + 4);
  int64_t s2 = load_4(s + 8);
  int64_t s3 = load_4(s + 12);
  int64_t s4 = load_4(s + 16);
  int64_t s5 = load_4(s + 20);
  int64_t s6 = load_4(s + 24);
  int64_t s7 = load_4(s + 28);
  return (signum(1559614444 - s0) + (signum(1477600026 - s1) << 1) + (signum(2734136534 - s2) << 2) + (signum(350157278 - s3) << 3) + (signum(-s4) << 4) + (signum(-s5) << 5) + (signum(-s6) << 6) + (signum(268435456 - s7) << 7)) >> 8;
}

//x
int sc_isnonzero(const unsigned char *s) {
  return (((int) (s[0] | s[1] | s[2] | s[3] | s[4] | s[5] | s[6] | s[7] | s[8] |
    s[9] | s[10] | s[11] | s[12] | s[13] | s[14] | s[15] | s[16] | s[17] |
    s[18] | s[19] | s[20] | s[21] | s[22] | s[23] | s[24] | s[25] | s[26] |
    s[27] | s[28] | s[29] | s[30] | s[31]) - 1) >> 8) + 1;
}
