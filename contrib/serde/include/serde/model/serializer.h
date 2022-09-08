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

#include <string>

#include "../internal/deps.h"

namespace serde::model
{
    struct Serializer
    {
        Serializer() = default;
        virtual ~Serializer() {};

        virtual void serialize_int64  (int64_t)                   = 0;
        virtual void serialize_int32  (int32_t)                   = 0;
        virtual void serialize_int16  (int16_t)                   = 0;
        virtual void serialize_int8   (int8_t)                    = 0;
        virtual void serialize_uint64 (uint64_t)                  = 0;
        virtual void serialize_uint32 (uint32_t)                  = 0;
        virtual void serialize_uint16 (uint16_t)                  = 0;
        virtual void serialize_uint8  (uint8_t)                   = 0;
        virtual void serialize_float64(double)                    = 0;
        virtual void serialize_bytes  (const const_byte_span&)    = 0;
        // serialize_string() defers to serialize_bytes(), for convenience
        virtual void serialize_string (const std::string&)           ;
        virtual void serialize_boolean(bool)                      = 0;

        virtual void serialize_start_array(size_t)                = 0;
        virtual void serialize_end_array()                        = 0;

        virtual void serialize_start_object(size_t)               = 0;
        virtual void serialize_key(const const_byte_span&)        = 0;
        virtual void serialize_end_object()                       = 0;
    };
} // namespace serde::binary

