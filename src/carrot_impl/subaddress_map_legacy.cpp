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

//paired header
#include "subaddress_map_legacy.h"

//local headers

//third party headers

//standard headers
#include <algorithm>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
subaddress_map_legacy::subaddress_map_legacy(
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map_legacy)
:
    m_subaddress_map_legacy(subaddress_map_legacy)
{}
//-------------------------------------------------------------------------------------------------------------------
std::optional<subaddress_index_extended> subaddress_map_legacy::get_index_for_address_spend_pubkey(
    const crypto::public_key &address_spend_pubkey) const
{
    const auto it = m_subaddress_map_legacy.find(address_spend_pubkey);
    if (it == m_subaddress_map_legacy.cend())
        return std::nullopt;
    const cryptonote::subaddress_index &subaddr_index = it->second;
    return subaddress_index_extended{
        .index = subaddress_index{
            .major = subaddr_index.major,
            .minor = subaddr_index.minor
        },
        .derive_type = AddressDeriveType::PreCarrot
    };
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<crypto::public_key> subaddress_map_legacy::get_address_spend_pubkey_for_index(
    const subaddress_index_extended &subaddr_index) const
{
    if (subaddr_index.derive_type != AddressDeriveType::PreCarrot)
        return std::nullopt;
    const auto it = std::find_if(m_subaddress_map_legacy.cbegin(), m_subaddress_map_legacy.cend(),
        [&subaddr_index](const std::pair<const crypto::public_key, cryptonote::subaddress_index> &e) {
            return e.second.major == subaddr_index.index.major && e.second.minor == subaddr_index.index.minor;
        }
    );
    if (it == m_subaddress_map_legacy.cend())
        return std::nullopt;
    return it->first;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
