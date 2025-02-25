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

// Utilities for building carrot addresses.

#pragma once

//local headers
#include "crypto/crypto.h"

//third party headers

//standard headers

//forward declarations


namespace carrot
{

/**
* brief: is_main_address_index - determine whether j=(j_major, j_minor) represents the main address
*/
static constexpr bool is_main_address_index(const std::uint32_t j_major, const std::uint32_t j_minor)
{
    return !(j_major || j_minor);
}

/**
* brief: make_carrot_index_extension_generator - s^j_gen
*   s^j_gen = H_32[s_ga](j_major, j_minor)
* param: s_generate_address - s_ga
* param: j_major -
* param: j_minor -
* outparam: address_generator_out - s^j_gen
*/
void make_carrot_index_extension_generator(const crypto::secret_key &s_generate_address,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::secret_key &address_generator_out);
/**
* brief: make_carrot_address_privkey - d^j_a
*   k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
* param: account_spend_pubkey - K_s = k_vb X + k_m U
* param: s_address_generator - s^j_gen
* param: j_major -
* param: j_minor -
* outparam: subaddress_scalar_out - k^j_subscal
*/
void make_carrot_subaddress_scalar(const crypto::public_key &account_spend_pubkey,
    const crypto::secret_key &s_address_generator,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::secret_key &subaddress_scalar_out);
/**
* brief: make_carrot_address_spend_pubkey - K^j_s
*   K^j_s = k^j_subscal * K_s
* param: account_spend_pubkey - K_s = k_gi G + k_ps U
* param: s_generate_address - s_ga
* param: j_major -
* param: j_minor -
* outparam: address_spend_pubkey_out - K^j_s
*/
void make_carrot_address_spend_pubkey(const crypto::public_key &account_spend_pubkey,
    const crypto::secret_key &s_generate_address,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::public_key &address_spend_pubkey_out);

} //namespace carrot
