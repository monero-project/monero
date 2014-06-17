#include "MoneroWallet.hh"

#include "MoneroWalletObserver.hh"

#include <math.h>


#include "wallet/wallet2.h"

using namespace Monero;


class WalletCallback : public tools::i_wallet2_callback {

public:
    WalletCallback(WalletObserver* pObserver) : observer(pObserver) {}

    virtual void on_new_block(uint64_t height, const cryptonote::block& block) {
        std::cout << "Impl observer : " << "on_new_block" << std::endl;

    }

    virtual void on_money_received(uint64_t height, const cryptonote::transaction& tx, size_t out_index) {
        std::cout << "Impl observer : " << "on_money_received" << std::endl;
        observer->on_money_received(height, tx.vout[out_index].amount);
    }

    virtual void on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx) {
        std::cout << "Impl observer : " << "on_money_spent" << std::endl;

    }

    virtual void on_skip_transaction(uint64_t height, const cryptonote::transaction& tx) {
        std::cout << "Impl observer : " << "on_skip_transaction" << std::endl;

    }

private:

    WalletObserver* observer;

};





amount_t fromMini(amount_mini_t pAmountMini) {
    return pAmountMini * pow(10,-12);
}

amount_mini_t toMini(amount_t pAmount) {
    return pAmount * pow(10,12);
}




static amount_mini_t default_fee = DEFAULT_FEE;




Wallet::Wallet(const std::string& pWalletFile, const std::string& pWalletPassword)
    : wallet_impl(new tools::wallet2()), observer(NULL)
{
    // default_fee = DEFAULT_FEE;

    try {
        wallet_impl->load(pWalletFile, pWalletPassword);
    }
    catch(tools::error::invalid_password) {
        throw(Errors::iInvalidPassword);
    }
    catch(tools::error::file_error_base<1>) {
        throw(Errors::iInvalidFile);
    }

    std::cout << "Default FEE : " << default_fee << std::endl;
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

const std::vector<Transfer> Wallet::getIncomingTransfers() const 
{

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

bool Wallet::refresh() {

    size_t lFetchedBlocks;
    bool lHasReceivedMoney;
    bool lIsOk;

    wallet_impl->refresh(lFetchedBlocks, lHasReceivedMoney, lIsOk);

    std::cout << "Fetched : " << lFetchedBlocks << std::endl;
    std::cout << "Has Received Moneey : " << lHasReceivedMoney << std::endl;
    std::cout << "Is Ok : " << lIsOk << std::endl;

    return lIsOk;
}


bool Wallet::connect(const std::string pDaemonRPCEndpoint) {

    wallet_impl->init(pDaemonRPCEndpoint);
    return true;

}

// bool transferMini(const std::string& pRecipient, amount_mini_t pAmountMini, pFee = DEFAULT_FEE, std::string& pPaymentId = "" ) 
// {

// }

bool Wallet::transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, amount_mini_t pFee, const std::string& pPaymentId) 
{
    std::vector<cryptonote::tx_destination_entry> lDestinations;
    for(std::pair<std::string,amount_mini_t> lDestAmountPair : pDestsToAmountMini) {
        cryptonote::tx_destination_entry lDestEntry;
        if(!get_account_address_from_str(lDestEntry.addr, lDestAmountPair.first)) {
            // TODO throw
            return false;
        }
        lDestEntry.amount = lDestAmountPair.second;
        
        lDestinations.push_back(lDestEntry);
    }



    std::string lExtraNonce;


    if (!pPaymentId.empty()) {

        crypto::hash lPaymentIdBytes;

        /* Parse payment ID */
        if (!tools::wallet2::parse_payment_id(pPaymentId, lPaymentIdBytes)) {
            // TODO throw
            std::cout << "Should throw : parse_payment_id error" << std::endl;
            return false;
        }
        cryptonote::set_payment_id_to_tx_extra_nonce(lExtraNonce, lPaymentIdBytes);

    }

    std::vector<uint8_t> lExtra;
    /* Append Payment ID data into extra */
    if (!cryptonote::add_extra_nonce_to_tx_extra(lExtra, lExtraNonce)) {
        // TODO throw
        std::cout << "Should throw : add_extra_nonce_to_tx_extra error" << std::endl;
        return false;
    }


    std::cout << "Commiting transfer ..." << std::endl;

    cryptonote::transaction lTransaction;
    wallet_impl->transfer(lDestinations, 0, 0ULL, pFee, lExtra, lTransaction);

    return true;

}



void Wallet::setObserver(WalletObserver* pObserver) {
    WalletCallback* lWalletCallbackImpl = new WalletCallback(pObserver);

    /* Binds to wallet2 */
    wallet_impl->callback(lWalletCallbackImpl);

    observer = pObserver;
}


/**********/
/* STATIC */
/**********/

amount_mini_t Wallet::getDefaultFee() {
    return default_fee;
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