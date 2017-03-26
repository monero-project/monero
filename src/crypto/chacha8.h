// Copyright (c) 2014-2017, The Monero Project
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

#include <stdint.h>
#include <stddef.h>

#define CHACHA8_KEY_SIZE 32
#define CHACHA8_IV_SIZE 8

#if defined(__cplusplus)
#include <memory.h>

#include "hash.h"

namespace crypto {
  extern "C" {
#endif
    void chacha8(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher);
#if defined(__cplusplus)
  }

#pragma pack(push, 1)
  struct chacha8_key {
    uint8_t data[CHACHA8_KEY_SIZE];

    ~chacha8_key()
    {
      memset(data, 0, sizeof(data));
    }
  };

  // MS VC 2012 doesn't interpret `class chacha8_iv` as POD in spite of [9.0.10], so it is a struct
  struct chacha8_iv {
    uint8_t data[CHACHA8_IV_SIZE];
  };
#pragma pack(pop)

  static_assert(sizeof(chacha8_key) == CHACHA8_KEY_SIZE && sizeof(chacha8_iv) == CHACHA8_IV_SIZE, "Invalid structure size");

  inline void chacha8(const void* data, std::size_t length, const chacha8_key& key, const chacha8_iv& iv, char* cipher) {
    chacha8(data, length, reinterpret_cast<const uint8_t*>(&key), reinterpret_cast<const uint8_t*>(&iv), cipher);
  }

  inline void generate_chacha8_key(const void *data, size_t size, chacha8_key& key) {
    static_assert(sizeof(chacha8_key) <= sizeof(hash), "Size of hash must be at least that of chacha8_key");
    char pwd_hash[HASH_SIZE];
    crypto::cn_slow_hash(data, size, pwd_hash);
    memcpy(&key, pwd_hash, sizeof(key));
    memset(pwd_hash, 0, sizeof(pwd_hash));
  }

  inline void generate_chacha8_key(std::string password, chacha8_key& key) {
    return generate_chacha8_key(password.data(), password.size(), key);
  }
}

#endif
