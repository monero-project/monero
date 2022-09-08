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

#include "../model/visitor.h"

namespace serde::json::internal
{
    struct ValueObjectPair
    {
        ValueObjectPair(const Value& val):
            begin(val.MemberBegin()), end(val.MemberEnd()), iter_key(true)
        {}

        Value::ConstMemberIterator begin;
        Value::ConstMemberIterator end;
        bool iter_key;
    };

    struct ValueArrayPair
    {
        ValueArrayPair(const Value& val): begin(val.Begin()), end(val.End()) {}

        Value::ConstValueIterator begin;
        Value::ConstValueIterator end;
    };

    class ValueIteratorVariant
    {
    public:

        ValueIteratorVariant(const Value& value)
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                is_iterable(value),
                "JSON value with type " << value.GetType() << " is not iterable"
            );

            if (value.IsObject())
            {
                m_is_object = true;
                m_object_pair = ValueObjectPair(value);
            }
            else // IsArray
            {
                m_is_object = false;
                m_array_pair = ValueArrayPair(value);
            }
        }

        const_byte_span deref_key()
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                this->is_key_next(),
                "expecting to dereference value, not key"
            );

            const Value& name = m_object_pair.begin->name;
            // name.GetString() and name.GetStringLength() should always work since
            // rapidjson requires them to be string-like GenericValues
            const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(name.GetString());
            const const_byte_span key{byte_ptr, name.GetStringLength()};
            return key;
        }

        const Value* deref_value()
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                !this->is_key_next(), // value is next
                "expecting to dereference key, not value"
            );

            return m_is_object ? &m_object_pair.begin->value : &(*m_array_pair.begin);
        }

        void inc()
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                !this->done(),
                "Can't iterate pair past end"
            );

            if (m_is_object)
            {
                if (m_object_pair.iter_key)
                {
                    m_object_pair.iter_key = false;
                }
                else
                {
                    m_object_pair.begin++;
                    m_object_pair.iter_key = true;
                }
            }
            else
            {
                m_array_pair.begin++;
            }
        }

        bool done() const
        {
            if (m_is_object) return m_object_pair.begin == m_object_pair.end;
            else return m_array_pair.begin == m_array_pair.end;
        }

        bool is_key_next() const { return m_is_object && m_object_pair.iter_key; }

        bool is_object() const { return m_is_object; }

        static bool is_iterable(const Value& value)
        {
            return value.IsObject() || value.IsArray();
        }

    private:

        union
        {
            ValueArrayPair m_array_pair;
            ValueObjectPair m_object_pair;
        };
        bool m_is_object;
    }; // class ValueIteratorVariant
} // namespace serde::json::internal

namespace serde::json
{
    class ValueDeserializer: public model::SelfDescribingDeserializer
    {
        typedef internal::ValueIteratorVariant ValueIteratorVariant;

    public:

        ValueDeserializer(const Value& root_value): m_iter_stack(), m_root(true), m_finished(false)
        {
            m_iter_stack.push_back(root_value);
        }

        void deserialize_any(model::BasicVisitor& visitor) override final
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                !m_finished,
                "Value has already been completely deserialized"
            );
            CHECK_AND_ASSERT_THROW_MES
            (
                !m_iter_stack.empty(),
                "ValueDeserializer stack is exhausted but not finished. This should not happen!!!"
            );

            if (m_root) // deserialize start of first object
            {
                m_root = false;
                if (this->top().is_object()) { visitor.visit_object({}); }
                else { visitor.visit_array({}); }
            }
            else if (this->top().done()) // deserialize end object/array
            {
                const bool top_was_object = this->top().is_object();
                m_iter_stack.pop_back();
                if (m_iter_stack.empty()) { m_finished = true; }
                if (top_was_object) { visitor.visit_end_object(); }
                else { visitor.visit_end_array(); }
            }
            else if (this->top().is_key_next()) // deserialize key
            {
                const const_byte_span key = this->top().deref_key();
                this->top().inc();
                visitor.visit_key(key);
            }
            else // deserialize value
            {
                const Value* val_ptr = this->top().deref_value();
                this->top().inc();
                deserialize_json_value(val_ptr, visitor);
            }
        }

    private:

        void deserialize_json_value(const Value* val_ptr, model::BasicVisitor& visitor)
        {
            switch (val_ptr->GetType())
            {
            case rapidjson::kNullType: ASSERT_MES_AND_THROW("null is not valid in serde model");
            case rapidjson::kFalseType: return visitor.visit_boolean(false);
            case rapidjson::kTrueType: return visitor.visit_boolean(true);
            case rapidjson::kObjectType:
                m_iter_stack.push_back(*val_ptr);
                return visitor.visit_object(val_ptr->MemberCount());
            case rapidjson::kArrayType:
                m_iter_stack.push_back(*val_ptr);
                return visitor.visit_array(val_ptr->Size());        
            case rapidjson::kStringType:
                {
                    const auto str_start = reinterpret_cast<const uint8_t*>(val_ptr->GetString());
                    const const_byte_span strspan{str_start, val_ptr->GetStringLength()};
                    return visitor.visit_bytes(strspan);
                }
            case rapidjson::kNumberType:
                if      (val_ptr->IsDouble()) return visitor.visit_float64(val_ptr->GetDouble());
                else if (val_ptr->IsInt())    return visitor.visit_int32  (val_ptr->GetInt());
                else if (val_ptr->IsUint())   return visitor.visit_uint32 (val_ptr->GetUint());
                else if (val_ptr->IsInt64())  return visitor.visit_int64  (val_ptr->GetInt64());
                else                          return visitor.visit_uint64 (val_ptr->GetUint64());
            default:
                ASSERT_MES_AND_THROW("unrecognized value type: " << val_ptr->GetType());
            }
        }

        // Assumes m_iter_stack is not empty
        ValueIteratorVariant& top() { return m_iter_stack.back(); }

        std::vector<ValueIteratorVariant> m_iter_stack;
        bool m_root;
        bool m_finished;
    }; // class ValueDeserializer
} // namespace serde::json
