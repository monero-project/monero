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

namespace
{
#if BYTE_ORDER != LITTLE_ENDIAN
epee::levin::bucket_head2 swap_header_endian(const epee::levin::bucket_head2 &head)
{
  epee::levin::bucket_head2 little_head = head;
  little_head.m_signature = SWAP64(little_head.m_signature);
  little_head.m_cb = SWAP64(little_head.m_cb);
  little_head.m_command = SWAP32(little_head.m_command);
  little_head.m_return_code =  SWAP32(little_head.m_return_code);
  little_head.m_flags = SWAP32(little_head.m_flags);
  little_head.m_protocol_version = SWAP32(little_head.m_protocol_version);
  return little_head;
}
#endif
} //anonymous namespace

namespace epee
{
namespace levin
{
  message_writer::message_writer(const std::size_t reserve)
    : buffer()
  {
    buffer.reserve(reserve);
    buffer.put_n(0, levin_v1_header_size);
  }

  byte_slice message_writer::finalize(const uint32_t command, const uint32_t flags, const uint32_t return_code, const bool expect_response)
  {
    if (buffer.size() < levin_v1_header_size)
      throw std::runtime_error{"levin_writer::finalize already called"};

    header head = make_header(command, payload_size(), flags, expect_response);
    head.m_return_code = return_code;

    write_header(head, buffer.tellp() - buffer.size());
    return byte_slice{std::move(buffer)};
  }

  bucket_head2 make_header(uint32_t command, uint64_t msg_size, uint32_t flags, bool expect_response) noexcept
  {
    bucket_head2 head = {0};
    head.m_signature = LEVIN_SIGNATURE;
    head.m_have_to_return_data = expect_response;
    head.m_cb = msg_size;

    head.m_command = command;
    head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
    head.m_flags = flags;
    return head;
  }

  void write_header(const bucket_head2 &head, unsigned char head_bytes[levin_v1_header_size]) noexcept
  {
    static constexpr std::size_t w_size = 2 * sizeof(uint64_t);
    static_assert(offsetof(bucket_head2, m_have_to_return_data) == w_size);
    static_assert(sizeof(bucket_head2::m_have_to_return_data) == 1);
    static constexpr std::size_t end_offset = offsetof(bucket_head2, m_protocol_version) + sizeof(bucket_head2::m_protocol_version);
    static_assert(end_offset - offsetof(bucket_head2, m_command) == w_size);
    static_assert(2 * w_size + 1 == levin_v1_header_size);

    #if BYTE_ORDER == LITTLE_ENDIAN
    const bucket_head2 &little_head = head;
    #else
    bucket_head2 little_head = swap_header_endian(head);
    #endif

    unsigned char *pout = reinterpret_cast<unsigned char*>(head_bytes);

    memcpy(pout, &little_head, w_size);
    pout += w_size;
    *(pout++) = little_head.m_have_to_return_data;
    memcpy(pout, &little_head.m_command, w_size);
    assert(pout + w_size == head_bytes + levin_v1_header_size);
  }

  void read_header(const unsigned char head_bytes[levin_v1_header_size], bucket_head2 &head) noexcept
  {
    static constexpr std::size_t w_size = 2 * sizeof(uint64_t);
    static_assert(offsetof(bucket_head2, m_have_to_return_data) == w_size);
    static_assert(sizeof(bucket_head2::m_have_to_return_data) == 1);
    static constexpr std::size_t end_offset = offsetof(bucket_head2, m_protocol_version) + sizeof(bucket_head2::m_protocol_version);
    static_assert(end_offset - offsetof(bucket_head2, m_command) == w_size);
    static_assert(2 * w_size + 1 == levin_v1_header_size);

    head = {};

    const unsigned char *pin = reinterpret_cast<const unsigned char*>(head_bytes);
  
    memcpy(&head, pin, w_size);
    pin += w_size;
    head.m_have_to_return_data = *(pin++);
    memcpy(&head.m_command, pin, w_size);
    assert(pin + w_size == head_bytes + levin_v1_header_size);

    #if BYTE_ORDER != LITTLE_ENDIAN
    head = swap_header_endian(head);
    #endif
  }

  byte_slice make_noise_notify(const std::size_t noise_bytes)
  {
    static constexpr const std::uint32_t flags =
        LEVIN_PACKET_BEGIN | LEVIN_PACKET_END;

    if (noise_bytes < levin_v1_header_size)
      return nullptr;

    std::vector<std::uint8_t> buffer(noise_bytes);
    const bucket_head2 head = make_header(0, noise_bytes - levin_v1_header_size, flags, false);
    write_header(head, std::addressof(buffer[0]));

    return byte_slice{std::move(buffer)};
  }

  byte_slice make_fragmented_notify(const std::size_t noise_size, const int command, message_writer message)
  {
    if (noise_size < levin_v1_header_size * 2)
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

    const size_t payload_space = noise_size - levin_v1_header_size;
    const size_t expected_fragments = ((payload.size() - 2) / payload_space) + 1;

    byte_stream buffer{};
    buffer.reserve(expected_fragments * noise_size);

    bucket_head2 head = make_header(0, payload_space, LEVIN_PACKET_BEGIN, false);
    unsigned char head_bytes[levin_v1_header_size];
    write_header(head, head_bytes);
    buffer.write(head_bytes, levin_v1_header_size);

    // internal levin header is in payload already

    size_t copy_size = payload.remove_prefix(payload_space);
    buffer.write(payload.data() - copy_size, copy_size);

    head.m_flags = 0;
    while (!payload.empty())
    {
      copy_size = payload.remove_prefix(payload_space);

      if (payload.empty())
        head.m_flags = LEVIN_PACKET_END;

      write_header(head, head_bytes);
      buffer.write(head_bytes, levin_v1_header_size);
      buffer.write(payload.data() - copy_size, copy_size);
    }

    const size_t padding = noise_size - copy_size - levin_v1_header_size;
    buffer.put_n(0, padding);

    return byte_slice{std::move(buffer)};
  }
} // levin
} // epee
