// Copyright (c) 2014-2018, The Monero Project
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
class UnsignedTransactionImpl : public UnsignedTransaction
{
public:
    UnsignedTransactionImpl(WalletImpl &wallet);
    ~UnsignedTransactionImpl();
    int status() const override;
    std::string errorString() const override;
    std::vector<uint64_t> amount() const override;
    std::vector<uint64_t> fee() const override;
    std::vector<uint64_t> mixin() const override;
    std::vector<std::string> paymentId() const override;
    std::vector<std::string> recipientAddress() const override;
    uint64_t txCount() const override;
    // sign txs and save to file
    bool sign(const std::string &signedFileName) override;
    std::string confirmationMessage() const override {return m_confirmationMessage;}
    uint64_t minMixinCount() const override;

private:
    // Callback function to check all loaded tx's and generate confirmationMessage
    bool checkLoadedTx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message);
    
    friend class WalletImpl;
    WalletImpl &m_wallet;

    int  m_status;
    std::string m_errorString;
    tools::wallet2::unsigned_tx_set m_unsigned_tx_set;
    std::string m_confirmationMessage;
};


}

namespace Bitmonero = Monero;
