/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_UTIL_HEADER_DEFINED
#define NS_UTIL_HEADER_DEFINED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAX_PATH_SIZE
#define MAX_PATH_SIZE 500
#endif

const char *ns_skip(const char *, const char *, const char *, struct ns_str *);
int ns_ncasecmp(const char *s1, const char *s2, size_t len);
int ns_vcmp(const struct ns_str *str2, const char *str1);
int ns_vcasecmp(const struct ns_str *str2, const char *str1);
void ns_base64_decode(const unsigned char *s, int len, char *dst);
void ns_base64_encode(const unsigned char *src, int src_len, char *dst);
int ns_stat(const char *path, ns_stat_t *st);
FILE *ns_fopen(const char *path, const char *mode);
int ns_open(const char *path, int flag, int mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif  /* NS_UTIL_HEADER_DEFINED */
