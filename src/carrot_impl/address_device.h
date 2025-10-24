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

struct address_device
{
    /**
     * brief: get K^j_s given j
     * param: subaddr_index - j
     * outparam: address_spend_pubkey_out - K^j_s
     */
    virtual void get_address_spend_pubkey(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out) const = 0;

    /**
     * brief: get (K^j_s, K^j_v) given j
     * param: subaddr_index - j
     * outparam: address_spend_pubkey_out - K^j_s
     * outparam: address_view_pubkey_out - K^j_v
     */
    virtual void get_address_pubkeys(const subaddress_index_extended &subaddr_index,
        crypto::public_key &address_spend_pubkey_out,
        crypto::public_key &address_view_pubkey_out) const = 0;

    /**
     * get (k^j_subext, k^j_subscalar) given j s.t. K^j_s = k^j_subscalar K_s + k^j_subext G
     */
    virtual void get_address_openings(const subaddress_index_extended &subaddr_index,
        crypto::secret_key &address_extension_g_out,
        crypto::secret_key &address_scalar_out) const = 0;

    virtual ~address_device() = default;
};
} //namespace carrot
