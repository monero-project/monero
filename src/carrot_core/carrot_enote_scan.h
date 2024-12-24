// Copyright (c) 2024, The Monero Project
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

//! @file Utilities for scanning carrot enotes

#pragma once

//local headers
#include "carrot_enote_types.h"
#include "device.h"

//third party headers

//standard headers
#include <optional>

//forward declarations


namespace carrot
{
bool verify_carrot_janus_protection(const input_context_t &input_context,
    const crypto::public_key &onetime_address,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    const crypto::public_key &nominal_address_spend_pubkey,
    const crypto::x25519_pubkey &enote_ephemeral_pubkey,
    const janus_anchor_t &nominal_anchor,
    payment_id_t &nominal_payment_id_inout);

bool try_scan_carrot_coinbase_enote(const CarrotCoinbaseEnoteV1 &enote,
    const crypto::x25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out);

bool try_scan_carrot_enote_external(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> encrypted_payment_id,
    const crypto::x25519_pubkey &s_sender_receiver_unctx,
    const view_incoming_key_device &k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    payment_id_t &payment_id_out,
    CarrotEnoteType &enote_type_out);

bool try_scan_carrot_enote_internal(const CarrotEnoteV1 &enote,
    const view_balance_secret_device &s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    crypto::public_key &address_spend_pubkey_out,
    rct::xmr_amount &amount_out,
    crypto::secret_key &amount_blinding_factor_out,
    CarrotEnoteType &enote_type_out);

} //namespace carrot
