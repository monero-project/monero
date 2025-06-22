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
#include "multi_tx_proposal_utils.h"

//local headers
#include "carrot_core/config.h"
#include "carrot_core/exceptions.h"
#include "common/container_helpers.h"
#include "format_utils.h"
#include "misc_log_ex.h"

//third party headers
#include <boost/iterator/transform_iterator.hpp>

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void make_multiple_carrot_transaction_proposals_transfer(
    std::vector<CarrotPaymentProposalV1> &&normal_payment_proposals,
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &&selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<InputCandidate> &&input_candidates,
    const epee::span<const input_selection_policy_t> input_selection_policies,
    const std::uint32_t input_selection_flags,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    const std::set<std::size_t> &subtractable_normal_payment_proposals,
    const std::set<std::size_t> &subtractable_selfsend_payment_proposals,
    std::vector<CarrotTransactionProposalV1> &tx_proposals_out)
{
    tx_proposals_out.clear();

    static constexpr std::size_t max_n_dsts_per_tx = FCMP_PLUS_PLUS_MAX_OUTPUTS - 1;

    std::size_t n_dsts = normal_payment_proposals.size() + selfsend_payment_proposals.size();
    tx_proposals_out.reserve((n_dsts + max_n_dsts_per_tx - 1) / max_n_dsts_per_tx);
    while (n_dsts)
    {
        // build payment proposals and subtractable info
        std::vector<carrot::CarrotPaymentProposalV1> tx_normal_payment_proposals;
        std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> tx_selfsend_payment_proposals;
        std::set<std::size_t> tx_subtractable_normal_payment_proposals;
        std::set<std::size_t> tx_subtractable_selfsend_payment_proposals;
        std::size_t n_tx_dsts = 0;
        while (n_dsts && n_tx_dsts < max_n_dsts_per_tx)
        {
            if (normal_payment_proposals.size())
            {
                tx_normal_payment_proposals.push_back(normal_payment_proposals.back());
                normal_payment_proposals.pop_back();
                if (subtractable_normal_payment_proposals.count(normal_payment_proposals.size()))
                    tx_subtractable_normal_payment_proposals.insert(tx_normal_payment_proposals.size() - 1);
            }
            else // selfsend_payment_proposals.size()
            {
                CARROT_CHECK_AND_THROW(selfsend_payment_proposals.size(),
                    carrot::carrot_logic_error, "bug in payment proposal counting");
                tx_selfsend_payment_proposals.push_back(selfsend_payment_proposals.back());
                selfsend_payment_proposals.pop_back();
                if (subtractable_selfsend_payment_proposals.count(selfsend_payment_proposals.size()))
                    tx_subtractable_selfsend_payment_proposals.insert(tx_selfsend_payment_proposals.size() - 1);
            }

            ++n_tx_dsts;
            --n_dsts;
        }

        // make input selector
        std::set<size_t> selected_transfer_indices;
        carrot::select_inputs_func_t select_inputs = make_single_transfer_input_selector(
            epee::to_span(input_candidates),
            input_selection_policies,
            input_selection_flags,
            &selected_transfer_indices);

        // make proposal
        carrot::CarrotTransactionProposalV1 &tx_proposal = tools::add_element(tx_proposals_out);
        carrot::make_carrot_transaction_proposal_v1_transfer(
            tx_normal_payment_proposals,
            tx_selfsend_payment_proposals,
            fee_per_weight,
            extra,
            std::move(select_inputs),
            change_address_spend_pubkey,
            change_address_index,
            tx_subtractable_normal_payment_proposals,
            tx_subtractable_selfsend_payment_proposals,
            tx_proposal);

        // update `input_candidates` for next proposal by removing already-selected one-time addresses
        std::unordered_set<crypto::public_key> used_otas;
        for (const carrot::InputProposalV1 &input_proposal : tx_proposal.input_proposals)
            used_otas.insert(onetime_address_ref(input_proposal));
        tools::for_all_in_vector_erase_no_preserve_order_if(input_candidates,
            [&used_otas](const auto &ic) -> bool { return used_otas.count(onetime_address_ref(ic.core.input)); }
        );
    }
}
//-------------------------------------------------------------------------------------------------------------------
void make_multiple_carrot_transaction_proposals_sweep(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<CarrotSelectedInput> &&selected_inputs,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_address_index,
    const bool ignore_dust,
    std::vector<CarrotTransactionProposalV1> &tx_proposals_out)
{
    tx_proposals_out.clear();

    const std::size_t n_dests_per_tx = normal_payment_proposals.size() + selfsend_payment_proposals.size();
    CARROT_CHECK_AND_THROW(selected_inputs.size(), carrot::too_few_inputs, "no inputs provided");
    CARROT_CHECK_AND_THROW(n_dests_per_tx, carrot::too_few_outputs, "sweep must have at least one destination");
    CARROT_CHECK_AND_THROW(n_dests_per_tx <= FCMP_PLUS_PLUS_MAX_OUTPUTS,
        carrot::too_many_outputs, "too many sweep destinations per transaction");

    // Check for duplicate one-time addresses in `selected_inputs`
    std::unordered_set<crypto::public_key> input_onetime_address;
    for (const CarrotSelectedInput &selected_input : selected_inputs)
        input_onetime_address.insert(onetime_address_ref(selected_input.input));
    CARROT_CHECK_AND_THROW(input_onetime_address.size() == selected_inputs.size(),
        component_out_of_order, "found duplicate one-time addressess when sweeping");

    // Sort `selected_inputs` by amount, descending. We do this so that we can try to pair
    // small inputs with big inputs so that hopefully each tx can pay for its own fees
    std::sort(selected_inputs.begin(), selected_inputs.end(),
        [](const carrot::CarrotSelectedInput &a, const carrot::CarrotSelectedInput &b) { return a.amount > b.amount; });

    const std::size_t n_outputs = std::max<std::size_t>(CARROT_MIN_TX_OUTPUTS, normal_payment_proposals.size()
        + std::max<std::size_t>(1, selfsend_payment_proposals.size()));
    
    const std::map<std::size_t, rct::xmr_amount> fee_by_input_count = get_fee_by_input_count(n_outputs, extra.size(), fee_per_weight);

    tx_proposals_out.reserve((selected_inputs.size() + FCMP_PLUS_PLUS_MAX_INPUTS - 1) / FCMP_PLUS_PLUS_MAX_INPUTS);

    // callback for calling `get_input_count_for_max_usable_money()` on some slice of inputs starting at `window_offset`
    const auto get_input_count_for_max_usable_money_in_window = 
        [&selected_inputs, &fee_by_input_count](const std::size_t window_offset)
    {
            CARROT_CHECK_AND_THROW(window_offset <= selected_inputs.size(),
                carrot_logic_error, "selected_offset out of range");
            const std::size_t window_size = std::min<std::size_t>(CARROT_MAX_TX_INPUTS,
                selected_inputs.size() - window_offset);

            const auto deref_amount = [](const CarrotSelectedInput &i) { return i.amount; };

            const auto window_begin = selected_inputs.cbegin() + window_offset;
            const auto window_end = window_begin + window_size;

            return carrot::get_input_count_for_max_usable_money(
                boost::make_transform_iterator(window_begin, deref_amount),
                boost::make_transform_iterator(window_end, deref_amount),
                window_size,
                fee_by_input_count).first;
    };

    // To try to maximize the total amount of money sent in a sweep, we first order the amounts in
    // ascending order. Then from low to high, we slide a "window" over a slice of contiguous
    // amounts and call `get_input_count_for_max_usable_money_in_window` over that window. That call
    // will tell how many of the inputs in that window are dusty. If any are dusty, we should keep
    // sliding the window until we can't. Once that window stops, we use that window for the inputs
    // for the current transaction. Repeat until no inputs are left or only dusty inputs are left.

    // while some selection of inputs of the highest amounts at some input count yields a net positive output sum...
    while (get_input_count_for_max_usable_money_in_window(0))
    {
        // slide a window in ascending amount order until all inputs in that window yield a net positive output sum...
        const std::size_t max_window_size = std::min<std::size_t>(CARROT_MAX_TX_INPUTS, selected_inputs.size());
        std::size_t window_offset = selected_inputs.size() - max_window_size;
        std::size_t n_tx_inputs = 0;
        do
        {
            n_tx_inputs = get_input_count_for_max_usable_money_in_window(window_offset);
            if (0 == window_offset || max_window_size == n_tx_inputs)
                break;
            --window_offset;
        }
        while (1);

        // if that doesn't happen, stop early
        if (!n_tx_inputs)
            break;

        // calculate iterators into `selected_inputs` based on the input count returned by `get_input_count_for_max_...`
        const std::size_t window_offset_end = window_offset + n_tx_inputs;
        CARROT_CHECK_AND_THROW(window_offset_end <= selected_inputs.size(),
            carrot_logic_error, "BUG: window_offset_end out of range");
        const auto tx_input_begin = selected_inputs.cbegin() + window_offset;
        const auto tx_input_end = selected_inputs.cbegin() + window_offset_end;

        // pop those selected inputs out of `selected_inputs` into `tx_selected_inputs`
        std::vector<CarrotSelectedInput> tx_selected_inputs;
        tx_selected_inputs.reserve(window_offset_end - window_offset);
        tx_selected_inputs.insert(tx_selected_inputs.cend(), tx_input_begin, tx_input_end);
        selected_inputs.erase(tx_input_begin, tx_input_end);

        carrot::make_carrot_transaction_proposal_v1_sweep(normal_payment_proposals,
            selfsend_payment_proposals,
            fee_per_weight,
            extra,
            std::move(tx_selected_inputs),
            change_address_spend_pubkey,
            change_address_index,
            tools::add_element(tx_proposals_out));
    }

    CARROT_CHECK_AND_THROW(ignore_dust || selected_inputs.empty(),
        carrot::not_enough_usable_money, "some inputs couldn't pay for their own marginal fee and `ignore_dust`=false");
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
