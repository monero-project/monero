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
#include "net/http.h"
#include <string>

namespace Monero {

class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language, NetworkType nettype, uint64_t kdf_rounds = 1, bool create_address_file = false, bool non_deterministic = false, bool unattended = true) override;
    Wallet * openWallet(const std::string &path, const std::string &password, NetworkType nettype, uint64_t kdf_rounds = 1, WalletListener * listener = nullptr, bool unattended = true) override;
    virtual Wallet * recoveryWallet(const std::string &path,
                                       const std::string &password,
                                       const std::string &mnemonic,
                                       NetworkType nettype,
                                       uint64_t restoreHeight,
                                       uint64_t kdf_rounds = 1,
                                       const std::string &seed_offset = {},
                                       const bool unattended = true) override;
    virtual Wallet * createWalletFromKeys(const std::string &path,
                                             const std::string &password,
                                             const std::string &language,
                                             NetworkType nettype,
                                             uint64_t restoreHeight,
                                             const std::string &addressString,
                                             const std::string &viewKeyString,
                                             const std::string &spendKeyString = "",
                                             uint64_t kdf_rounds = 1,
                                             const bool create_address_file = false,
                                             const bool unattended = true) override;
    Wallet * createWalletFromJson(const std::string &json_file_path,
                                  NetworkType nettype,
                                  std::string &pw_out,
                                  uint64_t kdf_rounds = 1,
                                  const bool unattended = true) override;
    Wallet * createWalletFromMultisigSeed(const std::string &path,
                                          const std::string &password,
                                          const std::string &language,
                                          NetworkType nettype,
                                          uint64_t restoreHeight,
                                          const std::string &multisig_seed,
                                          const std::string seed_pass = "",
                                          uint64_t kdf_rounds = 1,
                                          const bool create_address_file = false,
                                          const bool unattended = true) override;
    // next two methods are deprecated - use the above version which allow setting of a password
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &mnemonic, NetworkType nettype, uint64_t restoreHeight) override;
    // deprecated: use createWalletFromKeys(..., password, ...) instead
    virtual Wallet * createWalletFromKeys(const std::string &path, 
                                                    const std::string &language,
                                                    NetworkType nettype, 
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "") override;
    virtual Wallet * createWalletFromDevice(const std::string &path,
                                            const std::string &password,
                                            NetworkType nettype,
                                            const std::string &deviceName,
                                            uint64_t restoreHeight = 0,
                                            const std::string &subaddressLookahead = "",
                                            uint64_t kdf_rounds = 1,
                                            WalletListener * listener = nullptr,
                                            const std::string device_derivation_path = "",
                                            const bool create_address_file = false,
                                            const bool unattended = true) override;
    virtual bool closeWallet(Wallet *wallet, bool store = true, bool do_delete_pointer = true) override;
    bool walletExists(const std::string &path) override;
    bool verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool no_spend_key, uint64_t kdf_rounds = 1) const override;
    bool queryWalletDevice(Wallet::Device& device_type, const std::string &keys_file_name, const std::string &password, uint64_t kdf_rounds = 1) const override;
    std::vector<std::string> findWallets(const std::string &path) override;
    std::string errorString() const override;
    void setDaemonAddress(const std::string &address) override;
    bool connected(uint32_t *version = NULL) override;
    uint64_t blockchainHeight() override;
    uint64_t blockchainTargetHeight() override;
    uint64_t networkDifficulty() override;
    double miningHashRate() override;
    uint64_t blockTarget() override;
    bool wasBootstrapEverUsed() override;
    bool isBackgroundMiningEnabled() override;
    bool isMining() override;
    bool startMining(const std::string &address, uint32_t threads = 1, bool background_mining = false, bool ignore_battery = true) override;
    bool stopMining() override;
    bool saveBlockchain() override;
    bool getOutsBin(const std::vector<std::pair<std::uint64_t, std::uint64_t>> &output_amount_index, const bool do_get_txid, std::vector<std::string> &enote_public_key_out, std::vector<std::string> &rct_key_mask_out, std::vector<bool> &unlocked_out, std::vector<std::uint64_t> &height_out, std::vector<std::string> &tx_id_out) override;
    std::string resolveOpenAlias(const std::string &address, bool &dnssec_valid) const override;
    bool setProxy(const std::string &address) override;

private:
    WalletManagerImpl();
    friend struct WalletManagerFactory;
    net::http::client m_http_client;
    std::string m_errorString;
};

} // namespace
