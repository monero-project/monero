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

//third party headers
#include "carrot_core/carrot_enote_types.h"
#include "span.h"
#include "subaddress_index.h"

//standard headers
#include <optional>
#include <variant>

//forward declarations
namespace carrot
{
struct view_incoming_key_device;
struct view_balance_secret_device;
}

namespace carrot
{
struct LegacyOutputOpeningHintV1
{
    // WARNING: Using this opening hint is unsafe and enables for HW devices to
    //          accidentally burn XMR if an attacker controls the hot wallet and
    //          can publish a new enote with the same K_o as an existing enote,
    //          but with a different amount. However, it is unavoidable for
    //          legacy enotes, since the computation of K_o is not directly nor
    //          indirectly bound to the amount.

    // Informs remote prover (implied to know opening of K^j_s given j) how to open O, C such that:
    // O = K^j_s + Hs1(8 k_v K_e, i) G
    // C = z G + a H

    // O
    crypto::public_key onetime_address;

    // K_e
    crypto::public_key ephemeral_tx_pubkey;

    // j (legacy only)
    subaddress_index subaddr_index;

    // a
    rct::xmr_amount amount;

    // z
    rct::key amount_blinding_factor;

    // i
    std::size_t local_output_index;
};
bool operator==(const LegacyOutputOpeningHintV1&, const LegacyOutputOpeningHintV1&);

struct CarrotOutputOpeningHintV1
{
    // source enote
    CarrotEnoteV1 source_enote;

    // pid_enc
    std::optional<encrypted_payment_id_t> encrypted_payment_id;

    // j, derive type
    subaddress_index_extended subaddr_index;
};
bool operator==(const CarrotOutputOpeningHintV1&, const CarrotOutputOpeningHintV1&);

struct CarrotOutputOpeningHintV2
{
    /// K_o
    crypto::public_key onetime_address;
    /// C_a
    rct::key amount_commitment;
    /// anchor_enc
    encrypted_janus_anchor_t anchor_enc;
    /// view_tag
    view_tag_t view_tag;
    /// D_e
    mx25519_pubkey enote_ephemeral_pubkey;
    /// L_0
    crypto::key_image tx_first_key_image;

    /// a
    rct::xmr_amount amount;

    // pid_enc
    std::optional<encrypted_payment_id_t> encrypted_payment_id;

    // j, derive type
    subaddress_index_extended subaddr_index;
};
bool operator==(const CarrotOutputOpeningHintV2&, const CarrotOutputOpeningHintV2&);

struct CarrotCoinbaseOutputOpeningHintV1
{
    // source enote
    CarrotCoinbaseEnoteV1 source_enote;

    // no encrypted pids for coinbase transactions

    // subaddress index is assumed to be (0, 0) in coinbase transactions
    AddressDeriveType derive_type;
};
bool operator==(const CarrotCoinbaseOutputOpeningHintV1&, const CarrotCoinbaseOutputOpeningHintV1&);

using OutputOpeningHintVariant = std::variant<
        LegacyOutputOpeningHintV1,
        CarrotOutputOpeningHintV1,
        CarrotOutputOpeningHintV2,
        CarrotCoinbaseOutputOpeningHintV1
    >;
const crypto::public_key &onetime_address_ref(const OutputOpeningHintVariant&);
rct::key amount_commitment_ref(const OutputOpeningHintVariant&);
subaddress_index_extended subaddress_index_ref(const OutputOpeningHintVariant&);

/**
 * brief: try_scan_sender_extensions_on_opening_hint - get sender extensions for given opening hint
 * param: opening_hint
 * param: k_view_incoming_dev - k_v [OPTIONAL]
 * param: s_view_balance_dev - s_vb [OPTIONAL]
 * param: sender_extension_g_out - k^g_o
 * param: sender_extension_t_out - k^t_o
 * return: true iff Carrot enote scan was successful, or if nominal legacy derivation-to-scalar didn't fail
 */
bool try_scan_opening_hint(const OutputOpeningHintVariant &opening_hint,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out);
} //namespace carrot
