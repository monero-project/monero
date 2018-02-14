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
// Adapted from Java code by Sarang Noether

#include <stdlib.h>
#include <openssl/ssl.h>
#include <boost/thread/mutex.hpp>
#include "misc_log_ex.h"
#include "common/perf_timer.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "rctOps.h"
#include "bulletproofs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bulletproofs"

//#define DEBUG_BP

#define PERF_TIMER_START_BP(x) PERF_TIMER_START_UNIT(x, 1000000)

namespace rct
{

static rct::key vector_exponent(const rct::keyV &a, const rct::keyV &b);
static rct::keyV vector_powers(rct::key x, size_t n);
static rct::key inner_product(const rct::keyV &a, const rct::keyV &b);

static constexpr size_t maxN = 64;
static rct::key Hi[maxN], Gi[maxN];
static ge_dsmp Gprecomp[64], Hprecomp[64];
static const rct::key TWO = { {0x02, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
static const rct::keyV oneN = vector_powers(rct::identity(), maxN);
static const rct::keyV twoN = vector_powers(TWO, maxN);
static const rct::key ip12 = inner_product(oneN, twoN);
static boost::mutex init_mutex;

static rct::key get_exponent(const rct::key &base, size_t idx)
{
  static const std::string salt("bulletproof");
  std::string hashed = std::string((const char*)base.bytes, sizeof(base)) + salt + tools::get_varint_data(idx);
  return rct::hashToPoint(rct::hash2rct(crypto::cn_fast_hash(hashed.data(), hashed.size())));
}

static void init_exponents()
{
  boost::lock_guard<boost::mutex> lock(init_mutex);

  static bool init_done = false;
  if (init_done)
    return;
  for (size_t i = 0; i < maxN; ++i)
  {
    Hi[i] = get_exponent(rct::H, i * 2);
    rct::precomp(Hprecomp[i], Hi[i]);
    Gi[i] = get_exponent(rct::H, i * 2 + 1);
    rct::precomp(Gprecomp[i], Gi[i]);
  }
  init_done = true;
}

/* Given two scalar arrays, construct a vector commitment */
static rct::key vector_exponent(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN, "Incompatible sizes of a and maxN");
  rct::key res = rct::identity();
  for (size_t i = 0; i < a.size(); ++i)
  {
    rct::key term;
    rct::addKeys3(term, a[i], Gprecomp[i], b[i], Hprecomp[i]);
    rct::addKeys(res, res, term);
  }
  return res;
}

/* Compute a custom vector-scalar commitment */
static rct::key vector_exponent_custom(const rct::keyV &A, const rct::keyV &B, const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(A.size() == B.size(), "Incompatible sizes of A and B");
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  CHECK_AND_ASSERT_THROW_MES(a.size() == A.size(), "Incompatible sizes of a and A");
  CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN, "Incompatible sizes of a and maxN");
  rct::key res = rct::identity();
  for (size_t i = 0; i < a.size(); ++i)
  {
    rct::key term;
#if 0
    // we happen to know where A and B might fall, so don't bother checking the rest
    ge_dsmp *Acache = NULL, *Bcache = NULL;
    ge_dsmp Acache_custom[1], Bcache_custom[1];
    if (Gi[i] == A[i])
      Acache = Gprecomp + i;
    else if (i<32 && Gi[i+32] == A[i])
      Acache = Gprecomp + i + 32;
    else
    {
      rct::precomp(Acache_custom[0], A[i]);
      Acache = Acache_custom;
    }
    if (i == 0 && B[i] == Hi[0])
      Bcache = Hprecomp;
    else
    {
      rct::precomp(Bcache_custom[0], B[i]);
      Bcache = Bcache_custom;
    }
    rct::addKeys3(term, a[i], *Acache, b[i], *Bcache);
#else
    ge_dsmp Acache, Bcache;
    rct::precomp(Bcache, B[i]);
    rct::addKeys3(term, a[i], A[i], b[i], Bcache);
#endif
    rct::addKeys(res, res, term);
  }
  return res;
}

/* Given a scalar, construct a vector of powers */
static rct::keyV vector_powers(rct::key x, size_t n)
{
  rct::keyV res(n);
  if (n == 0)
    return res;
  res[0] = rct::identity();
  if (n == 1)
    return res;
  res[1] = x;
  for (size_t i = 2; i < n; ++i)
  {
    sc_mul(res[i].bytes, res[i-1].bytes, x.bytes);
  }
  return res;
}

/* Given two scalar arrays, construct the inner product */
static rct::key inner_product(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  rct::key res = rct::zero();
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_muladd(res.bytes, a[i].bytes, b[i].bytes, res.bytes);
  }
  return res;
}

/* Given two scalar arrays, construct the Hadamard product */
static rct::keyV hadamard(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_mul(res[i].bytes, a[i].bytes, b[i].bytes);
  }
  return res;
}

/* Given two curvepoint arrays, construct the Hadamard product */
static rct::keyV hadamard2(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    rct::addKeys(res[i], a[i], b[i]);
  }
  return res;
}

/* Add two vectors */
static rct::keyV vector_add(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_add(res[i].bytes, a[i].bytes, b[i].bytes);
  }
  return res;
}

/* Subtract two vectors */
static rct::keyV vector_subtract(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_sub(res[i].bytes, a[i].bytes, b[i].bytes);
  }
  return res;
}

/* Multiply a scalar and a vector */
static rct::keyV vector_scalar(const rct::keyV &a, const rct::key &x)
{
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_mul(res[i].bytes, a[i].bytes, x.bytes);
  }
  return res;
}

/* Exponentiate a curve vector by a scalar */
static rct::keyV vector_scalar2(const rct::keyV &a, const rct::key &x)
{
  rct::keyV res(a.size());
  for (size_t i = 0; i < a.size(); ++i)
  {
    rct::scalarmultKey(res[i], a[i], x);
  }
  return res;
}

static rct::key switch_endianness(rct::key k)
{
  std::reverse(k.bytes, k.bytes + sizeof(k));
  return k;
}

/* Compute the inverse of a scalar, the stupid way */
static rct::key invert(const rct::key &x)
{
  rct::key inv;

  BN_CTX *ctx = BN_CTX_new();
  BIGNUM *X = BN_new();
  BIGNUM *L = BN_new();
  BIGNUM *I = BN_new();

  BN_bin2bn(switch_endianness(x).bytes, sizeof(rct::key), X);
  BN_bin2bn(switch_endianness(rct::curveOrder()).bytes, sizeof(rct::key), L);

  CHECK_AND_ASSERT_THROW_MES(BN_mod_inverse(I, X, L, ctx), "Failed to invert");

  const int len = BN_num_bytes(I);
  CHECK_AND_ASSERT_THROW_MES((size_t)len <= sizeof(rct::key), "Invalid number length");
  inv = rct::zero();
  BN_bn2bin(I, inv.bytes);
  std::reverse(inv.bytes, inv.bytes + len);

  BN_free(I);
  BN_free(L);
  BN_free(X);
  BN_CTX_free(ctx);

#ifdef DEBUG_BP
  rct::key tmp;
  sc_mul(tmp.bytes, inv.bytes, x.bytes);
  CHECK_AND_ASSERT_THROW_MES(tmp == rct::identity(), "invert failed");
#endif
  return inv;
}

/* Compute the slice of a vector */
static rct::keyV slice(const rct::keyV &a, size_t start, size_t stop)
{
  CHECK_AND_ASSERT_THROW_MES(start < a.size(), "Invalid start index");
  CHECK_AND_ASSERT_THROW_MES(stop <= a.size(), "Invalid stop index");
  CHECK_AND_ASSERT_THROW_MES(start < stop, "Invalid start/stop indices");
  rct::keyV res(stop - start);
  for (size_t i = start; i < stop; ++i)
  {
    res[i - start] = a[i];
  }
  return res;
}

static rct::key hash_cache_mash(rct::key &hash_cache, const rct::key &mash0, const rct::key &mash1)
{
  rct::keyV data;
  data.reserve(3);
  data.push_back(hash_cache);
  data.push_back(mash0);
  data.push_back(mash1);
  return hash_cache = rct::hash_to_scalar(data);
}

static rct::key hash_cache_mash(rct::key &hash_cache, const rct::key &mash0, const rct::key &mash1, const rct::key &mash2)
{
  rct::keyV data;
  data.reserve(4);
  data.push_back(hash_cache);
  data.push_back(mash0);
  data.push_back(mash1);
  data.push_back(mash2);
  return hash_cache = rct::hash_to_scalar(data);
}

static rct::key hash_cache_mash(rct::key &hash_cache, const rct::key &mash0, const rct::key &mash1, const rct::key &mash2, const rct::key &mash3)
{
  rct::keyV data;
  data.reserve(5);
  data.push_back(hash_cache);
  data.push_back(mash0);
  data.push_back(mash1);
  data.push_back(mash2);
  data.push_back(mash3);
  return hash_cache = rct::hash_to_scalar(data);
}

/* Given a value v (0..2^N-1) and a mask gamma, construct a range proof */
Bulletproof bulletproof_PROVE(const rct::key &sv, const rct::key &gamma)
{
  init_exponents();

  PERF_TIMER_UNIT(PROVE, 1000000);

  constexpr size_t logN = 6; // log2(64)
  constexpr size_t N = 1<<logN;

  rct::key V;
  rct::keyV aL(N), aR(N);

  PERF_TIMER_START_BP(PROVE_v);
  rct::addKeys2(V, gamma, sv, rct::H);
  PERF_TIMER_STOP(PROVE_v);

  PERF_TIMER_START_BP(PROVE_aLaR);
  for (size_t i = N; i-- > 0; )
  {
    if (sv[i/8] & (((uint64_t)1)<<(i%8)))
    {
      aL[i] = rct::identity();
    }
    else
    {
      aL[i] = rct::zero();
    }
    sc_sub(aR[i].bytes, aL[i].bytes, rct::identity().bytes);
  }
  PERF_TIMER_STOP(PROVE_aLaR);

  rct::key hash_cache = rct::hash_to_scalar(V);

  // DEBUG: Test to ensure this recovers the value
#ifdef DEBUG_BP
  uint64_t test_aL = 0, test_aR = 0;
  for (size_t i = 0; i < N; ++i)
  {
    if (aL[i] == rct::identity())
      test_aL += ((uint64_t)1)<<i;
    if (aR[i] == rct::zero())
      test_aR += ((uint64_t)1)<<i;
  }
  uint64_t v_test = 0;
  for (int n = 0; n < 8; ++n) v_test |= (((uint64_t)sv[n]) << (8*n));
  CHECK_AND_ASSERT_THROW_MES(test_aL == v_test, "test_aL failed");
  CHECK_AND_ASSERT_THROW_MES(test_aR == v_test, "test_aR failed");
#endif

  PERF_TIMER_START_BP(PROVE_step1);
  // PAPER LINES 38-39
  rct::key alpha = rct::skGen();
  rct::key ve = vector_exponent(aL, aR);
  rct::key A;
  rct::addKeys(A, ve, rct::scalarmultBase(alpha));

  // PAPER LINES 40-42
  rct::keyV sL = rct::skvGen(N), sR = rct::skvGen(N);
  rct::key rho = rct::skGen();
  ve = vector_exponent(sL, sR);
  rct::key S;
  rct::addKeys(S, ve, rct::scalarmultBase(rho));

  // PAPER LINES 43-45
  rct::key y = hash_cache_mash(hash_cache, A, S);
  rct::key z = hash_cache = rct::hash_to_scalar(y);

  // Polynomial construction before PAPER LINE 46
  rct::key t0 = rct::zero();
  rct::key t1 = rct::zero();
  rct::key t2 = rct::zero();

  const auto yN = vector_powers(y, N);

  rct::key ip1y = inner_product(oneN, yN);
  rct::key tmp;
  sc_muladd(t0.bytes, z.bytes, ip1y.bytes, t0.bytes);

  rct::key zsq;
  sc_mul(zsq.bytes, z.bytes, z.bytes);
  sc_muladd(t0.bytes, zsq.bytes, sv.bytes, t0.bytes);

  rct::key k = rct::zero();
  sc_mulsub(k.bytes, zsq.bytes, ip1y.bytes, k.bytes);

  rct::key zcu;
  sc_mul(zcu.bytes, zsq.bytes, z.bytes);
  sc_mulsub(k.bytes, zcu.bytes, ip12.bytes, k.bytes);
  sc_add(t0.bytes, t0.bytes, k.bytes);

  // DEBUG: Test the value of t0 has the correct form
#ifdef DEBUG_BP
  rct::key test_t0 = rct::zero();
  rct::key iph = inner_product(aL, hadamard(aR, yN));
  sc_add(test_t0.bytes, test_t0.bytes, iph.bytes);
  rct::key ips = inner_product(vector_subtract(aL, aR), yN);
  sc_muladd(test_t0.bytes, z.bytes, ips.bytes, test_t0.bytes);
  rct::key ipt = inner_product(twoN, aL);
  sc_muladd(test_t0.bytes, zsq.bytes, ipt.bytes, test_t0.bytes);
  sc_add(test_t0.bytes, test_t0.bytes, k.bytes);
  CHECK_AND_ASSERT_THROW_MES(t0 == test_t0, "t0 check failed");
#endif
  PERF_TIMER_STOP(PROVE_step1);

  PERF_TIMER_START_BP(PROVE_step2);
  const auto HyNsR = hadamard(yN, sR);
  const auto vpIz = vector_scalar(oneN, z);
  const auto vp2zsq = vector_scalar(twoN, zsq);
  const auto aL_vpIz = vector_subtract(aL, vpIz);
  const auto aR_vpIz = vector_add(aR, vpIz);

  rct::key ip1 = inner_product(aL_vpIz, HyNsR);
  sc_add(t1.bytes, t1.bytes, ip1.bytes);

  rct::key ip2 = inner_product(sL, vector_add(hadamard(yN, aR_vpIz), vp2zsq));
  sc_add(t1.bytes, t1.bytes, ip2.bytes);

  rct::key ip3 = inner_product(sL, HyNsR);
  sc_add(t2.bytes, t2.bytes, ip3.bytes);

  // PAPER LINES 47-48
  rct::key tau1 = rct::skGen(), tau2 = rct::skGen();

  rct::key T1 = rct::addKeys(rct::scalarmultKey(rct::H, t1), rct::scalarmultBase(tau1));
  rct::key T2 = rct::addKeys(rct::scalarmultKey(rct::H, t2), rct::scalarmultBase(tau2));

  // PAPER LINES 49-51
  rct::key x = hash_cache_mash(hash_cache, z, T1, T2);

  // PAPER LINES 52-53
  rct::key taux = rct::zero();
  sc_mul(taux.bytes, tau1.bytes, x.bytes);
  rct::key xsq;
  sc_mul(xsq.bytes, x.bytes, x.bytes);
  sc_muladd(taux.bytes, tau2.bytes, xsq.bytes, taux.bytes);
  sc_muladd(taux.bytes, gamma.bytes, zsq.bytes, taux.bytes);
  rct::key mu;
  sc_muladd(mu.bytes, x.bytes, rho.bytes, alpha.bytes);

  // PAPER LINES 54-57
  rct::keyV l = vector_add(aL_vpIz, vector_scalar(sL, x));
  rct::keyV r = vector_add(hadamard(yN, vector_add(aR_vpIz, vector_scalar(sR, x))), vp2zsq);
  PERF_TIMER_STOP(PROVE_step2);

  PERF_TIMER_START_BP(PROVE_step3);
  rct::key t = inner_product(l, r);

  // DEBUG: Test if the l and r vectors match the polynomial forms
#ifdef DEBUG_BP
  rct::key test_t;
  sc_muladd(test_t.bytes, t1.bytes, x.bytes, t0.bytes);
  sc_muladd(test_t.bytes, t2.bytes, xsq.bytes, test_t.bytes);
  CHECK_AND_ASSERT_THROW_MES(test_t == t, "test_t check failed");
#endif

  // PAPER LINES 32-33
  rct::key x_ip = hash_cache_mash(hash_cache, x, taux, mu, t);

  // These are used in the inner product rounds
  size_t nprime = N;
  rct::keyV Gprime(N);
  rct::keyV Hprime(N);
  rct::keyV aprime(N);
  rct::keyV bprime(N);
  const rct::key yinv = invert(y);
  rct::key yinvpow = rct::identity();
  for (size_t i = 0; i < N; ++i)
  {
    Gprime[i] = Gi[i];
    Hprime[i] = scalarmultKey(Hi[i], yinvpow);
    sc_mul(yinvpow.bytes, yinvpow.bytes, yinv.bytes);
    aprime[i] = l[i];
    bprime[i] = r[i];
  }
  rct::keyV L(logN);
  rct::keyV R(logN);
  int round = 0;
  rct::keyV w(logN); // this is the challenge x in the inner product protocol
  PERF_TIMER_STOP(PROVE_step3);

  PERF_TIMER_START_BP(PROVE_step4);
  // PAPER LINE 13
  while (nprime > 1)
  {
    // PAPER LINE 15
    nprime /= 2;

    // PAPER LINES 16-17
    rct::key cL = inner_product(slice(aprime, 0, nprime), slice(bprime, nprime, bprime.size()));
    rct::key cR = inner_product(slice(aprime, nprime, aprime.size()), slice(bprime, 0, nprime));

    // PAPER LINES 18-19
    L[round] = vector_exponent_custom(slice(Gprime, nprime, Gprime.size()), slice(Hprime, 0, nprime), slice(aprime, 0, nprime), slice(bprime, nprime, bprime.size()));
    sc_mul(tmp.bytes, cL.bytes, x_ip.bytes);
    rct::addKeys(L[round], L[round], rct::scalarmultKey(rct::H, tmp));
    R[round] = vector_exponent_custom(slice(Gprime, 0, nprime), slice(Hprime, nprime, Hprime.size()), slice(aprime, nprime, aprime.size()), slice(bprime, 0, nprime));
    sc_mul(tmp.bytes, cR.bytes, x_ip.bytes);
    rct::addKeys(R[round], R[round], rct::scalarmultKey(rct::H, tmp));

    // PAPER LINES 21-22
    w[round] = hash_cache_mash(hash_cache, L[round], R[round]);

    // PAPER LINES 24-25
    const rct::key winv = invert(w[round]);
    Gprime = hadamard2(vector_scalar2(slice(Gprime, 0, nprime), winv), vector_scalar2(slice(Gprime, nprime, Gprime.size()), w[round]));
    Hprime = hadamard2(vector_scalar2(slice(Hprime, 0, nprime), w[round]), vector_scalar2(slice(Hprime, nprime, Hprime.size()), winv));

    // PAPER LINES 28-29
    aprime = vector_add(vector_scalar(slice(aprime, 0, nprime), w[round]), vector_scalar(slice(aprime, nprime, aprime.size()), winv));
    bprime = vector_add(vector_scalar(slice(bprime, 0, nprime), winv), vector_scalar(slice(bprime, nprime, bprime.size()), w[round]));

    ++round;
  }
  PERF_TIMER_STOP(PROVE_step4);

  // PAPER LINE 58 (with inclusions from PAPER LINE 8 and PAPER LINE 20)
  return Bulletproof(V, A, S, T1, T2, taux, mu, L, R, aprime[0], bprime[0], t);
}

Bulletproof bulletproof_PROVE(uint64_t v, const rct::key &gamma)
{
  // vG + gammaH
  PERF_TIMER_START_BP(PROVE_v);
  rct::key sv = rct::zero();
  sv.bytes[0] = v & 255;
  sv.bytes[1] = (v >> 8) & 255;
  sv.bytes[2] = (v >> 16) & 255;
  sv.bytes[3] = (v >> 24) & 255;
  sv.bytes[4] = (v >> 32) & 255;
  sv.bytes[5] = (v >> 40) & 255;
  sv.bytes[6] = (v >> 48) & 255;
  sv.bytes[7] = (v >> 56) & 255;
  PERF_TIMER_STOP(PROVE_v);
  return bulletproof_PROVE(sv, gamma);
}

/* Given a range proof, determine if it is valid */
bool bulletproof_VERIFY(const Bulletproof &proof)
{
  init_exponents();

  CHECK_AND_ASSERT_MES(proof.V.size() == 1, false, "V does not have exactly one element");
  CHECK_AND_ASSERT_MES(proof.L.size() == proof.R.size(), false, "Mismatched L and R sizes");
  CHECK_AND_ASSERT_MES(proof.L.size() > 0, false, "Empty proof");
  CHECK_AND_ASSERT_MES(proof.L.size() == 6, false, "Proof is not for 64 bits");

  const size_t logN = proof.L.size();
  const size_t N = 1 << logN;

  // Reconstruct the challenges
  PERF_TIMER_START_BP(VERIFY);
  PERF_TIMER_START_BP(VERIFY_start);
  rct::key hash_cache = rct::hash_to_scalar(proof.V[0]);
  rct::key y = hash_cache_mash(hash_cache, proof.A, proof.S);
  rct::key z = hash_cache = rct::hash_to_scalar(y);
  rct::key x = hash_cache_mash(hash_cache, z, proof.T1, proof.T2);
  PERF_TIMER_STOP(VERIFY_start);

  PERF_TIMER_START_BP(VERIFY_line_60);
  // Reconstruct the challenges
  rct::key x_ip = hash_cache_mash(hash_cache, x, proof.taux, proof.mu, proof.t);
  PERF_TIMER_STOP(VERIFY_line_60);

  PERF_TIMER_START_BP(VERIFY_line_61);
  // PAPER LINE 61
  rct::key L61Left = rct::addKeys(rct::scalarmultBase(proof.taux), rct::scalarmultKey(rct::H, proof.t));

  rct::key k = rct::zero();
  const auto yN = vector_powers(y, N);
  rct::key ip1y = inner_product(oneN, yN);
  rct::key zsq;
  sc_mul(zsq.bytes, z.bytes, z.bytes);
  rct::key tmp, tmp2;
  sc_mulsub(k.bytes, zsq.bytes, ip1y.bytes, k.bytes);
  rct::key zcu;
  sc_mul(zcu.bytes, zsq.bytes, z.bytes);
  sc_mulsub(k.bytes, zcu.bytes, ip12.bytes, k.bytes);
  PERF_TIMER_STOP(VERIFY_line_61);

  PERF_TIMER_START_BP(VERIFY_line_61rl);
  sc_muladd(tmp.bytes, z.bytes, ip1y.bytes, k.bytes);
  rct::key L61Right = rct::scalarmultKey(rct::H, tmp);

  CHECK_AND_ASSERT_MES(proof.V.size() == 1, false, "proof.V does not have exactly one element");
  tmp = rct::scalarmultKey(proof.V[0], zsq);
  rct::addKeys(L61Right, L61Right, tmp);

  tmp = rct::scalarmultKey(proof.T1, x);
  rct::addKeys(L61Right, L61Right, tmp);

  rct::key xsq;
  sc_mul(xsq.bytes, x.bytes, x.bytes);
  tmp = rct::scalarmultKey(proof.T2, xsq);
  rct::addKeys(L61Right, L61Right, tmp);
  PERF_TIMER_STOP(VERIFY_line_61rl);

  if (!(L61Right == L61Left))
  {
    MERROR("Verification failure at step 1");
    return false;
  }

  PERF_TIMER_START_BP(VERIFY_line_62);
  // PAPER LINE 62
  rct::key P = rct::addKeys(proof.A, rct::scalarmultKey(proof.S, x));
  PERF_TIMER_STOP(VERIFY_line_62);

  // Compute the number of rounds for the inner product
  const size_t rounds = proof.L.size();
  CHECK_AND_ASSERT_MES(rounds > 0, false, "Zero rounds");

  PERF_TIMER_START_BP(VERIFY_line_21_22);
  // PAPER LINES 21-22
  // The inner product challenges are computed per round
  rct::keyV w(rounds);
  for (size_t i = 0; i < rounds; ++i)
  {
    w[i] = hash_cache_mash(hash_cache, proof.L[i], proof.R[i]);
  }
  PERF_TIMER_STOP(VERIFY_line_21_22);

  PERF_TIMER_START_BP(VERIFY_line_24_25);
  // Basically PAPER LINES 24-25
  // Compute the curvepoints from G[i] and H[i]
  rct::key inner_prod = rct::identity();
  rct::key yinvpow = rct::identity();
  rct::key ypow = rct::identity();

  PERF_TIMER_START_BP(VERIFY_line_24_25_invert);
  const rct::key yinv = invert(y);
  rct::keyV winv(rounds);
  for (size_t i = 0; i < rounds; ++i)
    winv[i] = invert(w[i]);
  PERF_TIMER_STOP(VERIFY_line_24_25_invert);

  for (size_t i = 0; i < N; ++i)
  {
    // Convert the index to binary IN REVERSE and construct the scalar exponent
    rct::key g_scalar = proof.a;
    rct::key h_scalar;
    sc_mul(h_scalar.bytes, proof.b.bytes, yinvpow.bytes);

    for (size_t j = rounds; j-- > 0; )
    {
      size_t J = w.size() - j - 1;

      if ((i & (((size_t)1)<<j)) == 0)
      {
        sc_mul(g_scalar.bytes, g_scalar.bytes, winv[J].bytes);
        sc_mul(h_scalar.bytes, h_scalar.bytes, w[J].bytes);
      }
      else
      {
        sc_mul(g_scalar.bytes, g_scalar.bytes, w[J].bytes);
        sc_mul(h_scalar.bytes, h_scalar.bytes, winv[J].bytes);
      }
    }

    // Adjust the scalars using the exponents from PAPER LINE 62
    sc_add(g_scalar.bytes, g_scalar.bytes, z.bytes);
    sc_mul(tmp.bytes, zsq.bytes, twoN[i].bytes);
    sc_muladd(tmp.bytes, z.bytes, ypow.bytes, tmp.bytes);
    sc_mulsub(h_scalar.bytes, tmp.bytes, yinvpow.bytes, h_scalar.bytes);

    // Now compute the basepoint's scalar multiplication
    // Each of these could be written as a multiexp operation instead
    rct::addKeys3(tmp, g_scalar, Gprecomp[i], h_scalar, Hprecomp[i]);
    rct::addKeys(inner_prod, inner_prod, tmp);

    if (i != N-1)
    {
      sc_mul(yinvpow.bytes, yinvpow.bytes, yinv.bytes);
      sc_mul(ypow.bytes, ypow.bytes, y.bytes);
    }
  }
  PERF_TIMER_STOP(VERIFY_line_24_25);

  PERF_TIMER_START_BP(VERIFY_line_26);
  // PAPER LINE 26
  rct::key pprime;
  sc_sub(tmp.bytes, rct::zero().bytes, proof.mu.bytes);
  rct::addKeys(pprime, P, rct::scalarmultBase(tmp));

  for (size_t i = 0; i < rounds; ++i)
  {
    sc_mul(tmp.bytes, w[i].bytes, w[i].bytes);
    sc_mul(tmp2.bytes, winv[i].bytes, winv[i].bytes);
#if 1
    ge_dsmp cacheL, cacheR;
    rct::precomp(cacheL, proof.L[i]);
    rct::precomp(cacheR, proof.R[i]);
    rct::addKeys3(tmp, tmp, cacheL, tmp2, cacheR);
    rct::addKeys(pprime, pprime, tmp);
#else
    rct::addKeys(pprime, pprime, rct::scalarmultKey(proof.L[i], tmp));
    rct::addKeys(pprime, pprime, rct::scalarmultKey(proof.R[i], tmp2));
#endif
  }
  sc_mul(tmp.bytes, proof.t.bytes, x_ip.bytes);
  rct::addKeys(pprime, pprime, rct::scalarmultKey(rct::H, tmp));
  PERF_TIMER_STOP(VERIFY_line_26);

  PERF_TIMER_START_BP(VERIFY_step2_check);
  sc_mul(tmp.bytes, proof.a.bytes, proof.b.bytes);
  sc_mul(tmp.bytes, tmp.bytes, x_ip.bytes);
  tmp = rct::scalarmultKey(rct::H, tmp);
  rct::addKeys(tmp, tmp, inner_prod);
  PERF_TIMER_STOP(VERIFY_step2_check);
  if (!(pprime == tmp))
  {
    MERROR("Verification failure at step 2");
    return false;
  }

  PERF_TIMER_STOP(VERIFY);
  return true;
}

}
