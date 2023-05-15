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

#include "serialization/wire/json/write.h"

#include <boost/container/small_vector.hpp>
#include <stdexcept>

#include "hex.h"

namespace
{
  constexpr const unsigned flush_threshold = 100;
  constexpr const unsigned max_buffer = 4096;

  boost::container::small_vector<char, 256> json_hex(const epee::span<const std::uint8_t> source)
  {
    boost::container::small_vector<char, 256> buffer;
    buffer.resize(source.size() * 2);
    if (!epee::to_hex::buffer(epee::to_mut_span(buffer), source))
      throw std::logic_error{"Invalid buffer size for binary->hex conversion in json_writer"};
    return buffer;
  }
}

namespace wire
{
  void json_writer::do_flush(epee::span<const char>)
  {}

  void json_writer::check_flush()
  {
    if (needs_flush_ && (max_buffer < stream_.buffer_.size() || stream_.buffer_.capacity() - stream_.buffer_.size() < flush_threshold))
      flush();
  }

  void json_writer::check_complete()
  {
    if (!formatter_.IsComplete())
      throw std::logic_error{"json_writer::take_json() failed with incomplete JSON tree"};
  }
  std::string json_writer::take_json()
  {
    check_complete();
    std::string out{std::move(stream_.buffer_)};
    stream_.buffer_.clear();
    formatter_.Reset(stream_);
    return out;
  }

  json_writer::~json_writer() noexcept
  {}

  std::array<char, uint_to_string_size> json_writer::to_string(const std::uintmax_t value) noexcept
  {
    static_assert(std::numeric_limits<std::uintmax_t>::max() <= std::numeric_limits<std::uint64_t>::max(), "bad uint conversion");
    std::array<char, uint_to_string_size> buf{{}};
    rapidjson::internal::u64toa(std::uint64_t(value), buf.data());
    return buf;
  }

  void json_writer::boolean(const bool source)
  {
    formatter_.Bool(source);
    check_flush();
  }

  void json_writer::integer(const std::intmax_t source)
  {
    static_assert(std::numeric_limits<std::int64_t>::min() <= std::numeric_limits<std::intmax_t>::min(), "too small");
    static_assert(std::numeric_limits<std::intmax_t>::max() <= std::numeric_limits<std::int64_t>::max(), "too large");
    formatter_.Int64(source);
    check_flush();
  }
  void json_writer::unsigned_integer(const std::uintmax_t source)
  {
    static_assert(std::numeric_limits<std::uintmax_t>::max() <= std::numeric_limits<std::uint64_t>::max(), "too large");
    formatter_.Uint64(source);
    check_flush();
  }
  void json_writer::real(const double source)
  {
    formatter_.Double(source);
    check_flush();
  }

  void json_writer::string(const boost::string_ref source)
  {
    formatter_.String(source.data(), source.size());
    check_flush();
  }
  void json_writer::binary(const epee::span<const std::uint8_t> source)
  {
    const auto buffer = json_hex(source);
    string({buffer.data(), buffer.size()});
  }

  void json_writer::start_array(std::size_t)
  {
    formatter_.StartArray();
  }
  void json_writer::end_array()
  {
    formatter_.EndArray();
  }

  void json_writer::start_object(std::size_t)
  {
    formatter_.StartObject();
  }
  void json_writer::key(const boost::string_ref str)
  {
    formatter_.Key(str.data(), str.size());
    check_flush();
  }
  void json_writer::binary_key(const epee::span<const std::uint8_t> source)
  {
    const auto buffer = json_hex(source);
    key({buffer.data(), buffer.size()});
  }
  void json_writer::end_object()
  {
    formatter_.EndObject();
  }

  void json_stream_writer::do_flush(const epee::span<const char> bytes)
  {
    dest.write(bytes.data(), bytes.size());
  }
}
