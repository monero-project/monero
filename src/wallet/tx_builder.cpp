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
#include "carrot_impl/carrot_tx_builder_inputs.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet"

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
carrot::select_inputs_func_t make_wallet2_single_transfer_input_selector(
    const wallet2::transfer_container &transfers,
    const std::uint32_t from_account,
    const std::set<std::uint32_t> &from_subaddresses,
    const rct::xmr_amount ignore_above,
    const rct::xmr_amount ignore_below,
    const std::uint64_t top_block_index,
    const bool allow_carrot_external_inputs_in_normal_transfers,
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
                .is_external = true, // @TODO: derive this info from field in transfer_details
                .block_index = td.m_block_height
            });
            input_candidates_transfer_indices.push_back(i);
        }
    }

    // Create wrapper around `make_single_transfer_input_selector`
    return [input_candidates = std::move(input_candidates),
            input_candidates_transfer_indices = std::move(input_candidates_transfer_indices),
            allow_carrot_external_inputs_in_normal_transfers,
            &selected_transfer_indices_out
            ](
                const boost::multiprecision::int128_t& nominal_output_sum,
                const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count,
                const std::size_t num_normal_payment_proposals,
                const std::size_t num_selfsend_payment_proposals,
                std::vector<carrot::CarrotSelectedInput> &selected_inputs_outs
            ){
                const std::vector<carrot::InputSelectionPolicy> policies{
                    carrot::InputSelectionPolicy::TwoInputsPreferOldest
                }; // @TODO

                // TODO: not all carrot is internal
                const std::uint32_t flags = allow_carrot_external_inputs_in_normal_transfers
                    ? carrot::InputSelectionFlags::ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS : 0;

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
} //namespace wallet
} //namespace tools