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
#include "carrot_impl/carrot_tx_builder_inputs.h"
#include "carrot_impl/carrot_tx_builder_utils.h"
#include "carrot_impl/input_selection.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

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
static bool is_transfer_unlocked_for_next_fcmp_pp_block(const wallet2::transfer_details &td,
    const uint64_t top_block_index)
{
    const uint64_t next_block_index = top_block_index + 1;

    // @TODO: handle FCMP++ conversion of UNIX unlock time to block index number

    if (td.m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > next_block_index)
        return false;

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool is_transfer_usable_for_input_selection(const wallet2::transfer_details &td,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const uint64_t top_block_index)
{
    return !td.m_spent
        && td.m_key_image_known
        && !td.m_key_image_partial
        && !td.m_frozen
        && is_transfer_unlocked_for_next_fcmp_pp_block(td, top_block_index)
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
            .is_subaddress = tx_dest_entry.is_subaddress
            //! @TODO: payment ID 
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
std::unordered_map<crypto::key_image, size_t> collect_non_burned_transfers_by_key_image(
    const wallet2::transfer_container &transfers)
{
    std::unordered_map<crypto::key_image, size_t> best_transfer_index_by_ki;
    for (size_t i = 0; i < transfers.size(); ++i)
    {
        const wallet2::transfer_details &td = transfers.at(i);
        if (!td.m_key_image_known || td.m_key_image_partial)
            continue;
        const auto it = best_transfer_index_by_ki.find(td.m_key_image);
        if (it == best_transfer_index_by_ki.end())
        {
            best_transfer_index_by_ki.insert({td.m_key_image, i});
            break;
        }
        const wallet2::transfer_details &other_td = transfers.at(it->second);
        if (td.amount() < other_td.amount())
            continue;
        else if (td.amount() > other_td.amount())
            it->second = i;
        else if (td.m_global_output_index > other_td.m_global_output_index)
            continue;
        else if (td.m_global_output_index < other_td.m_global_output_index)
            it->second = i;
    }
    return best_transfer_index_by_ki;
}
//-------------------------------------------------------------------------------------------------------------------
carrot::select_inputs_func_t make_wallet2_single_transfer_input_selector(
    const wallet2::transfer_container &transfers,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> &from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index,
    const bool allow_carrot_external_inputs_in_normal_transfers,
    const bool allow_pre_carrot_inputs_in_normal_transfers,
    std::set<size_t> &selected_transfer_indices_out)
{
    // Collect transfer_container into a `std::vector<carrot::CarrotPreSelectedInput>` for usable inputs
    std::vector<carrot::CarrotPreSelectedInput> input_candidates;
    std::vector<size_t> input_candidates_transfer_indices;
    input_candidates.reserve(transfers.size());
    input_candidates_transfer_indices.reserve(transfers.size());
    for (size_t i = 0; i < transfers.size(); ++i)
    {
        const wallet2::transfer_details& td = transfers.at(i);
        if (is_transfer_usable_for_input_selection(td,
            from_account,
            from_subaddresses,
            ignore_above,
            ignore_below,
            top_block_index))
        {
            input_candidates.push_back(carrot::CarrotPreSelectedInput{
                .core = carrot::CarrotSelectedInput{
                    .amount = td.amount(),
                    .key_image = td.m_key_image
                },
                .is_pre_carrot = true, //! @TODO: handle post-Carrot enotes in transfer_details
                .is_external = true, //! @TODO: derive this info from field in transfer_details
                .block_index = td.m_block_height
            });
            input_candidates_transfer_indices.push_back(i);
        }
    }

    // Create wrapper around `make_single_transfer_input_selector`
    return [input_candidates = std::move(input_candidates),
            input_candidates_transfer_indices = std::move(input_candidates_transfer_indices),
            allow_carrot_external_inputs_in_normal_transfers,
            allow_pre_carrot_inputs_in_normal_transfers,
            &selected_transfer_indices_out
            ](
                const boost::multiprecision::int128_t& nominal_output_sum,
                const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count,
                const std::size_t num_normal_payment_proposals,
                const std::size_t num_selfsend_payment_proposals,
                std::vector<carrot::CarrotSelectedInput> &selected_inputs_outs
            ){
                const std::vector<carrot::input_selection_policy_t> policies{
                    &carrot::ispolicy::select_two_inputs_prefer_oldest
                }; // @TODO

                // TODO: not all carrot is internal
                std::uint32_t flags = 0;
                if (allow_carrot_external_inputs_in_normal_transfers)
                    flags |= carrot::InputSelectionFlags::ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS;
                if (allow_pre_carrot_inputs_in_normal_transfers)
                    flags |= carrot::InputSelectionFlags::ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS;

                // Make inner input selection functor
                std::set<size_t> selected_input_indices;
                const carrot::select_inputs_func_t inner = carrot::make_single_transfer_input_selector(
                    epee::to_span(input_candidates),
                    epee::to_span(policies),
                    flags,
                    &selected_input_indices);

                // Call input selection
                inner(nominal_output_sum,
                    fee_by_input_count,
                    num_normal_payment_proposals,
                    num_selfsend_payment_proposals,
                    selected_inputs_outs);

                // Collect converted selected_input_indices -> selected_transfer_indices_out
                selected_transfer_indices_out.clear();
                for (const size_t input_index : selected_input_indices)
                    selected_transfer_indices_out.insert(input_candidates_transfer_indices.at(input_index));
            };
}
//-------------------------------------------------------------------------------------------------------------------
carrot::CarrotTransactionProposalV1 make_carrot_transaction_proposal_wallet2_transfer_subtractable(
    const wallet2::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const wallet2::unique_index_container& subtract_fee_from_outputs,
    const std::uint64_t top_block_index,
    const cryptonote::account_base &acb)
{
    // build payment proposals and subtractable info
    std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    std::set<std::size_t> subtractable_normal_payment_proposals;
    std::set<std::size_t> subtractable_selfsend_payment_proposals;
    for (size_t i = 0; i < dsts.size(); ++i)
    {
        const cryptonote::tx_destination_entry &dst = dsts.at(i);
        const bool is_selfsend = build_payment_proposals(normal_payment_proposals,
            selfsend_payment_proposals,
            dst,
            subaddress_map);
        if (subtract_fee_from_outputs.count(i))
        {
            if (is_selfsend)
                subtractable_selfsend_payment_proposals.insert(selfsend_payment_proposals.size() - 1);
            else
                subtractable_normal_payment_proposals.insert(normal_payment_proposals.size() - 1);
        }
    }

    // make input selector
    std::set<size_t> selected_transfer_indices;
    carrot::select_inputs_func_t select_inputs = make_wallet2_single_transfer_input_selector(
        transfers,
        subaddr_account,
        subaddr_indices,
        ignore_above,
        ignore_below,
        top_block_index,
        /*allow_carrot_external_inputs_in_normal_transfers=*/true,
        /*allow_pre_carrot_inputs_in_normal_transfers=*/true,
        selected_transfer_indices);

    //! @TODO: handle HW devices
    carrot::view_incoming_key_ram_borrowed_device k_view_incoming_dev(acb.get_keys().m_view_secret_key);

    carrot::CarrotTransactionProposalV1 tx_proposal;
    if (subtract_fee_from_outputs.size())
    {
        carrot::make_carrot_transaction_proposal_v1_transfer_subtractable(
            normal_payment_proposals,
            selfsend_payment_proposals,
            fee_per_weight,
            extra,
            std::move(select_inputs),
            /*s_view_balance_dev=*/nullptr, //! @TODO: handle carrot
            &k_view_incoming_dev,
            acb.get_keys().m_account_address.m_spend_public_key,
            subtractable_normal_payment_proposals,
            subtractable_selfsend_payment_proposals,
            tx_proposal);
    }
    else // non-subtractable
    {
        carrot::make_carrot_transaction_proposal_v1_transfer(
            normal_payment_proposals,
            selfsend_payment_proposals,
            fee_per_weight,
            extra,
            std::move(select_inputs),
            /*s_view_balance_dev=*/nullptr, //! @TODO: handle carrot
            &k_view_incoming_dev,
            acb.get_keys().m_account_address.m_spend_public_key,
            tx_proposal);
    }

    return tx_proposal;
}
//-------------------------------------------------------------------------------------------------------------------
carrot::CarrotTransactionProposalV1 make_carrot_transaction_proposal_wallet2_transfer(
    const wallet2::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<cryptonote::tx_destination_entry> &dsts,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    const uint32_t subaddr_account,
    const std::set<uint32_t> &subaddr_indices,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index,
    const cryptonote::account_base &acb)
{
    return make_carrot_transaction_proposal_wallet2_transfer_subtractable(
        transfers,
        subaddress_map,
        dsts,
        fee_per_weight,
        extra,
        subaddr_account,
        subaddr_indices,
        ignore_above,
        ignore_below,
        /*subtract_fee_from_outputs=*/{},
        top_block_index,
        acb);
}
//-------------------------------------------------------------------------------------------------------------------
carrot::CarrotTransactionProposalV1 make_carrot_transaction_proposal_wallet2_sweep(
    const wallet2::transfer_container &transfers,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const std::vector<crypto::key_image> &input_key_images,
    const cryptonote::account_public_address &address,
    const bool is_subaddress,
    const size_t n_dests,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t>& extra,
    const std::uint64_t top_block_index,
    const cryptonote::account_base &acb)
{
    CHECK_AND_ASSERT_THROW_MES(!input_key_images.empty(),
        "make carrot transaction proposal wallet2 sweep: no key images provided");
    CHECK_AND_ASSERT_THROW_MES(n_dests < FCMP_PLUS_PLUS_MAX_OUTPUTS,
        "make carrot transaction proposal wallet2 sweep: too many destinations");

    // Check that the key image is available and isn't spent, and collect amounts
    std::vector<rct::xmr_amount> input_amounts;
    input_amounts.reserve(input_key_images.size());
    const auto best_transfers_by_ki = collect_non_burned_transfers_by_key_image(transfers);
    for (const crypto::key_image &ki : input_key_images)
    {
        const auto ki_it = best_transfers_by_ki.find(ki);
        CHECK_AND_ASSERT_THROW_MES(ki_it != best_transfers_by_ki.cend(),
            "make carrot transaction proposal wallet2 sweep: unknown key image");
        const wallet2::transfer_details &td = transfers.at(ki_it->second);
        CHECK_AND_ASSERT_THROW_MES(is_transfer_usable_for_input_selection(td,
                td.m_subaddr_index.major,
                /*from_subaddresses=*/{},
                /*ignore_above=*/MONEY_SUPPLY,
                /*ignore_below=*/0,
                top_block_index),
            "make carrot transaction proposal wallet2 sweep: transfer not usable as an input");
        input_amounts.push_back(td.amount());
    }

    // build n_dests payment proposals
    std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    for (size_t i = 0; i < n_dests; ++i)
    {
        build_payment_proposals(normal_payment_proposals,
            selfsend_payment_proposals,
            cryptonote::tx_destination_entry(/*amount=*/0, address, is_subaddress),
            subaddress_map);
    }

    // Collect CarrotSelectedInput
    std::vector<carrot::CarrotSelectedInput> selected_inputs(input_key_images.size());
    for (size_t i = 0; i < input_key_images.size(); ++i)
    {
        selected_inputs[i] = carrot::CarrotSelectedInput{
            .amount = input_amounts.at(i),
            .key_image = input_key_images.at(i)
        };
    }

    //! @TODO: handle HW devices
    carrot::view_incoming_key_ram_borrowed_device k_view_incoming_dev(acb.get_keys().m_view_secret_key);

    carrot::CarrotTransactionProposalV1 tx_proposal;
    carrot::make_carrot_transaction_proposal_v1_sweep(normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::move(selected_inputs),
        /*s_view_balance_dev=*/nullptr, //! @TODO: handle carrot
        &k_view_incoming_dev,
        acb.get_keys().m_account_address.m_spend_public_key,
        tx_proposal);

    return tx_proposal;
}
//-------------------------------------------------------------------------------------------------------------------
carrot::OutputOpeningHintVariant make_sal_opening_hint_from_transfer_details(
    const wallet2::transfer_details &td,
    const crypto::secret_key &k_view,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses_map,
    hw::device &hwdev)
{
    //! @TODO: handle Carrot coinbase and non-coinbase enotes

    // K_o
    const crypto::public_key onetime_address = td.get_public_key();

    // R
    const crypto::public_key main_tx_pubkey = cryptonote::get_tx_pub_key_from_extra(td.m_tx, td.m_pk_index);
    const std::vector<crypto::public_key> additional_tx_pubkeys =
        cryptonote::get_additional_tx_pub_keys_from_extra(td.m_tx);

    //! @TODO: reject too many additional tx pubkeys??

    std::vector<crypto::key_derivation> ecdhs;
    ecdhs.reserve(additional_tx_pubkeys.size() + 1);

    // 8 * k_v * R
    crypto::key_derivation ecdh;
    if (hwdev.generate_key_derivation(main_tx_pubkey, k_view, ecdh))
        ecdhs.push_back(ecdh);

    // 8 * k_v * R
    for (const crypto::public_key &additional_tx_pubkey : additional_tx_pubkeys)
        if (hwdev.generate_key_derivation(additional_tx_pubkey, k_view, ecdh))
            ecdhs.push_back(ecdh);

    // Search for (j, K^j_s, k^g_o) s.t. K^j_s = K_o + H_n(8 * k_v * R, output_index)
    for (const crypto::key_derivation &ecdh : ecdhs)
    {
        // K^j_s' = K_o - k^g_o G
        crypto::public_key nominal_address_spend_pubkey;
        if (!hwdev.derive_subaddress_public_key(onetime_address,
                ecdh,
                td.m_internal_output_index,
                nominal_address_spend_pubkey))
            continue;

        // Know about K^j_s?
        const auto subaddr_it = subaddresses_map.find(nominal_address_spend_pubkey);
        if (subaddr_it == subaddresses_map.cend())
            continue;

        // k^g_o = H_n(8 * k_v * R, output_index)
        //! @TODO: find out if any mainline HWs "conceal" the derived scalar
        crypto::secret_key sender_extension_g;
        if (!hwdev.derivation_to_scalar(ecdh, td.m_internal_output_index, sender_extension_g))
            continue;

        // j
        const carrot::subaddress_index subaddr_index = {subaddr_it->second.major, subaddr_it->second.minor};

        return carrot::LegacyOutputOpeningHintV1{
            .onetime_address = onetime_address,
            .sender_extension_g = sender_extension_g,
            .subaddr_index = subaddr_index,
            .amount = td.amount(),
            .amount_blinding_factor = rct::rct2sk(td.m_mask)
        };
    }

    ASSERT_MES_AND_THROW("make sal opening hint from transfer details: cannot find subaddress and sender extension "
        "for given transfer info");
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
