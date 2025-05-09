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

//paired header
#include "input_selection.h"

//local headers
#include "carrot_core/config.h"
#include "carrot_core/exceptions.h"
#include "common/container_helpers.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "misc_log_ex.h"

//third party headers

//standard headers
#include <algorithm>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::ostream &operator<<(std::ostream &os, const CarrotSelectedInput &input)
{
    os << "{ key_image = '" << input.key_image << "', amount = " << cryptonote::print_money(input.amount) << " }";
    return os;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::ostream &operator<<(std::ostream &os, const CarrotPreSelectedInput &input)
{
    os << "{ core = " << input.core << ", block_index = " << input.block_index
        << ", carrot = " << !input.is_pre_carrot << ", external = " << input.is_external << " }";
    return os;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::set<size_t> set_union(const std::set<size_t> &a, const std::set<size_t> &b)
{
    std::set<size_t> c = a;
    c.merge(std::set<size_t>(b));
    return c;
};
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void stable_sort_indices_by_amount(const epee::span<const CarrotPreSelectedInput> input_candidates,
    std::vector<size_t> &indices_inout)
{
    std::stable_sort(indices_inout.begin(), indices_inout.end(),
        [input_candidates](const std::size_t a, const std::size_t b) -> bool
        {
            CARROT_CHECK_AND_THROW(a < input_candidates.size() && b < input_candidates.size(),
                std::out_of_range, "input candidate index out of range");
            return input_candidates[a].core.amount < input_candidates[b].core.amount;
        });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void stable_sort_indices_by_block_index(const epee::span<const CarrotPreSelectedInput> input_candidates,
    std::vector<size_t> &indices_inout)
{
    std::stable_sort(indices_inout.begin(), indices_inout.end(),
        [input_candidates](const std::size_t a, const std::size_t b) -> bool
        {
            CARROT_CHECK_AND_THROW(a < input_candidates.size() && b < input_candidates.size(),
                std::out_of_range, "input candidate index out of range");
            return input_candidates[a].block_index < input_candidates[b].block_index;
        });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::pair<std::size_t, boost::multiprecision::uint128_t> input_count_for_max_usable_money(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<std::size_t> &selectable_inputs,
    std::size_t max_num_input_count,
    const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count)
{
    // Returns (N, X) where the X is the sum of the amounts of the greatest N <= max_num_input_count
    // inputs from selectable_inputs, maximizing X - F(N). F(N) is the fee for this transaction,
    // given input count N. This should correctly handle "almost-dust": inputs which are less than
    // the fee, but greater than or equal to the difference of the fee compared to excluding that
    // input. If this function returns N == 0, then there aren't enough usable funds, i.e. no N
    // exists such that X - F(N) > 0.

    if (fee_by_input_count.empty() || selectable_inputs.empty())
        return {0, 0};

    max_num_input_count = std::min(max_num_input_count, selectable_inputs.size());
    CARROT_CHECK_AND_THROW(max_num_input_count <= fee_by_input_count.crbegin()->first,
        too_few_inputs, "fee by input count does not contain info for provided max input count");

    // maintain list of top amounts of selectable_inputs
    std::multiset<rct::xmr_amount> top_amounts;
    for (const std::size_t selectable_input : selectable_inputs)
    {
        CARROT_CHECK_AND_THROW(selectable_input < input_candidates.size(),
            std::out_of_range, "selectable input out of range");
        const rct::xmr_amount amount = input_candidates[selectable_input].core.amount;
        top_amounts.insert(amount);

        if (top_amounts.size() > max_num_input_count)
            top_amounts.erase(top_amounts.cbegin());
    }

    // add up all the top amounts from the greatest to least until one fails to pay for its own marginal fee
    std::size_t num_ins = 0;
    rct::xmr_amount last_fee = 0;
    boost::multiprecision::uint128_t cumulative_input_sum = 0;
    for (auto amount_it = top_amounts.crbegin(); amount_it != top_amounts.crend(); ++amount_it)
    {
        const rct::xmr_amount amount = *amount_it;
        const rct::xmr_amount current_fee = fee_by_input_count.at(num_ins + 1);
        CARROT_CHECK_AND_THROW(current_fee > last_fee,
            carrot_logic_error, "provided fee by input count is not monotonically increasing");
        const rct::xmr_amount marginal_fee_diff = current_fee - last_fee;
        if (amount <= marginal_fee_diff)
            break;
        ++num_ins;
        last_fee = current_fee;
        cumulative_input_sum += amount;
    }

    return {num_ins, cumulative_input_sum};
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
int compare_input_candidate_same_ki(const CarrotPreSelectedInput &lhs, const CarrotPreSelectedInput &rhs)
{
    CARROT_CHECK_AND_THROW(lhs.core.key_image == rhs.core.key_image,
        component_out_of_order, "this function is not meant to compare inputs of different key images");

    // First prefer the higher amount,
    if (lhs.core.amount < rhs.core.amount)
        return -1;
    else if (lhs.core.amount > rhs.core.amount)
        return 1;

    // Then prefer older,
    if (lhs.block_index < rhs.block_index)
        return 1;
    else if (lhs.block_index > rhs.block_index)
        return -1;

    // Then prefer Carrot over pre-Carrot. It should be computationally intractable for
    // lhs.is_pre_carrot != rhs.is_pre_carrot, when they both successfully scan, but I haven't
    // looked into it too deeply.
    if (lhs.is_pre_carrot && !rhs.is_pre_carrot)
        return -1;
    else if (!lhs.is_pre_carrot && rhs.is_pre_carrot)
        return 1;

    // Then prefer internal over external. Same tractability note as with is_pre_carrot.
    if (lhs.is_external && !rhs.is_external)
        return -1;
    else if (!lhs.is_external && rhs.is_external)
        return 1;

    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::set<std::size_t>> form_preferred_input_candidate_subsets(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::uint32_t flags,
    const bool is_normal_transfer)
{
    using namespace InputSelectionFlags;

    // Sanity check flags
    const bool confused_qfs = (flags & ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS) &&
        !(flags & ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS);
    CARROT_CHECK_AND_THROW(!confused_qfs, std::invalid_argument,
        "It does not make sense to allow pre-carrot inputs in normal transfers, but not external carrot inputs.");

    // 1. Compile map of best input candidates by key image to mitigate the "burning bug" for legacy enotes
    std::unordered_map<crypto::key_image, std::size_t> best_input_by_key_image;
    for (size_t i = 0; i < input_candidates.size(); ++i)
    {
        const CarrotPreSelectedInput &input_candidate = input_candidates[i];
        auto it = best_input_by_key_image.find(input_candidate.core.key_image);
        if (it == best_input_by_key_image.end())
        {
           best_input_by_key_image[input_candidate.core.key_image] = i;
        }
        else
        {
            const CarrotPreSelectedInput &other_input_candidate = input_candidates[it->second];
            if (compare_input_candidate_same_ki(other_input_candidate, input_candidate) < 0)
                it->second = i;
        }
    }

    // 2. Collect set of non-burned inputs
    std::set<std::size_t> all_non_burned_inputs;
    for (const auto &best_input : best_input_by_key_image)
        all_non_burned_inputs.insert(best_input.second);

    // 3. Partition into:
    //      a) Pre-carrot (no quantum forward secrecy)
    //      b) External carrot (quantum forward secret if public address not known)
    //      c) Internal carrot (always quantum forward secret unless secret keys known)
    std::set<std::size_t> pre_carrot_inputs;
    std::set<std::size_t> external_carrot_inputs;
    std::set<std::size_t> internal_inputs;
    for (std::size_t candidate_idx : all_non_burned_inputs)
    {
        if (input_candidates[candidate_idx].is_pre_carrot)
            pre_carrot_inputs.insert(candidate_idx);
        else if (input_candidates[candidate_idx].is_external)
            external_carrot_inputs.insert(candidate_idx);
        else
            internal_inputs.insert(candidate_idx);
    }

    // 4. Calculate misc features
    const bool must_use_internal = !(flags & ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS) && is_normal_transfer;
    const bool allow_mixed_externality = (flags & ALLOW_MIXED_INTERNAL_EXTERNAL) && !must_use_internal;
    const bool must_use_carrot = !(flags & ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS) && is_normal_transfer;
    const bool allow_mixed_carrotness = (flags & ALLOW_MIXED_CARROT_PRE_CARROT) && !must_use_carrot;

    // 5. We should prefer to spend non-forward-secret enotes in transactions where all the outputs
    // are going back to ourself. Otherwise, if we spend these enotes while transferring money to
    // another entity, an external observer who A) has a quantum computer, and B) knows one of their
    // public addresses, will be able to trace the money transfer. Such an observer will always be
    // able to tell which view-incoming keys / accounts these non-forward-secrets enotes belong to,
    // their amounts, and where they're spent. So since they already know that information, churning
    // back to oneself doesn't actually reveal that much more additional information.
    const bool prefer_non_fs = !is_normal_transfer;
    CARROT_CHECK_AND_THROW(!must_use_internal || !prefer_non_fs,
        carrot_logic_error, "bug: must_use_internal AND prefer_non_fs are true");

    // There is no "prefer pre-carrot" variable since in the case that we prefer spending
    // non-forward-secret, we always prefer first spending pre-carrot over carrot, if it is allowed

    // 6. Define input_candidate_subsets and how to add to it
    std::vector<std::set<std::size_t>> input_candidate_subsets;
    input_candidate_subsets.reserve(8);
    const auto push_subset = [&input_candidate_subsets](const std::set<std::size_t> &subset)
    {
        if (subset.empty()) return;
        const auto subset_it = std::find(input_candidate_subsets.cbegin(), input_candidate_subsets.cend(), subset);
        if (subset_it != input_candidate_subsets.cend()) return; // subset already present (could be more efficient)
        input_candidate_subsets.push_back(subset);
    };

    // 7. Try dispatching for non-forward-secret input subsets, if preferred in this context
    if (prefer_non_fs)
    {
        // try getting rid of pre-carrot enotes first, if allowed
        if (!must_use_carrot)
            push_subset(pre_carrot_inputs);

        // ... then external carrot
        push_subset(external_carrot_inputs);
    }

    // 8. Try dispatching for internal
    push_subset(internal_inputs);

    // 9. Try dispatching for non-FS *after* internal, if allowed and not already tried
    if (!must_use_internal || !prefer_non_fs)
    {
        // Spending non-FS inputs in a normal transfer transaction is not ideal, but at least
        // when partition it like this, we aren't "dirtying" the carrot with the pre-carrot, and
        // the internal with the external
        if (!must_use_carrot)
            push_subset(pre_carrot_inputs);
        push_subset(external_carrot_inputs);
    }

    // 10. Try dispatching for all non-FS (mixed pre-carrot & carrot external), if allowed
    if (allow_mixed_carrotness)
    {
        // We're mixing carrot/pre-carrot spends here, but avoiding "dirtying" the internal
        push_subset(set_union(pre_carrot_inputs, external_carrot_inputs));
    }

    // 11. Try dispatching for all carrot, if allowed
    if (allow_mixed_externality)
    {
        // We're mixing internal & external carrot spends here, but avoiding "dirtying" the
        // carrot spends with pre-carrot spends. This will be quantum forward secret iff the
        // adversary doesn't know one of your public addresses
        push_subset(set_union(external_carrot_inputs, internal_inputs));
    }

    //! @TODO: MRL discussion about whether step 11 or step 12 should go first. In other words,
    //         do we prefer to avoid dirtying internal, and protect against quantum adversaries
    //         who know your public addresses? Or do we prefer to avoid dirtying w/ pre-carrot,
    //         and protect against quantum adversaries with no special knowledge of your public
    //         addresses, but whose attacks are only relevant when spending pre-FCMP++ enotes?

    // 12. Try dispatching for everything, if allowed
    if (allow_mixed_carrotness && allow_mixed_externality)
        push_subset(all_non_burned_inputs);

    // Notice that we don't combine just the pre_carrot_inputs and internal_inputs by themselves

    return input_candidate_subsets;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::size_t> get_input_counts_in_preferred_order()
{
    // 1 or 2 randomly, then
    // other ascending powers of 2, then
    // other ascending positive numbers

    //! @TODO: MRL discussion about 2 vs 1 default input count when 1 input can pay. If we default
    // to 1, then that may reveal more information about the amount, and reveals that one can't pay
    // with 1 output when using 2. Vice versa, if we default to 2, then that means that one only
    // owns 1 output when using 1. It may be the most advantageous to randomly switch between
    // preferring 1 vs 2. See: https://lavalle.pl/planning/node437.html. Con to this approach: if we
    // default to 1 over 2 always then there's scenarios where we net save tx fees and proving time.

    static_assert(CARROT_MAX_TX_INPUTS == FCMP_PLUS_PLUS_MAX_INPUTS, "inconsistent input count max limit");
    static_assert(CARROT_MIN_TX_INPUTS == 1 && CARROT_MAX_TX_INPUTS == 8,
        "refactor this function for different input count limits");

    const bool random_bit = 0 == (crypto::rand<uint8_t>() & 0x01);
    if (random_bit)
        return {2, 1, 4, 8, 3, 5, 6, 7};
    else
        return {1, 2, 4, 8, 3, 5, 6, 7};
}
//-------------------------------------------------------------------------------------------------------------------
select_inputs_func_t make_single_transfer_input_selector(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const epee::span<const input_selection_policy_t> policies,
    const std::uint32_t flags,
    std::set<size_t> *selected_input_indices_out)
{
    // input selector :D
    return [=](const boost::multiprecision::uint128_t &nominal_output_sum,
        const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count,
        const std::size_t num_normal_payment_proposals,
        const std::size_t num_selfsend_payment_proposals,
        std::vector<CarrotSelectedInput> &selected_inputs_out)
    {
        using namespace InputSelectionFlags;

        // 1. Sanity checks valid arguments
        const std::size_t n_candidates = input_candidates.size();
        CARROT_CHECK_AND_THROW(!fee_by_input_count.empty(), missing_components, "no provided allowed input count");
        CARROT_CHECK_AND_THROW(!policies.empty(), missing_components, "no input selection policies provided");
        CARROT_CHECK_AND_THROW(n_candidates, not_enough_money, "no input candidates provided");

        // 2. Log
        MDEBUG("Running single transfer input selector with " << input_candidates.size() << " candidates and "
            << policies.size() << " policies, for " << num_normal_payment_proposals << " normal payment proposals, "
            << num_selfsend_payment_proposals << " self-send payment proposals, "
            << cryptonote::print_money(nominal_output_sum)
            << " output sum, and fee range " << cryptonote::print_money(fee_by_input_count.cbegin()->second)
            << "-" << cryptonote::print_money(fee_by_input_count.crbegin()->second));

        // 3. Calculate minimum required input money sum for a given input count
        const bool subtract_fee = flags & IS_KNOWN_FEE_SUBTRACTABLE;
        std::map<std::size_t, boost::multiprecision::uint128_t> required_money_by_input_count;
        for (const auto &fee_and_input_count : fee_by_input_count)
        {
            required_money_by_input_count[fee_and_input_count.first] = 
                nominal_output_sum + (subtract_fee ? 0 : fee_and_input_count.second);
        }
        const boost::multiprecision::uint128_t absolute_minimum_required_money
            = required_money_by_input_count.cbegin()->second;

        // 4. Quick check of total money and single tx input count limited total money
        boost::multiprecision::uint128_t total_candidate_money = 0;
        for (const CarrotPreSelectedInput &input_candidate : input_candidates)
            total_candidate_money += input_candidate.core.amount;
        CARROT_CHECK_AND_THROW(total_candidate_money >= absolute_minimum_required_money,
            not_enough_money,
            "Not enough money in all inputs (" << cryptonote::print_money(total_candidate_money)
            << ") to fund minimum output sum (" << cryptonote::print_money(absolute_minimum_required_money) << ')');

        std::set<std::size_t> all_idxs; for (std::size_t i = 0; i < input_candidates.size(); ++i) all_idxs.insert(i);
        const std::pair<std::size_t, boost::multiprecision::uint128_t> max_usable_money =
            input_count_for_max_usable_money(input_candidates, all_idxs, FCMP_PLUS_PLUS_MAX_INPUTS, fee_by_input_count);
        CARROT_CHECK_AND_THROW(max_usable_money.second >= absolute_minimum_required_money,
            not_enough_usable_money,
            "Not enough usable money in top " << max_usable_money.first << " inputs ("
            << cryptonote::print_money(max_usable_money.second) << ") to fund minimum output sum ("
            << cryptonote::print_money(absolute_minimum_required_money) << ')');

        // 5. Get preferred input candidate subsets
        //! @TODO: dummy check num_normal_payment_proposals
        const std::vector<std::set<std::size_t>> input_candidate_subsets = form_preferred_input_candidate_subsets(
            input_candidates,
            flags,
            num_normal_payment_proposals);

        // 6. Get preferred transaction input counts
        const std::vector<std::size_t> input_counts = get_input_counts_in_preferred_order();

        // 7. For each input candidate subset...
        std::set<size_t> selected_inputs_indices;
        for (const std::set<std::size_t> &input_candidate_subset : input_candidate_subsets)
        {
            if (selected_inputs_indices.size()) break;

            // Skip if not enough money in this selectable set for max number of tx inputs...
            const auto max_usable_money = input_count_for_max_usable_money(input_candidates,
                input_candidate_subset, FCMP_PLUS_PLUS_MAX_INPUTS, fee_by_input_count);
            if (!max_usable_money.first)
                continue;
            else if (max_usable_money.second < required_money_by_input_count.at(max_usable_money.first))
                continue;

            // Debug log input candidate subset
            MDEBUG("Trying to dispatch input selection on " << input_candidate_subset.size() <<
                "-input subset with tx max usable money: " << cryptonote::print_money(max_usable_money.second));
            for (const std::size_t selectable_index : input_candidate_subset)
            {
                const CarrotPreSelectedInput &input_candidate = input_candidates[selectable_index];
                MDEBUG("    " << input_candidate);
            }

            // For each transaction input count...
            for (const std::size_t n_inputs : input_counts)
            {
                if (selected_inputs_indices.size()) break;

                const boost::multiprecision::uint128_t &required_money = required_money_by_input_count.at(n_inputs);

                // Skip if not enough money in this selectable set for exact number of inputs...
                const auto max_usable_money = input_count_for_max_usable_money(input_candidates,
                    input_candidate_subset, n_inputs, fee_by_input_count);
                if (max_usable_money.first != n_inputs)
                    continue;
                else if (max_usable_money.second < required_money)
                    continue;

                // After this point, we expect one of the policies to succeed, otherwise all input selection fails

                // at least one call to an input selection subroutine has enough usable money to work with
                MDEBUG("Trying input selection with " << n_inputs << " tx inputs");

                // Filter all dust out of subset unless ALLOW_DUST flag is provided
                std::set<std::size_t> candidate_subset_filtered = input_candidate_subset;
                if (!(flags * ALLOW_DUST))
                {
                    const rct::xmr_amount dust_threshold = fee_by_input_count.at(n_inputs)
                        - (n_inputs > CARROT_MIN_TX_INPUTS ? fee_by_input_count.at(n_inputs - 1) : 0);
                    for (auto it = candidate_subset_filtered.cbegin(); it != candidate_subset_filtered.cend();)
                    {
                        if (*it >= input_candidates.size() || input_candidates[*it].core.amount <= dust_threshold)
                            it = candidate_subset_filtered.erase(it);
                        else
                            ++it;
                    }
                }

                // For each input selection policy...
                for (const input_selection_policy_t &policy : policies)
                {
                    if (selected_inputs_indices.size()) break;

                    policy(input_candidates,
                        candidate_subset_filtered,
                        n_inputs,
                        required_money,
                        selected_inputs_indices);
                }

                // Check nominal success
                CARROT_CHECK_AND_THROW(selected_inputs_indices.size(),
                    carrot_runtime_error, "provided input selection policies failed with enough usable money");
                CARROT_CHECK_AND_THROW(selected_inputs_indices.size() == n_inputs,
                    carrot_logic_error, "bug in policy: selected wrong number of inputs");

                // Check selected indices were actually selectable
                for (const std::size_t selected_inputs_index : selected_inputs_indices)
                    CARROT_CHECK_AND_THROW(candidate_subset_filtered.count(selected_inputs_index),
                        carrot_logic_error, "bug in policy: returned unselectable index");
            }
        };

        // 8. Sanity check indices
        CARROT_CHECK_AND_THROW(!selected_inputs_indices.empty(),
            not_enough_usable_money,
            "No single allowed subset of candidates had enough money to fund payment proposals and fees for inputs");
        CARROT_CHECK_AND_THROW(*selected_inputs_indices.crbegin() < input_candidates.size(),
            carrot_logic_error, "bug: selected inputs index out of range");

        // 9. Check the sum of input amounts is great enough
        const std::size_t num_selected = selected_inputs_indices.size();
        const boost::multiprecision::uint128_t required_money = required_money_by_input_count.at(num_selected);
        boost::multiprecision::uint128_t input_amount_sum = 0;
        for (const std::size_t idx : selected_inputs_indices)
            input_amount_sum += input_candidates[idx].core.amount;
        CARROT_CHECK_AND_THROW(input_amount_sum >= required_money,
            carrot_logic_error, "bug: input selection returned successful without enough funds");

        // 10. Collect selected inputs
        selected_inputs_out.clear();
        selected_inputs_out.reserve(num_selected);
        for (size_t selected_input_index : selected_inputs_indices)
            selected_inputs_out.push_back(input_candidates[selected_input_index].core);

        if (selected_input_indices_out != nullptr)
            *selected_input_indices_out = std::move(selected_inputs_indices);
    };
}
//-------------------------------------------------------------------------------------------------------------------
namespace ispolicy
{
//-------------------------------------------------------------------------------------------------------------------
void select_greedy_aging(const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<std::size_t> &selectable_inputs,
    const std::size_t n_inputs,
    const boost::multiprecision::uint128_t &required_money,
    std::set<std::size_t> &selected_inputs_indices_out)
{
    MTRACE(__func__ << ": n_inputs=" << n_inputs << ", selectable_inputs.size()=" << selectable_inputs.size());

    selected_inputs_indices_out.clear();

    CHECK_AND_ASSERT_MES(n_inputs,, "select_greedy_aging: n_inputs must be non-zero");
    CHECK_AND_ASSERT_MES(n_inputs <= selectable_inputs.size(),,
        "select_greedy_aging: not enough inputs: " << selectable_inputs.size() << '/' << n_inputs);

    // Sort selectable inputs by amount
    std::vector<std::size_t> selectable_inputs_by_amount(selectable_inputs.cbegin(), selectable_inputs.cend());
    stable_sort_indices_by_amount(input_candidates, selectable_inputs_by_amount);

    // Select highest amount inputs and collect ordered multi-map of block indices of current selected inputs
    boost::multiprecision::uint128_t input_amount_sum = 0;
    std::multimap<std::uint64_t, std::size_t> selected_indices_by_block_index;
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const std::size_t selectable_idx = selectable_inputs_by_amount.at(selectable_inputs_by_amount.size() - i - 1);
        const CarrotPreSelectedInput &input = input_candidates[selectable_idx];
        input_amount_sum += input.core.amount;
        selected_inputs_indices_out.insert(selectable_idx);
        selected_indices_by_block_index.emplace(input.block_index, selectable_idx);
    }

    // Check enough money
    if (input_amount_sum < required_money)
    {
        MDEBUG("not enough money in " << n_inputs << " inputs: " << cryptonote::print_money(input_amount_sum));
        selected_inputs_indices_out.clear();
        return;
    }

    // Right now, we have the highest amount inputs selected. Perform a greedy search to replace the newest inputs
    // with the oldest possible input that still provides enough money
    for (auto bi_it = selected_indices_by_block_index.rbegin(); bi_it != selected_indices_by_block_index.rend();)
    {
        std::uint64_t min_block_index = bi_it->first;
        std::size_t input_of_min_block_index_input = bi_it->second;
        const boost::multiprecision::uint128_t surplus = input_amount_sum - required_money;
        const rct::xmr_amount currently_selected_amount = input_candidates[bi_it->second].core.amount;
        const rct::xmr_amount lowest_replacement_amount = (currently_selected_amount > surplus)
            ? boost::numeric_cast<rct::xmr_amount>(currently_selected_amount - surplus) : 0;
        const auto lower_amount_it = std::lower_bound(selectable_inputs_by_amount.cbegin(),
            selectable_inputs_by_amount.cend(), lowest_replacement_amount);
        for (auto amount_it = lower_amount_it; amount_it != selectable_inputs_by_amount.cend(); ++amount_it)
        {
            const std::size_t potential_replacement_idx = *amount_it;
            if (selected_inputs_indices_out.count(potential_replacement_idx))
                continue;
            const CarrotPreSelectedInput &potential_replacement_input = input_candidates[potential_replacement_idx];
            if (potential_replacement_input.block_index < min_block_index)
            {
                min_block_index = potential_replacement_input.block_index;
                input_of_min_block_index_input = potential_replacement_idx;
            }
        }

        if (input_of_min_block_index_input != bi_it->second) // i.e. found a replacement
        {
            selected_inputs_indices_out.erase(bi_it->second);
            selected_inputs_indices_out.insert(input_of_min_block_index_input);
            bi_it = tools::reverse_erase(selected_indices_by_block_index, bi_it);
            selected_indices_by_block_index.emplace(min_block_index, input_of_min_block_index_input);
            input_amount_sum -= currently_selected_amount;
            input_amount_sum += input_candidates[input_of_min_block_index_input].core.amount;
            CARROT_CHECK_AND_THROW(input_amount_sum >= required_money,
                carrot_logic_error, "BUG: replaced an input with one of too low amount");
        }
        else // no replacement, go to next input
        {
            ++bi_it;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace ispolicy
} //namespace carrot
