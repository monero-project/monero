// Copyright (c) 2022-2025, The Monero Project
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

// Transcript class for assembling data that needs to be hashed.

#pragma once

//local headers
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "ringct/rctTypes.h"
#include "wipeable_string.h"

//third party headers
#include <boost/utility/string_ref.hpp>

//standard headers
#include <list>
#include <string>
#include <type_traits>
#include <vector>

//forward declarations


namespace sp
{

////
// SpTranscriptBuilder
// - build a transcript
// - the user must provide a label when trying to append something to the transcript; labels are prepended to objects in
//   the transcript
// - data types
//     - unsigned int: uint_flag || varint(uint_variable)
//     - signed int: int_flag || uchar{int_variable < 0 ? 1 : 0} || varint(abs(int_variable))
//     - byte buffer (assumed little-endian): buffer_flag || buffer_length || buffer
//       - all labels are treated as byte buffers
//     - named container: container_flag || container_name || data_member1 || ... || container_terminator_flag
//     - list-type container (same-type elements only): list_flag || list_length || element1 || element2 || ...
// - the transcript can be used like a string via the .data() and .size() member functions
// - simple mode: exclude all labels, flags, and lengths
///
class SpTranscriptBuilder final
{
public:
//public member types
    /// transcript builder mode
    enum class Mode
    {
        FULL,
        SIMPLE
    };

//constructors
    /// normal constructor
    SpTranscriptBuilder(const std::size_t estimated_data_size, const Mode mode) :
        m_mode{mode}
    {
        m_transcript.reserve(2 * estimated_data_size + 20);
    }

//overloaded operators
    /// disable copy/move (this is a scoped manager [of the 'transcript' concept])
    SpTranscriptBuilder& operator=(SpTranscriptBuilder&&) = delete;

//member functions
    /// append a value to the transcript
    template <typename T>
    void append(const boost::string_ref label, const T &value)
    {
        this->append_impl(label, value);
    }

    /// access the transcript data
    const void* data() const { return m_transcript.data(); }
    std::size_t size() const { return m_transcript.size(); }

private:
//member variables
    /// in simple mode, exclude labels, flags, and lengths
    Mode m_mode;
    /// the transcript buffer (wipeable in case it contains sensitive data)
    epee::wipeable_string m_transcript;

//private member types
    /// flags for separating items added to the transcript
    enum SpTranscriptBuilderFlag : unsigned char
    {
        UNSIGNED_INTEGER = 0,
        SIGNED_INTEGER = 1,
        BYTE_BUFFER = 2,
        NAMED_CONTAINER = 3,
        NAMED_CONTAINER_TERMINATOR = 4,
        LIST_TYPE_CONTAINER = 5
    };

//transcript builders
    void append_character(const unsigned char character)
    {
        m_transcript += static_cast<char>(character);
    }
    void append_uint(const std::uint64_t unsigned_integer)
    {
        unsigned char v_variable[(sizeof(std::uint64_t) * 8 + 6) / 7];
        unsigned char *v_variable_end = v_variable;

        // append uint to string as a varint
        tools::write_varint(v_variable_end, unsigned_integer);
        assert(v_variable_end <= v_variable + sizeof(v_variable));
        m_transcript.append(reinterpret_cast<const char*>(v_variable), v_variable_end - v_variable);
    }
    void append_flag(const SpTranscriptBuilderFlag flag)
    {
        if (m_mode == Mode::SIMPLE)
            return;

        static_assert(sizeof(SpTranscriptBuilderFlag) == sizeof(unsigned char), "");
        this->append_character(static_cast<unsigned char>(flag));
    }
    void append_length(const std::size_t length)
    {
        if (m_mode == Mode::SIMPLE)
            return;

        static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t), "");
        this->append_uint(static_cast<std::uint64_t>(length));
    }
    void append_buffer(const void *data, const std::size_t length)
    {
        this->append_flag(SpTranscriptBuilderFlag::BYTE_BUFFER);
        this->append_length(length);
        m_transcript.append(reinterpret_cast<const char*>(data), length);
    }
    void append_label(const boost::string_ref label)
    {
        if (m_mode == Mode::SIMPLE ||
            label.size() == 0)
            return;

        this->append_buffer(label.data(), label.size());
    }
    void append_labeled_buffer(const boost::string_ref label, const void *data, const std::size_t length)
    {
        this->append_label(label);
        this->append_buffer(data, length);
    }
    void begin_named_container(const boost::string_ref container_name)
    {
        this->append_flag(SpTranscriptBuilderFlag::NAMED_CONTAINER);
        this->append_label(container_name);
    }
    void end_named_container()
    {
        this->append_flag(SpTranscriptBuilderFlag::NAMED_CONTAINER_TERMINATOR);
    }
    void begin_list_type_container(const std::size_t list_length)
    {
        this->append_flag(SpTranscriptBuilderFlag::LIST_TYPE_CONTAINER);
        this->append_length(list_length);
    }

//append overloads
    void append_impl(const boost::string_ref label, const rct::key &key_buffer)
    {
        this->append_labeled_buffer(label, key_buffer.bytes, sizeof(key_buffer));
    }
    void append_impl(const boost::string_ref label, const rct::ctkey &ctkey)
    {
        this->append_label(label);
        this->append_impl("ctmask", ctkey.mask);
        this->append_impl("ctdest", ctkey.dest);
    }
    void append_impl(const boost::string_ref label, const crypto::secret_key &point_buffer)
    {
        this->append_labeled_buffer(label, point_buffer.data, sizeof(point_buffer));
    }
    void append_impl(const boost::string_ref label, const crypto::public_key &scalar_buffer)
    {
        this->append_labeled_buffer(label, scalar_buffer.data, sizeof(scalar_buffer));
    }
    void append_impl(const boost::string_ref label, const crypto::key_derivation &derivation_buffer)
    {
        this->append_labeled_buffer(label, derivation_buffer.data, sizeof(derivation_buffer));
    }
    void append_impl(const boost::string_ref label, const crypto::key_image &key_image_buffer)
    {
        this->append_labeled_buffer(label, key_image_buffer.data, sizeof(key_image_buffer));
    }
    void append_impl(const boost::string_ref label, const std::string &string_buffer)
    {
        this->append_labeled_buffer(label, string_buffer.data(), string_buffer.size());
    }
    void append_impl(const boost::string_ref label, const epee::wipeable_string &string_buffer)
    {
        this->append_labeled_buffer(label, string_buffer.data(), string_buffer.size());
    }
    void append_impl(const boost::string_ref label, const boost::string_ref string_buffer)
    {
        this->append_labeled_buffer(label, string_buffer.data(), string_buffer.size());
    }
    template<std::size_t Sz>
    void append_impl(const boost::string_ref label, const unsigned char(&uchar_buffer)[Sz])
    {
        this->append_labeled_buffer(label, uchar_buffer, Sz);
    }
    template<std::size_t Sz>
    void append_impl(const boost::string_ref label, const char(&char_buffer)[Sz])
    {
        this->append_labeled_buffer(label, char_buffer, Sz);
    }
    void append_impl(const boost::string_ref label, const std::vector<unsigned char> &vector_buffer)
    {
        this->append_labeled_buffer(label, vector_buffer.data(), vector_buffer.size());
    }
    void append_impl(const boost::string_ref label, const std::vector<char> &vector_buffer)
    {
        this->append_labeled_buffer(label, vector_buffer.data(), vector_buffer.size());
    }
    void append_impl(const boost::string_ref label, const char single_character)
    {
        this->append_label(label);
        this->append_character(static_cast<unsigned char>(single_character));
    }
    void append_impl(const boost::string_ref label, const unsigned char single_character)
    {
        this->append_label(label);
        this->append_character(single_character);
    }
    template<typename T,
        std::enable_if_t<std::is_unsigned<T>::value, bool> = true>
    void append_impl(const boost::string_ref label, const T unsigned_integer)
    {
        static_assert(sizeof(T) <= sizeof(std::uint64_t), "SpTranscriptBuilder: unsupported unsigned integer type.");
        this->append_label(label);
        this->append_flag(SpTranscriptBuilderFlag::UNSIGNED_INTEGER);
        this->append_uint(unsigned_integer);
    }
    template<typename T,
        std::enable_if_t<std::is_integral<T>::value, bool> = true,
        std::enable_if_t<!std::is_unsigned<T>::value, bool> = true>
    void append_impl(const boost::string_ref label, const T signed_integer)
    {
        using unsigned_type = std::make_unsigned<T>::type;
        static_assert(sizeof(unsigned_type) <= sizeof(std::uint64_t),
            "SpTranscriptBuilder: unsupported signed integer type.");
        this->append_label(label);
        this->append_flag(SpTranscriptBuilderFlag::SIGNED_INTEGER);
        this->append_uint(static_cast<std::uint64_t>(static_cast<unsigned_type>(signed_integer)));
    }
    template<typename T,
        std::enable_if_t<!std::is_integral<T>::value, bool> = true>
    void append_impl(const boost::string_ref label, const T &named_container)
    {
        // named containers must satisfy two concepts:
        //   const boost::string_ref container_name(const T &container);
        //   void append_to_transcript(const T &container, SpTranscriptBuilder &transcript_inout);
        //todo: container_name() and append_to_transcript() must be defined in the sp namespace, but that is not generic
        this->append_label(label);
        this->begin_named_container(container_name(named_container));
        append_to_transcript(named_container, *this);  //non-member function assumed to be implemented elsewhere
        this->end_named_container();
    }
    template<typename T>
    void append_impl(const boost::string_ref label, const std::vector<T> &list_container)
    {
        this->append_label(label);
        this->begin_list_type_container(list_container.size());
        for (const T &element : list_container)
            this->append_impl("", element);
    }
    template<typename T>
    void append_impl(const boost::string_ref label, const std::list<T> &list_container)
    {
        this->append_label(label);
        this->begin_list_type_container(list_container.size());
        for (const T &element : list_container)
            this->append_impl("", element);
    }
};

////
// SpFSTranscript
// - build a Fiat-Shamir transcript
// - main format: prefix || domain_separator || object1_label || object1 || object2_label || object2 || ...
// note: prefix defaults to "monero"
///
class SpFSTranscript final
{
public:
//constructors
    /// normal constructor: start building a transcript with the domain separator
    SpFSTranscript(const boost::string_ref domain_separator,
        const std::size_t estimated_data_size,
        const boost::string_ref prefix = config::TRANSCRIPT_PREFIX) :
        m_transcript_builder{15 + domain_separator.size() + estimated_data_size + prefix.size(),
            SpTranscriptBuilder::Mode::FULL}
    {
        // transcript = prefix || domain_separator
        m_transcript_builder.append("FSp", prefix);
        m_transcript_builder.append("ds", domain_separator);
    }

//overloaded operators
    /// disable copy/move (this is a scoped manager [of the 'transcript' concept])
    SpFSTranscript& operator=(SpFSTranscript&&) = delete;

//member functions
    /// build the transcript
    template<typename T>
    void append(const boost::string_ref label, const T &value)
    {
        m_transcript_builder.append(label, value);
    }

    /// access the transcript data
    const void* data() const { return m_transcript_builder.data(); }
    std::size_t size() const { return m_transcript_builder.size(); }

//member variables
private:
    /// underlying transcript builder
    SpTranscriptBuilder m_transcript_builder;
};

////
// SpKDFTranscript
// - build a data string for a key-derivation function
// - mainly intended for short transcripts (~128 bytes or less) with fixed-length and statically ordered components
// - main format: prefix || domain_separator || object1 || object2 || ...
// - simple transcript mode: no labels, flags, or lengths
// note: prefix defaults to "monero"
///
class SpKDFTranscript final
{
public:
//constructors
    /// normal constructor: start building a transcript with the domain separator
    SpKDFTranscript(const boost::string_ref domain_separator,
        const std::size_t estimated_data_size,
        const boost::string_ref prefix = config::TRANSCRIPT_PREFIX) :
        m_transcript_builder{10 + domain_separator.size() + estimated_data_size + prefix.size(),
            SpTranscriptBuilder::Mode::SIMPLE}
    {
        // transcript = prefix || domain_separator
        m_transcript_builder.append("", prefix);
        m_transcript_builder.append("", domain_separator);
    }

//overloaded operators
    /// disable copy/move (this is a scoped manager [of the 'transcript' concept])
    SpKDFTranscript& operator=(SpKDFTranscript&&) = delete;

//member functions
    /// build the transcript
    template<typename T>
    void append(const boost::string_ref, const T &value)
    {
        m_transcript_builder.append("", value);
    }

    /// access the transcript data
    const void* data() const { return m_transcript_builder.data(); }
    std::size_t size() const { return m_transcript_builder.size(); }

//member variables
private:
    /// underlying transcript builder
    SpTranscriptBuilder m_transcript_builder;
};

} //namespace sp
