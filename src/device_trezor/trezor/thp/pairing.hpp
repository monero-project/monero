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
//

#ifndef MONERO_TREZOR_THP_PAIRING_H
#define MONERO_TREZOR_THP_PAIRING_H

#include "noise.hpp"
#include "wipeable_string.h"

#include <array>
#include <cstdint>
#include <string>

namespace hw {
namespace trezor {
namespace thp {

  // Elligator 2 map for Curve25519 (RFC 9380 section 6.7.1 and Appendix
  // G.2.1 / draft-irtf-cfrg-cpace-10 section 7.2, CPaceMontgomery). Maps a
  // uniform 32-byte input to a Curve25519 u-coordinate, which is the
  // password-binding step of CPace.
  //
  // `r_bytes` is interpreted little-endian with bit 255 cleared and reduced
  // mod p = 2^255 - 19; the u-coordinate is encoded little-endian.
  //
  // `r_bytes` carries the pairing code, so the map runs in constant time:
  // no branch and no memory index depends on it.
  void elligator2_curve25519(const uint8_t r_bytes[32], uint8_t out_u[32]);

  // CPace generator derivation per the THP spec, matching the IRTF
  // CPACE-X25519-SHA512 symmetric-setting suite:
  //   pregenerator = SHA-512(prefix || code || padding || handshake_hash || 0x00)[:32]
  //   generator    = ELLIGATOR2(pregenerator)
  // `prefix` and `padding` are the constants in specification.md, Notes,
  // under "Code Entry pairing sequence".
  //
  // `code` must be exactly six ASCII digits, e.g. "001234": the length is
  // baked into both constants, so any other length silently derives a
  // different domain. Throws otherwise. `handshake_hash` is the
  // post-handshake h.
  void cpace_derive_generator(const std::string &code,
                              const NoiseHash    &handshake_hash,
                              uint8_t out_generator[32]);

  // Contents of ThpCodeEntryCpaceHostTag, for the caller to place into the
  // generated protobuf message.
  struct CpaceHostTag {
    std::array<uint8_t, 32> cpace_host_public_key{};
    std::array<uint8_t, 32> tag{};
  };

  // Host side of the CodeEntry pairing FSM: states HP2 -> HP3a -> HP4 -> HP5
  // -> HC0 of specification.md. The preceding ThpPairingRequest /
  // ThpSelectMethod exchange (HP0 -> HP2) is the caller's.
  class CodeEntryPairing {
  public:
    explicit CodeEntryPairing(const NoiseHash &handshake_hash);
    ~CodeEntryPairing();

    // HP2 -> HP3a: consume ThpCodeEntryCommitment and draw the random
    // 16-byte challenge, which the caller then sends.
    void consume_commitment_build_challenge(const uint8_t *commitment, size_t len);

    // HP3a -> HP4: consume ThpCodeEntryCpaceTrezor.
    void consume_cpace_trezor(const uint8_t *cpace_trezor_pub, size_t len);

    // HP4 -> HP5: derive the ThpCodeEntryCpaceHostTag fields from the
    // user-entered `code` (one to six ASCII digits, left-padded to six).
    CpaceHostTag build_host_tag(const epee::wipeable_string &code);

    // HP5 -> HC0: consume ThpCodeEntrySecret. Verifies that the commitment
    // matches SHA-256(secret) and that the user-entered code matches the
    // SHA-256 derivation. Returns false if either check fails.
    bool consume_secret(const uint8_t *secret, size_t len);

    bool is_paired() const { return m_paired; }
    const std::array<uint8_t, 16> &challenge() const { return m_challenge; }

  private:
    NoiseHash                m_h{};
    std::array<uint8_t, 16>  m_challenge{};
    std::array<uint8_t, 32>  m_commitment{};
    std::array<uint8_t, 32>  m_cpace_trezor_pub{};
    std::string              m_code;
    bool                     m_have_trezor_pub = false;
    bool                     m_have_host_tag   = false;
    bool                     m_paired          = false;
  };

}}}

#endif // MONERO_TREZOR_THP_PAIRING_H
