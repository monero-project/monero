// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__cplusplus)
#include "crypto/crypto.h"

extern "C" {
#endif

void setup_random(void);

#if defined(__cplusplus)
}

bool check_scalar(const crypto::ec_scalar &scalar);
void random_scalar(crypto::ec_scalar &res);
void hash_to_scalar(const void *data, std::size_t length, crypto::ec_scalar &res);
void hash_to_point(const crypto::hash &h, crypto::ec_point &res);
void hash_to_ec(const crypto::public_key &key, crypto::ec_point &res);
#endif
