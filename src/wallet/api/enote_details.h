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

#include "wallet/api/wallet2_api.h"


namespace Monero {

class EnoteDetailsImpl : public EnoteDetails
{
public:
    EnoteDetailsImpl();
    ~EnoteDetailsImpl() override;
    std::string onetimeAddress() const override;
    std::string viewTag() const override;
    std::uint64_t blockHeight() const override;
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

    // Multisig
    bool isKeyImagePartial() const override;

private:
    friend class WalletImpl;

    // Ko
    std::string m_onetime_address;
    // view_tag
    std::string m_view_tag;
    // this enote was received at block height
    std::uint64_t m_block_height;
    // tx id in which tx enote was received
    std::string m_tx_id;
    // relative index in tx
    std::uint64_t m_internal_enote_index;
    // absolute index from `cryptonote::COMMAND_RPC_GET_TRANSACTIONS::entry.output_indices`
    std::uint64_t m_global_enote_index;
    // is spent
    bool m_spent;
    // is frozen
    bool m_frozen;
    // blockchain height, set if spent
    std::uint64_t m_spent_height;
    // key image
    std::string m_key_image;
    // x, blinding factor in amount commitment C = x G + a H
    std::string m_mask;
    // a
    std::uint64_t m_amount;
    // protocol version : TxProtocol_CryptoNote / TxProtocol_RingCT
    TxProtocol m_protocol_version;
    // is key image known
    bool m_key_image_known;
    // view wallets: we want to request it; cold wallets: it was requested
    bool m_key_image_request;
    // public key index in tx_extra
    std::uint64_t m_pk_index;
    // track uses of this enote in the blockchain in the format [ [block_height, tx_id], ... ] if `wallet2::m_track_uses` is true (default is false)
    std::vector<std::pair<std::uint64_t, std::string>> m_uses;

    // Multisig
    bool m_key_image_partial;
    // NOTE : These multisig members are part of wallet2 transfer_details and may need to get added here.
/*
    std::vector<rct::key> m_multisig_k;
    std::vector<multisig_info> m_multisig_info; // one per other participant
*/
};

} // namespace
