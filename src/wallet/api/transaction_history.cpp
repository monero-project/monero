// Copyright (c) 2014-2024, The Monero Project
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


#include "transaction_history.h"
#include "transaction_info.h"
#include "wallet.h"

#include "crypto/hash.h"
#include "wallet/wallet2.h"


#include <string>
#include <list>

using namespace epee;

namespace Monero {

TransactionHistory::~TransactionHistory() {}


TransactionHistoryImpl::TransactionHistoryImpl(WalletImpl *wallet)
    : m_wallet(wallet)
{

}

TransactionHistoryImpl::~TransactionHistoryImpl()
{
    for (auto t : m_history)
        delete t;
}

int TransactionHistoryImpl::count() const
{
    boost::shared_lock<boost::shared_mutex> lock(m_historyMutex);
    int result = m_history.size();
    return result;
}

TransactionInfo *TransactionHistoryImpl::transaction(int index) const
{
    boost::shared_lock<boost::shared_mutex> lock(m_historyMutex);
    // sanity check
    if (index < 0)
        return nullptr;
    unsigned index_ = static_cast<unsigned>(index);
    return index_ < m_history.size() ? m_history[index_] : nullptr;
}

TransactionInfo *TransactionHistoryImpl::transaction(const std::string &id) const
{
    boost::shared_lock<boost::shared_mutex> lock(m_historyMutex);
    auto itr = std::find_if(m_history.begin(), m_history.end(),
                            [&](const TransactionInfo * ti) {
        return ti->hash() == id;
    });
    return itr != m_history.end() ? *itr : nullptr;
}

std::vector<TransactionInfo *> TransactionHistoryImpl::getAll() const
{
    boost::shared_lock<boost::shared_mutex> lock(m_historyMutex);
    return m_history;
}

void TransactionHistoryImpl::setTxNote(const std::string &txid, const std::string &note)
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
        return;
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    m_wallet->m_wallet->set_tx_note(htxid, note);
    refresh();
}

void TransactionHistoryImpl::refresh()
{
    // multithreaded access:
    // boost::lock_guard<boost::mutex> guarg(m_historyMutex);
    // for "write" access, locking exclusively
    boost::unique_lock<boost::shared_mutex> lock(m_historyMutex);

    // TODO: configurable values;
    uint64_t min_height = 0;
    uint64_t max_height = (uint64_t)-1;
    uint64_t wallet_height = m_wallet->blockChainHeight();

    // delete old transactions;
    for (auto t : m_history)
        delete t;
    m_history.clear();

    // transactions are stored in wallet2:
    // - confirmed_transfer_details   - out transfers
    // - unconfirmed_transfer_details - pending out transfers
    // - payment_details              - input transfers

    // payments are "input transactions";
    // one input transaction contains only one transfer. e.g. <transaction_id> - <100XMR>

    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> in_payments;
    m_wallet->m_wallet->get_payments(in_payments, min_height, max_height);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = in_payments.begin(); i != in_payments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
            payment_id = payment_id.substr(0,16);
        TransactionInfoImpl * ti = new TransactionInfoImpl();
        ti->m_paymentid = payment_id;
        ti->m_coinbase = pd.m_coinbase;
        ti->m_amount    = pd.m_amount;
        ti->m_fee       = pd.m_fee;
        ti->m_direction = TransactionInfo::Direction_In;
        ti->m_hash      = string_tools::pod_to_hex(pd.m_tx_hash);
        ti->m_blockheight = pd.m_block_height;
        ti->m_description = m_wallet->m_wallet->get_tx_note(pd.m_tx_hash);
        ti->m_subaddrIndex = { pd.m_subaddr_index.minor };
        ti->m_subaddrAccount = pd.m_subaddr_index.major;
        ti->m_label     = m_wallet->m_wallet->get_subaddress_label(pd.m_subaddr_index);
        ti->m_timestamp = pd.m_timestamp;
        ti->m_confirmations = (wallet_height > pd.m_block_height) ? wallet_height - pd.m_block_height : 0;
        ti->m_unlock_time = pd.m_unlock_time;
        m_history.push_back(ti);

    }

    // confirmed output transactions
    // one output transaction may contain more than one money transfer, e.g.
    // <transaction_id>:
    //    transfer1: 100XMR to <address_1>
    //    transfer2: 50XMR  to <address_2>
    //    fee: fee charged per transaction
    //

    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> out_payments;
    m_wallet->m_wallet->get_payments_out(out_payments, min_height, max_height);

    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = out_payments.begin();
         i != out_payments.end(); ++i) {
        
        const crypto::hash &hash = i->first;
        const tools::wallet2::confirmed_transfer_details &pd = i->second;
        
        uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
        uint64_t fee = pd.m_amount_in - pd.m_amount_out;
        

        std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
            payment_id = payment_id.substr(0,16);


        TransactionInfoImpl * ti = new TransactionInfoImpl();
        ti->m_paymentid = payment_id;
        ti->m_amount = pd.m_amount_in - change - fee;
        ti->m_fee    = fee;
        ti->m_direction = TransactionInfo::Direction_Out;
        ti->m_hash = string_tools::pod_to_hex(hash);
        ti->m_blockheight = pd.m_block_height;
        ti->m_description = m_wallet->m_wallet->get_tx_note(hash);
        ti->m_subaddrIndex = pd.m_subaddr_indices;
        ti->m_subaddrAccount = pd.m_subaddr_account;
        ti->m_label = pd.m_subaddr_indices.size() == 1 ? m_wallet->m_wallet->get_subaddress_label({pd.m_subaddr_account, *pd.m_subaddr_indices.begin()}) : "";
        ti->m_timestamp = pd.m_timestamp;
        ti->m_confirmations = (wallet_height > pd.m_block_height) ? wallet_height - pd.m_block_height : 0;

        // single output transaction might contain multiple transfers
        for (const auto &d: pd.m_dests) {
            ti->m_transfers.push_back({d.amount, d.address(m_wallet->m_wallet->nettype(), pd.m_payment_id)});
        }

        m_history.push_back(ti);
    }

    // unconfirmed output transactions
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments_out;
    m_wallet->m_wallet->get_unconfirmed_payments_out(upayments_out);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments_out.begin(); i != upayments_out.end(); ++i) {
        const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
        const crypto::hash &hash = i->first;
        uint64_t amount = pd.m_amount_in;
        uint64_t fee = amount - pd.m_amount_out;
        std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
            payment_id = payment_id.substr(0,16);
        bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;

        TransactionInfoImpl * ti = new TransactionInfoImpl();
        ti->m_paymentid = payment_id;
        ti->m_amount = amount - pd.m_change - fee;
        ti->m_fee    = fee;
        ti->m_direction = TransactionInfo::Direction_Out;
        ti->m_failed = is_failed;
        ti->m_pending = true;
        ti->m_hash = string_tools::pod_to_hex(hash);
        ti->m_description = m_wallet->m_wallet->get_tx_note(hash);
        ti->m_subaddrIndex = pd.m_subaddr_indices;
        ti->m_subaddrAccount = pd.m_subaddr_account;
        ti->m_label = pd.m_subaddr_indices.size() == 1 ? m_wallet->m_wallet->get_subaddress_label({pd.m_subaddr_account, *pd.m_subaddr_indices.begin()}) : "";
        ti->m_timestamp = pd.m_timestamp;
        ti->m_confirmations = 0;
        for (const auto &d : pd.m_dests)
        {
            ti->m_transfers.push_back({d.amount, d.address(m_wallet->m_wallet->nettype(), pd.m_payment_id)});
        }        
        m_history.push_back(ti);
    }
    
    
    // unconfirmed payments (tx pool)
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> upayments;
    m_wallet->m_wallet->get_unconfirmed_payments(upayments);
    for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second.m_pd;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
            payment_id = payment_id.substr(0,16);
        TransactionInfoImpl * ti = new TransactionInfoImpl();
        ti->m_paymentid = payment_id;
        ti->m_amount    = pd.m_amount;
        ti->m_direction = TransactionInfo::Direction_In;
        ti->m_hash      = string_tools::pod_to_hex(pd.m_tx_hash);
        ti->m_blockheight = pd.m_block_height;
        ti->m_description = m_wallet->m_wallet->get_tx_note(pd.m_tx_hash);
        ti->m_pending = true;
        ti->m_subaddrIndex = { pd.m_subaddr_index.minor };
        ti->m_subaddrAccount = pd.m_subaddr_index.major;
        ti->m_label     = m_wallet->m_wallet->get_subaddress_label(pd.m_subaddr_index);
        ti->m_timestamp = pd.m_timestamp;
        ti->m_confirmations = 0;
        m_history.push_back(ti);
        
        LOG_PRINT_L1(__FUNCTION__ << ": Unconfirmed payment found " << pd.m_amount);
    }
     
}

} // namespace
