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

#include <cstddef>
#include <cstring>
#include <functional>
#include <sodium/crypto_verify_32.h>

#define CRYPTO_MAKE_COMPARABLE(type) \
namespace crypto { \
  inline bool operator==(const type &_v1, const type &_v2) { \
    return !memcmp(&_v1, &_v2, sizeof(_v1)); \
  } \
  inline bool operator!=(const type &_v1, const type &_v2) { \
    return !operator==(_v1, _v2); \
  } \
}

#define CRYPTO_MAKE_COMPARABLE_CONSTANT_TIME(type) \
namespace crypto { \
  inline bool operator==(const type &_v1, const type &_v2) { \
    static_assert(sizeof(_v1) == 32, "constant time comparison is only implenmted for 32 bytes"); \
    return crypto_verify_32((const unsigned char*)&_v1, (const unsigned char*)&_v2) == 0; \
  } \
  inline bool operator!=(const type &_v1, const type &_v2) { \
    return !operator==(_v1, _v2); \
  } \
}

#define CRYPTO_DEFINE_HASH_FUNCTIONS(type) \
namespace crypto { \
  static_assert(sizeof(std::size_t) <= sizeof(type), "Size of " #type " must be at least that of size_t"); \
  inline std::size_t hash_value(const type &_v) { \
    return reinterpret_cast<const std::size_t &>(_v); \
  } \
} \
namespace std { \
  template<> \
  struct hash<crypto::type> { \
    std::size_t operator()(const crypto::type &_v) const { \
      return reinterpret_cast<const std::size_t &>(_v); \
    } \
  }; \
}

#define CRYPTO_MAKE_HASHABLE(type) \
CRYPTO_MAKE_COMPARABLE(type) \
CRYPTO_DEFINE_HASH_FUNCTIONS(type)

#define CRYPTO_MAKE_HASHABLE_CONSTANT_TIME(type) \
CRYPTO_MAKE_COMPARABLE_CONSTANT_TIME(type) \
CRYPTO_DEFINE_HASH_FUNCTIONS(type)

