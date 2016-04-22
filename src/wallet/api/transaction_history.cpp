// Copyright (c) 2014-2016, The Monero Project
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

namespace Bitmonero {

TransactionHistory::~TransactionHistory() {}


TransactionHistoryImpl::TransactionHistoryImpl(WalletImpl *wallet)
    : m_wallet(wallet)
{

}

TransactionHistoryImpl::~TransactionHistoryImpl()
{

}

int TransactionHistoryImpl::count() const
{
    return m_history.size();
}

TransactionInfo *TransactionHistoryImpl::transaction(const std::string &id) const
{
    return nullptr;
}

std::vector<TransactionInfo *> TransactionHistoryImpl::getAll() const
{
    return m_history;
}

void TransactionHistoryImpl::refresh()
{
    // TODO: configurable values;
    uint64_t min_height = 0;
    uint64_t max_height = (uint64_t)-1;

    // delete old transactions;
    for (auto t : m_history)
        delete t;

    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;

    // transactions are stored in wallet2:
    // - confirmed_transfer_details   - out transfers
    // - unconfirmed_transfer_details - pending out transfers
    // - payment_details              - input transfers

    // payments are "input transactions";
    m_wallet->m_wallet->get_payments(payments, min_height, max_height);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
            payment_id = payment_id.substr(0,16);
        // TODO
        TransactionInfoImpl * ti = new TransactionInfoImpl();
        ti->m_paymentid = payment_id;
        ti->m_amount    = pd.m_amount;
        ti->m_direction = TransactionInfo::Direction_In;
        ti->m_hash      = string_tools::pod_to_hex(pd.m_tx_hash);
        ti->m_blockheight = pd.m_block_height;
        // TODO:
        // ti->m_timestamp = pd.m_timestamp;
        m_history.push_back(ti);

        /* output.insert(std::make_pair(pd.m_block_height, std::make_pair(true, (boost::format("%20.20s %s %s %s")
                                                                                 % print_money(pd.m_amount)
                                                                                 % string_tools::pod_to_hex(pd.m_tx_hash)
                                                                                 % payment_id % "-").str())));*/
    }
}

TransactionInfo *TransactionHistoryImpl::transaction(int index) const
{
    return nullptr;
}

}
