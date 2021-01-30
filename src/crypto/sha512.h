#ifndef SHA512_H
#define SHA512_H 1
#ifdef __cplusplus
extern "C" {
#endif

int crypto_hashblocks(unsigned char *statebytes,const unsigned char *in,unsigned long long inlen);
int crypto_hash_sha512(unsigned char *out,const unsigned char *in,unsigned long long inlen);

#define crypto_hash_sha512_BYTES 64

#ifdef __cplusplus
}
#endif
#endif