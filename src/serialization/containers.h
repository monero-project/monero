// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <vector>
#include <deque>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include "serialization.h"

namespace serialization
{
    //! @brief Is this type a STL-like container?
    //! To add a new container to be serialized, partially specialize the template is_container like so:
    template <typename T>     struct is_container:                                 std::false_type {};
    template <typename... TA> struct is_container<std::deque<TA...>>:              std::true_type {};
    template <typename... TA> struct is_container<std::map<TA...>>:                std::true_type {};
    template <typename... TA> struct is_container<std::multimap<TA...>>:           std::true_type {};
    template <typename... TA> struct is_container<std::set<TA...>>:                std::true_type {};
    template <typename... TA> struct is_container<std::unordered_map<TA...>>:      std::true_type {};
    template <typename... TA> struct is_container<std::unordered_multimap<TA...>>: std::true_type {};
    template <typename... TA> struct is_container<std::unordered_set<TA...>>:      std::true_type {};
    template <typename... TA> struct is_container<std::vector<TA...>>:             std::true_type {};
}

#include "container.h"

template <class Archive, class Container>
std::enable_if_t<::serialization::is_container<Container>::value, bool>
do_serialize(Archive &ar, Container &c)
{
    return ::do_serialize_container(ar, c);
}
