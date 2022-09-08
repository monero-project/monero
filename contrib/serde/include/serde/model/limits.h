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

#include <cstddef>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace serde::limits
{
    constexpr size_t DEFAULT_MAX_PACKET_SIZE = 100000000; // 100 MB
    constexpr size_t DEFAULT_MAX_TOTAL_CONTAINERS = 10000; // 10 thousand
    constexpr size_t DEFAULT_MAX_DEPTH = 32;

    /*
     * @brief Rough resource limits which should be enforced by Deserializers
     *
     * Scalars (ints, double, bool, etc) can live on the stack, and they use a similar amount of
     * memory serialized vs unserialized. On the other hand, containers require heap allocation
     * and may consume much more memory when deserialized as compared to their serialized
     * counterparts. The combination of max_packet_size and max_total_containers should construct
     * a rough _portable_ ceiling on the amount of RAM/CPU a Deserializer can consume per packet.
     *
     * The max_depth_sanity limit prevents stack overflows and other DoS attacks.
     */
    struct ResourceLimits
    {
        size_t max_packet_size; // !< Max size (in raw bytes) of message allowed to be deserialized
        size_t max_total_containers; // !< Max number of total visit_(array/object) calls allowed
        size_t max_depth_sanity; // !< Max number of nested visit_(array/object) calls allowed

        /*
         * @brief Default constructor creates object with default limits specificied in limits.h
         */
        constexpr ResourceLimits():
            max_packet_size(DEFAULT_MAX_PACKET_SIZE),
            max_total_containers(DEFAULT_MAX_TOTAL_CONTAINERS),
            max_depth_sanity(DEFAULT_MAX_DEPTH)
        {}

        constexpr ResourceLimits(const ResourceLimits& other) = default;
    };

    constexpr size_t DEFAULT_MAX_STRING_LENGTH = std::numeric_limits<size_t>::max();
    constexpr size_t DEFAULT_MIN_KEY_LENGTH = 1;
    constexpr size_t DEFAULT_MAX_KEY_LENGTH = 255;
    constexpr size_t DEFAULT_MAX_ARRAY_WIDTH = std::numeric_limits<size_t>::max();
    constexpr size_t DEFAULT_MAX_OBJECT_WIDTH = std::numeric_limits<size_t>::max();

    struct TypedLimits
    {
        size_t max_string_length;
        size_t min_key_length;
        size_t max_key_length;
        size_t max_array_width;
        size_t max_object_width;
        size_t max_depth;

        constexpr TypedLimits():
            max_string_length(DEFAULT_MAX_STRING_LENGTH),
            min_key_length(DEFAULT_MIN_KEY_LENGTH),
            max_key_length(DEFAULT_MAX_KEY_LENGTH),
            max_array_width(DEFAULT_MAX_ARRAY_WIDTH),
            max_object_width(DEFAULT_MAX_OBJECT_WIDTH),
            max_depth(DEFAULT_MAX_DEPTH)
        {}

        constexpr TypedLimits(const TypedLimits& other) = default;

        constexpr TypedLimits
        (
            size_t max_string_length, size_t min_key_length,
            size_t max_key_length, size_t max_array_width,
            size_t max_object_width, size_t max_depth
        ):
            max_string_length(max_string_length), min_key_length(min_key_length),
            max_key_length(max_key_length), max_array_width(max_array_width),
            max_object_width(max_object_width), max_depth(max_depth)
        {}

        constexpr TypedLimits operator|(const TypedLimits& other)
        {
            return TypedLimits
            (
                std::max(max_string_length, other.max_string_length),
                std::min(min_key_length, other.min_key_length),
                std::max(max_key_length, other.max_key_length),
                std::max(max_array_width, other.max_array_width),
                std::max(max_object_width, other.max_object_width),
                std::max(max_depth, other.max_depth)
            );
        }
    };
    
    struct Limits
    {
        TypedLimits typed;
        ResourceLimits resource;

        constexpr size_t max_depth() const
        {
            return std::min(typed.max_depth, resource.max_depth_sanity);
        }
    }; // struct Limits
} // namespace serde::limits
