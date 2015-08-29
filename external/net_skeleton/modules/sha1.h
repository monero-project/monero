/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#if !defined(NS_SHA1_HEADER_INCLUDED) && !defined(NS_DISABLE_SHA1)
#define NS_SHA1_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Init(SHA1_CTX *);
void SHA1Update(SHA1_CTX *, const unsigned char *data, uint32_t len);
void SHA1Final(unsigned char digest[20], SHA1_CTX *);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* NS_SHA1_HEADER_INCLUDED */
