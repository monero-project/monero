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

#include "protocol_v2.hpp"
#include "framing.hpp"
#include "device_trezor/trezor/exceptions.hpp"

#include "crypto/crypto.h"

#include <cstdio>
#include <cstring>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor.thp"

namespace hw {
namespace trezor {
namespace thp {

  static constexpr size_t NONCE_BYTES = 8;
  static constexpr size_t ALLOC_RESPONSE_FIXED_BYTES = NONCE_BYTES + sizeof(uint16_t);

  // Budget for the whole handshake, in milliseconds. Nothing in it waits on
  // the user, so a device that stops answering must not hold the device lock
  // for longer than this.
  static constexpr unsigned int HANDSHAKE_TIMEOUT_MS = 30000;
  // Budget for one application-level exchange. A response may wait on an
  // on-device confirmation, hence the generous value; it exists only so the
  // wallet cannot block forever on a wedged device.
  static constexpr unsigned int EXCHANGE_TIMEOUT_MS = 300000;

  // Chunks read during one channel allocation, and frames tolerated within
  // one exchange, before the operation is abandoned.
  static constexpr unsigned int MAX_ALLOC_CHUNKS = 64;
  static constexpr unsigned int MAX_FRAMES_PER_EXCHANGE = 16;

  using Deadline = std::chrono::steady_clock::time_point;

  static Deadline deadline_in(unsigned int timeout_ms)
  {
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  }

  // Milliseconds left before `deadline`, zero once it has passed. Callers
  // must not pass zero on to Transport::read_chunk, where it means "no
  // timeout".
  static unsigned int remaining_ms(const Deadline &deadline)
  {
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count();
    return left > 0 ? static_cast<unsigned int>(left) : 0;
  }

  // A protocol-v1 device answers the broadcast allocation request with a
  // codec_v1 report, which always starts with the magic '?','#','#'.
  static bool is_codec_v1_report(const uint8_t *chunk, size_t len)
  {
    return len >= 3 && chunk[0] == '?' && chunk[1] == '#' && chunk[2] == '#';
  }

  AllocationResult allocate_channel(Transport &transport, AllocatedChannel &out,
                                    unsigned int timeout_ms)
  {
    AllocatedChannel alloc;
    alloc.nonce.resize(NONCE_BYTES);
    crypto::generate_random_bytes_thread_safe(alloc.nonce.size(), alloc.nonce.data());

    const size_t cs = transport.chunk_size();
    auto wire = encode_frame(CTRL_CHANNEL_ALLOC_REQUEST, CID_BROADCAST,
                             alloc.nonce.data(), alloc.nonce.size(), cs);
    for (size_t off = 0; off < wire.size(); off += cs) {
      transport.write_chunk(wire.data() + off, cs);
    }

    // Allocation read loop. Chunks that do not parse as THP are discarded
    // rather than fatal: this is the probe, and a protocol-v1 device answers
    // it with codec_v1 line noise that must not abort the connection. The
    // encrypted-transport loop in recv_frame() below deliberately does the
    // opposite.
    //
    // Per THP spec, "Allocation layer": a response whose nonce differs from
    // the one sent is ignored and the host keeps waiting. The same applies to
    // stale frames left in the USB buffer by a previous session.
    const Deadline deadline = deadline_in(timeout_ms);
    FrameAssembler asm_;
    std::vector<uint8_t> chunk(cs);
    unsigned int chunks = 0;

    while (chunks++ < MAX_ALLOC_CHUNKS) {
      const unsigned int left = remaining_ms(deadline);
      if (left == 0) {
        return AllocationResult::no_response;
      }

      size_t got = 0;
      try {
        got = transport.read_chunk(chunk.data(), cs, left);
      } catch (const exc::TimeoutException &) {
        return AllocationResult::no_response;
      }
      if (got == 0) {
        throw exc::CommunicationException("THP: zero-length chunk during channel allocation");
      }
      if (is_codec_v1_report(chunk.data(), got)) {
        MDEBUG("THP alloc: codec_v1 reply to the broadcast probe; legacy device");
        return AllocationResult::codec_v1_reply;
      }

      Frame f;
      try {
        if (!asm_.feed_chunk(chunk.data(), got)) {
          continue;
        }
        f = asm_.take();
      } catch (const exc::CommunicationException &e) {
        MDEBUG("THP alloc: discarding unparseable chunk (" << e.what() << ")");
        asm_.reset();
        continue;
      }

      if (f.channel_id != CID_BROADCAST) {
        MDEBUG("THP alloc: ignoring frame on non-broadcast channel 0x"
               << std::hex << f.channel_id);
        continue;
      }
      if (f.control_byte != CTRL_CHANNEL_ALLOC_RESPONSE) {
        MDEBUG("THP alloc: ignoring frame with control byte 0x"
               << std::hex << int(f.control_byte));
        continue;
      }
      if (f.payload.size() < ALLOC_RESPONSE_FIXED_BYTES) {
        MDEBUG("THP alloc: ignoring truncated alloc response (size=" << f.payload.size() << ")");
        continue;
      }
      if (std::memcmp(f.payload.data(), alloc.nonce.data(), NONCE_BYTES) != 0) {
        MDEBUG("THP alloc: ignoring response with mismatched nonce (likely stale from previous session)");
        continue;
      }

      alloc.channel_id = read_be16(f.payload.data() + NONCE_BYTES);
      if (alloc.channel_id == CID_INVALID || alloc.channel_id >= CID_RESERVED_LOW) {
        throw exc::CommunicationException("THP: device returned reserved CID");
      }

      // The remaining bytes are the protobuf-encoded device properties; the
      // caller decodes them lazily.
      if (f.payload.size() > ALLOC_RESPONSE_FIXED_BYTES) {
        alloc.device_properties_pb.assign(f.payload.begin() + ALLOC_RESPONSE_FIXED_BYTES,
                                          f.payload.end());
      }
      out = std::move(alloc);
      return AllocationResult::allocated;
    }

    MDEBUG("THP alloc: no usable response within " << MAX_ALLOC_CHUNKS << " chunks");
    return AllocationResult::no_response;
  }

  ProtocolV2::ProtocolV2() = default;

  void ProtocolV2::send_frame(Transport &transport, uint8_t control_byte,
                              const uint8_t *payload, size_t payload_len)
  {
    const size_t cs = transport.chunk_size();
    auto wire = encode_frame(control_byte, m_channel.channel_id,
                             payload, payload_len, cs);
    for (size_t off = 0; off < wire.size(); off += cs) {
      transport.write_chunk(wire.data() + off, cs);
    }
  }

  Frame ProtocolV2::recv_frame(Transport &transport, const Deadline &deadline)
  {
    FrameAssembler asm_;
    const size_t cs = transport.chunk_size();
    std::vector<uint8_t> chunk(cs);

    // Encrypted-transport read loop. Unlike the allocation probe above,
    // nothing is discarded here: the channel is established, so a chunk that
    // does not parse is a genuine fault and must abort the exchange.
    while (true) {
      const unsigned int left = remaining_ms(deadline);
      if (left == 0) {
        throw exc::TimeoutException("THP: timed out waiting for a device frame");
      }
      const size_t got = transport.read_chunk(chunk.data(), cs, left);
      if (got == 0) {
        throw exc::CommunicationException("THP: zero-length chunk in recv_frame");
      }
      // Some carriers deliver chunks shorter than cs; FrameAssembler ends
      // the frame on the declared length rather than on the chunk count.
      if (asm_.feed_chunk(chunk.data(), got)) {
        return asm_.take();
      }
    }
  }

  void ProtocolV2::set_host_static_key(const HostStaticKey &key)
  {
    m_host_static = key;
    m_have_host_static = true;
  }

  void ProtocolV2::set_known_devices(std::vector<KnownDevice> known)
  {
    m_known_devices = std::move(known);
  }

  void ProtocolV2::adopt_allocation(const AllocatedChannel &allocation)
  {
    m_channel         = allocation;
    m_have_allocation = true;
  }

  void ProtocolV2::session_begin(Transport &transport) {
    if (m_session_open) {
      return;
    }
    if (!m_have_host_static) {
      // Without a persisted key the unpaired (TOFU) flow still has to run;
      // the device-glue layer persists this one and passes it back via
      // set_host_static_key on the next session.
      MINFO("THP: no persisted host static key; generating a fresh one (TOFU)");
      noise_crypto::x25519_keypair(m_host_static.pub, m_host_static.priv);
      m_have_host_static = true;
    }

    // 1. Channel allocation on the broadcast CID, unless the auto-detect
    //    layer already did it as part of the probe.
    if (!m_have_allocation) {
      if (allocate_channel(transport, m_channel) != AllocationResult::allocated) {
        throw exc::TimeoutException("THP: no channel allocation response from device");
      }
    }
    MTRACE("THP: allocated channel " << m_channel.channel_id);

    // 2. Wire device properties + persisted credentials into the handshake.
    m_handshake.set_device_properties(m_channel.device_properties_pb.data(),
                                      m_channel.device_properties_pb.size());
    m_handshake.set_host_static_key(m_host_static);
    m_handshake.set_known_devices(std::move(m_known_devices));

    // Per THP spec (docs/common/thp/specification.md, "Transport packet
    // structure" table):
    //  - DATA-packet type mask is 0xE7. Bit 4 (0x10) is the sequence bit;
    //    bit 3 (0x08) is the optional piggyback-ACK bit. Both must be
    //    masked off when matching the message type.
    //  - ACK packets are matched via Frame::is_ack() below; the 0xF7 ACK
    //    type-mask is encoded inside that helper, so it is not needed
    //    as a local constant here.
    constexpr uint8_t MSG_TYPE_MASK = CTRL_DATA_MASK; // 0xE7
    auto hex2 = [](uint8_t b){ char s[5]; std::snprintf(s, sizeof(s), "%02x", b); return std::string(s); };

    // The handshake never waits on the user, so the whole of it shares one
    // budget.
    const Deadline deadline = deadline_in(HANDSHAKE_TIMEOUT_MS);

    auto recv_specific = [&](uint8_t want_base, const char *what) -> Frame {
      unsigned int frames = 0;
      while (true) {
        if (frames++ >= MAX_FRAMES_PER_EXCHANGE) {
          throw exc::ProtocolException(std::string("THP: too many unexpected frames waiting for ") + what);
        }
        Frame fr = recv_frame(transport, deadline);
        MTRACE("THP: rx ctrl=0x" << hex2(fr.control_byte)
               << " cid=0x" << std::hex << fr.channel_id << std::dec
               << " len=" << fr.payload.size()
               << " (waiting for " << what << ")");
        if (fr.channel_id != m_channel.channel_id) {
          MDEBUG("THP: dropping frame on unexpected channel " << fr.channel_id);
          continue;
        }
        if ((fr.control_byte & MSG_TYPE_MASK) == (want_base & MSG_TYPE_MASK)) {
          return fr;
        }
        if (fr.is_ack()) {
          MDEBUG("THP: stray ACK 0x" << hex2(fr.control_byte) << " while waiting for " << what);
          continue;
        }
        if (fr.control_byte == CTRL_TRANSPORT_ERROR) {
          throw exc::ProtocolException(std::string("THP: transport error during ") + what);
        }
        throw exc::ProtocolException(std::string("THP: unexpected frame waiting for ") + what +
                                     " (got control 0x" + hex2(fr.control_byte) + ")");
      }
    };

    auto consume_ack = [&](uint8_t want_seq, const char *what) {
      unsigned int frames = 0;
      while (true) {
        if (frames++ >= MAX_FRAMES_PER_EXCHANGE) {
          throw exc::ProtocolException(std::string("THP: too many unexpected frames waiting for ACK after ") + what);
        }
        Frame fr = recv_frame(transport, deadline);
        MTRACE("THP: rx ctrl=0x" << hex2(fr.control_byte)
               << " cid=0x" << std::hex << fr.channel_id << std::dec
               << " (waiting for ACK seq=" << int(want_seq) << " after " << what << ")");
        if (fr.channel_id != m_channel.channel_id) {
          continue;
        }
        // ACK match must check is_ack() (under CTRL_ACK_MASK 0xF7, which
        // strips bit 3 / the seq bit) AND then read the seq bit from the
        // raw control byte. Comparing the masked value to an unmasked
        // CTRL_ACK_SEQ1 (=0x28) would always fail because the mask clears
        // the seq bit on the left side of the equality.
        if (fr.is_ack()) {
          if (fr.ack_seq_bit() == want_seq) {
            return;
          }
          MDEBUG("THP: ignoring ACK 0x" << hex2(fr.control_byte) << " (wrong seq) after " << what);
          continue;
        }
        throw exc::ProtocolException(std::string("THP: expected ACK after ") + what +
                                     " (got control 0x" + hex2(fr.control_byte) + ")");
      }
    };

    MINFO("THP: starting handshake on channel " << m_channel.channel_id);

    // 3. host -> device: HandshakeInitiationRequest (base 0x00, seq=0).
    auto init_req = m_handshake.build_init_request(/*try_to_unlock=*/false);
    MTRACE("THP: tx HandshakeInitRequest len=" << init_req.size());
    send_frame(transport, CTRL_HANDSHAKE_INIT_REQ,
               init_req.data(), init_req.size());
    // 3a. device ACKs the request (seq=0).
    consume_ack(0, "HandshakeInitRequest");

    // 4. device -> host: HandshakeInitiationResponse (base 0x01, device seq=0).
    Frame init_resp = recv_specific(CTRL_HANDSHAKE_INIT_RESP, "HandshakeInitResponse");
    m_handshake.consume_init_response(init_resp.payload.data(),
                                      init_resp.payload.size());
    // 4a. host ACKs the response (seq=0).
    send_frame(transport, CTRL_ACK_SEQ0, nullptr, 0);

    // 5. host -> device: HandshakeCompletionRequest (base 0x02, seq=1, wire 0x12).
    //    Per THP spec, the data-packet sequence bit lives at 0x10, not 0x08.
    //    Bit 0x08 is the optional piggyback-ACK bit and must remain 0 here
    //    (host has already sent a standalone ACK_SEQ0 for the init response).
    auto comp_req = m_handshake.build_completion_request();
    MTRACE("THP: tx HandshakeCompletionRequest len=" << comp_req.size() << " ctrl=0x12");
    send_frame(transport, CTRL_HANDSHAKE_COMP_REQ | CTRL_DATA_SEQ_BIT,
               comp_req.data(), comp_req.size());
    // 5a. device ACKs (seq=1).
    consume_ack(1, "HandshakeCompletionRequest");

    // 6. device -> host: HandshakeCompletionResponse (base 0x03, device seq=1, wire 0x13).
    Frame comp_resp = recv_specific(CTRL_HANDSHAKE_COMP_RESP, "HandshakeCompletionResponse");
    m_handshake.consume_completion_response(comp_resp.payload.data(),
                                            comp_resp.payload.size());
    // 6a. host ACKs (seq=1).
    send_frame(transport, CTRL_ACK_SEQ1, nullptr, 0);

    MINFO("THP: handshake complete on channel " << m_channel.channel_id);

    // If consume_init_response matched a previously-paired device, the
    // handshake's internal host_static was swapped to the credential
    // we'd persisted for that device. Pull that authoritative copy back
    // up so host_static() returns the correct keypair for the duration
    // of this session.
    m_host_static = m_handshake.host_static_in_use();

    // 7. Build cipherstates per specification.md (encryption_state: nonce
    //    counters start at 0 for outgoing requests and 1 for incoming
    //    responses, since the HandshakeCompletionResponse already consumed
    //    the response counter at 0).
    const HandshakeKeys &keys = m_handshake.keys();
    m_send_cipher = std::make_unique<TransportCipher>(keys.key_request,  /*nonce=*/0);
    m_recv_cipher = std::make_unique<TransportCipher>(keys.key_response, /*nonce=*/1);
    m_send_seq = 0;
    m_recv_seq = 0;
    m_have_pending_frame = false;
    m_pending_frame = Frame{};

    if (!m_handshake.is_known_device() &&
        m_handshake.trezor_state() == STATE_UNPAIRED) {
      // First-time pairing is required to upgrade to an authenticated
      // session.  The caller (ProtocolAutoDetect) drives the CodeEntry
      // pairing FSM right after this returns; until that succeeds the
      // channel must be treated as unauthenticated.
      MINFO("THP: device is unpaired; CodeEntry pairing follows");
    }
    m_session_open = true;
  }

  void ProtocolV2::session_end(Transport & /*transport*/) {
    m_send_cipher.reset();
    m_recv_cipher.reset();
    m_have_pending_frame = false;
    m_pending_frame = Frame{};
    m_session_open = false;
  }

  void ProtocolV2::write(Transport &transport,
                         const google::protobuf::Message &req) {
    if (!m_session_open || !m_send_cipher) {
      throw exc::ProtocolException("THP: write before session_begin");
    }
    if (m_have_pending_frame) {
      // The caller issued a new request without reading the previous
      // response; the parked frame is now stale.
      MDEBUG("THP: discarding unconsumed response frame on new write");
      m_pending_frame      = Frame{};
      m_have_pending_frame = false;
    }

    // Plaintext layout for THP application messages (matches the canonical
    // Python `trezorlib.thp.client` HEADER_FMT = ">BH"):
    //   byte 0    : session_id (1 byte). 0x00 is the implicit seedless
    //                                    management session (pairing /
    //                                    credential / GetFeatures);
    //                                    application traffic that needs seed
    //                                    access must run on a session
    //                                    allocated via ThpCreateNewSession.
    //   bytes 1-2 : message_type wire number (uint16, big-endian)
    //   bytes 3.. : protobuf-serialized payload
    constexpr size_t  PLAIN_HEADER_LEN      = 3;
    uint16_t wire_num = MessageMapper::get_message_wire_number(req);
#if GOOGLE_PROTOBUF_VERSION < 3006001
    size_t msg_size = req.ByteSize();
#else
    size_t msg_size = req.ByteSizeLong();
#endif

    // V1-vs-V2 message translation. Monero's device_trezor_base unconditionally
    // sends a management::Initialize (msg_type 0) at the start of every command
    // chain. THP devices reject Initialize as Failure_UnexpectedMessage; the
    // canonical trezorlib client uses GetFeatures (msg_type 55) instead. The
    // Initialize.session_id field is meaningless under THP (sessions are
    // multiplexed via the encrypted plaintext header byte, not in-protobuf),
    // so we drop the body and send an empty GetFeatures.
    bool translated_initialize = false;
    if (wire_num == static_cast<uint16_t>(messages::MessageType_Initialize)) {
      wire_num = static_cast<uint16_t>(messages::MessageType_GetFeatures);
      msg_size = 0; // GetFeatures has no fields
      translated_initialize = true;
    }

    std::vector<uint8_t> plain;
    plain.resize(PLAIN_HEADER_LEN + msg_size);
    plain[0] = m_session_id;
    write_be16(plain.data() + 1, wire_num);
    if (!translated_initialize && msg_size > 0) {
      if (!req.SerializeToArray(plain.data() + PLAIN_HEADER_LEN, msg_size)) {
        throw exc::EncodingException("THP: protobuf serialize failed");
      }
    }

    std::vector<uint8_t> sealed;
    m_send_cipher->seal(/*aad=*/nullptr, /*aad_len=*/0,
                        plain.data(), plain.size(), sealed);

    const uint8_t control = CTRL_ENCRYPTED_TRANSPORT |
                            (m_send_seq ? CTRL_DATA_SEQ_BIT : 0);
    send_frame(transport, control, sealed.data(), sealed.size());

    // Wait for the device to acknowledge the alternating bit.  USB does
    // not reorder, but ACKs can be duplicated or piggybacked, so accept
    // every spec-legal way the device can signal receipt instead of
    // demanding the very next frame be the exact standalone ACK:
    //   - skip frames addressed to other channels,
    //   - skip duplicate ACKs bearing the previous sequence bit,
    //   - re-ACK and drop a retransmission of the response we already
    //     consumed (our earlier ACK may have been lost),
    //   - treat a response data frame as implicit acknowledgement (the
    //     spec allows piggybacking the ACK bit on data; and a response
    //     proves the request was received either way) and park it for
    //     the next read().
    // Host-side retransmission-on-timeout is not implemented; like the
    // pre-existing v1 protocol we rely on the carrier being reliable.
    const Deadline deadline = deadline_in(EXCHANGE_TIMEOUT_MS);
    unsigned int frames = 0;
    while (true) {
      if (frames++ >= MAX_FRAMES_PER_EXCHANGE) {
        throw exc::ProtocolException("THP: too many unexpected frames while waiting for write ACK");
      }
      Frame fr = recv_frame(transport, deadline);
      if (fr.channel_id != m_channel.channel_id) {
        MDEBUG("THP: dropping frame on unexpected channel " << fr.channel_id
               << " while waiting for write ACK");
        continue;
      }
      if (fr.is_ack()) {
        if (fr.ack_seq_bit() == m_send_seq) {
          break;
        }
        MDEBUG("THP: ignoring duplicate ACK (seq " << int(fr.ack_seq_bit())
               << ") while waiting for write ACK");
        continue;
      }
      if ((fr.control_byte & CTRL_DATA_MASK) == CTRL_ENCRYPTED_TRANSPORT) {
        if (fr.data_seq_bit() != m_recv_seq) {
          // Retransmission of the previous response: our ACK for it was
          // lost.  Re-ACK with the frame's own sequence bit and keep
          // waiting.
          MDEBUG("THP: re-ACKing retransmitted response (seq "
                 << int(fr.data_seq_bit()) << ") while waiting for write ACK");
          send_frame(transport,
                     fr.data_seq_bit() ? CTRL_ACK_SEQ1 : CTRL_ACK_SEQ0,
                     nullptr, 0);
          continue;
        }
        // The device's response to the request we just sent is an implicit
        // acknowledgement.  Hand it to the next read().
        m_pending_frame      = std::move(fr);
        m_have_pending_frame = true;
        break;
      }
      if (fr.control_byte == CTRL_TRANSPORT_ERROR) {
        throw exc::ProtocolException("THP: transport error while waiting for write ACK");
      }
      throw exc::ProtocolException("THP: missing or mismatched ACK after write");
    }
    m_send_seq ^= 1;
  }

  void ProtocolV2::read(Transport &transport,
                        std::shared_ptr<google::protobuf::Message> &msg,
                        messages::MessageType *msg_type) {
    if (!m_session_open || !m_recv_cipher) {
      throw exc::ProtocolException("THP: read before session_begin");
    }

    // A write() may have parked the device's response here when the ACK
    // was piggybacked on it; otherwise read from the wire, tolerating
    // duplicated ACKs and retransmitted frames (re-ACK and drop) instead
    // of aborting the session mid-operation.
    Frame f;
    if (m_have_pending_frame) {
      f = std::move(m_pending_frame);
      m_pending_frame      = Frame{};
      m_have_pending_frame = false;
    } else {
      const Deadline deadline = deadline_in(EXCHANGE_TIMEOUT_MS);
      unsigned int frames = 0;
      while (true) {
        if (frames++ >= MAX_FRAMES_PER_EXCHANGE) {
          throw exc::ProtocolException("THP: too many unexpected frames during read");
        }
        f = recv_frame(transport, deadline);
        if (f.channel_id != m_channel.channel_id) {
          MDEBUG("THP: dropping frame on unexpected channel " << f.channel_id
                 << " during read");
          continue;
        }
        if (f.is_ack()) {
          MDEBUG("THP: ignoring stray ACK during read");
          continue;
        }
        // Match encrypted_transport under the spec's DATA_MASK (0xE7),
        // which strips both the seq bit (0x10) and the optional
        // piggyback-ACK bit (0x08). Using 0xF7 here would reject every
        // frame with seq=1.
        if ((f.control_byte & CTRL_DATA_MASK) != CTRL_ENCRYPTED_TRANSPORT) {
          throw exc::ProtocolException("THP: read got unexpected frame type");
        }
        if (f.data_seq_bit() != m_recv_seq) {
          // Retransmission of the previous frame: the device did not see
          // our ACK.  Per the alternating-bit protocol, re-ACK it and
          // keep waiting for the frame we actually expect.
          MDEBUG("THP: re-ACKing retransmitted frame (seq "
                 << int(f.data_seq_bit()) << ") during read");
          send_frame(transport,
                     f.data_seq_bit() ? CTRL_ACK_SEQ1 : CTRL_ACK_SEQ0,
                     nullptr, 0);
          continue;
        }
        break;
      }
    }

    std::vector<uint8_t> plain;
    if (!m_recv_cipher->open(/*aad=*/nullptr, /*aad_len=*/0,
                             f.payload.data(), f.payload.size(), plain)) {
      throw exc::SecurityException("THP: AES-GCM decrypt failed");
    }
    // Plaintext: session_id(1) || msg_type(2 BE) || protobuf payload.
    constexpr size_t PLAIN_HEADER_LEN = 3;
    if (plain.size() < PLAIN_HEADER_LEN) {
      throw exc::ProtocolException("THP: decrypted payload too small");
    }
    // The device mirrors the request's session id; a mismatch would mean
    // a reply routed from a different session.  Warn rather than throw:
    // the host drives strictly one request/response pair at a time, and
    // the message itself authenticated under the channel keys.
    if (plain[0] != m_session_id) {
      MWARNING("THP: response carries session id " << int(plain[0])
               << " but session " << int(m_session_id) << " is active");
    }
    const uint16_t wire_num = read_be16(plain.data() + 1);

    std::shared_ptr<google::protobuf::Message> wrap(MessageMapper::get_message(wire_num));
    if (!wrap->ParseFromArray(plain.data() + PLAIN_HEADER_LEN,
                              plain.size() - PLAIN_HEADER_LEN)) {
      throw exc::EncodingException("THP: protobuf parse failed");
    }
    msg = wrap;
    if (msg_type) {
      *msg_type = static_cast<messages::MessageType>(wire_num);
    }

    // ACK the frame.
    const uint8_t ack_ctrl = m_recv_seq ? CTRL_ACK_SEQ1 : CTRL_ACK_SEQ0;
    send_frame(transport, ack_ctrl, nullptr, 0);
    m_recv_seq ^= 1;
  }

}}}
