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

#include <boost/optional/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "byte_stream.h"
#include "serialization/wire/write.h"
#include "span.h"
#include "storages/portable_storage_base.h"
#include "storages/portable_storage_bin_utils.h"

namespace wire
{
  //! Writes epee tokens at a time for DOMless output.
  class epee_writer final : public writer
  {
    epee::byte_stream bytes_; //!< Output sink
    boost::optional<std::size_t> array_count_; //!< Count for upcoming array
    bool needs_tag_; //!< True iff type tag needs to be written

    // Convert built-in types to epee binary tag value
    static constexpr std::uint8_t get_tag(bool) noexcept { return SERIALIZE_TYPE_BOOL; }
    static constexpr std::uint8_t get_tag(std::int8_t) noexcept { return SERIALIZE_TYPE_INT8; }
    static constexpr std::uint8_t get_tag(std::int16_t) noexcept { return SERIALIZE_TYPE_INT16; }
    static constexpr std::uint8_t get_tag(std::int32_t) noexcept { return SERIALIZE_TYPE_INT32; }
    static constexpr std::uint8_t get_tag(std::int64_t) noexcept { return SERIALIZE_TYPE_INT64; }
    static constexpr std::uint8_t get_tag(std::uint8_t) noexcept { return SERIALIZE_TYPE_UINT8; }
    static constexpr std::uint8_t get_tag(std::uint16_t) noexcept { return SERIALIZE_TYPE_UINT16; }
    static constexpr std::uint8_t get_tag(std::uint32_t) noexcept { return SERIALIZE_TYPE_UINT32; }
    static constexpr std::uint8_t get_tag(std::uint64_t) noexcept { return SERIALIZE_TYPE_UINT64; }
    static constexpr std::uint8_t get_tag(double) noexcept { return SERIALIZE_TYPE_DOUBLE; }

    template<typename T>
    static constexpr std::uint8_t get_tag(const T&) noexcept
    {
      static_assert(std::is_same<T, void>::value, "write_arithmetic/get_tag only supported for limited types");
      return 0;
    }

    void write_tag(std::uint8_t tag);
    void write_varint(std::size_t value);

  public:
    epee_writer()
      : epee_writer(epee::byte_stream{})
    {}

    //! Append data to existing `buffer`.
    epee_writer(epee::byte_stream&& buffer);

    epee_writer(const epee_writer&) = delete;
    virtual ~epee_writer() noexcept;
    epee_writer& operator=(const epee_writer&) = delete;

    epee::byte_stream take_buffer();
    epee::byte_slice take_bytes();

    template<typename T>
    void write_arithmetic(T value)
    {
      static_assert(std::is_arithmetic<T>::value, "only arithmetic types allowed");
      if (needs_tag_)
        write_tag(get_tag(value));
      value = CONVERT_POD(value);
      bytes_.write(reinterpret_cast<const std::uint8_t*>(std::addressof(value)), sizeof(value));
    }

    //! 32-bit float not supported by epee format, upgrade
    void write_arithmetic(const float value) { write_arithmetic(double(value)); }

    void boolean(bool) override final;

    void integer(std::intmax_t) override final;
    void unsigned_integer(std::uintmax_t) override final;

    void real(double) override final;

    void binary(epee::span<const std::uint8_t> source) override final;
    void string(const boost::string_ref source) override final
    { binary(epee::strspan<std::uint8_t>(source)); }

    void start_array(std::size_t) override final;
    void end_array() override final;

    void start_object(std::size_t) override final;
    void key(boost::string_ref) override final;
    void binary_key(const epee::span<const std::uint8_t> source) override final
    { key({reinterpret_cast<const char*>(source.data()), source.size()}); }
    void end_object() override final;
  };

  // optimization for all numeric types
  template<typename T>
  inline std::enable_if_t<std::is_arithmetic<T>::value> write_bytes(epee_writer& dest, const T source)
  {
    dest.write_arithmetic(source);
  }
}
