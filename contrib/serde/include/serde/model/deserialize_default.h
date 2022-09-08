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

#pragma once

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <vector>

#include "deserializer.h"
#include "../internal/enable_if.h"

#define DESERIALIZE_OPERATOR_FRIEND(thisname)                                \
    friend bool deserialize_default(serde::model::Deserializer&, thisname&); \

namespace serde::model
{
    ///////////////////////////////////////////////////////////////////////////
    // deserialize_default() interface                                       //
    ///////////////////////////////////////////////////////////////////////////

    bool deserialize_default(Deserializer& deserializer, int64_t& value);
    bool deserialize_default(Deserializer& deserializer, int32_t& value);
    bool deserialize_default(Deserializer& deserializer, int16_t& value);
    bool deserialize_default(Deserializer& deserializer, int8_t& value);
    bool deserialize_default(Deserializer& deserializer, uint64_t& value);
    bool deserialize_default(Deserializer& deserializer, uint32_t& value);
    bool deserialize_default(Deserializer& deserializer, uint16_t& value);
    bool deserialize_default(Deserializer& deserializer, uint8_t& value);
    bool deserialize_default(Deserializer& deserializer, double& value);
    bool deserialize_default(Deserializer& deserializer, std::string& value);
    bool deserialize_default(Deserializer& deserializer, bool& value);

    template <typename T>
    bool deserialize_default(Deserializer& deserializer, std::list<T>& values);
    template <typename T>
    bool deserialize_default(Deserializer& deserializer, std::set<T>& values);
    template <typename T>
    bool deserialize_default(Deserializer& deserializer, std::vector<T>& values);
}

#include "../internal/deserialize_default.inl"
