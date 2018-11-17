// Copyright (c) 2018-2019, The Monero Project
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

#include "random_outputs.h"

#include <algorithm>
#include <boost/range/combine.hpp>
#include <cmath>
#include <limits>
#include <random>
#include <utility>

#include "cryptonote_config.h"
#include "light_wallet_server/error.h"
#include "wallet/output_selection.h"

#include <iostream>
#include <boost/spirit/include/karma.hpp>

namespace lws
{
    namespace
    {
        struct by_amount
        {
            template<typename T>
            bool operator()(T const& left, T const& right) const noexcept
            {
                return left.amount < right.amount;
            }

            template<typename T>
            bool operator()(std::uint64_t left, T const& right) const noexcept
            {
                return left < right.amount;
            }

            template<typename T>
            bool operator()(T const& left, std::uint64_t right) const noexcept
            {
                return left.amount < right;
            }
        };

        struct by_index
        {
            template<typename T, typename U>
            bool operator()(T const& left, U const& right) const noexcept
            {
                return left.index < right.index;
            }
        };

        struct same_index
        {
            bool operator()(lws::output_ref const& left, lws::output_ref const& right) const noexcept
            {
                return left.index == right.index;
            }
        };

        expect<void> pick_all(epee::span<lws::output_ref> out, const std::uint64_t amount) noexcept
        {
            static_assert(
                std::numeric_limits<std::size_t>::max() <= std::numeric_limits<std::uint64_t>::max(),
                "size_t is really large"
            );

            std::size_t index = 0;
            for (auto& entry : out)
            {
                entry.amount = amount;
                entry.index = index++;
            }
            return success();
        }

        expect<void> triangular_pick(epee::span<lws::output_ref> out, lws::histogram const& hist)
        {
            MONERO_PRECOND(hist.unlocked_count <= hist.total_count);

            if (hist.unlocked_count < out.size())
                return {lws::error::not_enough_mixin};

            if (hist.unlocked_count == out.size())
                return pick_all(out, hist.amount);

            /* This does not match the wallet2 selection code - recents are not
            considered. There should be no new recents for this selection
            algorithm because it is only used for non-ringct outputs. */

            static constexpr const std::uint64_t max = std::uint64_t(1) << 53;
            for (auto& entry : out)
            {
                entry.amount = hist.amount;

               /* \TODO Update REST API to send real outputs so selection
               algorithm can use fork information (like wallet2). */

                do
                {
                    const std::uint64_t r = crypto::rand<std::uint64_t>() % max;
                    const double frac = std::sqrt(double(r) / double(max));
                    entry.index = std::uint64_t(frac * double(hist.total_count));
                } while (hist.unlocked_count < entry.index);
            }

            return success();
        }

        expect<void> gamma_pick(epee::span<lws::output_ref> out, tools::gamma_picker& pick_rct)
        {
            if (!pick_rct)
                return {lws::error::not_enough_mixin};

            const std::uint64_t spendable = pick_rct.spendable_upper_bound();
            if (spendable < out.size())
                return {lws::error::not_enough_mixin};
            if (spendable == out.size())
                return pick_all(out, 0);

            for (auto& entry : out)
            {
                entry.amount = 0;
                entry.index = pick_rct();

                /* \TODO Update REST API to send real outputs so selection
                algorithm can use fork information (like wallet2). */
            }
            return success();
        }
    }

    expect<std::vector<random_ring>> pick_random_outputs(
        const std::uint32_t mixin,
        const epee::span<const std::uint64_t> amounts, 
        tools::gamma_picker& pick_rct,
        epee::span<histogram> histograms,
        const std::function<key_fetcher> fetch
    ) {
        if (mixin == 0 || amounts.empty())
            return std::vector<random_ring>{amounts.size()};

        const std::size_t sizet_max = std::numeric_limits<std::size_t>::max();
        MONERO_PRECOND(bool(fetch));
        MONERO_PRECOND(mixin <= (sizet_max / amounts.size()));

        std::vector<output_ref> proposed{};
        std::vector<random_ring> rings{};
        rings.resize(amounts.size());

        for (auto ring : boost::combine(amounts, rings))
            boost::get<1>(ring).amount = boost::get<0>(ring);

        std::sort(histograms.begin(), histograms.end(), by_amount{});
        for (unsigned tries = 0; tries < 64; ++tries)
        {
            proposed.clear();
            proposed.reserve(rings.size() * mixin);

            // select indexes foreach ring below mixin count
            for (auto ring = rings.begin(); ring != rings.end(); /* handled below */)
            {
                const std::size_t count = proposed.size();
                if (ring->ring.size() < mixin)
                {
                    const std::size_t diff = mixin - ring->ring.size();
                    proposed.resize(proposed.size() + diff);
                    {
                        const epee::span<output_ref> latest{proposed.data() + count, diff};

                        expect<void> picked{};
                        const std::uint64_t amount = ring->amount;
                        if (amount == 0)
                            picked = gamma_pick(latest, pick_rct);
                        else
                        {
                            const auto match =
                                std::lower_bound(histograms.begin(), histograms.end(), amount, by_amount{});
                            MONERO_PRECOND(match != histograms.end() && match->amount == amount);
                            picked = triangular_pick(latest, *match);
                        }

                        if (!picked)
                        {
                            if (picked == lws::error::not_enough_mixin)
                            {
                                proposed.resize(proposed.size() - diff);
                                ring = rings.erase(ring);
                                continue;
                            }
                            return picked.error();
                        }

                        // drop dupes in latest selection
                        std::sort(latest.begin(), latest.end(), by_index{});
                        const auto last = std::unique(latest.begin(), latest.end(), same_index{});
                        proposed.resize(last - proposed.data());
                    }

                    ring->ring.reserve(mixin);
                    epee::span<random_output> current = epee::to_mut_span(ring->ring);
                    std::sort(current.begin(), current.end(), by_index{});

                    // See if new list has duplicates with existing ring
                    for (auto ref = proposed.begin() + count; ref < proposed.end(); /* see branches */ )
                    {
                        // must update after push_back call
                        current = {ring->ring.data(), current.size()};

                        const auto match =
                            std::lower_bound(current.begin(), current.end(), *ref, by_index{});
                        if (match == current.end() || match->index != ref->index)
                        {
                            ring->ring.push_back(random_output{{}, ref->index});
                            ring->ring.back().keys.unlocked = false; // for tracking below
                            ++ref;
                        }
                        else // dupe
                            ref = proposed.erase(ref);
                    }
                }

                ++ring;
            }

            // all amounts lack enough mixin
            if (rings.empty())
                return rings;

            /* \TODO For maximum privacy, the real outputs need to be fetched
            below. This requires an update of the REST API. */

            // fetch all new keys in one shot
            const std::size_t expected = proposed.size();
            auto result = fetch(std::move(proposed));
            if (!result)
                return result.error();

            if (expected != result->size())
                return {lws::error::bad_daemon_response};

            bool done = true;
            std::size_t offset = 0;
            for (auto& ring : rings)
            {
                // this should never fail, else the logic in here is bad
                assert(ring.ring.size() <= ring.ring.size());

                // if we dropped a selection due to dupe, must try again
                done = (done && mixin <= ring.ring.size());

                for (auto entry = ring.ring.begin(); entry < ring.ring.end(); /* see branches */)
                {
                    // check only new keys
                    if (entry->keys.unlocked)
                        ++entry;
                    else
                    {
                        if (result->size() <= offset)
                            return {lws::error::bad_daemon_response};

                        output_keys const& keys = result->at(offset);
                        ++offset;

                        if (keys.unlocked)
                        {
                            entry->keys = keys;
                            ++entry;
                        }
                        else
                        {
                            done = false;
                            entry = ring.ring.erase(entry);
                        }
                    }
                }
            }
            assert(offset == result->size());

            if (done)
                return {std::move(rings)};
        }

        return {lws::error::not_enough_mixin};
    }
}

