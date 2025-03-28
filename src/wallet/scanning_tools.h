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
#include "carrot_core/lazy_amount_commitment.h"
#include "carrot_impl/subaddress_index.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/tx_extra.h"

//third party headers

//standard headers
#include <optional>
#include <vector>

//forward declarations

namespace tools
{
namespace wallet
{
struct enote_view_incoming_scan_info_t
{
    // K_o
    crypto::public_key onetime_address;
    // k^g_o
    crypto::secret_key sender_extension_g;
    // k^t_o
    crypto::secret_key sender_extension_t;
    // K^j_s
    crypto::public_key address_spend_pubkey;
    // pid
    crypto::hash payment_id;
    // j
    std::optional<carrot::subaddress_index_extended> subaddr_index;

    // a
    rct::xmr_amount amount;
    // z
    rct::key amount_blinding_factor;

    // legacy: 8 k_v R, carrot: s^ctx_sr
    crypto::key_derivation derivation;
    // i
    std::size_t local_output_index;
    //
    std::size_t main_tx_pubkey_index;
};

std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote_destination(
    const cryptonote::tx_out &enote_destination,
    const carrot::lazy_amount_commitment_t &lazy_amount_commitment,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const cryptonote::txin_v &first_tx_input,
    const std::size_t local_output_index,
    const epee::span<const crypto::key_derivation> main_derivations,
    const std::vector<crypto::key_derivation> &additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map);
std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote_destination(
    const cryptonote::transaction_prefix &tx_prefix,
    const carrot::lazy_amount_commitment_t &lazy_amount_commitment,
    const std::size_t local_output_index,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map);

bool try_decrypt_enote_amount(const crypto::public_key &onetime_address,
    const rct::key &enote_amount_commitment,
    const std::uint8_t rct_type,
    const rct::ecdhTuple &rct_ecdh_tuple,
    const crypto::key_derivation &derivation,
    const std::size_t local_output_index,
    const crypto::public_key &address_spend_pubkey,
    hw::device &hwdev,
    rct::xmr_amount &amount_out,
    rct::key &amount_blinding_factor_out);

std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote(
    const cryptonote::tx_out &enote_destination,
    const rct::rctSigBase &rct_sig,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const cryptonote::txin_v &first_tx_input,
    const std::size_t local_output_index,
    const epee::span<const crypto::key_derivation> main_derivations,
    const std::vector<crypto::key_derivation> &additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map);

void view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const epee::span<std::optional<enote_view_incoming_scan_info_t>> &enote_scan_infos_out);
std::vector<std::optional<enote_view_incoming_scan_info_t>> view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map);

bool is_long_payment_id(const crypto::hash &pid);

std::optional<crypto::key_image> try_derive_enote_key_image(
    const enote_view_incoming_scan_info_t &enote_scan_info,
    const cryptonote::account_keys &acc);
} //namespace wallet
} //namespace tools
