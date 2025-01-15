// SPDX-License-Identifier: ISC
// SPDX-FileCopyrightText: 2013-2024 Frank Denis <j at pureftpd dot org>

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// fix naming collision with libsodium
#define crypto_verify_32 monero_crypto_verify_32

int crypto_verify_32(const unsigned char *x, const unsigned char *y)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif
