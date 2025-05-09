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
#include "tx_proposal_utils.h"

//third party headers

//standard headers
#include <set>

//forward declarations

namespace carrot
{
struct CarrotPreSelectedInput
{
    CarrotSelectedInput core;

    bool is_pre_carrot;
    bool is_external;
    uint64_t block_index;
};

namespace InputSelectionFlags
{
    // Quantum forward secrecy (ON = unsafe)
    static constexpr std::uint32_t ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS   = 1 << 0;
    static constexpr std::uint32_t ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS = 1 << 1;
    static constexpr std::uint32_t ALLOW_MIXED_INTERNAL_EXTERNAL               = 1 << 2;
    static constexpr std::uint32_t ALLOW_MIXED_CARROT_PRE_CARROT               = 1 << 3;

    // Amount handling
    static constexpr std::uint32_t IS_KNOWN_FEE_SUBTRACTABLE                   = 1 << 4;
    static constexpr std::uint32_t ALLOW_DUST                                  = 1 << 5;
}

using input_selection_policy_t = std::function<void(
    epee::span<const CarrotPreSelectedInput>,        // input candidates
    const std::set<std::size_t>&,                    // selectable subset indices
    std::size_t,                                     // input count
    const boost::multiprecision::uint128_t&,         // required money for input count
    std::set<std::size_t>&                           // selected indices
)>;

/**
 * brief: compare_input_candidate_same_ki - compare two input candidates who share a key image; we can only choose one!
 * param: lhs -
 * param: rhs -
 * return: 1 if lhs is better, -1 if rhs is better, 0 if neutral
 * throw: component_out_of_order - iff rhs.core.key_image != lhs.core.key_image
 *
 * The better candidate is determined by criteria in descending order of importance as follows:
 *     1. Amount (higher is better, duh)
 *     2. Age (older is better for protection against double spend attacks)
 *     3. Is pre-Carrot enote? (`false` is better for spending QFS)
 *     4. Is external enote?  (`false` is better for spending QFS)
 */
int compare_input_candidate_same_ki(const CarrotPreSelectedInput &lhs, const CarrotPreSelectedInput &rhs);
/**
 * brief: form_preferred_input_candidate_subsets - make subsets of input candidates to try selection in preferred order
 * param: input_candidates - slice to user-provided input candidates
 * param: flags - values in InputSelectionFlags namespace OR'ed together
 * param: is_normal_transfer - true iff num normal non-dummy payments in tx to perform selection for is >= 1
 * return: ordered list of subsets (represented by 0-based indices) of input_candidates to try selection on
 *
 * This function also performs a burning bug check; no indices returned in any subset will reference
 * an input candidate when another input candidate shares the same key image but is "better" as
 * determined by compare_input_candidate_same_ki.
 *
 * Purpose: Mainly due to quantum forward secrecy properties of spending different types of Monero
 * enotes, it isn't always preferable to be able to select all usable input candidates together in
 * the same transaction. For example, spending internal Carrot enotes is always quantum forward
 * secret, even when one's public Monero address is known. By contrast, spending a pre-Carrot enote
 * is never quantum forward secret, even with no public address knowledge. As such, a spender should
 * prefer not to spend these enotes in the same transaction, since the pre-Carrot enote will "taint"
 * the quantum forward spending secrecy of the internal Carrot enote. Based on certain heuristics
 * and user-provided flags, this function creates a list of subsets of input_candidates, in
 * preferred order, to perform input selection on.
 *
 * Flags:
 *   * ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS - external inputs in normal txs are allowed iff=1
 *   * ALLOW_PRE_CARROT_INPUTS_IN_NORMAL_TRANSFERS - pre-carrot inputs in normal txs are allowed iff=1
 *   * ALLOW_MIXED_INTERNAL_EXTERNAL - mixing internal/external inputs in any txs is allowed iff=1
 *   * ALLOW_MIXED_CARROT_PRE_CARROT - mixing pre-carrot/carrot inputs in any txs is allowed iff=1
 *
 * General rules (not in any specific order):
 *   * It should be preferred NOT to use external inputs in normal transfers
 *   * It should be preferred NOT to use pre-carrot inputs in normal transfers
 *   * It should be preferred NOT to mix internal/external inputs
 *   * It should be preferred NOT to mix pre-carrot/carrot inputs
 *   * It should be preferred YES to use external & pre-carrots inputs in self-send transactions
 *
 * In scenarios where a user-provided flag allows a non-preferred subset of input candidates, this
 * function will FIRST add the subset of input candidates as if the user didn't provide that
 * flag, and THEN add the non-preferred subset. For example, let's say that you pass input
 * candidates span {A, B}, flags=ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS, and
 * is_normal_transfer=true. In this example, A is internal and B is external. The return
 * value of this function will be {{0}, {1}, {0, 1}}. This means: "try input selection on 0 (A)
 * first, then 1 (B), and then both {0, 1} (A & B)". In this example, although external inputs are
 * *allowed* in normal transfers by the flag provided by the user, they are not *preferred*, so
 * {A} comes before {B} in the subset list. Mixing is the least preferred, so {A, B} is the last
 * subset.
 *
 * If unsure which flags to use, flags=0 is the "safest" option for input selection. Note that this
 * completely disallows normal transfers for legacy key hierarchies, since inputs will never be
 * internal due to the lack of the view-balance secret s_vb in legacy key hierarchies.
 */
std::vector<std::set<std::size_t>> form_preferred_input_candidate_subsets(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const std::uint32_t flags,
    const bool is_normal_transfer);
/**
 * brief: get_input_counts_in_preferred_order - return list of tx input counts in order that we should prefer to select
 *
 * Transaction input counts are trivially observable on-chain, so picking a wrong input count when
 * given the chance between multiple choices can have privacy consequences. The purpose of this
 * function is to determine the order in which we should try to select a certain number of inputs.
 */
std::vector<std::size_t> get_input_counts_in_preferred_order();

select_inputs_func_t make_single_transfer_input_selector(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const epee::span<const input_selection_policy_t> policies,
    const std::uint32_t flags,
    std::set<size_t> *selected_input_indices_out);

namespace ispolicy
{
void select_greedy_aging(const epee::span<const CarrotPreSelectedInput>,
    const std::set<std::size_t>&,
    std::size_t,
    const boost::multiprecision::uint128_t&,
    std::set<std::size_t>&);

} //namespace ispolicy
} //namespace carrot
