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

//paired header
#include "hash_functions.h"

//local headers
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "crypto/blake2b.h"
#include "exceptions.h"
#include "misc_log_ex.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
// H_x[k](data)
// - if derivation_key == nullptr, then the hash is NOT keyed
//-------------------------------------------------------------------------------------------------------------------
static void hash_base(const void *derivation_key,  //32 bytes
    const void *data,
    const std::size_t data_length,
    void *hash_out,
    const std::size_t out_length)
{
    CARROT_CHECK_AND_THROW(blake2b(hash_out,
            out_length,
            data,
            data_length,
            derivation_key,
            derivation_key ? 32 : 0) == 0,
        crypto_function_failed, "carrot hash base: blake2b failed");
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void derive_bytes_3(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_3(x): 2-byte output
    hash_base(key, data, data_length, hash_out, 3);
}
//-------------------------------------------------------------------------------------------------------------------
void derive_bytes_8(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_8(x): 8-byte output
    hash_base(key, data, data_length, hash_out, 8);
}
//-------------------------------------------------------------------------------------------------------------------
void derive_bytes_16(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_16(x): 16-byte output
    hash_base(key, data, data_length, hash_out, 16);
}
//-------------------------------------------------------------------------------------------------------------------
void derive_bytes_32(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_32(x): 32-byte output
    hash_base(key, data, data_length, hash_out, 32);
}
//-------------------------------------------------------------------------------------------------------------------
void derive_bytes_64(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_64(x): 64-byte output
    hash_base(key, data, data_length, hash_out, 64);
}
//-------------------------------------------------------------------------------------------------------------------
void derive_scalar(const void *data, const std::size_t data_length, const void *key, void *hash_out)
{
    // H_n(x): Ed25519 group scalar output (32 bytes)
    // note: hash to 64 bytes then mod l
    unsigned char temp[64];
    hash_base(key, data, data_length, temp, 64);
    sc_reduce(temp);  //mod l
    memcpy(hash_out, temp, 32);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
