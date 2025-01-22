// Copyright (c) 2019-2024, The Monero Project
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

#include "net/levin_base.h"

#include "int-util.h"

namespace epee
{
namespace levin
{
  message_writer::message_writer(const std::size_t reserve)
    : buffer()
  {
    buffer.reserve(reserve);
    buffer.put_n(0, sizeof(header));
  }

  byte_slice message_writer::finalize(const uint32_t command, const uint32_t flags, const uint32_t return_code, const bool expect_response)
  {
    if (buffer.size() < sizeof(header))
      throw std::runtime_error{"levin_writer::finalize already called"};

    header head = make_header(command, payload_size(), flags, expect_response);
    head.m_return_code = SWAP32LE(return_code);

    std::memcpy(buffer.tellp() - buffer.size(), std::addressof(head), sizeof(head));
    return byte_slice{std::move(buffer)};
  }

  bucket_head2 make_header(uint32_t command, uint64_t msg_size, uint32_t flags, bool expect_response) noexcept
  {
    bucket_head2 head = {0};
    head.m_signature = SWAP64LE(LEVIN_SIGNATURE);
    head.m_have_to_return_data = expect_response;
    head.m_cb = SWAP64LE(msg_size);

    head.m_command = SWAP32LE(command);
    head.m_protocol_version = SWAP32LE(LEVIN_PROTOCOL_VER_1);
    head.m_flags = SWAP32LE(flags);
    return head;
  }

  byte_slice make_noise_notify(const std::size_t noise_bytes)
  {
    static constexpr const std::uint32_t flags =
        LEVIN_PACKET_BEGIN | LEVIN_PACKET_END;

    if (noise_bytes < sizeof(bucket_head2))
      return nullptr;

    std::string buffer(noise_bytes, char(0));
    const bucket_head2 head = make_header(0, noise_bytes - sizeof(bucket_head2), flags, false);
    std::memcpy(std::addressof(buffer[0]), std::addressof(head), sizeof(head));

    return byte_slice{std::move(buffer)};
  }

  byte_slice make_fragmented_notify(const std::size_t noise_size, const int command, message_writer message)
  {
    if (noise_size < sizeof(bucket_head2) * 2)
      return nullptr;

    if (message.buffer.size() <= noise_size)
    {
      /* The entire message can be sent at once, and the levin binary parser
         will ignore extra bytes. So just pad with zeroes and otherwise send
         a "normal", not fragmented message. */

      message.buffer.put_n(0, noise_size - message.buffer.size());
      return message.finalize_notify(command);
    }

    // fragment message
    const byte_slice payload_bytes = message.finalize_notify(command);
    span<const std::uint8_t> payload = to_span(payload_bytes);

    const size_t payload_space = noise_size - sizeof(bucket_head2);
    const size_t expected_fragments = ((payload.size() - 2) / payload_space) + 1;

    byte_stream buffer{};
    buffer.reserve(expected_fragments * noise_size);

    bucket_head2 head = make_header(0, payload_space, LEVIN_PACKET_BEGIN, false);
    buffer.write(as_byte_span(head));

    // internal levin header is in payload already

    size_t copy_size = payload.remove_prefix(payload_space);
    buffer.write(payload.data() - copy_size, copy_size);

    head.m_flags = 0;
    while (!payload.empty())
    {
      copy_size = payload.remove_prefix(payload_space);

      if (payload.empty())
        head.m_flags = LEVIN_PACKET_END;

      buffer.write(as_byte_span(head));
      buffer.write(payload.data() - copy_size, copy_size);
    }

    const size_t padding = noise_size - copy_size - sizeof(bucket_head2);
    buffer.put_n(0, padding);

    return byte_slice{std::move(buffer)};
  }
} // levin
} // epee
