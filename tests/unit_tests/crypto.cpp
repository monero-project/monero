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

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

extern "C"
{
#include "crypto/crypto-ops.h"
#include "mx25519.h"
}
#include "crypto/x25519.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/merge_mining.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"
#include "string_tools.h"

namespace
{
  static constexpr const std::uint8_t source[] = {
    0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1, 0xad, 0xd0, 0xea,
    0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94,
    0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94,
    0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1, 0xad, 0xd0, 0xea
  };

  static constexpr const char expected[] =
    "8b655970153799af2aeadc9ff1add0ea6c7251d54154cfa92c173a0dd39c1f94"
    "6c7251d54154cfa92c173a0dd39c1f948b655970153799af2aeadc9ff1add0ea";

  template<typename T> void *addressof(T &t) { return &t; }
  template<> void *addressof(crypto::secret_key &k) { return addressof(unwrap(unwrap(k))); }

  template<typename T>
  bool is_formatted()
  {
    T value{};

    static_assert(alignof(T) == 1, "T must have 1 byte alignment");
    static_assert(sizeof(T) <= sizeof(source), "T is too large for source");
    static_assert(sizeof(T) * 2 <= sizeof(expected), "T is too large for destination");
    std::memcpy(addressof(value), source, sizeof(T));

    std::stringstream out;
    out << "BEGIN" << value << "END";  
    return out.str() == "BEGIN<" + std::string{expected, sizeof(T) * 2} + ">END";
  }

  std::vector<const mx25519_impl*> get_available_mx25519_impls()
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

  std::string get_name_of_mx25519_impl(const mx25519_impl* impl)
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
}

static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
  return memcmp(&a, &b, sizeof(mx25519_pubkey)) == 0;
}

static inline bool operator!=(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
  return !(a == b);
}

TEST(Crypto, Ostream)
{
  EXPECT_TRUE(is_formatted<crypto::hash8>());
  EXPECT_TRUE(is_formatted<crypto::hash>());
  EXPECT_TRUE(is_formatted<crypto::public_key>());
  EXPECT_TRUE(is_formatted<crypto::signature>());
  EXPECT_TRUE(is_formatted<crypto::key_derivation>());
  EXPECT_TRUE(is_formatted<crypto::key_image>());
  EXPECT_TRUE(is_formatted<rct::key>());
}

TEST(Crypto, null_keys)
{
  char zero[32];
  memset(zero, 0, 32);
  ASSERT_EQ(memcmp(crypto::null_skey.data, zero, 32), 0);
  ASSERT_EQ(memcmp(crypto::null_pkey.data, zero, 32), 0);
}

TEST(Crypto, verify_32)
{
  // all bytes are treated the same, so we can brute force just one byte
  unsigned char k0[32] = {0}, k1[32] = {0};
  for (unsigned int i0 = 0; i0 < 256; ++i0)
  {
    k0[0] = i0;
    for (unsigned int i1 = 0; i1 < 256; ++i1)
    {
      k1[0] = i1;
      ASSERT_EQ(!crypto_verify_32(k0, k1), i0 == i1);
    }
  }
}

TEST(Crypto, tree_branch)
{
  crypto::hash inputs[6];
  crypto::hash branch[8];
  crypto::hash branch_1[8 + 1];
  crypto::hash root, root2;
  size_t depth;
  uint32_t path, path2;

  auto hasher = [](const crypto::hash &h0, const crypto::hash &h1) -> crypto::hash
  {
    char buffer[64];
    memcpy(buffer, &h0, 32);
    memcpy(buffer + 32, &h1, 32);
    crypto::hash res;
    cn_fast_hash(buffer, 64, res);
    return res;
  };

  for (int n = 0; n < 6; ++n)
  {
    memset(&inputs[n], 0, 32);
    inputs[n].data[0] = n + 1;
  }

  // empty
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 0, crypto::null_hash.data, (char(*)[32])branch, &depth, &path));

  // one, matching
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 1, inputs[0].data, (char(*)[32])branch, &depth, &path));
  ASSERT_EQ(depth, 0);
  ASSERT_EQ(path, 0);
  ASSERT_TRUE(crypto::tree_path(1, 0, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 1, root.data);
  ASSERT_EQ(root, inputs[0]);
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // one, not found
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 1, inputs[1].data, (char(*)[32])branch, &depth, &path));

  // two, index 0
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 2, inputs[0].data, (char(*)[32])branch, &depth, &path));
  ASSERT_EQ(depth, 1);
  ASSERT_EQ(path, 0);
  ASSERT_TRUE(crypto::tree_path(2, 0, &path2));
  ASSERT_EQ(path, path2);
  ASSERT_EQ(branch[0], inputs[1]);
  crypto::tree_hash((const char(*)[32])inputs, 2, root.data);
  ASSERT_EQ(root, hasher(inputs[0], inputs[1]));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // two, index 1
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 2, inputs[1].data, (char(*)[32])branch, &depth, &path));
  ASSERT_EQ(depth, 1);
  ASSERT_EQ(path, 1);
  ASSERT_TRUE(crypto::tree_path(2, 1, &path2));
  ASSERT_EQ(path, path2);
  ASSERT_EQ(branch[0], inputs[0]);
  crypto::tree_hash((const char(*)[32])inputs, 2, root.data);
  ASSERT_EQ(root, hasher(inputs[0], inputs[1]));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // two, not found
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 2, inputs[2].data, (char(*)[32])branch, &depth, &path));

  // a b c 0
  //  x   y
  //    z

  // three, index 0
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 3, inputs[0].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 1);
  ASSERT_LE(depth, 2);
  ASSERT_TRUE(crypto::tree_path(3, 0, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 3, root.data);
  ASSERT_EQ(root, hasher(inputs[0], hasher(inputs[1], inputs[2])));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // three, index 1
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 3, inputs[1].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 1);
  ASSERT_LE(depth, 2);
  ASSERT_TRUE(crypto::tree_path(3, 1, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 3, root.data);
  ASSERT_EQ(root, hasher(inputs[0], hasher(inputs[1], inputs[2])));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // three, index 2
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 3, inputs[2].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 1);
  ASSERT_LE(depth, 2);
  ASSERT_TRUE(crypto::tree_path(3, 2, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 3, root.data);
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::tree_branch_hash(inputs[2].data, (const char(*)[32])branch, depth, path, root2.data));
  ASSERT_EQ(root, root2);

  // three, not found
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 3, inputs[3].data, (char(*)[32])branch, &depth, &path));

  // a b c d e 0 0 0
  //    x   y
  //      z
  //    w

  // five, index 0
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 5, inputs[0].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 2);
  ASSERT_LE(depth, 3);
  ASSERT_TRUE(crypto::tree_path(5, 0, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 5, root.data);
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[5].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // five, index 1
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 5, inputs[1].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 2);
  ASSERT_LE(depth, 3);
  ASSERT_TRUE(crypto::tree_path(5, 1, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 5, root.data);
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[5].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // five, index 2
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 5, inputs[2].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 2);
  ASSERT_LE(depth, 3);
  ASSERT_TRUE(crypto::tree_path(5, 2, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 5, root.data);
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[5].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // five, index 4
  ASSERT_TRUE(crypto::tree_branch((const char(*)[32])inputs, 5, inputs[4].data, (char(*)[32])branch, &depth, &path));
  ASSERT_GE(depth, 2);
  ASSERT_LE(depth, 3);
  ASSERT_TRUE(crypto::tree_path(5, 4, &path2));
  ASSERT_EQ(path, path2);
  crypto::tree_hash((const char(*)[32])inputs, 5, root.data);
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[0].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[1].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[2].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[3].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_TRUE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[5].data, root.data, (const char(*)[32])branch, depth, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(crypto::null_hash.data, root.data, (const char(*)[32])branch, depth, path));

  // a version with an extra (dummy) hash
  memcpy(branch_1, branch, sizeof(branch));
  branch_1[depth] = crypto::null_hash;

  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth - 1, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch_1, depth + 1, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 1));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 2));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 3));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])(branch_1 + 1), depth, path));

  // five, not found
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 5, crypto::null_hash.data, (char(*)[32])branch, &depth, &path));

  // depth encoding roundtrip
  for (uint32_t n_chains = 1; n_chains <= 256; ++n_chains)
  {
    for (uint32_t nonce = 0xffffffff - 512; nonce != 1025; ++nonce)
    {
      const uint64_t depth = cryptonote::encode_mm_depth(n_chains, nonce);
      uint32_t n_chains_2, nonce_2;
      ASSERT_TRUE(cryptonote::decode_mm_depth(depth, n_chains_2, nonce_2));
      ASSERT_EQ(n_chains, n_chains_2);
      ASSERT_EQ(nonce, nonce_2);
    }
  }

  // 257 chains is too much
  try { cryptonote::encode_mm_depth(257, 0); ASSERT_TRUE(false); }
  catch (...) {}
}

TEST(Crypto, generator_consistency)
{
  // crypto/generators.h
  const crypto::public_key G{crypto::get_G()};
  const crypto::public_key H{crypto::get_H()};
  const ge_p3 H_p3 = crypto::get_H_p3();

  // crypto/crypto-ops.h
  ASSERT_TRUE(memcmp(&H_p3, &ge_p3_H, sizeof(ge_p3)) == 0);

  // ringct/rctOps.h
  ASSERT_TRUE(memcmp(G.data, rct::G.bytes, 32) == 0);

  // ringct/rctTypes.h
  ASSERT_TRUE(memcmp(H.data, rct::H.bytes, 32) == 0);
}

TEST(Crypto, x25519_impl_scmul_key_convergence)
{
  std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  if (available_impls.size() < 2)
    return;

  dump_mx25519_impls(available_impls);

  crypto::x25519_pubkey P_fixed;
  epee::string_tools::hex_to_pod("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a", P_fixed);

  crypto::x25519_pubkey P_random;
  rct::key tmp = rct::pkGen();
  edwards_bytes_to_x25519_vartime(P_random.data, tmp.bytes);

  tmp = rct::skGen();
  mx25519_privkey a;
  memcpy(&a, &tmp, sizeof(a));

  mx25519_pubkey res_fixed;
  mx25519_pubkey res_random;

  bool first = true;
  for (const mx25519_impl *impl : available_impls)
  {
    mx25519_pubkey tmp_mx;
    mx25519_scmul_key(impl, &tmp_mx, &a, &P_fixed);
    if (!first)
    {
      EXPECT_EQ(res_fixed, tmp_mx);
    }
    
    memcpy(&res_fixed, &tmp_mx, 32);

    mx25519_scmul_key(impl, &tmp_mx, &a, &P_random);
    if (!first)
    {
      EXPECT_EQ(res_random, tmp_mx);
    }

    memcpy(&res_random, &tmp_mx, 32);

    first = false;
  }
}

TEST(Crypto, x25519_secret_key_1_scmul_base)
{
  const crypto::secret_key one({crypto::ec_scalar{.data = {1}}});
  const mx25519_pubkey B{.data={9}};

  const std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  for (const mx25519_impl *impl : available_impls)
  {
    mx25519_pubkey B1;
    mx25519_scmul_base(impl, &B1, reinterpret_cast<const mx25519_privkey*>(&one));

    EXPECT_EQ(B, B1);
    if (B1 != B)
    {
      std::cout << "Failure occurred with " << get_name_of_mx25519_impl(impl) << " impl" << std::endl;
    }
  }
}

TEST(Crypto, x25519_secret_key_2_scmul_base)
{
  const crypto::secret_key two({crypto::ec_scalar{.data = {2}}});
  const rct::key G_doubled = rct::addKeys(rct::G, rct::G);
  mx25519_pubkey B_doubled;
  edwards_bytes_to_x25519_vartime(B_doubled.data, G_doubled.bytes);

  const std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  for (const mx25519_impl *impl : available_impls)
  {
    mx25519_pubkey B2;
    mx25519_scmul_base(impl, &B2, reinterpret_cast<const mx25519_privkey*>(&two));

    EXPECT_EQ(B_doubled, B2);
    if (B2 != B_doubled)
    {
      std::cout << "Failure occurred with " << get_name_of_mx25519_impl(impl) << " impl" << std::endl;
    }
  }
}

TEST(Crypto, x25519_secret_key_4_scmul_base)
{
  const crypto::secret_key four({crypto::ec_scalar{.data = {4}}});
  const rct::key G_quad = rct::scalarmultBase({.bytes = {4}});
  mx25519_pubkey B_quad;
  edwards_bytes_to_x25519_vartime(B_quad.data, G_quad.bytes);

  const std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  for (const mx25519_impl *impl : available_impls)
  {
    mx25519_pubkey B4;
    mx25519_scmul_base(impl, &B4, reinterpret_cast<const mx25519_privkey*>(&four));

    EXPECT_EQ(B_quad, B4);
    if (B4 != B_quad)
    {
      std::cout << "Failure occurred with " << get_name_of_mx25519_impl(impl) << " impl" << std::endl;
    }
  }
}

TEST(Crypto, x25519_secret_key_8_scmul_base)
{
  const crypto::secret_key eight({crypto::ec_scalar{.data = {8}}});
  const rct::key G_oct = rct::scalarmultBase({.bytes = {8}});
  mx25519_pubkey B_oct;
  edwards_bytes_to_x25519_vartime(B_oct.data, G_oct.bytes);

  const std::vector<const mx25519_impl*> available_impls = get_available_mx25519_impls();

  for (const mx25519_impl *impl : available_impls)
  {
    mx25519_pubkey B8;
    mx25519_scmul_base(impl, &B8, reinterpret_cast<const mx25519_privkey*>(&eight));

    EXPECT_EQ(B_oct, B8);
    if (B8 != B_oct)
    {
      std::cout << "Failure occurred with " << get_name_of_mx25519_impl(impl) << " impl" << std::endl;
    }
  }
}

TEST(Crypto, ConvertPointE_Base)
{
  const crypto::public_key G = crypto::get_G();
  const crypto::x25519_pubkey B_expected = {{9}};

  crypto::x25519_pubkey B_actual;
  edwards_bytes_to_x25519_vartime(B_actual.data, to_bytes(G));

  EXPECT_EQ(B_expected, B_actual);
}

TEST(Crypto, ConvertPointE_PreserveScalarMultBase)
{
  // *clamped* private key a
  const crypto::secret_key a = rct::rct2sk(rct::skGen());
  rct::key a_key;
  memcpy(&a_key, &a, sizeof(rct::key));

  // P_ed = a G
  const rct::key P_edward = rct::scalarmultBase(a_key);

  // P_mont = a B
  crypto::x25519_pubkey P_mont;
  crypto::x25519_scmul_base(a, P_mont);

  // P_mont' = ConvertPointE(P_ed)
  crypto::x25519_pubkey P_mont_converted;
  edwards_bytes_to_x25519_vartime(P_mont_converted.data, P_edward.bytes);

  // P_mont' ?= P_mont
  EXPECT_EQ(P_mont_converted, P_mont);
}

TEST(Crypto, ConvertPointE_PreserveScalarMultBase_gep3)
{
  // compared to ConvertPointE_PreserveScalarMultBase, this test will use Z != 1 (probably)

  const crypto::secret_key a = rct::rct2sk(rct::skGen());
  rct::key a_key;
  memcpy(&a_key, &a, sizeof(rct::key));

  // P_ed = a G
  ge_p3 P_p3;
  ge_scalarmult_base(&P_p3, to_bytes(a));

  // check that Z != 1, otherwise this test is a dup of ConvertPointE_PreserveScalarMultBase
  rct::key Z_bytes;
  fe_tobytes(Z_bytes.bytes, P_p3.Z);
  ASSERT_NE(Z_bytes, rct::I); // check Z != 1

  // P_mont = a B
  crypto::x25519_pubkey P_mont;
  crypto::x25519_scmul_base(a, P_mont);

  // P_mont' = ConvertPointE(P_ed)
  crypto::x25519_pubkey P_mont_converted;
  ge_p3_to_x25519(P_mont_converted.data, &P_p3);

  // P_mont' ?= P_mont
  EXPECT_EQ(P_mont_converted, P_mont);
}

TEST(Crypto, ConvertPointE_EraseSign)
{
  // generate a random point P and test that ConvertPointE(P) == ConvertPointE(-P)

  const rct::key P = rct::pkGen();
  rct::key negP;
  rct::subKeys(negP, rct::I, P);

  crypto::x25519_pubkey P_mont;
  edwards_bytes_to_x25519_vartime(P_mont.data, P.bytes);

  crypto::x25519_pubkey negP_mont;
  edwards_bytes_to_x25519_vartime(negP_mont.data, negP.bytes);

  EXPECT_EQ(P_mont, negP_mont);
}
