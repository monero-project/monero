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
#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/destination.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_core/scan.h"
#include "carrot_impl/subaddress_index.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers

//forward declarations
namespace cryptonote { struct tx_destination_entry; }

namespace carrot
{
namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
static constexpr std::uint32_t MAX_SUBADDRESS_MAJOR_INDEX = 5;
static constexpr std::uint32_t MAX_SUBADDRESS_MINOR_INDEX = 20;
//----------------------------------------------------------------------------------------------------------------------
struct mock_carrot_and_legacy_keys
{
    cryptonote::account_base legacy_acb;

    crypto::secret_key s_master;
    crypto::secret_key k_prove_spend;
    crypto::secret_key s_view_balance;
    crypto::secret_key k_generate_image;
    crypto::secret_key s_generate_address;

    crypto::public_key carrot_account_spend_pubkey;
    crypto::public_key carrot_account_view_pubkey;

    view_incoming_key_ram_borrowed_device k_view_incoming_dev;
    view_balance_secret_ram_borrowed_device s_view_balance_dev;
    generate_address_secret_ram_borrowed_device s_generate_address_dev;

    std::unordered_map<crypto::public_key, subaddress_index_extended> subaddress_map;

    AddressDeriveType default_derive_type;

    mock_carrot_and_legacy_keys(): k_view_incoming_dev(legacy_acb.get_keys().m_view_secret_key),
        s_view_balance_dev(s_view_balance),
        s_generate_address_dev(s_generate_address)
    {}

    mock_carrot_and_legacy_keys(const mock_carrot_and_legacy_keys &k) = delete;
    mock_carrot_and_legacy_keys(mock_carrot_and_legacy_keys&&) = delete;

    mock_carrot_and_legacy_keys& operator=(const mock_carrot_and_legacy_keys&) = delete;
    mock_carrot_and_legacy_keys& operator=(mock_carrot_and_legacy_keys&&) = delete;

    CarrotDestinationV1 cryptonote_address(const payment_id_t payment_id = null_payment_id,
        const AddressDeriveType derive_type = AddressDeriveType::Auto) const;

    CarrotDestinationV1 subaddress(const subaddress_index_extended &subaddress_index) const;

    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddress_map_cn() const;

    // brief: opening_for_subaddress - return (k^g_a, k^t_a) for j s.t. K^j_s = (k^g_a * G + k^t_a * T)
    void opening_for_subaddress(const subaddress_index_extended &subaddress_index,
        crypto::secret_key &address_privkey_g_out,
        crypto::secret_key &address_privkey_t_out,
        crypto::public_key &address_spend_pubkey_out) const;

    bool try_searching_for_opening_for_subaddress(const crypto::public_key &address_spend_pubkey,
        crypto::secret_key &address_privkey_g_out,
        crypto::secret_key &address_privkey_t_out) const;

    bool try_searching_for_opening_for_onetime_address(const crypto::public_key &address_spend_pubkey,
        const crypto::secret_key &sender_extension_g,
        const crypto::secret_key &sender_extension_t,
        crypto::secret_key &x_out,
        crypto::secret_key &y_out) const;

    bool can_open_fcmp_onetime_address(const crypto::public_key &address_spend_pubkey,
        const crypto::secret_key &sender_extension_g,
        const crypto::secret_key &sender_extension_t,
        const crypto::public_key &onetime_address) const;

    crypto::key_image derive_key_image(const crypto::public_key &address_spend_pubkey,
        const crypto::secret_key &sender_extension_g,
        const crypto::secret_key &sender_extension_t,
        const crypto::public_key &onetime_address) const;

    void generate_subaddress_map();

    void generate(const AddressDeriveType default_derive_type = AddressDeriveType::Carrot);

    AddressDeriveType resolve_derive_type(const AddressDeriveType derive_type) const;
};
//----------------------------------------------------------------------------------------------------------------------
struct mock_scan_result_t
{
    crypto::public_key address_spend_pubkey;
    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;

    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;

    CarrotEnoteType enote_type;

    payment_id_t payment_id;

    janus_anchor_t internal_message;

    size_t output_index;
};
//----------------------------------------------------------------------------------------------------------------------
void mock_scan_enote_set(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const mock_carrot_and_legacy_keys &keys,
    std::vector<mock_scan_result_t> &res);
//----------------------------------------------------------------------------------------------------------------------
void mock_scan_coinbase_enote_set(
    const std::vector<CarrotCoinbaseEnoteV1> &coinbase_enotes,
    const mock_carrot_and_legacy_keys &keys,
    std::vector<mock_scan_result_t> &res);
//----------------------------------------------------------------------------------------------------------------------
bool compare_scan_result(const mock_scan_result_t &scan_res,
    const CarrotPaymentProposalV1 &normal_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt = 0);
//----------------------------------------------------------------------------------------------------------------------
bool compare_scan_result(const mock_scan_result_t &scan_res,
    const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal,
    const rct::xmr_amount allowed_fee_margin_opt = 0);
//----------------------------------------------------------------------------------------------------------------------
crypto::key_image gen_key_image();
//----------------------------------------------------------------------------------------------------------------------
crypto::secret_key gen_secret_key();
//----------------------------------------------------------------------------------------------------------------------
subaddress_index gen_subaddress_index();
//----------------------------------------------------------------------------------------------------------------------
subaddress_index_extended gen_subaddress_index_extended(const AddressDeriveType derive_type = AddressDeriveType::Auto);
//----------------------------------------------------------------------------------------------------------------------
std::vector<CarrotEnoteV1> collect_enotes(const std::vector<RCTOutputEnoteProposal> &output_enote_proposals);
//----------------------------------------------------------------------------------------------------------------------
std::uint64_t gen_block_index();
//----------------------------------------------------------------------------------------------------------------------
CarrotDestinationV1 convert_destination_v1(const cryptonote::tx_destination_entry &cn_dst);
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_destination_entry convert_destination_v1(const CarrotDestinationV1 &dst, const rct::xmr_amount amount);
//----------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalV1 convert_normal_payment_proposal_v1(const cryptonote::tx_destination_entry &cn_dst,
    const janus_anchor_t randomness = gen_janus_anchor());
//----------------------------------------------------------------------------------------------------------------------
CarrotPaymentProposalSelfSendV1 convert_selfsend_payment_proposal_v1(const cryptonote::tx_destination_entry &cn_dst);
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
} //namespace carrot
static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
    return 0 == memcmp(&a, &b, sizeof(mx25519_pubkey));
}
//----------------------------------------------------------------------------------------------------------------------
