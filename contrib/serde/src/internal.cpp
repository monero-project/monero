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

#include "serde/internal/deps.h"
#include "serde/internal/visitor_specializations.h"

namespace serde::internal
{
    const_byte_span string_to_byte_span(const std::string& s) noexcept
    {
        return {reinterpret_cast<const_byte_iterator>(s.data()), s.size()};
    }

    std::string byte_span_to_string(const const_byte_span& bytes)
    {
        return std::string(SPAN_TO_CSTR(bytes), bytes.size());
    }

    #define DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(numtype)                     \
        template<>                                                                               \
        void NumericVisitor<numtype>::visit_bytes(const const_byte_span& bytes)                  \
        {                                                                                        \
            constexpr size_t MAX_NUMERIC_STRING_SIZE = 50;                                       \
            CHECK_AND_ASSERT_THROW_MES                                                           \
            (                                                                                    \
                bytes.size() < MAX_NUMERIC_STRING_SIZE,                                          \
                "potential numeric string is too long: " << internal::byte_span_to_string(bytes) \
            );                                                                                   \
            numtype value;                                                                       \
            std::istringstream ss(internal::byte_span_to_string(bytes));                         \
            ss >> value;                                                                         \
            CHECK_AND_ASSERT_THROW_MES                                                           \
            (                                                                                    \
                !ss.fail(),                                                                      \
                "could not parse numeric string: " << internal::byte_span_to_string(bytes)       \
            );                                                                                   \
            this->visit(std::move(value));                                                       \
        }                                                                                        \

    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(int64_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(int32_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(int16_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(int8_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(uint64_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(uint32_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(uint16_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(uint8_t)
    DEF_EXPLICIT_SPECIALIZATION_FOR_NUMERIC_VISIT_BY_STRING(double)
}