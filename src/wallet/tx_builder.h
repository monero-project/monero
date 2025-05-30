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
#include "carrot_impl/tx_builder_inputs.h"
#include "carrot_impl/tx_proposal_utils.h"
#include "fcmp_pp/tree_cache.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers

//standard headers
#include <unordered_map>

//forward declarations

namespace tools
{
class wallet2;

namespace wallet
{
struct multisig_sig
{
    rct::rctSig sigs;
    std::unordered_set<crypto::public_key> ignore;
    std::unordered_set<rct::key> used_L;
    std::unordered_set<crypto::public_key> signing_keys;
    rct::multisig_out msout;

    rct::keyM total_alpha_G;
    rct::keyM total_alpha_H;
    rct::keyV c_0;
    rct::keyV s;

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        if (version < 1)
            return false;
        FIELD(sigs)
        FIELD(ignore)
        FIELD(used_L)
        FIELD(signing_keys)
        FIELD(msout)
        FIELD(total_alpha_G)
        FIELD(total_alpha_H)
        FIELD(c_0)
        FIELD(s)
    END_SERIALIZE()
};

struct tx_construction_data
{
    std::vector<cryptonote::tx_source_entry> sources;
    cryptonote::tx_destination_entry change_dts;
    std::vector<cryptonote::tx_destination_entry> splitted_dsts; // split, includes change
    std::vector<size_t> selected_transfers;
    std::vector<uint8_t> extra;
    uint64_t unlock_time;
    bool use_rct;
    rct::RCTConfig rct_config;
    bool use_view_tags;
    std::vector<cryptonote::tx_destination_entry> dests; // original setup, does not include change
    uint32_t subaddr_account;   // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> subaddr_indices;  // set of address indices used as inputs in this transfer

    enum construction_flags_ : uint8_t
    {
        _use_rct          = 1 << 0, // 00000001
        _use_view_tags    = 1 << 1  // 00000010
        // next flag      = 1 << 2  // 00000100
        // ...
        // final flag     = 1 << 7  // 10000000
    };
    uint8_t construction_flags;

    BEGIN_SERIALIZE_OBJECT()
        FIELD(sources)
        FIELD(change_dts)
        FIELD(splitted_dsts)
        FIELD(selected_transfers)
        FIELD(extra)
        FIELD(unlock_time)

        // converted `use_rct` field into construction_flags when view tags
        // were introduced to maintain backwards compatibility
        if (!typename Archive<W>::is_saving())
        {
            FIELD_N("use_rct", construction_flags)
            use_rct = (construction_flags & _use_rct) > 0;
            use_view_tags = (construction_flags & _use_view_tags) > 0;
        }
        else
        {
            construction_flags = 0;
            if (use_rct)
                construction_flags ^= _use_rct;
            if (use_view_tags)
                construction_flags ^= _use_view_tags;

            FIELD_N("use_rct", construction_flags)
        }

        FIELD(rct_config)
        FIELD(dests)
        FIELD(subaddr_account)
        FIELD(subaddr_indices)
    END_SERIALIZE()
};

// The convention for destinations is:
// dests does not include change
// splitted_dsts (in construction_data) does
struct pending_tx
{
    cryptonote::transaction tx;
    uint64_t dust, fee;
    bool dust_added_to_fee;
    cryptonote::tx_destination_entry change_dts;
    std::vector<size_t> selected_transfers;
    std::string key_images;
    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    std::vector<cryptonote::tx_destination_entry> dests;
    std::vector<multisig_sig> multisig_sigs;
    crypto::secret_key multisig_tx_key_entropy;
    uint32_t subaddr_account;            // subaddress account of your wallet to be used in this transfer
    std::set<uint32_t> subaddr_indices;  // set of address indices used as inputs in this transfer

    using tx_reconstruct_variant_t = std::variant<
        tx_construction_data,
        carrot::CarrotTransactionProposalV1
        >;
    tx_reconstruct_variant_t construction_data;

    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(2)
        FIELD(tx)
        FIELD(dust)
        FIELD(fee)
        FIELD(dust_added_to_fee)
        FIELD(change_dts)
        FIELD(selected_transfers)
        FIELD(key_images)
        FIELD(tx_key)
        FIELD(additional_tx_keys)
        FIELD(dests)
        if (version < 2)
        {
            tx_construction_data pre_carrot_construction_data;
            FIELD_N("construction_data", pre_carrot_construction_data)
            construction_data = pre_carrot_construction_data;
            subaddr_account = pre_carrot_construction_data.subaddr_account;
            subaddr_indices = pre_carrot_construction_data.subaddr_indices;
        }
        else // version >= 2
        {
            FIELD(construction_data)
            FIELD(subaddr_account)
            FIELD(subaddr_indices)
        }
        FIELD(multisig_sigs)
        if (version < 1)
        {
            multisig_tx_key_entropy = crypto::null_skey;
            return true;
        }
        FIELD(multisig_tx_key_entropy)
    END_SERIALIZE()
};

std::unordered_map<crypto::public_key, size_t> collect_non_burned_transfers_by_onetime_address(
    const wallet2_basic::transfer_container &transfers);

carrot::select_inputs_func_t make_wallet2_single_transfer_input_selector(
    const wallet2_basic::transfer_container &transfers,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> &from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index,
    const bool allow_carrot_external_inputs_in_normal_transfers,
    const bool allow_pre_carrot_inputs_in_normal_transfers,
    std::set<size_t> &selected_transfer_indices_out);

std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    std::vector<cryptonote::tx_destination_entry> dsts,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    std::set<std::uint32_t> subtract_fee_from_outputs,
    const std::uint64_t top_block_index);
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    wallet2 &w,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const std::set<std::uint32_t> &subtract_fee_from_outputs);

std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint64_t top_block_index);
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep(
    wallet2 &w,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra);

std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep_all(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const rct::xmr_amount only_below,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const std::uint64_t top_block_index);
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep_all(
    wallet2 &w,
    const rct::xmr_amount only_below,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices);

carrot::OutputOpeningHintVariant make_sal_opening_hint_from_transfer_details(const wallet2_basic::transfer_details &td);

std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sign_carrot_transaction_proposal_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const wallet2_basic::transfer_container &transfers,
    const cryptonote::account_keys &acc_keys);

//! @TODO: accept already-calculated rerandomized outputs and their blinds
cryptonote::transaction finalize_all_proofs_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2_basic::transfer_container &transfers,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys);

pending_tx make_pending_carrot_tx(const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<crypto::key_image> &sorted_input_key_images,
    const wallet2_basic::transfer_container &transfers,
    const crypto::secret_key &k_view,
    hw::device &hwdev);

pending_tx finalize_all_proofs_from_transfer_details_as_pending_tx(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2_basic::transfer_container &transfers,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys);
pending_tx finalize_all_proofs_from_transfer_details_as_pending_tx(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2 &w);
} //namespace wallet
} //namespace tools
