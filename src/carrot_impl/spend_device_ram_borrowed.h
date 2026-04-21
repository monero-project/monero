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
#include "crypto/crypto.h"
#include "spend_device.h"

//third party headers

//standard headers

//forward declarations

namespace carrot
{
class spend_device_ram_borrowed: public spend_device
{
public:
    /// @brief device composed (except k_s, k_ps, k_gi)
    spend_device_ram_borrowed(std::shared_ptr<view_incoming_key_device> k_view_incoming_dev,
        std::shared_ptr<view_balance_secret_device> s_view_balance_dev,
        std::shared_ptr<address_device> address_dev,
        const crypto::secret_key &privkey_g,
        const crypto::secret_key &privkey_t);

    /// @brief cryptonote-derived & ram borrowed from k_s, k_v
    spend_device_ram_borrowed(const crypto::secret_key &k_spend, const crypto::secret_key &k_view);

    bool try_sign_carrot_transaction_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
        const std::unordered_map<crypto::public_key, FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
        crypto::hash &signable_tx_hash_out,
        signed_input_set_t &signed_inputs_out
    ) const override;

    crypto::key_image derive_key_image(const OutputOpeningHintVariant &opening_hint) const override;

    crypto::key_image derive_key_image_prescanned(const crypto::secret_key &sender_extension_g,
        const crypto::public_key &onetime_address,
        const subaddress_index_extended &subaddr_index,
        const bool use_biased) const override;

protected:
    std::shared_ptr<view_incoming_key_device> m_k_view_incoming_dev;
    std::shared_ptr<view_balance_secret_device> m_s_view_balance_dev;
    std::shared_ptr<address_device> m_address_dev;
    const crypto::secret_key &m_privkey_g;
    const crypto::secret_key &m_privkey_t;
};
} //namespace carrot
