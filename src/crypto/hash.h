// Copyright (c) 2014-2018, The Monero Project
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

#pragma once

#include <stddef.h>
#include <iostream>

#include "common/pod-class.h"
#include "generic-ops.h"
#include "hex.h"
#include "span.h"
#include "crypto/cn_heavy_hash.hpp"

namespace crypto {

  extern "C" {
#include "hash-ops.h"
  }

#pragma pack(push, 1)
  POD_CLASS hash {
    char data[HASH_SIZE];
  };
  POD_CLASS hash8 {
    char data[8];
  };
#pragma pack(pop)

  static_assert(sizeof(hash) == HASH_SIZE, "Invalid structure size");
  static_assert(sizeof(hash8) == 8, "Invalid structure size");

  /*
    Cryptonight hash functions
  */

  inline void cn_fast_hash(const void *data, std::size_t length, hash &hash) {
    cn_fast_hash(data, length, reinterpret_cast<char *>(&hash));
  }

  inline hash cn_fast_hash(const void *data, std::size_t length) {
    hash h;
    cn_fast_hash(data, length, reinterpret_cast<char *>(&h));
    return h;
  }

  enum struct cn_slow_hash_type
  {
      heavy_v1,
      heavy_v2,
      turtle_lite_v2,
  };

  inline void cn_slow_hash(const void *data, std::size_t length, hash &hash, cn_slow_hash_type type) {
    switch(type)
    {
      case cn_slow_hash_type::heavy_v1:
      case cn_slow_hash_type::heavy_v2:
      {
        static thread_local cn_heavy_hash_v2 v2;
        static thread_local cn_heavy_hash_v1 v1 = cn_heavy_hash_v1::make_borrowed(v2);

        if (type == cn_slow_hash_type::heavy_v1) v1.hash(data, length, hash.data);
        else                                     v2.hash(data, length, hash.data);
      }
      break;

      case cn_slow_hash_type::turtle_lite_v2:
      default:
      {
         const uint32_t CN_TURTLE_PAGE_SIZE = 262144;
         const uint32_t CN_TURTLE_SCRATCHPAD = 262144;
         const uint32_t CN_TURTLE_ITERATIONS = 131072;
         cn_turtle_hash(data,
             length,
             hash.data,
             1, // light
             2, // variant
             0, // pre-hashed
             CN_TURTLE_PAGE_SIZE, CN_TURTLE_SCRATCHPAD, CN_TURTLE_ITERATIONS);
      }
      break;
    }
  }

  inline void tree_hash(const hash *hashes, std::size_t count, hash &root_hash) {
    tree_hash(reinterpret_cast<const char (*)[HASH_SIZE]>(hashes), count, reinterpret_cast<char *>(&root_hash));
  }

  inline std::ostream &operator <<(std::ostream &o, const crypto::hash &v) {
    epee::to_hex::formatted(o, epee::as_byte_span(v)); return o;
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::hash8 &v) {
    epee::to_hex::formatted(o, epee::as_byte_span(v)); return o;
  }

  const static crypto::hash null_hash = {};
  const static crypto::hash8 null_hash8 = {};
}

CRYPTO_MAKE_HASHABLE(hash)
CRYPTO_MAKE_COMPARABLE(hash8)
