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
#include "address_utils_compat.h"

//local headers
#include "cryptonote_config.h"
#include "int-util.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void make_legacy_subaddress_extension(const crypto::secret_key &k_view,
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &legacy_subaddress_extension_out)
{
    if (!major_index && !minor_index)
    {
        sc_0(to_bytes(legacy_subaddress_extension_out));
        return;
    }

    char data[sizeof(config::HASH_KEY_SUBADDRESS) + sizeof(crypto::secret_key) + 2 * sizeof(uint32_t)];

    // "Subaddr" || IntToBytes(0)
    memcpy(data, config::HASH_KEY_SUBADDRESS, sizeof(config::HASH_KEY_SUBADDRESS));

    // ... || k_v
    memcpy(data + sizeof(config::HASH_KEY_SUBADDRESS), &k_view, sizeof(crypto::secret_key));

    // ... || IntToBytes32(j_major)
    uint32_t idx = SWAP32LE(major_index);
    memcpy(data + sizeof(config::HASH_KEY_SUBADDRESS) + sizeof(crypto::secret_key), &idx, sizeof(uint32_t));

    // ... || IntToBytes32(j_minor)
    idx = SWAP32LE(minor_index);
    memcpy(data + sizeof(config::HASH_KEY_SUBADDRESS) + sizeof(crypto::secret_key) + sizeof(uint32_t),
        &idx,
        sizeof(uint32_t));

    // k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
    crypto::hash_to_scalar(data, sizeof(data), legacy_subaddress_extension_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_legacy_subaddress_spend_pubkey(const crypto::secret_key &legacy_subaddress_extension,
    const crypto::public_key &account_spend_pubkey,
    crypto::public_key &legacy_subaddress_spend_pubkey_out)
{
    // K^j_s = K_s + k^j_subext G
    rct::key res_k;
    rct::addKeys1(res_k, rct::sk2rct(legacy_subaddress_extension), rct::pk2rct(account_spend_pubkey));
    legacy_subaddress_spend_pubkey_out = rct::rct2pk(res_k);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
