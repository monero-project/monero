// Copyright (c) 2014-2022, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "crypto/crypto.cpp"

#include "crypto-tests.h"

static void get_ge_p3_for_identity_test(const crypto::public_key &point, crypto::ge_p3 &result_out_p3)
{
  // compute (K + K) - K - K to get a specific ge_p3 point representation of identity
  crypto::ge_cached temp_cache;
  crypto::ge_p1p1 temp_p1p1;

  crypto::ge_frombytes_vartime(&result_out_p3, &point);  // K
  crypto::ge_p3_to_cached(&temp_cache, &result_out_p3);
  crypto::ge_add(&temp_p1p1, &result_out_p3, &temp_cache);  // K + K
  crypto::ge_p1p1_to_p3(&result_out_p3, &temp_p1p1);
  crypto::ge_sub(&temp_p1p1, &result_out_p3, &temp_cache);  // (K + K) - K
  crypto::ge_p1p1_to_p3(&result_out_p3, &temp_p1p1);
  crypto::ge_sub(&temp_p1p1, &result_out_p3, &temp_cache);  // ((K + K) - K) - K
  crypto::ge_p1p1_to_p3(&result_out_p3, &temp_p1p1);
}

static int ge_p3_is_point_at_infinity_vartime_bad(const crypto::ge_p3 *p) {
  // X = 0 and Y == Z
  // bad: components of 'p' are not reduced mod q
  int n;
  for (n = 0; n < 10; ++n)
  {
    if (p->X[n] | p->T[n])
      return 0;
    if (p->Y[n] != p->Z[n])
      return 0;
  }
  return 1;
}

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

bool check_ge_p3_identity_failure(const crypto::public_key &point)
{
  crypto::ge_p3 ident_p3;
  get_ge_p3_for_identity_test(point, ident_p3);

  return ge_p3_is_point_at_infinity_vartime_bad(&ident_p3) == 1;
}

bool check_ge_p3_identity_success(const crypto::public_key &point)
{
  crypto::ge_p3 ident_p3;
  get_ge_p3_for_identity_test(point, ident_p3);

  return crypto::ge_p3_is_point_at_infinity_vartime(&ident_p3) == 1;
}
