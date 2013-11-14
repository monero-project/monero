#include <assert.h>

#include "crypto.h"
#include "random.h"

using namespace crypto;

int main(int argc, char *argv[]) {
  char message[10];
  hash mh;
  public_key pk1, pk2;
  secret_key sk1, sk2;
  key_image ki;
  public_key *ppk[2];
  signature sig[2];
  bool res;
  /* Call this before using functions that depend on randomness. */
  init_random();
  /* Generate a random message. */
  generate_random_bytes(sizeof message, message);
  /* Find its hash */
  keccak(message, sizeof message, mh);
  /* Generate some keys */
  generate_keys(pk1, sk1);
  generate_keys(pk2, sk2);
  /* Sign the message */
  generate_signature(mh, pk1, sk1, sig[0]);
  /* Check the signature */
  res = check_signature(mh, pk1, sig[0]);
  assert(res);
  /* Sign the message using ring signature */
  /* First, generate a key image */
  generate_key_image(pk2, sk2, ki);
  /* Then, generate the signature */
  ppk[0] = &pk1;
  ppk[1] = &pk2;
  generate_ring_signature(mh, ki, ppk, 2, sk2, 1, sig);
  /* Check it */
  res = check_ring_signature(mh, ki, ppk, 2, sig);
  assert(res);
  return 0;
}