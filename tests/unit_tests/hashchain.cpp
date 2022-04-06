// Copyright (c) 2014-2022, The Monero Project
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

// FIXME: move this into a full wallet2 unit test suite, if possible

#include "gtest/gtest.h"

#include "wallet/wallet2.h"

static crypto::hash make_hash(uint64_t n)
{
  union
  {
    crypto::hash hash;
    uint64_t n;
  } hash;
  hash.hash = crypto::null_hash;
  hash.n = n;
  return hash.hash;
}

TEST(hashchain, empty)
{
  tools::hashchain hashchain;
  ASSERT_EQ(hashchain.size(), 0);
  ASSERT_EQ(hashchain.offset(), 0);
}

TEST(hashchain, genesis)
{
  tools::hashchain hashchain;
  hashchain.push_back(make_hash(1));
  ASSERT_EQ(hashchain.size(), 1);
  ASSERT_EQ(hashchain.genesis(), make_hash(1));
  hashchain.push_back(make_hash(2));
  ASSERT_EQ(hashchain.size(), 2);
  ASSERT_EQ(hashchain.genesis(), make_hash(1));
}

TEST(hashchain, push_back)
{
  tools::hashchain hashchain;
  hashchain.push_back(make_hash(1));
  hashchain.push_back(make_hash(2));
  hashchain.push_back(make_hash(3));
  ASSERT_EQ(hashchain[0], make_hash(1));
  ASSERT_EQ(hashchain[1], make_hash(2));
  ASSERT_EQ(hashchain[2], make_hash(3));
}

TEST(hashchain, clear_empty)
{
  tools::hashchain hashchain;
  ASSERT_TRUE(hashchain.empty());
  hashchain.push_back(make_hash(1));
  ASSERT_FALSE(hashchain.empty());
  hashchain.push_back(make_hash(2));
  ASSERT_FALSE(hashchain.empty());
  hashchain.clear();
  ASSERT_TRUE(hashchain.empty());
}

TEST(hashchain, crop)
{
  tools::hashchain hashchain;
  hashchain.push_back(make_hash(1));
  hashchain.push_back(make_hash(2));
  hashchain.push_back(make_hash(3));
  ASSERT_EQ(hashchain.size(), 3);
  ASSERT_EQ(hashchain[0], make_hash(1));
  ASSERT_EQ(hashchain[1], make_hash(2));
  ASSERT_EQ(hashchain[2], make_hash(3));
  hashchain.crop(3);
  ASSERT_EQ(hashchain.size(), 3);
  hashchain.crop(2);
  ASSERT_EQ(hashchain.size(), 2);
  ASSERT_EQ(hashchain[0], make_hash(1));
  ASSERT_EQ(hashchain[1], make_hash(2));
  ASSERT_EQ(hashchain.genesis(), make_hash(1));
  hashchain.crop(0);
  ASSERT_TRUE(hashchain.empty());
  ASSERT_EQ(hashchain.size(), 0);
  hashchain.push_back(make_hash(5));
  ASSERT_EQ(hashchain.genesis(), make_hash(5));
  ASSERT_EQ(hashchain.size(), 1);
}

TEST(hashchain, trim)
{
  tools::hashchain hashchain;
  hashchain.push_back(make_hash(1));
  hashchain.push_back(make_hash(2));
  hashchain.push_back(make_hash(3));
  ASSERT_EQ(hashchain.offset(), 0);
  hashchain.trim(2);
  ASSERT_EQ(hashchain.offset(), 2);
  ASSERT_EQ(hashchain.size(), 3);
  ASSERT_EQ(hashchain[2], make_hash(3));
  hashchain.trim(3);
  ASSERT_EQ(hashchain.offset(), 2); // never gets it empty
  ASSERT_EQ(hashchain.size(), 3);
  ASSERT_FALSE(hashchain.empty());
  ASSERT_EQ(hashchain.genesis(), make_hash(1));
}
