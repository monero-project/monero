#pragma once

#include <string>
#include <vector>
#include <inttypes.h>
#include <map>

namespace tools { class wallet2; }

namespace Monero {

class WalletObserver;
// class WalletCallback;


typedef unsigned long long amount_mini_t;
typedef long double amount_t;

namespace Errors {

    class InvalidPassword: public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
        return "Wrong wallet password";
        }
    } iInvalidPassword;

    class InvalidFile: public std::exception
    {
    public: 
        virtual const char* what() const throw()
        {
        return "Wallet file not found or invalid";
        }
    } iInvalidFile;

    class NotWritableFile: public std::exception
    {
    public: 
        virtual const char* what() const throw()
        {
        return "Unable to write file";
        }

    } iNotWritableFile;

}

struct Transfer {
    uint64_t block_height;
    uint64_t global_output_index;
    size_t local_output_index;
    bool spent;
    amount_mini_t amount_mini;
    amount_t amount;
};

class Wallet {
    
public:
    Wallet(const std::string& pWalletFile, const std::string& pWalletPassword);
    ~Wallet();

    bool connect(const std::string pDaemonRPCEndpoint = "http://localhost:18081");

    /* Offline methods */
    const std::string getAddress() const;

    amount_mini_t getBalanceMini() const;
    amount_mini_t getUnlockedBalanceMini() const;

    amount_t getBalance() const;
    amount_t getUnlockedBalance() const;

    const std::vector<Transfer> getIncomingTransfers() const;

    /**/


    // bool transferMini(const std::string& pRecipient, amount_mini_t pAmountMini, pFee = DEFAULT_FEE, std::string& pPaymentId = "" );
    bool transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, amount_mini_t pFee = Wallet::getDefaultFee(), const std::string& pPaymentId = "");
    bool refresh();



    void setObserver(WalletObserver* pObserver);



    static bool walletExists(const std::string pWalletFile, bool& oWallet_data_exists, bool& oWallet_keys_exist);
    static Wallet generateWallet(const std::string pWalletFile, const std::string& pWalletPassword);

    static amount_mini_t getDefaultFee();


private:
    tools::wallet2* wallet_impl;
    // WalletCallback* wallet_callback_impl;
    WalletObserver* observer;

    Wallet(tools::wallet2* pWalletImpl);

};



}