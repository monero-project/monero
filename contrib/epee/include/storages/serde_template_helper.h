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

#include "byte_slice.h" // from epee
#include "byte_stream.h" // from epee
#include "file_io_utils.h" // from epee
#include "serde/epee_binary/deserializer.h"
#include "serde/epee_binary/serializer.h"
#include "serde/json/deserializer.h"
#include "serde/json/serializer.h"

namespace epee::serialization
{
    // json_buff reference must be non-const since ParseInsitu mutates buffer in place
    template<class t_struct>
    bool load_t_from_json(t_struct& out, const std::string& json_buff) noexcept
    {
        serde::json::Deserializer deserializer(&json_buff.front());
        try
        {
            return deserialize_default(deserializer, out);
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }

    template<class t_struct>
    bool load_t_from_json_file(t_struct& out, const std::string& json_file_path) noexcept
    {
        std::string file_contents;
        // @TODO: maybe put a file size limit
        // @TODO: stream file contents into deserializer, not load entire file into memory
        if (epee::file_io_utils::load_file_to_string(json_file_path, file_contents))
        {
            MERROR("Could not read file contents from path '" << json_file_path << "'");
            return false;
        }

        return load_t_from_json(out, file_contents);
    }

    template<class t_struct>
    bool store_t_to_json(const t_struct& src_value, std::string& json_buff) noexcept
    {
        std::stringstream ss;
        serde::json::Serializer<std::stringstream> serializer(ss);
        try
        {
            serialize_default(src_value, serializer);
            json_buff = ss.str(); // @TODO: str() copies the string buffer
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }

    // possibly throws
    template<class t_struct>
    std::string store_t_to_json(t_struct& str_in)
    {
        std::string json_buff;
        CHECK_AND_ASSERT_THROW_MES
        (
            store_t_to_json(str_in, json_buff),
            "internal call to store_to_json failed"
        );
        
        return json_buff;
    }

    template<class t_struct>
    bool store_t_to_json_file(t_struct& str_in, const std::string& fpath) noexcept
    {
        std::string json_buff;
        store_t_to_json(str_in, json_buff);
        return file_io_utils::save_string_to_file(fpath, json_buff);
    }

    template<class t_struct>
    bool load_t_from_binary(t_struct& out, const epee::span<const uint8_t>& binary_buff) noexcept
    {
        serde::epee_binary::Deserializer deserializer{binary_buff};
        try
        {
            return deserialize_default(deserializer, out);
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }

    template<class t_struct>
    bool load_t_from_binary(t_struct& out, const std::string& binary_buff) noexcept
    {
        return load_t_from_binary(out, serde::internal::string_to_byte_span(binary_buff));
    }

    template<class t_struct>
    bool load_t_from_binary(t_struct& target_value, const byte_slice& binary_buff) noexcept
    {
        return load_t_from_binary(target_value, to_span(binary_buff));
    }

    template<class t_struct>
    bool load_t_from_binary_file(t_struct& target_value, const std::string& binary_file_path) noexcept
    {
        std::string file_contents;
        if(!file_io_utils::load_file_to_string(binary_file_path, file_contents))
            return false;

        return load_t_from_binary(target_value, file_contents);
    }

    template<class t_struct>
    bool store_t_to_binary(const t_struct& src_value, byte_stream& bystream) noexcept
    {
        serde::epee_binary::Serializer<byte_stream> serializer(bystream);
        try
        {
            serialize_default(src_value, serializer);
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }

    template<class t_struct>
    bool store_t_to_binary(const t_struct& src_value, byte_slice& binary_buff, size_t initial_buffer_size = 8192)
    {
        byte_stream bystream;
        bystream.reserve(initial_buffer_size);
        if (!store_t_to_binary(src_value, bystream))
        {
            return false;
        }
        binary_buff = byte_slice{std::move(bystream), false}; // false says to not shrink byte_slice
        return true;
    }

    // possibly throws
    template<class t_struct>
    byte_slice store_t_to_binary(const t_struct& src_value, size_t initial_buffer_size = 8192)
    {
        byte_slice binary_buff;
        CHECK_AND_ASSERT_THROW_MES
        (
            store_t_to_binary(src_value, binary_buff, initial_buffer_size),
            "internal call to store_t_to_binary failed"
        );
        return binary_buff;
    }
} // namespace epee::serialization
