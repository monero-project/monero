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
#include <vector>

#include "./blob_visitors.h"
#include "../model/deserializer.h"

namespace serde::model
{
    bool deserialize_as_blob(Deserializer& deserializer, std::string& value);

    template <typename T, typename B = BLOB_TYPE(T)>
    bool deserialize_as_blob(Deserializer& deserializer, T& value)
    {
        internal::BlobVisitor<T> blob_visitor(value);
        deserializer.deserialize_bytes(blob_visitor);
        return blob_visitor.was_visited();
    }

    #define DEF_DESER_AS_BLOB_SPZTION_FOR_CONT(contname)                                \
        template <typename Element, ENABLE_TPARAM_IF(IS_BLOBBABLE(Element))>            \
        bool deserialize_as_blob(Deserializer& deserializer, contname<Element>& values) \
        {                                                                               \
            internal::BlobContainerVisitor<contname<Element>> blob_visitor(values);     \
            deserializer.deserialize_bytes(blob_visitor);                               \
            return blob_visitor.was_visited();                                          \
        }                                                                               \

    DEF_DESER_AS_BLOB_SPZTION_FOR_CONT(std::list)
    DEF_DESER_AS_BLOB_SPZTION_FOR_CONT(std::set)
    DEF_DESER_AS_BLOB_SPZTION_FOR_CONT(std::vector)

    // @TODO: contiguous specializations
}
