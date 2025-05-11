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
#include "carrot_core/payment_proposal.h"
#include "subaddress_index.h"

//third party headers

//standard headers
#include <vector>

//forward declarations

namespace carrot
{
/**
 * brief: CarrotPaymentProposalVerifiableSelfSendV1 - A selfsend payment proposal, verified to an owned address
 *
 * The `subaddr_index` field is intended to be be used to derive
 * `proposal.destination_address_spend_pubkey`, without the need for a subaddress lookahead table.
 */
struct CarrotPaymentProposalVerifiableSelfSendV1
{
    CarrotPaymentProposalSelfSendV1 proposal;
    subaddress_index_extended subaddr_index;
};

/**
 * brief: CarrotTransactionProposalV1 - A specification on how to construct a Carrot transaction, minus key material
 *
 * The fields in `CarrotTransactionProposalV1` are chosen as what is the absolute minimum amount of
 * information required to verifiably, in a human-meaningful way, reconstruct the "signable
 * transaction hash". The signable transaction hash, (usually written `signable_tx_hash` in code),
 * is the actual 32-byte message that the FCMP++ Spend-Authorization & Linkability (SA/L) proofs
 * sign and verify against. In situations where the signing device may be separate from the device
 * formulating the transaction (e.g. cold signing, hardware devices, multisig, etc), this struct
 * can be passed amongst signers so that the signers can actually verifying what they are signing.
 *
 * For exact details on what goes into the signable transaction hash, see `rct::get_pre_mlsag_hash`.
 */
struct CarrotTransactionProposalV1
{
    std::vector<crypto::key_image> key_images_sorted;

    std::vector<CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    encrypted_payment_id_t dummy_encrypted_payment_id;
    rct::xmr_amount fee;

    std::vector<std::uint8_t> extra;
};
} //namespace carrot
