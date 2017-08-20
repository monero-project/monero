#include <string.h>
#include "crypto_sign.h"
#include "crypto_hash_sha512.h"
#include "ge25519.h"

int crypto_sign(
    unsigned char *sm,unsigned long long *smlen,
    const unsigned char *m,unsigned long long mlen,
    const unsigned char *sk
    )
{
  unsigned char pk[32];
  unsigned char az[64];
  unsigned char nonce[64];
  unsigned char hram[64];
  sc25519 sck, scs, scsk;
  ge25519 ger;

  memmove(pk,sk + 32,32);
  /* pk: 32-byte public key A */

  crypto_hash_sha512(az,sk,32);
  az[0] &= 248;
  az[31] &= 127;
  az[31] |= 64;
  /* az: 32-byte scalar a, 32-byte randomizer z */

  *smlen = mlen + 64;
  memmove(sm + 64,m,mlen);
  memmove(sm + 32,az + 32,32);
  /* sm: 32-byte uninit, 32-byte z, mlen-byte m */

  crypto_hash_sha512(nonce, sm+32, mlen+32);
  /* nonce: 64-byte H(z,m) */

  sc25519_from64bytes(&sck, nonce);
  ge25519_scalarmult_base(&ger, &sck);
  ge25519_pack(sm, &ger);
  /* sm: 32-byte R, 32-byte z, mlen-byte m */
  
  memmove(sm + 32,pk,32);
  /* sm: 32-byte R, 32-byte A, mlen-byte m */

  crypto_hash_sha512(hram,sm,mlen + 64);
  /* hram: 64-byte H(R,A,m) */

  sc25519_from64bytes(&scs, hram);
  sc25519_from32bytes(&scsk, az);
  sc25519_mul(&scs, &scs, &scsk);
  sc25519_add(&scs, &scs, &sck);
  /* scs: S = nonce + H(R,A,m)a */

  sc25519_to32bytes(sm + 32,&scs);
  /* sm: 32-byte R, 32-byte S, mlen-byte m */

  return 0;
}
