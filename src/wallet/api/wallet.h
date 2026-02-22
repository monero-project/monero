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
class EnoteDetailsImpl;
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
    WalletImpl(NetworkType nettype = MAINNET, uint64_t kdf_rounds = 1, const bool unattended = true);
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language, bool create_address_file = false, bool non_deterministic = false);
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
    bool createFromJson(const std::string &json_file_path, std::string &pw_out);
    bool recoverFromMultisigSeed(const std::string &path,
                                 const std::string &password,
                                 const std::string &language,
                                 const std::string &multisig_seed,
                                 const std::string seed_pass = "",
                                 const bool do_create_address_file = false);
    bool recoverFromDevice(const std::string &path,
                           const std::string &password,
                           const std::string &device_name);
    Device getDeviceType() const override;
    bool close(bool store = true);
    std::string seed(const std::string& seed_offset = "") const override;
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
    bool connectToDaemon(uint32_t *version = NULL, bool *ssl = NULL, uint32_t timeout = 20000, bool *wallet_is_outdated = NULL, bool *daemon_is_outdated = NULL) override;
    ConnectionStatus connected() const override;
    void setTrustedDaemon(bool arg) override;
    bool trustedDaemon() const override;
    bool setProxy(const std::string &address) override;
    uint64_t balance(uint32_t accountIndex = 0) const override;
    std::map<uint32_t, uint64_t> balancePerSubaddress(uint32_t accountIndex = 0) const override;
    uint64_t unlockedBalance(uint32_t accountIndex = 0, std::uint64_t *blocks_to_unlock = NULL, std::uint64_t *time_to_unlock = NULL) const override;
    std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> unlockedBalancePerSubaddress(uint32_t accountIndex = 0) const override;
    uint64_t blockChainHeight() const override;
    uint64_t approximateBlockChainHeight() const override;
    uint64_t estimateBlockChainHeight() const override;
    uint64_t daemonBlockChainHeight() const override;
    uint64_t daemonBlockChainTargetHeight() const override;
    bool daemonSynced() const override;
    bool synchronized() const override;
    bool refresh(std::uint64_t start_height = 0, bool check_pool = true, bool try_incremental = false, std::uint64_t max_blocks = std::numeric_limits<uint64_t>::max(), std::uint64_t *blocks_fetched_out = nullptr, bool *received_money_out = nullptr) override;
    void refreshAsync() override;
    bool rescanBlockchain(bool do_hard_rescan = true, bool do_keep_key_images = false, bool do_skip_refresh = false) override;
    void rescanBlockchainAsync(bool do_hard_rescan = true, bool do_keep_key_images = false) override;
    void setAutoRefreshInterval(int millis) override;
    int autoRefreshInterval() const override;
    void setRecoveringFromSeed(bool recoveringFromSeed) override;
    void setRecoveringFromDevice(bool recoveringFromDevice) override;
    bool watchOnly() const override;
    bool isDeterministic() const override;
    bool rescanSpent() override;
    NetworkType nettype() const override;
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
    PendingTransaction* restoreMultisigTransaction(const std::string& signData, bool ask_for_confirmation = false) override;

    PendingTransaction * createTransactionMultDest(const std::vector<std::string> &dst_addr, const std::string &payment_id,
                                        optional<std::vector<uint64_t>> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low,
                                        uint32_t subaddr_account = 0,
                                        std::set<uint32_t> subaddr_indices = {},
                                        std::set<uint32_t> subtract_fee_from_outputs = {},
                                        const std::string key_image = "",
                                        const size_t outputs = 1,
                                        const std::uint64_t below = 0) override;
    PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                        optional<uint64_t> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low,
                                        uint32_t subaddr_account = 0,
                                        std::set<uint32_t> subaddr_indices = {}) override;
    virtual PendingTransaction * createSweepUnmixableTransaction() override;
    bool submitTransaction(const std::string &fileName) override;
    virtual UnsignedTransaction * loadUnsignedTx(const std::string &unsigned_filename) override;
    UnsignedTransaction * loadUnsignedTxFromStr(const std::string &unsigned_tx_str) override;
    bool exportKeyImages(const std::string &filename, bool all = false) override;
    std::string exportKeyImagesAsString(bool all = false) override;
    bool importKeyImages(const std::string &filename, std::uint64_t *spent_out = nullptr, std::uint64_t *unspent_out = nullptr, std::uint64_t *import_height = nullptr) override;
    bool importKeyImagesFromStr(const std::string &data) override;
    bool exportOutputs(const std::string &filename, bool all = false) override;
    bool importOutputs(const std::string &filename) override;
    bool scanTransactions(const std::vector<std::string> &txids, bool *wont_reprocess_recent_txs_via_untrusted_daemon = nullptr) override;

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
    virtual bool verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature, bool *is_old_out = nullptr, std::string *signature_type_out = nullptr) const override;
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
    std::unique_ptr<WalletKeysDecryptGuard> createKeysDecryptGuard(const std::string_view &password) override;
    virtual uint64_t coldKeyImageSync(uint64_t &spent, uint64_t &unspent) override;
    virtual void deviceShowAddress(uint32_t accountIndex, uint32_t addressIndex, const std::string &paymentId) override;
    virtual bool reconnectDevice() override;
    virtual uint64_t getBytesReceived() override;
    virtual uint64_t getBytesSent() override;

    std::string getMultisigSeed(const std::string &seed_offset) const override;
    std::pair<std::uint32_t, std::uint32_t> getSubaddressIndex(const std::string &address) const override;
    void freeze(const std::string &key_image) override;
    void freezeByPubKey(const std::string &enote_pub_key) override;
    void thaw(const std::string &key_image) override;
    void thawByPubKey(const std::string &enote_pub_key) override;
    bool isFrozen(const std::string &key_image) const override;
    void createOneOffSubaddress(std::uint32_t account_index, std::uint32_t address_index) override;
    WalletState getWalletState() const override;
    DeviceState getDeviceState() const override;
    void rewriteWalletFile(const std::string &wallet_name, const std::string_view &password) override;
    void writeWatchOnlyWallet(const std::string_view &password, std::string &new_keys_file_name) override;
    void refreshPoolOnly(bool refreshed = false, bool try_incremental = false) override;
    std::vector<std::unique_ptr<EnoteDetails>> getEnoteDetails() const override;
    std::unique_ptr<EnoteDetails> getEnoteDetails(const std::string &enote_pub_key) const override;
    std::unique_ptr<EnoteDetails> getEnoteDetails(const std::size_t enote_index) const override;
    std::string convertMultisigTxToStr(const PendingTransaction &multisig_ptx) const override;
    bool saveMultisigTx(const PendingTransaction &multisig_ptx, const std::string &filename) const override;
    PendingTransaction* parseTxFromStr(const std::string &signed_tx_str) override;
    PendingTransaction* parseMultisigTxFromStr(const std::string &multisig_tx_str) override;
    std::uint64_t getFeeMultiplier(std::uint32_t priority, int fee_algorithm) const override;
    std::uint64_t getBaseFee() const override;
    std::uint32_t adjustPriority(std::uint32_t priority) override;
    void coldTxAuxImport(const PendingTransaction &ptx, const std::vector<std::string> &tx_device_aux) const override;
    void coldSignTx(const PendingTransaction &ptx_in, PendingTransaction &exported_txs_out) const override;
    void discardUnmixableEnotes() override;
    void setTxKey(const std::string &txid, const std::string &tx_key, const std::vector<std::string> &additional_tx_keys, const std::string &single_destination_subaddress) override;
    const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& getAccountTags() const override;
    void setAccountTag(const std::set<uint32_t> &account_indices, const std::string &tag) override;
    void setAccountTagDescription(const std::string &tag, const std::string &description) override;
    std::string exportEnotesToStr(bool all = false, std::uint32_t start = 0, std::uint32_t count = 0xffffffff) const override;
    std::size_t importEnotesFromStr(const std::string &enotes_str) override;
    std::uint64_t getBlockchainHeightByDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) const override;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> estimateBacklog(const std::vector<std::pair<double, double>> &fee_levels) const override;
    std::uint64_t hashEnotes(std::uint64_t enote_idx, std::string &hash) const override;
    void finishRescanBcKeepKeyImages(std::uint64_t enote_idx, const std::string &hash) override;
    std::vector<std::tuple<std::string, std::uint16_t, std::uint64_t>> getPublicNodes(bool white_only = true) const override;
    std::pair<std::size_t, std::uint64_t> estimateTxSizeAndWeight(bool use_rct, int n_inputs, int ring_size, int n_outputs, std::size_t extra_size) const override;
    std::uint64_t importKeyImages(const std::vector<std::pair<std::string, std::string>> &signed_key_images, std::size_t offset, std::uint64_t &spent, std::uint64_t &unspent, bool check_spent = true) override;
    bool importKeyImages(const std::vector<std::string> &key_images, std::size_t offset = 0, const std::unordered_set<std::size_t> &selected_enotes_indices = {}) override;
    bool getAllowMismatchedDaemonVersion() const override;
    void setAllowMismatchedDaemonVersion(bool allow_mismatch) override;
    std::string getDeviceDerivationPath() const override;
    void setDeviceDerivationPath(std::string device_derivation_path) override;
    bool setDaemon(const std::string &daemon_address, const std::string &daemon_username = "", const std::string &daemon_password = "", bool trusted_daemon = false, Wallet::SSLSupport ssl_support = Wallet::SSLSupport::SSLSupport_Autodetect, const std::string &ssl_private_key_path = "", const std::string &ssl_certificate_path = "", const std::string &ssl_ca_file_path = "", const std::vector<std::string> &ssl_allowed_fingerprints_str = {}, bool ssl_allow_any_cert = false, const std::string &proxy = "") override;
    bool verifyPassword(const std::string_view &password) override;
    void encryptKeys(const std::string_view &password) override;
    void decryptKeys(const std::string_view &password) override;
    std::uint64_t getMinRingSize() const override;
    std::uint64_t getMaxRingSize() const override;
    std::uint64_t adjustMixin(const std::uint64_t fake_outs_count) const override;

    std::uint64_t getDaemonAdjustedTime() const override;
    std::uint64_t getLastBlockReward() const override;
    bool hasUnknownKeyImages() const override;

    bool getExplicitRefreshFromBlockHeight() const override;
    void setExplicitRefreshFromBlockHeight(bool do_explicit_refresh) override;

    // Wallet Settings getter/setter
    std::string getSeedLanguage() const override;
    void setSeedLanguage(const std::string &arg) override;
    bool getAlwaysConfirmTransfers() const override;
    void setAlwaysConfirmTransfers(bool do_always_confirm) override;
    bool getPrintRingMembers() const override;
    void setPrintRingMembers(bool do_print_ring_members) override;
    bool getStoreTxInfo() const override;
    void setStoreTxInfo(bool do_store_tx_info) override;
    bool getAutoRefresh() const override;
    void setAutoRefresh(bool do_auto_refresh) override;
    RefreshType getRefreshType() const override;
    void setRefreshType(RefreshType refresh_type) override;
    std::uint32_t getDefaultPriority() const override;
    void setDefaultPriority(std::uint32_t default_priority) override;
    AskPasswordType getAskPasswordType() const override;
    void setAskPasswordType(AskPasswordType ask_password_type) override;
    std::uint64_t getMaxReorgDepth() const override;
    void setMaxReorgDepth(std::uint64_t max_reorg_depth) override;
    std::uint32_t getMinOutputCount() const override;
    void setMinOutputCount(std::uint32_t min_output_count) override;
    std::uint64_t getMinOutputValue() const override;
    void setMinOutputValue(std::uint64_t min_output_value) override;
    bool getMergeDestinations() const override;
    void setMergeDestinations(bool do_merge_destinations) override;
    std::uint32_t getConfirmBacklogThreshold() const override;
    bool getConfirmBacklog() const override;
    void setConfirmBacklog(bool do_confirm_backlog) override;
    void setConfirmBacklogThreshold(std::uint32_t confirm_backlog_threshold) override;
    bool getConfirmExportOverwrite() const override;
    void setConfirmExportOverwrite(bool do_confirm_export_overwrite) override;
    uint64_t getRefreshFromBlockHeight() const override;
    void setRefreshFromBlockHeight(uint64_t refresh_from_block_height) override;
    bool getAutoLowPriority() const override;
    void setAutoLowPriority(bool use_auto_low_priority) override;
    bool getSegregatePreForkOutputs() const override;
    void setSegregatePreForkOutputs(bool do_segregate) override;
    bool getKeyReuseMitigation2() const override;
    void setKeyReuseMitigation2(bool mitigation) override;
    std::pair<std::uint32_t, std::uint32_t> getSubaddressLookahead() const override;
    void setSubaddressLookahead(uint32_t major, uint32_t minor) override;
    std::uint64_t getSegregationHeight() const override;
    void setSegregationHeight(std::uint64_t height) override;
    bool getIgnoreFractionalOutputs() const override;
    void setIgnoreFractionalOutputs(bool do_ignore_fractional_outputs) override;
    std::uint64_t getIgnoreOutputsAbove() const override;
    void setIgnoreOutputsAbove(std::uint64_t ignore_outputs_above) override;
    std::uint64_t getIgnoreOutputsBelow() const override;
    void setIgnoreOutputsBelow(std::uint64_t ignore_outputs_below) override;
    bool getTrackUses() const override;
    void setTrackUses(bool do_track_uses) override;
    BackgroundMiningSetupType getSetupBackgroundMining() const override;
    void setSetupBackgroundMining(BackgroundMiningSetupType background_mining_setup) override;
    std::string getDeviceName() const override;
    void setDeviceName(const std::string &device_name) override;
    ExportFormat getExportFormat() const override;
    void setExportFormat(ExportFormat export_format) override;
    bool getShowWalletNameWhenLocked() const override;
    void setShowWalletNameWhenLocked(bool do_show_wallet_name) override;
    std::uint32_t getInactivityLockTimeout() const override;
    void setInactivityLockTimeout(std::uint32_t seconds) override;
    bool getEnableMultisig() const override;
    void setEnableMultisig(bool do_enable_multisig) override;

private:
    void clearStatus() const;
    void setStatusError(const std::string& message) const;
    void setStatusCritical(const std::string& message) const;
    void setStatus(int status, const std::string& message) const;
    void refreshThreadFunc();
    void doRefresh(std::uint64_t start_height = 0, bool check_pool = true, bool try_incremental = false, std::uint64_t max_blocks = std::numeric_limits<uint64_t>::max(), bool *error_out = nullptr, std::uint64_t *blocks_fetched_out = nullptr, bool *received_money_out = nullptr);
    void stopRefresh();
    bool isNewWallet() const;
    void pendingTxPostProcess(PendingTransactionImpl * pending);
    bool doInit(const std::string &daemon_address, const std::string &proxy_address, uint64_t upper_transaction_size_limit = 0, bool ssl = false);
    bool checkBackgroundSync(const std::string &message) const;

    /**
    * brief: getEnoteIndex - get the index of an enote in local enote storage
    * param: key_image - key image to identify the enote
    * return: enote index
    */
    std::size_t getEnoteIndex(const std::string &key_image) const;
    /**
    * brief: getPaymentIdFromExtra -
    * param: tx_extra - as raw bytes
    * return: payment_id if succeeded, else empty string
    *         (size is either A) 16 for 8 byte short encrypted payment IDs,
    *                         B) 64 for obsolete 32 byte long unencrypted payment IDs)
    */
    std::string getPaymentIdFromExtra(const std::vector<std::uint8_t> &tx_extra) const;
    /**
    * brief: statusOk -
    * return: true if status is ok, else false
    */
    bool statusOk() const;

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
    std::atomic<bool> m_do_hard_rescan;
    std::atomic<bool> m_do_keep_key_images_on_rescan;
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
    // number of rounds for key derivation function for wallet password
    std::uint64_t m_kdf_rounds;
};


} // namespace

#endif
