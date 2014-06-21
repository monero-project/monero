#include "MoneroWallet.hh"

#include "MoneroWalletObserver.hh"

#include <math.h>


#include "wallet/wallet2.h"

using namespace Monero;


amount_t fromMini(amount_mini_t pAmountMini) {
    return pAmountMini * pow(10,-12);
}

amount_mini_t toMini(amount_t pAmount) {
    return pAmount * pow(10,12);
}

const Transfer transferFromRawTransferDetails(const tools::wallet2::transfer_details& pTransferDetails)
{
    Transfer lTransfer;
    lTransfer.transaction_id = epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(pTransferDetails.m_tx));
    lTransfer.block_height = pTransferDetails.m_block_height;
    lTransfer.global_output_index = pTransferDetails.m_global_output_index;
    lTransfer.local_output_index = pTransferDetails.m_internal_output_index;
    lTransfer.spent = pTransferDetails.m_spent;
    lTransfer.amount_mini = pTransferDetails.amount();
    lTransfer.amount = fromMini(lTransfer.amount_mini);

    return lTransfer;
}

const Payment paymentFromRawPaymentDetails(const tools::wallet2::payment_details& pPaymentDetails)
{
    Payment lPayment;
    lPayment.transaction_id = epee::string_tools::pod_to_hex(pPaymentDetails.m_tx_hash);
    lPayment.block_height = pPaymentDetails.m_block_height;
    lPayment.unlock_time = pPaymentDetails.m_unlock_time;
    lPayment.amount_mini = pPaymentDetails.m_amount;
    lPayment.amount = fromMini(lPayment.amount_mini);

    return lPayment;
}



class WalletCallback : public tools::i_wallet2_callback {

public:
    WalletCallback(WalletObserver* pObserver) : observer(pObserver) {}

    virtual void on_new_block(uint64_t height, const cryptonote::block& block) {
        // std::cout << "Impl observer : " << "on_new_block" << std::endl;
        observer->on_new_block(height);
    }

    virtual void on_money_received(uint64_t height, const cryptonote::transaction& tx, size_t out_index) {
        observer->on_money_received(height, epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(tx)), tx.vout[out_index].amount);
    }

    virtual void on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx) {
        // std::cout << "Impl observer : " << "on_money_spent" << std::endl;
        observer->on_money_spent(height, epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(spend_tx)), in_tx.vout[out_index].amount);

    }

    virtual void on_skip_transaction(uint64_t height, const cryptonote::transaction& tx) {
        observer->on_skip_transaction(height, epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(tx)));
    }

    virtual void on_payment_received(uint64_t height, const crypto::hash payment_id, const tools::wallet2::payment_details& payment) {
        observer->on_payment_received(height, epee::string_tools::pod_to_hex(payment_id), paymentFromRawPaymentDetails(payment));
    }

private:

    WalletObserver* observer;

};



static amount_mini_t default_fee = DEFAULT_FEE;




Wallet::Wallet(const std::string& pWalletFile, const std::string& pWalletPassword)
    : wallet_impl(new tools::wallet2()), observer(NULL)
{

    try {
        wallet_impl->load(pWalletFile, pWalletPassword);
    }
    catch(tools::error::invalid_password) {
        throw(Errors::InvalidPassword());
    }
    catch(tools::error::file_error_base<1>) {
        throw(Errors::InvalidFile());
    }

}

Wallet::Wallet(tools::wallet2* pWalletImpl) 
    : wallet_impl(pWalletImpl), observer(NULL)
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

const std::vector<Transfer> Wallet::getIncomingTransfers(GetIncomingTransfersFilter pFilter) 
{

    if (transfers_cache.size() != wallet_impl->get_transfers_count()) {
        
        /* TODO : Throw exception */
        tools::wallet2::transfer_container lIncomingTransfers;   
        wallet_impl->get_transfers(lIncomingTransfers);

        std::vector<Transfer> lTransfers;
        for(tools::wallet2::transfer_details lTransferDetail : lIncomingTransfers) {
            const Transfer& lTransfer = transferFromRawTransferDetails(lTransferDetail);
            lTransfers.push_back(lTransfer);
        }

        transfers_cache = lTransfers;

    }

    if ( pFilter != GetIncomingTransfersFilter::NoFilter) {
        return fitlerTransfers(transfers_cache, pFilter);
    }

    return transfers_cache;

}

const std::list<Payment> Wallet::getPayments(const std::string& pPaymentId) const 
{

    crypto::hash lPaymentIdBytes;

    /* Parse payment ID */
    if (!tools::wallet2::parse_payment_id(pPaymentId, lPaymentIdBytes)) {
        throw(Errors::InvalidPaymentID());
    }

    std::list<tools::wallet2::payment_details> oPaymentsDetails;
    wallet_impl->get_payments(lPaymentIdBytes, oPaymentsDetails);

    std::list<Payment> lPayments;
    for (const tools::wallet2::payment_details lPaymentDetails : oPaymentsDetails) {
        const Payment& lPayment = paymentFromRawPaymentDetails(lPaymentDetails);
        lPayments.push_back(lPayment);
    }

    return lPayments;
}

const std::multimap<std::string,Payment> Wallet::getAllPayments() 
{

    if (payments_cache.size() != wallet_impl->get_payments_count()) {

        std::multimap<std::string,Payment> lPaymentsMap;
        const tools::wallet2::payment_container& lPaymentsContainer = wallet_impl->get_all_payments();

        for (const std::pair<crypto::hash,tools::wallet2::payment_details> lHashPaymentDetails : lPaymentsContainer) {
            const std::string& lPaymentId = epee::string_tools::pod_to_hex(lHashPaymentDetails.first);
            const Payment& lPayment = paymentFromRawPaymentDetails(lHashPaymentDetails.second);
            lPaymentsMap.emplace(lPaymentId, lPayment);
        }

        payments_cache = lPaymentsMap;
        
    }

    return payments_cache;

}

bool Wallet::refresh() {

    refresh_mutex.lock();

    size_t lFetchedBlocks;
    bool lHasReceivedMoney;
    bool lIsOk;

    wallet_impl->refresh(lFetchedBlocks, lHasReceivedMoney, lIsOk);

    refresh_mutex.unlock();

    if (lIsOk && observer) {
        observer->on_wallet_refreshed(this, lFetchedBlocks, lHasReceivedMoney);
    }

    return lIsOk;
}


bool Wallet::connect(const std::string pDaemonRPCEndpoint) 
{
    wallet_impl->init(pDaemonRPCEndpoint);
    return true;
}

void Wallet::store() 
{
    std::lock_guard<std::mutex> lLockStore(store_mutex);

    wallet_impl->store();
}

const std::string Wallet::transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, const std::string& pPaymentId) 
{
    return transferMini(pDestsToAmountMini, getDefaultFee(), pPaymentId);
}

const std::string Wallet::transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, amount_mini_t pFee, const std::string& pPaymentId) 
{
    return transferMini(pDestsToAmountMini, 0, 0, pFee, pPaymentId);
}

const std::string Wallet::transferMini(const std::string& pDestAddress, amount_mini_t pAmount, const std::string& pPaymentId) 
{
    return transferMini(pDestAddress, pAmount, getDefaultFee(), pPaymentId);
}

const std::string Wallet::transferMini(const std::string& pDestAddress, amount_mini_t pAmount, amount_mini_t pFee, const std::string& pPaymentId) 
{
    std::multimap<std::string,amount_mini_t> lDestsMap;
    lDestsMap.emplace(pDestAddress, pAmount);
    return transferMini(lDestsMap, pFee, pPaymentId);
}


const std::string Wallet::transfer(const std::string& pDestAddress, amount_t pAmount, const std::string& pPaymentId) 
{
    return transferMini(pDestAddress, toMini(pAmount), pPaymentId);
}


const std::string Wallet::transfer(const std::string& pDestAddress, amount_t pAmount, amount_t pFee, const std::string& pPaymentId) 
{
    return transferMini(pDestAddress, toMini(pAmount), toMini(pFee), pPaymentId);
}


const std::string Wallet::transferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee, const std::string& pPaymentId) 
{
    /* All "transfer" calls pass in this blocl */
    std::lock_guard<std::mutex> lTransferLock(transfer_mutex);

    return doTransferMini(pDestsToAmountMini, pFakeOutputsCount, pUnlockTime, pFee, pPaymentId);
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


static size_t default_fake_output_count = 0;

static uint64_t default_unlock_time = 0;


amount_mini_t Wallet::getDefaultFee() {
    return default_fee;
}

size_t Wallet::getDefaultFakeOutputCount() {
    return default_fake_output_count;
}

uint64_t Wallet::getDefaultUnlockTime() {
    return default_unlock_time;
}

bool Wallet::walletExists(const std::string pWalletFile, bool& oWallet_data_exists, bool& oWallet_keys_exist) {

    bool lDataExists = false;
    bool lKeysExists = false;
    tools::wallet2::wallet_exists(pWalletFile, lDataExists, lKeysExists);

    return lKeysExists;

}

Wallet* Wallet::generateWallet(const std::string pWalletFile, const std::string& pWalletPassword) {

    try {
        tools::wallet2* lWalletImpl = new tools::wallet2();
        lWalletImpl->generate(pWalletFile, pWalletPassword);
        return new Wallet(lWalletImpl);    
    }
    catch(tools::error::file_save_error) {
        throw(Errors::NotWritableFile());
    }
    catch(tools::error::file_exists) {
        throw(Errors::NotWritableFile());
    }

}


const std::string Wallet::doTransferMini(const std::multimap<std::string,amount_mini_t> pDestsToAmountMini, size_t pFakeOutputsCount, uint64_t pUnlockTime, amount_mini_t pFee, const std::string& pPaymentId) 
{

    std::vector<cryptonote::tx_destination_entry> lDestinations;
    for(std::pair<std::string,amount_mini_t> lDestAmountPair : pDestsToAmountMini) {
        cryptonote::tx_destination_entry lDestEntry;
        if(!get_account_address_from_str(lDestEntry.addr, lDestAmountPair.first)) {
            throw(Errors::InvalidAddress());
        }
        lDestEntry.amount = lDestAmountPair.second;
        
        lDestinations.push_back(lDestEntry);
    }


    std::string lExtraNonce;

    if (!pPaymentId.empty()) {

        crypto::hash lPaymentIdBytes;

        /* Parse payment ID */
        if (!tools::wallet2::parse_payment_id(pPaymentId, lPaymentIdBytes)) {
            throw(Errors::InvalidPaymentID());
        }

        cryptonote::set_payment_id_to_tx_extra_nonce(lExtraNonce, lPaymentIdBytes);
    }

    std::vector<uint8_t> lExtra;
    /* Append Payment ID data into extra */
    if (!cryptonote::add_extra_nonce_to_tx_extra(lExtra, lExtraNonce)) {
        // TODO throw
        throw(Errors::InvalidNonce());

    }
          
    cryptonote::transaction lTransaction;
    try {
        wallet_impl->transfer(lDestinations, pFakeOutputsCount, pUnlockTime, pFee, lExtra, lTransaction);
    }
    catch(tools::error::no_connection_to_daemon) {
        throw(Errors::NoDaemonConnection());
    }
    catch(tools::error::daemon_busy) {
        throw(Errors::DaemonBusy());
    }
    catch(tools::error::tx_too_big) {
        throw(Errors::TransactionTooBig());
    }
    catch(tools::error::zero_destination) {
        throw(Errors::TransactionZeroDestination());
    }
    catch(tools::error::tx_sum_overflow) {
        throw(Errors::TransactionSumOverflow());
    }
    catch(tools::error::not_enough_money) {
        throw(Errors::TransactionNotEnoughMoney());
    }
    catch(tools::error::unexpected_txin_type) {
        throw(Errors::TransactionUnexpectedType());
    }
    catch(tools::error::wallet_internal_error) {
        throw(Errors::WalletInternalError());
    }
    catch(tools::error::not_enough_outs_to_mix) {
        throw(Errors::TransactionNotEnoughOuts());
    }
    catch(tools::error::tx_rejected) {
        throw(Errors::TransactionRejected());
    }

    const std::string& lTransactionId = boost::lexical_cast<std::string>(get_transaction_hash(lTransaction));

    return lTransactionId;

}


bool canTransferPassFilter(const Transfer& pTransfer, GetIncomingTransfersFilter pFilter) {

    switch(pFilter) {
        case GetIncomingTransfersFilter::AvailablesOnly:
            return !pTransfer.spent;
        case GetIncomingTransfersFilter::UnavailablesOnly:        
            return pTransfer.spent;
        default:
            return true;
    }

}

const std::vector<Transfer> Wallet::fitlerTransfers(const std::vector<Transfer>& pTransfers, GetIncomingTransfersFilter pFilter) {

    std::vector<Transfer> lFilteredTransfers;
    for (const Transfer& lTransfer : pTransfers) {

        if (canTransferPassFilter(lTransfer, pFilter)) {
            lFilteredTransfers.push_back(lTransfer);
        }

    }

    return lFilteredTransfers;

}