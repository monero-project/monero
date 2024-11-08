// Copyright (c) 2024, The Monero Project
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

// Core hash functions for Seraphis (note: this implementation satisfies the Jamtis specification).

#pragma once

//local headers

//third party headers

//standard headers
#include <cstddef>

//forward declarations


namespace carrot
{

/// H_3(x): 3-byte output
void derive_bytes_3(const void *data, const std::size_t data_length, const void *key, void *hash_out);
/// H_8(x): 8-byte output
void derive_bytes_8(const void *data, const std::size_t data_length, const void* key, void *hash_out);
/// H_16(x): 16-byte output
void derive_bytes_16(const void *data, const std::size_t data_length, const void *key, void *hash_out);
/// H_32(x): 32-byte output
void derive_bytes_32(const void *data, const std::size_t data_length, const void *key, void *hash_out);
/// H_64(x): 64-byte output
void derive_bytes_64(const void *data, const std::size_t data_length, const void *key, void *hash_out);
/// H_n(x): unclamped Curve25519/Ed25519 group scalar output (32 bytes)
void derive_scalar(const void *data, const std::size_t data_length, const void *key, void *hash_out);

} //namespace carrot
