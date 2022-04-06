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

#include "transaction_info.h"


using namespace std;

namespace Monero {

TransactionInfo::~TransactionInfo() {}

TransactionInfo::Transfer::Transfer(uint64_t _amount, const string &_address)
    : amount(_amount), address(_address) {}


TransactionInfoImpl::TransactionInfoImpl()
    : m_direction(Direction_Out)
      , m_pending(false)
      , m_failed(false)
      , m_coinbase(false)
      , m_amount(0)
      , m_fee(0)
      , m_blockheight(0)
      , m_subaddrAccount(0)
      , m_timestamp(0)
      , m_confirmations(0)
      , m_unlock_time(0)
{

}

TransactionInfoImpl::~TransactionInfoImpl()
{

}

int TransactionInfoImpl::direction() const
{
    return m_direction;
}


bool TransactionInfoImpl::isPending() const
{
    return m_pending;
}

bool TransactionInfoImpl::isFailed() const
{
    return m_failed;
}

bool TransactionInfoImpl::isCoinbase() const
{
    return m_coinbase;
}

uint64_t TransactionInfoImpl::amount() const
{
    return m_amount;
}

uint64_t TransactionInfoImpl::fee() const
{
    return m_fee;
}

uint64_t TransactionInfoImpl::blockHeight() const
{
    return m_blockheight;
}

std::string TransactionInfoImpl::description() const
{
    return m_description;
}

std::set<uint32_t> TransactionInfoImpl::subaddrIndex() const
{
    return m_subaddrIndex;
}

uint32_t TransactionInfoImpl::subaddrAccount() const
{
    return m_subaddrAccount;
}

string TransactionInfoImpl::label() const
{
    return m_label;
}


string TransactionInfoImpl::hash() const
{
    return m_hash;
}

std::time_t TransactionInfoImpl::timestamp() const
{
    return m_timestamp;
}

string TransactionInfoImpl::paymentId() const
{
    return m_paymentid;
}

const std::vector<TransactionInfo::Transfer> &TransactionInfoImpl::transfers() const
{
    return m_transfers;
}

uint64_t TransactionInfoImpl::confirmations() const
{
    return m_confirmations;
}

uint64_t TransactionInfoImpl::unlockTime() const
{
    return m_unlock_time;
}

} // namespace
