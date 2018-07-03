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
#include "net/http_client.h"
#include <string>

namespace Monero {

class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language, NetworkType nettype);
    Wallet * openWallet(const std::string &path, const std::string &password, NetworkType nettype);
    virtual Wallet * recoveryWallet(const std::string &path,
                                       const std::string &password,
                                       const std::string &mnemonic,
                                       NetworkType nettype,
                                       uint64_t restoreHeight);
    virtual Wallet * createWalletFromKeys(const std::string &path,
                                             const std::string &password,
                                             const std::string &language,
                                             NetworkType nettype,
                                             uint64_t restoreHeight,
                                             const std::string &addressString,
                                             const std::string &viewKeyString,
                                             const std::string &spendKeyString = "");
    // next two methods are deprecated - use the above version which allow setting of a password
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &mnemonic, NetworkType nettype, uint64_t restoreHeight);
    // deprecated: use createWalletFromKeys(..., password, ...) instead
    virtual Wallet * createWalletFromKeys(const std::string &path, 
                                                    const std::string &language,
                                                    NetworkType nettype, 
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "");
    virtual Wallet * createWalletFromDevice(const std::string &path,
                                            const std::string &password,
                                            NetworkType nettype,
                                            const std::string &deviceName,
                                            uint64_t restoreHeight = 0,
                                            const std::string &subaddressLookahead = "");
    virtual bool closeWallet(Wallet *wallet, bool store = true);
    bool walletExists(const std::string &path);
    bool verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool no_spend_key) const;
    std::vector<std::string> findWallets(const std::string &path);
    std::string errorString() const;
    void setDaemonAddress(const std::string &address);
    bool connected(uint32_t *version = NULL);
    uint64_t blockchainHeight();
    uint64_t blockchainTargetHeight();
    uint64_t networkDifficulty();
    double miningHashRate();
    uint64_t blockTarget();
    bool isMining();
    bool startMining(const std::string &address, uint32_t threads = 1, bool background_mining = false, bool ignore_battery = true);
    bool stopMining();
    std::string resolveOpenAlias(const std::string &address, bool &dnssec_valid) const;

private:
    WalletManagerImpl() {}
    friend struct WalletManagerFactory;
    std::string m_daemonAddress;
    epee::net_utils::http::http_simple_client m_http_client;
    std::string m_errorString;
};

} // namespace

namespace Bitmonero = Monero;
