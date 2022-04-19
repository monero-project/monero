extern int crypto_hashblocks(unsigned char *statebytes,const unsigned char *in,unsigned long long inlen);
extern int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen);

#define crypto_hash_sha512_BYTES 64
