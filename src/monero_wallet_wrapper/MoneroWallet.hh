#pragma once

#include <string>
#include <vector>
#include <inttypes.h>
#include <map>
#include <list>
#include <mutex>

#include "MoneroErrors.hh"

namespace tools { class wallet2; }

namespace Monero {

class WalletObserver;

/**
* @typedef amount_mini_t Represents an amount in Mini (smallest unit of Monero : 0.000000000001 Monero)
*/
typedef unsigned long long amount_mini_t;

/**
* @typedef amount_t Represents an amount in Monero (human unit of Monero : 1000000000000 Mini)
*/
typedef long double amount_t;


/**
* @struct Transfer 
*/
struct Transfer {
    std::string transaction_id;
    uint64_t block_height;
    uint64_t global_output_index;
    size_t local_output_index;
    bool spent;
    amount_mini_t amount_mini;
    amount_t amount;
};

/**
* @struct Payment 
*/
struct Payment {
    std::string transaction_id;
    uint64_t block_height;
    uint64_t unlock_time;
    amount_mini_t amount_mini;
    amount_t amount;
};

/*! Filter types for getIncomingTransfers method */
enum GetIncomingTransfersFilter {
    NoFilter,           /*!< All transfers */
    AvailablesOnly,     /*!< Unspent transfers */
    UnavailablesOnly    /*!< Spent transfers */
};

/**
* @class Wallet Represents a Wallet instance. Can also be used with static methods for checking Wallets.
*
* In this class, we distinguish two types of "Monero" units : 
*    - Mini (amount_mini_t) : This is the minimal unit possible in Monero. Most internal function in Monero Core uses this unit.
*    - Monero (amount_t) : This is the human-readable value of Monero. Alias : XMR. Reprensented as a double with value 10^-12 * mini
*
* Examples :
*    - 123684360000 Mini =  1.2368436 Monero
*    - 500000000 Mini = 0.005 Monero (default fee in 06.18/14)
*
*
* In this class, most methods return information from the Wallet data file.
* Information will not change until "refresh()" command is executed in order to synchronize the Wallet.
*
* Following methods requires Daemon RPC conection and will throw execption if connection is not configured :
* - Refresh
* - Transfer
*
*/
class Wallet {
    
public:
    /**
    * @brief Opens an existing Wallet, using pWalletFile as wallet data and pWalletPassword as wallet password.
    * @param pWalletFile Data file associated with the Wallet.
    * @param pWalletPassword Password of the wallet.
    *
    * Will throw an excpetion if pWalletFile is not found or pWalletPassword is wrong.
    *
    * @throws Errors::InvalidPassword
    * @throws Errors::InvalidFile
    * @throws Errors::NotMatchingDataKeys
    */
    Wallet(const std::string& pWalletFile, const std::string& pWalletPassword);

    /**
    * @brief Destroys the Wallet. Can cause Universe destruction (please use bug reports).
    */
    ~Wallet();

    /**
    * @brief Set up connection to Daemon RPC.
    * @param pDaemonRPCEndpoint URL and port of the Daemon RPC server (bitmonerod)
    */
    bool connect(const std::string pDaemonRPCEndpoint = "http://localhost:18081");

    /**
    * @brief Writes the Wallet to its data file
    *
    */
    void store();

    /* Offline methods */
    /**
    * @brîef Returns the address of the Wakket account.
    * @return Address as string
    */
    const std::string getAddress() const;

    /**
    * @brief Experimental : Concatenates your address and previously generated payment id for unified displaying
    * The "paymentAddress" could be used for using implicitly a combined address and payment ID
    *
    * @throw Errors::InvalidAddress
    * @throw Errors::InvalidPaymentID
    */
    const std::string getPaymentAddress(const std::string& pPaymentId) const;

    /**
    * @brief Gets total balance in Mini unit.
    * @return Total balance in Mini unit.
    */
    amount_mini_t getBalanceMini() const;

    /**
    * @brief Gets unlocked balance in Mini unit.
    * @return Total balance (spendable funds) in Mini unit.
    */
    amount_mini_t getUnlockedBalanceMini() const;

    /**
    * @brief Gets total balance in Monero unit.
    * @return Total balance in Monero unit.
    */
    amount_t getBalance() const;

    /**
    * @brief Gets unlocked balance in Monero unit.
    * @return Total balance (spendable funds) in Monero unit.
    */
    amount_t getUnlockedBalance() const;

    /**
    * @brief Returns a list of incoming transfers
    * @param pFilter Optional filter (currently filtering spent or unspent transfers)
    *
    * @return A vector containing a copy of all filter-compliant Transfer
    */
    const std::vector<Transfer> getIncomingTransfers(GetIncomingTransfersFilter pFilter = GetIncomingTransfersFilter::NoFilter);

    /**
    * @brief Returns a list of received payments associated with the ID
    * @param pPaymentId ID of the payment. A Payment ID can be user in 'transfer' methods in order to 'deanonymize' an unique transaction. This method is used in services/exchanges.
    *
    * @return A list of Payment associated with the ID
    *
    * @throws Errors::InvalidPaymentID
    */
    const std::list<Payment> getPayments(const std::string& pPaymentId) const;

    /**
    * @brief Returns a map of all received payments
    *
    * @return A PaymentID-indexed map of all received payments
    */
    const std::multimap<std::string,Payment> getAllPayments();

    /**
    * @brief Performs a transfer to one or multiple destinations
    *
    * @param pDestsToAmountMini Multimap referencing destinations addresses (as std::string) with their respective amount (in Mini)
    * @param pFakeOutputCount Number of fake outputs to simulate : The more outputs, the more intraceable the transfer.
    * @param pUnlockTime TODO (unlocked balance delay)
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee = Wallet::getDefaultFeeMini(), const std::string& pPaymentId = "");
    
    /**
    * @brief Performs a transfer to one or multiple destinations
    *
    * @param pDestsToAmountMini Multimap referencing destinations addresses (as std::string) with their respective amount (in Mini)
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one or multiple destinations
    *
    * @param pDestsToAmountMini Multimap referencing destinations addresses (as std::string) with their respective amount (in Mini)
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, amount_mini_t pFee, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one destination
    *
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Mini
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::string& pDestAddress, amount_mini_t pAmount, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one destination
    *
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Mini
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::string& pDestAddress, amount_mini_t pAmount, amount_mini_t pFee, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one or multiple destinations
    *
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Mini
    * @param pFakeOutputCount Number of fake outputs to simulate : The more outputs, the more intraceable the transfer.
    * @param pUnlockTime TODO (unlocked balance delay)
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transferMini(const std::string& pDestAddress, amount_mini_t pAmount, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee = Wallet::getDefaultFeeMini(), const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one destination
    *
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Monero
    * @param pUnlockTime TODO (unlocked balance delay)
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transfer(const std::string& pDestAddress, amount_t pAmount, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_t pFee, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one destination
    *
    * @param pDestAddress String representation of destination address.
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFeeMini()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transfer(const std::string& pDestAddress, amount_t pAmount, amount_t pFee, const std::string& pPaymentId = "");
    
    /**
    * @brief Performs a transfer to one destination
    *
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Monero
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    *
    * @return Transaction ID of the transfer
    *
    * @throws Errors::NoDaemonConnection
    * @throws Errors::DaemonBusy
    * @throws Errors::TransactionTooBig
    * @throws Errors::TransactionZeroDestination
    * @throws Errors::TransactionSumOverflow
    * @throws Errors::TransactionNotEnoughMoney
    * @throws Errors::TransactionUnexpectedType
    * @throws Errors::WalletInternalError
    * @throws Errors::TransactionNotEnoughOuts
    * @throws Errors::TransactionRejected
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidNonce
    * @trhows Errors::InvalidPaymentID
    */
    const std::string transfer(const std::string& pDestAddress, amount_t pAmount, const std::string& pPaymentId = "");

    /**
    * @brief Refreshes the Wallet  using Daemon RPC endpoint.
    * This method will trigger Observers callback if they apply.
    */
    bool refresh();

    /**
    * @brief Set the observer of this Wallet instance.
    * @param pObserver Observer to be assigned.
    */
    void setObserver(WalletObserver* pObserver);


    /**
    * @brief Checks if the given Wallet file exists
    * @param pWalletFile File path of the Wallet data file.
    *
    * @return true if the given wallet keys exist, false otherwise
    */
    static bool walletExists(const std::string pWalletFile, bool& oWallet_data_exists, bool& oWallet_keys_exist);
    
    /**
    * @brief Generates a new Wallet
    *
    * @param pWalletFile File path of the Wallet to be created.
    * @param pWalletPassword New password of the Wallet to be created.
    *
    * @return Recovery seed of the Wallet. Save it in a SAFE PLACE (your brain is safe) for being able to recover your Wallet.
    *
    * @throws Errors::NotWritableFile
    */
    static const std::string generateWallet(const std::string pWalletFile, const std::string& pWalletPassword, bool pDeterministic = true);

    /**
    * @brief Recover an existing Wallet. Useful if you lose your wallet keys (<wallet_name>.keys)
    *
    * @param pWalletFile File path of the Wallet to be created.
    * @param pWalletPassword NEW password of the Wallet to be recovered.
    * @param pSeed Previously generated Electrum-style seed
    *
    * @return Recovery seed of the Wallet. Save it in a SAFE PLACE (your brain is safe) for being able to recover your Wallet.
    *
    * @throws Errors::InvalidSeed
    * @throws Errors::NotWritableFile
    */
    static const std::string recoverWallet(const std::string pWalletFile, const std::string& pWalletPassword, const std::string& pSeed);

    /**
    * @brief Converts Mini amount to Monero amount
    * @param pAmountMini Amount in Mini (smallest possible unit)
    *
    * @return Monero amount
    */
    static amount_t miniToMonero(amount_mini_t pAmountMini);

    /**
    * @brief Converts Monero amount to Mini amount
    * @param pAmount Amount in Monero (currently 1000000000000 Mini)
    *
    * @return Mini amount
    */
    static amount_mini_t moneroToMini(amount_t pAmount);

    /**
    * @brief Generates a new payment ID.
    * You should check duplicates if you are generating many Payment ID (if you find duplicate, please play lottery)
    *
    * @return A generated 64 hex-char payment ID.
    */
    static const std::string generatePaymentId();

    /**
    * @brief Get gets the default (a minimal) fee in Mini 
    *
    * @returns  The default fee, in Mini.
    */
    static amount_mini_t getDefaultFeeMini();

    static size_t getDefaultFakeOutputCount();

    static uint64_t getDefaultUnlockTime();

    /**
    * @brief Experimental : Concatenates address and payment id for unified displaying
    *
    * @throws Errors::InvalidAddress
    * @throws Errors::InvalidPaymentID
    */
    static const std::string concatenatePaymentAddress(const std::string& pAddress, const std::string& pPaymentId);

    /**
    * @brief Experimental : Extracts Address and Payment ID from PaymentAddress string
    *
    * @throws Errors::InvalidPaymentAddress
    *
    */
    static void extractPaymentAndAddress(const std::string& pPaymentAddress, std::string& oAddress, std::string& oPaymentId);


    static bool isValidAddress(const std::string& pAddress);
    static bool isValidPaymentId(const std::string& pPaymentId);
    // static bool isValidPaymentAddress(const std::string& pPaymentAddress);

private:
    tools::wallet2* wallet_impl;
    WalletObserver* observer;

    std::multimap<std::string,Payment> payments_cache;
    std::vector<Transfer> transfers_cache;

    const std::string doTransferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee, const std::string& pPaymentId);

    static const std::vector<Transfer> fitlerTransfers(const std::vector<Transfer>& pTransfers, GetIncomingTransfersFilter pFilter);


    Wallet(tools::wallet2* pWalletImpl);


    /* Mutexes (doesn't guarantee thread safety) */
    std::mutex transfer_mutex;
    std::mutex refresh_mutex;
    std::mutex store_mutex;
};



}