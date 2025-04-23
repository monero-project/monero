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
static int compare_input_candidate_same_ki(const CarrotPreSelectedInput &lhs, const CarrotPreSelectedInput &rhs)
{
    CHECK_AND_ASSERT_THROW_MES(lhs.core.key_image == rhs.core.key_image,
        "compare_input_candidate_same_ki: this function is not meant to compare inputs of different key images");

    // first prefer the higher amount
    if (lhs.core.amount < rhs.core.amount)
        return -1;
    else if (lhs.core.amount > rhs.core.amount)
        return 1;

    // then prefer older
    if (lhs.block_index < rhs.block_index)
        return 1;
    else if (lhs.block_index > rhs.block_index)
        return -1;
    
    // It should be computationally intractable for lhs.is_external != rhs.is_external, but I haven't
    // looked into it too deeply. I guess you would want to prefer whichever one !is_external.

    return 0;
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
        [input_candidates](const size_t a, const size_t b) -> bool
        {
            CHECK_AND_ASSERT_THROW_MES(a < input_candidates.size() && b < input_candidates.size(),
                "input candidate index out of range");
            return input_candidates[a].core.amount < input_candidates[b].core.amount;
        });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void stable_sort_indices_by_block_index(const epee::span<const CarrotPreSelectedInput> input_candidates,
    std::vector<size_t> &indices_inout)
{
    std::stable_sort(indices_inout.begin(), indices_inout.end(),
        [input_candidates](const size_t a, const size_t b) -> bool
        {
            CHECK_AND_ASSERT_THROW_MES(a < input_candidates.size() && b < input_candidates.size(),
                "input candidate index out of range");
            return input_candidates[a].block_index < input_candidates[b].block_index;
        });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::pair<size_t, boost::multiprecision::int128_t> input_count_for_max_usable_money(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<size_t> &selectable_inputs,
    const std::map<size_t, rct::xmr_amount> &fee_by_input_count)
{
    // Returns (N, X) where the X is the sum of the amounts of the greatest N <= CARROT_MAX_TX_INPUTS
    // inputs from selectable_inputs, maximizing X - F(N). F(N) is the fee for this transaction,
    // given input count N. This should correctly handle "almost-dust": inputs which are less than
    // the fee, but greater than or equal to the difference of the fee compared to excluding that
    // input. If this function returns N == 0, then there aren't enough usable funds, i.e. no N
    // exists such that X - F(N) > 0.

    size_t num_ins = 0;
    boost::multiprecision::int128_t cumulative_input_sum = 0;
    rct::xmr_amount last_fee = 0;

    std::vector<size_t> selectable_inputs_vec(selectable_inputs.cbegin(), selectable_inputs.cend());
    stable_sort_indices_by_amount(input_candidates, selectable_inputs_vec);

    // for selectable indices in descending amount...
    for (auto it = selectable_inputs_vec.crbegin(); it != selectable_inputs_vec.crend(); ++it)
    {
        if (num_ins == CARROT_MAX_TX_INPUTS)
            break;

        ++num_ins;

        const rct::xmr_amount amount = input_candidates[*it].core.amount;

        if (amount < fee_by_input_count.at(num_ins) - last_fee)
        {
            // then this input doesn't pay for itself, rollback previous state and break
            // since all next inputs will have same amount or less
            --num_ins;
            break;
        }

        cumulative_input_sum += amount;
        last_fee = fee_by_input_count.at(num_ins);
    }

    return {num_ins, cumulative_input_sum};
}
//-------------------------------------------------------------------------------------------------------------------
select_inputs_func_t make_single_transfer_input_selector(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const epee::span<const input_selection_policy_t> policies,
    const std::uint32_t flags,
    std::set<size_t> *selected_input_indices_out)
{
    using namespace InputSelectionFlags;

    CHECK_AND_ASSERT_THROW_MES(!policies.empty(),
        "make_single_transfer_input_selector: no input selection policies provided");

    // Sanity check flags
    const bool confused_qfs = (flags & ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS) &&
        !(flags & ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS);
    CHECK_AND_ASSERT_THROW_MES(!confused_qfs,
        "make single transfer input selector: It does not make sense to allow pre-carrot inputs in normal transfers, "
        "but not external carrot inputs.");

    // input selector :)
    return [=](const boost::multiprecision::int128_t &nominal_output_sum,
        const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count,
        const std::size_t num_normal_payment_proposals,
        const std::size_t num_selfsend_payment_proposals,
        std::vector<CarrotSelectedInput> &selected_inputs_out)
    {
        CHECK_AND_ASSERT_THROW_MES(!fee_by_input_count.empty(),
            "make_single_transfer_input_selector: no provided allowed input count");

        MDEBUG("Running single transfer input selector with " << input_candidates.size() << " candidates and "
            << policies.size() << " policies, for " << num_normal_payment_proposals << " normal payment proposals, "
            << num_selfsend_payment_proposals << " self-send payment proposals, "
            << cryptonote::print_money(boost::numeric_cast<boost::multiprecision::uint128_t>(nominal_output_sum))
            << " output sum, and fee range " << cryptonote::print_money(fee_by_input_count.cbegin()->second)
            << "-" << cryptonote::print_money(fee_by_input_count.crbegin()->second));

        // 1. Compile map of best input candidates by key image to mitigate the "burning bug" for legacy enotes
        std::unordered_map<crypto::key_image, size_t> best_input_by_key_image;
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
        std::set<size_t> all_non_burned_inputs;
        for (const auto &best_input : best_input_by_key_image)
            all_non_burned_inputs.insert(best_input.second);

        // 3. Partition into:
        //      a) Pre-carrot (no quantum forward secrecy)
        //      b) External carrot (quantum forward secret if public address not known)
        //      c) Internal carrot (always quantum forward secret unless secret keys known)
        std::set<size_t> pre_carrot_inputs;
        std::set<size_t> external_carrot_inputs;
        std::set<size_t> internal_inputs;
        for (size_t candidate_idx : all_non_burned_inputs)
        {
            if (input_candidates[candidate_idx].is_pre_carrot)
                pre_carrot_inputs.insert(candidate_idx);
            else if (input_candidates[candidate_idx].is_external)
                external_carrot_inputs.insert(candidate_idx);
            else
                internal_inputs.insert(candidate_idx);
        }

        // 4. Calculate minimum required input money sum for a given input count
        const bool subtract_fee = flags & IS_KNOWN_FEE_SUBTRACTABLE;
        std::map<size_t, boost::multiprecision::int128_t> required_money_by_input_count;
        for (const auto &fee_and_input_count : fee_by_input_count)
        {
            required_money_by_input_count[fee_and_input_count.first] = 
                nominal_output_sum + (subtract_fee ? 0 : fee_and_input_count.second);
        }

        // 5. Calculate misc features
        const bool must_use_internal = !(flags & ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS) &&
            (num_normal_payment_proposals != 0);
        const bool allow_mixed_externality = (flags & ALLOW_MIXED_INTERNAL_EXTERNAL) &&
            !must_use_internal;
        const bool must_use_carrot = !(flags & ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS) &&
            (num_normal_payment_proposals != 0);
        const bool allow_mixed_carrotness = (flags & ALLOW_MIXED_CARROT_PRE_CARROT) &&
            !must_use_carrot;

        // We should prefer to spend non-forward-secret enotes in transactions where all the outputs are going back to
        // ourself. Otherwise, if we spend these enotes while transferring money to another entity, an external observer
        // who A) has a quantum computer, and B) knows one of their public addresses, will be able to trace the money
        // transfer. Such an observer will always be able to tell which view-incoming keys / accounts these
        // non-forward-secrets enotes belong to, their amounts, and where they're spent. So since they already know that
        // information, churning back to oneself doesn't actually reveal that much more additional information.
        const bool prefer_non_fs = num_normal_payment_proposals == 0;
        CHECK_AND_ASSERT_THROW_MES(!must_use_internal || !prefer_non_fs,
            "make_single_transfer_input_selector: bug: must_use_internal AND prefer_non_fs are true");

        // There is no "prefer pre-carrot" variable since in the case that we prefer spending non-forward-secret, we
        // always prefer first spending pre-carrot over carrot, if it is allowed

        // 6. Short-hand functor for dispatching input selection on a subset of inputs
        //    Note: Result goes into `selected_inputs_indices`. If already populated, then this functor does nothing
        std::set<size_t> selected_inputs_indices;
        const auto try_dispatch_input_selection =
            [&](const std::set<size_t> &selectable_indices)
        {
            // Return early if already selected inputs or no available selectable
            const bool already_selected = !selected_inputs_indices.empty();
            if (already_selected || selectable_indices.empty())
                return;

            // Return early if not enough money in this selectable set...
            const auto max_usable_money = input_count_for_max_usable_money(input_candidates,
                    selectable_indices,
                    fee_by_input_count);
            const bool enough_money = max_usable_money.first > 0
                && max_usable_money.second >= required_money_by_input_count.at(max_usable_money.first);
            if (!enough_money)
                return;

            const boost::multiprecision::uint128_t max_usable_money_u128 =
                boost::numeric_cast<boost::multiprecision::uint128_t>(max_usable_money.second);
            MDEBUG("Trying to dispatch input selection on " << selectable_indices.size() <<
                "-input subset with max usable money: " << cryptonote::print_money(max_usable_money_u128) << " XMR");
            for (const size_t selectable_index : selectable_indices)
            {
                const CarrotPreSelectedInput &input_candidate = input_candidates[selectable_index];
                MDEBUG("    " << input_candidate);
            }

            // For each passed policy and while not already selected inputs, dispatch policy...
            for (size_t policy_idx = 0; policy_idx < policies.size() && selected_inputs_indices.empty(); ++policy_idx)
                policies[policy_idx](input_candidates,
                    selectable_indices,
                    required_money_by_input_count,
                    selected_inputs_indices);

            // Check that returned selected indices were actually selectable
            for (const size_t selected_inputs_index : selected_inputs_indices)
                CHECK_AND_ASSERT_THROW_MES(selectable_indices.count(selected_inputs_index),
                    "make_single_transfer_input_selector: bug in policy: returned unselectable index");
        };

        // 8. Try dispatching for non-forward-secret input subsets, if preferred in this context
        if (prefer_non_fs)
        {
            // try getting rid of pre-carrot enotes first, if allowed
            if (!must_use_carrot)
                try_dispatch_input_selection(pre_carrot_inputs);

            // ... then external carrot
            try_dispatch_input_selection(external_carrot_inputs);
        }

        // 9. Try dispatching for internal
        try_dispatch_input_selection(internal_inputs);

        // 10. Try dispatching for non-FS *after* internal, if allowed and not already tried
        if (!must_use_internal || !prefer_non_fs)
        {
            // Spending non-FS inputs in a normal transfer transaction is not ideal, but at least
            // when partition it like this, we aren't "dirtying" the carrot with the pre-carrot, and
            // the internal with the external
            if (!must_use_carrot)
                try_dispatch_input_selection(pre_carrot_inputs);
            try_dispatch_input_selection(external_carrot_inputs);
        }

        // 11. Try dispatching for all non-FS (mixed pre-carrot & carrot external), if allowed
        if (allow_mixed_carrotness)
        {
            // We're mixing carrot/pre-carrot spends here, but avoiding "dirtying" the internal
            try_dispatch_input_selection(set_union(pre_carrot_inputs, external_carrot_inputs));
        }

        // 12. Try dispatching for all carrot, if allowed
        if (allow_mixed_externality)
        {
            // We're mixing internal & external carrot spends here, but avoiding "dirtying" the
            // carrot spends with pre-carrot spends. This will be quantum forward secret iff the
            // adversary doesn't know one of your public addresses
            try_dispatch_input_selection(set_union(external_carrot_inputs, internal_inputs));
        }

        //! @TODO: MRL discussion about whether step 11 or step 12 should go first. In other words,
        //         do we prefer to avoid dirtying internal, and protect against quantum adversaries
        //         who know your public addresses? Or do we prefer to avoid dirtying w/ pre-carrot,
        //         and protect against quantum adversaries with no special knowledge of your public
        //         addresses, but whose attacks are only relevant when spending pre-FCMP++ enotes?

        // 13. Try dispatching for everything, if allowed
        if (allow_mixed_carrotness && allow_mixed_externality)
            try_dispatch_input_selection(all_non_burned_inputs);

        // Notice that we don't combine just the pre_carrot_inputs and internal_inputs by themselves

        // 14. Sanity check indices
        CHECK_AND_ASSERT_THROW_MES(!selected_inputs_indices.empty(),
            "make_single_transfer_input_selector: input selection failed");
        CHECK_AND_ASSERT_THROW_MES(*selected_inputs_indices.crbegin() < input_candidates.size(),
            "make_single_transfer_input_selector: bug: selected inputs index out of range");

        // 15. Do a greedy search for inputs whose amount doesn't pay for itself and drop them, logging debug messages
        //     Note: this also happens to be optimal if the fee difference between each input count is constant
        bool should_search_for_dust = !(flags & ALLOW_DUST);
        while (should_search_for_dust && selected_inputs_indices.size() > CARROT_MIN_TX_INPUTS)
        {
            should_search_for_dust = false; // only loop again if we remove an input below
            const boost::multiprecision::int128_t fee_diff =
                required_money_by_input_count.at(selected_inputs_indices.size()) - 
                required_money_by_input_count.at(selected_inputs_indices.size() - 1);
            CHECK_AND_ASSERT_THROW_MES(fee_diff >= 0,
                "make_single_transfer_input_selector: bug: fee is expected to be higher with fewer inputs");
            for (auto it = selected_inputs_indices.begin(); it != selected_inputs_indices.end(); ++it)
            {
                const CarrotPreSelectedInput &input_candidate = input_candidates[*it];
                if (input_candidate.core.amount < fee_diff)
                {
                    MDEBUG("make_single_transfer_input_selector: dropping dusty input "
                        << input_candidate.core.key_image << " with amount " << input_candidate.core.amount
                        << ", which is less than the difference in fee of this transaction with it: " << fee_diff);
                    selected_inputs_indices.erase(it);
                    should_search_for_dust = true;
                    break; // break out of inner `for` loop so we can recalculate `fee_diff`
                }
            }
        }

        // 16. Check the sum of input amounts is great enough
        const size_t num_selected = selected_inputs_indices.size();
        const boost::multiprecision::int128_t required_money = required_money_by_input_count.at(num_selected);
        boost::multiprecision::int128_t input_amount_sum = 0;
        for (const size_t idx : selected_inputs_indices)
            input_amount_sum += input_candidates[idx].core.amount;
        CHECK_AND_ASSERT_THROW_MES(input_amount_sum >= required_money,
            "make_single_transfer_input_selector: bug: input selection returned successful without enough funds");

        // 17. Collect selected inputs
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
std::vector<std::size_t> get_input_counts_in_preferred_order()
{
    // 1 or 2 randomly, then
    // other ascending non-zero powers of 2, then
    // other ascending non-zero numbers

    //! @TODO: MRL discussion about 2 vs 1 default input count when 1 input can pay. If we default to 1, then that may
    // reveal more information about the amount, and reveals that one can't pay with 1 output when using 2. Vice versa,
    // if we default to 2, then that means that one only owns 1 output when using 1. It may be the most advantageous to
    // randomly switch between preferring 1 vs 2. See: https://lavalle.pl/planning/node437.html

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
void select_two_inputs_prefer_oldest(const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<size_t> &selectable_inputs,
    const std::map<size_t, boost::multiprecision::int128_t> &required_money_by_input_count,
    std::set<size_t> &selected_inputs_indices_out)
{
    // calculate required money and fee diff from one to two inputs
    const boost::multiprecision::int128_t required_money = required_money_by_input_count.at(2);
    const rct::xmr_amount fee_diff = boost::numeric_cast<rct::xmr_amount>(required_money -
            required_money_by_input_count.at(1));

    // copy selectable_inputs, excluding dust, then sort by ascending block index
    std::vector<size_t> selectable_inputs_by_bi;
    selectable_inputs_by_bi.reserve(selectable_inputs.size());
    for (size_t idx : selectable_inputs)
        if (input_candidates[idx].core.amount > fee_diff)
            selectable_inputs_by_bi.push_back(idx);
    stable_sort_indices_by_block_index(input_candidates, selectable_inputs_by_bi);

    // then copy again and *stable* sort by amount 
    std::vector<size_t> selectable_inputs_by_amount_bi = selectable_inputs_by_bi;
    stable_sort_indices_by_amount(input_candidates, selectable_inputs_by_amount_bi);

    // for each input in ascending block index order...
    for (size_t low_bi_input : selectable_inputs_by_bi)
    {
        // calculate how much we need in a corresponding input to this one
        const rct::xmr_amount old_amount = input_candidates[low_bi_input].core.amount;
        const boost::multiprecision::int128_t required_money_in_other_128 = (required_money > old_amount)
            ? (required_money - old_amount) : 0;
        if (required_money_in_other_128 >= std::numeric_limits<rct::xmr_amount>::max())
            continue;
        const rct::xmr_amount required_money_in_other =
            boost::numeric_cast<rct::xmr_amount>(required_money_in_other_128);

        // do a binary search for an input with at least that amount
        auto other_it = std::lower_bound(selectable_inputs_by_amount_bi.cbegin(),
            selectable_inputs_by_amount_bi.cend(),
            required_money_in_other,
            [input_candidates](size_t selectable_index, rct::xmr_amount required_money_in_other) -> bool
                { return input_candidates[selectable_index].core.amount < required_money_in_other; });

        // check that the iterator is in bounds and the complementary input isn't equal to the first
        if (other_it == selectable_inputs_by_amount_bi.cend())
            continue;
        else if (*other_it == low_bi_input)
            ++other_it; // can't choose same input twice

        if (other_it == selectable_inputs_by_amount_bi.cend())
            continue;

        // we found a match !
        selected_inputs_indices_out = {low_bi_input, *other_it};
        return;
    }
}
//-------------------------------------------------------------------------------------------------------------------
void select_greedy_aging_fixed_count(const std::size_t fixed_n_inputs,
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<std::size_t> &selectable_inputs,
    const std::map<size_t, boost::multiprecision::int128_t> &required_money_by_input_count,
    std::set<std::size_t> &selected_inputs_indices_out)
{
    MTRACE(__func__ << ": fixed_n_inputs=" << fixed_n_inputs << ", selectable_inputs.size()="
        << selectable_inputs.size());

    selected_inputs_indices_out.clear();

    CHECK_AND_ASSERT_MES(fixed_n_inputs,, "select_greedy_aging: fixed_n_inputs must be non-zero");
    CHECK_AND_ASSERT_MES(fixed_n_inputs <= selectable_inputs.size(),,
        "select_greedy_aging: not enough inputs: " << selectable_inputs.size() << '/' << fixed_n_inputs);
    CHECK_AND_ASSERT_MES(required_money_by_input_count.count(fixed_n_inputs),,
        "select_greedy_aging: input count " << fixed_n_inputs << "not allowed");

    // Sort selectable inputs by amount
    std::vector<std::size_t> selectable_inputs_by_amount(selectable_inputs.cbegin(), selectable_inputs.cend());
    stable_sort_indices_by_amount(input_candidates, selectable_inputs_by_amount);

    // Select highest amount inputs and collect ordered multi-map of block indices of current selected inputs
    boost::multiprecision::uint128_t input_amount_sum = 0;
    std::multimap<std::uint64_t, std::size_t> selected_indices_by_block_index;
    for (size_t i = 0; i < fixed_n_inputs; ++i)
    {
        const std::size_t selectable_idx = selectable_inputs_by_amount.at(selectable_inputs_by_amount.size() - i - 1);
        const CarrotPreSelectedInput &input = input_candidates[selectable_idx];
        input_amount_sum += input.core.amount;
        selected_inputs_indices_out.insert(selectable_idx);
        selected_indices_by_block_index.emplace(input.block_index, selectable_idx);
    }

    // Check enough money
    const boost::multiprecision::uint128_t required_money =
        boost::numeric_cast<boost::multiprecision::uint128_t>(required_money_by_input_count.at(fixed_n_inputs));
    if (input_amount_sum < required_money)
    {
        MDEBUG("not enough money in " << fixed_n_inputs << " inputs: " << cryptonote::print_money(input_amount_sum));
        selected_inputs_indices_out.clear();
        return;
    }

    // Right now, we have the highest amount inputs selected. Perform a greedy search to replace the newest inputs
    // with the oldest possible input that still provides enough money
    for (auto bi_it = selected_indices_by_block_index.rbegin(); bi_it != selected_indices_by_block_index.rend();)
    {
        std::uint64_t min_block_index = bi_it->first;
        size_t input_of_min_block_index_input = bi_it->second;
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
            CHECK_AND_ASSERT_THROW_MES(input_amount_sum >= required_money,
                "select_greedy_aging: BUG: replaced an input with one of too low amount");
        }
        else // no replacement, go to next input
        {
            ++bi_it;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
void select_greedy_aging(const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::set<std::size_t> &selectable_inputs,
    const std::map<size_t, boost::multiprecision::int128_t> &required_money_by_input_count,
    std::set<std::size_t> &selected_inputs_indices_out)
{
    selected_inputs_indices_out.clear();

    for (const std::size_t n_inputs : get_input_counts_in_preferred_order())
    {
        select_greedy_aging_fixed_count(n_inputs,
            input_candidates,
            selectable_inputs,
            required_money_by_input_count,
            selected_inputs_indices_out);
        if (!selected_inputs_indices_out.empty())
            return;
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace ispolicy
} //namespace carrot
