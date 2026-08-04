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

#ifndef MONERO_TREZOR_THP_PROTOCOL_V2_H
#define MONERO_TREZOR_THP_PROTOCOL_V2_H

#include "device_trezor/trezor/transport.hpp"
#include "framing.hpp"
#include "noise.hpp"

#include <chrono>
#include <memory>

namespace hw {
namespace trezor {
namespace thp {

  // Trezor's reported state byte from HandshakeCompletionResponse.
  // Per specification.md these are the only documented values.
  constexpr uint8_t STATE_UNPAIRED            = 0x00;
  constexpr uint8_t STATE_PAIRED              = 0x01;
  constexpr uint8_t STATE_PAIRED_AUTOCONNECT  = 0x02;

  // Budget for a whole channel allocation, in milliseconds. A Safe 7
  // answers in milliseconds; a protocol-v1 device never answers at all,
  // and the caller falls back to ProtocolV1 once this expires.
  constexpr unsigned int ALLOC_TIMEOUT_MS = 3000;

  // Information returned by the device in a ChannelAllocationResponse.
  // device_properties_pb is an opaque protobuf payload (DeviceProperties
  // from trezor-firmware) that the higher protocol layer decodes.
  struct AllocatedChannel {
    uint16_t channel_id = 0;      // allocated CID, in 0x0001..0xFFEF
    std::vector<uint8_t> nonce;   // 8-byte nonce echoed by the device
    std::vector<uint8_t> device_properties_pb;
  };

  // Outcome of a channel allocation attempt.
  enum class AllocationResult {
    allocated,
    no_response,    // the budget expired without a usable response
    codec_v1_reply, // the device answered with a codec_v1 report
  };

  // Allocate a new channel on the broadcast CID 0xFFFF: send a
  // ChannelAllocationRequest carrying a random 8-byte nonce and wait for the
  // ChannelAllocationResponse that echoes it back with a CID.
  //
  // timeout_ms bounds the whole operation rather than each read, so a device
  // that answers promptly with unusable data cannot keep the call alive.
  // `out` is assigned only when the result is AllocationResult::allocated;
  // transport I/O failures propagate as exceptions.
  //
  // AllocationResult::codec_v1_reply means the device answered with the
  // codec_v1 magic '?','#','#', which only a protocol-v1 device (Model T /
  // Safe 3 / Safe 5) emits. Recognising it lets the caller select ProtocolV1
  // immediately instead of waiting out the budget.
  AllocationResult allocate_channel(Transport &transport, AllocatedChannel &out,
                                    unsigned int timeout_ms = ALLOC_TIMEOUT_MS);

  // ProtocolV2 implements the Protocol interface (write/read of protobuf
  // messages) on top of the THP wire format. Unlike ProtocolV1 it is
  // stateful: a single instance is bound to one device session for the
  // lifetime of an open connection.
  //
  // Lifecycle:
  //   1. set_host_static_key() / set_known_devices()  (before session_begin)
  //   2. session_begin(transport) -> alloc, handshake
  //   3. caller drives pairing FSM (if unpaired) and ThpEndRequest exchange
  //      using write/read on the implicit session 0 (pairing/credential phase)
  //   4. caller calls set_session_id(1), sends ThpCreateNewSession via write,
  //      reads Success, promoting the channel to a seeded application session
  //   5. write/read pairs        -> AES-GCM-sealed protobuf exchange on sid=1
  //   6. session_end(transport)
  class ProtocolV2 : public Protocol {
  public:
    ProtocolV2();

    // Provide the host's persistent X25519 keypair (loaded from disk or
    // freshly generated). MUST be called before session_begin.
    void set_host_static_key(const HostStaticKey &key);

    // Provide previously-paired devices. May be empty for first-time pairing.
    void set_known_devices(std::vector<KnownDevice> known);

    // Skip the channel allocation step on session_begin; reuse the
    // allocation that the probe already obtained.  Used by
    // ProtocolAutoDetect; callers that go straight to ProtocolV2 should
    // not need this.
    void adopt_allocation(const AllocatedChannel &allocation);

    // Read-only access to the host static keypair currently in use
    // (either the value passed to set_host_static_key or, in TOFU mode,
    // a freshly-generated one).
    const HostStaticKey &host_static() const { return m_host_static; }

    void session_begin(Transport &transport) override;
    void session_end  (Transport &transport) override;

    void write(Transport &transport,
               const google::protobuf::Message &req) override;
    void read (Transport &transport,
               std::shared_ptr<google::protobuf::Message> &msg,
               messages::MessageType *msg_type = nullptr) override;

    // Per THP application-layer sessions.md: after ThpEndResponse the channel
    // is in encrypted-transport state with only an implicit seedless management
    // session at session_id=0. Application traffic that needs seed derivation
    // must be sent on a session allocated via ThpCreateNewSession. The
    // session_id is the byte the host writes into the encrypted-transport
    // plaintext header (struct ">BH": session_id, msg_type). Callers (auto-
    // detect) flip this from 0 to 1 after the ThpEndRequest exchange and
    // before sending ThpCreateNewSession itself, so that the firmware's
    // SeedlessSessionContext for session_id=1 is the one that runs the
    // ThpCreateNewSession handler.
    void set_session_id(uint8_t sid) { m_session_id = sid; }
    uint8_t session_id() const { return m_session_id; }

    // For diagnostics / pairing UX.
    uint16_t                      channel_id()             const { return m_channel.channel_id; }
    const std::vector<uint8_t>   &device_properties()      const { return m_channel.device_properties_pb; }
    const NoisePubKey            &trezor_masked_static()   const { return m_handshake.trezor_masked_static_pubkey(); }
    uint8_t                       trezor_state()           const { return m_handshake.trezor_state(); }
    bool                          is_known_device()        const { return m_handshake.is_known_device(); }
    const NoiseHash              &handshake_hash()         const { return m_handshake.keys().handshake_hash; }

  private:
    // Encode one transport-layer frame and write it to the wire in
    // chunk_size()-sized pieces. Pure output: the caller is responsible
    // for setting any sequence bit in control_byte and for the ACK
    // exchange that may follow (see write()).
    void send_frame(Transport &transport, uint8_t control_byte,
                    const uint8_t *payload, size_t payload_len);

    // Read one transport-layer frame, giving up at `deadline`. The payload
    // has already been CRC-validated by the assembler.
    Frame recv_frame(Transport &transport,
                     const std::chrono::steady_clock::time_point &deadline);

    AllocatedChannel  m_channel;
    bool              m_have_allocation = false;
    NoiseXxInitiator  m_handshake;
    HostStaticKey     m_host_static{};
    bool              m_have_host_static = false;
    std::vector<KnownDevice> m_known_devices;
    std::unique_ptr<TransportCipher> m_send_cipher;
    std::unique_ptr<TransportCipher> m_recv_cipher;
    // Alternating-bit sequence counters for the encrypted-transport phase.
    // Both start at 0 because by the time encrypted transport begins, the
    // handshake has already toggled sync_bit_send twice (init_req to 0,
    // comp_req to 1) and sync_bit_receive twice (init_resp to 0, comp_resp
    // to 1), bringing both back to 0. See THP spec, "Alternating Bit
    // Protocol".
    uint8_t           m_send_seq = 0;
    uint8_t           m_recv_seq = 0;
    // One-frame lookahead for the encrypted-transport phase: the device
    // may acknowledge our last write by piggybacking the ACK bit on its
    // response data frame, or hand us the response while we still wait
    // for the standalone ACK (our request was clearly received either
    // way).  write() parks such a frame here and read() consumes it
    // before touching the wire.
    bool              m_have_pending_frame = false;
    Frame             m_pending_frame;
    bool              m_session_open = false;
    // THP application session identifier. 0x00 is the seedless management
    // session used for pairing-context and management messages only.
    // ProtocolAutoDetect promotes this to 0x01 via set_session_id() before
    // sending ThpCreateNewSession; afterwards write() embeds it in every
    // encrypted plaintext header so the device routes the message to the
    // correct seed-derived wallet session.
    uint8_t           m_session_id = 0x00;
  };

}}}

#endif // MONERO_TREZOR_THP_PROTOCOL_V2_H
