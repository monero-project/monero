// from djb's ref10 implementation, http://ed25519.cr.yp.to/software.html
//
// public domain

// return 0 if equal, non zero if unequal, constant time
extern int crypto_verify_32(const unsigned char *,const unsigned char *);
