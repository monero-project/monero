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
#include <ctime>

//  Public interface for libwallet library
namespace Bitmonero {

    namespace Utils {
        bool isAddressLocal(const std::string &hostaddr);
    }
/**
 * @brief Transaction-like interface for sending money
 */
struct PendingTransaction
{
    enum Status {
        Status_Ok,
        Status_Error
    };

    enum Priority {
        Priority_Low = 1,
        Priority_Medium = 2,
        Priority_High = 3,
        Priority_Last
    };

    virtual ~PendingTransaction() = 0;
    virtual int status() const = 0;
    virtual std::string errorString() const = 0;
    virtual bool commit() = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t dust() const = 0;
    virtual uint64_t fee() const = 0;
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

    struct Transfer {
        Transfer(uint64_t _amount, const std::string &address);
        const uint64_t amount;
        const std::string address;
    };

    virtual ~TransactionInfo() = 0;
    virtual int  direction() const = 0;
    virtual bool isPending() const = 0;
    virtual bool isFailed() const = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t fee() const = 0;
    virtual uint64_t blockHeight() const = 0;
    //! transaction_id
    virtual std::string hash() const = 0;
    virtual std::time_t timestamp() const = 0;
    virtual std::string paymentId() const = 0;
    //! only applicable for output transactions
    virtual const std::vector<Transfer> & transfers() const = 0;
};
/**
 * @brief The TransactionHistory - interface for displaying transaction history
 */
struct TransactionHistory
{
    virtual ~TransactionHistory() = 0;
    virtual int count() const = 0;
    virtual TransactionInfo * transaction(int index)  const = 0;
    virtual TransactionInfo * transaction(const std::string &id) const = 0;
    virtual std::vector<TransactionInfo*> getAll() const = 0;
    virtual void refresh() = 0;
};


struct WalletListener
{
    virtual ~WalletListener() = 0;
    /**
     * @brief moneySpent - called when money spent
     * @param txId       - transaction id
     * @param amount     - amount
     */
    virtual void moneySpent(const std::string &txId, uint64_t amount) = 0;

    /**
     * @brief moneyReceived - called when money received
     * @param txId          - transaction id
     * @param amount        - amount
     */
    virtual void moneyReceived(const std::string &txId, uint64_t amount) = 0;

    /**
     * @brief newBlock      - called when new block received
     * @param height        - block height
     */
    virtual void newBlock(uint64_t height) = 0;

    /**
     * @brief updated  - generic callback, called when any event (sent/received/block reveived/etc) happened with the wallet;
     */
    virtual void updated() = 0;


    /**
     * @brief refreshed - called when wallet refreshed by background thread or explicitly refreshed by calling "refresh" synchronously
     */
    virtual void refreshed() = 0;
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
    //! returns wallet status (Status_Ok | Status_Error)
    virtual int status() const = 0;
    //! in case error status, returns error string
    virtual std::string errorString() const = 0;
    virtual bool setPassword(const std::string &password) = 0;
    virtual std::string address() const = 0;
    
    /*!
     * \brief integratedAddress - returns integrated address for current wallet address and given payment_id.
     *                            if passed "payment_id" param is an empty string or not-valid payment id string
     *                            (16 characters hexadecimal string) - random payment_id will be generated
     *
     * \param payment_id        - 16 characters hexadecimal string or empty string if new random payment id needs to be
     *                            generated
     * \return                  - 106 characters string representing integrated address
     */
    virtual std::string integratedAddress(const std::string &payment_id) const = 0;
    
    /*!
     * \brief store - stores wallet to file.
     * \param path - main filename to store wallet to. additionally stores address file and keys file.
     *               to store to the same file - just pass empty string;
     * \return
     */
    virtual bool store(const std::string &path) = 0;
    /*!
     * \brief filename - returns wallet filename
     * \return
     */
    virtual std::string filename() const = 0;
    /*!
     * \brief keysFilename - returns keys filename. usually this formed as "wallet_filename".keys
     * \return
     */
    virtual std::string keysFilename() const = 0;
    /*!
     * \brief init - initializes wallet with daemon connection params. implicitly connects to the daemon
     *               and refreshes the wallet. "refreshed" callback will be invoked. if daemon_address is
     *               local address, "trusted daemon" will be set to true forcibly
     *
     * \param daemon_address - daemon address in "hostname:port" format
     * \param upper_transaction_size_limit
     * \return  - true if initialized and refreshed successfully
     */
    virtual bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit) = 0;

    /*!
     * \brief init - initalizes wallet asynchronously. logic is the same as "init" but returns immediately.
     *               "refreshed" callback will be invoked.
     *
     * \param daemon_address - daemon address in "hostname:port" format
     * \param upper_transaction_size_limit
     * \return  - true if initialized and refreshed successfully
     */
    virtual void initAsync(const std::string &daemon_address, uint64_t upper_transaction_size_limit) = 0;

   /*!
    * \brief setRefreshFromBlockHeight - start refresh from block height on recover
    *
    * \param refresh_from_block_height - blockchain start height
    */
    virtual void setRefreshFromBlockHeight(uint64_t refresh_from_block_height) = 0;

   /*!
    * \brief setRecoveringFromSeed - set state recover form seed
    *
    * \param recoveringFromSeed - true/false
    */
    virtual void setRecoveringFromSeed(bool recoveringFromSeed) = 0;

    /**
     * @brief connectToDaemon - connects to the daemon. TODO: check if it can be removed
     * @return
     */
    virtual bool connectToDaemon() = 0;

    /**
     * @brief connected - checks if the wallet connected to the daemon
     * @return - true if connected
     */
    virtual bool connected() const = 0;
    virtual void setTrustedDaemon(bool arg) = 0;
    virtual bool trustedDaemon() const = 0;
    virtual uint64_t balance() const = 0;
    virtual uint64_t unlockedBalance() const = 0;

    /**
     * @brief blockChainHeight - returns current blockchain height
     * @return
     */
    virtual uint64_t blockChainHeight() const = 0;

    /**
     * @brief daemonBlockChainHeight - returns daemon blockchain height
     * @return 0 - in case error communicating with the daemon.
     *             status() will return Status_Error and errorString() will return verbose error description
     */
    virtual uint64_t daemonBlockChainHeight() const = 0;

    /**
     * @brief daemonBlockChainTargetHeight - returns daemon blockchain target height
     * @return 0 - in case error communicating with the daemon.
     *             status() will return Status_Error and errorString() will return verbose error description
     */
    virtual uint64_t daemonBlockChainTargetHeight() const = 0;

    /**
     * @brief synchronized - checks if wallet was ever synchronized
     * @return
     */
    virtual bool synchronized() const = 0;

    static std::string displayAmount(uint64_t amount);
    static uint64_t amountFromString(const std::string &amount);
    static uint64_t amountFromDouble(double amount);
    static std::string genPaymentId();
    static bool paymentIdValid(const std::string &paiment_id);
    static bool addressValid(const std::string &str, bool testnet);
    static std::string paymentIdFromAddress(const std::string &str, bool testnet);
    static uint64_t maximumAllowedAmount();

    /**
     * @brief refresh - refreshes the wallet, updating transactions from daemon
     * @return - true if refreshed successfully;
     */
    virtual bool refresh() = 0;

    /**
     * @brief refreshAsync - refreshes wallet asynchronously.
     */
    virtual void refreshAsync() = 0;

    /**
     * @brief setAutoRefreshInterval - setup interval for automatic refresh.
     * @param seconds - interval in millis. if zero or less than zero - automatic refresh disabled;
     */
    virtual void setAutoRefreshInterval(int millis) = 0;

    /**
     * @brief autoRefreshInterval - returns automatic refresh interval in millis
     * @return
     */
    virtual int autoRefreshInterval() const = 0;


    /*!
     * \brief createTransaction creates transaction. if dst_addr is an integrated address, payment_id is ignored
     * \param dst_addr          destination address as string
     * \param payment_id        optional payment_id, can be empty string
     * \param amount            amount
     * \param mixin_count       mixin count. if 0 passed, wallet will use default value
     * \param priority
     * \return                  PendingTransaction object. caller is responsible to check PendingTransaction::status()
     *                          after object returned
     */

    virtual PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                                   uint64_t amount, uint32_t mixin_count,
                                                   PendingTransaction::Priority = PendingTransaction::Priority_Low) = 0;

    /*!
     * \brief disposeTransaction - destroys transaction object
     * \param t -  pointer to the "PendingTransaction" object. Pointer is not valid after function returned;
     */
    virtual void disposeTransaction(PendingTransaction * t) = 0;
    virtual TransactionHistory * history() const = 0;
    virtual void setListener(WalletListener *) = 0;
    /*!
     * \brief defaultMixin - returns number of mixins used in transactions
     * \return
     */
    virtual uint32_t defaultMixin() const = 0;
    /*!
     * \brief setDefaultMixin - setum number of mixins to be used for new transactions
     * \param arg
     */
    virtual void setDefaultMixin(uint32_t arg) = 0;
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
     * \param  testnet        testnet
     * \param  restoreHeight  restore from start height
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &memo, bool testnet = false, uint64_t restoreHeight = 0) = 0;

    /*!
     * \brief Closes wallet. In case operation succeded, wallet object deleted. in case operation failed, wallet object not deleted
     * \param wallet        previously opened / created wallet instance
     * \return              None
     */
    virtual bool closeWallet(Wallet *wallet) = 0;

    /*
     * ! checks if wallet with the given name already exists
     */

    /*!
     * @brief TODO: delme walletExists - check if the given filename is the wallet
     * @param path - filename
     * @return
     */
    virtual bool walletExists(const std::string &path) = 0;

    /*!
     * \brief findWallets - searches for the wallet files by given path name recursively
     * \param path - starting point to search
     * \return - list of strings with found wallets (absolute paths);
     */
    virtual std::vector<std::string> findWallets(const std::string &path) = 0;

    //! returns verbose error string regarding last error;
    virtual std::string errorString() const = 0;

};


struct WalletManagerFactory
{
    // logging levels for underlying library
    enum LogLevel {
        LogLevel_Silent = -1,
        LogLevel_0 = 0,
        LogLevel_1 = 1,
        LogLevel_2 = 2,
        LogLevel_3 = 3,
        LogLevel_4 = 4,
        LogLevel_Min = LogLevel_Silent,
        LogLevel_Max = LogLevel_4
    };

    static WalletManager * getWalletManager();
    static void setLogLevel(int level);
};


}

