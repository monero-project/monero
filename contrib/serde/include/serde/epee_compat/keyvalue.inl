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

#include <cstring>
#include <tuple>

#include "../blob/deserialize_as_blob.h"
#include "../blob/serialize_as_blob.h"
#include "../internal/container.h"
#include "../internal/deps.h"
#include "../internal/visitor_specializations.h"
#include "../model/deserialize_default.h"
#include "../model/serialize_default.h"

namespace serde::internal
{
    ///////////////////////////////////////////////////////////////////////////
    // Serialization and Deserialization Sides - Shared                      //
    ///////////////////////////////////////////////////////////////////////////

    template <typename ValRef, bool AsBlob>
    struct StructField
    {
        static constexpr bool do_as_blob = AsBlob;

        StructField(const const_byte_span& key, ValRef value): key(key), value(value) {}

        // StructDeserializeFields are passed an optional value while deserializing.
        // It is not needed for serialization, so just ignore that parameter
        StructField(const const_byte_span& key, ValRef value, ValRef ignored):
            StructField(key, value)
        {}

        const_byte_span key;
        ValRef value;

        bool matches_key(const const_byte_span& other_key) const
        {
            if (other_key.size() != this->key.size()) return false;
            return 0 == memcmp(key.begin(), other_key.begin(), key.size());
        }
    };
    
    template <typename V, bool AsBlob, bool Required>
    struct StructDeserializeField: public StructField<V&, AsBlob>
    {
        static constexpr bool required = Required;

        StructDeserializeField
        (
            const const_byte_span& key,
            V& value,
            optional<V> optional_value
        ):
            StructField<V&, AsBlob>(key, value), did_deser(false)
        {
            if (optional_value)
            {
                value = *optional_value;
            }
        }

        bool did_deser;
    }; // struct StructDeserializeField

    // Two specializations for serializing StructFields with AsBlob = true/false
    template <typename V>
    void serialize_field_value
    (
        const StructField<const V&, true>& blob_field,
        model::Serializer& serializer
    )
    {
        serialize_as_blob(blob_field.value, serializer);
    }

    template <typename V>
    void serialize_field_value
    (
        const StructField<const V&, false>& default_field,
        model::Serializer& serializer
    )
    {
        serialize_default(default_field.value, serializer);
    }

    // Two specializations for deserializing StructDeserializeFields with AsBlob = true/false
    template <typename V, bool Required>
    bool deserialize_field_value
    (
        model::Deserializer& deser,
        StructDeserializeField<V, true, Required>& blob_field
    )
    {
        return blob_field.did_deser = deserialize_as_blob(deser, blob_field.value);
    }

    template <typename V, bool Required>
    bool deserialize_field_value
    (
        model::Deserializer& deser,
        StructDeserializeField<V, false, Required>& default_field
    )
    {
        return default_field.did_deser = deserialize_default(deser, default_field.value);
    }

    template <bool SerializeSelector, typename V, bool AsBlob, bool Required> 
    using FieldSelector = typename std::conditional<SerializeSelector, 
        StructField<const V&, AsBlob>, 
        StructDeserializeField<V, AsBlob, Required>>::type;

    ///////////////////////////////////////////////////////////////////////////
    // Serialization Side                                                    //
    ///////////////////////////////////////////////////////////////////////////

    struct serialize_field
    {
        constexpr serialize_field(model::Serializer& serializer): serializer(serializer) {}

        template <class Field>
        bool operator()(const Field& field)
        {
            serializer.serialize_key(field.key);
            serialize_field_value(field, serializer);
            return true;
        }

        model::Serializer& serializer;
    }; // struct serialize_field

    // If true, serialize. If false, deserialize
    // @TODO: struct_serde doesn't need to be a bool overloaded struct
    template <bool SerializeSelector>
    struct struct_serde
    {
        template <typename... SF>
        static void call(const std::tuple<SF...>& fields, model::Serializer& serializer)
        {
            serialize_field ser(serializer);

            serializer.serialize_start_object(sizeof...(SF));
            tuple_for_each(fields, ser);
            serializer.serialize_end_object();
        }
    }; // struct struct_serde

    ///////////////////////////////////////////////////////////////////////////
    // Deserialization Side                                                  //
    ///////////////////////////////////////////////////////////////////////////

    static constexpr size_t NO_MATCH = std::numeric_limits<size_t>::max();

    class key_search
    {
    public:
        key_search(const const_byte_span& target_key):
            m_target_key(target_key), m_match_index(NO_MATCH), m_i(0)
        {}

        template <typename Field>
        bool operator()(Field& field)
        {
            if (field.matches_key(m_target_key))
            {
                m_match_index = m_i;
                return false;
            }
            else
            {
                m_i++;
                return true;
            } 
        }

        bool matched() const { return m_match_index != NO_MATCH; }
        size_t match_index() const { return m_match_index; }

    private:
        const const_byte_span& m_target_key;
        size_t m_match_index;
        size_t m_i;
    }; // class key_seach

    template <typename... SF>
    struct StructKeysVisitor: public model::BasicVisitor
    {
        constexpr StructKeysVisitor(std::tuple<SF...>& fields):
            fields(fields), object_ended(false), match_index(NO_MATCH)
        {}

        std::string expecting() const override final
        {
            return "keys";
        }

        void visit_key(const const_byte_span& key_bytes) override final
        {
            key_search key_search(key_bytes);
            tuple_for_each(fields, key_search);

            CHECK_AND_ASSERT_THROW_MES
            (
                key_search.matched(),
                "Key '" << byte_span_to_string(key_bytes) << "' was not found in struct"
            );

            match_index = key_search.match_index();
        }

        void visit_end_object() override final
        {
            object_ended = true;
        }

        std::tuple<SF...>& fields;
        bool object_ended;
        size_t match_index;
    }; // struct StructKeysVisitor

    // If true, serialize. If false, deserialize
    // @TODO: struct_serde doesn't need to be a bool overloaded struct
    template <>
    struct struct_serde<false>
    {
        class deserialize_nth_field
        {
        public:

            constexpr deserialize_nth_field(size_t n, model::Deserializer& deserializer):
                m_n(n), m_i(0), m_deser(deserializer)
            {} 

            template <typename Field>
            bool operator()(Field& field)
            {
                if (m_i == m_n)
                {
                    CHECK_AND_ASSERT_THROW_MES
                    (
                        !field.did_deser, // fields should be deserialized at most once
                        "key seen twice for same object"
                    );
                    CHECK_AND_ASSERT_THROW_MES
                    (
                        deserialize_field_value(m_deser, field), // also sets did_deser
                        "object ended after key"
                    );
                    return false;
                }
                else
                {
                    m_i++;
                    return true;
                }
            }

        private:

            size_t m_n;
            size_t m_i;
            model::Deserializer& m_deser;
        }; // class deserialize_nth_field

        template <typename... SF>
        static bool call(std::tuple<SF...>& fields, model::Deserializer& deserializer, bool partial)
        {
            if (!partial && !model::ProbeVisitor::assert_object_begin(deserializer)) return false;

            while (true) {
                StructKeysVisitor<SF...> keys_visitor(fields);
                deserializer.deserialize_key(keys_visitor);

                if (keys_visitor.object_ended) break;

                deserialize_nth_field dnf(keys_visitor.match_index, deserializer);
                tuple_for_each(fields, dnf);
            }

            return true;

            // @TODO: required check up etc 
        }
    }; // struct struct_serde
}

namespace serde::model
{
    // Overload the serialize_default operator if type has the serde_struct_enabled typedef
    template <class Struct, typename = typename Struct::serde_struct_enabled>
    void serialize_default(const Struct& struct_ref, Serializer& serializer)
    {
        using serde_struct_map = typename Struct::make_serde_fields<true>;
        const auto fields = serde_struct_map()(struct_ref);
        internal::struct_serde<true>::call(fields, serializer);
    }

    // Overload the deserialize_default operator if type has the serde_struct_enabled typedef
    template <class Struct, typename = typename Struct::serde_struct_enabled>
    bool deserialize_default(Deserializer& deserializer, Struct& struct_ref, bool partial)
    {
        using serde_struct_map = typename Struct::make_serde_fields<false>;
        auto fields = serde_struct_map()(struct_ref);
        return internal::struct_serde<false>::call(fields, deserializer, partial);
    }
}
