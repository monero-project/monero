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

#ifndef MONERO_TREZOR_THP_FRAMING_H
#define MONERO_TREZOR_THP_FRAMING_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <boost/endian/conversion.hpp>

namespace hw {
namespace trezor {
namespace thp {

  // Wire format constants (THP specification, data transfer layer).

  // USB always uses 64-byte packets.
  constexpr size_t USB_CHUNK_SIZE = 64;

  // Control-byte masks. Bit 7 distinguishes initiation (0) vs continuation (1).
  constexpr uint8_t CTRL_CONTINUATION_BIT = 0x80;
  // Per THP spec, "Transport packet structure":
  //   - DATA packets (handshake_*, encrypted_transport): bit 4 (0x10) is the
  //     sequence bit; bit 3 (0x08) is the optional piggyback-ACK bit.
  //   - ACK packets: bit 3 (0x08) is the sequence bit (which seq is being
  //     acknowledged); there is no piggyback bit.
  // The single-bit names match Python trezorlib/thp/control_byte.py.
  constexpr uint8_t CTRL_DATA_SEQ_BIT = 0x10;
  constexpr uint8_t CTRL_DATA_ACK_BIT = 0x08;
  constexpr uint8_t CTRL_ACK_SEQ_BIT  = 0x08;
  // Property masks used to identify the message type ignoring seq/ack bits.
  constexpr uint8_t CTRL_DATA_MASK = 0xE7;  // strips seq (0x10) and ack (0x08)
  constexpr uint8_t CTRL_ACK_MASK  = 0xF7;  // strips seq (0x08)

  // Reserved channel identifiers per THP spec.
  constexpr uint16_t CID_BROADCAST = 0xFFFF;
  constexpr uint16_t CID_RESERVED_LOW = 0xFFF0;
  constexpr uint16_t CID_INVALID = 0x0000;

  // Selected control-byte values (initiation packets).
  enum InitiationControl : uint8_t {
    CTRL_CHANNEL_ALLOC_REQUEST  = 0x40,
    CTRL_CHANNEL_ALLOC_RESPONSE = 0x41,
    CTRL_TRANSPORT_ERROR        = 0x42,
    CTRL_PING                   = 0x43,
    CTRL_PONG                   = 0x44,

    // ACK with sequence bit cleared / set.
    CTRL_ACK_SEQ0               = 0x20,
    CTRL_ACK_SEQ1               = 0x28,

    // Handshake init/completion. The low bits encode a request/response
    // and (for some) the sequence bit.
    CTRL_HANDSHAKE_INIT_REQ     = 0x00,
    CTRL_HANDSHAKE_INIT_RESP    = 0x01,
    CTRL_HANDSHAKE_COMP_REQ     = 0x02,
    CTRL_HANDSHAKE_COMP_RESP    = 0x03,

    // Encrypted application traffic. Sequence bit (CTRL_DATA_SEQ_BIT 0x10)
    // toggles between consecutive frames (yielding 0x04 / 0x14). The
    // optional piggyback-ACK bit (CTRL_DATA_ACK_BIT 0x08) may also be set.
    CTRL_ENCRYPTED_TRANSPORT    = 0x04,
  };

  // Transport error codes carried in TRANSPORT_ERROR frames.
  enum TransportError : uint8_t {
    TRANSPORT_ERR_BUSY                  = 1,
    TRANSPORT_ERR_UNALLOCATED_CHANNEL   = 2,
    TRANSPORT_ERR_DECRYPTION_FAILED     = 3,
    TRANSPORT_ERR_DEVICE_LOCKED         = 5,
  };

  // A reassembled THP frame at the data-transfer layer. The CRC has been
  // verified and stripped. `payload` is the message body (transport-layer
  // contents, *not* yet decrypted).
  struct Frame {
    uint8_t  control_byte = 0;
    uint16_t channel_id   = 0;
    std::vector<uint8_t> payload;

    bool is_continuation() const { return (control_byte & CTRL_CONTINUATION_BIT) != 0; }
    bool is_ack() const          { return (control_byte & CTRL_ACK_MASK) == CTRL_ACK_SEQ0; }
    // For DATA packets (handshake_*, encrypted_transport): sequence bit is at 0x10.
    uint8_t data_seq_bit() const { return (control_byte & CTRL_DATA_SEQ_BIT) ? 1 : 0; }
    // For DATA packets: piggyback-ACK bit is at 0x08.
    uint8_t data_ack_bit() const { return (control_byte & CTRL_DATA_ACK_BIT) ? 1 : 0; }
    // For ACK packets: sequence bit is at 0x08.
    uint8_t ack_seq_bit()  const { return (control_byte & CTRL_ACK_SEQ_BIT)  ? 1 : 0; }
  };

  // Build the wire bytes for a single transport-layer frame. Splits the
  // frame into one initiation chunk plus zero or more continuation chunks
  // of `chunk_size` bytes each (default 64 for USB), pads the final chunk
  // with zeros, and appends the CRC-32 to the payload before splitting.
  //
  // Output layout (concatenated):
  //   chunk_0 = ctrl || cid_be || len_be || payload[0..min(59,len)]      -> padded to chunk_size
  //   chunk_1 = 0x80 || cid_be || payload[59..min(59+61,len)]            -> padded to chunk_size
  //   ...
  // where len includes the appended CRC-32.
  //
  // Throws std::invalid_argument on impossible sizes.
  std::vector<uint8_t> encode_frame(uint8_t control_byte, uint16_t channel_id,
                                    const uint8_t *payload, size_t payload_len,
                                    size_t chunk_size = USB_CHUNK_SIZE);

  // Stateful assembler that consumes raw chunks (USB reads) and yields a
  // complete Frame once all continuation packets have arrived. The CRC is
  // verified at the end; mismatched CRC raises CommunicationException via
  // the caller's error-handling layer.
  class FrameAssembler {
  public:
    FrameAssembler() = default;

    // Reset internal state. Call between frames.
    void reset();

    // Feed a single raw chunk of exactly chunk_size bytes. Returns true
    // when the frame is complete (caller may then call take()).
    bool feed_chunk(const uint8_t *chunk, size_t chunk_size);

    // Take the assembled frame. Resets internal state. Only valid after
    // feed_chunk() returned true; otherwise throws.
    Frame take();

    // Whether a frame is currently being assembled but not yet complete.
    bool in_progress() const { return m_started && !m_complete; }

  private:
    bool      m_started   = false;
    bool      m_complete  = false;
    uint8_t   m_control   = 0;
    uint16_t  m_channel   = 0;
    uint32_t  m_declared_len = 0; // includes CRC trailer
    std::vector<uint8_t> m_buf;   // payload accumulator (without CRC trailer)
  };

  // Header fields are big-endian per THP spec and are not word-aligned on
  // the wire, hence the memcpy.
  inline uint16_t read_be16(const uint8_t *p) {
    uint16_t wire;
    std::memcpy(&wire, p, sizeof(wire));
    return boost::endian::big_to_native(wire);
  }
  inline uint32_t read_be32(const uint8_t *p) {
    uint32_t wire;
    std::memcpy(&wire, p, sizeof(wire));
    return boost::endian::big_to_native(wire);
  }
  inline void write_be16(uint8_t *p, uint16_t v) {
    const uint16_t wire = boost::endian::native_to_big(v);
    std::memcpy(p, &wire, sizeof(wire));
  }
  inline void write_be32(uint8_t *p, uint32_t v) {
    const uint32_t wire = boost::endian::native_to_big(v);
    std::memcpy(p, &wire, sizeof(wire));
  }

}}}

#endif // MONERO_TREZOR_THP_FRAMING_H
