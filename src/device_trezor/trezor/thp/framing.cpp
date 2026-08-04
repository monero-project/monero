// Copyright (c) 2026, The Monero Project
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

#include "framing.hpp"
#include "device_trezor/trezor/exceptions.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <boost/crc.hpp>

namespace hw {
namespace trezor {
namespace thp {

  // Initiation packets carry: control(1) + cid(2) + length(2) + payload-prefix
  static constexpr size_t INITIATION_HEADER_BYTES   = 5;
  // Continuation packets carry: control(1) + cid(2) + payload
  static constexpr size_t CONTINUATION_HEADER_BYTES = 3;
  // Trailer is a CRC-32 appended to the payload before chunking. The
  // checksum is CRC-32/ISO-HDLC over control || cid || length || payload.
  static constexpr size_t CRC32_BYTES = 4;

  std::vector<uint8_t> encode_frame(uint8_t control_byte, uint16_t channel_id,
                                    const uint8_t *payload, size_t payload_len,
                                    size_t chunk_size)
  {
    if (chunk_size <= INITIATION_HEADER_BYTES) {
      throw std::invalid_argument("chunk_size too small for THP initiation header");
    }
    if ((control_byte & CTRL_CONTINUATION_BIT) != 0) {
      throw std::invalid_argument("encode_frame: control byte indicates continuation");
    }

    // The "length" encoded in the header is payload_len + CRC32_BYTES.
    // Bounds-check payload_len on its own BEFORE the addition and the
    // narrowing, so an oversized value can neither wrap the size_t sum
    // nor the 16-bit field.
    if (payload_len > 0xFFFF - CRC32_BYTES) {
      throw std::invalid_argument("THP frame length exceeds 16-bit field");
    }
    const size_t length_field = payload_len + CRC32_BYTES;

    uint8_t header[INITIATION_HEADER_BYTES];
    header[0] = control_byte;
    write_be16(header + 1, channel_id);
    write_be16(header + 3, static_cast<uint16_t>(length_field));

    boost::crc_32_type crc;
    crc.process_bytes(header, sizeof(header));
    if (payload_len > 0) {
      crc.process_bytes(payload, payload_len);
    }
    uint8_t crc_be[CRC32_BYTES]; write_be32(crc_be, crc.checksum());

    // The transport body is payload || crc_be; it is chunked in place
    // rather than concatenated first.
    auto copy_body = [&](uint8_t *dst, size_t offset, size_t take) {
      if (offset < payload_len) {
        const size_t n = std::min(take, payload_len - offset);
        std::memcpy(dst, payload + offset, n);
        dst    += n;
        offset += n;
        take   -= n;
      }
      if (take > 0) {
        std::memcpy(dst, crc_be + (offset - payload_len), take);
      }
    };

    // The first chunk carries the initiation header (control + cid +
    // length); subsequent chunks carry the continuation header (control
    // with bit-7 set + cid).
    std::vector<uint8_t> out;
    size_t offset = 0;
    bool first = true;
    while (first || offset < length_field) {
      const size_t header_bytes = first ? INITIATION_HEADER_BYTES
                                        : CONTINUATION_HEADER_BYTES;
      const size_t room = chunk_size - header_bytes;
      const size_t take = std::min(room, length_field - offset);

      const size_t base = out.size();
      out.resize(base + chunk_size, 0); // pre-pad with zeros

      uint8_t *p = out.data() + base;
      if (first) {
        std::memcpy(p, header, INITIATION_HEADER_BYTES);
        copy_body(p + INITIATION_HEADER_BYTES, offset, take);
      } else {
        // Per THP spec: continuation packet control byte is exactly 0x80;
        // the low 7 bits are reserved and must be zero on transmit.
        p[0] = CTRL_CONTINUATION_BIT;
        write_be16(p + 1, channel_id);
        copy_body(p + CONTINUATION_HEADER_BYTES, offset, take);
      }

      offset += take;
      first = false;
    }

    return out;
  }

  void FrameAssembler::reset()
  {
    m_started = false;
    m_complete = false;
    m_control = 0;
    m_channel = 0;
    m_declared_len = 0;
    m_buf.clear();
  }

  bool FrameAssembler::feed_chunk(const uint8_t *chunk, size_t chunk_size)
  {
    if (m_complete) {
      throw exc::CommunicationException("THP: feed_chunk after frame completion (call take/reset)");
    }
    if (!chunk || chunk_size <= CONTINUATION_HEADER_BYTES) {
      throw exc::CommunicationException("THP: chunk too small");
    }

    if (!m_started) {
      // The initiation header is 5 bytes (ctrl + cid + len). The generic guard
      // above only rejects chunk_size <= CONTINUATION_HEADER_BYTES (3), so a
      // 4-byte chunk would let the read_be16(chunk+3) below read one byte out
      // of bounds. Require the full header; a chunk of exactly the header size
      // (no payload yet) is valid and handled by the declared-length logic.
      if (chunk_size < INITIATION_HEADER_BYTES) {
        throw exc::CommunicationException("THP: initiation chunk too small");
      }
      const uint8_t ctrl = chunk[0];
      if ((ctrl & CTRL_CONTINUATION_BIT) != 0) {
        throw exc::CommunicationException("THP: first chunk must be an initiation packet");
      }
      m_control = ctrl;
      m_channel = read_be16(chunk + 1);
      m_declared_len = read_be16(chunk + 3); // includes CRC32 trailer

      const size_t available = chunk_size - INITIATION_HEADER_BYTES;
      const size_t want = std::min<size_t>(m_declared_len, available);
      m_buf.insert(m_buf.end(), chunk + INITIATION_HEADER_BYTES,
                   chunk + INITIATION_HEADER_BYTES + want);
      m_started = true;
    } else {
      const uint8_t ctrl = chunk[0];
      if ((ctrl & CTRL_CONTINUATION_BIT) == 0) {
        throw exc::CommunicationException("THP: continuation chunk missing continuation bit");
      }
      const uint16_t cid = read_be16(chunk + 1);
      if (cid != m_channel) {
        throw exc::CommunicationException("THP: continuation chunk channel mismatch");
      }
      const size_t available = chunk_size - CONTINUATION_HEADER_BYTES;
      const size_t remaining = m_declared_len - m_buf.size();
      const size_t want = std::min(available, remaining);
      m_buf.insert(m_buf.end(), chunk + CONTINUATION_HEADER_BYTES,
                   chunk + CONTINUATION_HEADER_BYTES + want);
    }

    if (m_buf.size() < m_declared_len) {
      return false;
    }

    // Frame is fully received. Verify CRC.
    if (m_buf.size() < CRC32_BYTES) {
      throw exc::CommunicationException("THP: declared length below CRC trailer");
    }
    const size_t payload_len = m_buf.size() - CRC32_BYTES;
    const uint32_t got_crc = read_be32(m_buf.data() + payload_len);

    // Reconstruct the CRC input: control || cid || length || payload.
    uint8_t header[INITIATION_HEADER_BYTES];
    header[0] = m_control;
    write_be16(header + 1, m_channel);
    write_be16(header + 3, static_cast<uint16_t>(m_declared_len));

    boost::crc_32_type crc;
    crc.process_bytes(header, sizeof(header));
    if (payload_len > 0) {
      crc.process_bytes(m_buf.data(), payload_len);
    }
    if (crc.checksum() != got_crc) {
      throw exc::CommunicationException("THP: CRC mismatch on assembled frame");
    }

    m_buf.resize(payload_len);
    m_complete = true;
    return true;
  }

  Frame FrameAssembler::take()
  {
    if (!m_complete) {
      throw exc::CommunicationException("THP: take() called before frame complete");
    }
    Frame f;
    f.control_byte = m_control;
    f.channel_id   = m_channel;
    f.payload      = std::move(m_buf);
    reset();
    return f;
  }

}}}
