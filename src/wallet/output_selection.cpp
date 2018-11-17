// Copyright (c) 2019, The Monero Project
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

#include "output_selection.h"

#include <algorithm>
#include <stdexcept>

#include "crypto/crypto.h"
#include "cryptonote_config.h"

namespace tools
{
    namespace
    {
        constexpr const double gamma_shape = 19.28;
        constexpr const double gamma_scale = 1 / double(1.61);
        constexpr const std::size_t blocks_in_a_year = (86400 * 365) / DIFFICULTY_TARGET_V2;
    }

    gamma_picker::gamma_picker(std::vector<uint64_t> rct_offsets)
      : gamma_picker(std::move(rct_offsets), gamma_shape, gamma_scale)
    {}

    gamma_picker::gamma_picker(std::vector<std::uint64_t> offsets_in, double shape, double scale)
      : rct_offsets(std::move(offsets_in)),
        gamma(shape, scale),
        outputs_per_second(0)
    {
        if (!rct_offsets.empty())
        {
            const std::size_t blocks_to_consider = std::min(rct_offsets.size(), blocks_in_a_year);
            const std::uint64_t initial = blocks_to_consider < rct_offsets.size() ?
                rct_offsets[rct_offsets.size() - blocks_to_consider - 1] : 0;
            const std::size_t outputs_to_consider = rct_offsets.back() - initial;

            static_assert(0 < DIFFICULTY_TARGET_V2, "block target time cannot be zero");
            // this assumes constant target over the whole rct range
            outputs_per_second = outputs_to_consider / double(DIFFICULTY_TARGET_V2 * blocks_to_consider);
        }
    }

    bool gamma_picker::is_valid() const noexcept
    {
        return CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE < rct_offsets.size();
    }

    std::uint64_t gamma_picker::spendable_upper_bound() const noexcept
    {
        if (!is_valid())
            return 0;
        return *(rct_offsets.end() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1);
    }

    std::uint64_t gamma_picker::operator()()
    {
        if (!is_valid())
            throw std::logic_error{"Cannot select random output - blockchain height too small"};

        static_assert(std::is_empty<crypto::random_device>(), "random_device is no longer cheap to construct");
        static constexpr const crypto::random_device engine{};
        const auto end = offsets().end() - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

        for (unsigned tries = 0; tries < 100; ++tries)
        {
            std::uint64_t output_index = std::exp(gamma(engine)) * outputs_per_second;
            if (offsets().back() <= output_index)
                continue; // gamma selected older than blockchain height (rare)

            output_index = offsets().back() - 1 - output_index;
            const auto selection = std::lower_bound(offsets().begin(), end, output_index);
            if (selection == end)
                continue; // gamma selected within locked/non-spendable range (rare)

            const std::uint64_t first_rct = offsets().begin() == selection ? 0 : *(selection - 1);
            const std::uint64_t n_rct = *selection - first_rct;
            if (n_rct != 0)
                return first_rct + crypto::rand_idx(n_rct);
            // block had zero outputs (miner didn't collect XMR?)
        }
        throw std::runtime_error{"Unable to select random output in spendable range using gamma distribution after 1,024 attempts"};
    }

    std::vector<std::uint64_t> gamma_picker::take_offsets()
    {
        return std::vector<std::uint64_t>{std::move(rct_offsets)};
    }
}
