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

#include "serialization/wire/cbor/write.h"

#include <boost/endian/buffers.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/fusion/include/std_tuple.hpp>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "serialization/wire/error.h"
#include "serialization/wire/cbor/error.h"

namespace
{
  template<typename T, std::uint8_t M>
  void write_endian(epee::byte_stream& bytes, const std::uintmax_t value, const wire::cbor::major major, const wire::cbor::minor<T, M> minor)
  {
    assert(value <= minor.max());

    static constexpr const std::size_t bits = 8 * sizeof(T);
    using buffer_type =
      boost::endian::endian_buffer<boost::endian::order::big, T, bits>;
    const buffer_type buffer(value);

    bytes.put(std::uint8_t(major) | minor.value());
    bytes.write(buffer.data(), sizeof(buffer));
  }
}

namespace wire
{
  void cbor_writer::do_unsigned_integer(const cbor::major major, const std::uintmax_t value)
  {
    if (value <= cbor::inline_max())
        bytes_.put(std::uint8_t(major) | std::uint8_t(value));
    else
    {
      const auto match_size = [this, major, value] (const auto minor) -> bool
      {
        if (minor.max() < value)
          return false;
        write_endian(bytes_, value, major, minor);
        return true;
      };
      if (!boost::fusion::any(cbor::unsigned_types{}, match_size))
        WIRE_DLOG_THROW_(error::cbor::integer_encoding);
    }

    --expected_;
  }

  void cbor_writer::check_complete()
  {
    if (expected_)
      throw std::logic_error{"cbor_writer::take_cbor() failed with incomplete tree"};
  }
  epee::byte_stream cbor_writer::take_cbor()
  {
    check_complete();
    epee::byte_stream out{std::move(bytes_)};
    bytes_.clear();
    return out;
  }

  cbor_writer::~cbor_writer() noexcept
  {}

  void cbor_writer::integer(const std::intmax_t value)
  {
    if (0 <= value)
      unsigned_integer(std::uintmax_t(value));
    else
      do_unsigned_integer(cbor::major::negative_int, std::uintmax_t(-std::intmax_t(value + 1)));
  }

  void cbor_writer::unsigned_integer(const std::uintmax_t value)
  { do_unsigned_integer(cbor::major::positive_int, value); }

  void cbor_writer::real(const double source)
  {
    static_assert(std::numeric_limits<double>::is_iec559);
    static_assert(sizeof(double) == sizeof(std::uint64_t));

    std::uint64_t buf;
    std::memcpy(&buf, &source, sizeof(buf));
    write_endian(bytes_, buf, cbor::major::real_and_special, cbor::real64{}); 
    --expected_;
  }

  void cbor_writer::string(const std::string_view source)
  {
    do_unsigned_integer(cbor::major::string, source.size());
    bytes_.write(source.data(), source.size());
  }
  void cbor_writer::binary(epee::span<const std::uint8_t> source)
  {
    do_unsigned_integer(cbor::major::bytes, source.size());
    bytes_.write(source);
  }

  void cbor_writer::start_array(const std::size_t items)
  {
    do_unsigned_integer(cbor::major::array, items);
    expected_ += items + 1;
  }

  void cbor_writer::start_object(const std::size_t items)
  {
    do_unsigned_integer(cbor::major::map, items);
    expected_ += items * 2 + 1;
  }
}
