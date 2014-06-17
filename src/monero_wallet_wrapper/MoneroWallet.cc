#include "MoneroWallet.hh"


#include <math.h>


#include "wallet/wallet2.h"

using namespace Monero;

amount_t fromMini(amount_mini_t pAmountMini) {
    return pAmountMini * pow(10,-12);
}

amount_mini_t toMini(amount_t pAmount) {
    return pAmount * pow(10,12);
}

Wallet::Wallet(const std::string& pWalletFile, const std::string& pWalletPassword)
    : wallet_impl(new tools::wallet2())
{

    try {
        wallet_impl->load(pWalletFile, pWalletPassword);
    }
    catch(tools::error::invalid_password) {
        throw(Errors::iInvalidPassword);
    }
    catch(tools::error::file_error_base<1>) {
        throw(Errors::iInvalidFile);
    }
    // catch(tools::error::file_read_error) {
    //     throw(Errors::iInvalidFile);
    // }


}

Wallet::Wallet(tools::wallet2* pWalletImpl) 
    : wallet_impl(pWalletImpl) 
{

}

Wallet::~Wallet() {
    wallet_impl->store();
    delete wallet_impl;
}

const std::string Wallet::getAddress() const {

    return wallet_impl->get_account().get_public_address_str();

}

amount_mini_t Wallet::getBalanceMini() const {
    return wallet_impl->balance();
}

amount_mini_t Wallet::getUnlockedBalanceMini() const {
    return wallet_impl->unlocked_balance();

}

amount_t Wallet::getBalance() const {
    return fromMini(getBalanceMini());
}

amount_t Wallet::getUnlockedBalance() const {
    return fromMini(getUnlockedBalanceMini());
}

const std::vector<Transfer> Wallet::getIncomingTransfers() const {

    std::vector<Transfer> lTransfers;

    tools::wallet2::transfer_container lIncomingTransfers;
    

    /* TODO : Throw exception */
    wallet_impl->get_transfers(lIncomingTransfers);


    for(tools::wallet2::transfer_details lTransferDetail : lIncomingTransfers) {
        Transfer lTransfer;
        lTransfer.block_height = lTransferDetail.m_block_height;
        lTransfer.global_output_index = lTransferDetail.m_global_output_index;
        lTransfer.local_output_index = lTransferDetail.m_internal_output_index;
        lTransfer.spent = lTransferDetail.m_spent;
        lTransfer.amount_mini = lTransferDetail.amount();

        lTransfer.amount = fromMini(lTransfer.amount_mini);
        
        lTransfers.push_back(lTransfer);
    }

    return lTransfers;

}

bool Wallet::walletExists(const std::string pWalletFile, bool& oWallet_data_exists, bool& oWallet_keys_exist) {

    bool lDataExists = false;
    bool lKeysExists = false;
    tools::wallet2::wallet_exists(pWalletFile, lDataExists, lKeysExists);

    return lKeysExists;

}

Wallet Wallet::generateWallet(const std::string pWalletFile, const std::string& pWalletPassword) {

    try {
        tools::wallet2* lWalletImpl = new tools::wallet2();
        lWalletImpl->generate(pWalletFile, pWalletPassword);
        return Wallet(lWalletImpl);    
    }
    catch(tools::error::file_save_error) {
        throw(Errors::iNotWritableFile);
    }
    catch(tools::error::file_exists) {
        throw(Errors::iNotWritableFile);
    }

}