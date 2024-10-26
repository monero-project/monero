// Copyright (c) 2019-2024, The Monero Project
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

#include "dandelionpp.h"

#include <boost/container/small_vector.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <chrono>

#include "common/expect.h"
#include "cryptonote_config.h"
#include "crypto/crypto.h"

namespace net
{
namespace dandelionpp
{
    namespace
    {
        constexpr const std::size_t expected_max_channels = CRYPTONOTE_NOISE_CHANNELS;

        // could be in util somewhere
        struct key_less
        {
            template<typename K, typename V>
            bool operator()(const std::pair<K, V>& left, const K& right) const
            {
                return left.first < right;
            }

            template<typename K, typename V>
            bool operator()(const K& left, const std::pair<K, V>& right) const
            {
                return left < right.first;
            }
        };

        std::size_t select_stem(epee::span<const std::size_t> usage, epee::span<const boost::uuids::uuid> out_map)
        {
            assert(usage.size() < std::numeric_limits<std::size_t>::max()); // prevented in constructor
            if (usage.size() < out_map.size())
                return std::numeric_limits<std::size_t>::max();

            // small_vector uses stack space if `expected_max_channels < capacity()`
            std::size_t lowest = std::numeric_limits<std::size_t>::max();
            boost::container::small_vector<std::size_t, expected_max_channels> choices;
            static_assert(sizeof(choices) < 256, "choices is too large based on current configuration");

            for (const boost::uuids::uuid& out : out_map)
            {
                if (!out.is_nil())
                {
                    const std::size_t location = std::addressof(out) - out_map.begin();
                    if (usage[location] < lowest)
                    {
                        lowest = usage[location];
                        choices = {location};
                    }
                    else if (usage[location] == lowest)
                        choices.push_back(location);
                }
            }

            switch (choices.size())
            {
            case 0:
                return std::numeric_limits<std::size_t>::max();
            case 1:
                return choices[0];
            default:
                break;
            }

            return choices[crypto::rand_idx(choices.size())];
        }
    } // anonymous

    connection_map::connection_map(std::vector<boost::uuids::uuid> out_connections, const std::size_t stems)
      : out_mapping_(std::move(out_connections)),
        in_mapping_(),
        usage_count_()
    {
        // max value is used by `select_stem` as error case
        if (stems == std::numeric_limits<std::size_t>::max())
            MONERO_THROW(common_error::kInvalidArgument, "stems value cannot be max size_t");

        usage_count_.resize(stems);
        if (stems < out_mapping_.size())
        {
            for (unsigned i = 0; i < stems; ++i)
                std::swap(out_mapping_[i], out_mapping_.at(i + crypto::rand_idx(out_mapping_.size() - i)));

            out_mapping_.resize(stems);
        }
        else
        {
            std::shuffle(out_mapping_.begin(), out_mapping_.end(), crypto::random_device{});
        }
    }

    connection_map::~connection_map() noexcept
    {}

    connection_map connection_map::clone() const
    {
        return {*this};
    }

    bool connection_map::update(std::vector<boost::uuids::uuid> current)
    {
        std::sort(current.begin(), current.end());

        bool replace = false;
        for (auto& existing_out : out_mapping_)
        {
            const auto elem = std::lower_bound(current.begin(), current.end(), existing_out);
            if (elem == current.end() || *elem != existing_out)
            {
                existing_out = boost::uuids::nil_uuid();
                replace = true;
            }
            else // already using connection, remove it from candidate list
                current.erase(elem);
        }

        if (!replace && out_mapping_.size() == usage_count_.size())
            return false;

        const std::size_t existing_outs = out_mapping_.size();
        for (std::size_t i = 0; i < usage_count_.size() && !current.empty(); ++i)
        {
            const bool increase_stems = out_mapping_.size() <= i;
            if (increase_stems || out_mapping_[i].is_nil())
            {
                std::swap(current.back(), current.at(crypto::rand_idx(current.size())));
                if (increase_stems)
                    out_mapping_.push_back(current.back());
                else
                    out_mapping_[i] = current.back();
                current.pop_back();
            }
        }

        return replace || existing_outs < out_mapping_.size();
    }

    std::size_t connection_map::size() const noexcept
    {
        std::size_t count = 0;
        for (const boost::uuids::uuid& connection : out_mapping_)
        {
            if (!connection.is_nil())
                ++count;
        }
        return count;
    }

    boost::uuids::uuid connection_map::get_stem(const boost::uuids::uuid& source)
    {
        auto elem = std::lower_bound(in_mapping_.begin(), in_mapping_.end(), source, key_less{});
        if (elem == in_mapping_.end() || elem->first != source)
        {
            const std::size_t index = select_stem(epee::to_span(usage_count_), epee::to_span(out_mapping_));
            if (out_mapping_.size() < index)
                return boost::uuids::nil_uuid();

            elem = in_mapping_.emplace(elem, source, index);
            usage_count_[index]++;
        }
        else if (out_mapping_.at(elem->second).is_nil()) // stem connection disconnected after mapping
        {
            usage_count_.at(elem->second)--;
            const std::size_t index = select_stem(epee::to_span(usage_count_), epee::to_span(out_mapping_));
            if (out_mapping_.size() < index)
            {
                in_mapping_.erase(elem);
                return boost::uuids::nil_uuid();
            }

            elem->second = index;
            usage_count_[index]++;
        }

        return out_mapping_[elem->second];
    }
} // dandelionpp
} // net
