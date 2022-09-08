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

#include <type_traits>

#include "deps.h"

namespace serde::internal
{
    template <typename T>
    struct le_conversion 
    {
        static constexpr bool needed_for_type() { return false; }
        inline static constexpr T convert(T value) { return value; }
    };

    template <typename T>
    constexpr bool should_convert_pod()
    {
        #if BYTE_ORDER == BIG_ENDIAN
            constexpr bool is_big_endian = true;
        #else
            constexpr bool is_big_endian = false;
        #endif

        constexpr bool is_pod = std::is_pod<T>::value;
        constexpr bool is_needed_for_type = le_conversion<T>::needed_for_type();

        return is_big_endian && is_pod && is_needed_for_type;
    }

    #define SPECIALIZE_INT_CONVERSION(b)                                                          \
        template<> constexpr                                                                      \
        bool le_conversion<int##b##_t>::needed_for_type() { return true; }                        \
        template<> constexpr                                                                      \
        int##b##_t le_conversion<int##b##_t>::convert(int##b##_t v) { return SWAP##b##LE(v); }    \
        template<> constexpr                                                                      \
        bool le_conversion<uint##b##_t>::needed_for_type() { return true; }                       \
        template<> constexpr                                                                      \
        uint##b##_t le_conversion<uint##b##_t>::convert(uint##b##_t v) { return SWAP##b##LE(v); } \

    SPECIALIZE_INT_CONVERSION(64)
    SPECIALIZE_INT_CONVERSION(32)
    SPECIALIZE_INT_CONVERSION(16)
    // @TODO: double endianness
} // namespace serde::internal

// @TODO passthough if LE system
#ifndef CONVERT_POD
    #define CONVERT_POD(x)                                                   \
        (serde::internal::should_convert_pod<decltype(x)>()       \
        ? serde::internal::le_conversion<decltype(x)>::convert(x) \
        : x)
#endif