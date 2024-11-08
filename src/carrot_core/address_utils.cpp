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
#include "address_utils.h"

//local headers
#include "config.h"
#include "hash_functions.h"
#include "ringct/rctOps.h"
#include "transcript_fixed.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_index_extension_generator(const crypto::secret_key &s_generate_address,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::secret_key &address_generator_out)
{
    // s^j_gen = H_32[s_ga](j_major, j_minor)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_ADDRESS_INDEX_GEN>(j_major, j_minor);
    derive_bytes_32(transcript.data(), transcript.size(), &s_generate_address, &address_generator_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_subaddress_scalar(const crypto::public_key &account_spend_pubkey,
    const crypto::secret_key &s_address_generator,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::secret_key &subaddress_scalar_out)
{
    // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
    const auto transcript = sp::make_fixed_transcript<CARROT_DOMAIN_SEP_SUBADDRESS_SCALAR>(
        account_spend_pubkey, j_major, j_minor);
    derive_scalar(transcript.data(), transcript.size(), &s_address_generator, subaddress_scalar_out.data);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_address_spend_pubkey(const crypto::public_key &account_spend_pubkey,
    const crypto::secret_key &s_generate_address,
    const std::uint32_t j_major,
    const std::uint32_t j_minor,
    crypto::public_key &address_spend_pubkey_out)
{
    // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
    crypto::secret_key subaddress_scalar;
    make_carrot_subaddress_scalar(account_spend_pubkey, s_generate_address, j_major, j_minor, subaddress_scalar);

    // K^j_s = k^j_subscal * K_s
    address_spend_pubkey_out = rct::rct2pk(rct::scalarmultKey(
        rct::pk2rct(account_spend_pubkey), rct::sk2rct(subaddress_scalar)));
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
