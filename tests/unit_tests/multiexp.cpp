// Copyright (c) 2018-2024, The Monero Project

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

#include "gtest/gtest.h"

#include "crypto/crypto.h"
#include "ringct/rctOps.h"
#include "ringct/multiexp.h"

#define TESTSCALAR []{ static const rct::key TESTSCALAR = rct::skGen(); return TESTSCALAR; }()
#define TESTPOW2SCALAR []{ static const rct::key TESTPOW2SCALAR = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; return TESTPOW2SCALAR; }()
#define TESTSMALLSCALAR []{ static const rct::key TESTSMALLSCALAR = {{5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; return TESTSMALLSCALAR; }()
#define TESTPOINT []{ static const rct::key TESTPOINT = rct::scalarmultBase(rct::skGen()); return TESTPOINT; }()

static rct::key basic(const std::vector<rct::MultiexpData> &data)
{
  ge_p3 res_p3 = ge_p3_identity;
  for (const auto &d: data)
  {
    ge_cached cached;
    ge_p3 p3;
    ge_p1p1 p1;
    ge_scalarmult_p3(&p3, d.scalar.bytes, &d.point);
    ge_p3_to_cached(&cached, &p3);
    ge_add(&p1, &res_p3, &cached);
    ge_p1p1_to_p3(&res_p3, &p1);
  }
  rct::key res;
  ge_p3_tobytes(res.bytes, &res_p3);
  return res;
}

static ge_p3 get_p3(const rct::key &point)
{
  ge_p3 p3;
  EXPECT_TRUE(ge_frombytes_vartime(&p3, point.bytes) == 0);
  return p3;
}

TEST(multiexp, bos_coster_empty)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
}

TEST(multiexp, straus_empty)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == straus(data));
}

TEST(multiexp, pippenger_empty)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == pippenger(data));
}

TEST(multiexp, bos_coster_zero_and_non_zero)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(TESTPOINT)});
  data.push_back({TESTSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
}

TEST(multiexp, straus_zero_and_non_zero)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(TESTPOINT)});
  data.push_back({TESTSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == straus(data));
}

TEST(multiexp, pippenger_zero_and_non_zero)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({rct::zero(), get_p3(TESTPOINT)});
  data.push_back({TESTSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == pippenger(data));
}

TEST(multiexp, bos_coster_pow2_scalar)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({TESTPOW2SCALAR, get_p3(TESTPOINT)});
  data.push_back({TESTSMALLSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
}

TEST(multiexp, straus_pow2_scalar)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({TESTPOW2SCALAR, get_p3(TESTPOINT)});
  data.push_back({TESTSMALLSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == straus(data));
}

TEST(multiexp, pippenger_pow2_scalar)
{
  std::vector<rct::MultiexpData> data;
  data.push_back({TESTPOW2SCALAR, get_p3(TESTPOINT)});
  data.push_back({TESTSMALLSCALAR, get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == pippenger(data));
}

TEST(multiexp, bos_coster_only_zeroes)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({rct::zero(), get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
}

TEST(multiexp, straus_only_zeroes)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({rct::zero(), get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == straus(data));
}

TEST(multiexp, pippenger_only_zeroes)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({rct::zero(), get_p3(TESTPOINT)});
  ASSERT_TRUE(basic(data) == pippenger(data));
}

TEST(multiexp, bos_coster_only_identities)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({TESTSCALAR, get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
}

TEST(multiexp, straus_only_identities)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({TESTSCALAR, get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == straus(data));
}

TEST(multiexp, pippenger_only_identities)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 16; ++n)
    data.push_back({TESTSCALAR, get_p3(rct::identity())});
  ASSERT_TRUE(basic(data) == pippenger(data));
}

TEST(multiexp, bos_coster_random)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 32; ++n)
  {
    data.push_back({rct::skGen(), get_p3(rct::scalarmultBase(rct::skGen()))});
    ASSERT_TRUE(basic(data) == bos_coster_heap_conv_robust(data));
  }
}

TEST(multiexp, straus_random)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 32; ++n)
  {
    data.push_back({rct::skGen(), get_p3(rct::scalarmultBase(rct::skGen()))});
    ASSERT_TRUE(basic(data) == straus(data));
  }
}

TEST(multiexp, pippenger_random)
{
  std::vector<rct::MultiexpData> data;
  for (int n = 0; n < 32; ++n)
  {
    data.push_back({rct::skGen(), get_p3(rct::scalarmultBase(rct::skGen()))});
    ASSERT_TRUE(basic(data) == pippenger(data));
  }
}

TEST(multiexp, straus_cached)
{
  static constexpr size_t N = 256;
  std::vector<rct::MultiexpData> P(N);
  for (size_t n = 0; n < N; ++n)
  {
    P[n].scalar = rct::zero();
    ASSERT_TRUE(ge_frombytes_vartime(&P[n].point, rct::scalarmultBase(rct::skGen()).bytes) == 0);
  }
  std::shared_ptr<rct::straus_cached_data> cache = rct::straus_init_cache(P);
  for (size_t n = 0; n < N/16; ++n)
  {
    std::vector<rct::MultiexpData> data;
    size_t sz = 1 + crypto::rand<size_t>() % (N-1);
    for (size_t s = 0; s < sz; ++s)
    {
      data.push_back({rct::skGen(), P[s].point});
    }
    ASSERT_TRUE(basic(data) == straus(data, cache));
  }
}

TEST(multiexp, pippenger_cached)
{
  static constexpr size_t N = 256;
  std::vector<rct::MultiexpData> P(N);
  for (size_t n = 0; n < N; ++n)
  {
    P[n].scalar = rct::zero();
    ASSERT_TRUE(ge_frombytes_vartime(&P[n].point, rct::scalarmultBase(rct::skGen()).bytes) == 0);
  }
  std::shared_ptr<rct::pippenger_cached_data> cache = rct::pippenger_init_cache(P);
  for (size_t n = 0; n < N/16; ++n)
  {
    std::vector<rct::MultiexpData> data;
    size_t sz = 1 + crypto::rand<size_t>() % (N-1);
    for (size_t s = 0; s < sz; ++s)
    {
      data.push_back({rct::skGen(), P[s].point});
    }
    ASSERT_TRUE(basic(data) == pippenger(data, cache));
  }
}

TEST(multiexp, scalarmult_triple)
{
  std::vector<rct::MultiexpData> data;
  ge_p2 p2;
  rct::key res;
  ge_p3 Gp3;

  ASSERT_EQ(ge_frombytes_vartime(&Gp3, rct::G.bytes), 0);

  static const rct::key scalars[] = {
    rct::Z,
    rct::I,
    rct::L,
    rct::EIGHT,
    rct::INV_EIGHT,
  };
  static const ge_p3 points[] = {
    ge_p3_identity,
    ge_p3_H,
    Gp3,
  };
  ge_dsmp ppre[sizeof(points) / sizeof(points[0])];

  for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i)
    ge_dsm_precomp(ppre[i], &points[i]);

  data.resize(3);
  for (const rct::key &x: scalars)
  {
    data[0].scalar = x;
    for (const rct::key &y: scalars)
    {
      data[1].scalar = y;
      for (const rct::key &z: scalars)
      {
        data[2].scalar = z;
        for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i)
        {
          data[1].point = points[i];
          for (size_t j = 0; j < sizeof(points) / sizeof(points[0]); ++j)
          {
            data[0].point = Gp3;
            data[2].point = points[j];

            ge_triple_scalarmult_base_vartime(&p2, data[0].scalar.bytes, data[1].scalar.bytes, ppre[i], data[2].scalar.bytes, ppre[j]);
            ge_tobytes(res.bytes, &p2);
            ASSERT_TRUE(basic(data) == res);

            for (size_t k = 0; k < sizeof(points) / sizeof(points[0]); ++k)
            {
              data[0].point = points[k];
              ge_triple_scalarmult_precomp_vartime(&p2, data[0].scalar.bytes, ppre[k], data[1].scalar.bytes, ppre[i], data[2].scalar.bytes, ppre[j]);
              ge_tobytes(res.bytes, &p2);
              ASSERT_TRUE(basic(data) == res);
            }
          }
        }
      }
    }
  }
}
