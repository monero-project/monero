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

#include "wallet/api/wallet2_api.h"
#include <string>
#include <ctime>

using namespace std;

namespace Monero {

int TransactionInfo::direction() const
{
    return m_direction;
}


bool TransactionInfo::isPending() const
{
    return m_pending;
}

bool TransactionInfo::isFailed() const
{
    return m_failed;
}

bool TransactionInfo::isCoinbase() const
{
    return m_coinbase;
}

uint64_t TransactionInfo::amount() const
{
    return m_amount;
}

uint64_t TransactionInfo::fee() const
{
    return m_fee;
}

uint64_t TransactionInfo::blockHeight() const
{
    return m_blockheight;
}

const std::string& TransactionInfo::description() const
{
    return m_description;
}

const std::set<uint32_t>& TransactionInfo::subaddrIndex() const
{
    return m_subaddrIndex;
}

uint32_t TransactionInfo::subaddrAccount() const
{
    return m_subaddrAccount;
}

const string& TransactionInfo::label() const
{
    return m_label;
}


const string& TransactionInfo::hash() const
{
    return m_hash;
}

const std::time_t& TransactionInfo::timestamp() const
{
    return m_timestamp;
}

const string& TransactionInfo::paymentId() const
{
    return m_paymentid;
}

const std::vector<TransactionInfo::Transfer> &TransactionInfo::transfers() const
{
    return m_transfers;
}

uint64_t TransactionInfo::confirmations() const
{
    return m_confirmations;
}

uint64_t TransactionInfo::unlockTime() const
{
    return m_unlock_time;
}

} // namespace
