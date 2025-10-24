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
#include "span.h"

//third party headers

//standard headers
#include <cstdint>

//forward declarations
namespace carrot
{
struct address_device;
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
 * brief: get all supported main address spend pubkeys K_s from a hybrid address device
 * param: addr_dev -
 * outparam: main_address_spend_pubkeys_out -
 * return: number of supported pubkeys or span to supported main_address_spend_pubkeys_out
 */
std::size_t get_all_main_address_spend_pubkeys(
    const address_device &addr_dev,
    crypto::public_key main_address_spend_pubkeys_out[2]);
epee::span<const crypto::public_key> get_all_main_address_spend_pubkeys_span(
    const address_device &addr_dev,
    crypto::public_key main_address_spend_pubkeys_out[2]);
} //namespace carrot
