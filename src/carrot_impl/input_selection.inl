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
#include "ringct/rctTypes.h"

//third party headers
#include <boost/multiprecision/cpp_int.hpp>

//standard headers
#include <cstdint>
#include <utility>
#include <map>

//forward declarations

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
template <class AmountFwdIt>
std::pair<std::size_t, boost::multiprecision::uint128_t> get_input_count_for_max_usable_money(
    AmountFwdIt user_amount_begin,
    const AmountFwdIt &user_amount_end,
    const std::size_t max_num_input_count,
    const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count)
{
    // maintain list of top `max_num_input_count` amounts
    std::multiset<rct::xmr_amount> top_amounts;
    std::size_t n_amounts = 0;
    for (;user_amount_begin != user_amount_end; ++user_amount_begin, ++n_amounts)
    {
        top_amounts.insert(*user_amount_begin);
        if (top_amounts.size() > max_num_input_count)
            top_amounts.erase(top_amounts.cbegin());
    }

    std::size_t input_count = 0;
    boost::multiprecision::uint128_t cumulative_input_sum = 0;
    boost::multiprecision::uint128_t best_cumulative_input_sum = 0;
    std::size_t best_input_count = 0;
    // for all valid input counts (or existing amount counts, whichever is fewer)...
    for (auto top_amount_it = top_amounts.crbegin(); top_amount_it != top_amounts.crend(); ++top_amount_it)
    {
        // in order of largest amount to least...
        const rct::xmr_amount amount = *top_amount_it;

        // get fee of next number of inputs if available in F(N), else stop.
        cumulative_input_sum += amount;
        ++input_count;
        const auto fee_it = fee_by_input_count.find(input_count);
        if (fee_it == fee_by_input_count.cend())
            break;
        const rct::xmr_amount current_fee = fee_it->second;

        // if the input amount total is greater than fee for that number of inputs...
        if (cumulative_input_sum > current_fee)
        {
            // and if that input amount total is better than any other total observed yet...
            if (cumulative_input_sum > best_cumulative_input_sum)
            {
                // set that as best.
                best_cumulative_input_sum = cumulative_input_sum;
                best_input_count = input_count;
            }
        }
    }

    return {best_input_count, best_cumulative_input_sum};
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
