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
#include "address_device.h"
#include "carrot_core/device_ram_borrowed.h"

//third party headers

//standard headers

//forward declarations

namespace carrot
{
struct cryptonote_hierarchy_address_device_ram_borrowed:
    public cryptonote_hierarchy_address_device,
    public view_incoming_key_ram_borrowed_device
{
    cryptonote_hierarchy_address_device_ram_borrowed(
        const crypto::public_key &cryptonote_account_spend_pubkey,
        const crypto::secret_key &k_view_incoming):
        view_incoming_key_ram_borrowed_device(k_view_incoming),
        m_cryptonote_account_spend_pubkey(cryptonote_account_spend_pubkey)
    {}

    crypto::public_key get_cryptonote_account_spend_pubkey() const override
    {
        return m_cryptonote_account_spend_pubkey;
    }

    void make_legacy_subaddress_extension(const std::uint32_t major_index,
        const std::uint32_t minor_index,
        crypto::secret_key &legacy_subaddress_extension_out) const override;

protected:
    const crypto::public_key &m_cryptonote_account_spend_pubkey;
};
} //namespace carrot
