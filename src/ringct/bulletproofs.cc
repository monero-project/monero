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
#include <openssl/bn.h>
#include <boost/thread/mutex.hpp>
#include "misc_log_ex.h"
#include "common/perf_timer.h"
#include "cryptonote_config.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "rctOps.h"
#include "multiexp.h"
#include "bulletproofs.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bulletproofs"

//#define DEBUG_BP

#define PERF_TIMER_START_BP(x) PERF_TIMER_START_UNIT(x, 1000000)

#define STRAUS_SIZE_LIMIT 128
#define PIPPENGER_SIZE_LIMIT 0

namespace rct
{

static rct::key vector_exponent(const rct::keyV &a, const rct::keyV &b);
static rct::keyV vector_powers(const rct::key &x, size_t n);
static rct::keyV vector_dup(const rct::key &x, size_t n);
static rct::key inner_product(const rct::keyV &a, const rct::keyV &b);

static constexpr size_t maxN = 64;
static constexpr size_t maxM = BULLETPROOF_MAX_OUTPUTS;
static rct::key Hi[maxN*maxM], Gi[maxN*maxM];
static ge_p3 Hi_p3[maxN*maxM], Gi_p3[maxN*maxM];
static std::shared_ptr<straus_cached_data> straus_HiGi_cache;
static std::shared_ptr<pippenger_cached_data> pippenger_HiGi_cache;
static const rct::key TWO = { {0x02, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00 , 0x00, 0x00, 0x00,0x00  } };
static const rct::key MINUS_ONE = { { 0xec, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 } };
static const rct::key MINUS_INV_EIGHT = { { 0x74, 0xa4, 0x19, 0x7a, 0xf0, 0x7d, 0x0b, 0xf7, 0x05, 0xc2, 0xda, 0x25, 0x2b, 0x5c, 0x0b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a } };
static const rct::keyV oneN = vector_dup(rct::identity(), maxN);
static const rct::keyV twoN = vector_powers(TWO, maxN);
static const rct::key ip12 = inner_product(oneN, twoN);
static boost::mutex init_mutex;

static inline rct::key multiexp(const std::vector<MultiexpData> &data, bool HiGi)
{
  if (HiGi)
  {
    static_assert(128 <= STRAUS_SIZE_LIMIT, "Straus in precalc mode can only be calculated till STRAUS_SIZE_LIMIT");
    return data.size() <= 128 ? straus(data, straus_HiGi_cache, 0) : pippenger(data, pippenger_HiGi_cache, get_pippenger_c(data.size()));
  }
  else
    return data.size() <= 64 ? straus(data, NULL, 0) : pippenger(data, NULL, get_pippenger_c(data.size()));
}

static bool is_reduced(const rct::key &scalar)
{
  rct::key reduced = scalar;
  sc_reduce32(reduced.bytes);
  return scalar == reduced;
}

static void addKeys_acc_p3(ge_p3 *acc_p3, const rct::key &a, const rct::key &point)
{
    ge_p3 p3;
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&p3, point.bytes) == 0, "ge_frombytes_vartime failed");
    ge_scalarmult_p3(&p3, a.bytes, &p3);
    ge_cached cached;
    ge_p3_to_cached(&cached, acc_p3);
    ge_p1p1 p1;
    ge_add(&p1, &p3, &cached);
    ge_p1p1_to_p3(acc_p3, &p1);
}

static void add_acc_p3(ge_p3 *acc_p3, const rct::key &point)
{
    ge_p3 p3;
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&p3, point.bytes) == 0, "ge_frombytes_vartime failed");
    ge_cached cached;
    ge_p3_to_cached(&cached, &p3);
    ge_p1p1 p1;
    ge_add(&p1, acc_p3, &cached);
    ge_p1p1_to_p3(acc_p3, &p1);
}

static void sub_acc_p3(ge_p3 *acc_p3, const rct::key &point)
{
    ge_p3 p3;
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&p3, point.bytes) == 0, "ge_frombytes_vartime failed");
    ge_cached cached;
    ge_p3_to_cached(&cached, &p3);
    ge_p1p1 p1;
    ge_sub(&p1, acc_p3, &cached);
    ge_p1p1_to_p3(acc_p3, &p1);
}

static rct::key scalarmultKey(const ge_p3 &P, const rct::key &a)
{
  ge_p2 R;
  ge_scalarmult(&R, a.bytes, &P);
  rct::key aP;
  ge_tobytes(aP.bytes, &R);
  return aP;
}

static rct::key get_exponent(const rct::key &base, size_t idx)
{
  static const std::string salt("bulletproof");
  std::string hashed = std::string((const char*)base.bytes, sizeof(base)) + salt + tools::get_varint_data(idx);
  const rct::key e = rct::hashToPoint(rct::hash2rct(crypto::cn_fast_hash(hashed.data(), hashed.size())));
  CHECK_AND_ASSERT_THROW_MES(!(e == rct::identity()), "Exponent is point at infinity");
  return e;
}

static void init_exponents()
{
  boost::lock_guard<boost::mutex> lock(init_mutex);

  static bool init_done = false;
  if (init_done)
    return;
  std::vector<MultiexpData> data;
  for (size_t i = 0; i < maxN*maxM; ++i)
  {
    Hi[i] = get_exponent(rct::H, i * 2);
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&Hi_p3[i], Hi[i].bytes) == 0, "ge_frombytes_vartime failed");
    Gi[i] = get_exponent(rct::H, i * 2 + 1);
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&Gi_p3[i], Gi[i].bytes) == 0, "ge_frombytes_vartime failed");

    data.push_back({rct::zero(), Gi[i]});
    data.push_back({rct::zero(), Hi[i]});
  }

  straus_HiGi_cache = straus_init_cache(data, STRAUS_SIZE_LIMIT);
  pippenger_HiGi_cache = pippenger_init_cache(data, PIPPENGER_SIZE_LIMIT);

  MINFO("Hi/Gi cache size: " << (sizeof(Hi)+sizeof(Gi))/1024 << " kB");
  MINFO("Hi_p3/Gi_p3 cache size: " << (sizeof(Hi_p3)+sizeof(Gi_p3))/1024 << " kB");
  MINFO("Straus cache size: " << straus_get_cache_size(straus_HiGi_cache)/1024 << " kB");
  MINFO("Pippenger cache size: " << pippenger_get_cache_size(pippenger_HiGi_cache)/1024 << " kB");
  size_t cache_size = (sizeof(Hi)+sizeof(Hi_p3))*2 + straus_get_cache_size(straus_HiGi_cache) + pippenger_get_cache_size(pippenger_HiGi_cache);
  MINFO("Total cache size: " << cache_size/1024 << "kB");
  init_done = true;
}

/* Given two scalar arrays, construct a vector commitment */
static rct::key vector_exponent(const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN*maxM, "Incompatible sizes of a and maxN");

  std::vector<MultiexpData> multiexp_data;
  multiexp_data.reserve(a.size()*2);
  for (size_t i = 0; i < a.size(); ++i)
  {
    multiexp_data.emplace_back(a[i], Gi_p3[i]);
    multiexp_data.emplace_back(b[i], Hi_p3[i]);
  }
  return multiexp(multiexp_data, true);
}

/* Compute a custom vector-scalar commitment */
static rct::key vector_exponent_custom(const rct::keyV &A, const rct::keyV &B, const rct::keyV &a, const rct::keyV &b)
{
  CHECK_AND_ASSERT_THROW_MES(A.size() == B.size(), "Incompatible sizes of A and B");
  CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
  CHECK_AND_ASSERT_THROW_MES(a.size() == A.size(), "Incompatible sizes of a and A");
  CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN*maxM, "Incompatible sizes of a and maxN");

  std::vector<MultiexpData> multiexp_data;
  multiexp_data.reserve(a.size()*2);
  for (size_t i = 0; i < a.size(); ++i)
  {
    multiexp_data.resize(multiexp_data.size() + 1);
    multiexp_data.back().scalar = a[i];
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&multiexp_data.back().point, A[i].bytes) == 0, "ge_frombytes_vartime failed");
    multiexp_data.resize(multiexp_data.size() + 1);
    multiexp_data.back().scalar = b[i];
    CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&multiexp_data.back().point, B[i].bytes) == 0, "ge_frombytes_vartime failed");
  }
  return multiexp(multiexp_data, false);
}

/* Given a scalar, construct a vector of powers */
static rct::keyV vector_powers(const rct::key &x, size_t n)
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

/* Given a scalar, return the sum of its powers from 0 to n-1 */
static rct::key vector_power_sum(const rct::key &x, size_t n)
{
  if (n == 0)
    return rct::zero();
  rct::key res = rct::identity();
  if (n == 1)
    return res;
  rct::key prev = x;
  for (size_t i = 1; i < n; ++i)
  {
    if (i > 1)
      sc_mul(prev.bytes, prev.bytes, x.bytes);
    sc_add(res.bytes, res.bytes, prev.bytes);
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

/* Create a vector from copies of a single value */
static rct::keyV vector_dup(const rct::key &x, size_t N)
{
  return rct::keyV(N, x);
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

/* Get the sum of a vector's elements */
static rct::key vector_sum(const rct::keyV &a)
{
  rct::key res = rct::zero();
  for (size_t i = 0; i < a.size(); ++i)
  {
    sc_add(res.bytes, res.bytes, a[i].bytes);
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
  V = rct::scalarmultKey(V, INV_EIGHT);
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

try_again:
  PERF_TIMER_START_BP(PROVE_step1);
  // PAPER LINES 38-39
  rct::key alpha = rct::skGen();
  rct::key ve = vector_exponent(aL, aR);
  rct::key A;
  rct::addKeys(A, ve, rct::scalarmultBase(alpha));
  A = rct::scalarmultKey(A, INV_EIGHT);

  // PAPER LINES 40-42
  rct::keyV sL = rct::skvGen(N), sR = rct::skvGen(N);
  rct::key rho = rct::skGen();
  ve = vector_exponent(sL, sR);
  rct::key S;
  rct::addKeys(S, ve, rct::scalarmultBase(rho));
  S = rct::scalarmultKey(S, INV_EIGHT);

  // PAPER LINES 43-45
  rct::key y = hash_cache_mash(hash_cache, A, S);
  if (y == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step1);
    MINFO("y is 0, trying again");
    goto try_again;
  }
  rct::key z = hash_cache = rct::hash_to_scalar(y);
  if (z == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step1);
    MINFO("z is 0, trying again");
    goto try_again;
  }

  // Polynomial construction before PAPER LINE 46
  rct::key t0 = rct::zero();
  rct::key t1 = rct::zero();
  rct::key t2 = rct::zero();

  const auto yN = vector_powers(y, N);

  rct::key ip1y = vector_sum(yN);
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
  const auto vpIz = vector_dup(z, N);
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

  rct::key T1 = rct::addKeys(rct::scalarmultH(t1), rct::scalarmultBase(tau1));
  T1 = rct::scalarmultKey(T1, INV_EIGHT);
  rct::key T2 = rct::addKeys(rct::scalarmultH(t2), rct::scalarmultBase(tau2));
  T2 = rct::scalarmultKey(T2, INV_EIGHT);

  // PAPER LINES 49-51
  rct::key x = hash_cache_mash(hash_cache, z, T1, T2);
  if (x == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step2);
    MINFO("x is 0, trying again");
    goto try_again;
  }

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
    Hprime[i] = scalarmultKey(Hi_p3[i], yinvpow);
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
    rct::addKeys(L[round], L[round], rct::scalarmultH(tmp));
    L[round] = rct::scalarmultKey(L[round], INV_EIGHT);
    R[round] = vector_exponent_custom(slice(Gprime, 0, nprime), slice(Hprime, nprime, Hprime.size()), slice(aprime, nprime, aprime.size()), slice(bprime, 0, nprime));
    sc_mul(tmp.bytes, cR.bytes, x_ip.bytes);
    rct::addKeys(R[round], R[round], rct::scalarmultH(tmp));
    R[round] = rct::scalarmultKey(R[round], INV_EIGHT);

    // PAPER LINES 21-22
    w[round] = hash_cache_mash(hash_cache, L[round], R[round]);
    if (w[round] == rct::zero())
    {
      PERF_TIMER_STOP(PROVE_step4);
      MINFO("w[round] is 0, trying again");
      goto try_again;
    }

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

/* Given a set of values v (0..2^N-1) and masks gamma, construct a range proof */
Bulletproof bulletproof_PROVE(const rct::keyV &sv, const rct::keyV &gamma)
{
  CHECK_AND_ASSERT_THROW_MES(sv.size() == gamma.size(), "Incompatible sizes of sv and gamma");
  CHECK_AND_ASSERT_THROW_MES(!sv.empty(), "sv is empty");
  for (const rct::key &sve: sv)
    CHECK_AND_ASSERT_THROW_MES(is_reduced(sve), "Invalid sv input");
  for (const rct::key &g: gamma)
    CHECK_AND_ASSERT_THROW_MES(is_reduced(g), "Invalid gamma input");

  init_exponents();

  PERF_TIMER_UNIT(PROVE, 1000000);

  constexpr size_t logN = 6; // log2(64)
  constexpr size_t N = 1<<logN;
  size_t M, logM;
  for (logM = 0; (M = 1<<logM) <= maxM && M < sv.size(); ++logM);
  CHECK_AND_ASSERT_THROW_MES(M <= maxM, "sv/gamma are too large");
  const size_t logMN = logM + logN;
  const size_t MN = M * N;

  rct::keyV V(sv.size());
  rct::keyV aL(MN), aR(MN);
  rct::key tmp;

  PERF_TIMER_START_BP(PROVE_v);
  for (size_t i = 0; i < sv.size(); ++i)
  {
    rct::addKeys2(V[i], gamma[i], sv[i], rct::H);
    V[i] = rct::scalarmultKey(V[i], INV_EIGHT);
  }
  PERF_TIMER_STOP(PROVE_v);

  PERF_TIMER_START_BP(PROVE_aLaR);
  for (size_t j = 0; j < M; ++j)
  {
    for (size_t i = N; i-- > 0; )
    {
      if (j >= sv.size())
      {
        aL[j*N+i] = rct::zero();
      }
      else if (sv[j][i/8] & (((uint64_t)1)<<(i%8)))
      {
        aL[j*N+i] = rct::identity();
      }
      else
      {
        aL[j*N+i] = rct::zero();
      }
      sc_sub(aR[j*N+i].bytes, aL[j*N+i].bytes, rct::identity().bytes);
    }
  }
  PERF_TIMER_STOP(PROVE_aLaR);

  // DEBUG: Test to ensure this recovers the value
#ifdef DEBUG_BP
  for (size_t j = 0; j < M; ++j)
  {
    uint64_t test_aL = 0, test_aR = 0;
    for (size_t i = 0; i < N; ++i)
    {
      if (aL[j*N+i] == rct::identity())
        test_aL += ((uint64_t)1)<<i;
      if (aR[j*N+i] == rct::zero())
        test_aR += ((uint64_t)1)<<i;
    }
    uint64_t v_test = 0;
    if (j < sv.size())
      for (int n = 0; n < 8; ++n) v_test |= (((uint64_t)sv[j][n]) << (8*n));
    CHECK_AND_ASSERT_THROW_MES(test_aL == v_test, "test_aL failed");
    CHECK_AND_ASSERT_THROW_MES(test_aR == v_test, "test_aR failed");
  }
#endif

try_again:
  rct::key hash_cache = rct::hash_to_scalar(V);

  PERF_TIMER_START_BP(PROVE_step1);
  // PAPER LINES 38-39
  rct::key alpha = rct::skGen();
  rct::key ve = vector_exponent(aL, aR);
  rct::key A;
  rct::addKeys(A, ve, rct::scalarmultBase(alpha));
  A = rct::scalarmultKey(A, INV_EIGHT);

  // PAPER LINES 40-42
  rct::keyV sL = rct::skvGen(MN), sR = rct::skvGen(MN);
  rct::key rho = rct::skGen();
  ve = vector_exponent(sL, sR);
  rct::key S;
  rct::addKeys(S, ve, rct::scalarmultBase(rho));
  S = rct::scalarmultKey(S, INV_EIGHT);

  // PAPER LINES 43-45
  rct::key y = hash_cache_mash(hash_cache, A, S);
  if (y == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step1);
    MINFO("y is 0, trying again");
    goto try_again;
  }
  rct::key z = hash_cache = rct::hash_to_scalar(y);
  if (z == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step1);
    MINFO("z is 0, trying again");
    goto try_again;
  }

  // Polynomial construction by coefficients
  const auto zMN = vector_dup(z, MN);
  rct::keyV l0 = vector_subtract(aL, zMN);
  const rct::keyV &l1 = sL;

  // This computes the ugly sum/concatenation from PAPER LINE 65
  rct::keyV zero_twos(MN);
  const rct::keyV zpow = vector_powers(z, M+2);
  for (size_t i = 0; i < MN; ++i)
  {
    zero_twos[i] = rct::zero();
    for (size_t j = 1; j <= M; ++j)
    {
      if (i >= (j-1)*N && i < j*N)
      {
        CHECK_AND_ASSERT_THROW_MES(1+j < zpow.size(), "invalid zpow index");
        CHECK_AND_ASSERT_THROW_MES(i-(j-1)*N < twoN.size(), "invalid twoN index");
        sc_muladd(zero_twos[i].bytes, zpow[1+j].bytes, twoN[i-(j-1)*N].bytes, zero_twos[i].bytes);
      }
    }
  }

  rct::keyV r0 = vector_add(aR, zMN);
  const auto yMN = vector_powers(y, MN);
  r0 = hadamard(r0, yMN);
  r0 = vector_add(r0, zero_twos);
  rct::keyV r1 = hadamard(yMN, sR);

  // Polynomial construction before PAPER LINE 46
  rct::key t1_1 = inner_product(l0, r1);
  rct::key t1_2 = inner_product(l1, r0);
  rct::key t1;
  sc_add(t1.bytes, t1_1.bytes, t1_2.bytes);
  rct::key t2 = inner_product(l1, r1);

  PERF_TIMER_STOP(PROVE_step1);

  PERF_TIMER_START_BP(PROVE_step2);
  // PAPER LINES 47-48
  rct::key tau1 = rct::skGen(), tau2 = rct::skGen();

  rct::key T1 = rct::addKeys(rct::scalarmultH(t1), rct::scalarmultBase(tau1));
  T1 = rct::scalarmultKey(T1, INV_EIGHT);
  rct::key T2 = rct::addKeys(rct::scalarmultH(t2), rct::scalarmultBase(tau2));
  T2 = rct::scalarmultKey(T2, INV_EIGHT);

  // PAPER LINES 49-51
  rct::key x = hash_cache_mash(hash_cache, z, T1, T2);
  if (x == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step2);
    MINFO("x is 0, trying again");
    goto try_again;
  }

  // PAPER LINES 52-53
  rct::key taux;
  sc_mul(taux.bytes, tau1.bytes, x.bytes);
  rct::key xsq;
  sc_mul(xsq.bytes, x.bytes, x.bytes);
  sc_muladd(taux.bytes, tau2.bytes, xsq.bytes, taux.bytes);
  for (size_t j = 1; j <= sv.size(); ++j)
  {
    CHECK_AND_ASSERT_THROW_MES(j+1 < zpow.size(), "invalid zpow index");
    sc_muladd(taux.bytes, zpow[j+1].bytes, gamma[j-1].bytes, taux.bytes);
  }
  rct::key mu;
  sc_muladd(mu.bytes, x.bytes, rho.bytes, alpha.bytes);

  // PAPER LINES 54-57
  rct::keyV l = l0;
  l = vector_add(l, vector_scalar(l1, x));
  rct::keyV r = r0;
  r = vector_add(r, vector_scalar(r1, x));
  PERF_TIMER_STOP(PROVE_step2);

  PERF_TIMER_START_BP(PROVE_step3);
  rct::key t = inner_product(l, r);

  // DEBUG: Test if the l and r vectors match the polynomial forms
#ifdef DEBUG_BP
  rct::key test_t;
  const rct::key t0 = inner_product(l0, r0);
  sc_muladd(test_t.bytes, t1.bytes, x.bytes, t0.bytes);
  sc_muladd(test_t.bytes, t2.bytes, xsq.bytes, test_t.bytes);
  CHECK_AND_ASSERT_THROW_MES(test_t == t, "test_t check failed");
#endif

  // PAPER LINES 32-33
  rct::key x_ip = hash_cache_mash(hash_cache, x, taux, mu, t);
  if (x_ip == rct::zero())
  {
    PERF_TIMER_STOP(PROVE_step3);
    MINFO("x_ip is 0, trying again");
    goto try_again;
  }

  // These are used in the inner product rounds
  size_t nprime = MN;
  rct::keyV Gprime(MN);
  rct::keyV Hprime(MN);
  rct::keyV aprime(MN);
  rct::keyV bprime(MN);
  const rct::key yinv = invert(y);
  rct::key yinvpow = rct::identity();
  for (size_t i = 0; i < MN; ++i)
  {
    Gprime[i] = Gi[i];
    Hprime[i] = scalarmultKey(Hi_p3[i], yinvpow);
    sc_mul(yinvpow.bytes, yinvpow.bytes, yinv.bytes);
    aprime[i] = l[i];
    bprime[i] = r[i];
  }
  rct::keyV L(logMN);
  rct::keyV R(logMN);
  int round = 0;
  rct::keyV w(logMN); // this is the challenge x in the inner product protocol
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
    rct::addKeys(L[round], L[round], rct::scalarmultH(tmp));
    L[round] = rct::scalarmultKey(L[round], INV_EIGHT);
    R[round] = vector_exponent_custom(slice(Gprime, 0, nprime), slice(Hprime, nprime, Hprime.size()), slice(aprime, nprime, aprime.size()), slice(bprime, 0, nprime));
    sc_mul(tmp.bytes, cR.bytes, x_ip.bytes);
    rct::addKeys(R[round], R[round], rct::scalarmultH(tmp));
    R[round] = rct::scalarmultKey(R[round], INV_EIGHT);

    // PAPER LINES 21-22
    w[round] = hash_cache_mash(hash_cache, L[round], R[round]);
    if (w[round] == rct::zero())
    {
      PERF_TIMER_STOP(PROVE_step4);
      MINFO("w[round] is 0, trying again");
      goto try_again;
    }

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

Bulletproof bulletproof_PROVE(const std::vector<uint64_t> &v, const rct::keyV &gamma)
{
  CHECK_AND_ASSERT_THROW_MES(v.size() == gamma.size(), "Incompatible sizes of v and gamma");

  // vG + gammaH
  PERF_TIMER_START_BP(PROVE_v);
  rct::keyV sv(v.size());
  for (size_t i = 0; i < v.size(); ++i)
  {
    sv[i] = rct::zero();
    sv[i].bytes[0] = v[i] & 255;
    sv[i].bytes[1] = (v[i] >> 8) & 255;
    sv[i].bytes[2] = (v[i] >> 16) & 255;
    sv[i].bytes[3] = (v[i] >> 24) & 255;
    sv[i].bytes[4] = (v[i] >> 32) & 255;
    sv[i].bytes[5] = (v[i] >> 40) & 255;
    sv[i].bytes[6] = (v[i] >> 48) & 255;
    sv[i].bytes[7] = (v[i] >> 56) & 255;
  }
  PERF_TIMER_STOP(PROVE_v);
  return bulletproof_PROVE(sv, gamma);
}

/* Given a range proof, determine if it is valid */
bool bulletproof_VERIFY(const std::vector<const Bulletproof*> &proofs)
{
  init_exponents();

  PERF_TIMER_START_BP(VERIFY);

  // sanity and figure out which proof is longest
  size_t max_length = 0;
  for (const Bulletproof *p: proofs)
  {
    const Bulletproof &proof = *p;

    // check scalar range
    CHECK_AND_ASSERT_MES(is_reduced(proof.taux), false, "Input scalar not in range");
    CHECK_AND_ASSERT_MES(is_reduced(proof.mu), false, "Input scalar not in range");
    CHECK_AND_ASSERT_MES(is_reduced(proof.a), false, "Input scalar not in range");
    CHECK_AND_ASSERT_MES(is_reduced(proof.b), false, "Input scalar not in range");
    CHECK_AND_ASSERT_MES(is_reduced(proof.t), false, "Input scalar not in range");

    CHECK_AND_ASSERT_MES(proof.V.size() >= 1, false, "V does not have at least one element");
    CHECK_AND_ASSERT_MES(proof.L.size() == proof.R.size(), false, "Mismatched L and R sizes");
    CHECK_AND_ASSERT_MES(proof.L.size() > 0, false, "Empty proof");

    max_length = std::max(max_length, proof.L.size());
  }
  CHECK_AND_ASSERT_MES(max_length < 32, false, "At least one proof is too large");
  size_t maxMN = 1u << max_length;

  const size_t logN = 6;
  const size_t N = 1 << logN;
  rct::key tmp;

  // setup weighted aggregates
  rct::key Z0 = rct::identity();
  rct::key z1 = rct::zero();
  rct::key Z2 = rct::identity();
  rct::key z3 = rct::zero();
  rct::keyV z4(maxMN, rct::zero()), z5(maxMN, rct::zero());
  rct::key Y2 = rct::identity(), Y3 = rct::identity(), Y4 = rct::identity();
  rct::key y0 = rct::zero(), y1 = rct::zero();
  for (const Bulletproof *p: proofs)
  {
    const Bulletproof &proof = *p;

    size_t M, logM;
    for (logM = 0; (M = 1<<logM) <= maxM && M < proof.V.size(); ++logM);
    CHECK_AND_ASSERT_MES(proof.L.size() == 6+logM, false, "Proof is not the expected size");
    const size_t MN = M*N;
    rct::key weight = rct::skGen();

    // Reconstruct the challenges
    PERF_TIMER_START_BP(VERIFY_start);
    rct::key hash_cache = rct::hash_to_scalar(proof.V);
    rct::key y = hash_cache_mash(hash_cache, proof.A, proof.S);
    CHECK_AND_ASSERT_MES(!(y == rct::zero()), false, "y == 0");
    rct::key z = hash_cache = rct::hash_to_scalar(y);
    CHECK_AND_ASSERT_MES(!(z == rct::zero()), false, "z == 0");
    rct::key x = hash_cache_mash(hash_cache, z, proof.T1, proof.T2);
    CHECK_AND_ASSERT_MES(!(x == rct::zero()), false, "x == 0");
    rct::key x_ip = hash_cache_mash(hash_cache, x, proof.taux, proof.mu, proof.t);
    CHECK_AND_ASSERT_MES(!(x_ip == rct::zero()), false, "x_ip == 0");
    PERF_TIMER_STOP(VERIFY_start);

    // pre-multiply some points by 8
    rct::keyV proof8_V = proof.V; for (rct::key &k: proof8_V) k = rct::scalarmult8(k);
    rct::keyV proof8_L = proof.L; for (rct::key &k: proof8_L) k = rct::scalarmult8(k);
    rct::keyV proof8_R = proof.R; for (rct::key &k: proof8_R) k = rct::scalarmult8(k);
    rct::key proof8_T1 = rct::scalarmult8(proof.T1);
    rct::key proof8_T2 = rct::scalarmult8(proof.T2);
    rct::key proof8_S = rct::scalarmult8(proof.S);

    PERF_TIMER_START_BP(VERIFY_line_61);
    // PAPER LINE 61
    sc_muladd(y0.bytes, proof.taux.bytes, weight.bytes, y0.bytes);

    const rct::keyV zpow = vector_powers(z, M+3);

    rct::key k;
    const rct::key ip1y = vector_power_sum(y, MN);
    sc_mulsub(k.bytes, zpow[2].bytes, ip1y.bytes, rct::zero().bytes);
    for (size_t j = 1; j <= M; ++j)
    {
      CHECK_AND_ASSERT_MES(j+2 < zpow.size(), false, "invalid zpow index");
      sc_mulsub(k.bytes, zpow[j+2].bytes, ip12.bytes, k.bytes);
    }
    PERF_TIMER_STOP(VERIFY_line_61);

    PERF_TIMER_START_BP(VERIFY_line_61rl_new);
    sc_muladd(tmp.bytes, z.bytes, ip1y.bytes, k.bytes);
    std::vector<MultiexpData> multiexp_data;
    multiexp_data.reserve(proof.V.size());
    sc_sub(tmp.bytes, proof.t.bytes, tmp.bytes);
    sc_muladd(y1.bytes, tmp.bytes, weight.bytes, y1.bytes);
    for (size_t j = 0; j < proof8_V.size(); j++)
    {
      multiexp_data.emplace_back(zpow[j+2], proof8_V[j]);
    }
    rct::addKeys(Y2, Y2, rct::scalarmultKey(multiexp(multiexp_data, false), weight));
    sc_mul(tmp.bytes, x.bytes, weight.bytes);
    rct::addKeys(Y3, Y3, rct::scalarmultKey(proof8_T1, tmp));
    rct::key xsq;
    sc_mul(xsq.bytes, x.bytes, x.bytes);
    sc_mul(tmp.bytes, xsq.bytes, weight.bytes);
    rct::addKeys(Y4, Y4, rct::scalarmultKey(proof8_T2, tmp));
    PERF_TIMER_STOP(VERIFY_line_61rl_new);

    PERF_TIMER_START_BP(VERIFY_line_62);
    // PAPER LINE 62
    rct::addKeys(Z0, Z0, rct::scalarmultKey(rct::addKeys(rct::scalarmult8(proof.A), rct::scalarmultKey(proof8_S, x)), weight));
    PERF_TIMER_STOP(VERIFY_line_62);

    // Compute the number of rounds for the inner product
    const size_t rounds = logM+logN;
    CHECK_AND_ASSERT_MES(rounds > 0, false, "Zero rounds");

    PERF_TIMER_START_BP(VERIFY_line_21_22);
    // PAPER LINES 21-22
    // The inner product challenges are computed per round
    rct::keyV w(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      w[i] = hash_cache_mash(hash_cache, proof.L[i], proof.R[i]);
      CHECK_AND_ASSERT_MES(!(w[i] == rct::zero()), false, "w[i] == 0");
    }
    PERF_TIMER_STOP(VERIFY_line_21_22);

    PERF_TIMER_START_BP(VERIFY_line_24_25);
    // Basically PAPER LINES 24-25
    // Compute the curvepoints from G[i] and H[i]
    rct::key yinvpow = rct::identity();
    rct::key ypow = rct::identity();

    PERF_TIMER_START_BP(VERIFY_line_24_25_invert);
    const rct::key yinv = invert(y);
    rct::keyV winv(rounds);
    for (size_t i = 0; i < rounds; ++i)
      winv[i] = invert(w[i]);
    PERF_TIMER_STOP(VERIFY_line_24_25_invert);

    for (size_t i = 0; i < MN; ++i)
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
      CHECK_AND_ASSERT_MES(2+i/N < zpow.size(), false, "invalid zpow index");
      CHECK_AND_ASSERT_MES(i%N < twoN.size(), false, "invalid twoN index");
      sc_mul(tmp.bytes, zpow[2+i/N].bytes, twoN[i%N].bytes);
      sc_muladd(tmp.bytes, z.bytes, ypow.bytes, tmp.bytes);
      sc_mulsub(h_scalar.bytes, tmp.bytes, yinvpow.bytes, h_scalar.bytes);

      sc_muladd(z4[i].bytes, g_scalar.bytes, weight.bytes, z4[i].bytes);
      sc_muladd(z5[i].bytes, h_scalar.bytes, weight.bytes, z5[i].bytes);

      if (i != MN-1)
      {
        sc_mul(yinvpow.bytes, yinvpow.bytes, yinv.bytes);
        sc_mul(ypow.bytes, ypow.bytes, y.bytes);
      }
    }

    PERF_TIMER_STOP(VERIFY_line_24_25);

    // PAPER LINE 26
    PERF_TIMER_START_BP(VERIFY_line_26_new);
    multiexp_data.clear();
    multiexp_data.reserve(2*rounds);

    sc_muladd(z1.bytes, proof.mu.bytes, weight.bytes, z1.bytes);
    for (size_t i = 0; i < rounds; ++i)
    {
      sc_mul(tmp.bytes, w[i].bytes, w[i].bytes);
      multiexp_data.emplace_back(tmp, proof8_L[i]);
      sc_mul(tmp.bytes, winv[i].bytes, winv[i].bytes);
      multiexp_data.emplace_back(tmp, proof8_R[i]);
    }
    rct::key acc = multiexp(multiexp_data, false);
    rct::addKeys(Z2, Z2, rct::scalarmultKey(acc, weight));
    sc_mulsub(tmp.bytes, proof.a.bytes, proof.b.bytes, proof.t.bytes);
    sc_mul(tmp.bytes, tmp.bytes, x_ip.bytes);
    sc_muladd(z3.bytes, tmp.bytes, weight.bytes, z3.bytes);
    PERF_TIMER_STOP(VERIFY_line_26_new);
  }

  // now check all proofs at once
  PERF_TIMER_START_BP(VERIFY_step2_check);
  ge_p3 check1;
  ge_scalarmult_base(&check1, y0.bytes);
  addKeys_acc_p3(&check1, y1, rct::H);
  sub_acc_p3(&check1, Y2);
  sub_acc_p3(&check1, Y3);
  sub_acc_p3(&check1, Y4);
  if (!ge_p3_is_point_at_infinity(&check1))
  {
    MERROR("Verification failure at step 1");
    return false;
  }
  ge_p3 check2;
  sc_sub(tmp.bytes, rct::zero().bytes, z1.bytes);
  ge_double_scalarmult_base_vartime_p3(&check2, z3.bytes, &ge_p3_H, tmp.bytes);
  add_acc_p3(&check2, Z0);
  add_acc_p3(&check2, Z2);

  std::vector<MultiexpData> multiexp_data;
  multiexp_data.reserve(2 * maxMN);
  for (size_t i = 0; i < maxMN; ++i)
  {
    sc_sub(tmp.bytes, rct::zero().bytes, z4[i].bytes);
    multiexp_data.emplace_back(tmp, Gi_p3[i]);
    sc_sub(tmp.bytes, rct::zero().bytes, z5[i].bytes);
    multiexp_data.emplace_back(tmp, Hi_p3[i]);
  }
  add_acc_p3(&check2, multiexp(multiexp_data, true));
  PERF_TIMER_STOP(VERIFY_step2_check);

  if (!ge_p3_is_point_at_infinity(&check2))
  {
    MERROR("Verification failure at step 2");
    return false;
  }

  PERF_TIMER_STOP(VERIFY);
  return true;
}

bool bulletproof_VERIFY(const std::vector<Bulletproof> &proofs)
{
  std::vector<const Bulletproof*> proof_pointers;
  for (const Bulletproof &proof: proofs)
    proof_pointers.push_back(&proof);
  return bulletproof_VERIFY(proof_pointers);
}

bool bulletproof_VERIFY(const Bulletproof &proof)
{
  std::vector<const Bulletproof*> proofs;
  proofs.push_back(&proof);
  return bulletproof_VERIFY(proofs);
}

}
