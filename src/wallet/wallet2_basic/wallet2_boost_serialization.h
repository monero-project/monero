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

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/export.hpp>

#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "serialization/polymorphic_portable_binary_iarchive.h"
#include "serialization/polymorphic_portable_binary_oarchive.h"
#include "wallet2_storage.h"
#include "wallet2_types.h"

namespace wallet2_basic
{
struct HashchainAccessor
{
    hashchain& hc;
    HashchainAccessor(hashchain& hc): hc(hc) {}
    inline std::size_t& m_offset() { return hc.m_offset; }
    inline crypto::hash& m_genesis() { return hc.m_genesis; }
    inline std::deque<crypto::hash>& m_blockchain() { return hc.m_blockchain; }
};
} // namespace wallet2_basic

namespace boost
{
namespace serialization
{
template <class Archive>
void serialize(Archive &a, wallet2_basic::multisig_info::LR &x, const version_type ver)
{
    a & x.m_L;
    a & x.m_R;
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::multisig_info &x, const version_type ver)
{
    a & x.m_signer;
    a & x.m_LR;
    a & x.m_partial_key_images;
}

template <class Archive>
std::enable_if_t<!Archive::is_loading::value>
initialize_transfer_details(Archive &a, wallet2_basic::transfer_details &x, const version_type ver)
{}

template <class Archive>
std::enable_if_t<Archive::is_loading::value>
initialize_transfer_details(Archive &a, wallet2_basic::transfer_details &x, const version_type ver)
{
    if (ver < 1)
    {
        x.m_mask = rct::identity();
        x.m_amount = x.m_tx.vout[x.m_internal_output_index].amount;
    }
    if (ver < 2)
    {
        x.m_spent_height = 0;
    }
    if (ver < 4)
    {
        x.m_rct = x.m_tx.vout[x.m_internal_output_index].amount == 0;
    }
    if (ver < 6)
    {
        x.m_key_image_known = true;
    }
    if (ver < 7)
    {
        x.m_pk_index = 0;
    }
    if (ver < 8)
    {
        x.m_subaddr_index = {};
    }
    if (ver < 9)
    {
        x.m_key_image_partial = false;
        x.m_multisig_k.clear();
        x.m_multisig_info.clear();
    }
    if (ver < 10)
    {
        x.m_key_image_request = false;
    }
    if (ver < 12)
    {
        x.m_frozen = false;
    }
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::hashchain &x, const version_type ver)
{
    wallet2_basic::HashchainAccessor xaccess(x);
    a & xaccess.m_offset();
    a & xaccess.m_genesis();
    a & xaccess.m_blockchain();
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::transfer_details &x, const version_type ver)
{
    a & x.m_block_height;
    a & x.m_global_output_index;
    a & x.m_internal_output_index;
    if (ver < 3)
    {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
        x.m_txid = cryptonote::get_transaction_hash(tx);
    }
    else
    {
        a & x.m_tx;
    }
    a & x.m_spent;
    a & x.m_key_image;
    if (ver < 1)
    {
        // ensure mask and amount are set
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_mask;
    a & x.m_amount;
    if (ver < 2)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_spent_height;
    if (ver < 3)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_txid;
    if (ver < 4)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_rct;
    if (ver < 5)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    if (ver < 6)
    {
        // v5 did not properly initialize
        uint8_t u = 0;
        a & u;
        x.m_key_image_known = true;
        return;
    }
    a & x.m_key_image_known;
    if (ver < 7)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_pk_index;
    if (ver < 8)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_subaddr_index;
    if (ver < 9)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_multisig_info;
    a & x.m_multisig_k;
    a & x.m_key_image_partial;
    if (ver < 10)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_key_image_request;
    if (ver < 11)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_uses;
    if (ver < 12)
    {
        initialize_transfer_details(a, x, ver);
        return;
    }
    a & x.m_frozen;
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::unconfirmed_transfer_details &x, const version_type ver)
{
    a & x.m_change;
    a & x.m_sent_time;
    if (ver < 5)
    {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
    }
    else
    {
        a & x.m_tx;
    }
    if (ver < 1)
        return;
    a & x.m_dests;
    a & x.m_payment_id;
    if (ver < 2)
        return;
    a & x.m_state;
    if (ver < 3)
        return;
    a & x.m_timestamp;
    if (ver < 4)
        return;
    a & x.m_amount_in;
    a & x.m_amount_out;
    if (ver < 6)
    {
        // v<6 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs.
        // We convert it to include change from v6
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)1)
        x.m_amount_out += x.m_change;
    }
    if (ver < 7)
    {
        x.m_subaddr_account = 0;
        return;
    }
    a & x.m_subaddr_account;
    a & x.m_subaddr_indices;
    if (ver < 8)
        return;
    a & x.m_rings;
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::confirmed_transfer_details &x, const version_type ver)
{
    a & x.m_amount_in;
    a & x.m_amount_out;
    a & x.m_change;
    a & x.m_block_height;
    if (ver < 1)
        return;
    a & x.m_dests;
    a & x.m_payment_id;
    if (ver < 2)
        return;
    a & x.m_timestamp;
    if (ver < 3)
    {
        // v<3 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs. Whether it got added
        // or not depends on whether it came from a unconfirmed_transfer_details
        // (not included) or not (included). We can't reliably tell here, so we
        // check whether either yields a "negative" fee, or use the other if so.
        // We convert it to include change from v3
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)1)
        {
        if (x.m_amount_in > (x.m_amount_out + x.m_change))
            x.m_amount_out += x.m_change;
        }
    }
    if (ver < 4)
    {
        if (!typename Archive::is_saving())
        x.m_unlock_time = 0;
        return;
    }
    a & x.m_unlock_time;
    if (ver < 5)
    {
        x.m_subaddr_account = 0;
        return;
    }
    a & x.m_subaddr_account;
    a & x.m_subaddr_indices;
    if (ver < 6)
        return;
    a & x.m_rings;
}

template <class Archive>
void serialize(Archive& a, wallet2_basic::payment_details& x, const version_type ver)
{
    a & x.m_tx_hash;
    a & x.m_amount;
    a & x.m_block_height;
    a & x.m_unlock_time;
    if (ver < 1)
        return;
    a & x.m_timestamp;
    if (ver < 2)
    {
        x.m_coinbase = false;
        x.m_subaddr_index = {};
        return;
    }
    a & x.m_subaddr_index;
    if (ver < 3)
    {
        x.m_coinbase = false;
        x.m_fee = 0;
        return;
    }
    a & x.m_fee;
    if (ver < 4)
    {
        x.m_coinbase = false;
        return;
    }
    a & x.m_coinbase;
    if (ver < 5)
        return;
    a & x.m_amounts;
}

template <class Archive>
void serialize(Archive& a, wallet2_basic::pool_payment_details& x, const version_type ver)
{
    a & x.m_pd;
    a & x.m_double_spend_seen;
}

template <class Archive>
void serialize(Archive& a, wallet2_basic::address_book_row& x, const version_type ver)
{
    a & x.m_address;
    if (ver < 18)
    {
        crypto::hash payment_id;
        a & payment_id;
        x.m_has_payment_id = !(payment_id == crypto::null_hash);
        if (x.m_has_payment_id)
        {
        bool is_long = false;
        for (int i = 8; i < 32; ++i)
            is_long |= payment_id.data[i];
        if (is_long)
        {
            MWARNING("Long payment ID ignored on address book load");
            x.m_payment_id = crypto::null_hash8;
            x.m_has_payment_id = false;
        }
        else
            memcpy(x.m_payment_id.data, payment_id.data, 8);
        }
    }
    a & x.m_description;
    if (ver < 17)
    {
        x.m_is_subaddress = false;
        return;
    }
    a & x.m_is_subaddress;
    if (ver < 18)
        return;
    a & x.m_has_payment_id;
    if (x.m_has_payment_id)
        a & x.m_payment_id;
}

template <class Archive>
void serialize(Archive& a, wallet2_basic::cache& x, const version_type ver)
{
    using namespace wallet2_basic;

    uint64_t dummy_refresh_height = 0; // moved to keys file
    if(ver < 5)
        return;
    if (ver < 19)
    {
        std::vector<crypto::hash> blockchain;
        a & blockchain;
        x.m_blockchain.clear();
        for (const auto &b: blockchain)
        {
            x.m_blockchain.push_back(b);
        }
    }
    else
    {
        a & x.m_blockchain;
    }
    a & x.m_transfers;
    a & x.m_account_public_address;
    a & x.m_key_images.parent();
    if(ver < 6)
        return;
    a & x.m_unconfirmed_txs.parent();
    if(ver < 7)
        return;
    a & x.m_payments.parent();
    if(ver < 8)
        return;
    a & x.m_tx_keys.parent();
    if(ver < 9)
        return;
    a & x.m_confirmed_txs.parent();
    if(ver < 11)
        return;
    a & dummy_refresh_height;
    if(ver < 12)
        return;
    a & x.m_tx_notes.parent();
    if(ver < 13)
        return;
    if (ver < 17)
    {
        // we're loading an old version, where m_unconfirmed_payments was a std::map
        std::unordered_map<crypto::hash, payment_details> m;
        a & m;
        x.m_unconfirmed_payments.clear();
        for (const auto& i : m)
            x.m_unconfirmed_payments.insert({i.first, pool_payment_details{i.second, false}});
    }
    if(ver < 14)
        return;
    if(ver < 15)
    {
        // we're loading an older wallet without a pubkey map, rebuild it
        x.m_pub_keys.clear();
        for (size_t i = 0; i < x.m_transfers.size(); ++i)
        {
            const transfer_details &td = x.m_transfers[i];
            x.m_pub_keys.emplace(td.get_public_key(), i);
        }
        return;
    }
    a & x.m_pub_keys.parent();
    if(ver < 16)
        return;
    a & x.m_address_book;
    if(ver < 17)
        return;
    if (ver < 22)
    {
        // we're loading an old version, where m_unconfirmed_payments payload was payment_details
        std::unordered_multimap<crypto::hash, payment_details> m;
        a & m;
        x.m_unconfirmed_payments.clear();
        for (const auto &i: m)
            x.m_unconfirmed_payments.insert({i.first, pool_payment_details{i.second, false}});
    }
    if(ver < 18)
        return;
    a & x.m_scanned_pool_txs[0];
    a & x.m_scanned_pool_txs[1];
    if (ver < 20)
        return;
    a & x.m_subaddresses.parent();
    std::unordered_map<cryptonote::subaddress_index, crypto::public_key> dummy_subaddresses_inv;
    a & dummy_subaddresses_inv;
    a & x.m_subaddress_labels;
    a & x.m_additional_tx_keys.parent();
    if(ver < 21)
        return;
    a & x.m_attributes.parent();
    if(ver < 22)
        return;
    a & x.m_unconfirmed_payments.parent();
    if(ver < 23)
        return;
    a & (std::pair<std::map<std::string, std::string>, std::vector<std::string>>&) x.m_account_tags;
    if(ver < 24)
        return;
    a & x.m_ring_history_saved;
    if(ver < 25)
        return;
    a & x.m_last_block_reward;
    if(ver < 26)
        return;
    a & x.m_tx_device.parent();
    if(ver < 27)
        return;
    a & x.m_device_last_key_image_sync;
    if(ver < 28)
        return;
    a & x.m_cold_key_images.parent();
    if(ver < 29)
        return;
    crypto::secret_key dummy_rpc_client_secret_key; // Compatibility for old RPC payment system
    a & dummy_rpc_client_secret_key;
    if(ver < 30)
    {
        x.m_has_ever_refreshed_from_node = false;
        return;
    }
    a & x.m_has_ever_refreshed_from_node;
}
} // namespace serialization
} // namespace boost

BOOST_CLASS_VERSION(wallet2_basic::hashchain, 0)
BOOST_CLASS_VERSION(wallet2_basic::transfer_details, 12)
BOOST_CLASS_VERSION(wallet2_basic::multisig_info::LR, 0)
BOOST_CLASS_VERSION(wallet2_basic::multisig_info, 1)
BOOST_CLASS_VERSION(wallet2_basic::unconfirmed_transfer_details, 8)
BOOST_CLASS_VERSION(wallet2_basic::confirmed_transfer_details, 6)
BOOST_CLASS_VERSION(wallet2_basic::payment_details, 5)
BOOST_CLASS_VERSION(wallet2_basic::pool_payment_details, 1)
BOOST_CLASS_VERSION(wallet2_basic::address_book_row, 18)
BOOST_CLASS_VERSION(wallet2_basic::cache, 30)

BOOST_CLASS_EXPORT_KEY(::wallet2_basic::hashchain)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::transfer_details)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::multisig_info::LR)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::multisig_info)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::unconfirmed_transfer_details)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::confirmed_transfer_details)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::payment_details)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::pool_payment_details)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::address_book_row)
BOOST_CLASS_EXPORT_KEY(::wallet2_basic::cache)
