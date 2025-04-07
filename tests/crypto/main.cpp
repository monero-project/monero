// Copyright (c) 2014-2024, The Monero Project
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

#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "warnings.h"
#include "misc_log_ex.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto-tests.h"
#include "../io.h"

using namespace std;
using namespace crypto;
typedef crypto::hash chash;

bool operator !=(const ec_scalar &a, const ec_scalar &b) {
  return 0 != memcmp(&a, &b, sizeof(ec_scalar));
}

bool operator !=(const key_derivation &a, const key_derivation &b) {
  return 0 != memcmp(&a, &b, sizeof(key_derivation));
}

DISABLE_GCC_WARNING(maybe-uninitialized)

int main(int argc, char *argv[]) {
  TRY_ENTRY();
  fstream input;
  string cmd;
  size_t test = 0;
  bool error = false;
  setup_random();
  if (argc != 2) {
    cerr << "invalid arguments" << endl;
    return 1;
  }
  input.open(argv[1], ios_base::in);
  for (;;) {
    ++test;
    input.exceptions(ios_base::badbit);
    if (!(input >> cmd)) {
      break;
    }
    input.exceptions(ios_base::badbit | ios_base::failbit | ios_base::eofbit);
    if (cmd == "check_scalar") {
      ec_scalar scalar;
      bool expected, actual;
      get(input, scalar, expected);
      actual = check_scalar(scalar);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "random_scalar") {
      ec_scalar expected, actual;
      get(input, expected);
      random_scalar(actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hash_to_scalar") {
      vector<char> data;
      ec_scalar expected, actual;
      get(input, data, expected);
      crypto::hash_to_scalar(data.data(), data.size(), actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generate_keys") {
      public_key expected1, actual1;
      secret_key expected2, actual2;
      get(input, expected1, expected2);
      generate_keys(actual1, actual2);
      if (expected1 != actual1 || expected2 != actual2) {
        goto error;
      }
    } else if (cmd == "check_key") {
      public_key key;
      bool expected, actual;
      get(input, key, expected);
      actual = check_key(key);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "secret_key_to_public_key") {
      secret_key sec;
      bool expected1, actual1;
      public_key expected2, actual2;
      get(input, sec, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = secret_key_to_public_key(sec, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "generate_key_derivation") {
      public_key key1;
      secret_key key2;
      bool expected1, actual1;
      key_derivation expected2, actual2;
      get(input, key1, key2, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = generate_key_derivation(key1, key2, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "derive_public_key") {
      key_derivation derivation;
      size_t output_index;
      public_key base;
      bool expected1, actual1;
      public_key expected2, actual2;
      get(input, derivation, output_index, base, expected1);
      if (expected1) {
        get(input, expected2);
      }
      actual1 = derive_public_key(derivation, output_index, base, actual2);
      if (expected1 != actual1 || (expected1 && expected2 != actual2)) {
        goto error;
      }
    } else if (cmd == "derive_secret_key") {
      key_derivation derivation;
      size_t output_index;
      secret_key base;
      secret_key expected, actual;
      get(input, derivation, output_index, base, expected);
      derive_secret_key(derivation, output_index, base, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generate_signature") {
      chash prefix_hash;
      public_key pub;
      secret_key sec;
      signature expected, actual;
      get(input, prefix_hash, pub, sec, expected);
      generate_signature(prefix_hash, pub, sec, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "check_signature") {
      chash prefix_hash;
      public_key pub;
      signature sig;
      bool expected, actual;
      get(input, prefix_hash, pub, sig, expected);
      actual = check_signature(prefix_hash, pub, sig);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hash_to_point") {
      chash h;
      ec_point expected, actual;
      get(input, h, expected);
      hash_to_point(h, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "hash_to_ec") {
      public_key key;
      ec_point expected, actual;
      get(input, key, expected);
      hash_to_ec(key, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generate_key_image") {
      public_key pub;
      secret_key sec;
      key_image expected, actual;
      get(input, pub, sec, expected);
      generate_key_image(pub, sec, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "generate_ring_signature") {
      chash prefix_hash;
      key_image image;
      vector<public_key> vpubs;
      vector<const public_key *> pubs;
      size_t pubs_count;
      secret_key sec;
      size_t sec_index;
      vector<signature> expected, actual;
      size_t i;
      get(input, prefix_hash, image, pubs_count);
      vpubs.resize(pubs_count);
      pubs.resize(pubs_count);
      for (i = 0; i < pubs_count; i++) {
        get(input, vpubs[i]);
        pubs[i] = &vpubs[i];
      }
      get(input, sec, sec_index);
      expected.resize(pubs_count);
      getvar(input, pubs_count * sizeof(signature), expected.data());
      actual.resize(pubs_count);
      generate_ring_signature(prefix_hash, image, pubs.data(), pubs_count, sec, sec_index, actual.data());
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "check_ring_signature") {
      chash prefix_hash;
      key_image image;
      vector<public_key> vpubs;
      vector<const public_key *> pubs;
      size_t pubs_count;
      vector<signature> sigs;
      bool expected, actual;
      size_t i;
      get(input, prefix_hash, image, pubs_count);
      vpubs.resize(pubs_count);
      pubs.resize(pubs_count);
      for (i = 0; i < pubs_count; i++) {
        get(input, vpubs[i]);
        pubs[i] = &vpubs[i];
      }
      sigs.resize(pubs_count);
      getvar(input, pubs_count * sizeof(signature), sigs.data());
      get(input, expected);
      actual = check_ring_signature(prefix_hash, image, pubs.data(), pubs_count, sigs.data());
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "check_ge_p3_identity") {
      public_key point;
      bool expected_bad, expected_good, result_badfunc, result_goodfunc;
      get(input, point, expected_bad, expected_good);
      result_badfunc = check_ge_p3_identity_failure(point);
      result_goodfunc = check_ge_p3_identity_success(point);
      if (expected_bad != result_badfunc || expected_good != result_goodfunc) {
        goto error;
      }
    } else if (cmd == "derive_view_tag") {
      key_derivation derivation;
      size_t output_index;
      view_tag expected, actual;
      get(input, derivation, output_index, expected);
      derive_view_tag(derivation, output_index, actual);
      if (expected != actual) {
        goto error;
      }
    } else if (cmd == "key_image_to_y") {
      key_image ki;
      key_image_y expected_ki_y, actual_ki_y;
      bool expected_sign, actual_sign;
      get(input, ki, expected_ki_y, expected_sign);
      actual_sign = key_image_to_y(ki, actual_ki_y);
      if (expected_ki_y != actual_ki_y || expected_sign != actual_sign) {
        goto error;
      }
    } else if (cmd == "key_image_from_y") {
      key_image_y ki_y;
      bool sign;
      key_image expected_ki, actual_ki;
      get(input, ki_y, sign, expected_ki);
      key_image_from_y(ki_y, sign, actual_ki);
      if (expected_ki != actual_ki) {
        goto error;
      }
    } else {
      throw ios_base::failure("Unknown function: " + cmd);
    }
    continue;
error:
    cerr << "Wrong result on test " << test << endl;
    error = true;
  }
  return error ? 1 : 0;
  CATCH_ENTRY_L0("main", 1);
}
