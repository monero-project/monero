// Copyright (c) 2014-2017, The Monero Project
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

#include "wallet/wallet2_api.h"
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
struct Wallet2CallbackImpl;

class WalletImpl : public Wallet
{
public:
    WalletImpl(bool testnet = false);
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language);
    bool createWatchOnly(const std::string &path, const std::string &password,
                            const std::string &language) const;
    bool open(const std::string &path, const std::string &password);
    bool recover(const std::string &path, const std::string &seed);
    bool recoverFromKeys(const std::string &path,
                            const std::string &language,
                            const std::string &address_string, 
                            const std::string &viewkey_string,
                            const std::string &spendkey_string = "");
    bool close();
    std::string seed() const;
    std::string getSeedLanguage() const;
    void setSeedLanguage(const std::string &arg);
    // void setListener(Listener *) {}
    int status() const;
    std::string errorString() const;
    bool setPassword(const std::string &password);
    std::string address() const;
    std::string integratedAddress(const std::string &payment_id) const;
    std::string secretViewKey() const;
    std::string publicViewKey() const;
    std::string secretSpendKey() const;
    std::string publicSpendKey() const;
    std::string path() const;
    bool store(const std::string &path);
    std::string filename() const;
    std::string keysFilename() const;
    bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, const std::string &daemon_username = "", const std::string &daemon_password = "");
    bool connectToDaemon();
    ConnectionStatus connected() const;
    void setTrustedDaemon(bool arg);
    bool trustedDaemon() const;
    uint64_t balance() const;
    uint64_t unlockedBalance() const;
    uint64_t blockChainHeight() const;
    uint64_t approximateBlockChainHeight() const;
    uint64_t daemonBlockChainHeight() const;
    uint64_t daemonBlockChainTargetHeight() const;
    bool synchronized() const;
    bool refresh();
    void refreshAsync();
    void setAutoRefreshInterval(int millis);
    int autoRefreshInterval() const;
    void setRefreshFromBlockHeight(uint64_t refresh_from_block_height);
    uint64_t getRefreshFromBlockHeight() const { return m_wallet->get_refresh_from_block_height(); };
    void setRecoveringFromSeed(bool recoveringFromSeed);
    bool watchOnly() const;
    bool rescanSpent();
    bool testnet() const {return m_wallet->testnet();}
    void hardForkInfo(uint8_t &version, uint64_t &earliest_height) const;
    bool useForkRules(uint8_t version, int64_t early_blocks) const;

    PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                        optional<uint64_t> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low);
    virtual PendingTransaction * createSweepUnmixableTransaction();
    bool submitTransaction(const std::string &fileName);
    virtual UnsignedTransaction * loadUnsignedTx(const std::string &unsigned_filename);
    bool exportKeyImages(const std::string &filename);
    bool importKeyImages(const std::string &filename);

    virtual void disposeTransaction(PendingTransaction * t);
    virtual TransactionHistory * history() const;
    virtual AddressBook * addressBook() const;
    virtual void setListener(WalletListener * l);
    virtual uint32_t defaultMixin() const;
    virtual void setDefaultMixin(uint32_t arg);
    virtual bool setUserNote(const std::string &txid, const std::string &note);
    virtual std::string getUserNote(const std::string &txid) const;
    virtual std::string getTxKey(const std::string &txid) const;
    virtual std::string signMessage(const std::string &message);
    virtual bool verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const;
    virtual void startRefresh();
    virtual void pauseRefresh();
    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error);
    virtual std::string getDefaultDataDir() const;

private:
    void clearStatus() const;
    void refreshThreadFunc();
    void doRefresh();
    bool daemonSynced() const;
    void stopRefresh();
    bool isNewWallet() const;
    bool doInit(const std::string &daemon_address, uint64_t upper_transaction_size_limit);

private:
    friend class PendingTransactionImpl;
    friend class UnsignedTransactionImpl;    
    friend class TransactionHistoryImpl;
    friend struct Wallet2CallbackImpl;
    friend class AddressBookImpl;

    tools::wallet2 * m_wallet;
    mutable std::atomic<int>  m_status;
    mutable std::string m_errorString;
    std::string m_password;
    TransactionHistoryImpl * m_history;
    bool        m_trustedDaemon;
    WalletListener * m_walletListener;
    Wallet2CallbackImpl * m_wallet2Callback;
    AddressBookImpl *  m_addressBook;

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
    std::atomic<bool>   m_synchronized;
    std::atomic<bool>   m_rebuildWalletCache;
    // cache connection status to avoid unnecessary RPC calls
    mutable std::atomic<bool>   m_is_connected;
    boost::optional<epee::net_utils::http::login> m_daemon_login{};
};


} // namespace

namespace Bitmonero = Monero;

#endif

