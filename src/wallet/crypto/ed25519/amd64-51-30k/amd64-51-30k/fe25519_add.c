#include "fe25519.h"

void fe25519_add(fe25519 *r, const fe25519 *x, const fe25519 *y)
{
  r->v[0] = x->v[0] + y->v[0];
  r->v[1] = x->v[1] + y->v[1];
  r->v[2] = x->v[2] + y->v[2];
  r->v[3] = x->v[3] + y->v[3];
  r->v[4] = x->v[4] + y->v[4];
}
