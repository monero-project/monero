// Copyright (c) 2017-2018, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include "string_tools.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "ringct/bulletproofs.h"
#include "device/device.hpp"
#include "misc_log_ex.h"

TEST(bulletproofs, valid_zero)
{
  rct::Bulletproof proof = bulletproof_PROVE(0, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_max)
{
  rct::Bulletproof proof = bulletproof_PROVE(0xffffffffffffffff, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_random)
{
  for (int n = 0; n < 8; ++n)
  {
    rct::Bulletproof proof = bulletproof_PROVE(crypto::rand<uint64_t>(), rct::skGen());
    ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  }
}

TEST(bulletproofs, valid_multi_random)
{
  for (int n = 0; n < 8; ++n)
  {
    size_t outputs = 2 + n;
    std::vector<uint64_t> amounts;
    rct::keyV gamma;
    for (size_t i = 0; i < outputs; ++i)
    {
      amounts.push_back(crypto::rand<uint64_t>());
      gamma.push_back(rct::skGen());
    }
    rct::Bulletproof proof = bulletproof_PROVE(amounts, gamma);
    ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  }
}

TEST(bulletproofs, multi_splitting)
{
  rct::ctkeyV sc, pc;
  rct::ctkey sctmp, pctmp;
  std::vector<unsigned int> index;
  std::vector<uint64_t> inamounts, outamounts;

  std::tie(sctmp, pctmp) = rct::ctskpkGen(6000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  inamounts.push_back(6000);
  index.push_back(1);

  std::tie(sctmp, pctmp) = rct::ctskpkGen(7000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  inamounts.push_back(7000);
  index.push_back(1);

  const int mixin = 3, max_outputs = 16;

  for (int n_outputs = 1; n_outputs <= max_outputs; ++n_outputs)
  {
    std::vector<uint64_t> outamounts;
    rct::keyV amount_keys;
    rct::keyV destinations;
    rct::key Sk, Pk;
    uint64_t available = 6000 + 7000;
    uint64_t amount;
    rct::ctkeyM mixRing(sc.size());

    //add output
    for (size_t i = 0; i < n_outputs; ++i)
    {
      amount = rct::randXmrAmount(available);
      outamounts.push_back(amount);
      amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
      rct::skpkGen(Sk, Pk);
      destinations.push_back(Pk);
      available -= amount;
    }

    for (size_t i = 0; i < sc.size(); ++i)
    {
      for (size_t j = 0; j <= mixin; ++j)
      {
        if (j == 1)
          mixRing[i].push_back(pc[i]);
        else
          mixRing[i].push_back({rct::scalarmultBase(rct::skGen()), rct::scalarmultBase(rct::skGen())});
      }
    }

    rct::ctkeyV outSk;
    rct::rctSig s = rct::genRctSimple(rct::zero(), sc, destinations, inamounts, outamounts, available, mixRing, amount_keys, NULL, NULL, index, outSk, rct::RangeProofMultiOutputBulletproof, hw::get_device("default"));
    ASSERT_TRUE(rct::verRctSimple(s));
    for (size_t i = 0; i < n_outputs; ++i)
    {
      rct::key mask;
      rct::decodeRctSimple(s, amount_keys[i], i, mask, hw::get_device("default"));
      ASSERT_TRUE(mask == outSk[i].mask);
    }
  }
}

TEST(bulletproofs, valid_aggregated)
{
  static const size_t N_PROOFS = 8;
  std::vector<rct::Bulletproof> proofs(N_PROOFS);
  for (size_t n = 0; n < N_PROOFS; ++n)
  {
    size_t outputs = 2 + n;
    std::vector<uint64_t> amounts;
    rct::keyV gamma;
    for (size_t i = 0; i < outputs; ++i)
    {
      amounts.push_back(crypto::rand<uint64_t>());
      gamma.push_back(rct::skGen());
    }
    proofs[n] = bulletproof_PROVE(amounts, gamma);
  }
  ASSERT_TRUE(rct::bulletproof_VERIFY(proofs));
}


TEST(bulletproofs, invalid_8)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[8] = 1;
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, rct::skGen());
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, invalid_31)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[31] = 1;
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, rct::skGen());
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, invalid_gamma_0)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[8] = 1;
  rct::key gamma = rct::zero();
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, gamma);
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, invalid_gamma_ff)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[8] = 1;
  rct::key gamma = rct::zero();
  memset(&gamma, 0xff, sizeof(gamma));
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, gamma);
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

static const char * const torsion_elements[] =
{
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa",
  "0000000000000000000000000000000000000000000000000000000000000000",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85",
  "ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05",
  "0000000000000000000000000000000000000000000000000000000000000080",
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a",
};

TEST(bulletproofs, invalid_torsion)
{
  rct::Bulletproof proof = bulletproof_PROVE(7329838943733, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  for (const auto &xs: torsion_elements)
  {
    rct::key x;
    ASSERT_TRUE(epee::string_tools::hex_to_pod(xs, x));
    ASSERT_FALSE(rct::isInMainSubgroup(x));
    for (auto &k: proof.V)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    for (auto &k: proof.L)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    for (auto &k: proof.R)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    const rct::key org_A = proof.A;
    rct::addKeys(proof.A, org_A, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.A = org_A;
    const rct::key org_S = proof.S;
    rct::addKeys(proof.S, org_S, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.S = org_S;
    const rct::key org_T1 = proof.T1;
    rct::addKeys(proof.T1, org_T1, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.T1 = org_T1;
    const rct::key org_T2 = proof.T2;
    rct::addKeys(proof.T2, org_T2, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.T2 = org_T2;
  }
}
