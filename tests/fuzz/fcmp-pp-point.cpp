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

// Property-checks the FCMP++ point helpers (torsion clearing, Ed25519 -> Weierstrass
// conversion, batch inversion, Selene scalar FFI) on arbitrary 32-byte encodings.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "include_base_utils.h"
#include "fcmp_pp/fcmp_pp_crypto.h"
#include "fcmp_pp/tower_cycle.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

static constexpr size_t kMaxPoints = 16;

struct converted_point
{
  fcmp_pp::EdDerivatives derivatives;
  crypto::ec_coord wei_x;
  crypto::ec_coord wei_y;
};

static bool fe_bytes_equal(const fe f, const unsigned char *bytes)
{
  unsigned char f_bytes[32];
  fe_tobytes(f_bytes, f);
  return memcmp(f_bytes, bytes, 32) == 0;
}

static bool mul8_is_identity(const unsigned char *encoded)
{
  ge_p3 p3;
  FUZZ_CHECK(ge_frombytes_vartime(&p3, encoded) == 0);
  return fcmp_pp::mul8_is_identity_vartime(p3);
}

// Wei x must be the X25519 u-coordinate plus A/3, and (u, wei y) must be on v^2 = u^3 + A*u^2 + u.
static void check_against_montgomery(const crypto::ec_point &point, const crypto::ec_coord &wei_x, const crypto::ec_coord &wei_y)
{
  unsigned char u_bytes[32];
  FUZZ_CHECK(edwards_bytes_to_x25519_vartime(u_bytes, to_bytes(point)) == 0);
  fe u, v;
  FUZZ_CHECK(fe_frombytes_vartime(u, u_bytes) == 0);
  FUZZ_CHECK(fe_frombytes_vartime(v, to_bytes(wei_y)) == 0);

  fe expected_wei_x;
  fe_add(expected_wei_x, u, fe_a_inv_3);
  FUZZ_CHECK(fe_bytes_equal(expected_wei_x, to_bytes(wei_x)));

  fe u2, u3, a_u2, partial, partial_reduced, rhs, lhs;
  fe_sq(u2, u);
  fe_mul(u3, u2, u);
  fe_mul(a_u2, fe_a, u2);
  fe_add(partial, u3, a_u2);
  FUZZ_CHECK(fe_reduce_vartime(partial_reduced, partial) == 0);
  fe_add(rhs, partial_reduced, u);
  fe_sq(lhs, v);
  unsigned char rhs_bytes[32];
  fe_tobytes(rhs_bytes, rhs);
  FUZZ_CHECK(fe_bytes_equal(lhs, rhs_bytes));
}

static void check_selene_scalar_round_trip(const unsigned char *bytes)
{
  SeleneScalar scalar;
  FUZZ_CHECK(::selene_scalar_from_bytes(bytes, &scalar) == 0);
  unsigned char out[32];
  ::selene_scalar_to_bytes(&scalar, out);
  FUZZ_CHECK(memcmp(out, bytes, 32) == 0);
}

// The C field parser ignores bit 255, so only compare canonicity when it is clear.
static void check_raw_selene_scalar(const unsigned char *bytes)
{
  if (bytes[31] & 0x80)
    return;
  fe f;
  SeleneScalar scalar;
  const bool fe_ok = fe_frombytes_vartime(f, bytes) == 0;
  const bool selene_ok = ::selene_scalar_from_bytes(bytes, &scalar) == 0;
  FUZZ_CHECK(fe_ok == selene_ok);
  if (selene_ok)
    check_selene_scalar_round_trip(bytes);
}

static void check_point(const crypto::ec_point &point, std::vector<converted_point> &converted)
{
  ge_p3 p3;
  const bool decodes = ge_frombytes_vartime(&p3, to_bytes(point)) == 0;

  crypto::ec_point cleared;
  const bool valid = fcmp_pp::get_valid_torsion_cleared_point_vartime(point, cleared);
  if (!decodes)
  {
    FUZZ_CHECK(!valid);
    return;
  }
  FUZZ_CHECK(valid == !fcmp_pp::mul8_is_identity_vartime(p3));
  if (!valid)
    return;

  FUZZ_CHECK(!(cleared == fcmp_pp::EC_I));
  FUZZ_CHECK(!mul8_is_identity(to_bytes(cleared)));

  crypto::ec_point cleared_again;
  FUZZ_CHECK(fcmp_pp::get_valid_torsion_cleared_point_vartime(cleared, cleared_again));
  FUZZ_CHECK(cleared_again == cleared);

  // The removed part must be pure torsion.
  ge_p3 cleared_p3, diff_p3;
  FUZZ_CHECK(ge_frombytes_vartime(&cleared_p3, to_bytes(cleared)) == 0);
  ge_cached cleared_cached;
  ge_p3_to_cached(&cleared_cached, &cleared_p3);
  ge_p1p1 diff;
  ge_sub(&diff, &p3, &cleared_cached);
  ge_p1p1_to_p3(&diff_p3, &diff);
  FUZZ_CHECK(fcmp_pp::mul8_is_identity_vartime(diff_p3));

  converted_point c;
  FUZZ_CHECK(fcmp_pp::point_to_wei_x_y(cleared, c.wei_x, c.wei_y));
  FUZZ_CHECK(fcmp_pp::point_to_ed_derivatives(cleared, c.derivatives));
  check_against_montgomery(cleared, c.wei_x, c.wei_y);
  check_selene_scalar_round_trip(to_bytes(c.wei_x));
  check_selene_scalar_round_trip(to_bytes(c.wei_y));
  converted.push_back(c);
}

// Mirrors the batched path used by curve_trees when converting outputs to leaves.
static void check_batch(const std::vector<converted_point> &converted, const uint8_t *zero_selector)
{
  const size_t n = converted.size() * 2;
  if (n == 0)
    return;
  std::unique_ptr<fe[]> in = std::make_unique<fe[]>(n);
  std::unique_ptr<fe[]> out = std::make_unique<fe[]>(n);
  for (size_t i = 0; i < converted.size(); ++i)
  {
    memcpy(&in[2 * i], &converted[i].derivatives.one_minus_y, sizeof(fe));
    memcpy(&in[2 * i + 1], &converted[i].derivatives.one_minus_y_mul_x, sizeof(fe));
  }
  FUZZ_CHECK(fe_batch_invert(out.get(), in.get(), n) == 0);
  for (size_t i = 0; i < converted.size(); ++i)
  {
    crypto::ec_coord wei_x, wei_y;
    fe_ed_derivatives_to_wei_x_y(to_bytes(wei_x), to_bytes(wei_y), out[2 * i], converted[i].derivatives.one_plus_y, out[2 * i + 1]);
    FUZZ_CHECK(memcmp(&wei_x, &converted[i].wei_x, sizeof(wei_x)) == 0);
    FUZZ_CHECK(memcmp(&wei_y, &converted[i].wei_y, sizeof(wei_y)) == 0);
  }

  if (zero_selector)
  {
    fe_0(in[*zero_selector % n]);
    FUZZ_CHECK(fe_batch_invert(out.get(), in.get(), n) != 0);
  }
}

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  std::vector<converted_point> converted;
  const size_t n_points = std::min(len / 32, kMaxPoints);
  for (size_t i = 0; i < n_points; ++i)
  {
    crypto::ec_point point;
    memcpy(&point, buf + 32 * i, 32);
    check_raw_selene_scalar(buf + 32 * i);
    check_point(point, converted);
  }
  const uint8_t *zero_selector = len > 32 * n_points ? buf + 32 * n_points : NULL;
  check_batch(converted, zero_selector);
END_SIMPLE_FUZZER()
