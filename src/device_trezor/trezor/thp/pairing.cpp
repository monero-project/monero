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

#include "pairing.hpp"
#include "device_trezor/trezor/exceptions.hpp"

#include "crypto/crypto.h"
#include "memwipe.h"

extern "C"
{
#include "crypto/crypto-ops.h"
}

#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_hash_sha512.h>
#include <sodium/utils.h>

#include <cstring>
#include <string>
#include <vector>

namespace hw {
namespace trezor {
namespace thp {

  namespace {

    // The CPace generator is a deterministic function of the pairing code, so
    // 32 recovered generator bytes recover the code in a million trials. Every
    // code-derived buffer therefore lives in a self-scrubbing array, which
    // also covers the throwing paths.
    using CodeDerived32 = tools::scrubbed_arr<uint8_t, 32>;

    // J = 486662, the Montgomery coefficient of Curve25519 (RFC 7748
    // section 4.1), in ref10 limb form: limb 0 carries weight 1.
    const fe FE_J = {486662, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // ref10's fe_copy. crypto-ops.c declares it static, so it is restated
    // here over the fe type crypto-ops.h does export.
    void fe_copy(fe h, const fe f)
    {
      for (int i = 0; i < 10; ++i) {
        h[i] = f[i];
      }
    }

    // ref10's fe_cmov, likewise static in crypto-ops.c. Replaces f with g
    // when b == 1 and leaves f unchanged when b == 0, using an arithmetic
    // mask rather than a branch, as RFC 9380 section 4 requires of CMOV.
    // b must be 0 or 1.
    void fe_cmov(fe f, const fe g, unsigned int b)
    {
      const int32_t mask = -(int32_t) b;
      for (int i = 0; i < 10; ++i) {
        f[i] ^= (f[i] ^ g[i]) & mask;
      }
    }

    // ref10's fe_frombytes. crypto-ops.h exports only fe_frombytes_vartime,
    // which additionally rejects the 19 encodings in [p, 2^255 - 1] and so
    // branches on its input; that rejection is dropped here, leaving a
    // non-canonical encoding to be reduced by the field arithmetic that
    // follows. Bit 255 is masked off, per RFC 7748 section 5.
    void fe_frombytes(fe h, const unsigned char *s)
    {
      int64_t h0 = load_4(s);
      int64_t h1 = load_3(s + 4) << 6;
      int64_t h2 = load_3(s + 7) << 5;
      int64_t h3 = load_3(s + 10) << 3;
      int64_t h4 = load_3(s + 13) << 2;
      int64_t h5 = load_4(s + 16);
      int64_t h6 = load_3(s + 20) << 7;
      int64_t h7 = load_3(s + 23) << 5;
      int64_t h8 = load_3(s + 26) << 4;
      int64_t h9 = (load_3(s + 29) & 8388607) << 2;

      const int64_t carry9 = (h9 + (int64_t) (1 << 24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
      const int64_t carry1 = (h1 + (int64_t) (1 << 24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
      const int64_t carry3 = (h3 + (int64_t) (1 << 24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
      const int64_t carry5 = (h5 + (int64_t) (1 << 24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
      const int64_t carry7 = (h7 + (int64_t) (1 << 24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

      const int64_t carry0 = (h0 + (int64_t) (1 << 25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
      const int64_t carry2 = (h2 + (int64_t) (1 << 25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
      const int64_t carry4 = (h4 + (int64_t) (1 << 25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
      const int64_t carry6 = (h6 + (int64_t) (1 << 25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
      const int64_t carry8 = (h8 + (int64_t) (1 << 25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

      h[0] = (int32_t) h0;
      h[1] = (int32_t) h1;
      h[2] = (int32_t) h2;
      h[3] = (int32_t) h3;
      h[4] = (int32_t) h4;
      h[5] = (int32_t) h5;
      h[6] = (int32_t) h6;
      h[7] = (int32_t) h7;
      h[8] = (int32_t) h8;
      h[9] = (int32_t) h9;
    }

    // Constant-time test for h == 0 in F. fe_tobytes reduces fully mod p and
    // has no input-dependent branch, and folding the 32 canonical bytes
    // together with OR reads all of them whatever their values are. acc is at
    // most 255, so acc - 1 borrows into the high bit exactly when acc is 0.
    unsigned int fe_is_zero(const fe h)
    {
      unsigned char enc[32];
      fe_tobytes(enc, h);
      uint32_t acc = 0;
      for (size_t i = 0; i < sizeof(enc); ++i) {
        acc |= enc[i];
      }
      memwipe(enc, sizeof(enc));
      return (unsigned int) ((uint32_t) (acc - 1u) >> 31);
    }

    // z^((p-5)/8) = z^(2^252 - 3), the ref10 fe_pow22523 addition chain.
    // crypto-ops.c carries it inline inside the static fe_divpowm1, so it is
    // restated here. Every step is a squaring or a multiplication, so the
    // sequence of operations does not depend on z.
    void fe_pow22523(fe out, const fe z)
    {
      fe t0, t1, t2;
      int i;

      fe_sq(t0, z);
      fe_sq(t1, t0);
      fe_sq(t1, t1);
      fe_mul(t1, z, t1);
      fe_mul(t0, t0, t1);
      fe_sq(t0, t0);
      fe_mul(t0, t1, t0);
      fe_sq(t1, t0);
      for (i = 0; i < 4; ++i) {
        fe_sq(t1, t1);
      }
      fe_mul(t0, t1, t0);
      fe_sq(t1, t0);
      for (i = 0; i < 9; ++i) {
        fe_sq(t1, t1);
      }
      fe_mul(t1, t1, t0);
      fe_sq(t2, t1);
      for (i = 0; i < 19; ++i) {
        fe_sq(t2, t2);
      }
      fe_mul(t1, t2, t1);
      for (i = 0; i < 10; ++i) {
        fe_sq(t1, t1);
      }
      fe_mul(t0, t1, t0);
      fe_sq(t1, t0);
      for (i = 0; i < 49; ++i) {
        fe_sq(t1, t1);
      }
      fe_mul(t1, t1, t0);
      fe_sq(t2, t1);
      for (i = 0; i < 99; ++i) {
        fe_sq(t2, t2);
      }
      fe_mul(t1, t2, t1);
      for (i = 0; i < 50; ++i) {
        fe_sq(t1, t1);
      }
      fe_mul(t0, t1, t0);
      fe_sq(t0, t0);
      fe_sq(t0, t0);
      fe_mul(out, t0, z);

      memwipe(t0, sizeof(t0));
      memwipe(t1, sizeof(t1));
      memwipe(t2, sizeof(t2));
    }

    // RFC 9380 section 4: is_square(x) is true when x^((q - 1) / 2) is 0 or 1
    // in F, which Euler's criterion makes computable in constant time. Here
    // q = 2^255 - 19, so (q - 1) / 2 = 2^254 - 10 = 4 * (2^252 - 3) + 2 and
    // the exponentiation is the chain above squared twice, times x^2. The
    // result is 0, 1 or q - 1, so adding 1 gives zero exactly for the
    // non-squares. Zero is a square, which keeps gx1 == 0 on the x1 branch.
    unsigned int fe_is_square(const fe x)
    {
      fe e, xx, one;

      fe_pow22523(e, x);
      fe_sq(e, e);
      fe_sq(e, e);
      fe_sq(xx, x);
      fe_mul(e, e, xx);
      fe_1(one);
      fe_add(e, e, one);

      const unsigned int nonsquare = fe_is_zero(e);
      memwipe(e, sizeof(e));
      memwipe(xx, sizeof(xx));
      return nonsquare ^ 1u;
    }

  } // anon

  // Elligator 2 for Curve25519: RFC 9380 section 6.7.1 with the parameters
  // Appendix G.2.1 fixes for this curve, J = 486662, K = 1 and Z = 2.
  // draft-irtf-cfrg-cpace-10 section 7.2 makes this the generator step of
  // CPace and discards the v coordinate, so no square root is taken.
  //
  // The input is a deterministic function of the pairing code, so no step
  // branches on it or indexes memory with it: draft-irtf-cfrg-cpace-10
  // section 9.8 singles out calculate_generator as the substep to compute in
  // constant time, and RFC 9380 section 4 requires the same of CMOV and
  // is_square. The field arithmetic is Monero's ref10 implementation and
  // every selection is an arithmetic conditional move.
  void elligator2_curve25519(const uint8_t r_bytes[32], uint8_t out_u[32])
  {
    fe one; fe_1(one);

    // RFC 7748 section 5 decodeUCoordinate: clear bit 255 before reading the
    // little-endian bytes as a field element. draft-irtf-cfrg-cpace-10
    // section 7.2 applies it to the hashed generator string, and trezorlib
    // does the same in curve25519.decode_coordinate. Without the mask we
    // derive a different u than the device whenever the truncated SHA-512
    // high bit is set, i.e. about half of all pairings, and the CPace tags
    // mismatch.
    CodeDerived32 masked{};
    std::memcpy(masked.data(), r_bytes, 32);
    masked[31] &= 0x7F;

    fe u;
    fe_frombytes(u, masked.data());

    // d = 1 + Z * u^2. 2 * u^2 is formed as (u + u) * u because chaining two
    // fe_add calls would exceed the |f| <= 1.1*2^25 input bound ref10
    // documents for fe_add.
    fe d;
    fe_add(d, u, u);
    fe_mul(d, d, u);
    fe_add(d, d, one);

    // Step 1: x1 = -(J / K) * inv0(1 + Z * u^2), K = 1. fe_invert is
    // z^(p - 2), which is inv0: it maps 0 to 0 (RFC 9380 section 4).
    fe neg_J; fe_neg(neg_J, FE_J);
    fe x1;    fe_invert(x1, d);
              fe_mul(x1, neg_J, x1);

    // Step 2: if x1 == 0, set x1 = -(J / K). x1 is zero only when d is, i.e.
    // when u^2 == -1/2; -1 is a square mod p and 2 is not, so -1/2 is a
    // non-square and no such u exists. Section 6.7.1 records the same, noting
    // that the exceptional case arises only when q = 3 (mod 4), whereas
    // p = 5 (mod 8) and so p = 1 (mod 4).
    const unsigned int x1_is_zero = fe_is_zero(x1);
    fe_cmov(x1, neg_J, x1_is_zero);

    // Step 3: gx1 = x1^3 + (J / K) * x1^2 + x1 / K^2, K = 1, evaluated as
    // x1 * (x1 * (x1 + J) + 1).
    fe tv;  fe_add(tv, x1, FE_J);
            fe_mul(tv, x1, tv);
            fe_add(tv, tv, one);
    fe gx1; fe_mul(gx1, x1, tv);

    // Step 4: x2 = -x1 - (J / K).
    fe x2; fe_neg(x2, x1);
           fe_sub(x2, x2, FE_J);

    // Steps 6 and 7: x = x1 if gx1 is square, x2 otherwise. The y those steps
    // also produce, step 5's gx2 and step 9's t make up the discarded v
    // coordinate; step 8 is s = x * K with K = 1.
    const unsigned int nonsquare = fe_is_square(gx1) ^ 1u;
    fe x; fe_copy(x, x1);
    fe_cmov(x, x2, nonsquare);

    fe_tobytes(out_u, x);

    // Each of these is a deterministic function of the pairing code, so 32
    // recovered bytes of any of them recover the code in a million trials.
    memwipe(u, sizeof(u));
    memwipe(d, sizeof(d));
    memwipe(x1, sizeof(x1));
    memwipe(tv, sizeof(tv));
    memwipe(gx1, sizeof(gx1));
    memwipe(x2, sizeof(x2));
    memwipe(x, sizeof(x));
  }

  void cpace_derive_generator(const std::string &code,
                              const NoiseHash    &handshake_hash,
                              uint8_t out_generator[32])
  {
    // Per specification.md, Notes:
    //   prefix  = 0x08 "CPace255" 0x06                          (10 bytes)
    //             (the IRTF CPACE-X25519-SHA512 DSI tag in the symmetric
    //             setting; the trailing 0x06 is prepend_len(PRS))
    //   padding = 0x6f || 0x00 * 111 || 0x20                    (113 bytes)
    //   pregenerator = SHA-512(prefix || code || padding || handshake_hash || 0x00)[:32]
    //   generator    = ELLIGATOR2(pregenerator)
    static const uint8_t PREFIX[10] = {
      0x08, 0x43, 0x50, 0x61, 0x63, 0x65, 0x32, 0x35, 0x35, 0x06,
    };
    static const size_t  PADDING_ZERO_BYTES = 111;

    // Both constants are fixed for a six-digit code: the 0x06 above is the
    // declared code length and the padding is 128 - (9 + (1 + 6) + 1). Any
    // other length would silently derive a different domain, so refuse it,
    // as firmware's own assertion does.
    if (code.size() != 6) {
      throw exc::ProtocolException("THP pairing: CPace code must be exactly 6 digits");
    }

    std::vector<uint8_t> input;
    input.reserve(sizeof(PREFIX) + code.size() + 1 + PADDING_ZERO_BYTES + 1
                  + handshake_hash.size() + 1);
    input.insert(input.end(), PREFIX, PREFIX + sizeof(PREFIX));
    input.insert(input.end(), code.begin(), code.end());
    input.push_back(0x6f);
    input.insert(input.end(), PADDING_ZERO_BYTES, 0x00);
    input.push_back(0x20);
    input.insert(input.end(), handshake_hash.begin(), handshake_hash.end());
    input.push_back(0x00);

    tools::scrubbed_arr<uint8_t, crypto_hash_sha512_BYTES> hash{};
    crypto_hash_sha512(hash.data(), input.data(), input.size());
    // The hash input carries the code; scrub it before anything can throw.
    memwipe(input.data(), input.size());

    CodeDerived32 pregenerator{};
    std::memcpy(pregenerator.data(), hash.data(), pregenerator.size());
    elligator2_curve25519(pregenerator.data(), out_generator);
  }

  CodeEntryPairing::CodeEntryPairing(const NoiseHash &handshake_hash)
    : m_h(handshake_hash) {}

  CodeEntryPairing::~CodeEntryPairing()
  {
    // m_h scrubs itself; a std::string cannot, and the code is the secret
    // this whole exchange protects.
    if (!m_code.empty()) {
      memwipe(&m_code[0], m_code.size());
    }
  }

  void CodeEntryPairing::consume_commitment_build_challenge(
      const uint8_t *commitment, size_t len)
  {
    if (len != 32) {
      throw exc::ProtocolException("THP pairing: ThpCodeEntryCommitment.commitment must be 32 bytes");
    }
    std::memcpy(m_commitment.data(), commitment, 32);
    crypto::generate_random_bytes_thread_safe(m_challenge.size(), m_challenge.data());
  }

  void CodeEntryPairing::consume_cpace_trezor(const uint8_t *cpace_trezor_pub, size_t len)
  {
    if (len != 32) {
      throw exc::ProtocolException("THP pairing: ThpCodeEntryCpaceTrezor.cpace_trezor_public_key must be 32 bytes");
    }
    std::memcpy(m_cpace_trezor_pub.data(), cpace_trezor_pub, 32);
    m_have_trezor_pub = true;
  }

  CpaceHostTag CodeEntryPairing::build_host_tag(const epee::wipeable_string &code)
  {
    if (!m_have_trezor_pub) {
      throw exc::ProtocolException("THP pairing: build_host_tag before ThpCodeEntryCpaceTrezor");
    }
    if (code.empty() || code.size() > 6) {
      throw exc::ProtocolException("THP pairing: code must be 1 to 6 digits");
    }
    for (size_t i = 0; i < code.size(); ++i) {
      const char c = code.data()[i];
      if (c < '0' || c > '9') {
        throw exc::ProtocolException("THP pairing: code must be ASCII digits");
      }
    }
    // The device hashes the code as exactly six ASCII digits, zero-padded on
    // the left, so a user who drops a leading zero still pairs. trezorlib
    // demands six characters from its caller; being lenient costs nothing
    // because the device remains the authority - a wrong code of any length
    // fails its own tag check. Built in place so there is no second copy of
    // the code to scrub, and six characters never leave the string's own
    // storage.
    m_code.assign(6 - code.size(), '0');
    m_code.append(code.data(), code.size());

    // generator = ELLIGATOR2(SHA-512(prefix || code || padding || h || 0x00)[:32])
    CodeDerived32 generator{};
    cpace_derive_generator(m_code, m_h, generator.data());

    // CPace does not clamp its scalar, but libsodium's X25519 clamps
    // unconditionally; the IRTF draft allows this and the reference
    // implementation behaves the same way.
    NoisePrivKey priv{};
    crypto::generate_random_bytes_thread_safe(priv.size(), priv.data());

    CpaceHostTag out{};
    // cpace_host_public_key = X25519(cpace_host_private_key, generator)
    noise_crypto::x25519(priv, generator, out.cpace_host_public_key);

    // tag = SHA-256(X25519(cpace_host_private_key, cpace_trezor_public_key))
    CodeDerived32 shared{};
    noise_crypto::x25519(priv, m_cpace_trezor_pub, shared);
    crypto_hash_sha256(out.tag.data(), shared.data(), shared.size());

    m_have_host_tag = true;
    return out;
  }

  bool CodeEntryPairing::consume_secret(const uint8_t *secret, size_t len)
  {
    if (!m_have_host_tag) {
      throw exc::ProtocolException("THP pairing: consume_secret before host tag sent");
    }
    if (len != 16) {
      throw exc::ProtocolException("THP pairing: ThpCodeEntrySecret.secret must be 16 bytes");
    }

    // (a) commitment == SHA-256(secret)
    NoiseHash expected_commit{};
    crypto_hash_sha256(expected_commit.data(), secret, len);
    if (sodium_memcmp(expected_commit.data(), m_commitment.data(), 32) != 0) {
      return false;
    }

    // (b) code == SHA-256(ThpPairingMethod_CodeEntry || h || secret || challenge)
    //     read big-endian, mod 1000000. CodeEntry is enum value 2 and the
    //     hash input takes the raw enum number as one byte.
    std::vector<uint8_t> chash_in;
    chash_in.reserve(1 + m_h.size() + len + m_challenge.size());
    chash_in.push_back(0x02);
    chash_in.insert(chash_in.end(), m_h.begin(), m_h.end());
    chash_in.insert(chash_in.end(), secret, secret + len);
    chash_in.insert(chash_in.end(), m_challenge.begin(), m_challenge.end());
    NoiseHash code_hash{};
    crypto_hash_sha256(code_hash.data(), chash_in.data(), chash_in.size());
    memwipe(chash_in.data(), chash_in.size());

    uint64_t derived = 0;
    for (uint8_t b : code_hash) derived = (derived * 256 + b) % 1000000;

    // m_code was validated as digits by build_host_tag.
    uint64_t entered = 0;
    for (char c : m_code) entered = entered * 10 + uint64_t(c - '0');

    if (entered != derived) {
      return false;
    }
    m_paired = true;
    return true;
  }

}}}
