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

#include "serde/model/visitor.h"

#include "serde/internal/visitor_specializations.h"
#include "serde/model/deserialize_default.h"
#include "serde/model/serialize_default.h"

namespace serde::model
{
    ///////////////////////////////////////////////////////////////////////////
    // deserialize_default.h                                                 //
    ///////////////////////////////////////////////////////////////////////////

    #define DEF_DEFAULT_DESERIALIZE_NUM(tyname, mname)                      \
        bool deserialize_default(Deserializer& deserializer, tyname& value) \
        {                                                                   \
            internal::NumericVisitor<tyname> visitor(value);                \
            deserializer.deserialize_##mname(visitor);                      \
            return visitor.was_visited();                                   \
        }                                                                   \
    
    DEF_DEFAULT_DESERIALIZE_NUM(int64_t, int64)
    DEF_DEFAULT_DESERIALIZE_NUM(int32_t, int32)
    DEF_DEFAULT_DESERIALIZE_NUM(int16_t, int16)
    DEF_DEFAULT_DESERIALIZE_NUM(int8_t, int8)
    DEF_DEFAULT_DESERIALIZE_NUM(uint64_t, uint64)
    DEF_DEFAULT_DESERIALIZE_NUM(uint32_t, uint32)
    DEF_DEFAULT_DESERIALIZE_NUM(uint16_t, uint16)
    DEF_DEFAULT_DESERIALIZE_NUM(uint8_t, uint8)
    DEF_DEFAULT_DESERIALIZE_NUM(double, float64)
    DEF_DEFAULT_DESERIALIZE_NUM(bool, boolean)

    bool deserialize_default(Deserializer& deserializer, std::string& value)
    {
        internal::StringVisitor visitor(value);
        deserializer.deserialize_bytes(visitor);
        return visitor.was_visited();
    }

    ///////////////////////////////////////////////////////////////////////////
    // deserializer.h                                                        //
    ///////////////////////////////////////////////////////////////////////////

    Deserializer::~Deserializer() {}

    #define DEFER_DESER_SIMPLE_TO_ANY(mname)                                               \
        void SelfDescribingDeserializer::deserialize_##mname(model::BasicVisitor& visitor) \
            { return this->deserialize_any(visitor); }

    DEFER_DESER_SIMPLE_TO_ANY(int64)
    DEFER_DESER_SIMPLE_TO_ANY(int32)
    DEFER_DESER_SIMPLE_TO_ANY(int16)
    DEFER_DESER_SIMPLE_TO_ANY(int8)
    DEFER_DESER_SIMPLE_TO_ANY(uint64)
    DEFER_DESER_SIMPLE_TO_ANY(uint32)
    DEFER_DESER_SIMPLE_TO_ANY(uint16)
    DEFER_DESER_SIMPLE_TO_ANY(uint8)
    DEFER_DESER_SIMPLE_TO_ANY(float64)
    DEFER_DESER_SIMPLE_TO_ANY(bytes)
    DEFER_DESER_SIMPLE_TO_ANY(boolean)
    DEFER_DESER_SIMPLE_TO_ANY(key)
    DEFER_DESER_SIMPLE_TO_ANY(end_array)
    DEFER_DESER_SIMPLE_TO_ANY(end_object)

    void SelfDescribingDeserializer::deserialize_array(optional<size_t>, BasicVisitor& visitor)
    { this->deserialize_any(visitor); }

    void SelfDescribingDeserializer::deserialize_object(optional<size_t>, BasicVisitor& visitor)
    { this->deserialize_any(visitor); }

    ///////////////////////////////////////////////////////////////////////////
    // serialize_default.h                                                   //
    ///////////////////////////////////////////////////////////////////////////

    #define DEF_SERIALIZE_DEFAULT_COPIED(sername, tyname)            \
        void serialize_default(tyname value, Serializer& serializer) \
        {                                                            \
            serializer.serialize_##sername(value);                   \
        }                                                            \
    
    DEF_SERIALIZE_DEFAULT_COPIED(int64, int64_t)
    DEF_SERIALIZE_DEFAULT_COPIED(int32, int32_t)
    DEF_SERIALIZE_DEFAULT_COPIED(int16, int16_t)
    DEF_SERIALIZE_DEFAULT_COPIED(int8, int8_t)
    DEF_SERIALIZE_DEFAULT_COPIED(uint64, uint64_t)
    DEF_SERIALIZE_DEFAULT_COPIED(uint32, uint32_t)
    DEF_SERIALIZE_DEFAULT_COPIED(uint16, uint16_t)
    DEF_SERIALIZE_DEFAULT_COPIED(uint8, uint8_t)
    DEF_SERIALIZE_DEFAULT_COPIED(float64, double)
    DEF_SERIALIZE_DEFAULT_COPIED(boolean, bool)

    void serialize_default(const std::string& value, Serializer& serializer)
    {
        serializer.serialize_bytes(internal::string_to_byte_span(value));
    }

    ///////////////////////////////////////////////////////////////////////////
    // serializer.h                                                          //
    ///////////////////////////////////////////////////////////////////////////

    void Serializer::serialize_string(const std::string& value)
    {
        this->serialize_bytes(internal::string_to_byte_span(value));
    }

    ///////////////////////////////////////////////////////////////////////////
    // visitor.h                                                             //
    ///////////////////////////////////////////////////////////////////////////

    BasicVisitor::BasicVisitor() {}
    BasicVisitor::~BasicVisitor() {}

    #define DEF_BASIC_VISITOR_FALLBACK_METHOD(tyname, mname)                        \
        void BasicVisitor::visit_##mname(tyname)                                    \
        {                                                                           \
            ASSERT_MES_AND_THROW                                                    \
            (                                                                       \
                "called visit_" #mname "() but was expecting " << this->expecting() \
            );                                                                      \
        }                                                                           \
    
    DEF_BASIC_VISITOR_FALLBACK_METHOD(int64_t, int64)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(int32_t, int32)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(int16_t, int16)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(int8_t, int8)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(uint64_t, uint64)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(uint32_t, uint32)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(uint16_t, uint16)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(uint8_t, uint8)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(double, float64)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(const const_byte_span&, bytes)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(bool, boolean)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(optional<size_t>, array)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(void, end_array)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(optional<size_t>, object)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(const const_byte_span&, key)
    DEF_BASIC_VISITOR_FALLBACK_METHOD(void, end_object)

    // Static Probe Interface
    ProbeVisitor::Event ProbeVisitor::probe(Deserializer& deserializer)
    {
        ProbeVisitor pv;
        deserializer.deserialize_any(pv);
        return pv.take_event();
    }

    bool ProbeVisitor::assert_array_begin(Deserializer& deserializer, optional<size_t> size_hint)
    {
        ProbeVisitor pv;
        deserializer.deserialize_array(size_hint, pv);
        ProbeVisitor::EventType vis_event = pv.take_event().get_type();
        switch (vis_event)
        {
        case ProbeVisitor::EventType::StartArray:
            return true;
        case ProbeVisitor::EventType::EndArray:
        case ProbeVisitor::EventType::EndObject:
            return false;
        default:
            ASSERT_MES_AND_THROW("Got data from deserializer that was not the start of an array");
        }
    }

    bool ProbeVisitor::assert_object_begin(Deserializer& deserializer, optional<size_t> size_hint)
    {
        ProbeVisitor pv;
        deserializer.deserialize_object(size_hint, pv);
        ProbeVisitor::EventType vis_event = pv.take_event().get_type();
        switch (vis_event)
        {
        case ProbeVisitor::EventType::StartObject:
            return true;
        case ProbeVisitor::EventType::EndArray:
        case ProbeVisitor::EventType::EndObject:
            return false;
        default:
            ASSERT_MES_AND_THROW("Got data from deserializer that was not the start of an object");
        }
    }

    std::string ProbeVisitor::expecting() const
    {
        return "anything :)";
    }

    #define DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(mname, tyname, ename) \
        void ProbeVisitor::visit_##mname(tyname value)                  \
        {                                                               \
            m_event.m_type = ProbeVisitor::EventType::ename;            \
            m_event.m_##mname = value;                                  \
        }                                                               \

    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(int64, int64_t, Int64)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(int32, int32_t, Int32)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(int16, int16_t, Int16)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(int8, int8_t, Int8)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(uint64, uint64_t, UInt64)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(uint32, uint32_t, UInt32)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(uint16, uint16_t, UInt16)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(uint8, uint8_t, UInt8)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(float64, double, Float64)
    DEFINE_PROBE_VISITOR_VISIT_POD_METHOD(boolean, bool, Boolean)

    void ProbeVisitor::visit_bytes(const const_byte_span& bytes)
    {
        m_event.m_type = ProbeVisitor::EventType::String;
        m_event.m_string = internal::byte_span_to_string(bytes);
    }

    void ProbeVisitor::visit_array(optional<size_t> size_hint)
    {
        m_event.m_type = ProbeVisitor::EventType::StartArray;
        m_event.m_size_hint = size_hint ? *size_hint : ProbeVisitor::Event::NO_SIZE;
    }

    void ProbeVisitor::visit_end_array()
    {
        m_event.m_type = ProbeVisitor::EventType::EndArray;
    }

    void ProbeVisitor::visit_object(optional<size_t> size_hint)
    {
        m_event.m_type = ProbeVisitor::EventType::StartObject;
        m_event.m_size_hint = size_hint ? *size_hint : ProbeVisitor::Event::NO_SIZE;
    }

    void ProbeVisitor::visit_end_object()
    {
        m_event.m_type = ProbeVisitor::EventType::EndObject;
    }

    void ProbeVisitor::visit_key(const const_byte_span& bytes)
    {
        m_event.m_type = ProbeVisitor::EventType::Key;
        m_event.m_string = internal::byte_span_to_string(bytes);
    }
} // namespace serde::model
