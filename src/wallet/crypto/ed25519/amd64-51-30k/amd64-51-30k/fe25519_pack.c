#include "fe25519.h"

/* Assumes input x being reduced below 2^255 */
void fe25519_pack(unsigned char r[32], const fe25519 *x)
{
  fe25519 t;
  t = *x;
  fe25519_freeze(&t);
  r[0]   = (unsigned char) ( t.v[0]        & 0xff);
  r[1]   = (unsigned char) ((t.v[0] >>  8) & 0xff);
  r[2]   = (unsigned char) ((t.v[0] >> 16) & 0xff);
  r[3]   = (unsigned char) ((t.v[0] >> 24) & 0xff);
  r[4]   = (unsigned char) ((t.v[0] >> 32) & 0xff);
  r[5]   = (unsigned char) ((t.v[0] >> 40) & 0xff);
  r[6]   = (unsigned char) ((t.v[0] >> 48));

  r[6]  ^= (unsigned char) ((t.v[1] <<  3) & 0xf8);
  r[7]   = (unsigned char) ((t.v[1] >>  5) & 0xff);
  r[8]   = (unsigned char) ((t.v[1] >> 13) & 0xff);
  r[9]   = (unsigned char) ((t.v[1] >> 21) & 0xff);
  r[10]  = (unsigned char) ((t.v[1] >> 29) & 0xff);
  r[11]  = (unsigned char) ((t.v[1] >> 37) & 0xff);
  r[12]  = (unsigned char) ((t.v[1] >> 45));

  r[12] ^= (unsigned char) ((t.v[2] <<  6) & 0xc0);
  r[13]  = (unsigned char) ((t.v[2] >>  2) & 0xff);
  r[14]  = (unsigned char) ((t.v[2] >> 10) & 0xff);
  r[15]  = (unsigned char) ((t.v[2] >> 18) & 0xff);
  r[16]  = (unsigned char) ((t.v[2] >> 26) & 0xff);
  r[17]  = (unsigned char) ((t.v[2] >> 34) & 0xff);
  r[18]  = (unsigned char) ((t.v[2] >> 42) & 0xff);
  r[19]  = (unsigned char) ((t.v[2] >> 50));
  
  r[19] ^= (unsigned char) ((t.v[3] <<  1) & 0xfe);
  r[20]  = (unsigned char) ((t.v[3] >>  7) & 0xff);
  r[21]  = (unsigned char) ((t.v[3] >> 15) & 0xff);
  r[22]  = (unsigned char) ((t.v[3] >> 23) & 0xff);
  r[23]  = (unsigned char) ((t.v[3] >> 31) & 0xff);
  r[24]  = (unsigned char) ((t.v[3] >> 39) & 0xff);
  r[25]  = (unsigned char) ((t.v[3] >> 47));

  r[25] ^= (unsigned char) ((t.v[4] <<  4) & 0xf0);
  r[26]  = (unsigned char) ((t.v[4] >>  4) & 0xff);
  r[27]  = (unsigned char) ((t.v[4] >> 12) & 0xff);
  r[28]  = (unsigned char) ((t.v[4] >> 20) & 0xff);
  r[29]  = (unsigned char) ((t.v[4] >> 28) & 0xff);
  r[30]  = (unsigned char) ((t.v[4] >> 36) & 0xff);
  r[31]  = (unsigned char) ((t.v[4] >> 44));
}
