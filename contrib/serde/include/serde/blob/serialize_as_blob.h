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

#include "./blobbable.h"
#include "../internal/endianness.h"
#include "../model/serializer.h"

namespace serde::model
{
    void serialize_as_blob(const std::string&, Serializer&);

    template <typename T, typename B = BLOB_TYPE(T)>
    ENABLE_IF(!internal::should_convert_pod<B>())
    serialize_as_blob(const T& value, Serializer& serializer)
    {
        serializer.serialize_bytes({reinterpret_cast<const_byte_iterator>(&value), sizeof(T)});
    }

    template <typename T, typename B = BLOB_TYPE(T)>
    ENABLE_IF(internal::should_convert_pod<B>())
    serialize_as_blob(const T& value, Serializer& serializer)
    {
        const B conv_value = internal::le_conversion<B>::convert(static_cast<const B&>(value));
        const auto conv_begin = reinterpret_cast<const_byte_iterator>(&conv_value);
        serializer.serialize_bytes({conv_begin, sizeof(B)});
    }

    #define DEF_SERIALIZE_AS_BLOB_FOR_CONTAINER(contname)                            \
        template <typename Elem, typename B = BLOB_TYPE(Elem)>                       \
        void serialize_as_blob(const contname<Elem>& cont, Serializer& serializer) { \
            const size_t blob_size = cont.size() * sizeof(B);                        \
            std::string blob(blob_size, '\0');                                       \
            B* blob_ptr = reinterpret_cast<B*>(&blob.front());                       \
            for (const auto& elem: cont) {                                           \
                *(blob_ptr++) = CONVERT_POD(elem);                                   \
            }                                                                        \
            serializer.serialize_string(blob);                                       \
        }                                                                            \
    
    DEF_SERIALIZE_AS_BLOB_FOR_CONTAINER(std::list)
    DEF_SERIALIZE_AS_BLOB_FOR_CONTAINER(std::set)
    DEF_SERIALIZE_AS_BLOB_FOR_CONTAINER(std::vector)

    // @TODO: specialization for contiguous containers (remember vector<bool> evilness)
    // @TODO: should_convert_pod specialization for container
}
