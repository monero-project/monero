// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto/crypto.cpp"

#include "crypto-tests.h"

bool check_scalar(const crypto::ec_scalar &scalar) {
  return crypto::sc_check(crypto::operator &(scalar)) == 0;
}

void random_scalar(crypto::ec_scalar &res) {
  crypto::random_scalar(res);
}

void hash_to_scalar(const void *data, std::size_t length, crypto::ec_scalar &res) {
  crypto::hash_to_scalar(data, length, res);
}

void hash_to_point(const crypto::hash &h, crypto::ec_point &res) {
  crypto::ge_p2 point;
  crypto::ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
  crypto::ge_tobytes(crypto::operator &(res), &point);
}

void hash_to_ec(const crypto::public_key &key, crypto::ec_point &res) {
  crypto::ge_p3 tmp;
  crypto::hash_to_ec(key, tmp);
  crypto::ge_p3_tobytes(crypto::operator &(res), &tmp);
}
