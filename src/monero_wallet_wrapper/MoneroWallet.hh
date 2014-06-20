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

typedef unsigned long long amount_mini_t;
typedef long double amount_t;


struct Transfer {
    std::string transaction_id;
    uint64_t block_height;
    uint64_t global_output_index;
    size_t local_output_index;
    bool spent;
    amount_mini_t amount_mini;
    amount_t amount;
};

struct Payment {
    std::string transaction_id;
    uint64_t block_height;
    uint64_t unlock_time;
    amount_mini_t amount_mini;
    amount_t amount;
};

enum GetIncomingTransfersFilter {
    NoFilter,
    AvailablesOnly,
    UnavailablesOnly
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
    * @brief Insanciates a new Wallet, using pWalletFile as wallet data and pWalletPassword as wallet password.
    *
    * Will throw an excpetion if pWalletFile is not found or pWalletPassword is wrong.
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
    * @returns Address as string
    */
    const std::string getAddress() const;

    /**
    * @brief Gets total balance in Mini unit.
    * @returns Total balance in Mini unit.
    */
    amount_mini_t getBalanceMini() const;

    /**
    * @brief Gets unlocked balance in Mini unit.
    * @returns Total balance (spendable funds) in Mini unit.
    */
    amount_mini_t getUnlockedBalanceMini() const;

    /**
    * @brief Gets total balance in Monero unit.
    * @returns Total balance in Monero unit.
    */
    amount_t getBalance() const;

    /**
    * @brief Gets unlocked balance in Monero unit.
    * @returns Total balance (spendable funds) in Monero unit.
    */
    amount_t getUnlockedBalance() const;

    /**
    * @brief Returns a list of incoming transfers
    *
    */
    const std::vector<Transfer> getIncomingTransfers(GetIncomingTransfersFilter pFilter = GetIncomingTransfersFilter::NoFilter);

    const std::list<Payment> getPayments(const std::string& pPaymentId) const;

    const std::multimap<std::string,Payment> getAllPayments();

    /**
    * @brief Performs a transfer to one of multiple destinations
    *
    * @param pDestsToAmountMini Multimap referencing destinations addresses (as std::string) with their respective amount (in Mini)
    * @param pFakeOutputCount Number of fake outputs to simulate : The more outputs, the more intraceable the transfer.
    * @param pUnlockTime TODO (unlocked balance delay)
    * @param pFee Fee to be applied for this transfer. Fees are sent to the network and indirectly to block finders (miners). You should NOT try to use a fee below value returned by 'getDefaultFee()'.
    * @param pPaymentID 32 bits Hex string representing a payment ID (TODO: create a generator). Payment ID can be used by services/exchanges to authenticate user deposits.
    */
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee = Wallet::getDefaultFee(), const std::string& pPaymentId = "");
    
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, const std::string& pPaymentId = "");
    const std::string transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, amount_mini_t pFee, const std::string& pPaymentId = "");

    const std::string transferMini(const std::string& pDestAddress, amount_mini_t pAmount, const std::string& pPaymentId = "");
    const std::string transferMini(const std::string& pDestAddress, amount_mini_t pAmount, amount_mini_t pFee, const std::string& pPaymentId = "");

    /**
    * @brief Performs a transfer to one destination
    * 
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Monero
    * @param pFee Fee to apply for the transaction, in Monero
    * @param pPayment Choosen payment ID of the transaction
    */
    const std::string transfer(const std::string& pDestAddress, amount_t pAmount, amount_t pFee, const std::string& pPaymentId = "");
    
    /**
    * @brief Performs a transfer to one destination.
    * Default network fee applies.
    * 
    * @param pDestAddress String representation of destination address.
    * @param pAmount Amount to send in Monero
    * @param pPayment Choosen payment ID of the transaction
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
    * 
    * @param pWalletFile File path of the Wallet data file.
    */
    static bool walletExists(const std::string pWalletFile, bool& oWallet_data_exists, bool& oWallet_keys_exist);
    
    /**
    * @brief Generates a new Wallet
    *
    * @param pWalletFile File path of the Wallet to be created.
    * @param pWalletPassword Password of the Wallet to be created.
    *
    */
    static Wallet* generateWallet(const std::string pWalletFile, const std::string& pWalletPassword);

    /**
    * @brief Get gets the default (a minimal) fee in Mini 
    *
    * @returns  The default fee, in Mini.
    */
    static amount_mini_t getDefaultFee();

    static size_t getDefaultFakeOutputCount();

    static uint64_t getDefaultUnlockTime();

private:
    tools::wallet2* wallet_impl;
    WalletObserver* observer;

    std::multimap<std::string,Payment> payments_cache;
    std::vector<Transfer> transfers_cache;

    const std::string doTransferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee, const std::string& pPaymentId);

    const std::vector<Transfer> fitlerTransfers(const std::vector<Transfer>& pTransfers, GetIncomingTransfersFilter pFilter);


    Wallet(tools::wallet2* pWalletImpl);


    /* Mutexes (doesn't guarantee thread safety) */
    std::mutex transfer_mutex;
    std::mutex refresh_mutex;
    std::mutex store_mutex;
};



}