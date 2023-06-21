// Copyright (c) 2023, The Monero Project
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

#pragma once

#include "cryptonote_basic/cryptonote_basic.h"
#include "wallet/wallet_errors.h"

namespace wallet2_basic
{
namespace error = tools::error;

struct HashchainAccessor;
class hashchain
{
public:
    hashchain(): m_genesis(crypto::null_hash), m_offset(0) {}

    size_t size() const { return m_blockchain.size() + m_offset; }
    size_t offset() const { return m_offset; }
    const crypto::hash &genesis() const { return m_genesis; }
    void push_back(const crypto::hash &hash) { if (m_offset == 0 && m_blockchain.empty()) m_genesis = hash; m_blockchain.push_back(hash); }
    bool is_in_bounds(size_t idx) const { return idx >= m_offset && idx < size(); }
    const crypto::hash &operator[](size_t idx) const { return m_blockchain[idx - m_offset]; }
    crypto::hash &operator[](size_t idx) { return m_blockchain[idx - m_offset]; }
    void crop(size_t height) { m_blockchain.resize(height - m_offset); }
    void clear() { m_offset = 0; m_blockchain.clear(); }
    bool empty() const { return m_blockchain.empty() && m_offset == 0; }
    void trim(size_t height) { while (height > m_offset && m_blockchain.size() > 1) { m_blockchain.pop_front(); ++m_offset; } m_blockchain.shrink_to_fit(); }
    void refill(const crypto::hash &hash) { m_blockchain.push_back(hash); --m_offset; }

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        VARINT_FIELD(m_offset)
        FIELD(m_genesis)
        FIELD(m_blockchain)
    END_SERIALIZE()

private:
    size_t m_offset;
    crypto::hash m_genesis;
    std::deque<crypto::hash> m_blockchain;

    friend struct HashchainAccessor;
};

struct multisig_info
{
    struct LR
    {
        rct::key m_L;
        rct::key m_R;

        BEGIN_SERIALIZE_OBJECT()
            FIELD(m_L)
            FIELD(m_R)
        END_SERIALIZE()
    };

    crypto::public_key m_signer;
    std::vector<LR> m_LR;
    std::vector<crypto::key_image> m_partial_key_images; // one per key the participant has

    BEGIN_SERIALIZE_OBJECT()
        FIELD(m_signer)
        FIELD(m_LR)
        FIELD(m_partial_key_images)
    END_SERIALIZE()
};

struct transfer_details
{
    uint64_t m_block_height;
    cryptonote::transaction_prefix m_tx;
    crypto::hash m_txid;
    uint64_t m_internal_output_index;
    uint64_t m_global_output_index;
    bool m_spent;
    bool m_frozen;
    uint64_t m_spent_height;
    crypto::key_image m_key_image; //TODO: key_image stored twice :(
    rct::key m_mask;
    uint64_t m_amount;
    bool m_rct;
    bool m_key_image_known;
    bool m_key_image_request; // view wallets: we want to request it; cold wallets: it was requested
    uint64_t m_pk_index;
    cryptonote::subaddress_index m_subaddr_index;
    bool m_key_image_partial;
    std::vector<rct::key> m_multisig_k;
    std::vector<multisig_info> m_multisig_info; // one per other participant
    std::vector<std::pair<uint64_t, crypto::hash>> m_uses;

    bool is_rct() const { return m_rct; }
    uint64_t amount() const { return m_amount; }
    const crypto::public_key get_public_key() const {
        crypto::public_key output_public_key;
        THROW_WALLET_EXCEPTION_IF(m_tx.vout.size() <= m_internal_output_index,
            error::wallet_internal_error, "Too few outputs, outputs may be corrupted");
        THROW_WALLET_EXCEPTION_IF(!get_output_public_key(m_tx.vout[m_internal_output_index], output_public_key),
            error::wallet_internal_error, "Unable to get output public key from output");
        return output_public_key;
    };

    BEGIN_SERIALIZE_OBJECT()
        FIELD(m_block_height)
        FIELD(m_tx)
        FIELD(m_txid)
        FIELD(m_internal_output_index)
        FIELD(m_global_output_index)
        FIELD(m_spent)
        FIELD(m_frozen)
        FIELD(m_spent_height)
        FIELD(m_key_image)
        FIELD(m_mask)
        FIELD(m_amount)
        FIELD(m_rct)
        FIELD(m_key_image_known)
        FIELD(m_key_image_request)
        FIELD(m_pk_index)
        FIELD(m_subaddr_index)
        FIELD(m_key_image_partial)
        FIELD(m_multisig_k)
        FIELD(m_multisig_info)
        FIELD(m_uses)
    END_SERIALIZE()
};

typedef std::vector<transfer_details> transfer_container;

struct unconfirmed_transfer_details
{
    cryptonote::transaction_prefix m_tx;
    uint64_t m_amount_in;
    uint64_t m_amount_out;
    uint64_t m_change;
    time_t m_sent_time;
    std::vector<cryptonote::tx_destination_entry> m_dests;
    crypto::hash m_payment_id;
    enum { pending, pending_in_pool, failed } m_state;
    uint64_t m_timestamp;
    uint32_t m_subaddr_account;   // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> m_subaddr_indices;  // set of address indices used as inputs in this transfer
    std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> m_rings; // relative

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        FIELD(m_tx)
        VARINT_FIELD(m_amount_in)
        VARINT_FIELD(m_amount_out)
        VARINT_FIELD(m_change)
        VARINT_FIELD(m_sent_time)
        FIELD(m_dests)
        FIELD(m_payment_id)
        if (version >= 1)
            VARINT_FIELD(m_state)
        VARINT_FIELD(m_timestamp)
        VARINT_FIELD(m_subaddr_account)
        FIELD(m_subaddr_indices)
        FIELD(m_rings)
    END_SERIALIZE()
};

struct confirmed_transfer_details
{
    cryptonote::transaction_prefix m_tx;
    uint64_t m_amount_in;
    uint64_t m_amount_out;
    uint64_t m_change;
    uint64_t m_block_height;
    std::vector<cryptonote::tx_destination_entry> m_dests;
    crypto::hash m_payment_id;
    uint64_t m_timestamp;
    uint64_t m_unlock_time;
    uint32_t m_subaddr_account;   // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> m_subaddr_indices;  // set of address indices used as inputs in this transfer
    std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> m_rings; // relative

    confirmed_transfer_details()
        : m_amount_in(0)
        , m_amount_out(0)
        , m_change((uint64_t)-1)
        , m_block_height(0)
        , m_payment_id(crypto::null_hash)
        , m_timestamp(0)
        , m_unlock_time(0)
        , m_subaddr_account((uint32_t)-1)
    {}

    confirmed_transfer_details(const unconfirmed_transfer_details &utd, uint64_t height)
        : m_tx(utd.m_tx)
        , m_amount_in(utd.m_amount_in)
        , m_amount_out(utd.m_amount_out)
        , m_change(utd.m_change)
        , m_block_height(height)
        , m_dests(utd.m_dests)
        , m_payment_id(utd.m_payment_id)
        , m_timestamp(utd.m_timestamp)
        , m_unlock_time(utd.m_tx.unlock_time)
        , m_subaddr_account(utd.m_subaddr_account)
        , m_subaddr_indices(utd.m_subaddr_indices)
        , m_rings(utd.m_rings)
    {}

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        if (version >= 1)
            FIELD(m_tx)
        VARINT_FIELD(m_amount_in)
        VARINT_FIELD(m_amount_out)
        VARINT_FIELD(m_change)
        VARINT_FIELD(m_block_height)
        FIELD(m_dests)
        FIELD(m_payment_id)
        VARINT_FIELD(m_timestamp)
        VARINT_FIELD(m_unlock_time)
        VARINT_FIELD(m_subaddr_account)
        FIELD(m_subaddr_indices)
        FIELD(m_rings)
    END_SERIALIZE()
};

typedef std::vector<uint64_t> amounts_container;
struct payment_details
{
    crypto::hash m_tx_hash;
    uint64_t m_amount;
    amounts_container m_amounts;
    uint64_t m_fee;
    uint64_t m_block_height;
    uint64_t m_unlock_time;
    uint64_t m_timestamp;
    bool m_coinbase;
    cryptonote::subaddress_index m_subaddr_index;

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_tx_hash)
        VARINT_FIELD(m_amount)
        FIELD(m_amounts)
        VARINT_FIELD(m_fee)
        VARINT_FIELD(m_block_height)
        VARINT_FIELD(m_unlock_time)
        VARINT_FIELD(m_timestamp)
        FIELD(m_coinbase)
        FIELD(m_subaddr_index)
    END_SERIALIZE()
};

typedef serializable_unordered_multimap<crypto::hash, payment_details> payment_container;

struct pool_payment_details
{
    payment_details m_pd;
    bool m_double_spend_seen;

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_pd)
        FIELD(m_double_spend_seen)
    END_SERIALIZE()
};

// GUI Address book
struct address_book_row
{
    cryptonote::account_public_address m_address;
    crypto::hash8 m_payment_id;
    std::string m_description;
    bool m_is_subaddress;
    bool m_has_payment_id;

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_address)
        FIELD(m_payment_id)
        FIELD(m_description)
        FIELD(m_is_subaddress)
        FIELD(m_has_payment_id)
    END_SERIALIZE()
};

enum RefreshType
{
    RefreshFull,
    RefreshOptimizeCoinbase,
    RefreshNoCoinbase,
    RefreshDefault = RefreshOptimizeCoinbase,
};

enum AskPasswordType
{
    AskPasswordNever = 0,
    AskPasswordOnAction = 1,
    AskPasswordToDecrypt = 2,
};

enum BackgroundMiningSetupType
{
    BackgroundMiningMaybe = 0,
    BackgroundMiningYes = 1,
    BackgroundMiningNo = 2,
};

enum ExportFormat
{
    Binary = 0,
    Ascii,
};
} // namespace wallet2_basic
