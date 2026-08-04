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

#ifndef MONERO_TREZOR_THP_NOISE_H
#define MONERO_TREZOR_THP_NOISE_H

#include "memwipe.h"

#include <array>
#include <cstdint>
#include <vector>

namespace hw {
namespace trezor {
namespace thp {

  // Sizes of the Noise primitives used by THP (Noise_XX_25519_AESGCM_SHA256).
  constexpr size_t NOISE_DHLEN   = 32; // X25519 public/private key
  constexpr size_t NOISE_HASHLEN = 32; // SHA-256 digest
  constexpr size_t NOISE_KEYLEN  = 32; // AES-256 key
  constexpr size_t NOISE_TAGLEN  = 16; // AES-GCM authentication tag

  // Key material, and the handshake hash the pairing-code derivation mixes
  // in, scrub themselves on destruction, so a copy left behind by container
  // growth or by an unwound stack frame does not survive in the clear.
  // Public keys are not secret and stay plain arrays.
  using NoiseKey     = tools::scrubbed_arr<uint8_t, NOISE_KEYLEN>;
  using NoiseHash    = tools::scrubbed_arr<uint8_t, NOISE_HASHLEN>;
  using NoisePrivKey = tools::scrubbed_arr<uint8_t, NOISE_DHLEN>;
  using NoisePubKey  = std::array<uint8_t, NOISE_DHLEN>;

  // Output of a successful handshake. The two AES-GCM keys carry the
  // application traffic; handshake_hash is the final value of h.
  struct HandshakeKeys {
    NoiseKey  key_request;   // host -> device direction
    NoiseKey  key_response;  // device -> host direction
    NoiseHash handshake_hash;
  };

  // The host's static keypair, persisted across sessions per the spec.
  struct HostStaticKey {
    NoisePubKey  pub{};
    NoisePrivKey priv{};
  };

  // A previously-established pairing: the *unmasked* Trezor static pubkey
  // observed during an earlier handshake, the host static keypair used with
  // that device, and the opaque pairing credential Trezor issued.
  //
  // The masked form rotates per session (it depends on the device's fresh
  // ephemeral pubkey), so the unmasked pubkey is what we persist, re-masking
  // it each session before comparing. See specification.md HH1 step 11.
  struct KnownDevice {
    NoisePubKey   trezor_static_pubkey{};
    HostStaticKey host_static{};
    std::vector<uint8_t> pairing_credential; // opaque to the host

    // host_static.priv scrubs itself; a std::vector cannot, and the
    // credential is what authenticates this host to the device.
    ~KnownDevice() {
      if (!pairing_credential.empty())
        memwipe(pairing_credential.data(), pairing_credential.size());
    }
  };

  // Low-level Noise primitives, exposed for the unit tests. Deliberately not
  // called "crypto": that name is Monero's global namespace.
  namespace noise_crypto {
    void sha256(const uint8_t *data, size_t len, NoiseHash &out);
    void hmac_sha256(const uint8_t *key, size_t key_len,
                     const uint8_t *data, size_t data_len,
                     NoiseHash &out);

    // The HKDF defined in specification.md, Common definitions. Returns two
    // 32-byte outputs derived from a chaining key and input keying material.
    void thp_hkdf(const uint8_t *ck, size_t ck_len,
                  const uint8_t *ikm, size_t ikm_len,
                  NoiseKey &out1, NoiseKey &out2);

    // X25519 scalar multiplication. Throws on a degenerate peer key.
    void x25519(const NoisePrivKey &priv, const NoisePubKey &peer,
                NoisePubKey &out);

    // Generate a fresh X25519 keypair (clamps the private key per RFC 7748).
    void x25519_keypair(NoisePubKey &pub, NoisePrivKey &priv);

    // AES-256-GCM AEAD with the 12-byte IV the spec notates as a bit string
    // ("0^96", "0^95||1"). iv_for_nonce builds it from a 64-bit counter.
    using NoiseIv = std::array<uint8_t, 12>;
    void iv_for_nonce(uint64_t counter, NoiseIv &out);

    // Both return false on failure; decrypt returns false on tag mismatch
    // and leaves no plaintext behind.
    bool aes256gcm_encrypt(const NoiseKey &key, const NoiseIv &iv,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *plaintext, size_t plaintext_len,
                           std::vector<uint8_t> &ciphertext);
    bool aes256gcm_decrypt(const NoiseKey &key, const NoiseIv &iv,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *ciphertext, size_t ciphertext_len,
                           std::vector<uint8_t> &plaintext);

    // Trezor's static key masking (specification.md, Handshake phase):
    //   masked = X25519(SHA-256(static_pub || ephemeral_pub), static_pub)
    // Pure function; also used by the paired-device lookup (HH1 step 11).
    void trezor_mask_static(const NoisePubKey &trezor_static_pub,
                            const NoisePubKey &trezor_ephemeral_pub,
                            NoisePubKey &out_masked);
  }

  // THP host-side handshake driver: the Noise XX initiator role specialised
  // for the THP state machine (specification.md, Host's state machine).
  //
  //   NoiseXxInitiator h;
  //   h.set_device_properties(ChannelAllocationResponse.device_properties);
  //   h.set_host_static_key(persisted_or_freshly_generated_keypair);
  //   h.set_known_devices(persisted_credentials);
  //   auto req1 = h.build_init_request(try_to_unlock);  // send, then:
  //   h.consume_init_response(resp1.data(), resp1.size());
  //   auto req2 = h.build_completion_request();         // send, then:
  //   h.consume_completion_response(resp2.data(), resp2.size());
  class NoiseXxInitiator {
  public:
    NoiseXxInitiator();
    ~NoiseXxInitiator();
    NoiseXxInitiator(const NoiseXxInitiator &) = delete;
    NoiseXxInitiator &operator=(const NoiseXxInitiator &) = delete;

    // ChannelAllocationResponse.device_properties, which the handshake mixes
    // in as the Noise prologue. MUST be called before build_init_request().
    void set_device_properties(const uint8_t *data, size_t len);

    // The host's persistent static X25519 keypair: freshly generated for a
    // first-time pairing, or loaded from the credential store.
    void set_host_static_key(const HostStaticKey &host_static);

    // Previously-paired Trezors, so the handshake can recognise one by its
    // masked static pubkey (HH1 step 11). May be empty.
    void set_known_devices(std::vector<KnownDevice> known);

    // Step 1 (HH0): generate the ephemeral keypair, build the request body.
    std::vector<uint8_t> build_init_request(bool try_to_unlock);

    // Step 2 (HH1, partial): decrypt the masked Trezor static pubkey and the
    // empty-payload tag, mix in the se DH.
    void consume_init_response(const uint8_t *body, size_t len);

    // Step 3 (HH1, completion): paired vs. unpaired flow is selected from the
    // masked pubkey lookup done in consume_init_response.
    std::vector<uint8_t> build_completion_request();

    // Step 4 (HH2/HH3): finalise. keys() and trezor_state() valid afterwards.
    void consume_completion_response(const uint8_t *body, size_t len);

    // Whether the init response matched a previously-paired device (HH2).
    // False means the unpaired flow (HH3) and that pairing must follow.
    bool is_known_device() const { return m_paired; }

    // The masked Trezor static pubkey decrypted from the init response.
    const NoisePubKey &trezor_masked_static_pubkey() const {
      return m_trezor_masked_static;
    }

    // HandshakeCompletionResponse state byte:
    //   0x00 = STATE_UNPAIRED, 0x01 = STATE_PAIRED, 0x02 = STATE_PAIRED_AUTOCONNECT
    uint8_t trezor_state() const { return m_trezor_state; }

    bool is_complete() const { return m_complete; }
    const HandshakeKeys &keys() const;

    // The host static keypair in use after consume_init_response. On the
    // paired flow this is the keypair from the matching KnownDevice, so the
    // caller's outer copy is stale and should be re-synced from here.
    const HostStaticKey &host_static_in_use() const { return m_host_static; }

  private:
    // h = SHA-256(h || data)
    void mix_hash(const uint8_t *data, size_t len);

    bool                 m_have_device_properties = false;
    bool                 m_have_host_static       = false;
    bool                 m_have_ephemeral         = false;
    bool                 m_consumed_init_response = false;
    bool                 m_built_completion       = false;
    bool                 m_complete               = false;
    bool                 m_paired                 = false;
    uint8_t              m_trezor_state           = 0;

    std::vector<uint8_t> m_device_properties;
    HostStaticKey        m_host_static{};
    NoisePubKey          m_host_ephemeral_pub{};
    NoisePrivKey         m_host_ephemeral_priv{};
    NoisePubKey          m_trezor_ephemeral_pub{};
    NoisePubKey          m_trezor_masked_static{};
    bool                 m_try_to_unlock          = false;

    std::vector<KnownDevice>  m_known_devices;
    // Copy of the matched entry's credential when m_paired, else empty.
    std::vector<uint8_t>      m_pairing_credential;

    // Symmetric state (h, ck, k) per the spec.
    NoiseHash m_h{};
    NoiseKey  m_ck_or_zero{};   // re-used as ck after the first HKDF
    NoiseKey  m_k_handshake{};  // current handshake AES-GCM key

    HandshakeKeys m_keys{};
  };

  // Application-traffic AES-GCM cipherstate: a key plus a 64-bit counter.
  // Fails closed permanently after any seal() or open() failure, as the
  // Noise spec and Trezor's own host both do.
  class TransportCipher {
  public:
    explicit TransportCipher(const NoiseKey &key, uint64_t initial_nonce = 0);
    TransportCipher(const TransportCipher &) = delete;
    TransportCipher &operator=(const TransportCipher &) = delete;

    void seal(const uint8_t *aad, size_t aad_len,
              const uint8_t *plaintext, size_t plaintext_len,
              std::vector<uint8_t> &out);

    bool open(const uint8_t *aad, size_t aad_len,
              const uint8_t *ciphertext, size_t ciphertext_len,
              std::vector<uint8_t> &out);

    uint64_t nonce() const { return m_counter; }

  private:
    NoiseKey m_key;
    uint64_t m_counter;
    bool     m_failed = false;
  };

}}}

#endif // MONERO_TREZOR_THP_NOISE_H
