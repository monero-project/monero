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
#include "carrot_impl/carrot_tx_builder_types.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

//third party headers

//standard headers

//forward declarations

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    cryptonote::transaction& tx,
    const uint8_t hf_version,
    const size_t num_tx_outputs = 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    const uint8_t hf_version,
    const size_t num_tx_outputs = 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct stripped_down_tx_source_entry_t
{
    bool is_rct;
    uint64_t global_output_index;
    crypto::public_key onetime_address;
    crypto::public_key real_out_tx_key;
    std::vector<crypto::public_key> real_out_additional_tx_keys;
    size_t local_output_index;
    rct::xmr_amount amount;
    rct::key mask;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_source_entry gen_tx_source_entry_fake_members(
    const stripped_down_tx_source_entry_t &in,
    const size_t mixin,
    const uint64_t max_global_output_index);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    const cryptonote::account_keys &sender_account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &sender_subaddress_map,
    std::vector<stripped_down_tx_source_entry_t> &&stripped_sources,
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const boost::optional<cryptonote::account_public_address> &change_addr,
    const crypto::hash &payment_id,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    crypto::secret_key &main_tx_privkey_out,
    std::vector<crypto::secret_key> &additional_tx_privkeys_out,
    const bool sweep_unmixable_override = false);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    const cryptonote::account_keys &sender_account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &sender_subaddress_map,
    std::vector<stripped_down_tx_source_entry_t> &&stripped_sources,
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const boost::optional<cryptonote::account_public_address> &change_addr,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override = false);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override = false);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr rct::xmr_amount fake_fee_per_weight = 2023;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_carrot_pruned_transaction_fake_inputs(
    const std::vector<carrot::CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const cryptonote::account_keys &acc_keys);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
extern const cryptonote::account_public_address null_addr;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
