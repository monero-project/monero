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

#include <list>
#include <sstream>
#include <string>
#include <utility>

#include "constants.h"
#include "../internal/deps.h"
#include "../internal/endianness.h"
#include "../model/serializer.h"

#define VARINT_VAL_FITS_BYTE(val) (val < 63)
#define VARINT_VAL_FITS_WORD(val) (val < 16383)
#define VARINT_VAL_FITS_DWORD(val) (val < 1073741823)
// Below is same as checking val <= 4611686018427387903 but portable for 32-bit size_t
#define VARINT_VAL_FITS_QWORD(val) (!(val >> 31 >> 31))

namespace serde::epee_binary
{
    template<class t_ostream>
    class Serializer: public serde::model::Serializer
    {
    public:
        Serializer(t_ostream& stream);
        virtual ~Serializer() override final {}

    private:
        struct recursion_level
        {
            bool is_object;   // true if object, false if array
            size_t length;    // total number of elements / entries
            size_t remaining; // number of elements / entries which have yet to be serialized
        };

        void write_type_code(uint8_t code);
        void write_varint(size_t value);

        // m_stack convenience methods
        bool inside_array() const;
        bool inside_object() const;
        size_t remaining() const;
        bool first() const;
        bool root() const;
        void push_array(size_t num_elements);
        void push_object(size_t num_entries);
        void pop(bool should_be_object);
        void did_serialize();

        t_ostream& m_stream;
        std::list<recursion_level> m_stack;

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
        m_stack()
    {}

    #define DEF_SERIALIZE_LE_INT(inttype, typecode)                        \
        template<class t_ostream>                                          \
        void Serializer<t_ostream>::serialize_##inttype(inttype##_t value) \
        {                                                                  \
            this->write_type_code(typecode);                               \
            value = CONVERT_POD(value);                                    \
            m_stream.write(TO_CSTR(&value), sizeof(value));                \
            this->did_serialize();                                         \
        }                                                                  \

    DEF_SERIALIZE_LE_INT( int64,  SERIALIZE_TYPE_INT64);
    DEF_SERIALIZE_LE_INT( int32,  SERIALIZE_TYPE_INT32);
    DEF_SERIALIZE_LE_INT( int16,  SERIALIZE_TYPE_INT16);
    DEF_SERIALIZE_LE_INT(  int8,   SERIALIZE_TYPE_INT8);
    DEF_SERIALIZE_LE_INT(uint64, SERIALIZE_TYPE_UINT64);
    DEF_SERIALIZE_LE_INT(uint32, SERIALIZE_TYPE_UINT32);
    DEF_SERIALIZE_LE_INT(uint16, SERIALIZE_TYPE_UINT16);
    DEF_SERIALIZE_LE_INT( uint8,  SERIALIZE_TYPE_UINT8);

    template<class t_ostream>
    void Serializer<t_ostream>::serialize_float64(double value)
    {
        this->write_type_code(SERIALIZE_TYPE_DOUBLE);
        value = CONVERT_POD(value);
        m_stream.write(TO_CSTR(&value), sizeof(value));
        this->did_serialize();
    }

    template<class t_ostream>
    void Serializer<t_ostream>::serialize_bytes(const const_byte_span& bytes)
    {
        this->write_type_code(SERIALIZE_TYPE_STRING);
        this->write_varint(bytes.size());
        m_stream.write(SPAN_TO_CSTR(bytes), bytes.size());
        this->did_serialize();
    }

    template<class t_ostream>
    void Serializer<t_ostream>::serialize_boolean(bool value)
    {
        this->write_type_code(SERIALIZE_TYPE_BOOL);
        m_stream.put(value ? 1 : 0);
        this->did_serialize();
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_start_array(size_t num_entries)
    {
        this->push_array(num_entries);
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_end_array()
    {
        this->pop(false);
        this->did_serialize();
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_start_object(size_t num_entries)
    {
        if (this->root())
        {
            m_stream.write
            (
                TO_CSTR(PORTABLE_STORAGE_SIG_AND_VER),
                sizeof(PORTABLE_STORAGE_SIG_AND_VER)
            );
        }
        else
        {
            this->write_type_code(SERIALIZE_TYPE_OBJECT);
        }

        this->write_varint(num_entries);
        this->push_object(num_entries);
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_key(const const_byte_span& key_bytes)
    {
        CHECK_AND_ASSERT_THROW_MES
        (
            this->inside_object(),
            "invalid serializer usage: called key() inside array"
        );

        const size_t key_size = key_bytes.size();

        CHECK_AND_ASSERT_THROW_MES
        (
            key_size <= MAX_KEY_LENGTH,
            "key with length " << key_size << " exceeds maximum key size of " << MAX_KEY_LENGTH
        );

        m_stream.put(key_size);
        m_stream.write(SPAN_TO_CSTR(key_bytes), key_size);
    }

    template <class t_ostream>
    void Serializer<t_ostream>::serialize_end_object()
    {
        this->did_serialize();
        this->pop(true);
    }

    template <class t_ostream>
    void Serializer<t_ostream>::write_type_code(uint8_t code)
    {
        if (this->inside_object())
        {
            m_stream.put(code);
        }
        else if (this->first()) // if first element of array
        { 
            m_stream.put(code | SERIALIZE_FLAG_ARRAY);
            this->write_varint(this->remaining());
        } // else inside array
    }

    template <class t_ostream>
    void Serializer<t_ostream>::write_varint(size_t value)
    {
        uint64_t varint_size_code;
        if (VARINT_VAL_FITS_BYTE(value))
        {
            varint_size_code = 0;
        }
        else if (VARINT_VAL_FITS_WORD(value))
        {
            varint_size_code = 1;
        }
        else if (VARINT_VAL_FITS_DWORD(value))
        {
            varint_size_code = 2;
        }
        else if (VARINT_VAL_FITS_QWORD(value))
        {
            varint_size_code = 3;
        }
        else
        {
            ASSERT_MES_AND_THROW("size_t value is too large to be packed into varint: " << value);
        }

        const size_t varint_size = 1 << static_cast<size_t>(varint_size_code);
        uint64_t varint_data = static_cast<uint64_t>(value);
        varint_data <<= 2;
        varint_data |= varint_size_code;
        varint_data = CONVERT_POD(varint_data);
        m_stream.write(TO_CSTR(&varint_data), varint_size);
    }

    template <class t_ostream>
    bool Serializer<t_ostream>::inside_array() const
    {
        return m_stack.size() != 0 && !m_stack.back().is_object;
    }

    template <class t_ostream>
    bool Serializer<t_ostream>::inside_object() const
    {
        return m_stack.size() == 0 || m_stack.back().is_object;
    }

    template <class t_ostream>
    size_t Serializer<t_ostream>::remaining() const
    {
        CHECK_AND_ASSERT_THROW_MES
        (
            m_stack.size(),
            "invalid state: called remaining() when m_stack is empty"
        );

        return m_stack.back().remaining;
    }

    template <class t_ostream>
    bool Serializer<t_ostream>::first() const
    {
        CHECK_AND_ASSERT_THROW_MES
        (
            m_stack.size(),
            "invalid state: called first() when m_stack is empty"
        );

        const auto& back = m_stack.back();
        return back.remaining == back.length;
    }

    template <class t_ostream>
    bool Serializer<t_ostream>::root() const
    {
        return m_stack.size() == 0;
    }

    template <class t_ostream>
    void Serializer<t_ostream>::push_array(size_t num_elements) 
    {
        if (this->inside_array())
        {
            ASSERT_MES_AND_THROW("invalid serializer usage: directly nested arrays not allowed");
        }
        else if (m_stack.size() == 0)
        {
            ASSERT_MES_AND_THROW("invalid serializer usage: arrays must be serialized in objects");
        }

        m_stack.push_back({ false, num_elements, num_elements });
    }

    template <class t_ostream>
    void Serializer<t_ostream>::push_object(size_t num_entries)
    {
        m_stack.push_back({ true, num_entries, num_entries });
    }

    template <class t_ostream>
    void Serializer<t_ostream>::pop(bool should_be_object)
    {
        CHECK_AND_ASSERT_THROW_MES
        (
            m_stack.size(),
            "invalid serializer usage: called end_...() more than start_...()"
        );

        if (this->inside_object() ^ should_be_object)
        {
            if (should_be_object)
            {
                ASSERT_MES_AND_THROW("invalid serializer usage: called end_object() inside array");
            }
            else
            {
                ASSERT_MES_AND_THROW("invalid serializer usage: called end_array() inside object");
            }
        }

        m_stack.pop_back();
    }

    template <class t_ostream>
    void Serializer<t_ostream>::did_serialize()
    {
        CHECK_AND_ASSERT_THROW_MES
        (
            m_stack.size(),
            "invalid serializer usage: trying to serialize outside object/array"
        );

        m_stack.back().remaining--;
    }
} // namespace serde::binary_epee
