// Copyright (c) 2017-2024, The Monero Project
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

#include <cstring>
#include <gtest/gtest.h>
#include <vector>

#include "common/container_helpers.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "crypto/generators.h"
#include "misc_log_ex.h"
#include "mx25519.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

namespace
{
  static std::vector<const mx25519_impl*> get_available_mx25519_impls()
  {
    static constexpr const mx25519_type ALL_IMPL_TYPES[4] = {MX25519_TYPE_PORTABLE,
    MX25519_TYPE_ARM64,
    MX25519_TYPE_AMD64,
    MX25519_TYPE_AMD64X};
    static constexpr const size_t NUM_IMPLS = sizeof(ALL_IMPL_TYPES) / sizeof(ALL_IMPL_TYPES[0]);

    std::vector<const mx25519_impl*> available_impls;
    available_impls.reserve(NUM_IMPLS);
    for (int i = 0; i < NUM_IMPLS; ++i)
    {
      const mx25519_type impl_type = ALL_IMPL_TYPES[i];
      const mx25519_impl * const impl = mx25519_select_impl(impl_type);
      if (nullptr == impl)
        continue;
      available_impls.push_back(impl);
    }

    return available_impls;
  }

  static std::string get_name_of_mx25519_impl(const mx25519_impl* impl)
  {
#   define get_name_of_mx25519_impl_CASE(x) case x: return #x;
    CHECK_AND_ASSERT_THROW_MES(impl != nullptr, "null impl");
    const mx25519_type impl_type = mx25519_impl_type(impl);
    switch (impl_type)
    {
    get_name_of_mx25519_impl_CASE(MX25519_TYPE_PORTABLE)
    get_name_of_mx25519_impl_CASE(MX25519_TYPE_ARM64)
    get_name_of_mx25519_impl_CASE(MX25519_TYPE_AMD64)
    get_name_of_mx25519_impl_CASE(MX25519_TYPE_AMD64X)
    default:
      throw std::runtime_error("get name of mx25519 impl: unrecognized impl type");
    }
#   undef get_name_of_mx25519_impl_CASE
  }

  void dump_mx25519_impls(const std::vector<const mx25519_impl*> &impls)
  {
    std::cout << "Testing " << impls.size() << " mx25519 implementations:" << std::endl;
    for (const mx25519_impl *impl : impls)
      std::cout << "    - " << get_name_of_mx25519_impl(impl) << std::endl;
  }

  template <typename T>
  static T hex2pod(boost::string_ref s)
  {
    T v;
    if (!epee::string_tools::hex_to_pod(s, v))
      throw std::runtime_error("hex2pod conversion failed");
    return v;
  }
} // namespace

static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
  return memcmp(&a, &b, sizeof(mx25519_pubkey)) == 0;
}

static inline bool operator!=(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
  return !(a == b);
}

TEST(x25519, scmul_key_convergence)
{
  std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  ASSERT_GT(available_impls.size(), 0);

  dump_mx25519_impls(available_impls);

  std::vector<mx25519_privkey> scalars;
  for (int i = 0; i <= 254; ++i)
  {
    for (unsigned char j = 0; j < 8; ++j)
    {
      // add 2^i + j (sometimes with duplicates, which is okay)
      mx25519_privkey &s = tools::add_element(scalars);
      memset(s.data, 0, sizeof(mx25519_privkey));
      const int msb_byte_index = i >> 3;
      const int msb_bit_index = i & 7;
      s.data[msb_byte_index] = 1 << msb_bit_index;
      s.data[0] = j;
    }
  }
  // add -1
  scalars.push_back(hex2pod<mx25519_privkey>("ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"));
  // add random
  const rct::key a = rct::skGen();
  memcpy(tools::add_element(scalars).data, &a, sizeof(mx25519_privkey));

  std::vector<std::pair<rct::key, mx25519_pubkey>> points;
  // add base point
  points.push_back({rct::G, mx25519_pubkey{.data={9}}});
  // add RFC 7784 test point
  points.push_back({
    hex2pod<rct::key>("8120f299c37ae1ca64a179f638a6c6fafde968f1c33705e28c413c7579d9884f"),
    hex2pod<mx25519_pubkey>("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a")
  });
  // add random point
  const rct::key P_random = rct::pkGen();
  mx25519_pubkey P_random_x;
  edwards_bytes_to_x25519_vartime(P_random_x.data, P_random.bytes);
  points.push_back({P_random, P_random_x});

  for (const mx25519_privkey &scalar : scalars)
  {
    for (const auto &point : points)
    {
      // D1 = ConvertPointE(a * P_base)
      ge_p3 P_ed;
      ASSERT_EQ(0, ge_frombytes_vartime(&P_ed, point.first.bytes));
      ge_p3 res_p3;
      ge_scalarmult_p3(&res_p3, scalar.data, &P_ed);
      mx25519_pubkey res;
      ge_p3_to_x25519(res.data, &res_p3);

      for (const mx25519_impl *impl : available_impls)
      {
        // D2 = a * D_base
        mx25519_pubkey res_mx;
        mx25519_scmul_key(impl, &res_mx, &scalar, &point.second);

        // D1 ?= D2
        EXPECT_EQ(res, res_mx);
      }
    }
  }
}

TEST(x25519, ConvertPointE_Base)
{
  const crypto::public_key G = crypto::get_G();
  const mx25519_pubkey B_expected = {{9}};

  mx25519_pubkey B_actual;
  edwards_bytes_to_x25519_vartime(B_actual.data, to_bytes(G));

  EXPECT_EQ(B_expected, B_actual);
}

TEST(x25519, ConvertPointE_EraseSign)
{
  // generate a random point P and test that ConvertPointE(P) == ConvertPointE(-P)

  const rct::key P = rct::pkGen();
  rct::key negP;
  rct::subKeys(negP, rct::I, P);

  mx25519_pubkey P_mont;
  edwards_bytes_to_x25519_vartime(P_mont.data, P.bytes);

  mx25519_pubkey negP_mont;
  edwards_bytes_to_x25519_vartime(negP_mont.data, negP.bytes);

  EXPECT_EQ(P_mont, negP_mont);
}
