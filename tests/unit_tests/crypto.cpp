// Copyright (c) 2017-2022, The Monero Project
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

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/merge_mining.h"

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
}

TEST(Crypto, Ostream)
{
  EXPECT_TRUE(is_formatted<crypto::hash8>());
  EXPECT_TRUE(is_formatted<crypto::hash>());
  EXPECT_TRUE(is_formatted<crypto::public_key>());
  EXPECT_TRUE(is_formatted<crypto::secret_key>());
  EXPECT_TRUE(is_formatted<crypto::signature>());
  EXPECT_TRUE(is_formatted<crypto::key_derivation>());
  EXPECT_TRUE(is_formatted<crypto::key_image>());
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

  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth - 1, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth + 1, path));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 1));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 2));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])branch, depth, path ^ 3));
  ASSERT_FALSE(crypto::is_branch_in_tree(inputs[4].data, root.data, (const char(*)[32])(branch + 1), depth, path));

  // five, not found
  ASSERT_FALSE(crypto::tree_branch((const char(*)[32])inputs, 5, crypto::null_hash.data, (char(*)[32])branch, &depth, &path));

  // depth encoding roundtrip
  for (uint32_t n_chains = 1; n_chains <= 65; ++n_chains)
  {
    for (uint32_t nonce = 0; nonce < 1024; ++nonce)
    {
      const uint32_t depth = cryptonote::encode_mm_depth(n_chains, nonce);
      uint32_t n_chains_2, nonce_2;
      ASSERT_TRUE(cryptonote::decode_mm_depth(depth, n_chains_2, nonce_2));
      ASSERT_EQ(n_chains, n_chains_2);
      ASSERT_EQ(nonce, nonce_2);
    }
  }
}
