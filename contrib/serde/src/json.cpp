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

#include <rapidjson/reader.h>

#include "serde/json/deserializer.h"
#include "serde/json/value.h"
#include "serde/model/visitor.h"

namespace serde::json
{
    class JsonVisitorHandler: public rapidjson::BaseReaderHandler<rapidjson::UTF8<>>
    {
        typedef rapidjson::SizeType SizeType;
        static_assert(std::is_same<int, int32_t>::value); // @TODO: provide contigencies
        static_assert(std::is_same<unsigned int, uint32_t>::value); // @TODO: provide contigencies

    public:

        constexpr JsonVisitorHandler(model::BasicVisitor& vis): m_visitor(vis) {}

        bool Null() { ASSERT_MES_AND_THROW("null is not supported in the data model"); }
        bool Bool(bool b) { m_visitor.visit_boolean(b); return true; }
        bool Int(int i) { m_visitor.visit_int32(i); return true; }
        bool Uint(unsigned u) { m_visitor.visit_uint32(u); return true; }
        bool Int64(int64_t i) { m_visitor.visit_int64(i); return true; }
        bool Uint64(uint64_t u) { m_visitor.visit_uint64(u); return true; }
        bool Double(double d) { m_visitor.visit_float64(d); return true; }
        bool String(const char* str, SizeType length, bool copy) {
            const const_byte_span str_span(reinterpret_cast<const_byte_iterator>(str), length);
            m_visitor.visit_bytes(str_span);
            return true;
        }
        bool StartObject() { m_visitor.visit_object({}); return true; }
        bool Key(const char* str, SizeType length, bool copy) { 
            const const_byte_span str_span(reinterpret_cast<const_byte_iterator>(str), length);
            m_visitor.visit_key(str_span);
            return true;
        }
        bool EndObject(SizeType memberCount) { m_visitor.visit_end_object(); return true; }
        bool StartArray() { m_visitor.visit_array({}); return true; }
        bool EndArray(SizeType elementCount) { m_visitor.visit_end_array(); return true; }

    private:

        model::BasicVisitor& m_visitor;
    };

    Deserializer::Deserializer(const char* src):
        model::SelfDescribingDeserializer(),
        m_json_reader(),
        m_istream(const_cast<char*>(src)) // Not necessarily a safe cast
    {
        m_json_reader.IterativeParseInit();
    }

    void Deserializer::deserialize_any(model::BasicVisitor& visitor)
    {
        if (!m_json_reader.IterativeParseComplete())
        {
            JsonVisitorHandler handler = visitor;
            m_json_reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(m_istream, handler);
        }
        else
        {
            visitor.visit_end_object();
        }
    }

    Document parse_borrowed_document_from_cstr(char* str)
    {
        Document doc;
        CHECK_AND_ASSERT_THROW_MES
        (
            !doc.ParseInsitu(str).HasParseError(),
            "JSON parse error at offset " << doc.GetErrorOffset() << ": " <<
                rapidjson::GetParseError_En(doc.GetParseError())
        );
        return doc;
    }
}
