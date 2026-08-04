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

#include "noise.hpp"
#include "device_trezor/trezor/exceptions.hpp"

#include "crypto/crypto.h"
#include "memwipe.h"
#include "misc_log_ex.h"

#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_scalarmult.h>
#include <sodium/utils.h>

#include <openssl/evp.h>

#include <climits>
#include <cstdint>
#include <cstring>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor.thp"

namespace hw {
namespace trezor {
namespace thp {

  // Per specification.md, Common definitions: the protocol name is the ASCII
  // string `Noise_XX_25519_AESGCM_SHA256` followed by four NUL bytes. It is
  // both the initial chaining key and the prefix of h. This is stock Noise
  // section 5.2: the name is 28 bytes, which padded to HASHLEN is used
  // verbatim rather than hashed.
  static const uint8_t PROTOCOL_NAME[] = {
    'N','o','i','s','e','_','X','X','_','2','5','5','1','9',
    '_','A','E','S','G','C','M','_','S','H','A','2','5','6',
    0x00, 0x00, 0x00, 0x00,
  };
  static_assert(sizeof(PROTOCOL_NAME) == 32,
                "THP protocol name must be 32 bytes (28 ASCII + 4 zero)");

  namespace noise_crypto {

    void sha256(const uint8_t *data, size_t len, NoiseHash &out)
    {
      crypto_hash_sha256(out.data(), data, len);
    }

    void hmac_sha256(const uint8_t *key, size_t key_len,
                     const uint8_t *data, size_t data_len,
                     NoiseHash &out)
    {
      // The single-shot crypto_auth_hmacsha256() requires a 32-byte key. The
      // streaming API performs the RFC 2104 key schedule for other lengths.
      crypto_auth_hmacsha256_state st;
      crypto_auth_hmacsha256_init(&st, key, key_len);
      crypto_auth_hmacsha256_update(&st, data, data_len);
      crypto_auth_hmacsha256_final(&st, out.data());
    }

    void thp_hkdf(const uint8_t *ck, size_t ck_len,
                  const uint8_t *ikm, size_t ikm_len,
                  NoiseKey &out1, NoiseKey &out2)
    {
      // Per specification.md, Common definitions:
      //   temp_key = HMAC-SHA-256(ck, input)
      //   output_1 = HMAC-SHA-256(temp_key, 0x01)
      //   output_2 = HMAC-SHA-256(temp_key, output_1 || 0x02)
      NoiseHash temp_key{};
      hmac_sha256(ck, ck_len, ikm, ikm_len, temp_key);

      const uint8_t one = 0x01;
      NoiseHash o1{};
      hmac_sha256(temp_key.data(), temp_key.size(), &one, 1, o1);
      std::memcpy(out1.data(), o1.data(), out1.size());

      tools::scrubbed_arr<uint8_t, NOISE_HASHLEN + 1> o1_with_two{};
      std::memcpy(o1_with_two.data(), o1.data(), NOISE_HASHLEN);
      o1_with_two[NOISE_HASHLEN] = 0x02;
      NoiseHash o2{};
      hmac_sha256(temp_key.data(), temp_key.size(),
                  o1_with_two.data(), o1_with_two.size(), o2);
      std::memcpy(out2.data(), o2.data(), out2.size());
    }

    void x25519(const NoisePrivKey &priv, const NoisePubKey &peer,
                NoisePubKey &out)
    {
      // crypto_scalarmult returns -1 for an all-zero result. Per RFC 7748
      // 6.1 that indicates a degenerate peer key and MUST be an error.
      if (crypto_scalarmult(out.data(), priv.data(), peer.data()) != 0) {
        throw exc::SecurityException("THP: X25519 produced all-zero output (low-order peer key)");
      }
    }

    void x25519_keypair(NoisePubKey &pub, NoisePrivKey &priv)
    {
      crypto::generate_random_bytes_thread_safe(priv.size(), priv.data());
      // Clamp per RFC 7748 section 5.
      priv[0]  &= 248;
      priv[31] &= 127;
      priv[31] |= 64;
      crypto_scalarmult_base(pub.data(), priv.data());
    }

    void iv_for_nonce(uint64_t counter, NoiseIv &out)
    {
      // 12-byte IV: 4 zero bytes then the counter big-endian. That is stock
      // Noise (section 12.4 specifies big-endian AESGCM nonces; the
      // little-endian rule in 12.3 is ChaChaPoly's), and it reproduces the
      // spec's notation of 0^96 for counter 0 and 0^95||1 for counter 1.
      std::memset(out.data(), 0, out.size());
      out[4]  = uint8_t(counter >> 56);
      out[5]  = uint8_t(counter >> 48);
      out[6]  = uint8_t(counter >> 40);
      out[7]  = uint8_t(counter >> 32);
      out[8]  = uint8_t(counter >> 24);
      out[9]  = uint8_t(counter >> 16);
      out[10] = uint8_t(counter >>  8);
      out[11] = uint8_t(counter);
    }

    namespace {
      // RAII wrapper around EVP_CIPHER_CTX (avoids leaks on throw).
      struct EvpCtx {
        EVP_CIPHER_CTX *p = EVP_CIPHER_CTX_new();
        ~EvpCtx() { if (p) EVP_CIPHER_CTX_free(p); }
        EvpCtx() = default;
        EvpCtx(const EvpCtx &) = delete;
        EvpCtx &operator=(const EvpCtx &) = delete;
      };
    }

    bool aes256gcm_encrypt(const NoiseKey &key, const NoiseIv &iv,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *plaintext, size_t plaintext_len,
                           std::vector<uint8_t> &ciphertext)
    {
      // EVP takes int lengths. THP frames cap both at 0xFFFF, but this is a
      // public function with no other stated precondition.
      if (aad_len > INT_MAX || plaintext_len > INT_MAX) return false;

      EvpCtx ctx;
      if (!ctx.p) return false;
      if (EVP_EncryptInit_ex(ctx.p, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
        return false;
      if (EVP_CIPHER_CTX_ctrl(ctx.p, EVP_CTRL_GCM_SET_IVLEN, (int)iv.size(), nullptr) != 1)
        return false;
      if (EVP_EncryptInit_ex(ctx.p, nullptr, nullptr, key.data(), iv.data()) != 1)
        return false;

      ciphertext.resize(plaintext_len + NOISE_TAGLEN);
      int dummy_outl = 0;
      if (aad_len) {
        // AAD pass: EVP reports the AAD length here, no ciphertext is written.
        if (EVP_EncryptUpdate(ctx.p, nullptr, &dummy_outl, aad, (int)aad_len) != 1)
          return false;
      }
      int ct_outl = 0;
      if (plaintext_len) {
        if (EVP_EncryptUpdate(ctx.p, ciphertext.data(), &ct_outl,
                              plaintext, (int)plaintext_len) != 1)
          return false;
      }
      int ct_finall = 0;
      if (EVP_EncryptFinal_ex(ctx.p, ciphertext.data() + ct_outl, &ct_finall) != 1)
        return false;
      // AES-GCM has no padding, so this must hold exactly.
      if (size_t(ct_outl + ct_finall) != plaintext_len)
        return false;
      if (EVP_CIPHER_CTX_ctrl(ctx.p, EVP_CTRL_GCM_GET_TAG, NOISE_TAGLEN,
                              ciphertext.data() + plaintext_len) != 1)
        return false;
      return true;
    }

    bool aes256gcm_decrypt(const NoiseKey &key, const NoiseIv &iv,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *ciphertext, size_t ciphertext_len,
                           std::vector<uint8_t> &plaintext)
    {
      if (aad_len > INT_MAX || ciphertext_len > INT_MAX) return false;
      if (ciphertext_len < NOISE_TAGLEN) return false;
      const size_t plain_len = ciphertext_len - NOISE_TAGLEN;

      EvpCtx ctx;
      if (!ctx.p) return false;
      if (EVP_DecryptInit_ex(ctx.p, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
        return false;
      if (EVP_CIPHER_CTX_ctrl(ctx.p, EVP_CTRL_GCM_SET_IVLEN, (int)iv.size(), nullptr) != 1)
        return false;
      if (EVP_DecryptInit_ex(ctx.p, nullptr, nullptr, key.data(), iv.data()) != 1)
        return false;

      plaintext.resize(plain_len);
      int dummy_outl = 0;
      if (aad_len) {
        if (EVP_DecryptUpdate(ctx.p, nullptr, &dummy_outl, aad, (int)aad_len) != 1)
          return false;
      }
      int pt_outl = 0;
      if (plain_len) {
        if (EVP_DecryptUpdate(ctx.p, plaintext.data(), &pt_outl,
                              ciphertext, (int)plain_len) != 1)
          return false;
      }
      // Set the expected tag; EVP_DecryptFinal_ex then verifies it.
      // EVP_CTRL_GCM_SET_TAG wants a non-const pointer but does not write.
      if (EVP_CIPHER_CTX_ctrl(ctx.p, EVP_CTRL_GCM_SET_TAG, NOISE_TAGLEN,
                              const_cast<uint8_t *>(ciphertext + plain_len)) != 1)
        return false;
      // The empty-payload tag check runs on every handshake, so plain_len is
      // routinely 0 and plaintext.data() is then null: keep the pointer
      // arithmetic and the wipe off a null pointer.
      uint8_t *final_out = plain_len ? plaintext.data() + pt_outl : nullptr;
      int pt_finall = 0;
      if (EVP_DecryptFinal_ex(ctx.p, final_out, &pt_finall) != 1) {
        if (plain_len) memwipe(plaintext.data(), plaintext.size());
        plaintext.clear();
        return false;
      }
      if (size_t(pt_outl + pt_finall) != plain_len) {
        if (plain_len) memwipe(plaintext.data(), plaintext.size());
        plaintext.clear();
        return false;
      }
      return true;
    }

    void trezor_mask_static(const NoisePubKey &trezor_static_pub,
                            const NoisePubKey &trezor_ephemeral_pub,
                            NoisePubKey &out_masked)
    {
      // mask = SHA-256(static_pub || ephemeral_pub)
      std::array<uint8_t, NOISE_DHLEN * 2> concat{};
      std::memcpy(concat.data(),               trezor_static_pub.data(),    NOISE_DHLEN);
      std::memcpy(concat.data() + NOISE_DHLEN, trezor_ephemeral_pub.data(), NOISE_DHLEN);
      NoiseHash mask{};
      sha256(concat.data(), concat.size(), mask);

      NoisePrivKey scalar{};
      std::memcpy(scalar.data(), mask.data(), NOISE_DHLEN);
      x25519(scalar, trezor_static_pub, out_masked);
    }

  } // namespace noise_crypto

  NoiseXxInitiator::NoiseXxInitiator() = default;

  NoiseXxInitiator::~NoiseXxInitiator()
  {
    // Every key member scrubs itself; a std::vector cannot, and the known
    // devices carry their own scrubbing destructor.
    if (!m_pairing_credential.empty())
      memwipe(m_pairing_credential.data(), m_pairing_credential.size());
  }

  void NoiseXxInitiator::set_device_properties(const uint8_t *data, size_t len)
  {
    m_device_properties.clear();
    if (len) m_device_properties.assign(data, data + len);
    m_have_device_properties = true;
  }

  void NoiseXxInitiator::set_host_static_key(const HostStaticKey &host_static)
  {
    m_host_static = host_static;
    m_have_host_static = true;
  }

  void NoiseXxInitiator::set_known_devices(std::vector<KnownDevice> known)
  {
    m_known_devices = std::move(known);
  }

  void NoiseXxInitiator::mix_hash(const uint8_t *data, size_t len)
  {
    // h = SHA-256(h || data)
    std::vector<uint8_t> buf;
    buf.reserve(NOISE_HASHLEN + len);
    buf.insert(buf.end(), m_h.begin(), m_h.end());
    if (len) buf.insert(buf.end(), data, data + len);
    noise_crypto::sha256(buf.data(), buf.size(), m_h);
  }

  std::vector<uint8_t> NoiseXxInitiator::build_init_request(bool try_to_unlock)
  {
    if (!m_have_host_static) {
      throw exc::ProtocolException("THP: build_init_request without host static key");
    }
    if (!m_have_device_properties) {
      throw exc::ProtocolException("THP: build_init_request without device properties");
    }

    // Starting a second handshake on the same object must not inherit the
    // ordering flags of the first.
    m_have_ephemeral         = false;
    m_consumed_init_response = false;
    m_built_completion       = false;
    m_complete               = false;
    m_paired                 = false;

    noise_crypto::x25519_keypair(m_host_ephemeral_pub, m_host_ephemeral_priv);
    m_have_ephemeral = true;
    m_try_to_unlock  = try_to_unlock;

    std::vector<uint8_t> body;
    body.reserve(NOISE_DHLEN + 1);
    body.insert(body.end(), m_host_ephemeral_pub.begin(), m_host_ephemeral_pub.end());
    body.push_back(try_to_unlock ? 0x01 : 0x00);
    return body;
  }

  void NoiseXxInitiator::consume_init_response(const uint8_t *body, size_t len)
  {
    if (!m_have_ephemeral) {
      throw exc::ProtocolException("THP: consume_init_response before build_init_request");
    }
    // HandshakeInitiationResponse layout: 32 + 48 + 16 = 96.
    constexpr size_t TREZOR_EPH_OFF       = 0;
    constexpr size_t ENC_STATIC_OFF       = NOISE_DHLEN;
    constexpr size_t ENC_STATIC_LEN       = NOISE_DHLEN + NOISE_TAGLEN; // 48
    constexpr size_t TAG_OFF              = ENC_STATIC_OFF + ENC_STATIC_LEN;
    constexpr size_t TAG_LEN              = NOISE_TAGLEN;
    constexpr size_t EXPECTED_LEN         = TAG_OFF + TAG_LEN;
    if (len != EXPECTED_LEN) {
      throw exc::ProtocolException("THP: HandshakeInitResponse wrong size");
    }

    // h = SHA-256(protocol_name || device_properties). Mixing the device
    // properties is stock MixHash(prologue), Noise section 5.3.
    std::vector<uint8_t> hash_in;
    hash_in.reserve(sizeof(PROTOCOL_NAME) + m_device_properties.size());
    hash_in.insert(hash_in.end(), PROTOCOL_NAME, PROTOCOL_NAME + sizeof(PROTOCOL_NAME));
    hash_in.insert(hash_in.end(), m_device_properties.begin(), m_device_properties.end());
    noise_crypto::sha256(hash_in.data(), hash_in.size(), m_h);

    mix_hash(m_host_ephemeral_pub.data(), m_host_ephemeral_pub.size());
    {
      const uint8_t b = m_try_to_unlock ? 0x01 : 0x00;
      mix_hash(&b, 1);
    }

    std::memcpy(m_trezor_ephemeral_pub.data(), body + TREZOR_EPH_OFF, NOISE_DHLEN);
    mix_hash(m_trezor_ephemeral_pub.data(), m_trezor_ephemeral_pub.size());

    // (ck, k) = HKDF(protocol_name, X25519(host_eph_priv, trezor_eph_pub))
    NoisePubKey ee{};
    noise_crypto::x25519(m_host_ephemeral_priv, m_trezor_ephemeral_pub, ee);
    NoiseKey ck{}, k{};
    noise_crypto::thp_hkdf(PROTOCOL_NAME, sizeof(PROTOCOL_NAME),
                           ee.data(), ee.size(), ck, k);
    memwipe(ee.data(), ee.size());
    m_ck_or_zero  = ck;
    m_k_handshake = k;

    // Decrypt encrypted_trezor_static_pubkey at IV=0^96, AAD=h.
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> dec_masked;
    if (!noise_crypto::aes256gcm_decrypt(m_k_handshake, iv,
                                         m_h.data(), m_h.size(),
                                         body + ENC_STATIC_OFF, ENC_STATIC_LEN,
                                         dec_masked)) {
      throw exc::SecurityException("THP: HandshakeInitResponse: enc_static tag invalid");
    }
    if (dec_masked.size() != NOISE_DHLEN) {
      throw exc::ProtocolException("THP: HandshakeInitResponse: bad enc_static plaintext size");
    }
    std::memcpy(m_trezor_masked_static.data(), dec_masked.data(), NOISE_DHLEN);

    mix_hash(body + ENC_STATIC_OFF, ENC_STATIC_LEN);

    // (ck, k) = HKDF(ck, X25519(host_eph_priv, trezor_masked_static_pubkey))
    NoisePubKey se{};
    noise_crypto::x25519(m_host_ephemeral_priv, m_trezor_masked_static, se);
    noise_crypto::thp_hkdf(m_ck_or_zero.data(), m_ck_or_zero.size(),
                           se.data(), se.size(), ck, k);
    memwipe(se.data(), se.size());
    m_ck_or_zero  = ck;
    m_k_handshake = k;

    // Decrypt the empty-payload tag at IV=0^96, AAD=h.
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> empty_payload;
    if (!noise_crypto::aes256gcm_decrypt(m_k_handshake, iv,
                                         m_h.data(), m_h.size(),
                                         body + TAG_OFF, TAG_LEN,
                                         empty_payload)) {
      throw exc::SecurityException("THP: HandshakeInitResponse: trailing tag invalid");
    }
    if (!empty_payload.empty()) {
      throw exc::ProtocolException("THP: HandshakeInitResponse: trailing payload not empty");
    }
    mix_hash(body + TAG_OFF, TAG_LEN);

    // HH1 step 11: identify a paired device. The masked static pubkey rotates
    // every session because it is X25519(SHA-256(static_pub || trezor_eph_pub),
    // static_pub) over a fresh ephemeral, so a stored masked value would be
    // useless: re-mask each stored unmasked static with this session's
    // ephemeral and compare against the value just decrypted.
    m_paired = false;
    m_pairing_credential.clear();
    for (const auto &kd : m_known_devices) {
      NoisePubKey expected_masked{};
      try {
        noise_crypto::trezor_mask_static(kd.trezor_static_pubkey,
                                         m_trezor_ephemeral_pub,
                                         expected_masked);
      } catch (const std::exception &e) {
        // An all-zero or low-order stored pubkey makes X25519 fail. Skipping
        // that record keeps one corrupt entry from aborting the scan for
        // every other device on every future connect.
        MWARNING("THP: skipping unusable known-device record: " << e.what());
        continue;
      }
      if (sodium_memcmp(expected_masked.data(),
                        m_trezor_masked_static.data(),
                        NOISE_DHLEN) == 0)
      {
        m_paired             = true;
        m_host_static        = kd.host_static;
        m_have_host_static   = true;
        m_pairing_credential = kd.pairing_credential;
        break;
      }
    }
    m_consumed_init_response = true;
  }

  std::vector<uint8_t> NoiseXxInitiator::build_completion_request()
  {
    if (!m_consumed_init_response) {
      throw exc::ProtocolException("THP: build_completion_request before init response");
    }
    if (!m_have_host_static) {
      throw exc::ProtocolException("THP: build_completion_request without host static key");
    }

    // Encrypt host_static_pubkey at IV=0^95||1, AAD=h.
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(1, iv);
    std::vector<uint8_t> enc_host_static;
    if (!noise_crypto::aes256gcm_encrypt(m_k_handshake, iv,
                                         m_h.data(), m_h.size(),
                                         m_host_static.pub.data(), m_host_static.pub.size(),
                                         enc_host_static)) {
      throw exc::ProtocolException("THP: AES-GCM encrypt failed (host_static)");
    }
    mix_hash(enc_host_static.data(), enc_host_static.size());

    // (ck, k) = HKDF(ck, X25519(host_static_priv, trezor_eph_pub))
    NoisePubKey ss{};
    noise_crypto::x25519(m_host_static.priv, m_trezor_ephemeral_pub, ss);
    NoiseKey ck{}, k{};
    noise_crypto::thp_hkdf(m_ck_or_zero.data(), m_ck_or_zero.size(),
                           ss.data(), ss.size(), ck, k);
    memwipe(ss.data(), ss.size());
    m_ck_or_zero  = ck;
    m_k_handshake = k;

    // ThpHandshakeCompletionReqNoisePayload. The unpaired flow omits
    // host_pairing_credential, which serialises to zero bytes; the paired
    // flow emits field 1 as a length-delimited byte string. Encoded by hand
    // because this layer stays free of the generated message types.
    std::vector<uint8_t> payload_binary;
    if (m_paired && !m_pairing_credential.empty()) {
      payload_binary.push_back(0x0A); // field 1, wire type 2
      size_t n = m_pairing_credential.size();
      while (n >= 0x80) {
        payload_binary.push_back(uint8_t((n & 0x7F) | 0x80));
        n >>= 7;
      }
      payload_binary.push_back(uint8_t(n));
      payload_binary.insert(payload_binary.end(),
                            m_pairing_credential.begin(),
                            m_pairing_credential.end());
    }

    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> enc_payload;
    if (!noise_crypto::aes256gcm_encrypt(m_k_handshake, iv,
                                         m_h.data(), m_h.size(),
                                         payload_binary.data(), payload_binary.size(),
                                         enc_payload)) {
      throw exc::ProtocolException("THP: AES-GCM encrypt failed (completion payload)");
    }
    mix_hash(enc_payload.data(), enc_payload.size());

    std::vector<uint8_t> body;
    body.reserve(enc_host_static.size() + enc_payload.size());
    body.insert(body.end(), enc_host_static.begin(), enc_host_static.end());
    body.insert(body.end(), enc_payload.begin(),     enc_payload.end());
    m_built_completion = true;
    return body;
  }

  void NoiseXxInitiator::consume_completion_response(const uint8_t *body,
                                                     size_t len)
  {
    if (!m_consumed_init_response) {
      throw exc::ProtocolException("THP: consume_completion_response before init response");
    }
    if (!m_built_completion) {
      throw exc::ProtocolException("THP: consume_completion_response before build_completion_request");
    }
    // (key_request, key_response) = HKDF(ck, empty_string)
    NoiseKey key_request{}, key_response{};
    noise_crypto::thp_hkdf(m_ck_or_zero.data(), m_ck_or_zero.size(),
                           nullptr, 0, key_request, key_response);

    // trezor_state = AES-GCM-DECRYPT(key_response, IV=0^96, ad=empty, body)
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(0, iv);
    std::vector<uint8_t> trezor_state;
    if (!noise_crypto::aes256gcm_decrypt(key_response, iv,
                                         nullptr, 0,
                                         body, len,
                                         trezor_state)) {
      throw exc::SecurityException("THP: HandshakeCompletionResponse: tag invalid");
    }
    if (trezor_state.size() != 1) {
      throw exc::ProtocolException("THP: HandshakeCompletionResponse: bad state plaintext size");
    }
    m_trezor_state = trezor_state[0];

    m_keys.key_request    = key_request;
    m_keys.key_response   = key_response;
    m_keys.handshake_hash = m_h;
    m_complete            = true;
  }

  const HandshakeKeys &NoiseXxInitiator::keys() const
  {
    if (!m_complete) {
      throw exc::ProtocolException("THP: keys() called before handshake completion");
    }
    return m_keys;
  }

  TransportCipher::TransportCipher(const NoiseKey &key, uint64_t initial_nonce)
    : m_key(key), m_counter(initial_nonce) {}

  void TransportCipher::seal(const uint8_t *aad, size_t aad_len,
                             const uint8_t *plaintext, size_t plaintext_len,
                             std::vector<uint8_t> &out)
  {
    if (m_failed) {
      throw exc::SecurityException("THP: transport cipher failed closed");
    }
    // Fail-closed nonce-exhaustion guard: reusing a nonce under one AES-GCM
    // key is catastrophic, so refuse rather than wrap. Unreachable at 2^64-1
    // frames. The counter must not advance past here.
    if (m_counter == UINT64_MAX) {
      m_failed = true;
      throw exc::SecurityException("THP: transport cipher nonce exhausted (seal)");
    }
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(m_counter, iv);
    if (!noise_crypto::aes256gcm_encrypt(m_key, iv, aad, aad_len,
                                         plaintext, plaintext_len, out)) {
      m_failed = true;
      throw exc::ProtocolException("THP: AES-GCM seal failed");
    }
    // One counter per cipherstate (key_request / key_response), advanced only
    // after a successful operation.
    ++m_counter;
  }

  bool TransportCipher::open(const uint8_t *aad, size_t aad_len,
                             const uint8_t *ciphertext, size_t ciphertext_len,
                             std::vector<uint8_t> &out)
  {
    // A peer that has failed authentication once does not get to keep
    // streaming frames: the Noise spec and Trezor's own host both terminate
    // on a decryption failure.
    if (m_failed) {
      return false;
    }
    if (m_counter == UINT64_MAX) {
      m_failed = true;
      return false;
    }
    noise_crypto::NoiseIv iv{};
    noise_crypto::iv_for_nonce(m_counter, iv);
    if (!noise_crypto::aes256gcm_decrypt(m_key, iv, aad, aad_len,
                                         ciphertext, ciphertext_len, out)) {
      m_failed = true;
      return false;
    }
    ++m_counter;
    return true;
  }

}}}
