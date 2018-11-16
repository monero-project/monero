// Copyright (c) 2014-2018, The Monero Project
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

namespace Monero {

class TransactionHistoryImpl;

enum class reward_type {
    unspecified,
    service_node,
    miner
};

class TransactionInfoImpl : public TransactionInfo
{
public:
    TransactionInfoImpl();
    ~TransactionInfoImpl();
    //! in/out
    virtual int direction() const override;
    //! true if hold
    virtual bool isPending() const override;
    virtual bool isFailed() const override;
    virtual uint64_t amount() const override;
    //! always 0 for incoming txes
    virtual uint64_t fee() const override;
    virtual uint64_t blockHeight() const override;
    virtual std::set<uint32_t> subaddrIndex() const override;
    virtual uint32_t subaddrAccount() const override;
    virtual std::string label() const override;

    virtual std::string hash() const override;
    virtual std::time_t timestamp() const override;
    virtual std::string paymentId() const override;
    virtual const std::vector<Transfer> &transfers() const override;
    virtual uint64_t confirmations() const override;
    virtual uint64_t unlockTime() const override;
    bool isServiceNodeReward() const override;
    bool isMinerReward() const override;

private:
    int         m_direction;
    bool        m_pending;
    bool        m_failed;
    reward_type m_reward_type;                 // may have a value rather than `unspecified` after hf 10
    uint64_t    m_amount;
    uint64_t    m_fee;
    uint64_t    m_blockheight;
    std::set<uint32_t> m_subaddrIndex;        // always unique index for incoming transfers; can be multiple indices for outgoing transfers
    uint32_t m_subaddrAccount;
    std::string m_label;
    std::string m_hash;
    std::time_t m_timestamp;
    std::string m_paymentid;
    std::vector<Transfer> m_transfers;
    uint64_t    m_confirmations;
    uint64_t    m_unlock_time;

    friend class TransactionHistoryImpl;

};

} // namespace

namespace Bitmonero = Monero;
