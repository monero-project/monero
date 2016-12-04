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

#ifndef WALLET_IMPL_H
#define WALLET_IMPL_H

#include "wallet/wallet2_api.h"
#include "wallet/wallet2.h"

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>


namespace Bitmonero {
class TransactionHistoryImpl;
class PendingTransactionImpl;
struct Wallet2CallbackImpl;

class WalletImpl : public Wallet
{
public:
    WalletImpl(bool testnet = false);
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language);
    bool open(const std::string &path, const std::string &password);
    bool recover(const std::string &path, const std::string &seed);
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
    std::string path() const;
    bool store(const std::string &path);
    std::string filename() const;
    std::string keysFilename() const;
    bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit);
    void initAsync(const std::string &daemon_address, uint64_t upper_transaction_size_limit);
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
    void setRecoveringFromSeed(bool recoveringFromSeed);



    PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                        optional<uint64_t> amount, uint32_t mixin_count,
                                        PendingTransaction::Priority priority = PendingTransaction::Priority_Low);
    virtual PendingTransaction * createSweepUnmixableTransaction();

    virtual void disposeTransaction(PendingTransaction * t);
    virtual TransactionHistory * history() const;
    virtual void setListener(WalletListener * l);
    virtual uint32_t defaultMixin() const;
    virtual void setDefaultMixin(uint32_t arg);
    virtual bool setUserNote(const std::string &txid, const std::string &note);
    virtual std::string getUserNote(const std::string &txid) const;
    virtual std::string getTxKey(const std::string &txid) const;

    virtual std::string signMessage(const std::string &message);
    virtual bool verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const;

private:
    void clearStatus();
    void refreshThreadFunc();
    void doRefresh();
    void startRefresh();
    void stopRefresh();
    void pauseRefresh();
    bool isNewWallet() const;
    void doInit(const std::string &daemon_address, uint64_t upper_transaction_size_limit);

private:
    friend class PendingTransactionImpl;
    friend class TransactionHistoryImpl;
    friend class Wallet2CallbackImpl;

    tools::wallet2 * m_wallet;
    mutable std::atomic<int>  m_status;
    mutable std::string m_errorString;
    std::string m_password;
    TransactionHistoryImpl * m_history;
    bool        m_trustedDaemon;
    WalletListener * m_walletListener;
    Wallet2CallbackImpl * m_wallet2Callback;

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
    bool                m_recoveringFromSeed;
    std::atomic<bool>   m_synchronized;
    bool                m_rebuildWalletCache;
};


} // namespace

#endif

