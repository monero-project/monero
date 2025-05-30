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
#include "carrot_impl/address_device.h"
#include "carrot_impl/subaddress_index.h"
#include "span.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers

//standard headers
#include <variant>

//forward declarations

namespace tools
{
namespace wallet
{
struct exported_pre_carrot_transfer_details
{
    crypto::public_key m_pubkey;
    uint64_t m_internal_output_index;
    uint64_t m_global_output_index;
    crypto::public_key m_tx_pubkey;
    union
    {
        struct
        {
            uint8_t m_spent: 1;
            uint8_t m_frozen: 1;
            uint8_t m_rct: 1;
            uint8_t m_key_image_known: 1;
            uint8_t m_key_image_request: 1; // view wallets: we want to request it; cold wallets: it was requested
            uint8_t m_key_image_partial: 1;
        };
        uint8_t flags;
    } m_flags;
    uint64_t m_amount;
    std::vector<crypto::public_key> m_additional_tx_keys;
    uint32_t m_subaddr_index_major;
    uint32_t m_subaddr_index_minor;
};

struct exported_carrot_transfer_details
{
    // K^j_s can be derived on the cold side given j and derive_type. For normal enotes, d_e can
    // be computed from anchor_norm, input_context, K^j_s, and pid. Furthermore, D_e can be
    // recomputed with d_e and K^j_s. For selfsend enote, D_e is passed explicitly. s_sr can be
    // recomputed with D_e and private view key. s^ctx_sr can be recomputed with s_sr, D_e, and
    // input_context. k_a can be recomputed with s^ctx_sr, a, K^j_s, and given enote_type. C_a be
    // be recomputed with a and k_a. Sender extensions k^g_o and k^t_o can be recomputed with
    // s^ctx_sr and C_a. K_o can be recomputed with K^j_s, k^g_o, and k^t_o. For normal enotes,
    // the act of recomputing D_e from d_e and K^j_s prevents Janus attacks. For special enotes,
    // anchor_sp is explicitly provided, which can be checked. As for burning bugs, K_o is
    // recomputed as a function of k^g_o and k^t_o, which are functions of C_a and input_context.
    //
    // This struct is designed to provide just enough information so that a malicious hot wallet
    // or other unprivileged communicator passing an arbitrary packet will not be able to get the
    // cold wallet to sign a SA/L proof for a valid existing enote and perform either a Janus
    // attack or a burning bug attack.

    union
    {
        struct
        {
            uint64_t m_spent: 1;
            uint64_t m_key_image_known: 1;
            uint64_t m_key_image_request: 1; // view wallets: we want to request it; cold wallets: it was requested
            uint64_t m_selfsend: 1;
            uint64_t m_enote_type_change: 1; // true iff enote_type is "change"
            uint64_t m_carrot_derived_addr: 1; // true iff derive_type for receiving addr is AddressDeriveType::Carrot
            uint64_t m_internal: 1;
            uint64_t m_coinbase: 1;
            uint64_t m_has_pid: 1;
            uint64_t m_frozen: 1;
            uint64_t m_key_image_partial: 1;
        };
        uint64_t flags;
    } flags;

    std::uint64_t block_index;
    crypto::key_image tx_first_key_image;
    carrot::subaddress_index subaddr_index;
    carrot::payment_id_t payment_id;
    rct::xmr_amount amount;
    carrot::janus_anchor_t janus_anchor;
    mx25519_pubkey selfsend_enote_ephemeral_pubkey;
};

using exported_transfer_details_variant = std::variant<exported_pre_carrot_transfer_details,
    exported_carrot_transfer_details>;

exported_pre_carrot_transfer_details export_cold_pre_carrot_output(const wallet2_basic::transfer_details &td);

exported_carrot_transfer_details export_cold_carrot_output(const wallet2_basic::transfer_details &td,
    const carrot::cryptonote_hierarchy_address_device &addr_dev);

exported_transfer_details_variant export_cold_output(const wallet2_basic::transfer_details &td,
    const carrot::cryptonote_hierarchy_address_device &addr_dev);

wallet2_basic::transfer_details import_cold_pre_carrot_output(const exported_pre_carrot_transfer_details &etd,
    const cryptonote::account_keys &acc_keys);

wallet2_basic::transfer_details import_cold_carrot_output(const exported_carrot_transfer_details &etd,
    const cryptonote::account_keys &acc_keys);

wallet2_basic::transfer_details import_cold_output(const exported_transfer_details_variant &etd,
    const cryptonote::account_keys &acc_keys);

DECLARE_SERIALIZE_OBJECT(exported_pre_carrot_transfer_details, bool skip_version = false)
DECLARE_SERIALIZE_OBJECT(exported_carrot_transfer_details, bool skip_version = false)
DECLARE_SERIALIZE_OBJECT(exported_transfer_details_variant)
} //namespace wallet
} //namespace tools
