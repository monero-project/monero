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

#include <fstream>
#include <string>

#include "../model/serializer.h"

namespace serde::json {
    template<class t_ostream>
    class Serializer: public serde::model::Serializer {
    public:
        Serializer(t_ostream& stream);
        virtual ~Serializer() override final {};

    private:
        void write_string(const const_byte_span& bytes, bool escape = true);
        void write_escaped_string(const const_byte_span& bytes);

        // Gets called before every primitive serialize, start_object(), key()
        // Controls the serialization of entry / element delimination
        inline void comma();

        t_ostream& m_stream;

        // True after start_array() or start_object(), false after an element / entry is serialized
        bool m_first;

    // Serializer interface
    public:
        void serialize_int64(int64_t) override final;
        void serialize_int32(int32_t) override final;
        void serialize_int16(int16_t) override final;
        void serialize_int8(int8_t) override final;
        void serialize_uint64(uint64_t) override final;
        void serialize_uint32(uint32_t) override final;
        void serialize_uint16(uint16_t) override final;
        void serialize_uint8(uint8_t) override final;
        void serialize_float64(double) override final;
        void serialize_bytes(const const_byte_span&) override final;
        void serialize_boolean(bool) override final;

        void serialize_start_array(size_t) override final;
        void serialize_end_array() override final;

        void serialize_start_object(size_t) override final;
        void serialize_key(const const_byte_span&) override final;
        void serialize_end_object() override final;
    };

    template<class t_ostream>
    Serializer<t_ostream>::Serializer(t_ostream& stream):
        m_stream(stream),
        m_first(true)
    {}

    #define DEF_SERIALIZE_WITH_TO_STRING(tyname, mname)               \
        template<class t_ostream>                                     \
        void Serializer<t_ostream>::serialize_##mname(tyname value) { \
            this->comma();                                            \
            const std::string str_rep = std::to_string(value);        \
            m_stream.write(str_rep.c_str(), str_rep.length());        \
        }                                                             \

    DEF_SERIALIZE_WITH_TO_STRING( int64_t,   int64);
    DEF_SERIALIZE_WITH_TO_STRING( int32_t,   int32);
    DEF_SERIALIZE_WITH_TO_STRING( int16_t,   int16);
    DEF_SERIALIZE_WITH_TO_STRING(  int8_t,    int8);
    DEF_SERIALIZE_WITH_TO_STRING(uint64_t,  uint64);
    DEF_SERIALIZE_WITH_TO_STRING(uint32_t,  uint32);
    DEF_SERIALIZE_WITH_TO_STRING(uint16_t,  uint16);
    DEF_SERIALIZE_WITH_TO_STRING( uint8_t,   uint8);
    DEF_SERIALIZE_WITH_TO_STRING(  double, float64);

    template<class t_ostream>
    void Serializer<t_ostream>::serialize_bytes(const const_byte_span& bytes) {
        this->comma();
        this->write_string(bytes);
    }

    template<class t_ostream>
    void Serializer<t_ostream>::serialize_boolean(bool value) {
        this->comma();
        if (value)
        {
            m_stream.write("true", 4);
        }
        else {
            m_stream.write("false", 5);
        }
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_start_array(size_t num_entries) {
        this->comma(); // this should never run b/c nested arrays aren't allowed in model
        m_stream.put('[');
        m_first = true;
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_end_array() {
        m_stream.put(']');
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_start_object(size_t num_entries) {
        this->comma();
        m_stream.put('{');
        m_first = true;
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_key(const const_byte_span& key_bytes) {
        this->comma();
        this->write_string(key_bytes, false); // do not escape key
        m_stream.put(':');
        m_first = true; // needed so commas are not inserted after keys
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_end_object() {
        m_stream.put('}');
    }

    template<class t_ostream>
    void Serializer<t_ostream>::write_string(const const_byte_span& bytes, bool escape) {
        m_stream.put('"');
        if (escape) {
            this->write_escaped_string(bytes);
        } else {
            m_stream.write(SPAN_TO_CSTR(bytes), bytes.size());
        }
        m_stream.put('"');
    }

    // @TODO: compare performance against transform_to_escape_sequence
    // @TODO: remove in favor of rapidjson internal function
    template<class t_ostream>
    void Serializer<t_ostream>::write_escaped_string(const const_byte_span& bytes) {
        const_byte_iterator head;
        const_byte_iterator tail;
        const const_byte_iterator end = bytes.end();

        // The tail pointer steps through the string and searches for characters which require
        // escaping. m_stream is only written to when it must write escape characters or when
        // tail reaches the end of the string.

        for (head = tail = bytes.begin(); tail < end; tail++) { // while searching characters
            if (*tail < 0x20 || *tail == '\\' || *tail == '"') { // ctrl chars are ASCII bytes < 32
                if (head != tail) { // if not flushed
                    // flush characters in [head, tail)
                    m_stream.write(TO_CSTR(head), tail - head);
                    head = tail;
                }

                #define JSON_SER_ESCAPE_REPL(srcchar, replstr)                         \
                    case srcchar: m_stream.write(replstr, sizeof(replstr) - 1); break; \

                switch (*tail) {
                    JSON_SER_ESCAPE_REPL('\\', "\\\\")
                    JSON_SER_ESCAPE_REPL('"', "\\\"")
                    JSON_SER_ESCAPE_REPL('\b', "\\b")
                    JSON_SER_ESCAPE_REPL('\f', "\\f")
                    JSON_SER_ESCAPE_REPL('\n', "\\n")
                    JSON_SER_ESCAPE_REPL('\r', "\\r")
                    JSON_SER_ESCAPE_REPL('\t', "\\t")
                default:
                    // Encode control character in the form "\u00XX" where X are hex digits
                    static constexpr const char hex[17] = "0123456789ABCDEF";
                    const char hex_msb = hex[*tail >> 4];
                    const char hex_lsb = hex[*tail & 0xF];
                    const char unicode_sub_str[6] = { '\\', 'u', '0', '0', hex_msb, hex_lsb };
                    m_stream.write(unicode_sub_str, sizeof(unicode_sub_str));
                    break;
                } // end switch

                // If we execute the next statement, we have escaped a character previously.
                // tail will be incremented when control flow hits bottom of for loop.
                head++;
            } // end if
        } // end for

        if (head != tail) { // if not flushed
            // flush characters in [head, tail)
            m_stream.write(TO_CSTR(head), tail - head);
            head = tail;
        }
    }

    template <class t_ostream>
    inline void Serializer<t_ostream>::comma() {
        if (m_first) {
            m_first = false;
        } else {
            m_stream.put(',');
        }
    }
} // namespace serde::json
