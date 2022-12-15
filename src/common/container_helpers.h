// Copyright (c) 2022, The Monero Project
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

// Miscellaneous container helpers.

#pragma once

//local headers

//third party headers

//standard headers
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

//forward declarations


namespace tools
{

/// use operator< to get operator==
/// WARNING: equality is not always implied by operator<, depending on implementation
struct equals_from_less final
{
    template <typename T>
    bool operator()(const T &a, const T &b) { return !(a < b) && !(b < a); }
};
/// note: test for sorted and uniqueness using the same criteria (operator<)
template <typename T>
bool is_sorted_and_unique(const T& container)
{
    if (!std::is_sorted(container.begin(), container.end()))
        return false;

    if (std::adjacent_find(container.begin(), container.end(), equals_from_less{}) != container.end())
        return false;

    return true;
}
/// convenience wrapper for checking if a mapped object is mapped to a key embedded in that object
/// example: std::unorderd_map<rct::key, std::pair<rct::key, rct::xmr_amount>> where the map key is supposed to
///   reproduce the pair's rct::key; use the predicate to get the pair's rct::key element
template <typename KeyT, typename ValueT>
bool keys_match_internal_values(const std::unordered_map<KeyT, ValueT> &map,
    const std::function<
            const typename std::unordered_map<KeyT, ValueT>::key_type&
            (const typename std::unordered_map<KeyT, ValueT>::mapped_type&)
        > &get_internal_key_func)
{
    for (const auto &map_element : map)
    {
        if (!(map_element.first == get_internal_key_func(map_element.second)))
            return false;
    }

    return true;
}
/// convenience wrapper for getting the last element after emplacing back
template <typename ContainerT>
typename ContainerT::value_type& add_element(ContainerT &container)
{
    container.emplace_back();
    return container.back();
}
/// convenience erasor for unordered maps: std::erase_if(std::unordered_map) is C++20
template <typename KeyT, typename ValueT>
void for_all_in_map_erase_if(std::unordered_map<KeyT, ValueT> &map_inout,
    const std::function<bool(const typename std::unordered_map<KeyT, ValueT>::value_type&)> &predicate)
{
    for (auto map_it = map_inout.begin(); map_it != map_inout.end();)
    {
        if (predicate(*map_it))
            map_it = map_inout.erase(map_it);
        else
            ++map_it;
    }
}

} //namespace tools
