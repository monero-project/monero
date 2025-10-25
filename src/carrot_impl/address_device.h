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
#include "carrot_core/device.h"
#include "subaddress_index.h"

//third party headers

//standard headers

//forward declarations

namespace carrot
{
static constexpr const int E_UNSUPPORTED_ADDRESS_TYPE = 1;

struct cryptonote_hierarchy_address_device: virtual public view_incoming_key_device
{
    /**
     * brief: get_cryptonote_account_spend_pubkey - K_s
     * return: K_s
     */
    virtual crypto::public_key get_cryptonote_account_spend_pubkey() const = 0;

    /**
     * brief: make_legacy_subaddress_extension - k^j_subext
     *   k^j_subext = ScalarDeriveLegacy("SubAddr" || IntToBytes8(0) || k_v || IntToBytes32(j_major) || IntToBytes32(j_minor))
     * param: major_index - j_major
     * param: minor_index - j_minor
     * outparam: legacy_subaddress_extension_out - k^j_subext
     * throw: std::invalid_argument if major_index == minor_index == 0
     */
    virtual void make_legacy_subaddress_extension(const std::uint32_t major_index,
        const std::uint32_t minor_index,
        crypto::secret_key &legacy_subaddress_extension_out) const = 0;
};

struct carrot_hierarchy_address_device: public generate_address_secret_device
{
    /**
     * brief: get_carrot_account_spend_pubkey - K_s = K^0_s
     * return: K_s = K^0_s
     */
    virtual crypto::public_key get_carrot_account_spend_pubkey() const = 0;

    /**
     * brief: get_carrot_account_view_pubkey - K_v
     * return: K_v
     */
    virtual crypto::public_key get_carrot_account_view_pubkey() const = 0;

    /**
     * brief: get_carrot_main_address_view_pubkey - K^0_v
     * return: K^0_v
     */
    virtual crypto::public_key get_carrot_main_address_view_pubkey() const = 0;
};

struct hybrid_hierarchy_address_device
{
    virtual bool supports_address_derivation_type(AddressDeriveType derive_type) const = 0;

    virtual const cryptonote_hierarchy_address_device &access_cryptonote_hierarchy_device() const = 0;

    virtual const carrot_hierarchy_address_device &access_carrot_hierarchy_device() const = 0;

    virtual ~hybrid_hierarchy_address_device() = default;
};

struct interactive_address_device
{
    virtual void request_explicit_on_device_address_confirmation(
        const subaddress_index_extended &index) = 0;

    virtual ~interactive_address_device() = default;
};
} //namespace carrot
