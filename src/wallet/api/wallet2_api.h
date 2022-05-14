// Copyright (c) 2014-2022, The Monero Project
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
#include <stdexcept>

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

    /**
     * @brief multisigSignData
     * @return encoded multisig transaction with signers' keys.
     *         Transfer this data to another wallet participant to sign it.
     *         Assumed use case is:
     *         1. Initiator:
     *              auto data = pendingTransaction->multisigSignData();
     *         2. Signer1:
     *              pendingTransaction = wallet->restoreMultisigTransaction(data);
     *              pendingTransaction->signMultisigTx();
     *              auto signed = pendingTransaction->multisigSignData();
     *         3. Signer2:
     *              pendingTransaction = wallet->restoreMultisigTransaction(signed);
     *              pendingTransaction->signMultisigTx();
     *              pendingTransaction->commit();
     */
    virtual std::string multisigSignData() = 0;
    virtual void signMultisigTx() = 0;
    /**
     * @brief signersKeys
     * @return vector of base58-encoded signers' public keys
     */
    virtual std::vector<std::string> signersKeys() const = 0;
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
    virtual bool isCoinbase() const = 0;
    virtual uint64_t amount() const = 0;
    virtual uint64_t fee() const = 0;
    virtual uint64_t blockHeight() const = 0;
    virtual std::string description() const = 0;
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
    virtual void setTxNote(const std::string &txid, const std::string &note) = 0;
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
    virtual bool setDescription(std::size_t index, const std::string &description) = 0;
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

struct MultisigState {
    MultisigState() : isMultisig(false), isReady(false), threshold(0), total(0) {}

    bool isMultisig;
    bool isReady;
    uint32_t threshold;
    uint32_t total;
};


struct DeviceProgress {
    DeviceProgress(): m_progress(0), m_indeterminate(false) {}
    DeviceProgress(double progress, bool indeterminate=false): m_progress(progress), m_indeterminate(indeterminate) {}

    virtual double progress() const { return m_progress; }
    virtual bool indeterminate() const { return m_indeterminate; }

protected:
    double m_progress;
    bool m_indeterminate;
};

struct Wallet;
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

    /**
     * @brief called by device if the action is required
     */
    virtual void onDeviceButtonRequest(uint64_t code) { (void)code; }

    /**
     * @brief called by device if the button was pressed
     */
    virtual void onDeviceButtonPressed() { }

    /**
     * @brief called by device when PIN is needed
     */
    virtual optional<std::string> onDevicePinRequest() {
        throw std::runtime_error("Not supported");
    }

    /**
     * @brief called by device when passphrase entry is needed
     */
    virtual optional<std::string> onDevicePassphraseRequest(bool & on_device) {
        on_device = true;
        return optional<std::string>();
    }

    /**
     * @brief Signalizes device operation progress
     */
    virtual void onDeviceProgress(const DeviceProgress & event) { (void)event; };

    /**
     * @brief If the listener is created before the wallet this enables to set created wallet object
     */
    virtual void onSetWallet(Wallet * wallet) { (void)wallet; };
};


/**
 * @brief Interface for wallet operations.
 */
struct Wallet
{
    enum Device {
        Device_Software = 0,
        Device_Ledger = 1,
        Device_Trezor = 2
    };

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
    virtual std::string seed(const std::string& seed_offset = "") const = 0;
    virtual std::string getSeedLanguage() const = 0;
    virtual void setSeedLanguage(const std::string &arg) = 0;
    //! returns wallet status (Status_Ok | Status_Error)
    virtual int status() const = 0; //deprecated: use safe alternative statusWithErrorString
    //! in case error status, returns error string
    virtual std::string errorString() const = 0; //deprecated: use safe alternative statusWithErrorString
    //! returns both error and error string atomically. suggested to use in instead of status() and errorString()
    virtual void statusWithErrorString(int& status, std::string& errorString) const = 0;
    virtual bool setPassword(const std::string &password) = 0;
    virtual const std::string& getPassword() const = 0;
    virtual bool setDevicePin(const std::string &pin) { (void)pin; return false; };
    virtual bool setDevicePassphrase(const std::string &passphrase) { (void)passphrase; return false; };
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
     * \brief publicMultisigSignerKey - returns public signer key
     * \return                        - public multisignature signer key or empty string if wallet is not multisig
     */
    virtual std::string publicMultisigSignerKey() const = 0;

    /*!
     * \brief stop - interrupts wallet refresh() loop once (doesn't stop background refresh thread)
     */
    virtual void stop() = 0;

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
     * \param proxy_address - set proxy address, empty string to disable
     * \return  - true on success
     */
    virtual bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit = 0, const std::string &daemon_username = "", const std::string &daemon_password = "", bool use_ssl = false, bool lightWallet = false, const std::string &proxy_address = "") = 0;

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
    virtual bool setProxy(const std::string &address) = 0;
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
     * @brief isDeterministic - checks if wallet keys are deterministic
     * @return - true if deterministic
     */
    virtual bool isDeterministic() const = 0;

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
    * @brief estimateBlockChainHeight - returns estimate blockchain height. More accurate than approximateBlockChainHeight,
    *                                   uses daemon height and falls back to calculation from date/time
    * @return
    **/ 
    virtual uint64_t estimateBlockChainHeight() const = 0;
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
     * @brief rescanBlockchain - rescans the wallet, updating transactions from daemon
     * @return - true if refreshed successfully;
     */
    virtual bool rescanBlockchain() = 0;

    /**
     * @brief rescanBlockchainAsync - rescans wallet asynchronously, starting from genesys
     */
    virtual void rescanBlockchainAsync() = 0;

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

    /**
     * @brief multisig - returns current state of multisig wallet creation process
     * @return MultisigState struct
     */
    virtual MultisigState multisig() const = 0;
    /**
     * @brief getMultisigInfo
     * @return serialized and signed multisig info string
     */
    virtual std::string getMultisigInfo() const = 0;
    /**
     * @brief makeMultisig - switches wallet in multisig state. The one and only creation phase for N / N wallets
     * @param info - vector of multisig infos from other participants obtained with getMulitisInfo call
     * @param threshold - number of required signers to make valid transaction. Must be <= number of participants
     * @return in case of N / N wallets returns empty string since no more key exchanges needed. For N - 1 / N wallets returns base58 encoded extra multisig info
     */
    virtual std::string makeMultisig(const std::vector<std::string>& info, uint32_t threshold) = 0;
    /**
     * @brief exchange_multisig_keys - provides additional key exchange round for arbitrary multisig schemes (like N-1/N, M/N)
     * @param info - base58 encoded key derivations returned by makeMultisig or exchangeMultisigKeys function call
     * @param force_update_use_with_caution - force multisig account to update even if not all signers contribute round messages
     * @return new info string if more rounds required or an empty string if wallet creation is done
     */
    virtual std::string exchangeMultisigKeys(const std::vector<std::string> &info, const bool force_update_use_with_caution) = 0;
    /**
     * @brief exportMultisigImages - exports transfers' key images
     * @param images - output paramter for hex encoded array of images
     * @return true if success
     */
    virtual bool exportMultisigImages(std::string& images) = 0;
    /**
     * @brief importMultisigImages - imports other participants' multisig images
     * @param images - array of hex encoded arrays of images obtained with exportMultisigImages
     * @return number of imported images
     */
    virtual size_t importMultisigImages(const std::vector<std::string>& images) = 0;
    /**
     * @brief hasMultisigPartialKeyImages - checks if wallet needs to import multisig key images from other participants
     * @return true if there are partial key images
     */
    virtual bool hasMultisigPartialKeyImages() const = 0;

    /**
     * @brief restoreMultisigTransaction creates PendingTransaction from signData
     * @param signData encrypted unsigned transaction. Obtained with PendingTransaction::multisigSignData
     * @return PendingTransaction
     */
    virtual PendingTransaction*  restoreMultisigTransaction(const std::string& signData) = 0;

    /*!
     * \brief createTransactionMultDest creates transaction with multiple destinations. if dst_addr is an integrated address, payment_id is ignored
     * \param dst_addr                  vector of destination address as string
     * \param payment_id                optional payment_id, can be empty string
     * \param amount                    vector of amounts
     * \param mixin_count               mixin count. if 0 passed, wallet will use default value
     * \param subaddr_account           subaddress account from which the input funds are taken
     * \param subaddr_indices           set of subaddress indices to use for transfer or sweeping. if set empty, all are chosen when sweeping, and one or more are automatically chosen when transferring. after execution, returns the set of actually used indices
     * \param priority
     * \return                          PendingTransaction object. caller is responsible to check PendingTransaction::status()
     *                                  after object returned
     */

    virtual PendingTransaction * createTransactionMultDest(const std::vector<std::string> &dst_addr, const std::string &payment_id,
                                                   optional<std::vector<uint64_t>> amount, uint32_t mixin_count,
                                                   PendingTransaction::Priority = PendingTransaction::Priority_Low,
                                                   uint32_t subaddr_account = 0,
                                                   std::set<uint32_t> subaddr_indices = {}) = 0;

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
     * \brief Estimates transaction fee.
     * \param destinations Vector consisting of <address, amount> pairs.
     * \return Estimated fee.
     */
    virtual uint64_t estimateTransactionFee(const std::vector<std::pair<std::string, uint64_t>> &destinations,
                                            PendingTransaction::Priority priority) const = 0;

   /*!
    * \brief exportKeyImages - exports key images to file
    * \param filename
    * \param all - export all key images or only those that have not yet been exported
    * \return                  - true on success
    */
    virtual bool exportKeyImages(const std::string &filename, bool all = false) = 0;
   
   /*!
    * \brief importKeyImages - imports key images from file
    * \param filename
    * \return                  - true on success
    */
    virtual bool importKeyImages(const std::string &filename) = 0;

    /*!
     * \brief importOutputs - exports outputs to file
     * \param filename
     * \return                  - true on success
     */
    virtual bool exportOutputs(const std::string &filename, bool all = false) = 0;

    /*!
     * \brief importOutputs - imports outputs from file
     * \param filename
     * \return                  - true on success
     */
    virtual bool importOutputs(const std::string &filename) = 0;

    /*!
     * \brief scanTransactions - scan a list of transaction ids, this operation may reveal the txids to the remote node and affect your privacy
     * \param txids            - list of transaction ids
     * \return                 - true on success
     */
    virtual bool scanTransactions(const std::vector<std::string> &txids) = 0;

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
     * \brief setCacheAttribute - attach an arbitrary string to a wallet cache attribute
     * \param key - the key
     * \param val - the value
     * \return true if successful, false otherwise
     */
    virtual bool setCacheAttribute(const std::string &key, const std::string &val) = 0;
    /*!
     * \brief getCacheAttribute - return an arbitrary string attached to a wallet cache attribute
     * \param key - the key
     * \return the attached string, or empty string if there is none
     */
    virtual std::string getCacheAttribute(const std::string &key) const = 0;
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
    virtual std::string signMessage(const std::string &message, const std::string &address = "") = 0;
    /*!
     * \brief verifySignedMessage - verify a signature matches a given message
     * \param message - the message (arbitrary byte data)
     * \param address - the address the signature claims to be made with
     * \param signature - the signature
     * \return true if the signature verified, false otherwise
     */
    virtual bool verifySignedMessage(const std::string &message, const std::string &addres, const std::string &signature) const = 0;

    /*!
     * \brief signMultisigParticipant   signs given message with the multisig public signer key
     * \param message                   message to sign
     * \return                          signature in case of success. Sets status to Error and return empty string in case of error
     */
    virtual std::string signMultisigParticipant(const std::string &message) const = 0;
    /*!
     * \brief verifyMessageWithPublicKey verifies that message was signed with the given public key
     * \param message                    message
     * \param publicKey                  hex encoded public key
     * \param signature                  signature of the message
     * \return                           true if the signature is correct. false and sets error state in case of error
     */
    virtual bool verifyMessageWithPublicKey(const std::string &message, const std::string &publicKey, const std::string &signature) const = 0;

    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error) = 0;
    virtual std::string make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error) const = 0;

    virtual std::string getDefaultDataDir() const = 0;
   
   /*
    * \brief rescanSpent - Rescan spent outputs - Can only be used with trusted daemon
    * \return true on success
    */
    virtual bool rescanSpent() = 0;

   /*
    * \brief setOffline - toggle set offline on/off
    * \param offline - true/false
    */
    virtual void setOffline(bool offline) = 0;
    virtual bool isOffline() const = 0;
    
    //! blackballs a set of outputs
    virtual bool blackballOutputs(const std::vector<std::string> &outputs, bool add) = 0;

    //! blackballs an output
    virtual bool blackballOutput(const std::string &amount, const std::string &offset) = 0;

    //! unblackballs an output
    virtual bool unblackballOutput(const std::string &amount, const std::string &offset) = 0;

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

    //! locks/unlocks the keys file; returns true on success
    virtual bool lockKeysFile() = 0;
    virtual bool unlockKeysFile() = 0;
    //! returns true if the keys file is locked
    virtual bool isKeysFileLocked() = 0;

    /*!
     * \brief Queries backing device for wallet keys
     * \return Device they are on
     */
    virtual Device getDeviceType() const = 0;

    //! cold-device protocol key image sync
    virtual uint64_t coldKeyImageSync(uint64_t &spent, uint64_t &unspent) = 0;

    //! shows address on device display
    virtual void deviceShowAddress(uint32_t accountIndex, uint32_t addressIndex, const std::string &paymentId) = 0;

    //! attempt to reconnect to hardware device
    virtual bool reconnectDevice() = 0;

    //! get bytes received
    virtual uint64_t getBytesReceived() = 0;

    //! get bytes sent
    virtual uint64_t getBytesSent() = 0;
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
     * \param  kdf_rounds     Number of rounds for key derivation function
     * \return                Wallet instance (Wallet::status() needs to be called to check if created successfully)
     */
    virtual Wallet * createWallet(const std::string &path, const std::string &password, const std::string &language, NetworkType nettype, uint64_t kdf_rounds = 1) = 0;
    Wallet * createWallet(const std::string &path, const std::string &password, const std::string &language, bool testnet = false)      // deprecated
    {
        return createWallet(path, password, language, testnet ? TESTNET : MAINNET);
    }

    /*!
     * \brief  Opens existing wallet
     * \param  path           Name of wallet file
     * \param  password       Password of wallet file
     * \param  nettype        Network type
     * \param  kdf_rounds     Number of rounds for key derivation function
     * \param  listener       Wallet listener to set to the wallet after creation
     * \return                Wallet instance (Wallet::status() needs to be called to check if opened successfully)
     */
    virtual Wallet * openWallet(const std::string &path, const std::string &password, NetworkType nettype, uint64_t kdf_rounds = 1, WalletListener * listener = nullptr) = 0;
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
     * \param  kdf_rounds     Number of rounds for key derivation function
     * \param  seed_offset    Seed offset passphrase (optional)
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &password, const std::string &mnemonic,
                                    NetworkType nettype = MAINNET, uint64_t restoreHeight = 0, uint64_t kdf_rounds = 1,
                                    const std::string &seed_offset = {}) = 0;
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
     * \param  kdf_rounds     Number of rounds for key derivation function
     * \return                Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * createWalletFromKeys(const std::string &path,
                                                    const std::string &password,
                                                    const std::string &language,
                                                    NetworkType nettype,
                                                    uint64_t restoreHeight,
                                                    const std::string &addressString,
                                                    const std::string &viewKeyString,
                                                    const std::string &spendKeyString = "",
                                                    uint64_t kdf_rounds = 1) = 0;
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
     * \param  kdf_rounds           Number of rounds for key derivation function
     * \param  listener             Wallet listener to set to the wallet after creation
     * \return                      Wallet instance (Wallet::status() needs to be called to check if recovered successfully)
     */
    virtual Wallet * createWalletFromDevice(const std::string &path,
                                            const std::string &password,
                                            NetworkType nettype,
                                            const std::string &deviceName,
                                            uint64_t restoreHeight = 0,
                                            const std::string &subaddressLookahead = "",
                                            uint64_t kdf_rounds = 1,
                                            WalletListener * listener = nullptr) = 0;

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
     * @param kdf_rounds - number of rounds for key derivation function
     * @return - true if password is correct
     *
     * @note
     * This function will fail when the wallet keys file is opened because the wallet program locks the keys file.
     * In this case, Wallet::unlockKeysFile() and Wallet::lockKeysFile() need to be called before and after the call to this function, respectively.
     */
    virtual bool verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool no_spend_key, uint64_t kdf_rounds = 1) const = 0;

    /*!
     * \brief determine the key storage for the specified wallet file
     * \param device_type     (OUT) wallet backend as enumerated in Wallet::Device
     * \param keys_file_name  Keys file to verify password for
     * \param password        Password to verify
     * \return                true if password correct, else false
     *
     * for verification only - determines key storage hardware
     *
     */
    virtual bool queryWalletDevice(Wallet::Device& device_type, const std::string &keys_file_name, const std::string &password, uint64_t kdf_rounds = 1) const = 0;

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
    static std::tuple<bool, std::string, std::string, std::string, std::string> checkUpdates(
        const std::string &software,
        std::string subdir,
        const char *buildtag = nullptr,
        const char *current_version = nullptr);

    //! sets proxy address, empty string to disable
    virtual bool setProxy(const std::string &address) = 0;
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
