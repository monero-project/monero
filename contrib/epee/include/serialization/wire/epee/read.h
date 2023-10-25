// Copyright (c) 2023, The Monero Project
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
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "serialization/wire/fwd.h"
#include "serialization/wire/read.h"
#include "serialization/wire/traits.h"
#include "span.h"

namespace wire
{
  //! If `T` is an arithmetic value in epee array, enforce sizeof min wire size
  template<typename T>
  struct default_min_element_size<epee_reader, T, std::enable_if_t<std::is_arithmetic<T>::value>>
    : std::integral_constant<std::size_t, sizeof(T)>
  {};
  
  //! Reads Epee binary archives one element at a time for DOMless parsing
  class epee_reader final : public reader
  {
    std::vector<std::pair<std::size_t, std::uint8_t>> skip_stack_;
    std::size_t array_space_;//!< Tracks remaining space after "open" arrays
    std::uint8_t last_tag_;  //!< Last type tag read


    //! Space-check only. \return POD with endianness swapped.
    template<typename T> T read();

    //! Space-check only. \return Next type tag.
    std::uint8_t read_tag();

    //! Read varint tag and space-check. \return Length of next array, object or string.
    std::size_t read_varint();

    //! Space-check only \return Field name (for object) unmodified
    boost::string_ref read_name();

    //! Check last tag type and space. \return Bytes iff last tag is string.
    epee::span<const std::uint8_t> raw(error::schema expected);

    //! Skip `count` fixed-size items (as determined by last tag). \throw wire::exception if invalid last tag
    void skip_fixed(std::size_t count);

    //! Skips next value. \throw wire::exception if invalid encoding
    void skip_next();

    /*! \post Last tag is set to inner type.
        \throw wire::exception if last tag not start of array */
    std::size_t do_start_array(std::size_t min_element_size) override final;

    //! \throw wire::exception if last tag not an object
    std::size_t do_start_object() override final;

  public:
    //! Enable array optimizations (element count always specified before elements).
    static constexpr std::false_type delimited_arrays() noexcept { return {}; }

    //! Assume first (root) tag is an object. `source` must "outlive" `epee_reader`.
    explicit epee_reader(epee::span<const std::uint8_t> source);

    epee_reader(epee_reader&&) = delete;
    epee_reader(const epee_reader&) = delete;

    virtual ~epee_reader() noexcept;

    epee_reader& operator=(epee_reader&&) = delete;
    epee_reader& operator=(const epee_reader&) = delete;

    //! Last epee binary type tag read
    std::uint8_t last_tag() const noexcept { return last_tag_; }

    //! \throw wire::exception if epee parsing is incomplete.
    void check_complete() const override final;

    //! \throw wire::exception if array, object, or end of stream.
    basic_value basic() override final;

    //! \throw wire::exception if last tag not a boolean.
    bool boolean() override final;

    //! \throw wire::expception if last tag not an integer.
    std::intmax_t integer() override final;

    //! \throw wire::exception if last tag not an unsigned integer.
    std::uintmax_t unsigned_integer() override final;

    //! \throw wire::exception if last tag not a valid real number
    double real() override final;

    //! \throw wire::exception if last tag not a string
    std::string string() override final;

    //! See `binary(dest, exact)`
    std::size_t string(epee::span<char> dest, const bool exact) override final
    {
      return binary({reinterpret_cast<std::uint8_t*>(dest.data()), dest.size()}, exact);
    }

    //! \throw wire::exception if last tag not a string
    epee::byte_slice binary() override final;

    /*! Copy upcoming string directly into `dest`. epee format does not
      distinguish between binary and string data.
      \throw wire::exception if next token not string
      \throw wire::exception if next string exceeds `dest.size()`
      \throw wire::exception if `exact == true` and next string is not `dest.size()`
      \return Number of bytes read into `dest`. */
    std::size_t binary(epee::span<std::uint8_t> dest, bool exact) override final;

    //! Sets last tag to array if `count == 0`. \return True if end of array (`count == 0`).
    bool is_array_end(std::size_t count) override final;

    /*! \throw wire::exception if next token not key or `}`.
        \param[out] index of key match within `map`.
        \post Last tag is set to object when false is returned, otherwise
          is set to tag of `index`.
        \return True if another value to read. */
    bool key(epee::span<const key_map> map, std::size_t&, std::size_t& index) override final;
  };
} // wire
