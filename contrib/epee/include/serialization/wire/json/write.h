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

#include <array>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <limits>
#include <rapidjson/writer.h>
#include <string>

#include "serialization/wire/write.h"
#include "span.h"

namespace wire
{
  constexpr const std::size_t uint_to_string_size =
    std::numeric_limits<std::uintmax_t>::digits10 + 2;

  struct string_stream
  {
    using Ch = char;
    std::string buffer_;

    void Put(const char c) { buffer_.push_back(c); }
    static void Flush() noexcept {}
  };

  //! Compatability/optimization for rapidjson.
  inline void PutReserve(string_stream& dest, const std::size_t length)
  {
    dest.buffer_.reserve(length);
  }

  //! Compability/optimization for rapidjson.
  inline void PutN(string_stream& dest, const std::uint8_t ch, const std::size_t count)
  {
    dest.buffer_.append(count, ch);
  }

  //! Writes JSON tokens one-at-a-time for DOMless output.
  class json_writer : public writer
  {
    string_stream stream_;
    rapidjson::Writer<string_stream> formatter_;
    bool needs_flush_;

    virtual void do_flush(epee::span<const char>);

    //! Flush written bytes to `do_flush(...)` if configured
    void check_flush();

  protected:
    json_writer(bool needs_flush)
      : writer(), stream_{}, formatter_(stream_), needs_flush_(needs_flush)
    {}

    json_writer(std::string&& bytes)
      : writer(), stream_{std::move(bytes)}, formatter_(stream_), needs_flush_(false)
    {}

    //! \throw std::logic_error if incomplete JSON tree. \return JSON bytes
    std::string take_json();

    //! Flush bytes in local buffer to `do_flush(...)`
    void flush()
    {
      do_flush(epee::to_span(stream_.buffer_));
      stream_.buffer_.clear();
    }

  public:
    json_writer(const json_writer&) = delete;
    virtual ~json_writer() noexcept;
    json_writer& operator=(const json_writer&) = delete;

    //! JSON does not have length field for arrays
    static constexpr std::false_type need_array_size() noexcept { return {}; }

    //! \return Null-terminated buffer containing uint as decimal ascii
    static std::array<char, uint_to_string_size> to_string(std::uintmax_t) noexcept;

    //! \throw std::logic_error if incomplete JSON tree
    void check_complete();

    void boolean(bool) override final;

    void integer(std::intmax_t) override final;

    void unsigned_integer(std::uintmax_t) override final;

    void real(double) override final;

    void string(boost::string_ref) override final;
    void binary(epee::span<const std::uint8_t>) override final;

    void start_array(std::size_t) override final;
    void end_array() override final;

    void start_object(std::size_t) override final;
    void key(boost::string_ref) override final;
    void binary_key(epee::span<const std::uint8_t>) override final;
    void end_object() override final;
  };

  //! Buffers entire JSON message in memory
  struct json_string_writer final : json_writer
  {
    //! Writer json to an empty buffer.
    explicit json_string_writer()
      : json_string_writer(std::string{})
    {}

    //! Append json to existing `buffer`.
    explicit json_string_writer(std::string&& buffer)
      : json_writer(std::move(buffer))
    {}

    //! \throw std::logic_error if incomplete JSON tree \return JSON bytes
    std::string take_buffer()
    {
      return json_writer::take_json();
    }
  };

  //! Periodically flushes JSON data to `std::ostream`
  class json_stream_writer final : public json_writer
  {
    std::ostream& dest;

    virtual void do_flush(epee::span<const char>) override final;
  public:
    explicit json_stream_writer(std::ostream& dest)
      : json_writer(true), dest(dest)
    {}

    //! Flush remaining bytes to stream \throw std::logic_error if incomplete JSON tree
    void finish()
    {
      check_complete();
      flush();
    }
  };
}
