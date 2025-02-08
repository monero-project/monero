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

#pragma once

//local headers
#include "carrot_tx_builder_types.h"
#include "span.h"

//third party headers

//standard headers
#include <set>

//forward declarations

namespace carrot
{
struct CarrotPreSelectedInput
{
    CarrotSelectedInput core;

    bool is_external;
    uint64_t block_index;
};

enum class InputSelectionPolicy
{
    // Most of these schemes are going to be approximate, since finding true optimal solutions for
    // a lot of these policies boil down to NP-hard problems, like 0-1 knapsack and CMP.
    TwoInputsPreferOldest,
    HighestUnlockedBalance,
    LowestInputCountAndFee,
    ConsolidateDiscretized,
    ConsolidateFast,
    OldestInputs
};

namespace InputSelectionFlags
{
    static constexpr std::uint32_t ALLOW_EXTERNAL_INPUTS_IN_NORMAL_TRANSFERS = 1 << 0;
    static constexpr std::uint32_t ALLOW_MIXED_INTERNAL_EXTERNAL             = 1 << 1;
    static constexpr std::uint32_t IS_KNOWN_FEE_SUBTRACTABLE                 = 1 << 2;
    static constexpr std::uint32_t ALLOW_DUST                                = 1 << 3;
}

// - amount
// - key image
// - internal vs external
// - height

select_inputs_func_t make_single_transfer_input_selector(
    const epee::span<const CarrotPreSelectedInput> input_candidates,
    const epee::span<const InputSelectionPolicy> policies,
    const std::uint32_t flags,
    std::set<size_t> *selected_input_indices_out);
} //namespace carrot
