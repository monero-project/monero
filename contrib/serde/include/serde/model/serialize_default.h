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

#include <list>
#include <set>
#include <string>
#include <vector>

#include "serializer.h"
#include "../internal/enable_if.h"
#include "../internal/endianness.h"

#define SERIALIZE_OPERATOR_FRIEND(thisname)                                    \
    friend void serialize_default(const thisname&, serde::model::Serializer&); \

namespace serde::model
{
    ///////////////////////////////////////////////////////////////////////////
    // Default serialization interface                                       //
    ///////////////////////////////////////////////////////////////////////////

    void serialize_default(int64_t, Serializer&);
    void serialize_default(int32_t, Serializer&);
    void serialize_default(int16_t, Serializer&);
    void serialize_default(int8_t, Serializer&);
    void serialize_default(uint64_t, Serializer&);
    void serialize_default(uint32_t, Serializer&);
    void serialize_default(uint16_t, Serializer&);
    void serialize_default(uint8_t, Serializer&);
    void serialize_default(double, Serializer&);
    void serialize_default(const std::string&, Serializer&);
    void serialize_default(bool, Serializer&);

    ///////////////////////////////////////////////////////////////////////////
    // container overloads                                                   //
    ///////////////////////////////////////////////////////////////////////////

    template <typename Element>
    void serialize_default(const std::list<Element>& cont, Serializer& serializer);

    template <typename Element>
    void serialize_default(const std::set<Element>& cont, Serializer& serializer);

    template <typename Element>
    void serialize_default(const std::vector<Element>& cont, Serializer& serializer);
}

#include "../internal/serialize_default.inl"
