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

#include <memory.h>

#include "memwipe.h"
#include "mlocker.h"
#include "hash.h"

namespace crypto {
	template<size_t KeySize>
  using cipher_key = epee::mlocked<tools::scrubbed_arr<uint8_t, KeySize>>;

	template<size_t KeySize>
  inline void generate_cipher_key(const void *data, size_t size, cipher_key<KeySize>& key, uint64_t kdf_rounds) {
    static_assert(sizeof(key) <= sizeof(hash), "Size of hash must be at least that of key");
    epee::mlocked<tools::scrubbed_arr<char, HASH_SIZE>> pwd_hash;
    crypto::cn_slow_hash(data, size, pwd_hash.data(), 0/*variant*/, 0/*prehashed*/, 0/*height*/);
    for (uint64_t n = 1; n < kdf_rounds; ++n)
      crypto::cn_slow_hash(pwd_hash.data(), pwd_hash.size(), pwd_hash.data(), 0/*variant*/, 0/*prehashed*/, 0/*height*/);
    memcpy(&unwrap(unwrap(key)), pwd_hash.data(), sizeof(key));
  }

	template<size_t KeySize>
  inline void generate_cipher_key_prehashed(const void *data, size_t size, cipher_key<KeySize>& key, uint64_t kdf_rounds) {
    static_assert(sizeof(key) <= sizeof(hash), "Size of hash must be at least that of key");
    epee::mlocked<tools::scrubbed_arr<char, HASH_SIZE>> pwd_hash;
    crypto::cn_slow_hash(data, size, pwd_hash.data(), 0/*variant*/, 1/*prehashed*/, 0/*height*/);
    for (uint64_t n = 1; n < kdf_rounds; ++n)
      crypto::cn_slow_hash(pwd_hash.data(), pwd_hash.size(), pwd_hash.data(), 0/*variant*/, 0/*prehashed*/, 0/*height*/);
    memcpy(&unwrap(unwrap(key)), pwd_hash.data(), sizeof(key));
  }
}
