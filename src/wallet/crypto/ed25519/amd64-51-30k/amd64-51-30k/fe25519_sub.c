#include "fe25519.h"

void fe25519_sub(fe25519 *r, const fe25519 *x, const fe25519 *y)
{
  fe25519 yt = *y;
  /* Not required for reduced input */
  
  unsigned long long t;
  t = yt.v[0] >> 51;
  yt.v[0] &= 2251799813685247;
  yt.v[1] += t;

  t = yt.v[1] >> 51;
  yt.v[1] &= 2251799813685247;
  yt.v[2] += t;

  t = yt.v[2] >> 51;
  yt.v[2] &= 2251799813685247;
  yt.v[3] += t;

  t = yt.v[3] >> 51;
  yt.v[3] &= 2251799813685247;
  yt.v[4] += t;

  t = yt.v[4] >> 51;
  yt.v[4] &= 2251799813685247;
  yt.v[0] += 19*t;

  r->v[0] = x->v[0] + 0xFFFFFFFFFFFDA - yt.v[0];
  r->v[1] = x->v[1] + 0xFFFFFFFFFFFFE - yt.v[1];
  r->v[2] = x->v[2] + 0xFFFFFFFFFFFFE - yt.v[2];
  r->v[3] = x->v[3] + 0xFFFFFFFFFFFFE - yt.v[3];
  r->v[4] = x->v[4] + 0xFFFFFFFFFFFFE - yt.v[4];
}
