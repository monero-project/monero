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
#include <limits>
#include <list>

#include "../internal/deps.h"

namespace serde::model
{
    struct Deserializer;

    struct BasicVisitor
    {
        BasicVisitor();
        virtual ~BasicVisitor();

        virtual std::string expecting() const = 0;

        virtual void visit_int64(int64_t value);
        virtual void visit_int32(int32_t value);
        virtual void visit_int16 (int16_t value);
        virtual void visit_int8(int8_t value);
        virtual void visit_uint64(uint64_t value);
        virtual void visit_uint32(uint32_t value);
        virtual void visit_uint16(uint16_t value);
        virtual void visit_uint8(uint8_t value);
        virtual void visit_float64(double value);
        virtual void visit_bytes(const const_byte_span& value);
        virtual void visit_boolean(bool value);

        virtual void visit_array(optional<size_t> size_hint);
        virtual void visit_end_array();

        virtual void visit_object(optional<size_t> size_hint);
        virtual void visit_key(const const_byte_span& value);
        virtual void visit_end_object();
    }; // struct BasicVisitor

    template <typename Value>
    class RefVisitor: public BasicVisitor
    {
    public:

        RefVisitor(Value& value_ref): m_value_ref(value_ref), m_was_visited(false) {}
        virtual ~RefVisitor() = default;

        bool was_visited() const { return m_was_visited; }

        void visit_end_array() override final {}
        void visit_end_object() override final {}

    protected:

        void visit(Value&& value) { m_value_ref = value; m_was_visited = true; }
    
    private:

        Value& m_value_ref;
        bool m_was_visited;
    }; // class GetSetVisitor

    class ProbeVisitor: public BasicVisitor
    {
    public:

        enum class EventType
        {
            Int64, Int32, Int16, Int8,
            UInt64, UInt32, UInt16, UInt8,
            Float64, String, Boolean,
            StartArray, EndArray, StartObject, Key, EndObject,
            Unvisited
        };

        class Event
        {
        public:

            static constexpr size_t NO_SIZE = std::numeric_limits<size_t>::max();

            constexpr EventType get_type() const { return m_type; }
            constexpr bool has_size_hint() const { return m_size_hint != NO_SIZE; }

            constexpr int64_t get_int64() const { return m_int64; }
            constexpr int32_t get_int32() const { return m_int32; }
            constexpr int16_t get_int16() const { return m_int16; }
            constexpr int8_t get_int8() const { return m_int8; }
            constexpr uint64_t get_uint64() const { return m_uint64; }
            constexpr uint32_t get_uint32() const { return m_uint32; }
            constexpr uint16_t get_uint16() const { return m_uint16; }
            constexpr uint8_t get_uint8() const { return m_uint8; }
            constexpr double get_float64() const { return m_float64; }
            constexpr bool get_boolean() const { return m_boolean; }

            std::string get_string_copy() const { return m_string; }
            std::string take_string() { return std::move(m_string); }
 
        private:

            Event(): m_type(EventType::Unvisited), m_string() {}

            EventType m_type;

            union
            {
                int64_t m_int64;
                int32_t m_int32;
                int16_t m_int16;
                int8_t m_int8;
                uint64_t m_uint64;
                uint32_t m_uint32;
                uint16_t m_uint16;
                uint8_t m_uint8;
                double m_float64;
                bool m_boolean;
                size_t m_size_hint;
            };

            std::string m_string;

            friend class ProbeVisitor;
        }; // class Event

        // Static Probe Interface
        static Event probe(Deserializer& deserializer);
        static bool assert_array_begin(Deserializer& deserializer, optional<size_t> size_hint = {});
        static bool assert_object_begin(Deserializer& deserializer, optional<size_t> size_hint = {});

        ProbeVisitor(): m_event() {}

        Event take_event() { return std::move(m_event); }

        // Implement BasicVisitor interface
        std::string expecting() const override final;
        void visit_int64(int64_t value) override final;
        void visit_int32(int32_t value) override final;
        void visit_int16 (int16_t value) override final;
        void visit_int8(int8_t value) override final;
        void visit_uint64(uint64_t value) override final;
        void visit_uint32(uint32_t value) override final;
        void visit_uint16(uint16_t value) override final;
        void visit_uint8(uint8_t value) override final;
        void visit_float64(double value) override final;
        void visit_bytes(const const_byte_span& value) override final;
        void visit_boolean(bool value) override final;
        void visit_array(optional<size_t> size_hint) override final;
        void visit_end_array() override final;
        void visit_object(optional<size_t> size_hint) override final;
        void visit_key(const const_byte_span& value) override final;
        void visit_end_object() override final;

    private:

        Event m_event;
    }; // class ProbeVisitor
} // namespace serde::model
