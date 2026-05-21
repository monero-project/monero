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
#include "wallet2_types.h"

//third party headers

//standard headers

//forward declarations

namespace wallet2_basic
{
BEGIN_SERIALIZE_OBJECT_FN(hashchain)
    VERSION_FIELD(0)
    VARINT_FIELD_F(m_offset)
    FIELD_F(m_genesis)
    FIELD_F(m_blockchain)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(multisig_info::LR)
    FIELD_F(m_L)
    FIELD_F(m_R)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(multisig_info)
    FIELD_F(m_signer)
    FIELD_F(m_LR)
    FIELD_F(m_partial_key_images)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(transfer_details)
    FIELD_F(m_block_height)
    FIELD_F(m_tx)
    FIELD_F(m_txid)
    FIELD_F(m_internal_output_index)
    FIELD_F(m_global_output_index)
    FIELD_F(m_spent)
    FIELD_F(m_frozen)
    FIELD_F(m_spent_height)
    FIELD_F(m_key_image)
    FIELD_F(m_mask)
    FIELD_F(m_amount)
    FIELD_F(m_rct)
    FIELD_F(m_key_image_known)
    FIELD_F(m_key_image_request)
    FIELD_F(m_pk_index)
    FIELD_F(m_subaddr_index)
    FIELD_F(m_key_image_partial)
    FIELD_F(m_multisig_k)
    FIELD_F(m_multisig_info)
    FIELD_F(m_uses)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(unconfirmed_transfer_details)
    VERSION_FIELD(1)
    FIELD_F(m_tx)
    VARINT_FIELD_F(m_amount_in)
    VARINT_FIELD_F(m_amount_out)
    VARINT_FIELD_F(m_change)
    VARINT_FIELD_F(m_sent_time)
    FIELD_F(m_dests)
    FIELD_F(m_payment_id)
    if (version >= 1)
        VARINT_FIELD_F(m_state)
    VARINT_FIELD_F(m_timestamp)
    VARINT_FIELD_F(m_subaddr_account)
    FIELD_F(m_subaddr_indices)
    FIELD_F(m_rings)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(confirmed_transfer_details)
    VERSION_FIELD(1)
    if (version >= 1)
        FIELD_F(m_tx)
    VARINT_FIELD_F(m_amount_in)
    VARINT_FIELD_F(m_amount_out)
    VARINT_FIELD_F(m_change)
    VARINT_FIELD_F(m_block_height)
    FIELD_F(m_dests)
    FIELD_F(m_payment_id)
    VARINT_FIELD_F(m_timestamp)
    VARINT_FIELD_F(m_unlock_time)
    VARINT_FIELD_F(m_subaddr_account)
    FIELD_F(m_subaddr_indices)
    FIELD_F(m_rings)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(payment_details)
    VERSION_FIELD(0)
    FIELD_F(m_tx_hash)
    VARINT_FIELD_F(m_amount)
    FIELD_F(m_amounts)
    VARINT_FIELD_F(m_fee)
    VARINT_FIELD_F(m_block_height)
    VARINT_FIELD_F(m_unlock_time)
    VARINT_FIELD_F(m_timestamp)
    FIELD_F(m_coinbase)
    FIELD_F(m_subaddr_index)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(pool_payment_details)
    VERSION_FIELD(0)
    FIELD_F(m_pd)
    FIELD_F(m_double_spend_seen)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(address_book_row)
    VERSION_FIELD(0)
    FIELD_F(m_address)
    FIELD_F(m_payment_id)
    FIELD_F(m_description)
    FIELD_F(m_is_subaddress)
    FIELD_F(m_has_payment_id)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(background_synced_tx_t)
    VERSION_FIELD(0)
    VARINT_FIELD_F(index_in_background_sync_data)

    // prune tx; don't need to keep signature data
    if (!v.tx.serialize_base(ar))
        return false;

    FIELD_F(output_indices)
    VARINT_FIELD_F(height)
    VARINT_FIELD_F(block_timestamp)
    FIELD_F(double_spend_seen)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(background_sync_data_t)
    VERSION_FIELD(0)
    FIELD_F(first_refresh_done)
    FIELD_F(start_height)
    FIELD_F(txs)
    FIELD_F(wallet_refresh_from_block_height)
    VARINT_FIELD_F(subaddress_lookahead_major)
    VARINT_FIELD_F(subaddress_lookahead_minor)
    VARINT_FIELD_F(wallet_refresh_type)
END_SERIALIZE()
} //namespace wallet2_basic
