// Copyright (c) 2020, The Monero Project
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

#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <rapidjson/reader.h>
#include <string>

#include "byte_slice.h"
#include "serialization/wire/fwd.h"
#include "serialization/wire/read.h"
#include "span.h"

namespace wire
{
  //! Reads JSON tokens one-at-a-time for DOMless parsing
  class json_reader : public reader
  {
    struct rapidjson_sax;

    std::string str_buffer_;
    rapidjson::Reader reader_;

    void read_next_value(rapidjson_sax& handler);
    char get_next_token();
    boost::string_ref get_next_string();

    //! Skips next value. \throw wire::exception if invalid JSON syntax.
    void skip_value();

    //! \throw wire::exception if next token not `[`.
    std::size_t do_start_array(std::size_t) override final;

    //! \throw wire::exception if next token not `{`.
    std::size_t do_start_object() override final;

  public:
    //! `source` must "outlive" `json_reader`.
    explicit json_reader(epee::span<const char> source);

    //! \throw wire::exception if JSON parsing is incomplete.
    void check_complete() const override final;

    //! \throw wire::exception if array, object, or end of stream.
    basic_value basic() override final;

    //! \throw wire::exception if next token not a boolean.
    bool boolean() override final;

    //! \throw wire::expception if next token not an integer.
    std::intmax_t integer() override final;

    //! \throw wire::exception if next token not an unsigned integer.
    std::uintmax_t unsigned_integer() override final;

    //! \throw wire::exception if next token is not an integer encoded as string
    std::uintmax_t safe_unsigned_integer();

    //! \throw wire::exception if next token not a valid real number
    double real() override final;

    //! \throw wire::exception if next token not a string
    std::string string() override final;

    /*! Copy upcoming string directly into `dest`.
      \throw wire::exception if next value not string
      \throw wire::exception if next string exceeds `dest.size())`
      \throw wire::exception if `exact == true` and next string size is not
        `dest.size()`
      \return Number of bytes read into `dest`. */
    std::size_t string(epee::span<char> dest, bool exact) override final;

    //! \throw wire::exception if next token cannot be read as hex
    epee::byte_slice binary() override final;

    /* Copy upcoming binary, encoded as a hex string, into `dest`.
       \throw wire::exception if next value is not a string
       \throw wire::exception if next string is not valid hex.
       \throw wire::exception if `dest.size() * 2` is less than next string length
       \throw wire::exception if `exact == true` and next string length is
         not exactly `dest.size() * 2`
       \return Number of bytes read into `dest` (not number of bytes read
         from wire). */
    std::size_t binary(epee::span<std::uint8_t> dest, bool exact) override final;

    //! Skips whitespace to next token. \return True if next token is eof or ']'.
    bool is_array_end(std::size_t count) override final;
 
    /*! \throw wire::exception if next token not key or `}`.
        \param[out] index of key match within `map`.
        \return True if another value to read. */
    bool key(epee::span<const key_map> map, std::size_t&, std::size_t& index) override final;
  };
} // wire
