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

#pragma once

//local headers
#include "address_device.h"
#include "carrot_core/carrot_enote_types.h"
#include "carrot_tx_builder_types.h"
#include "fcmp_pp/curve_trees.h"
#include "span.h"

//third party headers

//standard headers
#include <variant>

//forward declarations

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
    // O = K^j_s + k_o G
    // C = z G + a H

    // O
    crypto::public_key onetime_address;

    // k_o
    crypto::secret_key sender_extension_g;

    // j (legacy only)
    subaddress_index subaddr_index;

    // a
    rct::xmr_amount amount;

    // z
    crypto::secret_key amount_blinding_factor;
};

struct CarrotOutputOpeningHintV1
{
    // source enote
    CarrotEnoteV1 source_enote;

    // pid_enc
    std::optional<encrypted_payment_id_t> encrypted_payment_id;

    // j, derive type
    subaddress_index_extended subaddr_index;
};

struct CarrotCoinbaseOutputOpeningHintV1
{
    // source enote
    CarrotCoinbaseEnoteV1 source_enote;

    // no encrypted pids for coinbase transactions

    // subaddress index is assumed to be (0, 0) in coinbase transactions
    AddressDeriveType derive_type;
};

using OutputOpeningHintVariant = std::variant<
        LegacyOutputOpeningHintV1,
        CarrotOutputOpeningHintV1,
        CarrotCoinbaseOutputOpeningHintV1
    >;
const crypto::public_key &onetime_address_ref(const OutputOpeningHintVariant&);
rct::key amount_commitment_ref(const OutputOpeningHintVariant&);

struct CarrotOpenableRerandomizedOutputV1
{
    FcmpRerandomizedOutputCompressed rerandomized_output;

    OutputOpeningHintVariant opening_hint;
};

struct CarrotSignableTransactionProposalV1
{
    CarrotTransactionProposalV1 tx_proposal;

    std::vector<CarrotOpenableRerandomizedOutputV1> inputs;
};

void make_carrot_rerandomized_outputs_nonrefundable(const std::vector<crypto::public_key> &input_onetime_addresses,
    const std::vector<rct::key> &input_amount_commitments,
    const std::vector<rct::key> &input_amount_blinding_factors,
    const std::vector<rct::key> &output_amount_blinding_factors,
    std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out);

bool verify_rerandomized_output_basic(const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const crypto::public_key &onetime_address,
    const rct::key &amount_commitment);

bool verify_openable_rerandomized_output_basic(const CarrotOpenableRerandomizedOutputV1&);

// spend legacy enote addressed to legacy address
void make_sal_proof_legacy_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const LegacyOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend carrot enote addressed to legacy address
void make_sal_proof_carrot_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend carrot enote addressed to carrot address
void make_sal_proof_carrot_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_balance_secret_device &s_view_balance_dev,
    const view_incoming_key_device &k_view_incoming_dev,
    const generate_address_secret_device &s_generate_address_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend carrot coinbase enote addressed to legacy address
void make_sal_proof_carrot_coinbase_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotCoinbaseOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend carrot coinbase enote addressed to carrot address
void make_sal_proof_carrot_coinbase_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotCoinbaseOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_incoming_key_device &k_view_incoming_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend any enote addressed to legacy address
void make_sal_proof_any_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const CarrotOpenableRerandomizedOutputV1 &openable_rerandomized_output,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

// spend any carrot enote addressed to carrot address
void make_sal_proof_any_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const CarrotOpenableRerandomizedOutputV1 &openable_rerandomized_output,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_balance_secret_device &s_view_balance_dev,
    const view_incoming_key_device &k_view_incoming_dev,
    const generate_address_secret_device &s_generate_address_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out);

} //namespace carrot
