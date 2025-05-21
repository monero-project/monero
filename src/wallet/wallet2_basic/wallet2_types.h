// Copyright (c) 2025, The Monero Project
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

//local headers
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/subaddress_index.h"
#include "wallet/wallet_errors.h"

//third party headers

//standard headers

//forward declarations
namespace wallet2_basic
{
class hashchain;
template <bool W, template <bool> class Archive>
bool do_serialize_object(Archive<W> &ar, hashchain &v);
}
namespace boost { namespace serialization {
template <class Archive> void serialize(Archive&, wallet2_basic::hashchain&, unsigned int);
}}


namespace wallet2_basic
{
/**
 * @brief: caches a contiguous list of block hashes and a genesis block
*/
class hashchain
{
public:
    hashchain(): m_genesis(crypto::null_hash), m_offset(0) {}

    /**
     * @brief: get the "height" of the blockchain, not the number of hashes stored
    */
    size_t size() const { return m_blockchain.size() + m_offset; }
    /**
     * @brief: get the height that the hash list begins at
    */
    size_t offset() const { return m_offset; }
    /**
     * @brief: get the genesis bloch hash
    */
    const crypto::hash &genesis() const { return m_genesis; }
    /**
     * @brief: add a block hash to the top of the chain
    */
    void push_back(const crypto::hash &hash) { if (m_offset == 0 && m_blockchain.empty()) m_genesis = hash; m_blockchain.push_back(hash); }
    /**
     * @brief: query if there is a hash available for a given height
    */
    bool is_in_bounds(size_t idx) const { return idx >= m_offset && idx < size(); }
    /**
     * @brief: get a const reference to the block hash at a given height
    */
    const crypto::hash &operator[](size_t idx) const { return m_blockchain[idx - m_offset]; }
    /**
     * @brief: get a mutable reference to the block hash at a given height
    */
    crypto::hash &operator[](size_t idx) { return m_blockchain[idx - m_offset]; }
    /**
     * @brief: crop stored hashes after a certain height, where the height of the top block == `height`-1
    */
    void crop(size_t height) { m_blockchain.resize(std::max(std::min(height, size()), m_offset) - m_offset); }
    /**
     * @brief: delete all stored hashes and set the offset to 0
    */
    void clear() { m_offset = 0; m_blockchain.clear(); }
    /**
     * @brief: query if the blockchain is "empty": there are no stored hashes and the offset is 0
    */
    bool empty() const { return m_blockchain.empty() && m_offset == 0; }
    /**
     * @brief: crop stored hashes before a certain height and shift the offset accordingly, but always leave at least 1 hash
    */
    void trim(size_t height) { while (height > m_offset && m_blockchain.size() > 1) { m_blockchain.pop_front(); ++m_offset; } m_blockchain.shrink_to_fit(); }
    /**
     * @brief: push a block hash onto the chain and move all block hashes back by one block
    */
    void refill(const crypto::hash &hash) { m_blockchain.push_back(hash); --m_offset; }

private:
    size_t m_offset;
    crypto::hash m_genesis;
    std::deque<crypto::hash> m_blockchain;

    template <bool W, template <bool> class Archive>
    friend bool ::wallet2_basic::do_serialize_object(Archive<W> &ar, hashchain &v);
    template <class Archive>
    friend void ::boost::serialization::serialize(Archive &a, hashchain &x, const unsigned int ver);
};

struct multisig_info
{
    struct LR
    {
        rct::key m_L;
        rct::key m_R;
    };

    crypto::public_key m_signer;
    std::vector<LR> m_LR;
    std::vector<crypto::key_image> m_partial_key_images; // one per key the participant has
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
            tools::error::wallet_internal_error, "Too few outputs, outputs may be corrupted");
        THROW_WALLET_EXCEPTION_IF(!get_output_public_key(m_tx.vout[m_internal_output_index], output_public_key),
            tools::error::wallet_internal_error, "Unable to get output public key from output");
        return output_public_key;
    };

    const fcmp_pp::curve_trees::OutputPair get_output_pair() const {
        return {
            .output_pubkey = get_public_key(),
            .commitment = this->is_rct() ? rct::commit(this->amount(), m_mask) : rct::zeroCommitVartime(this->amount())
        };
    };
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
};

typedef std::unordered_multimap<crypto::hash, payment_details> payment_container;

struct pool_payment_details
{
    payment_details m_pd;
    bool m_double_spend_seen;
};

// GUI Address book
struct address_book_row
{
    cryptonote::account_public_address m_address;
    crypto::hash8 m_payment_id;
    std::string m_description;
    bool m_is_subaddress;
    bool m_has_payment_id;
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

enum BackgroundSyncType
{
    BackgroundSyncOff = 0,
    BackgroundSyncReusePassword = 1,
    BackgroundSyncCustomPassword = 2,
};

enum ExportFormat
{
    Binary = 0,
    Ascii,
};

struct background_synced_tx_t
{
    uint64_t index_in_background_sync_data;
    cryptonote::transaction tx;
    std::vector<uint64_t> output_indices;
    uint64_t height;
    uint64_t block_timestamp;
    bool double_spend_seen;
};

struct background_sync_data_t
{
    bool first_refresh_done = false;
    uint64_t start_height = 0;
    std::unordered_map<crypto::hash, background_synced_tx_t> txs;

    // Relevant wallet settings
    uint64_t wallet_refresh_from_block_height;
    size_t subaddress_lookahead_major;
    size_t subaddress_lookahead_minor;
    RefreshType wallet_refresh_type;
};
} // namespace wallet2_basic
