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

//paired header
#include "tx_builder.h"

//local headers
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/exceptions.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_core/scan.h"
#include "carrot_impl/address_device_ram_borrowed.h"
#include "carrot_impl/format_utils.h"
#include "carrot_impl/key_image_device_composed.h"
#include "carrot_impl/multi_tx_proposal_utils.h"
#include "carrot_impl/tx_builder_inputs.h"
#include "carrot_impl/tx_builder_outputs.h"
#include "common/apply_permutation.h"
#include "common/perf_timer.h"
#include "common/threadpool.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "fcmp_pp/proof_len.h"
#include "fcmp_pp/prove.h"
#include "fcmp_pp/tower_cycle.h"
#include "ringct/bulletproofs_plus.h"
#include "ringct/rctSigs.h"
#include "wallet2.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.tx_builder"

namespace tools
{
namespace wallet
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T>
static constexpr T div_ceil(T dividend, T divisor)
{
    static_assert(std::is_unsigned_v<T>, "T not unsigned int");
    return (dividend + divisor - 1) / divisor;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool is_transfer_usable_for_input_selection(const wallet2_basic::transfer_details &td,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const uint64_t top_block_index)
{
    const uint64_t last_locked_block_index = cryptonote::get_last_locked_block_index(
        td.m_tx.unlock_time, td.m_block_height);

    return !td.m_spent
        && !td.m_frozen
        && last_locked_block_index <= top_block_index
        && td.m_subaddr_index.major == from_account
        && (from_subaddresses.empty() || from_subaddresses.count(td.m_subaddr_index.minor) == 1)
        && td.amount() >= ignore_below
        && td.amount() <= ignore_above
    ;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool build_payment_proposals(std::vector<carrot::CarrotPaymentProposalV1> &normal_payment_proposals_inout,
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_inout,
    const cryptonote::tx_destination_entry &tx_dest_entry,
    const crypto::hash8 payment_id,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    const auto subaddr_it = subaddress_map.find(tx_dest_entry.addr.m_spend_public_key);
    const bool is_selfsend_dest = subaddr_it != subaddress_map.cend();

    // Make N destinations
    if (is_selfsend_dest)
    {
        const carrot::subaddress_index subaddr_index{subaddr_it->second.major, subaddr_it->second.minor};
        selfsend_payment_proposals_inout.push_back(carrot::CarrotPaymentProposalVerifiableSelfSendV1{
            .proposal = carrot::CarrotPaymentProposalSelfSendV1{
                .destination_address_spend_pubkey = tx_dest_entry.addr.m_spend_public_key,
                .amount = tx_dest_entry.amount,
                .enote_type = carrot::CarrotEnoteType::PAYMENT
            },
            .subaddr_index = {subaddr_index, carrot::AddressDeriveType::PreCarrot} // @TODO: handle carrot
        });
    }
    else // not *known* self-send address
    {
        const carrot::CarrotDestinationV1 dest{
            .address_spend_pubkey = tx_dest_entry.addr.m_spend_public_key,
            .address_view_pubkey = tx_dest_entry.addr.m_view_public_key,
            .is_subaddress = tx_dest_entry.is_subaddress,
            .payment_id = carrot::raw_byte_convert<carrot::payment_id_t>(payment_id)
        };

        normal_payment_proposals_inout.push_back(carrot::CarrotPaymentProposalV1{
            .destination = dest,
            .amount = tx_dest_entry.amount,
            .randomness = carrot::gen_janus_anchor()
        });
    }

    return is_selfsend_dest;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void build_payment_proposals_sweep(std::vector<carrot::CarrotPaymentProposalV1> &normal_payment_proposals_inout,
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_inout,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const std::size_t n_dests,
    const crypto::hash8 payment_id,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    for (std::size_t i = 0; i < n_dests; ++i)
    {
        build_payment_proposals(normal_payment_proposals_inout,
            selfsend_payment_proposals_inout,
            cryptonote::tx_destination_entry(0, address, is_subaddress),
            i ? crypto::null_hash8 : payment_id,
            subaddress_map);
    }

    if (selfsend_payment_proposals_inout.size() == 2 && normal_payment_proposals_inout.empty())
        selfsend_payment_proposals_inout.back().proposal.enote_type = carrot::CarrotEnoteType::CHANGE;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static wallet2_basic::transfer_container get_transfers(const wallet2 &w)
{
    wallet2_basic::transfer_container transfers;
    w.get_transfers(transfers);
    return transfers;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static rct::xmr_amount get_fee_per_weight_from_priority(const std::uint32_t priority, wallet2 &w)
{
    const bool use_per_byte_fee = w.use_fork_rules(HF_VERSION_PER_BYTE_FEE, 0);
    CHECK_AND_ASSERT_THROW_MES(use_per_byte_fee, "not using per-byte base fee");

    const rct::xmr_amount fee_per_weight = w.get_base_fee(priority);
    MDEBUG("fee_per_weight = " << fee_per_weight << ", from priority = " << priority);

    return fee_per_weight;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::uint64_t get_top_block_index(const wallet2 &w)
{
    const std::uint64_t current_chain_height = w.get_blockchain_current_height();
    CHECK_AND_ASSERT_THROW_MES(current_chain_height > 0, "chain height is 0; there's no top block");
    return current_chain_height - 1;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static cryptonote::tx_destination_entry make_tx_destination_entry(
    const carrot::CarrotPaymentProposalV1 &payment_proposal)
{
    cryptonote::tx_destination_entry dest = cryptonote::tx_destination_entry(payment_proposal.amount,
        {payment_proposal.destination.address_spend_pubkey, payment_proposal.destination.address_view_pubkey},
        payment_proposal.destination.is_subaddress);
    dest.is_integrated = payment_proposal.destination.payment_id != carrot::null_payment_id;
    return dest;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static cryptonote::tx_destination_entry make_tx_destination_entry(
    const carrot::CarrotPaymentProposalVerifiableSelfSendV1 &payment_proposal,
    const carrot::view_incoming_key_device &k_view_dev)
{
    const crypto::public_key base_address_pubkey = payment_proposal.subaddr_index.index.is_subaddress()
        ? payment_proposal.proposal.destination_address_spend_pubkey : rct::rct2pk(rct::G);

    crypto::public_key address_view_pubkey;
    CHECK_AND_ASSERT_THROW_MES(k_view_dev.view_key_scalar_mult_ed25519(
            base_address_pubkey,
            address_view_pubkey),
        "make_tx_destination_entry: view-key multiplication failed");

   return cryptonote::tx_destination_entry(payment_proposal.proposal.amount,
        {payment_proposal.proposal.destination_address_spend_pubkey, address_view_pubkey},
        payment_proposal.subaddr_index.index.is_subaddress());
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static crypto::public_key find_change_address_spend_pubkey(
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::uint32_t subaddr_account)
{
    const auto change_it = std::find_if(subaddress_map.cbegin(), subaddress_map.cend(),
        [subaddr_account](const auto &p) { return p.second.major == subaddr_account && p.second.minor == 0; });
    CHECK_AND_ASSERT_THROW_MES(change_it != subaddress_map.cend(),
        "find_change_address_spend_pubkey: missing change address (index "
        << subaddr_account << ",0) in subaddress map");
    const auto change_it_2 = std::find_if(std::next(change_it), subaddress_map.cend(),
        [subaddr_account](const auto &p) { return p.second.major == subaddr_account && p.second.minor == 0; });
    CHECK_AND_ASSERT_THROW_MES(change_it_2 == subaddress_map.cend(),
        "find_change_address_spend_pubkey: provided subaddress map is malformed!!! At least two spend pubkeys map to "
        "index " << subaddr_account << ",0 in the subaddress map!");
    return change_it->first;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static carrot::InputCandidate make_input_candidate(const wallet2_basic::transfer_details &td)
{
    return carrot::InputCandidate{
        .core = carrot::CarrotSelectedInput{
            .amount = td.amount(),
            .input = make_sal_opening_hint_from_transfer_details(td)
        },
        .is_pre_carrot = !carrot::is_carrot_transaction_v1(td.m_tx),
        .is_external = true, //! @TODO: derive this info from field in transfer_details
        .block_index = td.m_block_height
    };
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::unordered_map<crypto::public_key, size_t> collect_non_burned_transfers_by_onetime_address(
    const wallet2_basic::transfer_container &transfers)
{
    std::unordered_set<crypto::public_key> spent;

    std::unordered_map<crypto::public_key, size_t> best_transfer_by_ota;
    for (size_t i = 0; i < transfers.size(); ++i)
    {
        const wallet2_basic::transfer_details &td = transfers.at(i);
        const crypto::public_key ota = td.get_public_key();
        if (td.m_spent)
            spent.insert(ota);
        if (spent.count(ota))
            continue;
        const auto it = best_transfer_by_ota.find(ota);
        if (it == best_transfer_by_ota.end())
        {
            best_transfer_by_ota.emplace(ota, i);
            continue;
        }
        const wallet2_basic::transfer_details &other_td = transfers.at(it->second);
        if (td.amount() < other_td.amount())
            continue;
        else if (td.amount() > other_td.amount())
            it->second = i;
        else if (td.m_global_output_index > other_td.m_global_output_index)
            continue;
        else if (td.m_global_output_index < other_td.m_global_output_index)
            it->second = i;
    }
    return best_transfer_by_ota;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::InputCandidate> collect_carrot_input_candidate_list(
    const wallet2_basic::transfer_container &transfers,
    const std::uint32_t from_subaddr_account,
    const std::set<std::uint32_t> &from_subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index)
{
    const auto best_transfer_by_ota = collect_non_burned_transfers_by_onetime_address(transfers);

    std::vector<carrot::InputCandidate> input_candidates;
    input_candidates.reserve(transfers.size());
    for (std::size_t i = 0; i < transfers.size(); ++i)
    {
        const wallet2_basic::transfer_details &td = transfers.at(i);
        const auto best_it = best_transfer_by_ota.find(td.get_public_key());
        if (best_it == best_transfer_by_ota.cend() || best_it->second != i)
            continue;

        if (!is_transfer_usable_for_input_selection(td,
                from_subaddr_account,
                from_subaddr_indices,
                ignore_above,
                ignore_below,
                top_block_index))
            continue;

        input_candidates.push_back(make_input_candidate(td));
    }
    return input_candidates;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const std::pair<crypto::hash8, std::size_t> &payment_id,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    std::set<std::uint32_t> subtract_fee_from_outputs,
    const std::uint64_t top_block_index)
{
    CARROT_CHECK_AND_THROW(subtract_fee_from_outputs.empty() || *subtract_fee_from_outputs.crbegin() < dsts.size(),
        carrot::component_out_of_order, "subtract_fee_from_outputs index is out of bounds of destinations list");

    // build payment proposals and subtractable info
    std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    std::set<std::size_t> subtractable_normal_payment_proposals;
    std::set<std::size_t> subtractable_selfsend_payment_proposals;
    for (size_t i = 0; i < dsts.size(); ++i)
    {
        const cryptonote::tx_destination_entry dst = dsts.at(i);
        const bool is_selfsend = build_payment_proposals(normal_payment_proposals,
            selfsend_payment_proposals,
            dst,
            i == payment_id.second ? payment_id.first : crypto::null_hash8,
            subaddress_map);
        if (subtract_fee_from_outputs.count(i))
        {
            if (is_selfsend)
                subtractable_selfsend_payment_proposals.insert(selfsend_payment_proposals.size() - 1);
            else
                subtractable_normal_payment_proposals.insert(normal_payment_proposals.size() - 1);
        }
    }

    std::vector<carrot::InputCandidate> input_candidates = collect_carrot_input_candidate_list(transfers,
        subaddr_account,
        subaddr_indices,
        ignore_above,
        ignore_below,
        top_block_index);

    const carrot::input_selection_policy_t isp = &carrot::ispolicy::select_greedy_aging;

    const std::uint32_t input_selection_flags = carrot::InputSelectionFlags::ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS
        | carrot::InputSelectionFlags::ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS; //! @TODO: check flags make sense

    // change spec
    const crypto::public_key change_address_spend_pubkey
        = find_change_address_spend_pubkey(subaddress_map, subaddr_account);
    const carrot::subaddress_index_extended change_address_index{
        .index = {subaddr_account, 0},
        .derive_type = carrot::AddressDeriveType::PreCarrot //! @TODO: handle Carrot keys
    };

    std::vector<carrot::CarrotTransactionProposalV1> tx_proposals;
    carrot::make_multiple_carrot_transaction_proposals_transfer(std::move(normal_payment_proposals),
        std::move(selfsend_payment_proposals),
        fee_per_weight,
        extra,
        std::move(input_candidates),
        {&isp, 1},
        input_selection_flags,
        change_address_spend_pubkey,
        change_address_index,
        subtractable_normal_payment_proposals,
        subtractable_selfsend_payment_proposals,
        tx_proposals);
    return tx_proposals;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_transfer(
    wallet2 &w,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const std::pair<crypto::hash8, std::size_t> &payment_id,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const std::set<std::uint32_t> &subtract_fee_from_outputs)
{
    return make_carrot_transaction_proposals_wallet2_transfer(
        get_transfers(w),
        w.get_subaddress_map_ref(),
        dsts,
        payment_id,
        get_fee_per_weight_from_priority(priority, w),
        extra,
        subaddr_account,
        subaddr_indices,
        w.ignore_outputs_above(),
        w.ignore_outputs_below(),
        subtract_fee_from_outputs,
        get_top_block_index(w));
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const crypto::hash8 payment_id,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint64_t top_block_index)
{
    // build payment proposals
    std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    build_payment_proposals_sweep(normal_payment_proposals,
        selfsend_payment_proposals,
        address,
        is_subaddress,
        n_dests_per_tx,
        payment_id,
        subaddress_map);

    // collect non-burned, non-spent transfers by key image
    const auto best_transfers_by_ota = collect_non_burned_transfers_by_onetime_address(transfers);
    std::unordered_map<crypto::key_image, std::size_t> best_transfers_by_ki;
    for (const auto &p : best_transfers_by_ota)
    {
        const wallet2_basic::transfer_details &td = transfers.at(p.second);
        if (!td.m_key_image_known || td.m_key_image_partial)
            continue;
        best_transfers_by_ki.emplace(td.m_key_image, p.second);
    }

    // collect `carrot::CarrotSelectedInput`'s and set subaddr account
    std::vector<carrot::CarrotSelectedInput> selected_inputs;
    selected_inputs.reserve(input_key_images.size());
    std::uint32_t subaddr_account = 0;
    for (const crypto::key_image &ki : input_key_images)
    {
        const auto ki_it = best_transfers_by_ki.find(ki);
        CARROT_CHECK_AND_THROW(ki_it != best_transfers_by_ki.cend(),
            carrot::missing_components, "unknown, partial, or spent key image: " << ki);
        const wallet2_basic::transfer_details &td = transfers.at(ki_it->second);
        selected_inputs.push_back(make_input_candidate(td).core);
        subaddr_account = std::max(subaddr_account, td.m_subaddr_index.major);
    }

    // change spec
    const crypto::public_key change_address_spend_pubkey
        = find_change_address_spend_pubkey(subaddress_map, subaddr_account);
    const carrot::subaddress_index_extended change_address_index{
        .index = {subaddr_account, 0},
        .derive_type = carrot::AddressDeriveType::PreCarrot //! @TODO: handle Carrot keys
    };

    std::vector<carrot::CarrotTransactionProposalV1> tx_proposals;
    carrot::make_multiple_carrot_transaction_proposals_sweep(normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::move(selected_inputs),
        change_address_spend_pubkey,
        change_address_index,
        /*ignore_dust=*/false,
        tx_proposals);
    return tx_proposals;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep(
    wallet2 &w,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const crypto::hash8 payment_id,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra)
{
    return make_carrot_transaction_proposals_wallet2_sweep(
        get_transfers(w),
        w.get_subaddress_map_ref(),
        input_key_images,
        address,
        is_subaddress,
        n_dests_per_tx,
        payment_id,
        get_fee_per_weight_from_priority(priority, w),
        extra,
        get_top_block_index(w));
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep_all(
    const wallet2_basic::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const rct::xmr_amount only_below,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const crypto::hash8 payment_id,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const std::uint64_t top_block_index)
{
    // build payment proposals
    std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    build_payment_proposals_sweep(normal_payment_proposals,
        selfsend_payment_proposals,
        address,
        is_subaddress,
        n_dests_per_tx,
        payment_id,
        subaddress_map);

    // collect all input candidates into `carrot::CarrotSelectedInput`'s
    std::vector<carrot::InputCandidate> input_candidates = collect_carrot_input_candidate_list(transfers,
        subaddr_account,
        subaddr_indices,
        only_below ? only_below : MONEY_SUPPLY,
        0,
        top_block_index);
    std::vector<carrot::CarrotSelectedInput> selected_inputs;
    selected_inputs.reserve(input_candidates.size());
    for (const carrot::InputCandidate &input_candidate: input_candidates)
        selected_inputs.push_back(input_candidate.core);

    // change spec
    const crypto::public_key change_address_spend_pubkey
        = find_change_address_spend_pubkey(subaddress_map, subaddr_account);
    const carrot::subaddress_index_extended change_address_index{
        .index = {subaddr_account, 0},
        .derive_type = carrot::AddressDeriveType::PreCarrot //! @TODO: handle Carrot keys
    };

    std::vector<carrot::CarrotTransactionProposalV1> tx_proposals;
    carrot::make_multiple_carrot_transaction_proposals_sweep(normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::move(selected_inputs),
        change_address_spend_pubkey,
        change_address_index,
        /*ignore_dust=*/true,
        tx_proposals);
    return tx_proposals;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<carrot::CarrotTransactionProposalV1> make_carrot_transaction_proposals_wallet2_sweep_all(
    wallet2 &w,
    const rct::xmr_amount only_below,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests_per_tx,
    const crypto::hash8 payment_id,
    const std::uint32_t priority,
    const std::vector<uint8_t> &extra,
    const std::uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices)
{
    return make_carrot_transaction_proposals_wallet2_sweep_all(
        get_transfers(w),
        w.get_subaddress_map_ref(),
        only_below,
        address,
        is_subaddress,
        n_dests_per_tx,
        payment_id,
        get_fee_per_weight_from_priority(priority, w),
        extra,
        subaddr_account,
        subaddr_indices,
        get_top_block_index(w));
}
//-------------------------------------------------------------------------------------------------------------------
carrot::OutputOpeningHintVariant make_sal_opening_hint_from_transfer_details(const wallet2_basic::transfer_details &td)
{
    // j
    const carrot::subaddress_index subaddr_index = {td.m_subaddr_index.major, td.m_subaddr_index.minor};

    const bool is_carrot = carrot::is_carrot_transaction_v1(td.m_tx);
    if (is_carrot)
    {
        std::vector<mx25519_pubkey> enote_ephemeral_pubkeys;
        std::optional<carrot::encrypted_payment_id_t> encrypted_payment_id;
        CHECK_AND_ASSERT_THROW_MES(carrot::try_load_carrot_extra_v1(td.m_tx.extra,
                enote_ephemeral_pubkeys,
                encrypted_payment_id),
            __func__ << ": failed to parse carrot tx extra");
        CHECK_AND_ASSERT_THROW_MES(!enote_ephemeral_pubkeys.empty(),
            __func__ << ": BUG: missing ephemeral pubkeys");

        const size_t ephemeral_pubkey_index = std::min<size_t>(td.m_internal_output_index,
            enote_ephemeral_pubkeys.size() - 1);
        const mx25519_pubkey &enote_ephemeral_pubkey = enote_ephemeral_pubkeys.at(ephemeral_pubkey_index);

        CHECK_AND_ASSERT_THROW_MES(!td.m_tx.vin.empty(),
            __func__ << ": carrot tx prefix missing inputs");
        const cryptonote::txin_v &first_in = td.m_tx.vin.at(0);

        CARROT_CHECK_AND_THROW(td.m_internal_output_index < td.m_tx.vout.size(),
            carrot::too_few_outputs, "internal output index in transfer details is out of range of tx prefix");
        const cryptonote::tx_out &out = td.m_tx.vout.at(td.m_internal_output_index);
        const auto &carrot_out = boost::get<cryptonote::txout_to_carrot_v1>(out.target);

        const bool is_coinbase = cryptonote::is_coinbase(td.m_tx);
        const rct::xmr_amount expected_cleartext_amount = is_coinbase ? td.amount() : 0;
        CHECK_AND_ASSERT_THROW_MES(out.amount == expected_cleartext_amount,
            __func__ << ": output cleartext amount mismatch");

        if (is_coinbase)
        {
            const uint64_t block_index = boost::get<cryptonote::txin_gen>(first_in).height;
            const carrot::CarrotCoinbaseEnoteV1 coinbase_enote{
                .onetime_address = carrot_out.key,
                .amount = td.amount(),
                .anchor_enc = carrot_out.encrypted_janus_anchor,
                .view_tag = carrot_out.view_tag,
                .enote_ephemeral_pubkey = enote_ephemeral_pubkey,
                .block_index = block_index
            };

            return carrot::CarrotCoinbaseOutputOpeningHintV1{
                .source_enote = coinbase_enote,
                .derive_type = carrot::AddressDeriveType::PreCarrot //! @TODO:
            };
        }
        else // !is_coinbase
        {
            CHECK_AND_ASSERT_THROW_MES(first_in.type() == typeid(cryptonote::txin_to_key),
                __func__ << ": unrecognized input type for carrot v1 tx");
            const crypto::key_image &tx_first_key_image = boost::get<cryptonote::txin_to_key>(first_in).k_image;

            // recreate amount parts of the enote and pass as non-coinbase carrot enote v1

            // C_a = z G + a H
            const rct::key amount_commitment = rct::commit(td.amount(), td.m_mask);

            return carrot::CarrotOutputOpeningHintV2{
                .onetime_address = carrot_out.key,
                .amount_commitment = amount_commitment,
                .anchor_enc = carrot_out.encrypted_janus_anchor,
                .view_tag = carrot_out.view_tag,
                .enote_ephemeral_pubkey = enote_ephemeral_pubkey,
                .tx_first_key_image = tx_first_key_image,
                .amount = td.amount(),
                .encrypted_payment_id = encrypted_payment_id,
                .subaddr_index = {subaddr_index, carrot::AddressDeriveType::PreCarrot } //! @TODO:
            };
        }
    }
    else // !is_carrot
    {
        // choose R
        const crypto::public_key main_tx_pubkey = cryptonote::get_tx_pub_key_from_extra(td.m_tx, td.m_pk_index);
        const std::vector<crypto::public_key> additional_tx_pubkeys =
            cryptonote::get_additional_tx_pub_keys_from_extra(td.m_tx);
        const crypto::public_key &ephemeral_tx_pubkey =
            (additional_tx_pubkeys.size() > td.m_internal_output_index && !td.m_subaddr_index.is_zero())
                ? additional_tx_pubkeys.at(td.m_internal_output_index)
                : main_tx_pubkey;

        return carrot::LegacyOutputOpeningHintV1{
            .onetime_address = td.get_public_key(),
            .ephemeral_tx_pubkey = ephemeral_tx_pubkey,
            .subaddr_index = subaddr_index,
            .amount = td.amount(),
            .amount_blinding_factor = td.m_mask,
            .local_output_index = static_cast<std::size_t>(td.m_internal_output_index)
        };

        ASSERT_MES_AND_THROW("make sal opening hint from transfer details: cannot find subaddress and sender extension "
            "for given transfer info");
    }
}
//-------------------------------------------------------------------------------------------------------------------
std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sign_carrot_transaction_proposal_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const wallet2_basic::transfer_container &transfers,
    const cryptonote::account_keys &acc_keys)
{
    const size_t n_inputs = tx_proposal.input_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(rerandomized_outputs.size() == n_inputs,
        __func__ << ": wrong size for rerandomized_outputs");

    //! @TODO: carrot hierarchy
    const carrot::cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        acc_keys.m_account_address.m_spend_public_key,
        acc_keys.m_view_secret_key);
    const carrot::hybrid_hierarchy_address_device_composed hybrid_addr_dev(&addr_dev, nullptr);
    const carrot::generate_image_key_ram_borrowed_device legacy_spend_image_dev(acc_keys.m_spend_secret_key);
    const carrot::key_image_device_composed key_image_dev(legacy_spend_image_dev,
        hybrid_addr_dev,
        nullptr,
        &addr_dev);

    crypto::hash signable_tx_hash;
    carrot::make_signable_tx_hash_from_proposal_v1(tx_proposal,
        /*s_view_balance_dev=*/nullptr,
        &addr_dev,
        key_image_dev,
        signable_tx_hash);

    std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sal_proofs_by_ki;
    for (size_t input_idx = 0; input_idx < n_inputs; ++input_idx)
    {
        //! @TODO: spend device
        fcmp_pp::FcmpPpSalProof sal_proof;
        crypto::key_image actual_key_image;
        carrot::make_sal_proof_any_to_legacy_v1(signable_tx_hash,
            rerandomized_outputs.at(input_idx),
            tx_proposal.input_proposals.at(input_idx),
            acc_keys.m_spend_secret_key,
            addr_dev,
            sal_proof,
            actual_key_image);
        sal_proofs_by_ki.emplace(actual_key_image, std::move(sal_proof));
    }

    return sal_proofs_by_ki;
}
//-------------------------------------------------------------------------------------------------------------------
cryptonote::transaction finalize_all_proofs_from_transfer_details(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2_basic::transfer_container &transfers,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys)
{
    const size_t n_inputs = tx_proposal.input_proposals.size();
    const size_t n_outputs = tx_proposal.normal_payment_proposals.size()
        + tx_proposal.selfsend_payment_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(n_inputs, __func__ << ": no inputs");

    LOG_PRINT_L2(__func__ << ": make all proofs for transaction proposal: "
        << n_inputs << "-in " << n_outputs << "-out, with "
        << tx_proposal.normal_payment_proposals.size() << " normal payment proposals, "
        << tx_proposal.selfsend_payment_proposals.size() << " self-send payment proposals, and a fee of "
        << tx_proposal.fee << " pXMR");

    // collect core selfsend proposals
    std::vector<carrot::CarrotPaymentProposalSelfSendV1> selfsend_payment_proposal_cores;
    selfsend_payment_proposal_cores.reserve(tx_proposal.selfsend_payment_proposals.size());
    for (const auto &selfsend_payment_proposal : tx_proposal.selfsend_payment_proposals)
        selfsend_payment_proposal_cores.push_back(selfsend_payment_proposal.proposal);

    //! @TODO: carrot hierarchy / HW device
    const carrot::cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        acc_keys.m_account_address.m_spend_public_key,
        acc_keys.m_view_secret_key);
    const carrot::hybrid_hierarchy_address_device_composed hybrid_addr_dev(&addr_dev, nullptr);
    const carrot::generate_image_key_ram_borrowed_device legacy_spend_image_dev(acc_keys.m_spend_secret_key);
    const carrot::key_image_device_composed key_image_dev(legacy_spend_image_dev,
        hybrid_addr_dev,
        nullptr,
        &addr_dev);

    // finalize key images
    std::vector<crypto::key_image> sorted_input_key_images;
    std::vector<std::size_t> key_image_order;
    carrot::get_sorted_input_key_images_from_proposal_v1(tx_proposal,
        key_image_dev,
        sorted_input_key_images,
        &key_image_order);

    // finalize enotes
    LOG_PRINT_L3("Getting output enote proposals");
    std::vector<carrot::RCTOutputEnoteProposal> output_enote_proposals;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    carrot::get_output_enote_proposals(tx_proposal.normal_payment_proposals,
        selfsend_payment_proposal_cores,
        tx_proposal.dummy_encrypted_payment_id,
        /*s_view_balance_dev=*/nullptr, //! @TODO: internal
        &addr_dev,
        sorted_input_key_images.at(0),
        output_enote_proposals,
        encrypted_payment_id);
    CHECK_AND_ASSERT_THROW_MES(output_enote_proposals.size() == n_outputs,
        __func__ << ": unexpected number of output enote proposals");

    // collect all non-burned inputs owned by wallet
    const auto best_transfers_by_ota = collect_non_burned_transfers_by_onetime_address(transfers);
    LOG_PRINT_L3("Did a burning bug pass, eliminated " << (transfers.size() - best_transfers_by_ota.size())
        << " eligible transfers");

    // collect output amount blinding factors
    std::vector<rct::key> output_amount_blinding_factors;
    output_amount_blinding_factors.reserve(output_enote_proposals.size());
    for (const carrot::RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
        output_amount_blinding_factors.push_back(rct::sk2rct(output_enote_proposal.amount_blinding_factor));

    // collect input onetime addresses, amount commitments, and blinding factors
    std::vector<crypto::public_key> input_onetime_addresses;
    std::vector<rct::key> input_amount_commitments;
    std::vector<rct::key> input_amount_blinding_factors;
    input_onetime_addresses.reserve(n_inputs);
    input_amount_commitments.reserve(n_inputs);
    input_amount_blinding_factors.reserve(n_inputs);
    for (const std::size_t &input_proposal_idx : key_image_order)
    {
        const carrot::InputProposalV1 &input_proposal = tx_proposal.input_proposals.at(input_proposal_idx);
        const auto ota_it = best_transfers_by_ota.find(onetime_address_ref(input_proposal));
        CHECK_AND_ASSERT_THROW_MES(ota_it != best_transfers_by_ota.cend(),
            __func__ << ": cannot find transfer by onetime address");
        const size_t transfer_idx = ota_it->second;
        CHECK_AND_ASSERT_THROW_MES(transfer_idx < transfers.size(),
            __func__ << ": transfer index out of range");
        const wallet2_basic::transfer_details &td = transfers.at(transfer_idx);
        const fcmp_pp::curve_trees::OutputPair input_pair = td.get_output_pair();
        input_onetime_addresses.push_back(input_pair.output_pubkey);
        input_amount_commitments.push_back(input_pair.commitment);
        input_amount_blinding_factors.push_back(td.m_mask);
    }

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    carrot::make_carrot_rerandomized_outputs_nonrefundable(input_onetime_addresses,
        input_amount_commitments,
        input_amount_blinding_factors,
        output_amount_blinding_factors,
        rerandomized_outputs);
    
    // collect FCMP paths
    std::vector<fcmp_pp::curve_trees::CurveTreesV1::Path> fcmp_paths;
    fcmp_paths.reserve(n_inputs);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const fcmp_pp::curve_trees::OutputPair input_pair{input_onetime_addresses.at(i),
            input_amount_commitments.at(i)};

        MDEBUG("Requesting FCMP path from tree cache for onetime address " << input_pair.output_pubkey);
        fcmp_pp::curve_trees::CurveTreesV1::Path &fcmp_path = fcmp_paths.emplace_back();
        CHECK_AND_ASSERT_THROW_MES(tree_cache.get_output_path(input_pair, fcmp_path),
            "finalize_all_proofs_from_transfer_details: failed to get FCMP path from tree cache");
        CHECK_AND_ASSERT_THROW_MES(!fcmp_path.empty(), "finalize_all_proofs_from_transfer_details: FCMP path is empty");
        assert(curve_trees.audit_path(fcmp_path, input_pair, tree_cache.get_n_leaf_tuples()));
    }

    // assert dimension properties of FCMP paths
    CHECK_AND_ASSERT_THROW_MES(fcmp_paths.size(), "finalize_all_proofs_from_transfer_details: missing FCMP paths");
    const auto &first_fcmp_path = fcmp_paths.at(0);
    for (const auto &fcmp_path : fcmp_paths)
    {
        CHECK_AND_ASSERT_THROW_MES(!fcmp_path.c1_layers.empty(),
            "finalize_all_proofs_from_transfer_details: missing hashes");
        CHECK_AND_ASSERT_THROW_MES(fcmp_path.c1_layers.size() == first_fcmp_path.c1_layers.size(),
            "finalize_all_proofs_from_transfer_details: FCMP path c1 layers mismatch");
        CHECK_AND_ASSERT_THROW_MES(fcmp_path.c2_layers.size() == first_fcmp_path.c2_layers.size(),
            "finalize_all_proofs_from_transfer_details: FCMP path c2 layers mismatch");
    }
    const size_t n_tree_layers = first_fcmp_path.c1_layers.size() + first_fcmp_path.c2_layers.size();
    const bool root_is_c1 = n_tree_layers % 2 == 1;
    const size_t num_c1_blinds = first_fcmp_path.c1_layers.size() - size_t(root_is_c1);
    const size_t num_c2_blinds = first_fcmp_path.c2_layers.size() - size_t(!root_is_c1);
    LOG_PRINT_L3("n_tree_layers: " << n_tree_layers);
    LOG_PRINT_L3("num_c1_blinds: " << num_c1_blinds);
    LOG_PRINT_L3("num_c2_blinds: " << num_c2_blinds);

    // start threadpool and waiter
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter pre_membership_waiter(tpool);

    LOG_PRINT_L3("Starting proof jobs...");
    LOG_PRINT_L3("Will submit a total of " << n_inputs * (4 + num_c1_blinds + num_c2_blinds) << " blind calculations");

    // Submit blinds calculation jobs
    std::vector<fcmp_pp::BlindedOBlind> blinded_o_blinds(n_inputs);
    std::vector<fcmp_pp::BlindedIBlind> blinded_i_blinds(n_inputs);
    std::vector<fcmp_pp::BlindedIBlindBlind> blinded_i_blind_blinds(n_inputs);
    std::vector<fcmp_pp::BlindedCBlind> blinded_c_blinds(n_inputs);

    std::vector<fcmp_pp::SeleneBranchBlind> flat_selene_branch_blinds(num_c1_blinds * n_inputs);
    std::vector<fcmp_pp::HeliosBranchBlind> flat_helios_branch_blinds(num_c2_blinds * n_inputs);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const FcmpRerandomizedOutputCompressed &rerandomized_output = rerandomized_outputs.at(i);
        tpool.submit(&pre_membership_waiter, [&rerandomized_output, &blinded_o_blinds, i]() {
            PERF_TIMER(blind_o_blind);
            blinded_o_blinds[i] = fcmp_pp::blind_o_blind(fcmp_pp::o_blind(rerandomized_output));});
        tpool.submit(&pre_membership_waiter, [&rerandomized_output, &blinded_i_blinds, i]() {
            PERF_TIMER(blind_i_blind);
            blinded_i_blinds[i] = fcmp_pp::blind_i_blind(fcmp_pp::i_blind(rerandomized_output));});
        tpool.submit(&pre_membership_waiter, [&rerandomized_output, &blinded_i_blind_blinds, i]() {
            PERF_TIMER(blind_i_blind_blind);
            blinded_i_blind_blinds[i] = fcmp_pp::blind_i_blind_blind(fcmp_pp::i_blind_blind(rerandomized_output));});
        tpool.submit(&pre_membership_waiter, [&rerandomized_output, &blinded_c_blinds, i]() {
            PERF_TIMER(blind_c_blind);
            blinded_c_blinds[i] = fcmp_pp::blind_c_blind(fcmp_pp::c_blind(rerandomized_output));});
        for (size_t j = 0; j < num_c1_blinds; ++j)
        {
            tpool.submit(&pre_membership_waiter, [&flat_selene_branch_blinds, num_c1_blinds, i, j]() {
                PERF_TIMER(selene_branch_blind);
                flat_selene_branch_blinds[(i * num_c1_blinds) + j] = fcmp_pp::gen_selene_branch_blind();});
        }
        for (size_t j = 0; j < num_c2_blinds; ++j)
        {
            tpool.submit(&pre_membership_waiter, [&flat_helios_branch_blinds, num_c2_blinds, i, j]() {
                PERF_TIMER(helios_branch_blind);
                flat_helios_branch_blinds[(i * num_c2_blinds) + j] = fcmp_pp::gen_helios_branch_blind();});
        }
    }

    // Submit BP+ job
    rct::BulletproofPlus bpp;
    tpool.submit(&pre_membership_waiter, [&output_enote_proposals, &output_amount_blinding_factors, &bpp]() {
        PERF_TIMER(bulletproof_plus_PROVE);
        std::vector<rct::xmr_amount> output_amounts;
        output_amounts.reserve(output_enote_proposals.size());
        for (const carrot::RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
            output_amounts.push_back(output_enote_proposal.amount);
        bpp = rct::bulletproof_plus_PROVE(output_amounts, output_amount_blinding_factors);
    });

    // Submit SA/L jobs
    //! @TODO: parallelize jobs
    std::vector<fcmp_pp::FcmpPpSalProof> sal_proofs(n_inputs);
    tpool.submit(&pre_membership_waiter, 
        [&tx_proposal, &sorted_input_key_images, &rerandomized_outputs, &transfers, &acc_keys, &sal_proofs, &key_image_order]() {
            // hacky: permutate tx proposal input proposals in key image order
            carrot::CarrotTransactionProposalV1 tx_proposal_sorted_ins = tx_proposal;
            tools::apply_permutation(key_image_order, tx_proposal_sorted_ins.input_proposals);

            PERF_TIMER(sign_carrot_transaction_proposal_from_transfer_details);
            std::unordered_map<crypto::key_image, fcmp_pp::FcmpPpSalProof> sal_proofs_by_ki =
                sign_carrot_transaction_proposal_from_transfer_details(
                    tx_proposal_sorted_ins, rerandomized_outputs, transfers, acc_keys);

            CHECK_AND_ASSERT_THROW_MES(sal_proofs.size() == sorted_input_key_images.size(),
                __func__ << ": bad SA/L result buffer size");

            for (size_t i = 0; i < sal_proofs.size(); ++i)
            {
                const crypto::key_image &ki = sorted_input_key_images.at(i);
                const auto sal_it = sal_proofs_by_ki.find(ki);
                CHECK_AND_ASSERT_THROW_MES(sal_it != sal_proofs_by_ki.end(),
                    __func__ << ": missing SA/L proof");
                CHECK_AND_ASSERT_THROW_MES(sal_it->second.size() == FCMP_PP_SAL_PROOF_SIZE_V1,
                    __func__ << ": unexpected SA/L proof size");
                sal_proofs[i] = std::move(sal_it->second);
                sal_proofs_by_ki.erase(sal_it);
            }
        });

    const uint64_t n_tree_synced_blocks = tree_cache.n_synced_blocks();
    CHECK_AND_ASSERT_THROW_MES(n_tree_synced_blocks > 0,
        "finalize_all_proofs_from_transfer_details: no reference block");
    const uint64_t reference_block = n_tree_synced_blocks - 1;

    // collect Rust API paths
    std::vector<fcmp_pp::Path> fcmp_paths_rust;
    fcmp_paths_rust.reserve(fcmp_paths.size());
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const fcmp_pp::curve_trees::CurveTreesV1::Path &fcmp_path = fcmp_paths.at(i);
        const fcmp_pp::curve_trees::OutputPair output_pair = {input_onetime_addresses.at(i),
            input_amount_commitments.at(i)};
        const auto output_tuple = fcmp_pp::curve_trees::output_to_tuple(output_pair);

        const auto path_for_proof = curve_trees.path_for_proof(fcmp_path, output_tuple);

        const auto helios_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::HeliosT>(
            path_for_proof.c2_scalar_chunks);
        const auto selene_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::SeleneT>(
            path_for_proof.c1_scalar_chunks);

        auto path_rust = fcmp_pp::path_new({path_for_proof.leaves.data(), path_for_proof.leaves.size()},
            path_for_proof.output_idx,
            {helios_scalar_chunks.data(), helios_scalar_chunks.size()},
            {selene_scalar_chunks.data(), selene_scalar_chunks.size()});

        fcmp_paths_rust.emplace_back(std::move(path_rust));
    }

    // collect enotes
    std::vector<carrot::CarrotEnoteV1> enotes(output_enote_proposals.size());
    for (size_t i = 0; i < enotes.size(); ++i)
        enotes[i] = output_enote_proposals.at(i).enote;

    // serialize transaction
    cryptonote::transaction tx = carrot::store_carrot_to_transaction_v1(enotes,
        sorted_input_key_images,
        tx_proposal.fee,
        encrypted_payment_id);

    // wait for pre-membership jobs to complete
    LOG_PRINT_L3("Waiting on jobs...");
    CHECK_AND_ASSERT_THROW_MES(pre_membership_waiter.wait(),
        "finalize_all_proofs_from_transfer_details: some prove jobs failed");

    // collect FCMP Rust API proving structures
    std::vector<fcmp_pp::FcmpPpProveMembershipInput> membership_proving_inputs;
    membership_proving_inputs.reserve(n_inputs);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const fcmp_pp::Path &path_rust = fcmp_paths_rust.at(i);
        const fcmp_pp::OutputBlinds output_blinds = fcmp_pp::output_blinds_new(
            blinded_o_blinds.at(i),
            blinded_i_blinds.at(i),
            blinded_i_blind_blinds.at(i),
            blinded_c_blinds.at(i));

        std::vector<fcmp_pp::SeleneBranchBlind> selene_branch_blinds;
        for (size_t j = 0; j < num_c1_blinds; ++j)
        {
            const size_t flat_idx = (i * num_c1_blinds) + j;
            selene_branch_blinds.emplace_back(std::move(flat_selene_branch_blinds.at(flat_idx)));
        }

        std::vector<fcmp_pp::HeliosBranchBlind> helios_branch_blinds;
        for (size_t j = 0; j < num_c2_blinds; ++j)
        {
            const size_t flat_idx = (i * num_c2_blinds) + j;
            helios_branch_blinds.emplace_back(std::move(flat_helios_branch_blinds.at(flat_idx)));
        }

        membership_proving_inputs.push_back(fcmp_pp::fcmp_pp_prove_input_new(
            path_rust,
            output_blinds,
            selene_branch_blinds,
            helios_branch_blinds));
    }

    // prove membership
    PERF_TIMER(prove_membership);
    const fcmp_pp::FcmpMembershipProof membership_proof = fcmp_pp::prove_membership(membership_proving_inputs,
        n_tree_layers);
    PERF_TIMER_PAUSE(prove_membership);
    CHECK_AND_ASSERT_THROW_MES(membership_proof.size() == fcmp_pp::membership_proof_len(n_inputs, n_tree_layers),
        "finalize_all_proofs_from_transfer_details: unexpected FCMP membership proof length");

    // store proofs
    tx.rct_signatures.p = carrot::store_fcmp_proofs_to_rct_prunable_v1(std::move(bpp),
        rerandomized_outputs,
        sal_proofs,
        membership_proof,
        reference_block,
        n_tree_layers);
    tx.pruned = false;
    CHECK_AND_ASSERT_THROW_MES(tx.rct_signatures.p.fcmp_pp.size() == fcmp_pp::fcmp_pp_proof_len(n_inputs, n_tree_layers),
        "finalize_all_proofs_from_transfer_details: unexpected FCMP++ combined proof length");

    // get FCMP tree root from provided path
    //! @TODO: make this a method of TreeCacheV1
    LOG_PRINT_L3("Making tree root from tree cache");
    fcmp_pp::TreeRootShared fcmp_tree_root;
    if (n_tree_layers % 2 == 0)
    {
        CHECK_AND_ASSERT_THROW_MES(!first_fcmp_path.c2_layers.empty(), "missing c2 layers");
        const auto &last_layer = first_fcmp_path.c2_layers.back();
        CHECK_AND_ASSERT_THROW_MES(last_layer.size() == 1, "unexpected n elems in c2 root");
        fcmp_tree_root = fcmp_pp::helios_tree_root(last_layer.back());
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(!first_fcmp_path.c1_layers.empty(), "missing c1 layers");
        const auto &last_layer = first_fcmp_path.c1_layers.back();
        CHECK_AND_ASSERT_THROW_MES(last_layer.size() == 1, "unexpected n elems in c1 root");
        fcmp_tree_root = fcmp_pp::selene_tree_root(last_layer.back());
    }

    // expand tx
    LOG_PRINT_L3("Expanding newly signed transaction");
    CHECK_AND_ASSERT_THROW_MES(cryptonote::expand_transaction_1(tx, /*base_only=*/false),
        "finalize_all_proofs_from_transfer_details: failed to perform transaction expansion phase 1");
    const crypto::hash tx_prefix_hash = cryptonote::get_transaction_prefix_hash(tx);
    CHECK_AND_ASSERT_THROW_MES(cryptonote::Blockchain::expand_transaction_2(tx,
            tx_prefix_hash,
            /*pubkeys=*/{},
            fcmp_tree_root),
        "finalize_all_proofs_from_transfer_details: failed to perform transaction expansion phase 2");

    // verify consensus rules against cached tree root
    {
        LOG_PRINT_L3("Verifying non-input transaction consensus rules");
        PERF_TIMER(ver_non_input_consensus);
        cryptonote::tx_verification_context tvc{};
        CHECK_AND_ASSERT_THROW_MES(cryptonote::ver_non_input_consensus(tx, tvc, HF_VERSION_FCMP_PLUS_PLUS + 1),
            "finalize_all_proofs_from_transfer_details: ver_non_input_consensus() failed");
        CHECK_AND_ASSERT_THROW_MES(!tvc.m_verifivation_failed,
            "finalize_all_proofs_from_transfer_details: ver_non_input_consensus() -> true && tvc.m_verifivation_failed");
    }
    {
        LOG_PRINT_L3("Verifying SA/L transaction proofs");
        PERF_TIMER(verify_sal);
        const crypto::hash signable_tx_hash =
            rct::rct2hash(rct::get_pre_mlsag_hash(tx.rct_signatures, acc_keys.get_device()));
        for (size_t i = 0; i < n_inputs; ++i)
        {
            CHECK_AND_ASSERT_THROW_MES(fcmp_pp::verify_sal(signable_tx_hash,
                    rerandomized_outputs.at(i).input,
                    sorted_input_key_images.at(i),
                    sal_proofs.at(i)),
                "finalize_all_proofs_from_transfer_details: SA/L proof verification failed");
        }
    }
    {
        LOG_PRINT_L3("Verifying input transaction consensus rules");
        PERF_TIMER(verRctNonSemanticsSimple);
        CHECK_AND_ASSERT_THROW_MES(rct::verRctNonSemanticsSimple(tx.rct_signatures),
            "finalize_all_proofs_from_transfer_details: verRctNonSemanticsSimple() failed");
    }

    return tx;
}
//-------------------------------------------------------------------------------------------------------------------
pending_tx make_pending_carrot_tx(const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const std::vector<crypto::key_image> &sorted_input_key_images,
    const wallet2_basic::transfer_container &transfers,
    const crypto::secret_key &k_view,
    hw::device &hwdev)
{
    const std::size_t n_inputs = tx_proposal.input_proposals.size();
    const std::size_t n_outputs = tx_proposal.normal_payment_proposals.size() +
        tx_proposal.selfsend_payment_proposals.size();
    const bool shared_ephemeral_pubkey = n_outputs == 2;

    CARROT_CHECK_AND_THROW(n_inputs >= 1, carrot::too_few_inputs, "carrot tx proposal missing inputs");
    CARROT_CHECK_AND_THROW(n_outputs >= 2, carrot::too_few_outputs, "carrot tx proposal missing outputs");
    CARROT_CHECK_AND_THROW(sorted_input_key_images.size() == tx_proposal.input_proposals.size(),
        carrot::missing_components, "Wrong number of key images for this tx proposal");

    // collect non-burned transfers
    const auto best_transfer_by_ota = collect_non_burned_transfers_by_onetime_address(transfers);

    // collect selected_transfers and key_images string
    std::vector<std::size_t> selected_transfers;
    selected_transfers.reserve(n_inputs);
    std::stringstream key_images_string;
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const crypto::public_key onetime_address = onetime_address_ref(tx_proposal.input_proposals.at(i));
        const auto ota_it = best_transfer_by_ota.find(onetime_address);
        CHECK_AND_ASSERT_THROW_MES(ota_it != best_transfer_by_ota.cend(),
            "make_pending_carrot_tx: unrecognized key image in transfers list");
        selected_transfers.push_back(ota_it->second);
        if (i)
            key_images_string << ' ';
        key_images_string << sorted_input_key_images.at(i);
    }

    //! @TODO: HW device
    carrot::view_incoming_key_ram_borrowed_device k_view_dev(k_view);

    const crypto::key_image &tx_first_key_image = sorted_input_key_images.at(0);

    // calculate change_dst index based whether 2-out tx has a dummy output
    // change_dst is set to dummy in 2-out self-send, otherwise last self-send
    const bool has_2out_dummy = n_outputs == 2
        && tx_proposal.normal_payment_proposals.size() == 1
        && tx_proposal.normal_payment_proposals.at(0).amount == 0;
    CHECK_AND_ASSERT_THROW_MES(!tx_proposal.selfsend_payment_proposals.empty(),
        "make_pending_carrot_tx: carrot tx proposal missing a self-send proposal");

    // collect destinations and private tx keys for normal enotes
    //! @TODO: payment proofs for special self-send, perhaps generate d_e deterministically
    cryptonote::tx_destination_entry change_dts;
    std::vector<cryptonote::tx_destination_entry> dests;
    std::vector<crypto::secret_key> ephemeral_privkeys;
    dests.reserve(n_outputs);
    ephemeral_privkeys.reserve(n_outputs);
    for (const carrot::CarrotPaymentProposalV1 &normal_payment_proposal : tx_proposal.normal_payment_proposals)
    {
        const cryptonote::tx_destination_entry dest = make_tx_destination_entry(normal_payment_proposal);
        ephemeral_privkeys.push_back(carrot::get_enote_ephemeral_privkey(normal_payment_proposal,
            carrot::make_carrot_input_context(tx_first_key_image)));
        if (has_2out_dummy)
            change_dts = dest;
        else
            dests.push_back(dest);
    }
    for (std::size_t i = 0; i < tx_proposal.selfsend_payment_proposals.size(); ++i)
    {
        const carrot::CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal
            = tx_proposal.selfsend_payment_proposals.at(i);
        const cryptonote::tx_destination_entry dest = make_tx_destination_entry(selfsend_payment_proposal, k_view_dev);
        if (!shared_ephemeral_pubkey)
            ephemeral_privkeys.push_back(crypto::null_skey);
        if (!has_2out_dummy && i == tx_proposal.selfsend_payment_proposals.size() - 1)
            change_dts = dest;
        else
            dests.push_back(dest);
    }

    // collect subaddr account and minor indices
    const std::uint32_t subaddr_account = transfers.at(selected_transfers.at(0)).m_subaddr_index.major;
    std::set<std::uint32_t> subaddr_indices;
    for (const size_t selected_transfer : selected_transfers)
    {
        const wallet2_basic::transfer_details &td = transfers.at(selected_transfer);
        const std::uint32_t other_subaddr_account = td.m_subaddr_index.major;
        if (other_subaddr_account != subaddr_account)
        {
            MWARNING("make_pending_carrot_tx: conflicting account indices: " << subaddr_account << " vs "
                << other_subaddr_account);
        }
        subaddr_indices.insert(td.m_subaddr_index.minor);
    }

    pending_tx ptx;
    ptx.tx.set_null();
    ptx.dust = 0;
    ptx.fee = tx_proposal.fee;
    ptx.dust_added_to_fee = false;
    ptx.change_dts = change_dts;
    ptx.selected_transfers = std::move(selected_transfers);
    ptx.key_images = key_images_string.str();
    ptx.tx_key = shared_ephemeral_pubkey && !ephemeral_privkeys.empty() ? ephemeral_privkeys.at(0) : crypto::null_skey;
    if (!shared_ephemeral_pubkey)
        ptx.additional_tx_keys = std::move(ephemeral_privkeys);
    ptx.dests = std::move(dests);
    ptx.multisig_sigs = {};
    ptx.multisig_tx_key_entropy = {};
    ptx.subaddr_account = subaddr_account;
    ptx.subaddr_indices = std::move(subaddr_indices);
    ptx.construction_data = tx_proposal;
    return ptx;
}
//-------------------------------------------------------------------------------------------------------------------
crypto::hash8 get_pending_tx_payment_id(const pending_tx &ptx)
{
    struct get_pending_tx_payment_id_visitor
    {
        crypto::hash8 operator()(const PreCarrotTransactionProposal &prop) const
        {
            std::vector<cryptonote::tx_extra_field> tx_extra_fields;
            cryptonote::parse_tx_extra(prop.extra, tx_extra_fields);

            cryptonote::tx_extra_nonce extra_nonce;
            if (!cryptonote::find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
                return crypto::null_hash8;

            crypto::hash8 pid;
            if (!cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, pid))
                return crypto::null_hash8;

            return pid;
        }

        crypto::hash8 operator()(const carrot::CarrotTransactionProposalV1 &prop) const
        {
            for (const carrot::CarrotPaymentProposalV1 &normal_payment_proposal : prop.normal_payment_proposals)
                if (normal_payment_proposal.destination.payment_id != carrot::null_payment_id)
                    return carrot::raw_byte_convert<crypto::hash8>(normal_payment_proposal.destination.payment_id);

            return crypto::null_hash8;
        }
    };

    return std::visit(get_pending_tx_payment_id_visitor{}, ptx.construction_data);
}
//-------------------------------------------------------------------------------------------------------------------
pending_tx finalize_all_proofs_from_transfer_details_as_pending_tx(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2_basic::transfer_container &transfers,
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const fcmp_pp::curve_trees::CurveTreesV1 &curve_trees,
    const cryptonote::account_keys &acc_keys)
{
    //! @TODO: carrot hierarchy
    const carrot::cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        acc_keys.m_account_address.m_spend_public_key,
        acc_keys.m_view_secret_key);
    const carrot::hybrid_hierarchy_address_device_composed hybrid_addr_dev(&addr_dev, nullptr);
    const carrot::generate_image_key_ram_borrowed_device legacy_spend_image_dev(acc_keys.m_spend_secret_key);
    const carrot::key_image_device_composed key_image_dev(legacy_spend_image_dev,
        hybrid_addr_dev,
        nullptr,
        &addr_dev);

    std::vector<crypto::key_image> sorted_input_key_images;
    carrot::get_sorted_input_key_images_from_proposal_v1(tx_proposal,
        key_image_dev,
        sorted_input_key_images);

    pending_tx ptx = make_pending_carrot_tx(tx_proposal,
        sorted_input_key_images,
        transfers,
        acc_keys.m_view_secret_key,
        acc_keys.get_device());

    ptx.tx = finalize_all_proofs_from_transfer_details(tx_proposal,
        transfers,
        tree_cache,
        curve_trees,
        acc_keys);

    return ptx;
}
//-------------------------------------------------------------------------------------------------------------------
pending_tx finalize_all_proofs_from_transfer_details_as_pending_tx(
    const carrot::CarrotTransactionProposalV1 &tx_proposal,
    const wallet2 &w)
{
    return finalize_all_proofs_from_transfer_details_as_pending_tx(
        tx_proposal,
        get_transfers(w),
        w.get_tree_cache_ref(),
        w.get_curve_trees_ref(),
        w.get_account().get_keys());
}
//-------------------------------------------------------------------------------------------------------------------
std::size_t get_num_payment_id_fields_in_tx_extra(const std::vector<std::uint8_t> &tx_extra)
{
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    cryptonote::parse_tx_extra(tx_extra, tx_extra_fields); // partial parse is fine

    std::size_t n_pid_fields = 0;
    for (const cryptonote::tx_extra_field &tx_extra_field : tx_extra_fields)
    {
        const cryptonote::tx_extra_nonce *tx_nonce = boost::strict_get<cryptonote::tx_extra_nonce>(&tx_extra_field);
        if (nullptr == tx_nonce || tx_nonce->nonce.empty())
            continue;

        const char &nonce_prefix = tx_nonce->nonce.at(0);
        if (nonce_prefix == TX_EXTRA_NONCE_ENCRYPTED_PAYMENT_ID || nonce_prefix == TX_EXTRA_NONCE_PAYMENT_ID)
            ++n_pid_fields;
    }

    return n_pid_fields;
}
//-------------------------------------------------------------------------------------------------------------------
std::size_t get_num_ephemeral_tx_pubkey_fields_in_tx_extra(const std::vector<std::uint8_t> &tx_extra)
{
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    cryptonote::parse_tx_extra(tx_extra, tx_extra_fields); // partial parse is fine

    std::size_t n_ephem_fields = 0;
    for (const cryptonote::tx_extra_field &tx_extra_field : tx_extra_fields)
    {
        const auto &field_type = tx_extra_field.type();
        const bool is_tx_pubkey_field = field_type == typeid(cryptonote::tx_extra_pub_key)
                || field_type == typeid(cryptonote::tx_extra_additional_pub_keys);
        n_ephem_fields += is_tx_pubkey_field;
    }

    return n_ephem_fields;
}
//-------------------------------------------------------------------------------------------------------------------
void insert_payment_id_into_legacy_extra_unencrypted(const crypto::hash8 &payment_id,
    std::vector<std::uint8_t> &extra_inout)
{
    CARROT_CHECK_AND_THROW(get_num_payment_id_fields_in_tx_extra(extra_inout) == 0,
        std::logic_error, "tx_extra already contains a payment ID field");

    cryptonote::blobdata extra_nonce;
    cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);

    CARROT_CHECK_AND_THROW(cryptonote::add_extra_nonce_to_tx_extra(extra_inout, extra_nonce),
        std::logic_error, "failed to insert extra tx nonce");
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
