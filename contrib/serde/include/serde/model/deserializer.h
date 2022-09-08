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

#include <vector>

#include "./limits.h"
#include "../internal/deps.h"

namespace serde::model {
    struct BasicVisitor;

    struct Deserializer
    {
        virtual ~Deserializer();

        virtual void deserialize_any(BasicVisitor&) = 0;

        virtual void deserialize_int64  (BasicVisitor&) = 0;
        virtual void deserialize_int32  (BasicVisitor&) = 0;
        virtual void deserialize_int16  (BasicVisitor&) = 0;
        virtual void deserialize_int8   (BasicVisitor&) = 0;
        virtual void deserialize_uint64 (BasicVisitor&) = 0;
        virtual void deserialize_uint32 (BasicVisitor&) = 0;
        virtual void deserialize_uint16 (BasicVisitor&) = 0;
        virtual void deserialize_uint8  (BasicVisitor&) = 0;
        virtual void deserialize_float64(BasicVisitor&) = 0;
        virtual void deserialize_bytes  (BasicVisitor&) = 0;
        virtual void deserialize_boolean(BasicVisitor&) = 0;

        virtual void deserialize_array(optional<size_t>, BasicVisitor&) = 0;
        virtual void deserialize_end_array(BasicVisitor&) = 0;

        virtual void deserialize_object(optional<size_t>, BasicVisitor&) = 0;
        virtual void deserialize_key(BasicVisitor&) = 0;
        virtual void deserialize_end_object(BasicVisitor&) = 0;
    };

    class StatedDeserializer: public Deserializer
    {
    protected:

        void previsit_int64  (               int64_t, BasicVisitor&);
        void previsit_int32  (               int32_t, BasicVisitor&);
        void previsit_int16  (               int16_t, BasicVisitor&);
        void previsit_int8   (                int8_t, BasicVisitor&);
        void previsit_uint64 (              uint64_t, BasicVisitor&);
        void previsit_uint32 (              uint32_t, BasicVisitor&);
        void previsit_uint16 (              uint16_t, BasicVisitor&);
        void previsit_uint8  (               uint8_t, BasicVisitor&);
        void previsit_float64(                double, BasicVisitor&);
        void previsit_bytes  (const const_byte_span&, BasicVisitor&);
        void previsit_boolean(                  bool, BasicVisitor&);

        void previsit_array(optional<size_t>, BasicVisitor&);
        void previsit_end_array(BasicVisitor&);

        void previsit_object(optional<size_t>, BasicVisitor&);
        void previsit_key(const const_byte_span&, BasicVisitor&);
        void previsit_end_object(BasicVisitor&);
    
    private:


    };

    // All deserialize_* methods defer to deserialize_any, so base classes only have to define that
    // one method. Useful for implementing Deserializers for a self-describing formats.
    struct SelfDescribingDeserializer: public Deserializer
    {
        void deserialize_int64  (BasicVisitor&) override final;
        void deserialize_int32  (BasicVisitor&) override final;
        void deserialize_int16  (BasicVisitor&) override final;
        void deserialize_int8   (BasicVisitor&) override final;
        void deserialize_uint64 (BasicVisitor&) override final;
        void deserialize_uint32 (BasicVisitor&) override final;
        void deserialize_uint16 (BasicVisitor&) override final;
        void deserialize_uint8  (BasicVisitor&) override final;
        void deserialize_float64(BasicVisitor&) override final;
        void deserialize_bytes  (BasicVisitor&) override final;
        void deserialize_boolean(BasicVisitor&) override final;

        void deserialize_array(optional<size_t>, BasicVisitor&) override final;
        void deserialize_end_array(BasicVisitor&) override final;

        void deserialize_object(optional<size_t>, BasicVisitor&) override final;
        void deserialize_key(BasicVisitor&) override final;
        void deserialize_end_object(BasicVisitor&) override final;
    };
} // namespace serde::model
