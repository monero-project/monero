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

#pragma once

#include <stdint.h>
#include <stddef.h>

#define TWOFISH_KEY_SIZE 32
#define TWOFISH_IV_SIZE 16

#if defined(__cplusplus)
#include "genkey.h"

namespace crypto {
  extern "C" {
#endif
    [[maybe_unused]] static size_t twofish_encrypted_size(size_t length) {
      constexpr size_t twofish_block_size = 16;
      return (length / twofish_block_size + 1) * twofish_block_size;
    }

    void   twofish_encrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher);
    size_t twofish_decrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* plain);
#if defined(__cplusplus)
  }

  using twofish_key = cipher_key<TWOFISH_KEY_SIZE>;

#pragma pack(push, 1)
  // MS VC 2012 doesn't interpret `class chacha_iv` as POD in spite of [9.0.10], so it is a struct
  struct chacha_iv {
    uint8_t data[TWOFISH_IV_SIZE];
  };
#pragma pack(pop)

  static_assert(sizeof(twofish_key) == TWOFISH_KEY_SIZE && sizeof(chacha_iv) == TWOFISH_IV_SIZE, "Invalid structure size");


  inline void twofish_encrypt(const void* data, std::size_t length, const twofish_key& key, const chacha_iv& iv, char* cipher) {
    twofish_encrypt(data, length, key.data(), iv.data, cipher);
  }

  inline void twofish_decrypt(const void* data, std::size_t length, const twofish_key& key, const chacha_iv& iv, char* plain) {
    twofish_decrypt(data, length, key.data(), iv.data, plain);
  }

  inline void generate_twofish_key(const void *data, size_t size, twofish_key& key, uint64_t kdf_rounds) {
    generate_cipher_key(data, size, key, kdf_rounds);
  }

  inline void generate_twofish_key_prehashed(const void *data, size_t size, twofish_key& key, uint64_t kdf_rounds) {
    generate_cipher_key_prehashed(data, size, key, kdf_rounds);
  }

  inline void generate_twofish_key(std::string password, twofish_key& key, uint64_t kdf_rounds) {
    return generate_twofish_key(password.data(), password.size(), key, kdf_rounds);
  }
}

#endif
