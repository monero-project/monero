// Copyright (c) 2026, The Monero Project
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

#include <cstdint>
#include <string_view>

#include "byte_stream.h"
#include "span.h"

#include "serialization/wire/cbor/base.h"
#include "serialization/wire/write.h"

namespace wire
{
  //! Writes CBOR tokens one-at-a-time for DOMless output.
  class cbor_writer : public writer
  {
    epee::byte_stream bytes_;
    std::size_t expected_; //! Tracks number of expected elements remaining

    void do_unsigned_integer(cbor::major major, std::uintmax_t);

  protected:
    cbor_writer(epee::byte_stream&& sink)
      : writer(), bytes_(std::move(sink)), expected_(1)
    {}

    //! \throw std::logic_error if tree was not completed
    void check_complete();

    //! \throw std::logic_error if incomplete cbor tree. \return cbor bytes
    epee::byte_stream take_cbor();

  public:
    cbor_writer(const cbor_writer&) = delete;
    virtual ~cbor_writer() noexcept;
    cbor_writer& operator=(const cbor_writer&) = delete;

    void boolean(const bool value) override final
    {
      if (value)
        bytes_.put(cbor::true_::value());
      else
        bytes_.put(cbor::false_::value());
      --expected_;
    }

    void integer(const std::intmax_t value) override final;
    void unsigned_integer(const std::uintmax_t value) override final;

    void real(double) override final;

    void string(std::string_view source) override final;
    void binary(epee::span<const std::uint8_t> source) override final;

    void start_array(std::size_t items) override final;
    void end_array() override final { --expected_; }

    void start_object(std::size_t items) override final;
    void key(const std::string_view key) override final { string(key); }
    void binary_key(const epee::span<const std::uint8_t> key) override final { binary(key); }
    void end_object() override final { --expected_; }
  };

  //! Buffers entire cbor message in memory
  struct cbor_slice_writer final : cbor_writer
  {
    explicit cbor_slice_writer(epee::byte_stream&& sink)
      : cbor_writer(std::move(sink))
    {}

    explicit cbor_slice_writer()
      : cbor_slice_writer(epee::byte_stream{})
    {}

    //! \throw std::logic_error if incomplete tree \return cbor bytes
    epee::byte_stream take_buffer()
    {
      return cbor_writer::take_cbor();
    }
  };
}
