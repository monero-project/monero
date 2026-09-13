// Copyright (c) 2014-2025, The Monero Project
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

#include "wallet/api/wallet2_api.h"


namespace Monero {

class EnoteDetailsImpl : public EnoteDetails
{
public:
    EnoteDetailsImpl();
    ~EnoteDetailsImpl() override;
    std::string onetimeAddress() const override;
    std::string viewTag() const override;
    std::string paymentId() const override;
    std::uint64_t blockHeight() const override;
    std::uint64_t unlockTime() const override;
    bool isUnlocked() const override;
    std::string txId() const override;
    std::uint64_t internalEnoteIndex() const override;
    std::uint64_t globalEnoteIndex() const override;
    bool isSpent() const override;
    bool isFrozen() const override;
    std::uint64_t spentHeight() const override;
    std::string keyImage() const override;
    std::string mask() const override;
    std::uint64_t amount() const override;
    TxProtocol protocolVersion() const override;
    bool isKeyImageKnown() const override;
    bool isKeyImageRequest() const override;
    std::uint64_t pkIndex() const override;
    std::vector<std::pair<std::uint64_t, std::string>> uses() const override;
    std::uint32_t subaddressIndexMajor() const override;
    std::uint32_t subaddressIndexMinor() const override;

    // Multisig
    bool isKeyImagePartial() const override;

private:
    friend class WalletImpl;

    std::string m_onetime_address;
    std::string m_view_tag;
    std::string m_payment_id;
    std::uint64_t m_block_height;
    std::uint64_t m_unlock_time;
    bool m_is_unlocked;
    std::string m_tx_id;
    std::uint64_t m_internal_enote_index;
    std::uint64_t m_global_enote_index;
    bool m_spent;
    bool m_frozen;
    std::uint64_t m_spent_height;
    std::string m_key_image;
    std::string m_mask;
    std::uint64_t m_amount;
    TxProtocol m_protocol_version;
    bool m_key_image_known;
    bool m_key_image_request;
    std::uint64_t m_pk_index;
    std::vector<std::pair<std::uint64_t, std::string>> m_uses;
    std::uint32_t m_subaddress_index_major;
    std::uint32_t m_subaddress_index_minor;

    // Multisig
    bool m_key_image_partial;
};

} // namespace Monero
