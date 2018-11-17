// Copyright (c) 2018, The Monero Project
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

#include <boost/optional/optional.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/combine.hpp>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <unordered_set>

#include "common/error.h"
#include "light_wallet_server/error.h"
#include "light_wallet_server/random_outputs.h"
#include "wallet/output_selection.h"

#define TRACE_EQ(left, right)              \
    do                                     \
    {                                      \
        EXPECT_EQ(left, right);            \
        if (::testing::Test::HasFailure()) \
            return false;                  \
    } while (0)

namespace
{
    
    lws::output_keys fake_key_fetch(const std::uint64_t index, const bool unlocked)
    {
        lws::output_keys out{};
        std::memcpy(std::addressof(out.key), std::addressof(index), sizeof(index));
        std::memcpy(std::addressof(out.mask), std::addressof(index), sizeof(index));
        out.unlocked = unlocked;
        return out;
    }

    class key_fetch_random
    {
        std::mt19937 roll_;
        const double probability_;

        bool is_unlocked()
        {
            return roll_() < std::mt19937::result_type(double(roll_.max() - roll_.min()) * probability_) + roll_.min();
        }

    public:
        key_fetch_random(const double probability)
          : roll_(std::random_device{}())
          , probability_(probability)
        {}

        key_fetch_random(key_fetch_random&&) = default;
        key_fetch_random(key_fetch_random const&) = default;

        expect<std::vector<lws::output_keys>> operator()(std::vector<lws::output_ref> src)
        {
            std::vector<lws::output_keys> keys{};
            keys.reserve(src.size());

            for (lws::output_ref const& entry : src)
                keys.push_back(fake_key_fetch(entry.index, is_unlocked()));

            return keys;
        }
    };

    struct key_fetch_static
    {
        const bool unlock_;

        expect<std::vector<lws::output_keys>> operator()(std::vector<lws::output_ref> src) const
        {
            std::vector<lws::output_keys> keys{};
            keys.reserve(src.size());

            for (lws::output_ref const& entry : src)
                keys.push_back(fake_key_fetch(entry.index, unlock_));

            return keys;
        }
    };


    // Provide non-conflicting custom `operator==` for `EXPECT_EQ` macros.
    struct expected_outputs
    {
        std::vector<lws::random_ring> expected;

        bool equal(std::vector<lws::random_ring> const& actual) const
        {
            return boost::equal(expected, actual, [] (lws::random_ring const& lhs, lws::random_ring const& rhs) {
                TRACE_EQ(lhs.amount, rhs.amount);

                return boost::equal(lhs.ring, rhs.ring, [] (lws::random_output const& lout, lws::random_output const& rout) {
                    TRACE_EQ(lout.index, rout.index);
                    TRACE_EQ(lout.keys.key, rout.keys.key);
                    TRACE_EQ(lout.keys.mask, rout.keys.mask);
                    TRACE_EQ(lout.keys.unlocked, rout.keys.unlocked);
                    return true;
                });
            });
        }
    };

    bool operator==(std::vector<lws::random_ring> const& left, expected_outputs const& right)
    {
        return right.equal(left);
    }

    struct valid_rings
    {
        const std::size_t rings;
        const std::uint32_t mixin;

        bool is_valid(std::vector<lws::random_ring> const& src) const
        {
            TRACE_EQ(rings, src.size());
            for (lws::random_ring const& ring : src)
            {
                TRACE_EQ(mixin, ring.ring.size());

                const std::uint64_t amount = ring.amount;
                std::unordered_set<std::uint64_t> found{};

                for (lws::random_output const& entry : ring.ring)
                {
                    const std::uint64_t index = entry.index;
                    TRACE_EQ(true, found.insert(index).second);

                    TRACE_EQ(true, entry.keys.unlocked);
                    TRACE_EQ(0u, std::memcmp(std::addressof(entry.keys.key), std::addressof(index), sizeof(index)));
                    TRACE_EQ(0u, std::memcmp(std::addressof(entry.keys.mask), std::addressof(index), sizeof(index)));
                }
            }
            return true;
        }
    };

    bool operator==(std::vector<lws::random_ring> const& left, valid_rings right)
    {
        return right.is_valid(left);
    }
}

TEST(pick_random_outputs, out_of_range)
{
    struct never_called
    {
        expect<std::vector<lws::output_keys>> operator()(std::vector<lws::output_ref>) const
        {
            return {lws::error::signal_abort_scan}; // should never be returned in tests
        }
    };

    const std::uint64_t rct_amounts[] = {0};
    const std::uint64_t non_rct_amounts[] = {1};

    tools::gamma_picker empty_rct{};
    tools::gamma_picker pick_rct{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    tools::gamma_picker pick_rct_trunc{{pick_rct.offsets().begin(), pick_rct.offsets().begin() + 10}};
    lws::histogram hist[] = {
        lws::histogram{1, 3, 2, 2}
    };
    lws::histogram bad_hist[] = {
        lws::histogram{2, 3, 2, 2}
    };

    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(0, {}, empty_rct, {}, nullptr));
    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(100, {}, empty_rct, {}, nullptr));
    {
        const expected_outputs expected{std::vector<lws::random_ring>{1}};
        EXPECT_EQ(expected, lws::pick_random_outputs(0, rct_amounts, empty_rct, {}, nullptr));
        EXPECT_EQ(expected, lws::pick_random_outputs(0, non_rct_amounts, empty_rct, {}, nullptr));
    }
    EXPECT_EQ(common_error::kInvalidArgument, lws::pick_random_outputs(1, rct_amounts, empty_rct, {}, nullptr));
    EXPECT_EQ(common_error::kInvalidArgument, lws::pick_random_outputs(1, non_rct_amounts, empty_rct, {}, nullptr));

    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(1, rct_amounts, empty_rct, {}, never_called{}));
    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(2, rct_amounts, pick_rct, {}, never_called{}));
    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(1, rct_amounts, pick_rct_trunc, {}, never_called{}));
    EXPECT_EQ(lws::error::not_enough_mixin, lws::pick_random_outputs(1, rct_amounts, pick_rct, {}, key_fetch_static{false}));

    EXPECT_EQ(common_error::kInvalidArgument, lws::pick_random_outputs(1, non_rct_amounts, empty_rct, {}, never_called{}));
    EXPECT_EQ(common_error::kInvalidArgument, lws::pick_random_outputs(1, non_rct_amounts, empty_rct, bad_hist, never_called{}));
    EXPECT_EQ(expected_outputs{}, lws::pick_random_outputs(3, non_rct_amounts, empty_rct, hist, never_called{}));
    EXPECT_EQ(lws::error::not_enough_mixin, lws::pick_random_outputs(1, non_rct_amounts, empty_rct, hist, key_fetch_static{false}));
}

TEST(pick_random_outputs, exact_amount)
{
    tools::gamma_picker empty_rct{};
    tools::gamma_picker pick_rct{{10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}};
    const std::uint64_t rct_amounts[] = {0, 0, 0};
    const std::uint64_t non_rct_amounts[] = {1, 2, 3};
    const std::uint64_t mixed_amounts[] = {0, 1, 0, 2, 3, 0};
    lws::histogram hist[] = {
        lws::histogram{1, 10, 10, 0},
        lws::histogram{2, 10, 10, 0},
        lws::histogram{3, 20, 10, 0}
    };

    expected_outputs expected = [] () {
        std::vector<lws::output_ref> outs{};
        outs.resize(10);

        std::size_t index = 0;
        for (auto& id : outs)
            id.index = index++;

        const std::vector<lws::output_keys> keys = key_fetch_static{true}(outs).value();

        std::vector<lws::random_output> ring{};
        ring.reserve(keys.size());
        for (auto entry : boost::combine(outs, keys))
            ring.push_back(lws::random_output{boost::get<1>(entry), boost::get<0>(entry).index});

        return expected_outputs{{lws::random_ring{std::move(ring), 0}}};
    }();

    const auto set_expected_amount = [&expected] (const std::size_t index, const std::uint64_t amount) {
        for (auto& ring : expected.expected)
            if (std::addressof(ring) - expected.expected.data() == index)
                ring.amount = amount;
    };

    EXPECT_EQ(expected, lws::pick_random_outputs(10, {rct_amounts, 1}, pick_rct, {}, key_fetch_static{true}));

    expected.expected.push_back(expected.expected.front());
    EXPECT_EQ(expected, lws::pick_random_outputs(10, {rct_amounts, 2}, pick_rct, {}, key_fetch_static{true}));

    expected.expected.push_back(expected.expected.front());
    EXPECT_EQ(expected, lws::pick_random_outputs(10, rct_amounts, pick_rct, {}, key_fetch_static{true}));

    expected.expected.resize(1);
    set_expected_amount(0, 1);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, {non_rct_amounts, 1}, empty_rct, {hist, 1}, key_fetch_static{true}));

    expected.expected.push_back(expected.expected.front());
    set_expected_amount(1, 2);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, {non_rct_amounts, 2}, empty_rct, {hist, 2}, key_fetch_static{true}));
    
    expected.expected.push_back(expected.expected.front());
    set_expected_amount(2, 3);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, non_rct_amounts, empty_rct, hist, key_fetch_static{true}));

    expected.expected.resize(2);
    set_expected_amount(0, 0);
    set_expected_amount(1, 1);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, {mixed_amounts, 2}, pick_rct, {hist, 1}, key_fetch_static{true}));

    expected.expected.push_back(expected.expected.front());
    expected.expected.push_back(expected.expected.front());
    set_expected_amount(2, 0);
    set_expected_amount(3, 2);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, {mixed_amounts, 4}, pick_rct, {hist, 2}, key_fetch_static{true}));

    expected.expected.push_back(expected.expected.front());
    expected.expected.push_back(expected.expected.front());
    set_expected_amount(4, 3);
    set_expected_amount(5, 0);
    EXPECT_EQ(expected, lws::pick_random_outputs(10, mixed_amounts, pick_rct, hist, key_fetch_static{true}));
}

TEST(pick_random_outputs, rct_only)
{
    const std::uint64_t amounts[] = {0, 0, 0};

    tools::gamma_picker pick_rct{};
    {
        std::vector<std::uint64_t> dist{};
        dist.resize(1000);
        {
            std::uint64_t count = 0;
            std::generate(dist.begin(), dist.end(), [&count] { return count += 10; });
        }
        pick_rct = tools::gamma_picker{std::move(dist)};
    }

    EXPECT_EQ((valid_rings{1, 10}), lws::pick_random_outputs(10, {amounts, 1}, pick_rct, {}, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{2, 10}), lws::pick_random_outputs(10, {amounts, 2}, pick_rct, {}, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{3, 10}), lws::pick_random_outputs(10, amounts, pick_rct, {}, key_fetch_random{0.90f}));

    auto rings = MONERO_UNWRAP(lws::pick_random_outputs(100, {amounts, 1}, pick_rct, {}, key_fetch_random{1.0f}));
    std::set<std::uint64_t> sorted{};
    for (auto const& outs : rings)
        for (auto const& out : outs.ring)
            EXPECT_TRUE(sorted.insert(out.index).second);
}

TEST(pick_random_outputs, non_rct_only)
{
    tools::gamma_picker empty_rct{};
    const std::uint64_t amounts[] = {1, 2, 3};
    lws::histogram hist[] = {
        lws::histogram{1, 100, 100, 0},
        lws::histogram{2, 100, 100, 0},
        lws::histogram{3, 200, 100, 0}
    };

    EXPECT_EQ((valid_rings{1, 10}), lws::pick_random_outputs(10, {amounts, 1}, empty_rct, hist, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{2, 10}), lws::pick_random_outputs(10, {amounts, 2}, empty_rct, hist, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{3, 10}), lws::pick_random_outputs(10, amounts, empty_rct, hist, key_fetch_random{0.90f}));
}

TEST(pick_random_outputs, both)
{
    const std::uint64_t amounts[] = {1, 0, 2, 0, 0, 3};
    const std::uint64_t amounts2[] = {0, 4};
    lws::histogram hist[] = {
        lws::histogram{1, 100, 100, 0},
        lws::histogram{2, 100, 100, 0},
        lws::histogram{3, 200, 100, 0},
        lws::histogram{4, 2000, 2000, 0}
    };

    tools::gamma_picker pick_rct{};
    {
        std::vector<std::uint64_t> dist{};
        dist.resize(100);
        {
            std::uint64_t count = 0;
            std::generate(dist.begin(), dist.end(), [&count] { return count += 10; });
        }
        pick_rct = tools::gamma_picker{std::move(dist)};
    }

    EXPECT_EQ((valid_rings{2, 10}), lws::pick_random_outputs(10, {amounts, 2}, pick_rct, hist, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{4, 10}), lws::pick_random_outputs(10, {amounts, 4}, pick_rct, hist, key_fetch_random{0.90f}));
    EXPECT_EQ((valid_rings{6, 10}), lws::pick_random_outputs(10, amounts, pick_rct, hist, key_fetch_random{0.90f}));

    EXPECT_EQ((valid_rings{1, 101}), lws::pick_random_outputs(101, {amounts, 2}, pick_rct, hist, key_fetch_random{0.95f}));
    EXPECT_EQ((valid_rings{1, 1001}), lws::pick_random_outputs(1001, amounts2, pick_rct, hist, key_fetch_random{0.95f}));
}

