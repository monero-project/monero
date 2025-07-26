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
#include "address_device_ram_borrowed.h"

//local headers
#include "address_utils.h"
#include "carrot_core/address_utils.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void cryptonote_hierarchy_address_device_ram_borrowed::make_legacy_subaddress_extension(
    const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &legacy_subaddress_extension_out) const
{
    return carrot::make_legacy_subaddress_extension(m_k_view_incoming,
        major_index,
        minor_index,
        legacy_subaddress_extension_out);
}
//-------------------------------------------------------------------------------------------------------------------
carrot_hierarchy_address_device_ram_borrowed::carrot_hierarchy_address_device_ram_borrowed(
        const crypto::public_key &account_spend_pubkey,
        const crypto::public_key &account_view_pubkey,
        const crypto::public_key &main_address_view_pubkey,
        const crypto::secret_key &s_generate_address)
    :
        m_account_spend_pubkey(account_spend_pubkey),
        m_account_view_pubkey(account_view_pubkey),
        m_main_address_view_pubkey(main_address_view_pubkey),
        m_s_generate_address(s_generate_address)
{}
//-------------------------------------------------------------------------------------------------------------------
void carrot_hierarchy_address_device_ram_borrowed::make_index_extension_generator(const std::uint32_t major_index,
    const std::uint32_t minor_index,
    crypto::secret_key &address_generator_out) const
{
    make_carrot_index_extension_generator(m_s_generate_address, major_index, minor_index, address_generator_out);
}
//-------------------------------------------------------------------------------------------------------------------
crypto::public_key carrot_hierarchy_address_device_ram_borrowed::get_carrot_account_spend_pubkey() const
{
    return m_account_spend_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::public_key carrot_hierarchy_address_device_ram_borrowed::get_carrot_account_view_pubkey() const
{
    return m_account_view_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::public_key carrot_hierarchy_address_device_ram_borrowed::get_carrot_main_address_view_pubkey() const
{
    return m_main_address_view_pubkey;
}
//-------------------------------------------------------------------------------------------------------------------
hybrid_hierarchy_address_device_composed::hybrid_hierarchy_address_device_composed(
        const cryptonote_hierarchy_address_device *cn_addr_dev,
        const carrot_hierarchy_address_device *carrot_addr_dev)
    :
        m_cn_addr_dev(cn_addr_dev),
        m_carrot_addr_dev(carrot_addr_dev)
{}
//-------------------------------------------------------------------------------------------------------------------
bool hybrid_hierarchy_address_device_composed::supports_address_derivation_type(AddressDeriveType derive_type) const
{
    switch (derive_type)
    {
        case AddressDeriveType::PreCarrot:
            return m_cn_addr_dev;
        case AddressDeriveType::Carrot:
            return m_carrot_addr_dev;
        case AddressDeriveType::Auto:
            return m_cn_addr_dev || m_carrot_addr_dev;
        default:
            throw device_error("Default",
                "hybrid_hierarchy_address_device_composed",
                "supports_address_derivation_type",
                "unrecognized derive type", -1);
    }
}
//-------------------------------------------------------------------------------------------------------------------
const cryptonote_hierarchy_address_device &hybrid_hierarchy_address_device_composed::access_cryptonote_hierarchy_device() const
{
    if (!m_cn_addr_dev)
    {
        throw device_error("Default",
            "hybrid_hierarchy_address_device_composed",
            "access_cryptonote_hierarchy_device",
            "cryptonote sub-device unavailable", -1);
    }
    return *m_cn_addr_dev;
}
//-------------------------------------------------------------------------------------------------------------------
const carrot_hierarchy_address_device &hybrid_hierarchy_address_device_composed::access_carrot_hierarchy_device() const
{
    if (!m_carrot_addr_dev)
    {
        throw device_error("Default",
            "hybrid_hierarchy_address_device_composed",
            "access_carrot_hierarchy_device",
            "carrot sub-device unavailable", -1);
    }
    return *m_carrot_addr_dev;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
