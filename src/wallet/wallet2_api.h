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

#pragma once


#include <string>
#include <vector>
#include <ctime>
#include <iostream>

//  Public interface for libwallet library
namespace Monero {

    namespace Utils {
        bool isAddressLocal(const std::string &hostaddr);
    }

    template<typename T>
    class optional {
      public:
        optional(): set(false) {}
        optional(const T &t): t(t), set(true) {}
        const T &operator*() const { return t; }
        T &operator*() { return t; }
        operator bool() const { return set; }
      private:
        T t;
        bool set;
    };

/**
 * @brief Transaction-like interface for sending money
 */
struct PendingTransaction
{
    enum Status {
        Status_Ok,
        Status_Error,
        Status_Critical
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
    // commit transaction or save to file if filename is provided.
    virtual bool commit(const std::string &filename = "", bool overwrite = false) = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t dust() const = 0;
    virtual uint64_t fee() const = 0;
    virtual std::vector<std::string> txid() const = 0;
    /*!
     * \brief txCount - number of transactions current transaction will be splitted to
     * \return
     */
    virtual uint64_t txCount() const = 0;
};

/**
 * @brief Transaction-like interface for sending money
 */
struct UnsignedTransaction
{
    enum Status {
        Status_Ok,
        Status_Error,
        Status_Critical
    };

    enum Priority {
        Priority_Low = 1,
        Priority_Medium = 2,
        Priority_High = 3,
        Priority_Last
    };

    virtual ~UnsignedTransaction() = 0;
    virtual int status() const = 0;
    virtual std::string errorString() const = 0;
    virtual std::vector<uint64_t> amount() const = 0;
    virtual std::vector<uint64_t>  fee() const = 0;
    virtual std::vector<uint64_t> mixin() const = 0;
    // returns a string with information about all transactions.
    virtual std::string confirmationMessage() const = 0;
    virtual std::vector<std::string> paymentId() const = 0;
    virtual std::vector<std::string> recipientAddress() const = 0;
    virtual uint64_t minMixinCount() const = 0;
    /*!
     * \brief txCount - number of transactions current transaction will be splitted to
     * \return
     */
    virtual uint64_t txCount() const = 0;
   /*!
    * @brief sign - Sign txs and saves to file
    * @param signedFileName
    * return - true on success
    */
    virtual bool sign(const std::string &signedFileName) = 0;
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
    virtual uint64_t confirmations() const = 0;
    virtual uint64_t unlockTime() const = 0;
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

/**
 * @brief AddressBookRow - provides functions to manage address book
 */
struct AddressBookRow {
public:
    AddressBookRow(std::size_t _rowId, const std::string &_address, const std::string &_paymentId, const std::string &_description):
        m_rowId(_rowId),
        m_address(_address),
        m_paymentId(_paymentId), 
        m_description(_description) {}
 
private:
    std::size_t m_rowId;
    std::string m_address;
    std::string m_paymentId;
    std::string m_description;
public:
    std::string extra;
    std::string getAddress() const {return m_address;} 
    std::string getDescription() const {return m_description;} 
    std::string getPaymentId() const {return m_paymentId;} 
    std::size_t getRowId() const {return m_rowId;}
};

/**
 * @brief The AddressBook - interface for 
Book
 */
struct AddressBook
{
    enum ErrorCode {
        Status_Ok,
        General_Error,
        Invalid_Address,
        Invalid_Payment_Id
    };
    virtual ~AddressBook() = 0;
    virtual std::vector<AddressBookRow*> getAll() const = 0;
    virtual bool addRow(const std::string &dst_addr , const std::string &payment_id, const std::string &description) = 0;  
    virtual bool deleteRow(std::size_t rowId) = 0;
    virtual void refresh() = 0;  
    virtual std::string errorString() const = 0;
    virtual int errorCode() const = 0;
    virtual int lookupPaymentID(const std::string &payment_id) const = 0;
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
    * @brief unconfirmedMoneyReceived - called when payment arrived in tx pool
    * @param txId          - transaction id
    * @param amount        - amount
    */
    virtual void unconfirmedMoneyReceived(const std::string &txId, uint64_t amount) = 0;

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
        Status_Error,
        Status_Critical
    };

    enum ConnectionStatus {
        ConnectionStatus_Disconnected,
        ConnectionStatus_Connected,
        ConnectionStatus_WrongVersion
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
    virtual std::string path() const = 0;
    virtual bool testnet() const = 0;
    //! returns current hard fork info
    virtual void hardForkInfo(uint8_t &version, uint64_t &earliest_height) const = 0;
    //! check if hard fork rules should be used
    virtual bool useForkRules(uint8_t version, int64_t early_blocks) const = 0;  
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
    * \brief secretViewKey     - returns secret view key
    * \return                  - secret view key
    */
    virtual std::string secretViewKey() const = 0;

   /*!
    * \brief publicViewKey     - returns public view key
    * \return                  - public view key
    */
    virtual std::string publicViewKey() const = 0;

   /*!
    * \brief secretSpendKey    - returns secret spend key
    * \return                  - secret spend key
    */
    virtual std::string secretSpendKey() const = 0;

   /*!
    * \brief publicSpendKey    - returns public spend key
    * \return                  - public spend key
    */
    virtual std::string publicSpendKey() const = 0;

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
     * \brief init - initializes wallet with daemon connection params.
     *               if daemon_address is local address, "trusted daemon" will be set to true forcibly
     *               startRefresh() should be called when wallet is initialized.
     *
     * \param daemon_address - daemon address in "hostname:port" format
     * \param upper_transaction_size_limit
     * \return  - true on success
     */
    virtual bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit, const std::string &daemon_username = "", const std::string &daemon_password = "") = 0;

   /*!
    * \brief createWatchOnly - Creates a watch only wallet
    * \param path - where to store the wallet
    * \param password
    * \param language
    * \return  - true if created successfully
    */
    virtual bool createWatchOnly(const std::string &path, const std::string &password, const std::string &language) const = 0;

   /*!
    * \brief setRefreshFromBlockHeight - start refresh from block height on recover
    *
    * \param refresh_from_block_height - blockchain start height
    */
    virtual void setRefreshFromBlockHeight(uint64_t refresh_from_block_height) = 0;

   /*!
    * \brief getRestoreHeight - get wallet creation height
    *
    */
    virtual uint64_t getRefreshFromBlockHeight() const = 0;

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
    virtual ConnectionStatus connected() const = 0;
    virtual void setTrustedDaemon(bool arg) = 0;
    virtual bool trustedDaemon() const = 0;
    virtual uint64_t balance() const = 0;
    virtual uint64_t unlockedBalance() const = 0;

   /**
    * @brief watchOnly - checks if wallet is watch only
    * @return - true if watch only
    */
    virtual bool watchOnly() const = 0;

    /**
     * @brief blockChainHeight - returns current blockchain height
     * @return
     */
    virtual uint64_t blockChainHeight() const = 0;

    /**
    * @brief approximateBlockChainHeight - returns approximate blockchain height calculated from date/time
    * @return
    */
    virtual uint64_t approximateBlockChainHeight() const = 0;

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
    static bool keyValid(const std::string &secret_key_string, const std::string &address_string, bool isViewKey, bool testnet, std::string &error);
    static std::string paymentIdFromAddress(const std::string &str, bool testnet);
    static uint64_t maximumAllowedAmount();
    // Easylogger wrapper
    static void init(const char *argv0, const char *default_log_base_name);
    static void debug(const std::string &str);

   /**
    * @brief StartRefresh - Start/resume refresh thread (refresh every 10 seconds)
    */
    virtual void startRefresh() = 0;
   /**
    * @brief pauseRefresh - pause refresh thread
    */
    virtual void pauseRefresh() = 0;

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
                                                   optional<uint64_t> amount, uint32_t mixin_count,
                                                   PendingTransaction::Priority = PendingTransaction::Priority_Low) = 0;

    /*!
     * \brief createSweepUnmixableTransaction creates transaction with unmixable outputs.
     * \return                  PendingTransaction object. caller is responsible to check PendingTransaction::status()
     *                          after object returned
     */

    virtual PendingTransaction * createSweepUnmixableTransaction() = 0;
    
   /*!
    * \brief loadUnsignedTx  - creates transaction from unsigned tx file
    * \return                - UnsignedTransaction object. caller is responsible to check UnsignedTransaction::status()
    *                          after object returned
    */
    virtual UnsignedTransaction * loadUnsignedTx(const std::string &unsigned_filename) = 0;
    
   /*!
    * \brief submitTransaction - submits transaction in signed tx file
    * \return                  - true on success
    */
    virtual bool submitTransaction(const std::string &fileName) = 0;
    

    /*!
     * \brief disposeTransaction - destroys transaction object
     * \param t -  pointer to the "PendingTransaction" object. Pointer is not valid after function returned;
     */
    virtual void disposeTransaction(PendingTransaction * t) = 0;

   /*!
    * \brief exportKeyImages - exports key images to file
    * \param filename
    * \return                  - true on success
    */
    virtual bool exportKeyImages(const std::string &filename) = 0;
   
   /*!
    * \brief importKeyImages - imports key images from file
    * \param filename
    * \return                  - true on success
    */
    virtual bool importKeyImages(const std::string &filename) = 0;


    virtual TransactionHistory * history() const = 0;
    virtual AddressBook * addressBook() const = 0;
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

    /*!
     * \brief setUserNote - attach an arbitrary string note to a txid
     * \param txid - the transaction id to attach the note to
     * \param note - the note
     * \return true if succesful, false otherwise
     */
    virtual bool setUserNote(const std::string &txid, const std::string &note) = 0;
    /*!
     * \brief getUserNote - return an arbitrary string note attached to a txid
     * \param txid - the transaction id to attach the note to
     * \return the attached note, or empty string if there is none
     */
    virtual std::string getUserNote(const std::string &txid) const = 0;
    virtual std::string getTxKey(const std::string &txid) const = 0;

    /*
     * \brief signMessage - sign a message with the spend private key
     * \param message - the message to sign (arbitrary byte data)
     * \return the signature
     */
    virtual std::string signMessage(const std::string &message) = 0;
    /*!
     * \brief verifySignedMessage - verify a signature matches a given message
     * \param message - the message (arbitrary byte data)
     * \param address - the address the signature claims to be made with
     * \param signature - the signature
     * \return true if the signature verified, false otherwise
     */
    virtual bool verifySignedMessage(const std::string &message, const std::string &addres, const std::string &signature) const = 0;

    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error) = 0;

    virtual std::string getDefaultDataDir() const = 0;
   
   /*
    * \brief rescanSpent - Rescan spent outputs - Can only be used with trusted daemon
    * \return true on success
    */
    virtual bool rescanSpent() = 0;
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
    * \brief  recovers existing wallet using keys. Creates a view only wallet if spend key is omitted
    * \param  path           Name of wallet file to be created
    * \param  language       language
    * \param  testnet        testnet
    * \param  restoreHeight  restore from start height
    * \param  addressString  public address
    * \param  viewKeyString  view key
    * \param  spendKeyString spend key (optional)
    * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
    */
    virtual Wallet * createWalletFromKeys(const std::string &path, 
                                                    const std::string &language,
                                                    bool testnet, 
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "") = 0;

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
     * @return - true if wallet exists
     */
    virtual bool walletExists(const std::string &path) = 0;

    /*!
     * @brief verifyWalletPassword - check if the given filename is the wallet
     * @param keys_file_name - location of keys file
     * @param password - password to verify
     * @param watch_only - verify only view keys?
     * @return - true if password is correct
     */
    virtual bool verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool watch_only) const = 0;

    /*!
     * \brief findWallets - searches for the wallet files by given path name recursively
     * \param path - starting point to search
     * \return - list of strings with found wallets (absolute paths);
     */
    virtual std::vector<std::string> findWallets(const std::string &path) = 0;

    /*!
     * \brief checkPayment - checks a payment was made using a txkey
     * \param address - the address the payment was sent to
     * \param txid - the transaction id for that payment
     * \param txkey - the transaction's secret key
     * \param daemon_address - the address (host and port) to the daemon to request transaction data
     * \param received - if succesful, will hold the amount of monero received
     * \param height - if succesful, will hold the height of the transaction (0 if only in the pool)
     * \param error - if unsuccesful, will hold an error string with more information about the error
     * \return - true is succesful, false otherwise
     */
    virtual bool checkPayment(const std::string &address, const std::string &txid, const std::string &txkey, const std::string &daemon_address, uint64_t &received, uint64_t &height, std::string &error) const = 0;

    //! returns verbose error string regarding last error;
    virtual std::string errorString() const = 0;

    //! set the daemon address (hostname and port)
    virtual void setDaemonAddress(const std::string &address) = 0;

    //! returns whether the daemon can be reached, and its version number
    virtual bool connected(uint32_t *version = NULL) const = 0;

    //! returns current blockchain height
    virtual uint64_t blockchainHeight() const = 0;

    //! returns current blockchain target height
    virtual uint64_t blockchainTargetHeight() const = 0;

    //! returns current network difficulty
    virtual uint64_t networkDifficulty() const = 0;

    //! returns current mining hash rate (0 if not mining)
    virtual double miningHashRate() const = 0;

    //! returns current block target
    virtual uint64_t blockTarget() const = 0;

    //! returns true iff mining
    virtual bool isMining() const = 0;

    //! starts mining with the set number of threads
    virtual bool startMining(const std::string &address, uint32_t threads = 1, bool background_mining = false, bool ignore_battery = true) = 0;

    //! stops mining
    virtual bool stopMining() = 0;

    //! resolves an OpenAlias address to a monero address
    virtual std::string resolveOpenAlias(const std::string &address, bool &dnssec_valid) const = 0;

    //! checks for an update and returns version, hash and url
    static std::tuple<bool, std::string, std::string, std::string, std::string> checkUpdates(const std::string &software, const std::string &subdir);
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
    static void setLogCategories(const std::string &categories);
};


}

namespace Bitmonero = Monero;

