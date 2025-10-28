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
#include "common/unordered_containers_boost_serialization.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "wallet2_types.h"

//third party headers
#include <boost/serialization/deque.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>

//standard headers

//forward declarations

namespace boost
{
namespace serialization
{
template <class Archive>
void serialize(Archive &a, wallet2_basic::multisig_info::LR &x, const unsigned int ver)
{
    a & x.m_L;
    a & x.m_R;
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::multisig_info &x, const unsigned int ver)
{
    a & x.m_signer;
    a & x.m_LR;
    a & x.m_partial_key_images;
}

template <class Archive>
std::enable_if_t<!Archive::is_loading::value>
initialize_transfer_details(Archive &a, wallet2_basic::transfer_details &x, const unsigned int ver)
{}

template <class Archive>
std::enable_if_t<Archive::is_loading::value>
initialize_transfer_details(Archive &a, wallet2_basic::transfer_details &x, const unsigned int ver)
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
void serialize(Archive &a, wallet2_basic::hashchain &x, const unsigned int ver)
{
    a & x.m_offset;
    a & x.m_genesis;
    a & x.m_blockchain;
}

template <class Archive>
void serialize(Archive &a, wallet2_basic::transfer_details &x, const unsigned int ver)
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
void serialize(Archive &a, wallet2_basic::unconfirmed_transfer_details &x, const unsigned int ver)
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
void serialize(Archive &a, wallet2_basic::confirmed_transfer_details &x, const unsigned int ver)
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
void serialize(Archive& a, wallet2_basic::payment_details& x, const unsigned int ver)
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
void serialize(Archive& a, wallet2_basic::pool_payment_details& x, const unsigned int ver)
{
    a & x.m_pd;
    a & x.m_double_spend_seen;
}

template <class Archive>
void serialize(Archive& a, wallet2_basic::address_book_row& x, const unsigned int ver)
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
inline void serialize(Archive& a, wallet2_basic::background_synced_tx_t &x, const unsigned int ver)
{
    a & x.index_in_background_sync_data;
    a & x.tx;
    a & x.output_indices;
    a & x.height;
    a & x.block_timestamp;
    a & x.double_spend_seen;
}

template <class Archive>
inline void serialize(Archive& a, wallet2_basic::background_sync_data_t &x, const unsigned int ver)
{
    a & x.first_refresh_done;
    a & x.start_height;
    a & x.txs;
    a & x.wallet_refresh_from_block_height;
    a & x.subaddress_lookahead_major;
    a & x.subaddress_lookahead_minor;
    a & x.wallet_refresh_type;
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
BOOST_CLASS_VERSION(wallet2_basic::background_synced_tx_t, 0)
BOOST_CLASS_VERSION(wallet2_basic::background_sync_data_t, 0)
