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

#pragma once


#include <string>
#include <vector>

//  Public interface for libwallet library
namespace Bitmonero {

/**
 * @brief Transaction-like interface for sending money
 */
struct PendingTransaction
{
    enum Status {
        Status_Ok,
        Status_Error
    };
    virtual ~PendingTransaction() = 0;
    virtual int status() const = 0;
    virtual std::string errorString() const = 0;
    virtual bool commit() = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t dust() const = 0;
    virtual uint64_t fee() const = 0;
};

struct TransactionInfo;
struct TransactionHistory
{
    virtual int count() const;
    virtual TransactionInfo * transaction(int index)  const = 0;
    virtual TransactionInfo * transaction(const std::string &id) const = 0;
    virtual std::vector<TransactionInfo*> getAll() const = 0;
};


/**
 * @brief The TransactionInfo - interface for displaying transaction information
 */
struct TransactionInfo
{
    enum Direction {
        Direction_In,
        Direction_Out
    };

    virtual bool isHold() const = 0;
    virtual bool isFailed() const = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t fee() const = 0;
    virtual std::string address() const = 0;
    virtual int direction() const = 0;
    // TODO
};

/**
 * @brief Interface for wallet operations.
 *        TODO: check if /include/IWallet.h is still actual
 */
struct Wallet
{

    enum Status {
        Status_Ok,
        Status_Error
    };

    virtual ~Wallet() = 0;
    virtual std::string seed() const = 0;
    virtual std::string getSeedLanguage() const = 0;
    virtual void setSeedLanguage(const std::string &arg) = 0;
    // virtual void setListener(Listener * listener) = 0;
    //! returns wallet status (Status_Ok | Status_Error)
    virtual int status() const = 0;
    //! in case error status, returns error string
    virtual std::string errorString() const = 0;
    virtual bool setPassword(const std::string &password) = 0;
    virtual std::string address() const = 0;
    virtual bool store(const std::string &path) = 0;
    virtual bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit) = 0;
    virtual bool connectToDaemon() = 0;
    virtual uint64_t balance() const = 0;
    virtual uint64_t unlockedBalance() const = 0;
    static std::string displayAmount(uint64_t amount);
    // TODO?
    // virtual uint64_t unlockedDustBalance() const = 0;
    virtual bool refresh() = 0;
    virtual PendingTransaction * createTransaction(const std::string &dst_addr, uint64_t amount) = 0;
    virtual void disposeTransaction(PendingTransaction * t) = 0;
    virtual TransactionHistory * history() const = 0;
};

/**
 * @brief WalletManager - provides functions to manage wallets
 */
struct WalletManager
{

    /*!
     * \brief  Creates new wallet
     * \param  path           Name of wallet file
     * \param  password       Password of wallet file
     * \param  language       Language to be used to generate electrum seed memo
     * \return                Wallet instance (Wallet::status() needs to be called to check if created successfully)
     */
    virtual Wallet * createWallet(const std::string &path, const std::string &password, const std::string &language, bool testnet = false) = 0;

    /*!
     * \brief  Opens existing wallet
     * \param  path           Name of wallet file
     * \param  password       Password of wallet file
     * \return                Wallet instance (Wallet::status() needs to be called to check if opened successfully)
     */
    virtual Wallet * openWallet(const std::string &path, const std::string &password, bool testnet = false) = 0;

    /*!
     * \brief  recovers existing wallet using memo (electrum seed)
     * \param  path           Name of wallet file to be created
     * \param  memo           memo (25 words electrum seed)
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &memo, bool testnet = false) = 0;

    /*!
     * \brief Closes wallet. In case operation succeded, wallet object deleted. in case operation failed, wallet object not deleted
     * \param wallet        previously opened / created wallet instance
     * \return              None
     */
    virtual bool closeWallet(Wallet *wallet) = 0;

    //! checks if wallet with the given name already exists
    virtual bool walletExists(const std::string &path) = 0;

    virtual std::string errorString() const = 0;
//    //! set
//    virtual void setDaemonAddress(const std::string &address) = 0;
};


struct WalletManagerFactory
{
    static WalletManager * getWalletManager();
};

}

