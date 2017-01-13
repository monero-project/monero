// Copyright (c) 2014-2016, The Monero Project
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


#include "wallet/wallet2_api.h"
#include <string>

namespace Monero {

class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language, bool testnet);
    Wallet * openWallet(const std::string &path, const std::string &password, bool testnet);
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &memo, bool testnet, uint64_t restoreHeight);
    virtual bool closeWallet(Wallet *wallet);
    bool walletExists(const std::string &path);
    std::vector<std::string> findWallets(const std::string &path);
    std::string errorString() const;
    void setDaemonAddress(const std::string &address);
    bool connected(uint32_t *version = NULL) const;
    bool checkPayment(const std::string &address, const std::string &txid, const std::string &txkey, const std::string &daemon_address, uint64_t &received, uint64_t &height, std::string &error) const;
    uint64_t blockchainHeight() const;
    uint64_t blockchainTargetHeight() const;
    uint64_t networkDifficulty() const;
    double miningHashRate() const;
    void hardForkInfo(uint8_t &version, uint64_t &earliest_height) const;
    uint64_t blockTarget() const;
    bool isMining() const;
    bool startMining(const std::string &address, uint32_t threads = 1);
    bool stopMining();
    std::string resolveOpenAlias(const std::string &address, bool &dnssec_valid) const;

private:
    WalletManagerImpl() {}
    friend struct WalletManagerFactory;
    std::string m_daemonAddress;
    std::string m_errorString;
};

} // namespace

namespace Bitmonero = Monero;
