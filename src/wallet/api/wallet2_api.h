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

#pragma once


#include <string>
#include <vector>
#include <list>
#include <set>
#include <ctime>
#include <iostream>

//  Public interface for libwallet library
namespace Monero {

enum NetworkType : uint8_t {
    MAINNET = 0,
    TESTNET,
    STAGENET
};

    namespace Utils {
        bool isAddressLocal(const std::string &hostaddr);
        void onStartup();
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
        Priority_Default = 0,
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
    virtual std::vector<uint32_t> subaddrAccount() const = 0;
    virtual std::vector<std::set<uint32_t>> subaddrIndices() const = 0;
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
    virtual std::set<uint32_t> subaddrIndex() const = 0;
    virtual uint32_t subaddrAccount() const = 0;
    virtual std::string label() const = 0;
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

struct SubaddressRow {
public:
    SubaddressRow(std::size_t _rowId, const std::string &_address, const std::string &_label):
        m_rowId(_rowId),
        m_address(_address),
        m_label(_label) {}
 
private:
    std::size_t m_rowId;
    std::string m_address;
    std::string m_label;
public:
    std::string extra;
    std::string getAddress() const {return m_address;}
    std::string getLabel() const {return m_label;}
    std::size_t getRowId() const {return m_rowId;}
};

struct Subaddress
{
    virtual ~Subaddress() = 0;
    virtual std::vector<SubaddressRow*> getAll() const = 0;
    virtual void addRow(uint32_t accountIndex, const std::string &label) = 0;
    virtual void setLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label) = 0;
    virtual void refresh(uint32_t accountIndex) = 0;
};

struct SubaddressAccountRow {
public:
    SubaddressAccountRow(std::size_t _rowId, const std::string &_address, const std::string &_label, const std::string &_balance, const std::string &_unlockedBalance):
        m_rowId(_rowId),
        m_address(_address),
        m_label(_label),
        m_balance(_balance),
        m_unlockedBalance(_unlockedBalance) {}

private:
    std::size_t m_rowId;
    std::string m_address;
    std::string m_label;
    std::string m_balance;
    std::string m_unlockedBalance;
public:
    std::string extra;
    std::string getAddress() const {return m_address;}
    std::string getLabel() const {return m_label;}
    std::string getBalance() const {return m_balance;}
    std::string getUnlockedBalance() const {return m_unlockedBalance;}
    std::size_t getRowId() const {return m_rowId;}
};

struct SubaddressAccount
{
    virtual ~SubaddressAccount() = 0;
    virtual std::vector<SubaddressAccountRow*> getAll() const = 0;
    virtual void addRow(const std::string &label) = 0;
    virtual void setLabel(uint32_t accountIndex, const std::string &label) = 0;
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
    virtual std::string address(uint32_t accountIndex = 0, uint32_t addressIndex = 0) const = 0;
    std::string mainAddress() const { return address(0, 0); }
    virtual std::string path() const = 0;
    virtual NetworkType nettype() const = 0;
    bool mainnet() const { return nettype() == MAINNET; }
    bool testnet() const { return nettype() == TESTNET; }
    bool stagenet() const { return nettype() == STAGENET; }
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
     * \param daemon_username
     * \param daemon_password
     * \param lightWallet - start wallet in light mode, connect to a openmonero compatible server.
     * \return  - true on success
     */
    virtual bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, const std::string &daemon_username = "", const std::string &daemon_password = "", bool use_ssl = false, bool lightWallet = false) = 0;

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

   /*!
    * \brief setRecoveringFromDevice - set state to recovering from device
    *
    * \param recoveringFromDevice - true/false
    */
    virtual void setRecoveringFromDevice(bool recoveringFromDevice) = 0;

    /*!
     * \brief setSubaddressLookahead - set size of subaddress lookahead
     *
     * \param major - size fot the major index
     * \param minor - size fot the minor index
     */
    virtual void setSubaddressLookahead(uint32_t major, uint32_t minor) = 0;

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
    virtual uint64_t balance(uint32_t accountIndex = 0) const = 0;
    uint64_t balanceAll() const {
        uint64_t result = 0;
        for (uint32_t i = 0; i < numSubaddressAccounts(); ++i)
            result += balance(i);
        return result;
    }
    virtual uint64_t unlockedBalance(uint32_t accountIndex = 0) const = 0;
    uint64_t unlockedBalanceAll() const {
        uint64_t result = 0;
        for (uint32_t i = 0; i < numSubaddressAccounts(); ++i)
            result += unlockedBalance(i);
        return result;
    }

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
    static bool addressValid(const std::string &str, NetworkType nettype);
    static bool addressValid(const std::string &str, bool testnet)          // deprecated
    {
        return addressValid(str, testnet ? TESTNET : MAINNET);
    }
    static bool keyValid(const std::string &secret_key_string, const std::string &address_string, bool isViewKey, NetworkType nettype, std::string &error);
    static bool keyValid(const std::string &secret_key_string, const std::string &address_string, bool isViewKey, bool testnet, std::string &error)     // deprecated
    {
        return keyValid(secret_key_string, address_string, isViewKey, testnet ? TESTNET : MAINNET, error);
    }
    static std::string paymentIdFromAddress(const std::string &str, NetworkType nettype);
    static std::string paymentIdFromAddress(const std::string &str, bool testnet)       // deprecated
    {
        return paymentIdFromAddress(str, testnet ? TESTNET : MAINNET);
    }
    static uint64_t maximumAllowedAmount();
    // Easylogger wrapper
    static void init(const char *argv0, const char *default_log_base_name) { init(argv0, default_log_base_name, "", true); }
    static void init(const char *argv0, const char *default_log_base_name, const std::string &log_path, bool console);
    static void debug(const std::string &category, const std::string &str);
    static void info(const std::string &category, const std::string &str);
    static void warning(const std::string &category, const std::string &str);
    static void error(const std::string &category, const std::string &str);

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

    /**
     * @brief addSubaddressAccount - appends a new subaddress account at the end of the last major index of existing subaddress accounts
     * @param label - the label for the new account (which is the as the label of the primary address (accountIndex,0))
     */
    virtual void addSubaddressAccount(const std::string& label) = 0;
    /**
     * @brief numSubaddressAccounts - returns the number of existing subaddress accounts
     */
    virtual size_t numSubaddressAccounts() const = 0;
    /**
     * @brief numSubaddresses - returns the number of existing subaddresses associated with the specified subaddress account
     * @param accountIndex - the major index specifying the subaddress account
     */
    virtual size_t numSubaddresses(uint32_t accountIndex) const = 0;
    /**
     * @brief addSubaddress - appends a new subaddress at the end of the last minor index of the specified subaddress account
     * @param accountIndex - the major index specifying the subaddress account
     * @param label - the label for the new subaddress
     */
    virtual void addSubaddress(uint32_t accountIndex, const std::string& label) = 0;
    /**
     * @brief getSubaddressLabel - gets the label of the specified subaddress
     * @param accountIndex - the major index specifying the subaddress account
     * @param addressIndex - the minor index specifying the subaddress
     */
    virtual std::string getSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex) const = 0;
    /**
     * @brief setSubaddressLabel - sets the label of the specified subaddress
     * @param accountIndex - the major index specifying the subaddress account
     * @param addressIndex - the minor index specifying the subaddress
     * @param label - the new label for the specified subaddress
     */
    virtual void setSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label) = 0;

    /*!
     * \brief createTransaction creates transaction. if dst_addr is an integrated address, payment_id is ignored
     * \param dst_addr          destination address as string
     * \param payment_id        optional payment_id, can be empty string
     * \param amount            amount
     * \param mixin_count       mixin count. if 0 passed, wallet will use default value
     * \param subaddr_account   subaddress account from which the input funds are taken
     * \param subaddr_indices   set of subaddress indices to use for transfer or sweeping. if set empty, all are chosen when sweeping, and one or more are automatically chosen when transferring. after execution, returns the set of actually used indices
     * \param priority
     * \return                  PendingTransaction object. caller is responsible to check PendingTransaction::status()
     *                          after object returned
     */

    virtual PendingTransaction * createTransaction(const std::string &dst_addr, const std::string &payment_id,
                                                   optional<uint64_t> amount, uint32_t mixin_count,
                                                   PendingTransaction::Priority = PendingTransaction::Priority_Low,
                                                   uint32_t subaddr_account = 0,
                                                   std::set<uint32_t> subaddr_indices = {}) = 0;

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


    virtual TransactionHistory * history() = 0;
    virtual AddressBook * addressBook() = 0;
    virtual Subaddress * subaddress() = 0;
    virtual SubaddressAccount * subaddressAccount() = 0;
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
     * \return true if successful, false otherwise
     */
    virtual bool setUserNote(const std::string &txid, const std::string &note) = 0;
    /*!
     * \brief getUserNote - return an arbitrary string note attached to a txid
     * \param txid - the transaction id to attach the note to
     * \return the attached note, or empty string if there is none
     */
    virtual std::string getUserNote(const std::string &txid) const = 0;
    virtual std::string getTxKey(const std::string &txid) const = 0;
    virtual bool checkTxKey(const std::string &txid, std::string tx_key, const std::string &address, uint64_t &received, bool &in_pool, uint64_t &confirmations) = 0;
    virtual std::string getTxProof(const std::string &txid, const std::string &address, const std::string &message) const = 0;
    virtual bool checkTxProof(const std::string &txid, const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &received, bool &in_pool, uint64_t &confirmations) = 0;
    virtual std::string getSpendProof(const std::string &txid, const std::string &message) const = 0;
    virtual bool checkSpendProof(const std::string &txid, const std::string &message, const std::string &signature, bool &good) const = 0;
    /*!
     * \brief getReserveProof - Generates a proof that proves the reserve of unspent funds
     *                          Parameters `account_index` and `amount` are ignored when `all` is true
     */
    virtual std::string getReserveProof(bool all, uint32_t account_index, uint64_t amount, const std::string &message) const = 0;
    virtual bool checkReserveProof(const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &total, uint64_t &spent) const = 0;

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
    
    //! blackballs a set of outputs
    virtual bool blackballOutputs(const std::vector<std::string> &pubkeys, bool add) = 0;

    //! unblackballs an output
    virtual bool unblackballOutput(const std::string &pubkey) = 0;

    //! gets the ring used for a key image, if any
    virtual bool getRing(const std::string &key_image, std::vector<uint64_t> &ring) const = 0;

    //! gets the rings used for a txid, if any
    virtual bool getRings(const std::string &txid, std::vector<std::pair<std::string, std::vector<uint64_t>>> &rings) const = 0;

    //! sets the ring used for a key image
    virtual bool setRing(const std::string &key_image, const std::vector<uint64_t> &ring, bool relative) = 0;

    //! sets whether pre-fork outs are to be segregated
    virtual void segregatePreForkOutputs(bool segregate) = 0;

    //! sets the height where segregation should occur
    virtual void segregationHeight(uint64_t height) = 0;

    //! secondary key reuse mitigation
    virtual void keyReuseMitigation2(bool mitigation) = 0;

    //! Light wallet authenticate and login
    virtual bool lightWalletLogin(bool &isNewWallet) const = 0;
    
    //! Initiates a light wallet import wallet request
    virtual bool lightWalletImportWalletRequest(std::string &payment_id, uint64_t &fee, bool &new_request, bool &request_fulfilled, std::string &payment_address, std::string &status) = 0;
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
     * \param  language       Language to be used to generate electrum seed mnemonic
     * \param  nettype        Network type
     * \return                Wallet instance (Wallet::status() needs to be called to check if created successfully)
     */
    virtual Wallet * createWallet(const std::string &path, const std::string &password, const std::string &language, NetworkType nettype) = 0;
    Wallet * createWallet(const std::string &path, const std::string &password, const std::string &language, bool testnet = false)      // deprecated
    {
        return createWallet(path, password, language, testnet ? TESTNET : MAINNET);
    }

    /*!
     * \brief  Opens existing wallet
     * \param  path           Name of wallet file
     * \param  password       Password of wallet file
     * \param  nettype        Network type
     * \return                Wallet instance (Wallet::status() needs to be called to check if opened successfully)
     */
    virtual Wallet * openWallet(const std::string &path, const std::string &password, NetworkType nettype) = 0;
    Wallet * openWallet(const std::string &path, const std::string &password, bool testnet = false)     // deprecated
    {
        return openWallet(path, password, testnet ? TESTNET : MAINNET);
    }

    /*!
     * \brief  recovers existing wallet using mnemonic (electrum seed)
     * \param  path           Name of wallet file to be created
     * \param  password       Password of wallet file
     * \param  mnemonic       mnemonic (25 words electrum seed)
     * \param  nettype        Network type
     * \param  restoreHeight  restore from start height
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &password, const std::string &mnemonic,
                                    NetworkType nettype = MAINNET, uint64_t restoreHeight = 0) = 0;
    Wallet * recoveryWallet(const std::string &path, const std::string &password, const std::string &mnemonic,
                                    bool testnet = false, uint64_t restoreHeight = 0)           // deprecated
    {
        return recoveryWallet(path, password, mnemonic, testnet ? TESTNET : MAINNET, restoreHeight);
    }

    /*!
     * \deprecated this method creates a wallet WITHOUT a passphrase, use the alternate recoverWallet() method
     * \brief  recovers existing wallet using mnemonic (electrum seed)
     * \param  path           Name of wallet file to be created
     * \param  mnemonic       mnemonic (25 words electrum seed)
     * \param  nettype        Network type
     * \param  restoreHeight  restore from start height
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &mnemonic, NetworkType nettype, uint64_t restoreHeight = 0) = 0;
    Wallet * recoveryWallet(const std::string &path, const std::string &mnemonic, bool testnet = false, uint64_t restoreHeight = 0)         // deprecated
    {
        return recoveryWallet(path, mnemonic, testnet ? TESTNET : MAINNET, restoreHeight);
    }

    /*!
     * \brief  recovers existing wallet using keys. Creates a view only wallet if spend key is omitted
     * \param  path           Name of wallet file to be created
     * \param  password       Password of wallet file
     * \param  language       language
     * \param  nettype        Network type
     * \param  restoreHeight  restore from start height
     * \param  addressString  public address
     * \param  viewKeyString  view key
     * \param  spendKeyString spend key (optional)
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * createWalletFromKeys(const std::string &path,
                                                    const std::string &password,
                                                    const std::string &language,
                                                    NetworkType nettype,
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "") = 0;
    Wallet * createWalletFromKeys(const std::string &path,
                                  const std::string &password,
                                  const std::string &language,
                                  bool testnet,
                                  uint64_t restoreHeight,
                                  const std::string &addressString,
                                  const std::string &viewKeyString,
                                  const std::string &spendKeyString = "")       // deprecated
    {
        return createWalletFromKeys(path, password, language, testnet ? TESTNET : MAINNET, restoreHeight, addressString, viewKeyString, spendKeyString);
    }

   /*!
    * \deprecated this method creates a wallet WITHOUT a passphrase, use createWalletFromKeys(..., password, ...) instead
    * \brief  recovers existing wallet using keys. Creates a view only wallet if spend key is omitted
    * \param  path           Name of wallet file to be created
    * \param  language       language
    * \param  nettype        Network type
    * \param  restoreHeight  restore from start height
    * \param  addressString  public address
    * \param  viewKeyString  view key
    * \param  spendKeyString spend key (optional)
    * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
    */
    virtual Wallet * createWalletFromKeys(const std::string &path, 
                                                    const std::string &language,
                                                    NetworkType nettype, 
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "") = 0;
    Wallet * createWalletFromKeys(const std::string &path, 
                                  const std::string &language,
                                  bool testnet, 
                                  uint64_t restoreHeight,
                                  const std::string &addressString,
                                  const std::string &viewKeyString,
                                  const std::string &spendKeyString = "")           // deprecated
    {
        return createWalletFromKeys(path, language, testnet ? TESTNET : MAINNET, restoreHeight, addressString, viewKeyString, spendKeyString);
    }

    /*!
     * \brief  creates wallet using hardware device.
     * \param  path                 Name of wallet file to be created
     * \param  password             Password of wallet file
     * \param  nettype              Network type
     * \param  deviceName           Device name
     * \param  restoreHeight        restore from start height (0 sets to current height)
     * \param  subaddressLookahead  Size of subaddress lookahead (empty sets to some default low value)
     * \return                      Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * createWalletFromDevice(const std::string &path,
                                            const std::string &password,
                                            NetworkType nettype,
                                            const std::string &deviceName,
                                            uint64_t restoreHeight = 0,
                                            const std::string &subaddressLookahead = "") = 0;

    /*!
     * \brief Closes wallet. In case operation succeeded, wallet object deleted. in case operation failed, wallet object not deleted
     * \param wallet        previously opened / created wallet instance
     * \return              None
     */
    virtual bool closeWallet(Wallet *wallet, bool store = true) = 0;

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
     * @param no_spend_key - verify only view keys?
     * @return - true if password is correct
     */
    virtual bool verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool no_spend_key) const = 0;

    /*!
     * \brief findWallets - searches for the wallet files by given path name recursively
     * \param path - starting point to search
     * \return - list of strings with found wallets (absolute paths);
     */
    virtual std::vector<std::string> findWallets(const std::string &path) = 0;

    //! returns verbose error string regarding last error;
    virtual std::string errorString() const = 0;

    //! set the daemon address (hostname and port)
    virtual void setDaemonAddress(const std::string &address) = 0;

    //! returns whether the daemon can be reached, and its version number
    virtual bool connected(uint32_t *version = NULL) = 0;

    //! returns current blockchain height
    virtual uint64_t blockchainHeight() = 0;

    //! returns current blockchain target height
    virtual uint64_t blockchainTargetHeight() = 0;

    //! returns current network difficulty
    virtual uint64_t networkDifficulty() = 0;

    //! returns current mining hash rate (0 if not mining)
    virtual double miningHashRate() = 0;

    //! returns current block target
    virtual uint64_t blockTarget() = 0;

    //! returns true iff mining
    virtual bool isMining() = 0;

    //! starts mining with the set number of threads
    virtual bool startMining(const std::string &address, uint32_t threads = 1, bool background_mining = false, bool ignore_battery = true) = 0;

    //! stops mining
    virtual bool stopMining() = 0;

    //! resolves an OpenAlias address to a monero address
    virtual std::string resolveOpenAlias(const std::string &address, bool &dnssec_valid) const = 0;

    //! checks for an update and returns version, hash and url
    static std::tuple<bool, std::string, std::string, std::string, std::string> checkUpdates(const std::string &software, std::string subdir);
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

