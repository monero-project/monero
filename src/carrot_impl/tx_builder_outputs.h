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
#include "cryptonote_basic/cryptonote_basic.h"
#include "tx_proposal.h"

//third party headers

//standard headers

//forward declarations

namespace carrot
{
/**
 * brief: get_output_enote_proposals_from_proposal_v1 - get_output_enote_proposals for transaction proposals
 * param: tx_proposal - transaction proposal
 * param: s_view_balance_dev - device for s_vb (optional)
 * param: k_view_dev - device for k_v (optional)
 * outparam: output_enote_proposals_out - resultant output enote set
 * putparam: encrypted_payment_id_out - resultant pid_enc
 * outparam: payment_proposal_order_out - order of payment proposals in resultant output enote set
 */
void get_output_enote_proposals_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    std::vector<std::pair<bool, std::size_t>> *payment_proposal_order_out = nullptr);
/**
 * brief: make_signable_tx_hash_from_proposal_v1 - make signable transaction hash from tx proposal and keys
 * param: tx_proposal - transaction proposal
 * param: s_view_balance_dev - device for s_vb (optional)
 * param: k_view_dev - device for k_v (optional)
 * outparam: signable_tx_hash_out - signable transaction hash
 */
void make_signable_tx_hash_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    crypto::hash &signable_tx_hash_out);
/**
 * brief: make_pruned_transaction_from_proposal_v1 - make pruned Carrot/FCMP++ transaction from tx proposal and keys
 * param: tx_proposal - transaction proposal
 * param: s_view_balance_dev - device for s_vb (optional)
 * param: k_view_dev - device for k_v (optional)
 * outparam: pruned_tx_out - pruned Carrot/FCMP++ transaction represented by transaction proposal
 */
void make_pruned_transaction_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    cryptonote::transaction &pruned_tx_out);
} //namespace carrot
