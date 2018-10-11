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

#ifndef WALLET_IMPL_H
#define WALLET_IMPL_H

#include "wallet/api/wallet2_api.h"
#include "wallet/wallet2.h"

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>


namespace Monero {
class TransactionHistoryImpl;
class PendingTransactionImpl;
class UnsignedTransactionImpl;
class AddressBookImpl;
class SubaddressImpl;
class SubaddressAccountImpl;
struct Wallet2CallbackImpl;

class WalletImpl : public Wallet
{
public:
    WalletImpl(NetworkType nettype = MAINNET, uint64_t kdf_rounds = 1);
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language);
    bool createWatchOnly(const std::string &path, const std::string &password,
                            const std::string &language) const override;
    bool open(const std::string &path, const std::string &password);
    bool recover(const std::string &path,const std::string &password,
                            const std::string &seed);
    bool recoverFromKeysWithPassword(const std::string &path,
                            const std::string &password,
                            const std::string &language,
                            const std::string &address_string,
                            const std::string &viewkey_string,
                            const std::string &spendkey_string = "");
    // following two methods are deprecated since they create passwordless wallets
    // use the two equivalent methods above
    bool recover(const std::string &path, const std::string &seed);
    // deprecated: use recoverFromKeysWithPassword() instead
    bool recoverFromKeys(const std::string &path,
                            const std::string &language,
                            const std::string &address_string, 
                            const std::string &viewkey_string,
                            const std::string &spendkey_string = "");
    bool recoverFromDevice(const std::string &path,
                           const std::string &password,
                           const std::string &device_name);
    Device getDeviceType() const override;
    bool close(bool store = true);
    std::string seed() const override;
    std::string getSeedLanguage() const override;
    void setSeedLanguage(const std::string &arg) override;
    // void setListener(Listener *) {}
    int status() const override;
    std::string errorString() const override;
    void statusWithErrorString(int& status, std::string& errorString) const override;
    bool setPassword(const std::string &password) override;
    std::string address(uint32_t accountIndex = 0, uint32_t addressIndex = 0) const override;
    std::string integratedAddress(const std::string &payment_id) const override;
    std::string secretViewKey() const override;
    std::string publicViewKey() const override;
    std::string secretSpendKey() const override;
    std::string publicSpendKey() const override;
    std::string publicMultisigSignerKey() const override;
    std::string path() const override;
    bool store(const std::string &path) override;
    std::string filename() const override;
    std::string keysFilename() const override;
    bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, const std::string &daemon_username = "", const std::string &daemon_password = "", bool use_ssl = false, bool lightWallet = false) override;
    bool connectToDaemon() override;
    ConnectionStatus connected() const override;
    void setTrustedDaemon(bool arg) override;
    bool trustedDaemon() const override;
    uint64_t balance(uint32_t accountIndex = 0) const override;
    uint64_t unlockedBalance(uint32_t accountIndex = 0) const override;
    uint64_t blockChainHeight() const override;
    uint64_t approximateBlockChainHeight() const override;
    uint64_t daemonBlockChainHeight() const override;
    uint64_t daemonBlockChainTargetHeight() const override;
    bool synchronized() const override;
    bool refresh() override;
    void refreshAsync() override;
    void setAutoRefreshInterval(int millis) override;
    int autoRefreshInterval() const override;
    void setRefreshFromBlockHeight(uint64_t refresh_from_block_height) override;
    uint64_t getRefreshFromBlockHeight() const override { return m_wallet->get_refresh_from_block_height(); };
    void setRecoveringFromSeed(bool recoveringFromSeed) override;
    void setRecoveringFromDevice(bool recoveringFromDevice) override;
    void setSubaddressLookahead(uint32_t major, uint32_t minor) override;
    bool watchOnly() const override;
    bool rescanSpent() override;
    NetworkType nettype() const override {return static_cast<NetworkType>(m_wallet->nettype());}
    void hardForkInfo(uint8_t &version, uint64_t &earliest_height) const override;
    bool useForkRules(uint8_t version, int64_t early_blocks) const override;

    void addSubaddressAccount(const std::string& label) override;
    size_t numSubaddressAccounts() const override;
    size_t numSubaddresses(uint32_t accountIndex) const override;
    void addSubaddress(uint32_t accountIndex, const std::string& label) override;
    std::string getSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex) const override;
    void setSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label) override;

    MultisigState multisig() const override;
    std::string getMultisigInfo() const override;
    std::string makeMultisig(const std::vector<std::string>& info, uint32_t threshold) override;
    std::string exchangeMultisigKeys(const std::vector<std::string> &info) override;
    bool finalizeMultisig(const std::vector<std::string>& extraMultisigInfo) override;
    bool exportMultisigImages(std::string& images) override;
    size_t importMultisigImages(const std::vector<std::string>& images) override;
    bool hasMultisigPartialKeyImages() const override;
    PendingTransaction*  restoreMultisigTransaction(const std::string& signData) override;

    PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                        optional<uint64_t> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low,
                                        uint32_t subaddr_account = 0,
                                        std::set<uint32_t> subaddr_indices = {}) override;
    virtual PendingTransaction * createSweepUnmixableTransaction() override;
    bool submitTransaction(const std::string &fileName) override;
    virtual UnsignedTransaction * loadUnsignedTx(const std::string &unsigned_filename) override;
    bool exportKeyImages(const std::string &filename) override;
    bool importKeyImages(const std::string &filename) override;

    virtual void disposeTransaction(PendingTransaction * t) override;
    virtual TransactionHistory * history() override;
    virtual AddressBook * addressBook() override;
    virtual Subaddress * subaddress() override;
    virtual SubaddressAccount * subaddressAccount() override;
    virtual void setListener(WalletListener * l) override;
    virtual uint32_t defaultMixin() const override;
    virtual void setDefaultMixin(uint32_t arg) override;
    virtual bool setUserNote(const std::string &txid, const std::string &note) override;
    virtual std::string getUserNote(const std::string &txid) const override;
    virtual std::string getTxKey(const std::string &txid) const override;
    virtual bool checkTxKey(const std::string &txid, std::string tx_key, const std::string &address, uint64_t &received, bool &in_pool, uint64_t &confirmations) override;
    virtual std::string getTxProof(const std::string &txid, const std::string &address, const std::string &message) const override;
    virtual bool checkTxProof(const std::string &txid, const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &received, bool &in_pool, uint64_t &confirmations) override;
    virtual std::string getSpendProof(const std::string &txid, const std::string &message) const override;
    virtual bool checkSpendProof(const std::string &txid, const std::string &message, const std::string &signature, bool &good) const override;
    virtual std::string getReserveProof(bool all, uint32_t account_index, uint64_t amount, const std::string &message) const override;
    virtual bool checkReserveProof(const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &total, uint64_t &spent) const override;
    virtual std::string signMessage(const std::string &message) override;
    virtual bool verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const override;
    virtual std::string signMultisigParticipant(const std::string &message) const override;
    virtual bool verifyMessageWithPublicKey(const std::string &message, const std::string &publicKey, const std::string &signature) const override;
    virtual void startRefresh() override;
    virtual void pauseRefresh() override;
    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error) override;
    virtual std::string getDefaultDataDir() const override;
    virtual bool lightWalletLogin(bool &isNewWallet) const override;
    virtual bool lightWalletImportWalletRequest(std::string &payment_id, uint64_t &fee, bool &new_request, bool &request_fulfilled, std::string &payment_address, std::string &status) override;
    virtual bool blackballOutputs(const std::vector<std::string> &outputs, bool add) override;
    virtual bool blackballOutput(const std::string &amount, const std::string &offset) override;
    virtual bool unblackballOutput(const std::string &amount, const std::string &offset) override;
    virtual bool getRing(const std::string &key_image, std::vector<uint64_t> &ring) const override;
    virtual bool getRings(const std::string &txid, std::vector<std::pair<std::string, std::vector<uint64_t>>> &rings) const override;
    virtual bool setRing(const std::string &key_image, const std::vector<uint64_t> &ring, bool relative) override;
    virtual void segregatePreForkOutputs(bool segregate) override;
    virtual void segregationHeight(uint64_t height) override;
    virtual void keyReuseMitigation2(bool mitigation) override;
    virtual bool lockKeysFile() override;
    virtual bool unlockKeysFile() override;
    virtual bool isKeysFileLocked() override;

private:
    void clearStatus() const;
    void setStatusError(const std::string& message) const;
    void setStatusCritical(const std::string& message) const;
    void setStatus(int status, const std::string& message) const;
    void refreshThreadFunc();
    void doRefresh();
    bool daemonSynced() const;
    void stopRefresh();
    bool isNewWallet() const;
    bool doInit(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, bool ssl = false);

private:
    friend class PendingTransactionImpl;
    friend class UnsignedTransactionImpl;    
    friend class TransactionHistoryImpl;
    friend struct Wallet2CallbackImpl;
    friend class AddressBookImpl;
    friend class SubaddressImpl;
    friend class SubaddressAccountImpl;

    std::unique_ptr<tools::wallet2> m_wallet;
    mutable boost::mutex m_statusMutex;
    mutable int m_status;
    mutable std::string m_errorString;
    std::string m_password;
    std::unique_ptr<TransactionHistoryImpl> m_history;
    std::unique_ptr<Wallet2CallbackImpl> m_wallet2Callback;
    std::unique_ptr<AddressBookImpl>  m_addressBook;
    std::unique_ptr<SubaddressImpl>  m_subaddress;
    std::unique_ptr<SubaddressAccountImpl>  m_subaddressAccount;

    // multi-threaded refresh stuff
    std::atomic<bool> m_refreshEnabled;
    std::atomic<bool> m_refreshThreadDone;
    std::atomic<int>  m_refreshIntervalMillis;
    // synchronizing  refresh loop;
    boost::mutex        m_refreshMutex;

    // synchronizing  sync and async refresh
    boost::mutex        m_refreshMutex2;
    boost::condition_variable m_refreshCV;
    boost::thread       m_refreshThread;
    // flag indicating wallet is recovering from seed
    // so it shouldn't be considered as new and pull blocks (slow-refresh)
    // instead of pulling hashes (fast-refresh)
    std::atomic<bool>   m_recoveringFromSeed;
    std::atomic<bool>   m_recoveringFromDevice;
    std::atomic<bool>   m_synchronized;
    std::atomic<bool>   m_rebuildWalletCache;
    // cache connection status to avoid unnecessary RPC calls
    mutable std::atomic<bool>   m_is_connected;
    boost::optional<epee::net_utils::http::login> m_daemon_login{};
};


} // namespace

namespace Bitmonero = Monero;

#endif

