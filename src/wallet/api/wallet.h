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

#ifndef WALLET_IMPL_H
#define WALLET_IMPL_H

#include "wallet/api/wallet2_api.h"
#include "wallet/wallet2.h"

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>

class WalletApiAccessorTest;

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
                            const std::string &seed, const std::string &seed_offset = {});
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
    std::string seed(const std::string& seed_offset = "") const override;
    std::string getSeedLanguage() const override;
    void setSeedLanguage(const std::string &arg) override;
    // void setListener(Listener *) {}
    int status() const override;
    std::string errorString() const override;
    void statusWithErrorString(int& status, std::string& errorString) const override;
    bool setPassword(const std::string &password) override;
    const std::string& getPassword() const override;
    bool setDevicePin(const std::string &password) override;
    bool setDevicePassphrase(const std::string &password) override;
    std::string address(uint32_t accountIndex = 0, uint32_t addressIndex = 0) const override;
    std::string integratedAddress(const std::string &payment_id) const override;
    std::string secretViewKey() const override;
    std::string publicViewKey() const override;
    std::string secretSpendKey() const override;
    std::string publicSpendKey() const override;
    std::string publicMultisigSignerKey() const override;
    std::string path() const override;
    void stop() override;
    bool store(const std::string &path) override;
    std::string filename() const override;
    std::string keysFilename() const override;
    bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, const std::string &daemon_username = "", const std::string &daemon_password = "", bool use_ssl = false, bool lightWallet = false, const std::string &proxy_address = "") override;
    bool connectToDaemon() override;
    ConnectionStatus connected() const override;
    void setTrustedDaemon(bool arg) override;
    bool trustedDaemon() const override;
    bool setProxy(const std::string &address) override;
    uint64_t balance(uint32_t accountIndex = 0) const override;
    uint64_t unlockedBalance(uint32_t accountIndex = 0) const override;
    uint64_t blockChainHeight() const override;
    uint64_t approximateBlockChainHeight() const override;
    uint64_t estimateBlockChainHeight() const override;
    uint64_t daemonBlockChainHeight() const override;
    uint64_t daemonBlockChainTargetHeight() const override;
    bool synchronized() const override;
    bool refresh() override;
    void refreshAsync() override;
    bool rescanBlockchain() override;
    void rescanBlockchainAsync() override;    
    void setAutoRefreshInterval(int millis) override;
    int autoRefreshInterval() const override;
    void setRefreshFromBlockHeight(uint64_t refresh_from_block_height) override;
    uint64_t getRefreshFromBlockHeight() const override { return m_wallet->get_refresh_from_block_height(); };
    void setRecoveringFromSeed(bool recoveringFromSeed) override;
    void setRecoveringFromDevice(bool recoveringFromDevice) override;
    void setSubaddressLookahead(uint32_t major, uint32_t minor) override;
    bool watchOnly() const override;
    bool isDeterministic() const override;
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
    std::string exchangeMultisigKeys(const std::vector<std::string> &info, const bool force_update_use_with_caution = false) override;
    std::string getMultisigKeyExchangeBooster(const std::vector<std::string> &info, const uint32_t threshold, const uint32_t num_signers) override;
    bool exportMultisigImages(std::string& images) override;
    size_t importMultisigImages(const std::vector<std::string>& images) override;
    bool hasMultisigPartialKeyImages() const override;
    PendingTransaction*  restoreMultisigTransaction(const std::string& signData) override;

    PendingTransaction * createTransactionMultDest(const std::vector<std::string> &dst_addr, const std::string &payment_id,
                                        optional<std::vector<uint64_t>> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low,
                                        uint32_t subaddr_account = 0,
                                        std::set<uint32_t> subaddr_indices = {}) override;
    PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                        optional<uint64_t> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low,
                                        uint32_t subaddr_account = 0,
                                        std::set<uint32_t> subaddr_indices = {}) override;
    virtual PendingTransaction * createSweepUnmixableTransaction() override;
    bool submitTransaction(const std::string &fileName) override;
    virtual UnsignedTransaction * loadUnsignedTx(const std::string &unsigned_filename) override;
    bool exportKeyImages(const std::string &filename, bool all = false) override;
    bool importKeyImages(const std::string &filename) override;
    bool exportOutputs(const std::string &filename, bool all = false) override;
    bool importOutputs(const std::string &filename) override;
    bool scanTransactions(const std::vector<std::string> &txids) override;

    bool setupBackgroundSync(const BackgroundSyncType background_sync_type, const std::string &wallet_password, const optional<std::string> &background_cache_password = optional<std::string>()) override;
    BackgroundSyncType getBackgroundSyncType() const override;
    bool startBackgroundSync() override;
    bool stopBackgroundSync(const std::string &wallet_password) override;
    bool isBackgroundSyncing() const override;
    bool isBackgroundWallet() const override;

    virtual void disposeTransaction(PendingTransaction * t) override;
    virtual uint64_t estimateTransactionFee(const std::vector<std::pair<std::string, uint64_t>> &destinations,
                                            PendingTransaction::Priority priority) const override;
    virtual TransactionHistory * history() override;
    virtual AddressBook * addressBook() override;
    virtual Subaddress * subaddress() override;
    virtual SubaddressAccount * subaddressAccount() override;
    virtual void setListener(WalletListener * l) override;
    virtual uint32_t defaultMixin() const override;
    virtual void setDefaultMixin(uint32_t arg) override;

    virtual bool setCacheAttribute(const std::string &key, const std::string &val) override;
    virtual std::string getCacheAttribute(const std::string &key) const override;

    virtual void setOffline(bool offline) override;
    virtual bool isOffline() const override;

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
    virtual std::string signMessage(const std::string &message, const std::string &address, bool sign_with_view_key = false) override;
    virtual bool verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const override;
    virtual std::string signMultisigParticipant(const std::string &message) const override;
    virtual bool verifyMessageWithPublicKey(const std::string &message, const std::string &publicKey, const std::string &signature) const override;
    virtual void startRefresh() override;
    virtual void pauseRefresh() override;
    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error) override;
    virtual std::string make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error) const override;
    virtual std::string getDefaultDataDir() const override;
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
    virtual uint64_t coldKeyImageSync(uint64_t &spent, uint64_t &unspent) override;
    virtual void deviceShowAddress(uint32_t accountIndex, uint32_t addressIndex, const std::string &paymentId) override;
    virtual bool reconnectDevice() override;
    virtual uint64_t getBytesReceived() override;
    virtual uint64_t getBytesSent() override;

    std::string getMultisigSeed(const std::string &seed_offset) const override;
    std::pair<std::uint32_t, std::uint32_t> getSubaddressIndex(const std::string &address) const override;
    void freeze(std::size_t idx) override;
    void freeze(const std::string &key_image) override;
    void thaw(std::size_t idx) override;
    void thaw(const std::string &key_image) override;
    bool isFrozen(std::size_t idx) const override;
    bool isFrozen(const std::string &key_image) const override;
    bool isFrozen(const PendingTransaction &multisig_ptxs) const override;
    bool isFrozen(const std::string multisig_sign_data) const override;
    void createOneOffSubaddress(std::uint32_t account_index, std::uint32_t address_index) override;
    virtual WalletState getWalletState() const = 0;
    bool hasUnknownKeyImages() const override;
    void rewrite(const std::string &wallet_name, const std::string &password) override;
    void writeWatchOnlyWallet(const std::string &wallet_name, const std::string &password, std::string &new_keys_file_name) override;
    std::map<std::uint32_t, std::uint64_t> balancePerSubaddress(std::uint32_t index_major, bool strict) const override;
    std::map<std::uint32_t, std::pair<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>>> unlockedBalancePerSubaddress(std::uint32_t index_major, bool strict) const override;
    bool isTransferUnlocked(std::uint64_t unlock_time, std::uint64_t block_height) const override;
    void updatePoolState(std::vector<std::tuple<cryptonote::transaction, std::string, bool>> &process_txs, bool refreshed = false, bool try_incremental = false) override;
    void processPoolState(const std::vector<std::tuple<cryptonote::transaction, std::string, bool>> &txs) override;
    std::string convertMultisigTxToStr(const PendingTransaction &multisig_ptx) const override;
    bool saveMultisigTx(const PendingTransaction &multisig_ptx, const std::string &filename) const override;
    std::string convertTxToStr(const PendingTransaction &ptxs) const override;
    bool parseUnsignedTxFromStr(const std::string &unsigned_tx_str, UnsignedTransaction &exported_txs) const override;
    bool parseMultisigTxFromStr(const std::string &multisig_tx_str, PendingTransaction &exported_txs) const override;
//    bool loadMultisigTxFromFile(const std::string &filename, PendingTransaction &exported_txs, std::function<bool(const PendingTransaction&)> accept_func) const override;
    std::uint64_t getFeeMultiplier(std::uint32_t priority, int fee_algorithm) const override;
    std::uint64_t getBaseFee() const override;
    std::uint64_t getMinRingSize() const override;
    std::uint64_t adjustMixin(std::uint64_t mixin) const override;
    std::uint32_t adjustPriority(std::uint32_t priority) const override;
    void coldTxAuxImport(const PendingTransaction &ptx, const std::vector<std::string> &tx_device_aux) const override;
//    void coldSignTx(const std::vector<pending_tx>& ptx_vector, signed_tx_set &exported_txs, std::vector<cryptonote::address_parse_info> &dsts_info, std::vector<std::string> & tx_device_aux) const override;
//    const wallet2::transfer_details &getTransferDetails(std::size_t idx) const override;
    void discardUnmixableOutputs() override;
    void setTxKey(const std::string &txid, const std::string &tx_key, const std::vector<std::string> &additional_tx_keys, const boost::optional<std::string> &single_destination_subaddress) override;
    std::string getDaemonAddress() const override;
    std::uint64_t getDaemonAdjustedTime() const override;
    void setCacheDescription(const std::string &description) override;
    std::string getCacheDescription() const override;
    const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& getAccountTags() override;
    void setAccountTag(const std::set<uint32_t> &account_indices, const std::string &tag) override;
    void setAccountTagDescription(const std::string &tag, const std::string &description) override;
    std::string exportOutputsToStr(bool all = false, std::uint32_t start = 0, std::uint32_t count = 0xffffffff) const override;
    std::size_t importOutputsFromStr(const std::string &outputs_str) override;
    std::uint64_t getBlockchainHeightByDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) const override;
    bool isSynced() const override;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> estimateBacklog(const std::vector<std::pair<double, double>> &fee_levels) const override;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> estimateBacklog(std::uint64_t min_tx_weight, std::uint64_t max_tx_weight, const std::vector<std::uint64_t> &fees) const override;
    bool saveToFile(const std::string &path_to_file, const std::string &binary, bool is_printable = false) const override;
    bool loadFromFile(const std::string &path_to_file, std::string &target_str, std::size_t max_size = 1000000000) const override;
    std::uint64_t hashTransfers(boost::optional<std::uint64_t> transfer_height, std::string &hash) const override;
    void finishRescanBcKeepKeyImages(std::uint64_t transfer_height, const std::string &hash) override;
    std::pair<std::size_t, std::uint64_t> estimateTxSizeAndWeight(bool use_rct, int n_inputs, int ring_size, int n_outputs, std::size_t extra_size) const override;
    std::uint64_t importKeyImages(const std::vector<std::pair<std::string, std::string>> &signed_key_images, size_t offset, std::uint64_t &spent, std::uint64_t &unspent, bool check_spent = true) override;
    bool importKeyImages(std::vector<std::string> key_images, size_t offset = 0, boost::optional<std::unordered_set<size_t>> selected_transfers = boost::none) override;

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
    void pendingTxPostProcess(PendingTransactionImpl * pending);
    bool doInit(const std::string &daemon_address, const std::string &proxy_address, uint64_t upper_transaction_size_limit = 0, bool ssl = false);
    bool checkBackgroundSync(const std::string &message) const;

    // QUESTION : Should I remove the private functions from this PR and work on them in another one?
    /**
    * brief: getTransferIndex - get the index of a stored transfer
    * param: key_image - key image of transfer
    * return: index in transfer storage
    */
    std::size_t getTransferIndex(const std::string &key_image) const;
    // TODO : consider changing the name and/or move to PendingTransaction
    /**
    * brief: makeMultisigTxSet - add multisig signers to pending transaction
    * param: ptx -
    */
    void makeMultisigTxSet(PendingTransaction &ptx) const;

private:
    friend class PendingTransactionImpl;
    friend class UnsignedTransactionImpl;    
    friend class TransactionHistoryImpl;
    friend struct Wallet2CallbackImpl;
    friend class AddressBookImpl;
    friend class SubaddressImpl;
    friend class SubaddressAccountImpl;
    friend class ::WalletApiAccessorTest;

    std::unique_ptr<tools::wallet2> m_wallet;
    mutable boost::mutex m_statusMutex;
    mutable int m_status;
    mutable std::string m_errorString;
    // TODO: harden password handling in the wallet API, see relevant discussion
    // https://github.com/monero-project/monero-gui/issues/1537
    // https://github.com/feather-wallet/feather/issues/72#issuecomment-1405602142
    // https://github.com/monero-project/monero/pull/8619#issuecomment-1632951461
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
    std::atomic<bool> m_refreshShouldRescan;
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

#endif
