// Copyright (c) 2014-2024, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "wallet/api/wallet2_api.h"
#include "wallet/wallet2.h"

#include <string>
#include <vector>


namespace Monero {

class WalletImpl;
class PendingTransactionImpl : public PendingTransaction
{
public:
    PendingTransactionImpl(WalletImpl &wallet);
    ~PendingTransactionImpl();
    int status() const override;
    std::string errorString() const override;
    bool commit(const std::string &filename = "", bool overwrite = false) override;
    uint64_t amount() const override;
    uint64_t dust() const override;
    uint64_t fee() const override;
    std::vector<std::string> txid() const override;
    uint64_t txCount() const override;
    std::vector<uint32_t> subaddrAccount() const override;
    std::vector<std::set<uint32_t>> subaddrIndices() const override;
    // TODO: continue with interface;

    std::string multisigSignData() override;
    void signMultisigTx() override;
    std::vector<std::string> signersKeys() const override;

private:
    friend class WalletImpl;
    WalletImpl &m_wallet;

    int  m_status;
    std::string m_errorString;
    std::vector<tools::wallet2::pending_tx> m_pending_tx;
    std::unordered_set<crypto::public_key> m_signers;
    std::vector<std::string> m_tx_device_aux;
    std::vector<crypto::key_image> m_key_images;
};


}
