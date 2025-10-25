// Copyright (c) 2025, The Monero Project
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

//local headers
#include "crypto/crypto.h"

//third party headers

//standard headers
#include <cstdint>

//forward declarations
namespace carrot
{
struct cryptonote_hierarchy_address_device;
}

namespace carrot
{
/**
 * brief: make_legacy_subaddress_extension - k^j_subext
 *   k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
 * param: k_view - k_v
 * param: major_index - j_major
 * param: minor_index - j_minor
 * outparam: legacy_subaddress_extension_out - k^j_subext
 */
void make_legacy_subaddress_extension(const crypto::secret_key &k_view,
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &legacy_subaddress_extension_out);
/**
 * brief: make_legacy_subaddress_spend_pubkey - K^j_s
 *   K^j_s = K_s + k^j_subext G
 * param: legacy_subaddress_extension_out - k^j_subext
 * param: account_spend_pubkey - K_s
 * param: addr_dev -
 * param: major_index - j_major
 * param: minor_index - j_minor
 * outparam: legacy_subaddress_spend_pubkey_out - K^j_s
 */
void make_legacy_subaddress_spend_pubkey(const crypto::secret_key &legacy_subaddress_extension,
    const crypto::public_key &account_spend_pubkey,
    crypto::public_key &legacy_subaddress_spend_pubkey_out);
void make_legacy_subaddress_spend_pubkey(const cryptonote_hierarchy_address_device &addr_dev,
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::public_key &legacy_subaddress_spend_pubkey_out);
/**
 * brief: make_legacy_subaddress_spend_pubkey - K^j_s
 *   K^j_s = K_s + k^j_subext G
 *   K^j_v = k_v (G if j=0, else K^j_s)
 * param: addr_dev
 * param: major_index - j_major
 * param: minor_index - j_minor
 * param: account_spend_pubkey - K_s
 * outparam: legacy_subaddress_spend_pubkey_out - K^j_s
 * outparam: legacy_subaddress_view_pubkey_out - K^j_v
 */
void make_legacy_subaddress_pubkeys(const cryptonote_hierarchy_address_device &addr_dev,
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::public_key &legacy_subaddress_spend_pubkey_out,
    crypto::public_key &legacy_subaddress_view_pubkey_out);
} //namespace carrot
