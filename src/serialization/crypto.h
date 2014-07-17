// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>

#include "serialization.h"
#include "debug_archive.h"
#include "crypto/chacha8.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"

// read
template <template <bool> class Archive>
bool do_serialize(Archive<false> &ar, std::vector<crypto::signature> &v)
{
  size_t cnt = v.size();
  v.clear();

  // very basic sanity check
  if (ar.remaining_bytes() < cnt*sizeof(crypto::signature)) {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  v.reserve(cnt);
  for (size_t i = 0; i < cnt; i++) {
    v.resize(i+1);
    ar.serialize_blob(&(v[i]), sizeof(crypto::signature), "");
    if (!ar.stream().good())
      return false;
  }
  return true;
}

// write
template <template <bool> class Archive>
bool do_serialize(Archive<true> &ar, std::vector<crypto::signature> &v)
{
  if (0 == v.size()) return true;
  ar.begin_string();
  size_t cnt = v.size();
  for (size_t i = 0; i < cnt; i++) {
    ar.serialize_blob(&(v[i]), sizeof(crypto::signature), "");
    if (!ar.stream().good())
      return false;
  }
  ar.end_string();
  return true;
}

BLOB_SERIALIZER(crypto::chacha8_iv);
BLOB_SERIALIZER(crypto::hash);
BLOB_SERIALIZER(crypto::public_key);
BLOB_SERIALIZER(crypto::secret_key);
BLOB_SERIALIZER(crypto::key_derivation);
BLOB_SERIALIZER(crypto::key_image);
BLOB_SERIALIZER(crypto::signature);
VARIANT_TAG(debug_archive, crypto::hash, "hash");
VARIANT_TAG(debug_archive, crypto::public_key, "public_key");
VARIANT_TAG(debug_archive, crypto::secret_key, "secret_key");
VARIANT_TAG(debug_archive, crypto::key_derivation, "key_derivation");
VARIANT_TAG(debug_archive, crypto::key_image, "key_image");
VARIANT_TAG(debug_archive, crypto::signature, "signature");
