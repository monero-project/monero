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

// Unit tests for the Trezor Host Protocol v2 (THP) data-transfer and
// secure-channel implementations.  Most tests in this file are entirely
// deterministic; the handful that draw from the system RNG are noted where
// they do.  None require an attached Trezor or trezor-emu instance.  They
// cover:
//
//   - CRC-32 / framing round-trip (encode_frame -> FrameAssembler)
//   - CRC mismatch detection (tampered byte triggers an error)
//   - Reassembly across exactly-chunk-size boundaries
//   - Cryptographic primitives against published RFC / NIST vectors
//   - The Noise XX state machine, exercised by playing both the host and
//     the (simulated) Trezor sides of the handshake described in
//     specification.md.  This proves the handshake's symbolic correctness
//     end-to-end without requiring hardware.
//   - ProtocolV2's alternating-bit sequencing and ProtocolAutoDetect's
//     v1/v2 selection, driven through a scripted Transport.

#include "gtest/gtest.h"

#if defined(DEVICE_TREZOR_READY)

#include "device_trezor/trezor/exceptions.hpp"
#include "device_trezor/trezor/thp/auto_detect.hpp"
#include "device_trezor/trezor/thp/framing.hpp"
#include "device_trezor/trezor/thp/noise.hpp"
#include "device_trezor/trezor/thp/pairing.hpp"
#include "device_trezor/trezor/thp/protocol_v2.hpp"
#include "device_trezor/trezor/thp/store.hpp"
#include "device_trezor/trezor/transport.hpp"

#include <boost/crc.hpp>
#include <boost/filesystem.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace hw::trezor::thp;

// CRC-32/ISO-HDLC, the checksum a THP frame carries.  framing.cpp feeds
// boost::crc_32_type incrementally to avoid copying header and payload
// together; this one-shot form keeps the published vectors below readable.
static uint32_t crc32_ieee(const void *data, size_t len) {
  boost::crc_32_type crc;
  if (len) crc.process_bytes(data, len);
  return crc.checksum();
}

static std::vector<uint8_t> from_hex(const std::string &s) {
  std::vector<uint8_t> out;
  for (size_t i = 0; i + 1 < s.size(); i += 2) {
    auto h = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return 0;
    };
    out.push_back(uint8_t((h(s[i]) << 4) | h(s[i+1])));
  }
  return out;
}

// ---------------------------------------------------------------------------
// CRC-32 (IEEE). The classic verification vector is CRC32("123456789")
// = 0xCBF43926 for the IEEE polynomial / left-shift convention; boost::crc
// uses the reflected form, which yields the same final checksum.
// ---------------------------------------------------------------------------
TEST(thp_crc32, check_vector_123456789)
{
  EXPECT_EQ(0xCBF43926u, crc32_ieee("123456789", 9));
}

// Mirrors trezor-firmware's THP checksum vectors, so our CRC matches the
// device's expectation bit-for-bit, including the empty input.  Source:
// trezor-firmware@bc97a6b2b00068cb36a13da27ddaba8306314870,
// core/tests/test_trezor.wire.thp.checksum.py.  That revision is pinned
// because 00ec9a7f8b6388f45e7825f48d6cb5b42d27d795 ("feat(core): switch
// from Python THP implementation to Rust-based one") deleted the file and
// the Rust replacement ships no vector tables.
TEST(thp_crc32, trezor_firmware_checksum_vectors)
{
  EXPECT_EQ(0x00000000u, crc32_ieee("", 0));
  EXPECT_EQ(0xE8B7BE43u, crc32_ieee("a", 1));
  EXPECT_EQ(0x352441C2u, crc32_ieee("abc", 3));
  const std::vector<uint8_t> zeros32(32, 0x00);
  EXPECT_EQ(0x190A55ADu, crc32_ieee(zeros32.data(), zeros32.size()));
  const char digits80[] =
      "12345678901234567890123456789012345678901234567890"
      "123456789012345678901234567890";
  EXPECT_EQ(0x7CA94A72u, crc32_ieee(digits80, 80));
  const char various[] = "various CRC algorithms input data";
  EXPECT_EQ(0x9BD366AEu, crc32_ieee(various, 33));
}

// ---------------------------------------------------------------------------
// Framing round-trip and chunk layout, across the sizes that matter.
//
// One table replaces the seven near-identical round-trip tests this file
// used to carry.  Each row encodes a payload, feeds the wire bytes back
// through the assembler one chunk at a time, and checks both the recovered
// frame and the per-chunk layout: whole chunks out, completion only on the
// final chunk, and a continuation control byte of exactly 0x80 carrying the
// channel id.
//
// Per the THP spec ("Transport packet structure") a continuation packet's
// control byte is 0x80 exactly, every other bit zero.  We once emitted
// (initiation_ctrl | 0x80), which bled the message-type and sequence bits
// into the continuation header and a strict device parser rejects it; the
// rows with 0x14 (encrypted transport, seq=1) would catch that again.
// ---------------------------------------------------------------------------
TEST(thp_framing, round_trip_and_chunk_layout)
{
  struct Case {
    const char *name;
    size_t      chunk_size;
    size_t      payload_len;
    uint8_t     control_byte;
    uint16_t    channel_id;
  };
  static const Case cases[] = {
    { "short payload, one chunk",        64,   9, 0x40, 0xFFFF },
    // 55 payload bytes plus the 4-byte CRC trailer is exactly 64 - 5, the
    // largest body that still fits the initiation chunk; 56 is the first
    // that does not, which is the off-by-one most likely to break the
    // assembler.
    { "exact one-chunk fit",             64,  55, 0x04, 0x4321 },
    { "one byte past the one-chunk fit", 64,  56, 0x04, 0x4321 },
    { "three chunks",                    64, 200, 0x04, 0x1234 },
    { "sequence bit set on initiation",  64, 180, 0x14, 0x0042 },
    // The framing layer is parameterised on the carrier's packet size; one
    // non-default row keeps that parameterisation exercised.
    { "non-default chunk size",         244, 600, 0x14, 0x4242 },
  };

  for (const Case &c : cases) {
    SCOPED_TRACE(c.name);

    std::vector<uint8_t> payload(c.payload_len);
    for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t(i ^ 0x5A);

    const auto wire = encode_frame(c.control_byte, c.channel_id,
                                   payload.data(), payload.size(),
                                   c.chunk_size);
    ASSERT_EQ(0u, wire.size() % c.chunk_size)
        << "encode_frame must emit a whole number of chunks";
    ASSERT_GE(wire.size(), c.chunk_size);
    EXPECT_EQ(c.control_byte, wire[0]);

    FrameAssembler asm_;
    bool done = false;
    for (size_t off = 0; off < wire.size(); off += c.chunk_size) {
      if (off > 0) {
        EXPECT_EQ(0x80, wire[off]) << "continuation chunk at offset " << off;
        EXPECT_EQ(c.channel_id, read_be16(wire.data() + off + 1));
      }
      done = asm_.feed_chunk(wire.data() + off, c.chunk_size);
      EXPECT_EQ(off + c.chunk_size == wire.size(), done)
          << "frame completed at the wrong chunk (offset " << off << ")";
    }
    ASSERT_TRUE(done);

    Frame f = asm_.take();
    EXPECT_EQ(c.control_byte, f.control_byte);
    EXPECT_EQ(c.channel_id,   f.channel_id);
    ASSERT_EQ(payload.size(), f.payload.size());
    EXPECT_EQ(0, std::memcmp(f.payload.data(), payload.data(), payload.size()));
  }
}

// Byte-exact cross-implementation pin: trezor-firmware's writer fixture for
// an empty encrypted payload (empty_payload_with_checksum_expected: ctrl
// 0x04, cid 0x1234, 64-byte packets) in
// trezor-firmware@bc97a6b2b00068cb36a13da27ddaba8306314870,
// core/tests/test_trezor.wire.thp.writer.py.  Jointly locks the header
// layout, the length-includes-CRC rule, the CRC-over-header semantics, and
// the zero padding to the chunk size.
// (The fixtures for longer payloads in that file are built with hand-rolled
// headers whose length field excludes the CRC, so they don't represent
// protocol-correct frames and are deliberately not mirrored.)
TEST(thp_framing, wire_bytes_match_trezor_firmware_empty_payload_fixture)
{
  static const uint8_t no_payload[1] = {0};
  const auto wire = encode_frame(0x04, 0x1234, no_payload, 0, 64);
  const auto expected = from_hex(
      "0412340004edbd479c0000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000");
  ASSERT_EQ(expected.size(), wire.size());
  EXPECT_EQ(expected, wire);
}

// Continuation layout for a payload that spans five chunks, asserted
// byte-by-byte against a hand-built expectation rather than by re-running
// the encoder.  This is the layout trezor-firmware's writer fixture for a
// 256-byte payload exhibits at the pinned revision above: control byte
// 0x80, the two-byte channel id, and a body split of 59 bytes in the
// initiation chunk followed by 61 in each continuation.  Those split
// offsets are the part of the format the assembler and the device must
// agree on exactly; everything else about a continuation is header.
TEST(thp_framing, multi_chunk_continuation_layout)
{
  std::vector<uint8_t> payload(256);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t(i);

  const auto wire = encode_frame(0x04, 0x1234, payload.data(), payload.size());
  // 256 payload + 4 CRC = 260 body bytes: 59 in the initiation chunk, 61 in
  // each of three full continuations, and the remaining 18 in a fifth.
  ASSERT_EQ(USB_CHUNK_SIZE * 5, wire.size());

  // Initiation header: control, channel id, declared length (incl. CRC).
  EXPECT_EQ(0x04,   wire[0]);
  EXPECT_EQ(0x1234, read_be16(wire.data() + 1));
  EXPECT_EQ(260,    read_be16(wire.data() + 3));

  for (size_t n = 1; n <= 3; ++n) {
    SCOPED_TRACE(n);
    std::vector<uint8_t> expected{0x80, 0x12, 0x34};
    const size_t from = 59 + 61 * (n - 1);
    expected.insert(expected.end(), payload.begin() + from,
                    payload.begin() + from + 61);
    const std::vector<uint8_t> got(wire.begin() + n * USB_CHUNK_SIZE,
                                   wire.begin() + (n + 1) * USB_CHUNK_SIZE);
    EXPECT_EQ(expected, got);
  }
}

TEST(thp_framing, crc_mismatch_throws)
{
  const uint8_t payload[] = "abcdefghij";
  auto wire = encode_frame(0x40, 0xFFFF, payload, sizeof(payload) - 1);

  // Flip a payload byte (offset 5 = first payload byte after the
  // initiation header).
  wire[5] ^= 0x01;

  FrameAssembler asm_;
  EXPECT_THROW(asm_.feed_chunk(wire.data(), USB_CHUNK_SIZE),
               hw::trezor::exc::CommunicationException);
}

// Regression: the data-packet sequence bit lives at 0x10, not 0x08.
//
// THP spec, "Transport packet structure", defines DATA packets as
// `000XX100` under mask 0xE7. Bit 4 (0x10) is the seq bit; bit 3 (0x08)
// is the optional piggyback-ACK bit; on an ACK packet the seq bit is at
// 0x08 instead. An earlier bug had us OR'ing 0x08 into the control byte
// to mark seq=1, which the device reads as a repeated seq=0 frame plus a
// piggyback-ACK signal, wedging the alternating-bit protocol on every
// multi-frame exchange. HandshakeCompletionRequest is the first frame to
// need seq=1, and with the bug it went out as 0x0A instead of 0x12: real
// Safe 7 firmware silently drops it and the wallet hangs forever on
// "Creating wallet from device...".
//
// This pins the bit positions and the Frame accessors that read them;
// thp_protocol_v2.write_read_control_byte_sequence_matches_spec pins the
// bytes ProtocolV2 actually emits.
TEST(thp_framing, data_and_ack_sequence_bits_have_distinct_positions)
{
  EXPECT_EQ(0x10, CTRL_DATA_SEQ_BIT);
  EXPECT_EQ(0x08, CTRL_DATA_ACK_BIT);
  EXPECT_EQ(0x08, CTRL_ACK_SEQ_BIT);
  EXPECT_EQ(0xE7, CTRL_DATA_MASK);
  EXPECT_EQ(0xF7, CTRL_ACK_MASK);
  EXPECT_EQ(0x12, CTRL_HANDSHAKE_COMP_REQ | CTRL_DATA_SEQ_BIT);

  struct Case { uint8_t control; bool ack; uint8_t base; uint8_t seq; uint8_t pb; };
  static const Case cases[] = {
    // Encrypted transport at seq=0 and seq=1, then with the piggyback-ACK
    // bit also set (0x1C), which must not disturb the type match.
    { CTRL_ENCRYPTED_TRANSPORT,                       false, CTRL_ENCRYPTED_TRANSPORT, 0, 0 },
    { CTRL_ENCRYPTED_TRANSPORT | CTRL_DATA_SEQ_BIT,   false, CTRL_ENCRYPTED_TRANSPORT, 1, 0 },
    { CTRL_ENCRYPTED_TRANSPORT | CTRL_DATA_SEQ_BIT |
      CTRL_DATA_ACK_BIT,                              false, CTRL_ENCRYPTED_TRANSPORT, 1, 1 },
    { CTRL_HANDSHAKE_COMP_REQ  | CTRL_DATA_SEQ_BIT,   false, CTRL_HANDSHAKE_COMP_REQ,  1, 0 },
  };
  for (const Case &c : cases) {
    SCOPED_TRACE(int(c.control));
    Frame f{};
    f.control_byte = c.control;
    EXPECT_EQ(c.ack, f.is_ack());
    EXPECT_EQ(c.base, f.control_byte & CTRL_DATA_MASK);
    EXPECT_EQ(c.seq, f.data_seq_bit());
    EXPECT_EQ(c.pb,  f.data_ack_bit());
  }

  // On ACK packets the sequence bit sits at 0x08, so ACK_SEQ1 is 0x28.
  Frame ack1{}; ack1.control_byte = CTRL_ACK_SEQ1;
  EXPECT_TRUE(ack1.is_ack());
  EXPECT_EQ(1, ack1.ack_seq_bit());
  Frame ack0{}; ack0.control_byte = CTRL_ACK_SEQ0;
  EXPECT_TRUE(ack0.is_ack());
  EXPECT_EQ(0, ack0.ack_seq_bit());
}

// Variable-size final chunk acceptance.  The FrameAssembler must
// tolerate a final chunk smaller than the negotiated transport
// chunk_size: the THP wire format carries a declared length, so the
// assembler completes a frame as soon as that length is covered rather
// than requiring a full padded chunk.  This test locks in that
// length-tolerant contract.
TEST(thp_framing, variable_size_final_chunk_accepted)
{
  // Build a frame that spans 2 full chunks plus a partial final
  // chunk.  244 - 5 = 239 bytes in chunk 0; 244 - 3 = 241 bytes per
  // continuation.  Total payload = 239 + 241 + 50 = 530 bytes.
  std::vector<uint8_t> payload(530);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t((i * 7) ^ 0x5A);

  auto wire = encode_frame(0x04, 0xBEEF, payload.data(), payload.size(),
                           /*chunk_size=*/244);
  // encode_frame produces fully-padded chunks; verify our test setup.
  ASSERT_EQ(0u, wire.size() % 244);
  ASSERT_EQ(244u * 3, wire.size());

  FrameAssembler asm_;
  EXPECT_FALSE(asm_.feed_chunk(wire.data() + 0,   244));
  EXPECT_FALSE(asm_.feed_chunk(wire.data() + 244, 244));
  // Trim the final chunk to a SHORT length covering the declared
  // payload + CRC32 (THP length field marks the legal end).  53 bytes
  // = 3-byte continuation header + remaining payload (50) ... 50 +
  // padding past the CRC isn't needed because the assembler should
  // accept whatever covers the declared length.
  const size_t kFinalShortLen = 244 - 100;
  EXPECT_TRUE(asm_.feed_chunk(wire.data() + 488, kFinalShortLen));

  Frame f = asm_.take();
  EXPECT_EQ(0x04, f.control_byte);
  EXPECT_EQ(0xBEEF, f.channel_id);
  ASSERT_EQ(payload.size(), f.payload.size());
  EXPECT_EQ(0, std::memcmp(f.payload.data(), payload.data(), payload.size()));
}

// Runt-chunk regression guard.  Although the FrameAssembler accepts a
// short final chunk (see above), it MUST still reject a continuation
// chunk too small to even hold the 3-byte continuation header.  This is
// the "THP: chunk too small" guard in framing.cpp.  This test admits a
// normal initiation chunk, then feeds a 1-byte continuation chunk
// (control byte only, no room for the 3-byte continuation header) and
// asserts the assembler throws a CommunicationException rather than
// over-reading.
TEST(thp_framing, runt_continuation_chunk_rejected)
{
  std::vector<uint8_t> payload(500, 0x77);
  auto wire = encode_frame(0x04, 0x1234, payload.data(), payload.size(), 244);
  ASSERT_GE(wire.size(), 244u * 2);

  FrameAssembler asm_;
  // First (initiation) chunk admitted normally.
  EXPECT_FALSE(asm_.feed_chunk(wire.data(), 244));

  // A 1-byte continuation should throw: too small to even hold the
  // continuation header (3 bytes).  framing.cpp throws
  // CommunicationException with "THP: chunk too small".
  std::vector<uint8_t> tiny{0x80};  // continuation control byte only
  EXPECT_THROW(asm_.feed_chunk(tiny.data(), tiny.size()),
               hw::trezor::exc::CommunicationException);
}

// Runt-initiation guard.  The initiation header is 5 bytes (ctrl + 2-byte
// cid + 2-byte length).  A 4-byte chunk passes the generic 3-byte "too
// small" guard but would let read_be16(chunk+3) read one byte out of
// bounds, so the initiation path must reject it explicitly.  A 5-byte
// chunk (exactly the header, no payload yet) is the boundary case: it is
// VALID: read_be16(chunk+3) stays in bounds and the declared-length
// logic awaits the continuation, so it must NOT be rejected by the
// too-small guard.  This pins the off-by-one boundary (< not <=).
TEST(thp_framing, runt_initiation_chunk_rejected)
{
  // 4-byte initiation chunk: ctrl=0x04 (initiation), cid=0x1234, then a
  // single length byte, one short of the 2-byte length field.
  std::vector<uint8_t> four{0x04, 0x12, 0x34, 0x00};
  FrameAssembler asm_four;
  EXPECT_THROW(asm_four.feed_chunk(four.data(), four.size()),
               hw::trezor::exc::CommunicationException);

  // 5-byte initiation chunk: full header, declared_len=0x000A (>0, so the
  // payload+CRC arrive in a later chunk).  The assembler must accept it
  // and return false (frame incomplete), NOT throw "initiation chunk too
  // small".
  std::vector<uint8_t> five{0x04, 0x12, 0x34, 0x00, 0x0A};
  FrameAssembler asm_five;
  EXPECT_FALSE(asm_five.feed_chunk(five.data(), five.size()));
}

// ---------------------------------------------------------------------------
// SHA-256 (NIST FIPS 180-4 example).
// ---------------------------------------------------------------------------
TEST(thp_crypto, sha256_abc)
{
  NoiseHash h{};
  noise_crypto::sha256(reinterpret_cast<const uint8_t *>("abc"), 3, h);
  auto exp = from_hex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  ASSERT_EQ(32u, exp.size());
  EXPECT_EQ(0, std::memcmp(h.data(), exp.data(), exp.size()));
}

// ---------------------------------------------------------------------------
// HMAC-SHA-256 (RFC 4231 test vectors 1 and 4).
// ---------------------------------------------------------------------------
TEST(thp_crypto, hmac_sha256_rfc4231_case1)
{
  auto key  = from_hex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
  auto data = from_hex("4869205468657265"); // "Hi There"
  auto exp  = from_hex("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
  NoiseHash mac{};
  noise_crypto::hmac_sha256(key.data(), key.size(), data.data(), data.size(), mac);
  EXPECT_EQ(0, std::memcmp(mac.data(), exp.data(), exp.size()));
}

TEST(thp_crypto, hmac_sha256_rfc4231_case4)
{
  auto key  = from_hex("0102030405060708090a0b0c0d0e0f10111213141516171819");
  auto data = from_hex("cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
                       "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd");
  auto exp  = from_hex("82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");
  NoiseHash mac{};
  noise_crypto::hmac_sha256(key.data(), key.size(), data.data(), data.size(), mac);
  EXPECT_EQ(0, std::memcmp(mac.data(), exp.data(), exp.size()));
}

// ---------------------------------------------------------------------------
// X25519: RFC 7748 sec. 5.2 raw-scalar test vectors.
// ---------------------------------------------------------------------------
TEST(thp_crypto, x25519_rfc7748_section_5_2_iter1)
{
  auto priv_v = from_hex("a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4");
  auto peer_v = from_hex("e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c");
  auto exp_v  = from_hex("c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552");
  NoisePrivKey priv{}; std::memcpy(priv.data(), priv_v.data(), 32);
  NoisePubKey  peer{}; std::memcpy(peer.data(), peer_v.data(), 32);
  NoisePubKey  out{};
  noise_crypto::x25519(priv, peer, out);
  EXPECT_EQ(0, std::memcmp(out.data(), exp_v.data(), exp_v.size()));
}

TEST(thp_crypto, x25519_rfc7748_section_5_2_iter2)
{
  auto priv_v = from_hex("4b66e9d4d1b4673c5ad22691957d6af5c11b6421e0ea01d42ca4169e7918ba0d");
  auto peer_v = from_hex("e5210f12786811d3f4b7959d0538ae2c31dbe7106fc03c3efc4cd549c715a493");
  auto exp_v  = from_hex("95cbde9476e8907d7aade45cb4b873f88b595a68799fa152e6f8f7647aac7957");
  NoisePrivKey priv{}; std::memcpy(priv.data(), priv_v.data(), 32);
  NoisePubKey  peer{}; std::memcpy(peer.data(), peer_v.data(), 32);
  NoisePubKey  out{};
  noise_crypto::x25519(priv, peer, out);
  EXPECT_EQ(0, std::memcmp(out.data(), exp_v.data(), exp_v.size()));
}

TEST(thp_crypto, x25519_dh_symmetry)
{
  // Generate two random keypairs; verify the standard DH identity.
  NoisePubKey  a_pub{}, b_pub{};
  NoisePrivKey a_priv{}, b_priv{};
  noise_crypto::x25519_keypair(a_pub, a_priv);
  noise_crypto::x25519_keypair(b_pub, b_priv);
  NoisePubKey k1{}, k2{};
  noise_crypto::x25519(a_priv, b_pub, k1);
  noise_crypto::x25519(b_priv, a_pub, k2);
  EXPECT_EQ(0, std::memcmp(k1.data(), k2.data(), 32));
}

// ---------------------------------------------------------------------------
// AES-256-GCM (NIST CAVS gcmEncryptExtIV256.rsp count 0 and 2).
// ---------------------------------------------------------------------------
TEST(thp_crypto, aes256gcm_empty_message_zero_iv)
{
  NoiseKey         key{}; std::memset(key.data(), 0, key.size());
  noise_crypto::NoiseIv  iv{};
  std::vector<uint8_t> ct;
  ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key, iv, nullptr, 0, nullptr, 0, ct));
  ASSERT_EQ(NOISE_TAGLEN, ct.size());
  auto exp = from_hex("530f8afbc74536b9a963b4f1c4cb738b");
  EXPECT_EQ(0, std::memcmp(ct.data(), exp.data(), exp.size()));

  std::vector<uint8_t> pt;
  ASSERT_TRUE(noise_crypto::aes256gcm_decrypt(key, iv, nullptr, 0,
                                        ct.data(), ct.size(), pt));
  EXPECT_TRUE(pt.empty());
}

TEST(thp_crypto, aes256gcm_16_bytes_zero_iv)
{
  NoiseKey         key{}; std::memset(key.data(), 0, key.size());
  noise_crypto::NoiseIv  iv{};
  std::vector<uint8_t> pt(16, 0);
  std::vector<uint8_t> ct;
  ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key, iv, nullptr, 0,
                                        pt.data(), pt.size(), ct));
  auto exp_ct  = from_hex("cea7403d4d606b6e074ec5d3baf39d18");
  auto exp_tag = from_hex("d0d1c8a799996bf0265b98b5d48ab919");
  ASSERT_EQ(32u, ct.size());
  EXPECT_EQ(0, std::memcmp(ct.data(),       exp_ct.data(),  16));
  EXPECT_EQ(0, std::memcmp(ct.data() + 16,  exp_tag.data(), 16));

  std::vector<uint8_t> rt;
  ASSERT_TRUE(noise_crypto::aes256gcm_decrypt(key, iv, nullptr, 0,
                                        ct.data(), ct.size(), rt));
  EXPECT_EQ(rt, pt);
}

TEST(thp_crypto, aes256gcm_tamper_detected)
{
  NoiseKey         key{}; std::memset(key.data(), 0, key.size());
  noise_crypto::NoiseIv  iv{};
  std::vector<uint8_t> pt(16, 0);
  std::vector<uint8_t> ct;
  ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key, iv, nullptr, 0,
                                        pt.data(), pt.size(), ct));
  ct[0] ^= 0x01;
  std::vector<uint8_t> rt;
  EXPECT_FALSE(noise_crypto::aes256gcm_decrypt(key, iv, nullptr, 0,
                                         ct.data(), ct.size(), rt));
}

// Mirrors trezor-firmware's THP encryption vectors
// (core/tests/test_trezor.wire.thp.crypto.py, vectors_enc).  Unlike the
// zero-IV NIST-style tests above, these pin the key + THP-nonce-derived IV
// + AAD path JOINTLY against the values the device computes.
TEST(thp_crypto, aes256gcm_trezor_firmware_thp_nonce_vectors)
{
  const auto key_bytes = from_hex(
      "0001020304050607000102030405060700010203040506070001020304050607");
  NoiseKey key{};
  std::memcpy(key.data(), key_bytes.data(), key.size());

  struct Vector {
    uint64_t    nonce;
    const char *aad_hex;
    const char *pt_hex;
    const char *ct_hex;
    const char *tag_hex;
  };
  const Vector vectors[] = {
    { 0,   "5564",       "00010203040506070809",
      "e2c9dd152fbee5821ea7", "10625812de81b14a46b9f1e5100a6d0c" },
    { 1,   "5564",       "00010203040506070809",
      "79811619ddb07c2b99f8", "71c6b872cdc499a7e9a3c7441f053214" },
    { 369, "5564",       "000102030405060708090a0b0c0d0e0f",
      "03bd030390f2dfe815a61c2b157a064f",
      "c1200f8a7ae9a6d32cef0fff878d55c2" },
    // Same key/nonce/plaintext as above but a longer AAD: the ciphertext
    // is unchanged and only the tag moves.
    { 369, "5564738291", "000102030405060708090a0b0c0d0e0f",
      "03bd030390f2dfe815a61c2b157a064f",
      "693ac160cd93a20f7fc255f049d808d0" },
  };

  for (const Vector &v : vectors) {
    const auto aad = from_hex(v.aad_hex);
    const auto pt  = from_hex(v.pt_hex);
    const auto exp_ct  = from_hex(v.ct_hex);
    const auto exp_tag = from_hex(v.tag_hex);

    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(v.nonce, iv);

    std::vector<uint8_t> ct;
    ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key, iv, aad.data(), aad.size(),
                                          pt.data(), pt.size(), ct));
    ASSERT_EQ(pt.size() + NOISE_TAGLEN, ct.size());
    EXPECT_EQ(0, std::memcmp(ct.data(), exp_ct.data(), exp_ct.size()))
        << "ciphertext mismatch for nonce " << v.nonce;
    EXPECT_EQ(0, std::memcmp(ct.data() + pt.size(),
                             exp_tag.data(), exp_tag.size()))
        << "tag mismatch for nonce " << v.nonce;

    std::vector<uint8_t> rt;
    ASSERT_TRUE(noise_crypto::aes256gcm_decrypt(key, iv, aad.data(), aad.size(),
                                          ct.data(), ct.size(), rt));
    EXPECT_EQ(pt, rt);
  }
}

// ---------------------------------------------------------------------------
// IV builder: nonce 0 yields 0^96, nonce 1 yields 0^95||1, larger counter
// values are encoded big-endian in the trailing 8 bytes.
// ---------------------------------------------------------------------------
TEST(thp_crypto, iv_for_nonce_layout)
{
  noise_crypto::NoiseIv iv{};
  noise_crypto::iv_for_nonce(0, iv);
  for (int i = 0; i < 12; ++i) EXPECT_EQ(0, iv[i]);

  noise_crypto::iv_for_nonce(1, iv);
  for (int i = 0; i < 11; ++i) EXPECT_EQ(0, iv[i]);
  EXPECT_EQ(1, iv[11]);

  noise_crypto::iv_for_nonce(0x0102030405060708ULL, iv);
  EXPECT_EQ(0x01, iv[4]);  EXPECT_EQ(0x02, iv[5]);
  EXPECT_EQ(0x03, iv[6]);  EXPECT_EQ(0x04, iv[7]);
  EXPECT_EQ(0x05, iv[8]);  EXPECT_EQ(0x06, iv[9]);
  EXPECT_EQ(0x07, iv[10]); EXPECT_EQ(0x08, iv[11]);
}

// Mirrors trezor-firmware's IV-from-nonce vectors
// (core/tests/test_trezor.wire.thp.crypto.py, vectors_iv), including the
// all-ones boundary the device treats as the last usable nonce.
TEST(thp_crypto, iv_for_nonce_trezor_firmware_vectors)
{
  struct Vector { uint64_t nonce; const char *iv_hex; };
  const Vector vectors[] = {
    { 0,                     "000000000000000000000000" },
    { 1,                     "000000000000000000000001" },
    { 7,                     "000000000000000000000007" },
    { 1025,                  "000000000000000000000401" },
    { 4294967295ULL,         "0000000000000000ffffffff" },
    { 0xFFFFFFFFFFFFFFFFULL, "00000000ffffffffffffffff" },
  };
  for (const Vector &v : vectors) {
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(v.nonce, iv);
    const auto exp = from_hex(v.iv_hex);
    EXPECT_EQ(0, std::memcmp(iv.data(), exp.data(), iv.size()))
        << "IV mismatch for nonce " << v.nonce;
  }
}

// ---------------------------------------------------------------------------
// THP HKDF self-consistency (regression check derived from the spec text).
// ---------------------------------------------------------------------------
TEST(thp_crypto, thp_hkdf_self_consistency)
{
  NoiseHash ck{};  std::memset(ck.data(),  'k', ck.size());
  NoiseHash ikm{}; std::memset(ikm.data(), 'v', ikm.size());

  NoiseHash temp{}, o1{}, o2{};
  noise_crypto::hmac_sha256(ck.data(), ck.size(), ikm.data(), ikm.size(), temp);
  const uint8_t one = 0x01;
  noise_crypto::hmac_sha256(temp.data(), temp.size(), &one, 1, o1);
  std::array<uint8_t, NOISE_HASHLEN + 1> o1p2{};
  std::memcpy(o1p2.data(), o1.data(), 32);
  o1p2[32] = 0x02;
  noise_crypto::hmac_sha256(temp.data(), temp.size(),
                      o1p2.data(), o1p2.size(), o2);

  NoiseKey out1{}, out2{};
  noise_crypto::thp_hkdf(ck.data(), ck.size(), ikm.data(), ikm.size(), out1, out2);
  EXPECT_EQ(0, std::memcmp(out1.data(), o1.data(), 32));
  EXPECT_EQ(0, std::memcmp(out2.data(), o2.data(), 32));
}

// Mirrors trezor-firmware's THP HKDF fixed vectors
// (core/tests/test_trezor.wire.thp.crypto.py, vectors_hkdf).  Unlike the
// self-consistency check above, which would miss a bug present on both
// sides, these pin the output against values computed by an independent
// implementation.  The first chaining key is the Noise protocol name the
// handshake hashes are seeded with; the second vector chains off the
// first vector's ck output, as the handshake itself does.
TEST(thp_crypto, thp_hkdf_trezor_firmware_vectors)
{
  // "Noise_XX_25519_AESGCM_SHA256" padded with NULs to 32 bytes.
  const uint8_t protocol_name[32] = {
    'N','o','i','s','e','_','X','X','_','2','5','5','1','9','_',
    'A','E','S','G','C','M','_','S','H','A','2','5','6',0,0,0,0,
  };

  const uint8_t ikm1[] = { 0x01, 0x02 };
  NoiseKey ck1{}, k1{};
  noise_crypto::thp_hkdf(protocol_name, sizeof(protocol_name),
                   ikm1, sizeof(ikm1), ck1, k1);
  const auto exp_ck1 = from_hex(
      "c784373a217d6be057cddc6068e6748f255fc8beb6f99b7b90cbc64aad947514");
  const auto exp_k1 = from_hex(
      "12695451e29bf08ffe5e4e6ab734b0c3d7cdd99b16cd409f57bd4eaa874944ba");
  EXPECT_EQ(0, std::memcmp(ck1.data(), exp_ck1.data(), ck1.size()));
  EXPECT_EQ(0, std::memcmp(k1.data(),  exp_k1.data(),  k1.size()));

  const auto ikm2 = from_hex("314159265212345678" "8904aa");
  NoiseKey ck2{}, k2{};
  noise_crypto::thp_hkdf(ck1.data(), ck1.size(),
                   ikm2.data(), ikm2.size(), ck2, k2);
  const auto exp_ck2 = from_hex(
      "f88c1e08d5c3bae8f6e4a3d3324c8cbc60a805603e399e69c4bf4eacb27c2f48");
  const auto exp_k2 = from_hex(
      "5f0216bdb7110ee05372286974da8c9c8b96e2efa15b4af430755f462bd79a76");
  EXPECT_EQ(0, std::memcmp(ck2.data(), exp_ck2.data(), ck2.size()));
  EXPECT_EQ(0, std::memcmp(k2.data(),  exp_k2.data(),  k2.size()));
}

// ---------------------------------------------------------------------------
// trezor_mask_static is deterministic for fixed inputs and changes whenever
// the ephemeral public key changes (so paired-device lookups discriminate).
// ---------------------------------------------------------------------------
TEST(thp_crypto, trezor_mask_static_deterministic_and_eph_bound)
{
  NoisePubKey  static_pub{};
  NoisePrivKey static_priv{};
  noise_crypto::x25519_keypair(static_pub, static_priv);

  NoisePubKey  eph_pub{};
  NoisePrivKey eph_priv{};
  noise_crypto::x25519_keypair(eph_pub, eph_priv);

  NoisePubKey m1{}, m2{};
  noise_crypto::trezor_mask_static(static_pub, eph_pub, m1);
  noise_crypto::trezor_mask_static(static_pub, eph_pub, m2);
  EXPECT_EQ(0, std::memcmp(m1.data(), m2.data(), 32));

  NoisePubKey  eph_pub2{};
  NoisePrivKey eph_priv2{};
  noise_crypto::x25519_keypair(eph_pub2, eph_priv2);
  NoisePubKey m_alt{};
  noise_crypto::trezor_mask_static(static_pub, eph_pub2, m_alt);
  EXPECT_NE(0, std::memcmp(m1.data(), m_alt.data(), 32));
}

// ---------------------------------------------------------------------------
// Self-handshake: simulate the Trezor side of the handshake exactly per the
// state machine in specification.md, run a complete handshake against the
// real host implementation, and verify the two sides agree on the final
// transport keys and handshake hash.
//
// This is a SELF test (both sides are this code), so it would not catch a
// bug that is mirrored on both sides.  Its value is in pinning the
// algorithm: any deviation in IV layout, AAD, mix_hash ordering, masked
// key construction, or HKDF call shape between the two sides causes the
// AES-GCM tag to mismatch and the test to fail loudly.
// ---------------------------------------------------------------------------
namespace {

struct TrezorSide {
  NoisePubKey  static_pub{};
  NoisePrivKey static_priv{};
  NoisePubKey  ephemeral_pub{};
  NoisePrivKey ephemeral_priv{};
  std::vector<uint8_t> device_properties;
  // The state byte reported in HandshakeCompletionResponse.
  uint8_t      state = STATE_UNPAIRED;

  NoiseHash h{};
  NoiseKey  ck{};
  NoiseKey  k{};

  HandshakeKeys keys{};
  std::vector<uint8_t> handshake_init_response;
  std::vector<uint8_t> handshake_completion_response;
  std::vector<uint8_t> decrypted_completion_payload;
  uint8_t              host_static_pub_received[NOISE_DHLEN] = {0};

  void mix_hash(const uint8_t *data, size_t len) {
    std::vector<uint8_t> buf;
    buf.reserve(NOISE_HASHLEN + len);
    buf.insert(buf.end(), h.begin(), h.end());
    if (len) buf.insert(buf.end(), data, data + len);
    noise_crypto::sha256(buf.data(), buf.size(), h);
  }

  void handle_init_request(const std::vector<uint8_t> &req) {
    // req = host_eph_pub (32) || try_to_unlock (1)
    ASSERT_EQ(NOISE_DHLEN + 1u, req.size());

    noise_crypto::x25519_keypair(ephemeral_pub, ephemeral_priv);

    // h = SHA-256(protocol_name || device_properties)
    static const uint8_t protocol_name[] = {
      'N','o','i','s','e','_','X','X','_','2','5','5','1','9',
      '_','A','E','S','G','C','M','_','S','H','A','2','5','6',
      0,0,0,0
    };
    std::vector<uint8_t> hash_in;
    hash_in.insert(hash_in.end(), protocol_name, protocol_name + sizeof(protocol_name));
    hash_in.insert(hash_in.end(), device_properties.begin(), device_properties.end());
    noise_crypto::sha256(hash_in.data(), hash_in.size(), h);

    // h = SHA-256(h || host_eph_pub)
    mix_hash(req.data(), NOISE_DHLEN);
    // h = SHA-256(h || try_to_unlock)
    mix_hash(req.data() + NOISE_DHLEN, 1);
    // h = SHA-256(h || trezor_eph_pub)
    mix_hash(ephemeral_pub.data(), ephemeral_pub.size());

    // (ck, k) = HKDF(protocol_name, X25519(trezor_eph_priv, host_eph_pub))
    NoisePubKey host_eph_pub{};
    std::memcpy(host_eph_pub.data(), req.data(), NOISE_DHLEN);
    NoisePubKey ee{};
    noise_crypto::x25519(ephemeral_priv, host_eph_pub, ee);
    noise_crypto::thp_hkdf(protocol_name, sizeof(protocol_name),
                     ee.data(), ee.size(), ck, k);

    // mask = SHA-256(static_pub || trezor_eph_pub)
    NoisePubKey masked{};
    noise_crypto::trezor_mask_static(static_pub, ephemeral_pub, masked);

    // enc_static = AES-GCM-Encrypt(k, IV=0^96, ad=h, masked)
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> enc_static;
    ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(k, iv, h.data(), h.size(),
                                          masked.data(), masked.size(),
                                          enc_static));
    // h = SHA-256(h || enc_static)
    mix_hash(enc_static.data(), enc_static.size());

    // (ck, k) = HKDF(ck, X25519(mask, X25519(static_priv, host_eph_pub)))
    NoisePubKey se{};
    noise_crypto::x25519(static_priv, host_eph_pub, se);
    // The trezor side computes X25519(mask_scalar, ...). Equivalently, the
    // host computes X25519(host_eph_priv, masked). They produce the same DH
    // shared secret. Spec phrasing: ck, k = HKDF(ck, X25519(mask, se)).
    std::array<uint8_t, NOISE_DHLEN * 2> concat{};
    std::memcpy(concat.data(),               static_pub.data(),    NOISE_DHLEN);
    std::memcpy(concat.data() + NOISE_DHLEN, ephemeral_pub.data(), NOISE_DHLEN);
    NoiseHash mask_h{};
    noise_crypto::sha256(concat.data(), concat.size(), mask_h);
    NoisePrivKey mask_scalar{};
    std::memcpy(mask_scalar.data(), mask_h.data(), NOISE_DHLEN);
    NoisePubKey ssm{};
    noise_crypto::x25519(mask_scalar, se, ssm);
    noise_crypto::thp_hkdf(ck.data(), ck.size(), ssm.data(), ssm.size(), ck, k);

    // tag = AES-GCM(k, IV=0^96, ad=h, plaintext=empty)
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> tag;
    ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(k, iv, h.data(), h.size(),
                                          nullptr, 0, tag));
    // h = SHA-256(h || tag)
    mix_hash(tag.data(), tag.size());

    handshake_init_response.clear();
    handshake_init_response.insert(handshake_init_response.end(),
                                   ephemeral_pub.begin(), ephemeral_pub.end());
    handshake_init_response.insert(handshake_init_response.end(),
                                   enc_static.begin(), enc_static.end());
    handshake_init_response.insert(handshake_init_response.end(),
                                   tag.begin(), tag.end());
  }

  void handle_completion_request(const std::vector<uint8_t> &req) {
    // req = enc_host_static (48) || enc_payload (var)
    ASSERT_GE(req.size(), 48u);
    const uint8_t *enc_host_static = req.data();
    const uint8_t *enc_payload     = req.data() + 48;
    const size_t   enc_payload_len = req.size() - 48;

    // host_static_pub = AES-GCM-Decrypt(k, IV=0^95||1, ad=h, enc_host_static)
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(1, iv);
    std::vector<uint8_t> host_static_pub;
    ASSERT_TRUE(noise_crypto::aes256gcm_decrypt(k, iv, h.data(), h.size(),
                                          enc_host_static, 48, host_static_pub));
    ASSERT_EQ(NOISE_DHLEN, host_static_pub.size());
    std::memcpy(host_static_pub_received, host_static_pub.data(), NOISE_DHLEN);
    // h = SHA-256(h || enc_host_static)
    mix_hash(enc_host_static, 48);

    // (ck, k) = HKDF(ck, X25519(trezor_eph_priv, host_static_pub))
    NoisePubKey hsp{}; std::memcpy(hsp.data(), host_static_pub.data(), NOISE_DHLEN);
    NoisePubKey ss{};
    noise_crypto::x25519(ephemeral_priv, hsp, ss);
    noise_crypto::thp_hkdf(ck.data(), ck.size(), ss.data(), ss.size(), ck, k);

    // payload = AES-GCM-Decrypt(k, IV=0^96, ad=h, enc_payload)
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> payload;
    ASSERT_TRUE(noise_crypto::aes256gcm_decrypt(k, iv, h.data(), h.size(),
                                          enc_payload, enc_payload_len, payload));
    decrypted_completion_payload = payload;
    // h = SHA-256(h || enc_payload)
    mix_hash(enc_payload, enc_payload_len);

    // (key_request, key_response) = HKDF(ck, empty)
    NoiseKey key_request{}, key_response{};
    noise_crypto::thp_hkdf(ck.data(), ck.size(), nullptr, 0, key_request, key_response);

    // encrypted_state = AES-GCM-Encrypt(key_response, IV=0^96, ad=empty,
    //                                   plaintext=trezor_state byte)
    const uint8_t state_byte = state;
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> encrypted_state;
    ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key_response, iv,
                                          nullptr, 0,
                                          &state_byte, 1,
                                          encrypted_state));
    handshake_completion_response = encrypted_state;

    keys.key_request    = key_request;
    keys.key_response   = key_response;
    keys.handshake_hash = h;
  }
};

} // namespace

TEST(thp_handshake, full_handshake_self_test)
{
  TrezorSide trezor;
  // Generate the persistent Trezor static keypair.
  noise_crypto::x25519_keypair(trezor.static_pub, trezor.static_priv);
  // Some non-empty device properties (real device sends a serialised
  // ThpDeviceProperties protobuf; for the purposes of this test the
  // bytes just need to match what the host hashes into h).
  trezor.device_properties = {0x0a, 0x04, 'T','2','B','1'};

  // Host side: fresh static keypair, no known devices.
  HostStaticKey host_static;
  noise_crypto::x25519_keypair(host_static.pub, host_static.priv);

  NoiseXxInitiator host;
  host.set_device_properties(trezor.device_properties.data(),
                             trezor.device_properties.size());
  host.set_host_static_key(host_static);

  // Step 1: host -> trezor: HandshakeInitiationRequest
  auto init_req = host.build_init_request(/*try_to_unlock=*/false);
  trezor.handle_init_request(init_req);

  // Step 2: trezor -> host: HandshakeInitiationResponse
  host.consume_init_response(trezor.handshake_init_response.data(),
                             trezor.handshake_init_response.size());
  EXPECT_FALSE(host.is_known_device());

  // Step 3: host -> trezor: HandshakeCompletionRequest
  auto comp_req = host.build_completion_request();
  trezor.handle_completion_request(comp_req);

  // The unpaired flow sends an empty ThpHandshakeCompletionReqNoisePayload,
  // which serialises to zero protobuf bytes. The Trezor must therefore see
  // a zero-length decrypted payload.
  EXPECT_TRUE(trezor.decrypted_completion_payload.empty());

  // The Trezor must see exactly the host's static public key.
  EXPECT_EQ(0, std::memcmp(trezor.host_static_pub_received,
                           host_static.pub.data(), NOISE_DHLEN));

  // Step 4: trezor -> host: HandshakeCompletionResponse
  host.consume_completion_response(trezor.handshake_completion_response.data(),
                                   trezor.handshake_completion_response.size());
  ASSERT_TRUE(host.is_complete());
  EXPECT_EQ(0x00 /*STATE_UNPAIRED*/, host.trezor_state());

  // Both sides must have arrived at the same transport keys + handshake hash.
  const HandshakeKeys &hk = host.keys();
  EXPECT_EQ(0, std::memcmp(hk.key_request.data(),    trezor.keys.key_request.data(),    NOISE_KEYLEN));
  EXPECT_EQ(0, std::memcmp(hk.key_response.data(),   trezor.keys.key_response.data(),   NOISE_KEYLEN));
  EXPECT_EQ(0, std::memcmp(hk.handshake_hash.data(), trezor.keys.handshake_hash.data(), NOISE_HASHLEN));

  // The masked-static-pubkey decoded by the host must match what the
  // Trezor would have computed for itself (round-trip identity check).
  NoisePubKey expected_masked{};
  noise_crypto::trezor_mask_static(trezor.static_pub, trezor.ephemeral_pub, expected_masked);
  EXPECT_EQ(0, std::memcmp(host.trezor_masked_static_pubkey().data(),
                           expected_masked.data(), NOISE_DHLEN));
}

// ---------------------------------------------------------------------------
// Paired-device recognition: a session N+1 with a fresh ephemeral must
// still match the unmasked-static stored from session N. Earlier we stored
// the masked form, which rotates per session and so could never re-match:
// the user was forced to re-pair every connect.
// ---------------------------------------------------------------------------
TEST(thp_handshake, paired_device_recognised_across_sessions)
{
  // Session 1: simulate a Trezor we've previously paired with.  Capture
  // its (unmasked) static pubkey and the host_static keypair we used.
  TrezorSide trezor;
  noise_crypto::x25519_keypair(trezor.static_pub, trezor.static_priv);
  trezor.device_properties = {0x0a, 0x04, 'T','S','7','x'};

  HostStaticKey host_static;
  noise_crypto::x25519_keypair(host_static.pub, host_static.priv);

  // Session 2: brand new initiator.  We feed it a KnownDevice whose
  // trezor_static_pubkey is the unmasked one from the (simulated)
  // ThpCredentialResponse.  The Trezor side will generate a fresh
  // ephemeral, so the *masked* form on the wire is different from what
  // session 1 saw, but the lookup must still succeed because we store
  // the unmasked pubkey and recompute the mask per session.
  KnownDevice kd;
  kd.trezor_static_pubkey   = trezor.static_pub;
  kd.host_static            = host_static;
  kd.pairing_credential     = {0xDE, 0xAD, 0xBE, 0xEF};

  NoiseXxInitiator host;
  host.set_device_properties(trezor.device_properties.data(),
                             trezor.device_properties.size());
  host.set_host_static_key(host_static);
  host.set_known_devices({kd});

  auto init_req = host.build_init_request(/*try_to_unlock=*/false);
  trezor.handle_init_request(init_req);
  host.consume_init_response(trezor.handshake_init_response.data(),
                             trezor.handshake_init_response.size());

  // The whole point of B2: the recompute-on-lookup must succeed even
  // though the masked form on the wire is novel.
  EXPECT_TRUE(host.is_known_device());
}

// ---------------------------------------------------------------------------
// Same lookup MUST NOT match a non-paired device: an attacker cannot
// trick the host into thinking a fresh device is "known" by guessing.
// ---------------------------------------------------------------------------
TEST(thp_handshake, paired_lookup_rejects_unrelated_device)
{
  TrezorSide trezor;
  noise_crypto::x25519_keypair(trezor.static_pub, trezor.static_priv);
  trezor.device_properties = {0x0a, 0x04, 'T','S','7','x'};

  // Known-device list contains some OTHER device's static pubkey.
  HostStaticKey host_static;
  noise_crypto::x25519_keypair(host_static.pub, host_static.priv);
  HostStaticKey other_host;
  noise_crypto::x25519_keypair(other_host.pub, other_host.priv);
  NoisePubKey other_static{}; NoisePrivKey other_priv{};
  noise_crypto::x25519_keypair(other_static, other_priv);

  KnownDevice kd;
  kd.trezor_static_pubkey   = other_static;          // not `trezor.static_pub`
  kd.host_static            = other_host;
  kd.pairing_credential     = {0x01, 0x02, 0x03};

  NoiseXxInitiator host;
  host.set_device_properties(trezor.device_properties.data(),
                             trezor.device_properties.size());
  host.set_host_static_key(host_static);
  host.set_known_devices({kd});

  auto init_req = host.build_init_request(/*try_to_unlock=*/false);
  trezor.handle_init_request(init_req);
  host.consume_init_response(trezor.handshake_init_response.data(),
                             trezor.handshake_init_response.size());

  EXPECT_FALSE(host.is_known_device());
}

// ---------------------------------------------------------------------------
// TransportCipher: round-trip seal / open with non-zero counters.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Elligator2 (RFC 9380 sec. G.2.1) for Curve25519.
//
// Provenance of the two expected vectors below: they are the byte-for-byte
// output of the reference Elligator2 map in trezor-firmware's
// python/src/trezorlib/thp/curve25519.py (pinned commit
// 236cb059dd9e3ac92694ee589ab4037761517d61) applied to the little-endian
// 32-byte field elements 1 and 2.  Reproduce with:
//
//     from trezorlib.thp import curve25519
//     curve25519.elligator2((1).to_bytes(32, "little")).hex()  # 9cdb5255...
//     curve25519.elligator2((2).to_bytes(32, "little")).hex()  # b349328e...
//
// elligator2() internally runs decode_coordinate(), which clears the input's
// high bit (`array[-1] &= 0x7F`, RFC 7748 sec. 5) before the map, so these two
// vectors also pin our decode_coordinate behaviour; the dedicated high-bit
// test below exercises that mask in isolation.
// ---------------------------------------------------------------------------
TEST(thp_elligator2, deterministic_known_inputs)
{
  // Input 0x01..0
  uint8_t r1[32] = {0}; r1[0] = 1;
  uint8_t out1[32];
  elligator2_curve25519(r1, out1);
  auto exp1 = from_hex("9cdb525555555555555555555555555555555555555555555555555555555555");
  EXPECT_EQ(0, std::memcmp(out1, exp1.data(), 32));

  // Input 0x02..0
  uint8_t r2[32] = {0}; r2[0] = 2;
  uint8_t out2[32];
  elligator2_curve25519(r2, out2);
  auto exp2 = from_hex("b349328ee3388ee3388ee3388ee3388ee3388ee3388ee3388ee3388ee3388e63");
  EXPECT_EQ(0, std::memcmp(out2, exp2.data(), 32));

  // Determinism: same input -> same output
  uint8_t out1b[32];
  elligator2_curve25519(r1, out1b);
  EXPECT_EQ(0, std::memcmp(out1, out1b, 32));

  // Distinct inputs -> distinct outputs
  EXPECT_NE(0, std::memcmp(out1, out2, 32));
}

// Regression: per RFC 7748 sec. 5 / RFC 9380 sec. G.2.1, decode_coordinate must
// clear the high bit of the input before reducing mod p. The Python
// reference (trezorlib/thp/curve25519.py) does `array[-1] &= 0x7F`. If we
// skip that mask, ~50% of pairings (whenever SHA-512's truncated high
// bit is set) produce a different field element than the device, and
// CPace tags mismatch even with the correct user code.
TEST(thp_elligator2, decode_coordinate_masks_high_bit)
{
  uint8_t with_high_bit[32]    = {0}; with_high_bit[31]    = 0x81;
  uint8_t without_high_bit[32] = {0}; without_high_bit[31] = 0x01;
  uint8_t out_a[32], out_b[32];
  elligator2_curve25519(with_high_bit,    out_a);
  elligator2_curve25519(without_high_bit, out_b);
  EXPECT_EQ(0, std::memcmp(out_a, out_b, 32))
      << "elligator2 must mask the high bit of the input (RFC 7748 sec. 5)";

  // Sanity: the mask actually changes nothing when the high bit was
  // already clear: the same bytes produce the same output.
  uint8_t out_c[32];
  elligator2_curve25519(without_high_bit, out_c);
  EXPECT_EQ(0, std::memcmp(out_b, out_c, 32));
}

// The CPace generator is the one construction that fails silently and
// totally: a wrong domain-separation prefix, length byte, pad length or
// hash input still produces a well-formed 32-byte generator, and pairing
// then fails against every real device with no diagnostic pointing at the
// cause.  Deterministic-and-input-bound is true of any hash-based
// derivation, so the three expected values are what make this test able to
// detect a wrong construction.
//
// They were computed from the IRTF CPACE-X25519-SHA512 symmetric-setting
// definition the THP spec cites (specification.md, Notes, "Code Entry
// pairing sequence"):
//
//   pregenerator = SHA-512(0x08 "CPace255" 0x06 || code ||
//                          0x6f || 0x00 * 111 || 0x20 ||
//                          handshake_hash || 0x00)[:32]
//   generator    = ELLIGATOR2(pregenerator)
//
// by an independent implementation, itself first validated against the two
// trezorlib-derived elligator2 vectors pinned above.
TEST(thp_elligator2, cpace_generator_matches_spec_vectors)
{
  NoiseHash h{}; for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t(i);
  uint8_t g1[32], g2[32], g3[32];
  cpace_derive_generator("123456", h, g1);
  cpace_derive_generator("123456", h, g2);
  EXPECT_EQ(0, std::memcmp(g1, g2, 32)) << "deterministic";

  cpace_derive_generator("654321", h, g3);
  EXPECT_NE(0, std::memcmp(g1, g3, 32)) << "different code -> different generator";

  NoiseHash h2{}; for (size_t i = 0; i < h2.size(); ++i) h2[i] = uint8_t(i + 1);
  uint8_t g4[32];
  cpace_derive_generator("123456", h2, g4);
  EXPECT_NE(0, std::memcmp(g1, g4, 32)) << "different handshake hash -> different generator";

  const auto exp_g1 = from_hex(
      "4e1da1dfe958d2243b655fd45da026889142bcc3aa96c7eb58f10242cd8af52a");
  const auto exp_g3 = from_hex(
      "7db4d7908f3b1d7c1c721f1195ed6d61af4d4c99f2d6a6e4d86dd41dbfc7a056");
  const auto exp_g4 = from_hex(
      "190f471b762da11231b3099f82686e43afd146f64cd43c1a4b72da9d13ae353b");
  EXPECT_EQ(0, std::memcmp(g1, exp_g1.data(), 32)) << "code 123456, h = 00..1f";
  EXPECT_EQ(0, std::memcmp(g3, exp_g3.data(), 32)) << "code 654321, h = 00..1f";
  EXPECT_EQ(0, std::memcmp(g4, exp_g4.data(), 32)) << "code 123456, h = 01..20";
}

// The prefix's trailing 0x06 is CPace's prepend_len(PRS) and the 111-byte
// pad is 128 - (9 + (1 + 6) + 1): both are fixed for a six-digit code, so
// any other length silently derives a different domain.  Firmware asserts
// the same length; we throw rather than derive something unusable.
TEST(thp_elligator2, cpace_generator_rejects_codes_that_are_not_six_digits)
{
  NoiseHash h{};
  uint8_t g[32];
  EXPECT_THROW(cpace_derive_generator("12345",   h, g),
               hw::trezor::exc::ProtocolException);
  EXPECT_THROW(cpace_derive_generator("1234567", h, g),
               hw::trezor::exc::ProtocolException);
  EXPECT_NO_THROW(cpace_derive_generator("123456", h, g));
}

// ---------------------------------------------------------------------------
// CodeEntry pairing: drive the FSM as the host with a simulated Trezor.
// The simulated Trezor exists only here; it follows specification.md's
// state machine TP* literally.  As with the handshake self-test, this
// pins the algorithm and would catch any deviation between the two
// sides, but a bug present on both sides cannot be detected here.
// ---------------------------------------------------------------------------
namespace {

struct SimulatedTrezor {
  NoiseHash handshake_hash{};
  uint8_t   secret[16] = {0};
  uint8_t   commitment[32] = {0};
  uint8_t   challenge[16] = {0};
  uint32_t  code_int = 0;
  std::string code_str;
  uint8_t   cpace_priv[32] = {0};
  uint8_t   cpace_pub[32]  = {0};
  uint8_t   shared[32]     = {0};

  void set_handshake_hash(const NoiseHash &h) { handshake_hash = h; }

  // Trezor builds the commitment from secret = SHA-256(secret).
  std::vector<uint8_t> step_select_method() {
    // Pick a deterministic secret for testability.
    for (int i = 0; i < 16; ++i) secret[i] = uint8_t(0xa0 + i);
    NoiseHash c{};
    noise_crypto::sha256(secret, 16, c);
    std::memcpy(commitment, c.data(), sizeof(commitment));
    return std::vector<uint8_t>(commitment, commitment + 32);
  }

  // After commitment + challenge, derive the code, then compute the
  // CPace ephemeral keys for the device side.
  std::vector<uint8_t> step_after_challenge(const uint8_t *host_challenge, size_t len) {
    if (len != 16) throw std::runtime_error("bad challenge size");
    std::memcpy(challenge, host_challenge, 16);
    // code = SHA-256(0x02 || h || secret || challenge) % 1_000_000 (BE)
    std::vector<uint8_t> in;
    in.push_back(0x02);
    in.insert(in.end(), handshake_hash.begin(), handshake_hash.end());
    in.insert(in.end(), secret, secret + 16);
    in.insert(in.end(), challenge, challenge + 16);
    NoiseHash code_hash;
    noise_crypto::sha256(in.data(), in.size(), code_hash);
    // Big-endian reduce mod 1e6
    uint64_t accum = 0;
    for (uint8_t b : code_hash) accum = (accum * 256 + b) % 1000000ULL;
    code_int = uint32_t(accum);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%06u", code_int);
    code_str = buf;
    // Derive cpace key pair
    uint8_t generator[32];
    cpace_derive_generator(code_str, handshake_hash, generator);
    for (int i = 0; i < 32; ++i) cpace_priv[i] = uint8_t(i + 1);
    NoisePrivKey priv{}; std::memcpy(priv.data(), cpace_priv, 32);
    NoisePubKey gen{};   std::memcpy(gen.data(),  generator,  32);
    NoisePubKey pub{};
    noise_crypto::x25519(priv, gen, pub);
    std::memcpy(cpace_pub, pub.data(), 32);
    return std::vector<uint8_t>(cpace_pub, cpace_pub + 32);
  }

  // After the host sends back its CPace public key + tag, verify and
  // (on success) hand back the secret.
  std::vector<uint8_t> step_consume_host_tag(const uint8_t *host_pub, const uint8_t *tag) {
    NoisePrivKey priv{}; std::memcpy(priv.data(), cpace_priv, 32);
    NoisePubKey  hp{};   std::memcpy(hp.data(),   host_pub,  32);
    NoisePubKey  s{};
    noise_crypto::x25519(priv, hp, s);
    std::memcpy(shared, s.data(), 32);
    NoiseHash expected_tag;
    noise_crypto::sha256(shared, 32, expected_tag);
    if (std::memcmp(expected_tag.data(), tag, 32) != 0) {
      throw std::runtime_error("simulated Trezor: tag mismatch");
    }
    return std::vector<uint8_t>(secret, secret + 16);
  }
};

} // namespace

TEST(thp_pairing, code_entry_full_self_test)
{
  // Pretend a handshake just completed.
  NoiseHash h{};
  for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t(0x10 + i);

  SimulatedTrezor trezor;
  trezor.set_handshake_hash(h);

  CodeEntryPairing host(h);

  // Trezor sends commitment.
  auto cmt_raw = trezor.step_select_method();

  // Host consumes commitment and draws the challenge the caller places in
  // ThpCodeEntryChallenge; the device reads those 16 bytes.
  host.consume_commitment_build_challenge(cmt_raw.data(), cmt_raw.size());

  // Trezor computes the code + cpace pubkey.
  auto cpace_t_pub_raw = trezor.step_after_challenge(host.challenge().data(),
                                                     host.challenge().size());

  // Host consumes Trezor's CPace public key.
  host.consume_cpace_trezor(cpace_t_pub_raw.data(), cpace_t_pub_raw.size());

  // The user sees Trezor's display, types the same code into the host.
  // We give the host the exact code the simulator computed.
  const CpaceHostTag host_tag = host.build_host_tag(trezor.code_str);

  // Trezor verifies the tag and (on success) sends the secret.
  auto secret_raw = trezor.step_consume_host_tag(
      host_tag.cpace_host_public_key.data(), host_tag.tag.data());

  // Host verifies commitment and code-secret-challenge equation.
  EXPECT_TRUE(host.consume_secret(secret_raw.data(), secret_raw.size()));
  EXPECT_TRUE(host.is_paired());
}

// Wrong-code fail-closed.
//
// Security outcome under test: when the user types the WRONG pairing
// code, the host derives a CPace generator that differs from the
// device's, so the host's CPace public key and SHA-256(shared_secret)
// tag DO NOT match what the device computed.  The device-side CPace tag
// check (step_consume_host_tag) MUST reject, failing closed, instead of
// handing back the secret.  is_paired() must remain false.
//
// History: an earlier version of this test computed `host.build_host_tag
// (wrong_code)` and then only asserted `EXPECT_FALSE(host.is_paired())`.
// That was a TAUTOLOGY: is_paired() can only become true after a
// successful consume_secret(), which the test never called, so the
// assertion was trivially true regardless of whether the wrong code was
// actually rejected by the CPace tag check.  It exercised no security
// path.
//
// This rewrite drives the real comparison: it feeds the wrong-code
// host_tag into the simulated device's tag verifier and asserts it
// THROWS (tag mismatch -> abort), AND, as a cross-check that the
// rejection is genuinely code-dependent and not a fluke of the harness,
// confirms the CORRECT code on the same FSM state verifies cleanly,
// pairs, and yields a secret.
TEST(thp_pairing, code_entry_wrong_code_fails_closed)
{
  NoiseHash h{};
  for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t(i);

  // ---- Wrong-code path: device tag check must fail closed. ----
  {
    SimulatedTrezor trezor;
    trezor.set_handshake_hash(h);
    CodeEntryPairing host(h);

    auto cmt = trezor.step_select_method();
    host.consume_commitment_build_challenge(cmt.data(), cmt.size());
    auto cpace_t_pub = trezor.step_after_challenge(host.challenge().data(),
                                                   host.challenge().size());
    host.consume_cpace_trezor(cpace_t_pub.data(), cpace_t_pub.size());

    // User mistypes the code: flip the leading digit so the host's CPace
    // generator (and hence shared secret and tag) diverges from the
    // device's.
    std::string wrong_code = trezor.code_str;
    if (wrong_code[0] == '9') wrong_code[0] = '0'; else wrong_code[0]++;
    ASSERT_NE(wrong_code, trezor.code_str);

    const CpaceHostTag host_tag = host.build_host_tag(wrong_code);

    // The security-relevant assertion: the device's CPace tag check
    // detects the mismatch and aborts (throws); it does NOT release the
    // secret.  This is the genuine fail-closed outcome, not a precondition.
    EXPECT_THROW(trezor.step_consume_host_tag(host_tag.cpace_host_public_key.data(),
                                              host_tag.tag.data()),
                 std::exception);

    // The host was never told it paired (no consume_secret reached).
    EXPECT_FALSE(host.is_paired());
  }

  // ---- Correct-code cross-check: same flow, right code -> pairs. ----
  // Proves the rejection above was caused by the wrong code, not by a
  // broken harness that rejects everything.
  {
    SimulatedTrezor trezor;
    trezor.set_handshake_hash(h);
    CodeEntryPairing host(h);

    auto cmt = trezor.step_select_method();
    host.consume_commitment_build_challenge(cmt.data(), cmt.size());
    auto cpace_t_pub = trezor.step_after_challenge(host.challenge().data(),
                                                   host.challenge().size());
    host.consume_cpace_trezor(cpace_t_pub.data(), cpace_t_pub.size());

    const CpaceHostTag host_tag = host.build_host_tag(trezor.code_str);

    std::vector<uint8_t> secret_raw;
    ASSERT_NO_THROW(secret_raw = trezor.step_consume_host_tag(
        host_tag.cpace_host_public_key.data(), host_tag.tag.data()));
    EXPECT_TRUE(host.consume_secret(secret_raw.data(), secret_raw.size()));
    EXPECT_TRUE(host.is_paired());
  }
}

// The host half of the authentication: consume_secret is where the host
// decides the device it just did CPace with is the one the user is looking
// at.  Both of its rejection branches are exercised here; without them a
// device that never knew the code, or one that returns a secret matching a
// commitment it made up after seeing the challenge, would pair silently.
TEST(thp_pairing, consume_secret_rejects_bad_commitment_and_wrong_code)
{
  NoiseHash h{};
  for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t(0x40 + i);

  // Branch (a): commitment != SHA-256(secret).  Correct code entered, but
  // the secret handed back is not the one the commitment covered.
  {
    SimulatedTrezor trezor;
    trezor.set_handshake_hash(h);
    CodeEntryPairing host(h);

    auto cmt = trezor.step_select_method();
    host.consume_commitment_build_challenge(cmt.data(), cmt.size());
    auto cpace_t_pub = trezor.step_after_challenge(host.challenge().data(),
                                                   host.challenge().size());
    host.consume_cpace_trezor(cpace_t_pub.data(), cpace_t_pub.size());
    host.build_host_tag(trezor.code_str);

    std::vector<uint8_t> tampered(trezor.secret, trezor.secret + 16);
    tampered[0] ^= 0x01;
    EXPECT_FALSE(host.consume_secret(tampered.data(), tampered.size()));
    EXPECT_FALSE(host.is_paired());
  }

  // Branch (b): the secret is genuine, so the commitment check passes, but
  // the code the user entered is not the one it derives.  A device that
  // displayed a different code than the user typed must not pair.
  {
    SimulatedTrezor trezor;
    trezor.set_handshake_hash(h);
    CodeEntryPairing host(h);

    auto cmt = trezor.step_select_method();
    host.consume_commitment_build_challenge(cmt.data(), cmt.size());
    auto cpace_t_pub = trezor.step_after_challenge(host.challenge().data(),
                                                   host.challenge().size());
    host.consume_cpace_trezor(cpace_t_pub.data(), cpace_t_pub.size());

    std::string wrong_code = trezor.code_str;
    if (wrong_code[5] == '9') wrong_code[5] = '0'; else wrong_code[5]++;
    host.build_host_tag(wrong_code);

    EXPECT_FALSE(host.consume_secret(trezor.secret, 16));
    EXPECT_FALSE(host.is_paired());
  }
}

// ---------------------------------------------------------------------------
// Code zero-padding: the spec requires the entered code to be hashed as
// exactly 6 ASCII digits, zero-padded on the left. The user types
// "942" but the Trezor displayed "000942" and hashed those 6 bytes:
// without padding the host computes a different CPace generator and the
// device rejects the host_tag. This test runs the full happy path with
// a code that genuinely needs padding (we brute-force a handshake_hash
// that yields a leading-zero code, then have the user type the
// leading-zero-stripped form).  This would silently fail before B3.
// ---------------------------------------------------------------------------
TEST(thp_pairing, code_entry_zero_padded_code_works)
{
  // Find a handshake_hash that produces a code with at least one
  // leading zero (i.e. code_int < 100000). The simulator's code is
  // derived from a SHA-256 over (0x02 || h || secret || challenge), so
  // varying h is enough; every ~10 attempts will yield a leading zero.
  NoiseHash h{};
  SimulatedTrezor trezor;
  std::vector<uint8_t> cmt_raw, cpace_t_pub_raw;
  std::unique_ptr<CodeEntryPairing> host;

  for (uint32_t seed = 0; seed < 256; ++seed) {
    h = NoiseHash{};
    for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t((seed * 7 + i) & 0xff);
    trezor = SimulatedTrezor{};
    trezor.set_handshake_hash(h);
    host.reset(new CodeEntryPairing(h));
    cmt_raw = trezor.step_select_method();
    host->consume_commitment_build_challenge(cmt_raw.data(), cmt_raw.size());
    cpace_t_pub_raw = trezor.step_after_challenge(host->challenge().data(),
                                                  host->challenge().size());
    if (trezor.code_int < 100000) break;
  }
  ASSERT_TRUE(host != nullptr);
  ASSERT_LT(trezor.code_int, 100000u) << "failed to find a leading-zero code";

  host->consume_cpace_trezor(cpace_t_pub_raw.data(), cpace_t_pub_raw.size());

  // User types the code WITHOUT the leading zero(s). Host must pad to
  // 6 digits internally before hashing.
  std::string unpadded = std::to_string(trezor.code_int);
  ASSERT_LT(unpadded.size(), 6u);

  const CpaceHostTag host_tag = host->build_host_tag(unpadded);

  // The Trezor (which hashed the 6-digit padded form) accepts the tag.
  auto secret_raw = trezor.step_consume_host_tag(
      host_tag.cpace_host_public_key.data(), host_tag.tag.data());
  EXPECT_TRUE(host->consume_secret(secret_raw.data(), secret_raw.size()));
  EXPECT_TRUE(host->is_paired());
}

// Reject obviously-malformed codes early instead of letting them flow
// into the CPace derivation.
//
// The FSM is primed with a REAL device CPace public key.  An earlier
// version primed it with an all-zero key, which crypto_scalarmult rejects
// on its own, so every case threw whether or not build_host_tag validated
// anything: it could not tell "rejected a bad code" from "choked on a
// degenerate key".  The positive control at the end is what makes the
// three rejections mean something.
TEST(thp_pairing, code_entry_rejects_malformed_codes)
{
  NoiseHash h{};
  for (size_t i = 0; i < h.size(); ++i) h[i] = uint8_t(0x90 + i);

  SimulatedTrezor trezor;
  trezor.set_handshake_hash(h);
  CodeEntryPairing host(h);

  auto cmt = trezor.step_select_method();
  host.consume_commitment_build_challenge(cmt.data(), cmt.size());
  auto cpace_t_pub = trezor.step_after_challenge(host.challenge().data(),
                                                 host.challenge().size());
  host.consume_cpace_trezor(cpace_t_pub.data(), cpace_t_pub.size());

  // 7-digit code: too long.
  EXPECT_THROW(host.build_host_tag("1234567"),
               hw::trezor::exc::ProtocolException);
  // Non-digits: reject.
  EXPECT_THROW(host.build_host_tag("12a456"),
               hw::trezor::exc::ProtocolException);
  // Empty: reject.
  EXPECT_THROW(host.build_host_tag(""),
               hw::trezor::exc::ProtocolException);

  // Positive control: a well-formed code on the same FSM state is accepted,
  // so the three rejections above are decisions about the code and not a
  // side effect of the state the FSM happens to be in.
  EXPECT_NO_THROW(host.build_host_tag(trezor.code_str));
}

TEST(thp_transport_cipher, seal_open_round_trip)
{
  NoiseKey key{};
  for (size_t i = 0; i < key.size(); ++i) key[i] = uint8_t(i);

  TransportCipher tx(key, /*initial_nonce=*/0);
  TransportCipher rx(key, /*initial_nonce=*/0);

  for (int i = 0; i < 4; ++i) {
    std::vector<uint8_t> pt(64, uint8_t(i + 1));
    std::vector<uint8_t> ct;
    tx.seal(nullptr, 0, pt.data(), pt.size(), ct);
    EXPECT_EQ(pt.size() + NOISE_TAGLEN, ct.size());

    std::vector<uint8_t> back;
    ASSERT_TRUE(rx.open(nullptr, 0, ct.data(), ct.size(), back));
    EXPECT_EQ(pt, back);
  }
  EXPECT_EQ(4u, tx.nonce());
  EXPECT_EQ(4u, rx.nonce());
}

// ---------------------------------------------------------------------------
// TransportCipher fail-closed nonce-exhaustion guard.
//
// The 96-bit AES-GCM IV is derived from a 64-bit counter; reusing a nonce
// under the same key is catastrophic for AES-GCM.  When the counter has
// reached UINT64_MAX the cipher MUST refuse rather than wrap:
//   - seal() throws hw::trezor::exc::SecurityException.
//   - open() returns false.
// In both cases the counter must NOT advance (no wrap to 0), so a repeated
// call behaves identically.  This is defence-in-depth, unreachable in
// practice at 2^64-1 frames, but a wrap would silently reuse nonce 0.
// ---------------------------------------------------------------------------
TEST(thp_transport_cipher, nonce_exhaustion_seal_throws_and_does_not_wrap)
{
  NoiseKey key{};
  for (size_t i = 0; i < key.size(); ++i) key[i] = uint8_t(0x11 * (i + 1));

  TransportCipher tx(key, /*initial_nonce=*/UINT64_MAX);
  ASSERT_EQ(UINT64_MAX, tx.nonce());

  std::vector<uint8_t> pt(8, 0xAB);
  std::vector<uint8_t> out;

  // First seal at the exhausted counter must fail closed.
  EXPECT_THROW(tx.seal(nullptr, 0, pt.data(), pt.size(), out),
               hw::trezor::exc::SecurityException);
  // Counter must not have advanced (no wrap to 0).
  EXPECT_EQ(UINT64_MAX, tx.nonce());

  // A second call still refuses identically: the guard is stable, not a
  // one-shot.
  EXPECT_THROW(tx.seal(nullptr, 0, pt.data(), pt.size(), out),
               hw::trezor::exc::SecurityException);
  EXPECT_EQ(UINT64_MAX, tx.nonce());
}

// The ciphertext here is GENUINELY VALID at nonce UINT64_MAX: it is
// produced by encrypting under the same key with the IV the exhausted
// counter would use.  That is what makes the test able to fail.  An
// earlier version fed 24 bytes of 0xCD, which the AES-GCM tag check
// rejects on its own, so both assertions held with the guard deleted and
// the test asserted nothing about the guard at all.
TEST(thp_transport_cipher, nonce_exhaustion_open_returns_false_and_does_not_wrap)
{
  NoiseKey key{};
  for (size_t i = 0; i < key.size(); ++i) key[i] = uint8_t(0x11 * (i + 1));

  const std::vector<uint8_t> pt(8, 0xAB);
  noise_crypto::NoiseIv iv{};
  noise_crypto::iv_for_nonce(UINT64_MAX, iv);
  std::vector<uint8_t> ct;
  ASSERT_TRUE(noise_crypto::aes256gcm_encrypt(key, iv, nullptr, 0,
                                              pt.data(), pt.size(), ct));
  ASSERT_EQ(pt.size() + NOISE_TAGLEN, ct.size());

  TransportCipher rx(key, /*initial_nonce=*/UINT64_MAX);
  ASSERT_EQ(UINT64_MAX, rx.nonce());

  // Without the guard this ciphertext decrypts and open() returns true.
  // With it, open() refuses (its failure convention is false, not a
  // throw) and the counter does not advance.
  std::vector<uint8_t> out;
  EXPECT_FALSE(rx.open(nullptr, 0, ct.data(), ct.size(), out));
  EXPECT_EQ(UINT64_MAX, rx.nonce());

  // Repeated call behaves identically: no wrap to 0.
  EXPECT_FALSE(rx.open(nullptr, 0, ct.data(), ct.size(), out));
  EXPECT_EQ(UINT64_MAX, rx.nonce());
}

// Sticky failure: one AEAD failure permanently disables the cipherstate,
// as the Noise spec and Trezor's own host require.  A peer that fails
// authentication once does not get to keep streaming frames, so a channel
// must not silently resume after one corrupt frame.
TEST(thp_transport_cipher, open_failure_is_sticky)
{
  NoiseKey key{};
  for (size_t i = 0; i < key.size(); ++i) key[i] = uint8_t(0x07 * (i + 1));

  TransportCipher tx(key, /*initial_nonce=*/0);
  TransportCipher rx(key, /*initial_nonce=*/0);

  const std::vector<uint8_t> pt(16, 0x5A);
  std::vector<uint8_t> ct;
  tx.seal(nullptr, 0, pt.data(), pt.size(), ct);

  // Corrupt the frame in flight: open() rejects it and latches.
  std::vector<uint8_t> tampered = ct;
  tampered[0] ^= 0x01;
  std::vector<uint8_t> out;
  EXPECT_FALSE(rx.open(nullptr, 0, tampered.data(), tampered.size(), out));

  // The untampered frame at the same counter would otherwise open cleanly;
  // after the latch it does not, and the counter has not advanced.
  EXPECT_FALSE(rx.open(nullptr, 0, ct.data(), ct.size(), out));
  EXPECT_EQ(0u, rx.nonce());
}

// ---------------------------------------------------------------------------
// ThpStore round-trip.  Persist a host static keypair plus two KnownDevice
// records (with non-trivial pairing-credential bytes) to a temp file, read
// it back into a fresh store, and assert every field
// survives byte-for-byte.  Uses a unique temp path (never the user's real
// ~/.trezor/thp_store.bin) and removes it afterward.
// ---------------------------------------------------------------------------
namespace {

// Small RAII helper: a unique temp path that is removed on scope exit.
struct ScopedTempPath {
  boost::filesystem::path p;
  ScopedTempPath()
    : p(boost::filesystem::temp_directory_path() /
        boost::filesystem::unique_path("monero_thp_store_%%%%-%%%%.bin")) {}
  ~ScopedTempPath() {
    boost::system::error_code ec;
    boost::filesystem::remove(p, ec);  // best-effort cleanup
  }
  std::string str() const { return p.string(); }
};

} // namespace

TEST(thp_store, round_trip_preserves_all_fields)
{
  ScopedTempPath tmp;

  // Host static keypair with distinctive, non-trivial bytes.
  HostStaticKey host;
  for (size_t i = 0; i < host.pub.size(); ++i)  host.pub[i]  = uint8_t(0x10 + i);
  for (size_t i = 0; i < host.priv.size(); ++i) host.priv[i] = uint8_t(0xA0 + i);

  // Device 1: distinct static pubkey, per-device host keypair and a
  // non-trivial pairing credential.
  KnownDevice kd1;
  for (size_t i = 0; i < kd1.trezor_static_pubkey.size(); ++i)
    kd1.trezor_static_pubkey[i] = uint8_t(0x01 + i);
  for (size_t i = 0; i < kd1.host_static.pub.size(); ++i)
    kd1.host_static.pub[i] = uint8_t(0x21 + i);
  for (size_t i = 0; i < kd1.host_static.priv.size(); ++i)
    kd1.host_static.priv[i] = uint8_t(0x41 + i);
  kd1.pairing_credential = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x7F, 0x80, 0xFF};

  // Device 2: different values throughout, including a longer credential
  // blob, to exercise variable-length records.
  KnownDevice kd2;
  for (size_t i = 0; i < kd2.trezor_static_pubkey.size(); ++i)
    kd2.trezor_static_pubkey[i] = uint8_t(0xF0 - i);
  for (size_t i = 0; i < kd2.host_static.pub.size(); ++i)
    kd2.host_static.pub[i] = uint8_t(0x5A ^ i);
  for (size_t i = 0; i < kd2.host_static.priv.size(); ++i)
    kd2.host_static.priv[i] = uint8_t(0xC3 ^ i);
  kd2.pairing_credential.assign(64, 0);
  for (size_t i = 0; i < kd2.pairing_credential.size(); ++i)
    kd2.pairing_credential[i] = uint8_t((i * 37 + 11) & 0xFF);

  // Write via a real ThpStore.  load_or_init on a non-existent path yields
  // an empty store; we then set the host key and upsert the devices, save.
  {
    ThpStore store;
    store.load_or_init(tmp.str());
    store.set_host_static(host);
    store.upsert_known_device(kd1);
    store.upsert_known_device(kd2);
    store.save(tmp.str());
  }
  ASSERT_TRUE(boost::filesystem::exists(tmp.p));

  // Read back into a fresh store.
  ThpStore loaded;
  loaded.load_or_init(tmp.str());

  // Host static key survives byte-for-byte.
  EXPECT_EQ(0, std::memcmp(loaded.host_static().pub.data(),
                           host.pub.data(), host.pub.size()));
  EXPECT_EQ(0, std::memcmp(loaded.host_static().priv.data(),
                           host.priv.data(), host.priv.size()));

  // Both device records survive, in insertion order.
  ASSERT_EQ(2u, loaded.known_devices().size());
  const KnownDevice &g1 = loaded.known_devices()[0];
  const KnownDevice &g2 = loaded.known_devices()[1];

  EXPECT_EQ(0, std::memcmp(g1.trezor_static_pubkey.data(),
                           kd1.trezor_static_pubkey.data(),
                           kd1.trezor_static_pubkey.size()));
  EXPECT_EQ(0, std::memcmp(g1.host_static.pub.data(),
                           kd1.host_static.pub.data(),
                           kd1.host_static.pub.size()));
  EXPECT_EQ(0, std::memcmp(g1.host_static.priv.data(),
                           kd1.host_static.priv.data(),
                           kd1.host_static.priv.size()));
  EXPECT_EQ(kd1.pairing_credential, g1.pairing_credential);

  EXPECT_EQ(0, std::memcmp(g2.trezor_static_pubkey.data(),
                           kd2.trezor_static_pubkey.data(),
                           kd2.trezor_static_pubkey.size()));
  EXPECT_EQ(0, std::memcmp(g2.host_static.pub.data(),
                           kd2.host_static.pub.data(),
                           kd2.host_static.pub.size()));
  EXPECT_EQ(0, std::memcmp(g2.host_static.priv.data(),
                           kd2.host_static.priv.data(),
                           kd2.host_static.priv.size()));
  EXPECT_EQ(kd2.pairing_credential, g2.pairing_credential);
}

// ---------------------------------------------------------------------------
// ThpStore malformed-input handling.
//
// Contract (store.cpp): the deserialiser throws hw::trezor::exc::
// EncodingException on bad magic / truncated / overflowing input, and the
// public load_or_init() catches that, logs a warning, and re-initialises to
// an EMPTY store (so a corrupt file forces a clean re-pair rather than
// bricking wallet-open or crashing).  Every case below must therefore:
//   - NOT crash / NOT UB, and
//   - leave the store in a clean empty state (no host static, no devices).
// We exercise the failure surface through the public API.
//
// Helper: write raw bytes to a path, then load and assert the store ended
// up empty (graceful rejection).
namespace {

void write_raw_file(const std::string &path, const std::vector<uint8_t> &bytes)
{
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  ASSERT_TRUE(f.good());
  if (!bytes.empty())
    f.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  f.flush();
  ASSERT_TRUE(f.good());
}

void expect_empty_store_after_load(const std::string &path)
{
  ThpStore store;
  // Must not throw / crash on malformed input: it discards and re-inits.
  ASSERT_NO_THROW(store.load_or_init(path));
  // No partial-corrupt state: no devices, and the host static key is the
  // default all-zero value (i.e. it was reset, not partially populated).
  EXPECT_TRUE(store.known_devices().empty());
  HostStaticKey zero{};
  EXPECT_EQ(0, std::memcmp(store.host_static().pub.data(),
                           zero.pub.data(), zero.pub.size()));
  EXPECT_EQ(0, std::memcmp(store.host_static().priv.data(),
                           zero.priv.data(), zero.priv.size()));
}

// The valid 8-byte magic header that prefixes every well-formed store file.
const std::vector<uint8_t> kStoreMagic =
    {'M','T','H','P','S','T','R','2'};

} // namespace

// (a) Truncated file: valid magic + a record header whose declared length
//     runs past the end of the buffer.  Must be rejected gracefully.
TEST(thp_store, malformed_truncated_record_body_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x01);  // TAG_HOST_STATIC_PUB
  bytes.push_back(0x20);  // varint length = 32
  // ...but only supply 4 of the promised 32 bytes -> body cut off.
  bytes.insert(bytes.end(), {0xAA, 0xBB, 0xCC, 0xDD});
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (b) Bogus/oversized length varint that would over-read far past the
//     buffer.  The deserialiser must bound-check and reject, not over-read.
TEST(thp_store, malformed_oversized_length_varint_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x01);  // TAG_HOST_STATIC_PUB
  // Multi-byte varint encoding an enormous length (~0xFFFFFFF) that vastly
  // exceeds the remaining buffer -> off + len > buf.size().
  bytes.insert(bytes.end(), {0xFF, 0xFF, 0xFF, 0x7F});
  bytes.insert(bytes.end(), {0x00, 0x01, 0x02});  // a few real bytes
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (c) Wrong / absent magic byte: a non-empty file that does not begin with
//     the MTHPSTR2 magic.  Rejected gracefully.
TEST(thp_store, malformed_wrong_magic_rejected)
{
  ScopedTempPath tmp;
  // Eight bytes that are the wrong magic, plus some trailing bytes.
  std::vector<uint8_t> bytes =
      {'M','T','H','P','S','T','R','1',  // legacy/wrong magic
       0x01, 0x20, 0x00, 0x00};
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (d) Empty file: present on disk but zero bytes -> shorter than the magic
//     header, so it fails the magic check and is rejected gracefully.
TEST(thp_store, malformed_empty_file_rejected)
{
  ScopedTempPath tmp;
  write_raw_file(tmp.str(), std::vector<uint8_t>{});
  ASSERT_TRUE(boost::filesystem::exists(tmp.p));

  expect_empty_store_after_load(tmp.str());
}

// A 10-byte varint encoding 2^64-1.  read_varint accepts it (the shift cap
// is 63 and the final byte contributes bit 63), so the *value* check in the
// deserialiser is the only line of defence.
const std::vector<uint8_t> kVarintU64Max =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01};

// (e) Length varint near 2^64: `off + len` wraps modulo 2^64 to a small
//     value, so a naive `off + len > buf.size()` truncation check passes
//     and the KNOWN_DEVICE inner loop reads attacker-chosen lengths past
//     the end of the buffer (heap over-read).  The bound must be computed
//     subtraction-side (`len > buf.size() - off`) to be wrap-safe.
TEST(thp_store, malformed_varint_wraparound_known_device_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x03);  // TAG_KNOWN_DEVICE
  bytes.insert(bytes.end(), kVarintU64Max.begin(), kVarintU64Max.end());
  bytes.insert(bytes.end(), {0x01, 0x20, 0xAA});  // a few inner-record bytes
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (f) Same wraparound under an unknown (skipped) tag: the skip path
//     `off += len` would wrap `off` backwards and re-parse the same bytes
//     forever, a hang the load_or_init catch cannot recover from.
TEST(thp_store, malformed_varint_wraparound_unknown_tag_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x7E);  // unknown tag -> forward-compat skip path
  bytes.insert(bytes.end(), kVarintU64Max.begin(), kVarintU64Max.end());
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// The three guards below live inside a KNOWN_DEVICE record, where the
// spill/wrap arithmetic is subtlest and where a random buffer reaches them
// only by luck.  Each case is built so that exactly one guard is the reason
// the record is rejected.
//
// (g) An inner varint whose continuation bytes run past the end of the
//     whole buffer.  read_varint bounds against the buffer, so this is the
//     "truncated inner varint" branch.
TEST(thp_store, malformed_inner_varint_truncated_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x03);  // TAG_KNOWN_DEVICE
  bytes.push_back(0x02);  // outer length = 2, which is all that remains
  bytes.push_back(0x02);  // KD_TAG_PAIRING_CREDENTIAL
  bytes.push_back(0x80);  // varint continues, but the buffer ends here
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (h) An inner varint that is complete, but only because it reads bytes
//     belonging to whatever follows the KNOWN_DEVICE record.  read_varint
//     cannot see the inner boundary, so the deserialiser checks it
//     afterwards ("inner varint past record").  Without that check a
//     hostile store steers the inner cursor using bytes outside its own
//     record.
TEST(thp_store, malformed_inner_varint_spilling_past_record_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x03);  // TAG_KNOWN_DEVICE
  bytes.push_back(0x02);  // outer length = 2
  bytes.push_back(0x02);  // KD_TAG_PAIRING_CREDENTIAL
  bytes.push_back(0x80);  // last byte of the record, varint incomplete
  bytes.push_back(0x01);  // outside the record: completes the varint
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (i) A well-formed inner varint declaring more bytes than the record has
//     left.  The bound has to be computed subtraction-side
//     (ilen > len - inner_off); the addition form wraps and admits a heap
//     over-read.
TEST(thp_store, malformed_inner_record_overflow_rejected)
{
  ScopedTempPath tmp;
  std::vector<uint8_t> bytes = kStoreMagic;
  bytes.push_back(0x03);              // TAG_KNOWN_DEVICE
  bytes.push_back(0x05);              // outer length = 5
  bytes.push_back(0x02);              // KD_TAG_PAIRING_CREDENTIAL
  bytes.push_back(0x20);              // inner length = 32, but 3 bytes left
  bytes.insert(bytes.end(), {0xAA, 0xBB, 0xCC});
  write_raw_file(tmp.str(), bytes);

  expect_empty_store_after_load(tmp.str());
}

// (j) Structure-aware fuzz battery.  The hand-written cases above pin the
//     known structural edges (truncation, oversized & wrap-around varints,
//     bad/legacy magic, the three inner-record guards).  This sprays a
//     deterministic spread of random tag/length/body permutations at the
//     public load path to cover the space between them.
//
//     Contract under test: load_or_init() must NEVER throw, hang, crash, or
//     leave UB behind, for ANY input.  We do NOT assert the store ends empty
//     (a random buffer can coincidentally encode a valid record, which the
//     parser is allowed to load), only that loading and then touching the
//     loaded state is always safe.  Fixed PRNG seed => any failure is
//     bit-for-bit reproducible from the reported iteration.
TEST(thp_store, malformed_fuzz_battery_rejected)
{
  // splitmix64: tiny, deterministic, well-distributed, enough to spray the
  // parser without pulling in <random> machinery.
  uint64_t state = 0x0123456789abcdefULL;
  auto next = [&state]() -> uint64_t {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  };
  auto byte = [&next]() -> uint8_t { return uint8_t(next() & 0xFF); };

  ScopedTempPath tmp;
  for (int iter = 0; iter < 512; ++iter) {
    std::vector<uint8_t> bytes;
    const int    mode = int(next() % 4);
    const size_t len  = size_t(next() % 64);

    switch (mode) {
      case 0:  // pure random bytes (usually fails the magic check)
        for (size_t i = 0; i < len; ++i) bytes.push_back(byte());
        break;
      case 1:  // valid magic + random tail
        bytes = kStoreMagic;
        for (size_t i = 0; i < len; ++i) bytes.push_back(byte());
        break;
      case 2: {  // magic + a few random (tag, multi-byte varint, body) records
        bytes = kStoreMagic;
        const int records = int(next() % 4);
        for (int r = 0; r < records; ++r) {
          bytes.push_back(byte());                    // random tag
          const int vlen = 1 + int(next() % 6);       // 1..6 varint bytes
          for (int v = 0; v < vlen; ++v)              // continuation bit on all but last
            bytes.push_back(uint8_t((byte() & 0x7F) | (v + 1 < vlen ? 0x80 : 0x00)));
          const size_t body = size_t(next() % 40);
          for (size_t b = 0; b < body; ++b) bytes.push_back(byte());
        }
        break;
      }
      default: {  // a plausible record truncated at a random offset
        bytes = kStoreMagic;
        bytes.push_back(uint8_t(next() % 6));          // a low tag value
        bytes.push_back(0x20);                          // claims a 32-byte body
        const size_t supplied = size_t(next() % 33);    // 0..32 actually provided
        for (size_t b = 0; b < supplied; ++b) bytes.push_back(byte());
        break;
      }
    }

    write_raw_file(tmp.str(), bytes);

    ThpStore store;
    ASSERT_NO_THROW(store.load_or_init(tmp.str()))
        << "load_or_init threw on iter=" << iter << " mode=" << mode
        << " (" << bytes.size() << " bytes)";
    // Touch whatever loaded: a parser bug that produced an oversized or
    // malformed device entry would surface as UB / a throw here.
    ASSERT_NO_THROW({
      for (const auto &kd : store.known_devices()) {
        (void)kd.trezor_static_pubkey.size();
        (void)kd.pairing_credential.size();
      }
      (void)store.host_static().pub[0];
    }) << "touching loaded state threw on iter=" << iter << " mode=" << mode;
  }
}

// ---------------------------------------------------------------------------
// ThpStore concurrent-writer semantics.  The store is per-user and shared
// by every wallet process (CLI and GUI may run at once), so save() folds in
// devices that another process persisted after we loaded instead of
// overwriting the whole file with our snapshot.
// ---------------------------------------------------------------------------
namespace {

KnownDevice make_known_device(uint8_t seed)
{
  KnownDevice kd;
  for (size_t i = 0; i < kd.trezor_static_pubkey.size(); ++i)
    kd.trezor_static_pubkey[i] = uint8_t(seed + i);
  for (size_t i = 0; i < kd.host_static.pub.size(); ++i)
    kd.host_static.pub[i] = uint8_t(seed ^ (0x30 + i));
  for (size_t i = 0; i < kd.host_static.priv.size(); ++i)
    kd.host_static.priv[i] = uint8_t(seed ^ (0x60 + i));
  kd.pairing_credential = {seed, uint8_t(seed + 1), uint8_t(seed + 2)};
  return kd;
}

bool store_has_device(const ThpStore &store, const KnownDevice &kd)
{
  for (const auto &got : store.known_devices()) {
    if (std::memcmp(got.trezor_static_pubkey.data(),
                    kd.trezor_static_pubkey.data(),
                    kd.trezor_static_pubkey.size()) == 0)
      return true;
  }
  return false;
}

} // namespace

// Two stores load the same (initially missing) file, each pairs a different
// device, and both save.  The second save must preserve the first writer's
// device instead of clobbering it; losing it would silently force a
// re-pair the next time that device is used.
TEST(thp_store, save_merges_devices_from_concurrent_writer)
{
  ScopedTempPath tmp;
  HostStaticKey host;
  for (size_t i = 0; i < host.pub.size(); ++i)  host.pub[i]  = uint8_t(0x10 + i);
  for (size_t i = 0; i < host.priv.size(); ++i) host.priv[i] = uint8_t(0xA0 + i);

  const KnownDevice device_x = make_known_device(0x01);
  const KnownDevice device_y = make_known_device(0x80);

  ThpStore a, b;
  a.load_or_init(tmp.str());  // both load while the file is absent
  b.load_or_init(tmp.str());
  a.set_host_static(host);
  b.set_host_static(host);
  a.upsert_known_device(device_x);
  b.upsert_known_device(device_y);

  a.save(tmp.str());
  b.save(tmp.str());  // unaware of device_x; must fold it in, not drop it

  ThpStore loaded;
  loaded.load_or_init(tmp.str());
  ASSERT_EQ(2u, loaded.known_devices().size());
  EXPECT_TRUE(store_has_device(loaded, device_x));
  EXPECT_TRUE(store_has_device(loaded, device_y));
}

// When both the disk file and the saving store know the SAME device, the
// in-memory entry wins: we may have just re-paired it, so our credential is
// the fresher one.
TEST(thp_store, save_conflicting_device_in_memory_entry_wins)
{
  ScopedTempPath tmp;
  HostStaticKey host;
  for (size_t i = 0; i < host.pub.size(); ++i)  host.pub[i]  = uint8_t(0x10 + i);
  for (size_t i = 0; i < host.priv.size(); ++i) host.priv[i] = uint8_t(0xA0 + i);

  KnownDevice stale = make_known_device(0x01);
  KnownDevice fresh = stale;
  fresh.pairing_credential = {0xCA, 0xFE, 0xBA, 0xBE};

  {
    ThpStore first;
    first.load_or_init(tmp.str());
    first.set_host_static(host);
    first.upsert_known_device(stale);
    first.save(tmp.str());
  }
  {
    ThpStore second;
    second.load_or_init(tmp.str());  // sees the stale credential
    second.upsert_known_device(fresh);
    second.save(tmp.str());
  }

  ThpStore loaded;
  loaded.load_or_init(tmp.str());
  ASSERT_EQ(1u, loaded.known_devices().size());
  EXPECT_EQ(fresh.pairing_credential,
            loaded.known_devices()[0].pairing_credential);
}

// ---------------------------------------------------------------------------
// Scripted transport, and the protocol layers driven through it.
//
// A trezor-core protocol-v1 device (Model T / Safe 3 / Safe 5) answers the
// THP broadcast probe with a codec_v1-framed Failure report ('?','#','#',
// msg_type, ...).  The allocation reader must recognise that as a legacy
// device rather than abort the connection, because the alternative breaks
// every v1 device on every connect.
// ---------------------------------------------------------------------------
namespace {

// Minimal scripted Transport: write_chunk records what the host emitted;
// the timed read_chunk serves the scripted frames in order, then times out.
// Scripts are generators so a response can be built from what the host has
// written by then (the allocation nonce and every handshake body are
// generated per call and cannot be baked into a fixture).
//
// A generator returns one whole encoded frame, which encode_frame has
// already padded to a whole number of chunks.  read_chunk hands out one
// chunk per call and parks the remainder, exactly as the 64-byte USB
// carrier delivers it: a HandshakeInitiationResponse carries 96 payload
// bytes and so spans two chunks, and serving only its first chunk would
// silently drop the second and pull the *following* generator in
// mid-frame, one response short for the rest of the script.
struct ScriptedTransport : public hw::trezor::Transport {
  std::vector<std::function<std::vector<uint8_t>()>> reads;
  size_t next_read = 0;
  std::vector<uint8_t> pending_read;   // frame handed out chunk by chunk
  size_t pending_read_off = 0;
  std::vector<uint8_t> last_written;
  std::vector<std::vector<uint8_t>> written_chunks;
  size_t next_unread_chunk = 0;

  void write(const google::protobuf::Message &) override {}
  void read(std::shared_ptr<google::protobuf::Message> &,
            hw::trezor::messages::MessageType *) override {}

  void write_chunk(const void *buff, size_t size) override {
    const uint8_t *p = static_cast<const uint8_t *>(buff);
    last_written.assign(p, p + size);
    written_chunks.push_back(last_written);
  }
  size_t read_chunk(void *buff, size_t size, unsigned int /*timeout_ms*/) override {
    if (pending_read_off >= pending_read.size()) {
      if (next_read >= reads.size())
        throw hw::trezor::exc::TimeoutException();
      pending_read = reads[next_read++]();
      pending_read_off = 0;
    }
    const size_t n = std::min(size, pending_read.size() - pending_read_off);
    std::memcpy(buff, pending_read.data() + pending_read_off, n);
    pending_read_off += n;
    return n;
  }

  // Reassemble the next frame the host wrote, in write order.  A script
  // that consumes exactly the frames it expects therefore also pins the
  // order in which the host writes them.
  Frame take_written_frame() {
    FrameAssembler asm_;
    while (next_unread_chunk < written_chunks.size()) {
      const auto &c = written_chunks[next_unread_chunk++];
      if (asm_.feed_chunk(c.data(), c.size())) return asm_.take();
    }
    throw std::runtime_error("ScriptedTransport: no complete frame written");
  }

  // Control byte of every initiation chunk written so far, in order.
  std::vector<uint8_t> written_control_bytes() const {
    std::vector<uint8_t> out;
    for (const auto &c : written_chunks) {
      if (!c.empty() && (c[0] & CTRL_CONTINUATION_BIT) == 0)
        out.push_back(c[0]);
    }
    return out;
  }
};

// A 64-byte codec_v1 report as a trezor-core v1 device sends it: magic
// '?','#','#', then big-endian msg_type 3 (Failure) and length.
std::vector<uint8_t> v1_codec_failure_report()
{
  std::vector<uint8_t> chunk(64, 0);
  chunk[0] = '?'; chunk[1] = '#'; chunk[2] = '#';
  chunk[4] = 0x03;  // msg_type = Failure
  chunk[8] = 0x02;  // payload length
  return chunk;
}

// A chunk that parses as a THP initiation packet but fails the CRC check.
std::vector<uint8_t> corrupt_thp_frame()
{
  const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  auto wire = encode_frame(CTRL_CHANNEL_ALLOC_RESPONSE, CID_BROADCAST,
                           payload, sizeof(payload));
  wire[5] ^= 0x01;  // corrupt the first payload byte
  return wire;
}

// Queue the ChannelAllocationResponse a THP device sends for the broadcast
// probe: the host's own 8-byte nonce echoed back, the granted CID, and the
// device properties the handshake later mixes in as its Noise prologue.
void script_allocation(ScriptedTransport &transport, uint16_t cid,
                       const std::vector<uint8_t> &device_properties)
{
  ScriptedTransport *t = &transport;
  const std::vector<uint8_t> props = device_properties;
  transport.reads.push_back([t, cid, props] {
    const Frame req = t->take_written_frame();
    std::vector<uint8_t> payload(req.payload.begin(), req.payload.begin() + 8);
    payload.push_back(uint8_t(cid >> 8));
    payload.push_back(uint8_t(cid & 0xFF));
    payload.insert(payload.end(), props.begin(), props.end());
    return encode_frame(CTRL_CHANNEL_ALLOC_RESPONSE, CID_BROADCAST,
                        payload.data(), payload.size());
  });
}

// Queue the four reads a THP device produces for one full Noise XX
// handshake on `cid`: ACK of the initiation request, the initiation
// response, ACK of the completion request, and the completion response.
// `dev` plays the device side and must outlive the transport.
void script_handshake(ScriptedTransport &transport, TrezorSide &device,
                      uint16_t cid)
{
  ScriptedTransport *t = &transport;
  TrezorSide        *d = &device;

  transport.reads.push_back([t, d, cid] {
    const Frame req = t->take_written_frame();
    d->handle_init_request(req.payload);
    return encode_frame(CTRL_ACK_SEQ0, cid, nullptr, 0);
  });
  transport.reads.push_back([d, cid] {
    return encode_frame(CTRL_HANDSHAKE_INIT_RESP, cid,
                        d->handshake_init_response.data(),
                        d->handshake_init_response.size());
  });
  transport.reads.push_back([t, d, cid] {
    (void)t->take_written_frame();               // host ACK of the response
    const Frame comp = t->take_written_frame();  // HandshakeCompletionRequest
    d->handle_completion_request(comp.payload);
    return encode_frame(CTRL_ACK_SEQ1, cid, nullptr, 0);
  });
  transport.reads.push_back([d, cid] {
    // Device sequence bit is 1 on the completion response (wire 0x13).
    return encode_frame(CTRL_HANDSHAKE_COMP_RESP | CTRL_DATA_SEQ_BIT, cid,
                        d->handshake_completion_response.data(),
                        d->handshake_completion_response.size());
  });
}

} // namespace

// The codec_v1 magic is proof of a legacy device, so it short-circuits the
// probe instead of costing the caller the full allocation budget.  A valid
// allocation response is queued directly behind the v1 report: only a
// short-circuit stops the reader consuming it and reporting `allocated`,
// so this distinguishes "recognised the v1 magic" from "gave up".
TEST(thp_channel, codec_v1_reply_short_circuits_allocation)
{
  ScriptedTransport t;
  t.reads.push_back([] { return v1_codec_failure_report(); });
  script_allocation(t, 0x1234, {});

  AllocatedChannel ch;
  EXPECT_EQ(AllocationResult::codec_v1_reply,
            allocate_channel(t, ch, /*timeout_ms=*/50));
  EXPECT_EQ(1u, t.next_read) << "the reader must stop at the codec_v1 magic";
  EXPECT_EQ(0, ch.channel_id) << "no allocation may be reported";
}

// Legacy firmware that ignores the unknown packet outright.
TEST(thp_channel, silent_device_reports_no_response)
{
  ScriptedTransport t;  // no scripted reads: every read times out

  AllocatedChannel ch;
  EXPECT_EQ(AllocationResult::no_response,
            allocate_channel(t, ch, /*timeout_ms=*/50));
  EXPECT_EQ(0, ch.channel_id);
}

TEST(thp_channel, allocation_resyncs_after_unparseable_chunk)
{
  ScriptedTransport t;
  // First read: a THP-shaped chunk with a broken CRC, which the assembler
  // rejects.  The probe must discard it, reset the assembler and keep
  // waiting; the second read is a well-formed allocation response echoing
  // the nonce from the request and granting channel 0x1234.
  t.reads.push_back([] { return corrupt_thp_frame(); });
  script_allocation(t, 0x1234, {0x0a, 0x04, 'T', 'S', '7', 'x'});

  AllocatedChannel got;
  ASSERT_EQ(AllocationResult::allocated,
            allocate_channel(t, got, /*timeout_ms=*/50));
  EXPECT_EQ(0x1234, got.channel_id);
  ASSERT_EQ(8u, got.nonce.size());
  EXPECT_EQ(0, std::memcmp(got.nonce.data(), t.written_chunks[0].data() + 5, 8));
  EXPECT_EQ(std::vector<uint8_t>({0x0a, 0x04, 'T', 'S', '7', 'x'}),
            got.device_properties_pb);
}

// ---------------------------------------------------------------------------
// ProtocolV2, driven end to end against the simulated device.
//
// The framing tests above prove control bytes can be encoded; this proves
// ProtocolV2 emits the right ones in the right order.  The expected
// sequence is the whole alternating-bit protocol written out: the
// handshake's four host frames, then two request/response exchanges whose
// sequence bits must toggle on both directions independently.  Anything
// that wedges the sequencing (the 0x08-vs-0x10 bug this file already
// carries a constant-level guard for, an ACK that is not sent, a counter
// that does not toggle) changes this list.
//
// It also pins the two things that are invisible from the framing layer:
// the 3-byte encrypted-transport plaintext header, and the cipherstate
// nonces (send starts at 0, receive at 1, because the completion response
// already consumed response nonce 0).  Both are asserted by the device
// side actually decrypting what the host sent and the host actually
// decrypting what the device sealed.
// ---------------------------------------------------------------------------
TEST(thp_protocol_v2, write_read_control_byte_sequence_matches_spec)
{
  constexpr uint16_t kCid = 0x1234;

  TrezorSide dev;
  noise_crypto::x25519_keypair(dev.static_pub, dev.static_priv);
  dev.device_properties = {0x0a, 0x04, 'T', 'S', '7', 'x'};

  ScriptedTransport t;
  script_handshake(t, dev, kCid);

  // The device's cipherstates mirror the host's; they can only be built
  // once the handshake has produced the keys, so the first application
  // read constructs them.
  std::unique_ptr<TransportCipher> dev_recv, dev_send;
  std::vector<std::vector<uint8_t>> device_saw;
  uint16_t success_wire = 0;

  // A device response carrying an empty common.Success on session 0.
  auto seal_success = [&](uint8_t control) {
    hw::trezor::messages::common::Success ok;
    success_wire = static_cast<uint16_t>(
        hw::trezor::MessageMapper::get_message_wire_number(ok));
    std::vector<uint8_t> plain(3);
    plain[0] = 0x00;
    write_be16(plain.data() + 1, success_wire);
    std::vector<uint8_t> sealed;
    dev_send->seal(nullptr, 0, plain.data(), plain.size(), sealed);
    return encode_frame(control, kCid, sealed.data(), sealed.size());
  };

  // Exchange 1: host request (seq 0) -> ACK, then device response (seq 0).
  t.reads.push_back([&] {
    (void)t.take_written_frame();  // host ACK of the completion response
    const Frame req = t.take_written_frame();
    EXPECT_EQ(CTRL_ENCRYPTED_TRANSPORT, req.control_byte);
    dev_recv.reset(new TransportCipher(dev.keys.key_request,  /*nonce=*/0));
    dev_send.reset(new TransportCipher(dev.keys.key_response, /*nonce=*/1));
    std::vector<uint8_t> plain;
    EXPECT_TRUE(dev_recv->open(nullptr, 0, req.payload.data(),
                               req.payload.size(), plain));
    device_saw.push_back(plain);
    return encode_frame(CTRL_ACK_SEQ0, kCid, nullptr, 0);
  });
  t.reads.push_back([&] { return seal_success(CTRL_ENCRYPTED_TRANSPORT); });

  // Exchange 2: the sequence bit must have toggled on both directions.
  t.reads.push_back([&] {
    (void)t.take_written_frame();  // host ACK of the first response
    const Frame req = t.take_written_frame();
    EXPECT_EQ(CTRL_ENCRYPTED_TRANSPORT | CTRL_DATA_SEQ_BIT, req.control_byte);
    std::vector<uint8_t> plain;
    EXPECT_TRUE(dev_recv->open(nullptr, 0, req.payload.data(),
                               req.payload.size(), plain));
    device_saw.push_back(plain);
    return encode_frame(CTRL_ACK_SEQ1, kCid, nullptr, 0);
  });
  t.reads.push_back([&] {
    return seal_success(CTRL_ENCRYPTED_TRANSPORT | CTRL_DATA_SEQ_BIT);
  });

  AllocatedChannel alloc;
  alloc.channel_id = kCid;
  alloc.nonce.assign(8, 0);
  alloc.device_properties_pb = dev.device_properties;

  ProtocolV2 v2;
  v2.adopt_allocation(alloc);
  ASSERT_NO_THROW(v2.session_begin(t));
  EXPECT_EQ(kCid, v2.channel_id());

  for (int i = 0; i < 2; ++i) {
    SCOPED_TRACE(i);
    hw::trezor::messages::management::Initialize req;
    ASSERT_NO_THROW(v2.write(t, req));

    std::shared_ptr<google::protobuf::Message> resp;
    hw::trezor::messages::MessageType mt{};
    ASSERT_NO_THROW(v2.read(t, resp, &mt));
    ASSERT_TRUE(resp != nullptr);
    EXPECT_EQ(success_wire, static_cast<uint16_t>(mt));
  }

  // The control bytes the host put on the wire, in order:
  //   0x00 HandshakeInitiationRequest      (data, seq 0)
  //   0x20 ACK seq 0                       (of the initiation response)
  //   0x12 HandshakeCompletionRequest      (data, seq 1 at bit 0x10)
  //   0x28 ACK seq 1                       (of the completion response)
  //   0x04 encrypted transport, seq 0      (first request)
  //   0x20 ACK seq 0                       (of the first response)
  //   0x14 encrypted transport, seq 1      (second request)
  //   0x28 ACK seq 1                       (of the second response)
  const std::vector<uint8_t> expected = {
    CTRL_HANDSHAKE_INIT_REQ,
    CTRL_ACK_SEQ0,
    CTRL_HANDSHAKE_COMP_REQ | CTRL_DATA_SEQ_BIT,
    CTRL_ACK_SEQ1,
    CTRL_ENCRYPTED_TRANSPORT,
    CTRL_ACK_SEQ0,
    CTRL_ENCRYPTED_TRANSPORT | CTRL_DATA_SEQ_BIT,
    CTRL_ACK_SEQ1,
  };
  EXPECT_EQ(expected, t.written_control_bytes());

  // Both requests decrypted on the device side, which is only possible if
  // the host's send counter started at 0 and advanced by exactly one per
  // frame.  Their plaintext is the 3-byte header and nothing else: a
  // management::Initialize is translated to an empty GetFeatures, because
  // THP devices reject Initialize as Failure_UnexpectedMessage.
  ASSERT_EQ(2u, device_saw.size());
  for (const auto &plain : device_saw) {
    ASSERT_EQ(3u, plain.size());
    EXPECT_EQ(0x00, plain[0]) << "session id byte";
    EXPECT_EQ(static_cast<uint16_t>(hw::trezor::messages::MessageType_GetFeatures),
              read_be16(plain.data() + 1));
  }

  // Every scripted device response was consumed exactly once: the host
  // neither skipped an exchange nor read one response short (the latter
  // would leave a later script entry running against a frame the host has
  // not written yet).
  EXPECT_EQ(t.reads.size(), t.next_read);
}

// ---------------------------------------------------------------------------
// ProtocolAutoDetect: the v1-vs-v2 decision, and the two states in which
// bring-up must fail closed rather than continue on an unauthenticated
// channel.
// ---------------------------------------------------------------------------
TEST(thp_auto_detect, silent_device_selects_protocol_v1)
{
  ScriptedTransport t;  // legacy firmware ignores the allocation request

  ProtocolAutoDetect ad;
  ad.configure(ProtocolConfig{});
  ASSERT_NO_THROW(ad.session_begin(t));
  EXPECT_STREQ("v1", ad.selected_protocol());
}

TEST(thp_auto_detect, codec_v1_reply_selects_protocol_v1)
{
  ScriptedTransport t;
  t.reads.push_back([] { return v1_codec_failure_report(); });

  ProtocolAutoDetect ad;
  ad.configure(ProtocolConfig{});
  ASSERT_NO_THROW(ad.session_begin(t));
  EXPECT_STREQ("v1", ad.selected_protocol());
}

// The control bytes a fail-closed bring-up puts on the wire and no more:
// the broadcast allocation request, the four handshake frames, and then
// nothing.  Both refusal tests below assert this, because it is what
// separates "reached the state check and refused" from "died earlier" and
// from "refused only after talking on the channel it would not vouch for".
std::vector<uint8_t> refused_handshake_control_bytes()
{
  return {
    CTRL_CHANNEL_ALLOC_REQUEST,
    CTRL_HANDSHAKE_INIT_REQ,
    CTRL_ACK_SEQ0,
    CTRL_HANDSHAKE_COMP_REQ | CTRL_DATA_SEQ_BIT,
    CTRL_ACK_SEQ1,
  };
}

// TOFU anti-spoof: a device that claims an existing pairing must be one we
// hold a credential for.  With no store configured there are no known
// devices, so STATE_PAIRED is unsupported by anything on this host and the
// bring-up must refuse instead of continuing on a channel whose peer it
// cannot place.
TEST(thp_auto_detect, paired_device_not_in_known_devices_is_refused)
{
  constexpr uint16_t kCid = 0x2222;

  TrezorSide dev;
  noise_crypto::x25519_keypair(dev.static_pub, dev.static_priv);
  dev.device_properties = {0x0a, 0x04, 'T', 'S', '7', 'x'};
  dev.state = STATE_PAIRED;

  ScriptedTransport t;
  script_allocation(t, kCid, dev.device_properties);
  script_handshake(t, dev, kCid);

  ProtocolAutoDetect ad;
  ad.configure(ProtocolConfig{});
  try {
    ad.session_begin(t);
    ADD_FAILURE() << "STATE_PAIRED with an empty known-devices list must be refused";
  } catch (const hw::trezor::exc::SecurityException &e) {
    // The v2 stack raises SecurityException from one other place (a failed
    // AES-GCM open in ProtocolV2::read), so match the refusal itself
    // rather than just its type.
    EXPECT_NE(std::string::npos, std::string(e.what()).find("not in our known-devices"))
        << "refused for the wrong reason: " << e.what();
  } catch (const std::exception &e) {
    ADD_FAILURE() << "expected a SecurityException, got: " << e.what();
  }

  // The whole handshake ran (all five scripted responses consumed) and the
  // refusal came out of the post-handshake state check, before a single
  // encrypted-transport frame was written.
  EXPECT_EQ(t.reads.size(), t.next_read);
  EXPECT_EQ(refused_handshake_control_bytes(), t.written_control_bytes());
  EXPECT_STREQ("unknown", ad.selected_protocol())
      << "a refused device must not be recorded as selected";
}

// STATE_UNPAIRED needs the CodeEntry pairing FSM, which needs the user.
// Without a prompt there is no way to authenticate the channel, so the
// bring-up must refuse rather than proceed unpaired.
TEST(thp_auto_detect, unpaired_device_without_pairing_prompt_is_refused)
{
  constexpr uint16_t kCid = 0x3333;

  TrezorSide dev;
  noise_crypto::x25519_keypair(dev.static_pub, dev.static_priv);
  dev.device_properties = {0x0a, 0x04, 'T', 'S', '7', 'x'};
  dev.state = STATE_UNPAIRED;

  ScriptedTransport t;
  script_allocation(t, kCid, dev.device_properties);
  script_handshake(t, dev, kCid);

  ProtocolConfig cfg;
  cfg.pairing_prompt = nullptr;  // no UI wired up

  ProtocolAutoDetect ad;
  ad.configure(cfg);
  try {
    ad.session_begin(t);
    ADD_FAILURE() << "STATE_UNPAIRED with no pairing prompt must be refused";
  } catch (const hw::trezor::exc::SecurityException &e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("no pairing prompt is configured"))
        << "refused for the wrong reason: " << e.what();
  } catch (const std::exception &e) {
    ADD_FAILURE() << "expected a SecurityException, got: " << e.what();
  }

  // As above: the refusal must land after a complete handshake and before
  // ThpPairingRequest, which is the first thing the pairing FSM would put
  // on the wire if the guard were removed.
  EXPECT_EQ(t.reads.size(), t.next_read);
  EXPECT_EQ(refused_handshake_control_bytes(), t.written_control_bytes());
  EXPECT_STREQ("unknown", ad.selected_protocol());
}

} // namespace

#endif // DEVICE_TREZOR_READY
