// Copyright (c) 2017, The Monero Project
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
// Adapted from Python code by Sarang Noether

#include "misc_log_ex.h"
#include "common/perf_timer.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "common/aligned.h"
#include "rctOps.h"
#include "multiexp.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multiexp"

//#define MULTIEXP_PERF(x) x
#define MULTIEXP_PERF(x)

#define RAW_MEMORY_BLOCK
//#define ALTERNATE_LAYOUT
//#define TRACK_STRAUS_ZERO_IDENTITY

//   per points us for N/B points (B point bands)
//   raw   alt   128/192  4096/192  4096/4096
//   0     0     52.6     71        71.2
//   0     1     53.2     72.2      72.4
//   1     0     52.7     67        67.1
//   1     1     52.8     70.4      70.2

// Pippenger:
// 	1	2	3	4	5	6	7	8	9	bestN
// 2	555	598	621	804	1038	1733	2486	5020	8304	1
// 4	783	747	800	1006	1428	2132	3285	5185	9806	2
// 8	1174	1071	1095	1286	1640	2398	3869	6378	12080	2
// 16	2279	1874	1745	1739	2144	2831	4209	6964	12007	4
// 32	3910	3706	2588	2477	2782	3467	4856	7489	12618	4
// 64	7184	5429	4710	4368	4010	4672	6027	8559	13684	5
// 128	14097	10574	8452	7297	6841	6718	8615	10580	15641	6
// 256	27715	20800	16000	13550	11875	11400	11505	14090	18460	6
// 512	55100	41250	31740	26570	22030	19830	20760	21380	25215	6
// 1024	111520	79000	61080	49720	43080	38320	37600	35040	36750	8
// 2048	219480	162680	122120	102080	83760	70360	66600	63920	66160	8
// 4096	453320	323080	247240	210200	180040	150240	132440	114920	110560	9

// 			2	4	8	16	32	64	128	256	512	1024	2048	4096
// Bos Coster		858	994	1316	1949	3183	5512	9865	17830	33485	63160	124280	246320
// Straus		226	341	548	980	1870	3538	7039	14490	29020	57200	118640	233640
// Straus/cached	226	315	485	785	1514	2858	5753	11065	22970	45120	98880	194840
// Pippenger		555	747	1071	1739	2477	4010	6718	11400	19830	35040	63920	110560

// Best/cached		Straus	Straus	Straus	Straus	Straus	Straus	Straus	Straus	Pip	Pip	Pip	Pip
// Best/uncached	Straus	Straus	Straus	Straus	Straus	Straus	Pip	Pip	Pip	Pip	Pip	Pip

namespace rct
{

static inline bool operator<(const rct::key &k0, const rct::key&k1)
{
  for (int n = 31; n >= 0; --n)
  {
    if (k0.bytes[n] < k1.bytes[n])
      return true;
    if (k0.bytes[n] > k1.bytes[n])
      return false;
  }
  return false;
}

static inline rct::key div2(const rct::key &k)
{
  rct::key res;
  int carry = 0;
  for (int n = 31; n >= 0; --n)
  {
    int new_carry = (k.bytes[n] & 1) << 7;
    res.bytes[n] = k.bytes[n] / 2 + carry;
    carry = new_carry;
  }
  return res;
}

static inline rct::key pow2(size_t n)
{
  CHECK_AND_ASSERT_THROW_MES(n < 256, "Invalid pow2 argument");
  rct::key res = rct::zero();
  res[n >> 3] |= 1<<(n&7);
  return res;
}

static inline int test(const rct::key &k, size_t n)
{
  if (n >= 256) return 0;
  return k[n >> 3] & (1 << (n & 7));
}

static inline void add(ge_p3 &p3, const ge_cached &other)
{
  ge_p1p1 p1;
  ge_add(&p1, &p3, &other);
  ge_p1p1_to_p3(&p3, &p1);
}

static inline void add(ge_p3 &p3, const ge_p3 &other)
{
  ge_cached cached;
  ge_p3_to_cached(&cached, &other);
  add(p3, cached);
}

rct::key bos_coster_heap_conv(std::vector<MultiexpData> data)
{
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(bos_coster, 1000000));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
  size_t points = data.size();
  CHECK_AND_ASSERT_THROW_MES(points > 1, "Not enough points");
  std::vector<size_t> heap(points);
  for (size_t n = 0; n < points; ++n)
    heap[n] = n;

  auto Comp = [&](size_t e0, size_t e1) { return data[e0].scalar < data[e1].scalar; };
  std::make_heap(heap.begin(), heap.end(), Comp);
  MULTIEXP_PERF(PERF_TIMER_STOP(setup));

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(loop, 1000000));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(pop, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(add, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(add));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(sub, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(push, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
  while (heap.size() > 1)
  {
    MULTIEXP_PERF(PERF_TIMER_RESUME(pop));
    std::pop_heap(heap.begin(), heap.end(), Comp);
    size_t index1 = heap.back();
    heap.pop_back();
    std::pop_heap(heap.begin(), heap.end(), Comp);
    size_t index2 = heap.back();
    heap.pop_back();
    MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));

    MULTIEXP_PERF(PERF_TIMER_RESUME(add));
    ge_cached cached;
    ge_p3_to_cached(&cached, &data[index1].point);
    ge_p1p1 p1;
    ge_add(&p1, &data[index2].point, &cached);
    ge_p1p1_to_p3(&data[index2].point, &p1);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(add));

    MULTIEXP_PERF(PERF_TIMER_RESUME(sub));
    sc_sub(data[index1].scalar.bytes, data[index1].scalar.bytes, data[index2].scalar.bytes);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));

    MULTIEXP_PERF(PERF_TIMER_RESUME(push));
    if (!(data[index1].scalar == rct::zero()))
    {
      heap.push_back(index1);
      std::push_heap(heap.begin(), heap.end(), Comp);
    }

    heap.push_back(index2);
    std::push_heap(heap.begin(), heap.end(), Comp);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
  }
  MULTIEXP_PERF(PERF_TIMER_STOP(push));
  MULTIEXP_PERF(PERF_TIMER_STOP(sub));
  MULTIEXP_PERF(PERF_TIMER_STOP(add));
  MULTIEXP_PERF(PERF_TIMER_STOP(pop));
  MULTIEXP_PERF(PERF_TIMER_STOP(loop));

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(end, 1000000));
  //return rct::scalarmultKey(data[index1].point, data[index1].scalar);
  std::pop_heap(heap.begin(), heap.end(), Comp);
  size_t index1 = heap.back();
  heap.pop_back();
  ge_p2 p2;
  ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
  rct::key res;
  ge_tobytes(res.bytes, &p2);
  return res;
}

rct::key bos_coster_heap_conv_robust(std::vector<MultiexpData> data)
{
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(bos_coster, 1000000));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
  size_t points = data.size();
  CHECK_AND_ASSERT_THROW_MES(points > 0, "Not enough points");
  std::vector<size_t> heap;
  heap.reserve(points);
  for (size_t n = 0; n < points; ++n)
  {
    if (!(data[n].scalar == rct::zero()) && !ge_p3_is_point_at_infinity(&data[n].point))
      heap.push_back(n);
  }
  points = heap.size();
  if (points == 0)
    return rct::identity();

  auto Comp = [&](size_t e0, size_t e1) { return data[e0].scalar < data[e1].scalar; };
  std::make_heap(heap.begin(), heap.end(), Comp);

  if (points < 2)
  {
    std::pop_heap(heap.begin(), heap.end(), Comp);
    size_t index1 = heap.back();
    ge_p2 p2;
    ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
    rct::key res;
    ge_tobytes(res.bytes, &p2);
    return res;
  }

  MULTIEXP_PERF(PERF_TIMER_STOP(setup));

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(loop, 1000000));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(pop, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(div, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(div));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(add, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(add));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(sub, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(push, 1000000)); MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
  while (heap.size() > 1)
  {
    MULTIEXP_PERF(PERF_TIMER_RESUME(pop));
    std::pop_heap(heap.begin(), heap.end(), Comp);
    size_t index1 = heap.back();
    heap.pop_back();
    std::pop_heap(heap.begin(), heap.end(), Comp);
    size_t index2 = heap.back();
    heap.pop_back();
    MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));

    ge_cached cached;
    ge_p1p1 p1;
    ge_p2 p2;

    MULTIEXP_PERF(PERF_TIMER_RESUME(div));
    while (1)
    {
      rct::key s1_2 = div2(data[index1].scalar);
      if (!(data[index2].scalar < s1_2))
       break;
      if (data[index1].scalar.bytes[0] & 1)
      {
        data.resize(data.size()+1);
        data.back().scalar = rct::identity();
        data.back().point = data[index1].point;
        heap.push_back(data.size() - 1);
        std::push_heap(heap.begin(), heap.end(), Comp);
      }
      data[index1].scalar = div2(data[index1].scalar);
      ge_p3_to_p2(&p2, &data[index1].point);
      ge_p2_dbl(&p1, &p2);
      ge_p1p1_to_p3(&data[index1].point, &p1);
    }
    MULTIEXP_PERF(PERF_TIMER_PAUSE(div));

    MULTIEXP_PERF(PERF_TIMER_RESUME(add));
    ge_p3_to_cached(&cached, &data[index1].point);
    ge_add(&p1, &data[index2].point, &cached);
    ge_p1p1_to_p3(&data[index2].point, &p1);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(add));

    MULTIEXP_PERF(PERF_TIMER_RESUME(sub));
    sc_sub(data[index1].scalar.bytes, data[index1].scalar.bytes, data[index2].scalar.bytes);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));

    MULTIEXP_PERF(PERF_TIMER_RESUME(push));
    if (!(data[index1].scalar == rct::zero()))
    {
      heap.push_back(index1);
      std::push_heap(heap.begin(), heap.end(), Comp);
    }

    heap.push_back(index2);
    std::push_heap(heap.begin(), heap.end(), Comp);
    MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
  }
  MULTIEXP_PERF(PERF_TIMER_STOP(push));
  MULTIEXP_PERF(PERF_TIMER_STOP(sub));
  MULTIEXP_PERF(PERF_TIMER_STOP(add));
  MULTIEXP_PERF(PERF_TIMER_STOP(pop));
  MULTIEXP_PERF(PERF_TIMER_STOP(loop));

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(end, 1000000));
  //return rct::scalarmultKey(data[index1].point, data[index1].scalar);
  std::pop_heap(heap.begin(), heap.end(), Comp);
  size_t index1 = heap.back();
  heap.pop_back();
  ge_p2 p2;
  ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
  rct::key res;
  ge_tobytes(res.bytes, &p2);
  return res;
}

static constexpr unsigned int STRAUS_C = 4;

struct straus_cached_data
{
#ifdef RAW_MEMORY_BLOCK
  size_t size;
  ge_cached *multiples;
  straus_cached_data(): size(0), multiples(NULL) {}
  ~straus_cached_data() { aligned_free(multiples); }
#else
  std::vector<std::vector<ge_cached>> multiples;
#endif
};
#ifdef RAW_MEMORY_BLOCK
#ifdef ALTERNATE_LAYOUT
#define CACHE_OFFSET(cache,point,digit) cache->multiples[(point)*((1<<STRAUS_C)-1)+((digit)-1)]
#else
#define CACHE_OFFSET(cache,point,digit) cache->multiples[(point)+cache->size*((digit)-1)]
#endif
#else
#ifdef ALTERNATE_LAYOUT
#define CACHE_OFFSET(cache,point,digit) local_cache->multiples[j][digit-1]
#else
#define CACHE_OFFSET(cache,point,digit) local_cache->multiples[digit][j]
#endif
#endif

std::shared_ptr<straus_cached_data> straus_init_cache(const std::vector<MultiexpData> &data, size_t N)
{
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(multiples, 1000000));
  if (N == 0)
    N = data.size();
  CHECK_AND_ASSERT_THROW_MES(N <= data.size(), "Bad cache base data");
  ge_cached cached;
  ge_p1p1 p1;
  ge_p3 p3;
  std::shared_ptr<straus_cached_data> cache(new straus_cached_data());

#ifdef RAW_MEMORY_BLOCK
  const size_t offset = cache->size;
  cache->multiples = (ge_cached*)aligned_realloc(cache->multiples, sizeof(ge_cached) * ((1<<STRAUS_C)-1) * std::max(offset, N), 4096);
  CHECK_AND_ASSERT_THROW_MES(cache->multiples, "Out of memory");
  cache->size = N;
  for (size_t j=offset;j<N;++j)
  {
    ge_p3_to_cached(&CACHE_OFFSET(cache, j, 1), &data[j].point);
    for (size_t i=2;i<1<<STRAUS_C;++i)
    {
      ge_add(&p1, &data[j].point, &CACHE_OFFSET(cache, j, i-1));
      ge_p1p1_to_p3(&p3, &p1);
      ge_p3_to_cached(&CACHE_OFFSET(cache, j, i), &p3);
    }
  }
#else
#ifdef ALTERNATE_LAYOUT
  const size_t offset = cache->multiples.size();
  cache->multiples.resize(std::max(offset, N));
  for (size_t i = offset; i < N; ++i)
  {
    cache->multiples[i].resize((1<<STRAUS_C)-1);
    ge_p3_to_cached(&cache->multiples[i][0], &data[i].point);
    for (size_t j=2;j<1<<STRAUS_C;++j)
    {
      ge_add(&p1, &data[i].point, &cache->multiples[i][j-2]);
      ge_p1p1_to_p3(&p3, &p1);
      ge_p3_to_cached(&cache->multiples[i][j-1], &p3);
    }
  }
#else
  cache->multiples.resize(1<<STRAUS_C);
  size_t offset = cache->multiples[1].size();
  cache->multiples[1].resize(std::max(offset, N));
  for (size_t i = offset; i < N; ++i)
    ge_p3_to_cached(&cache->multiples[1][i], &data[i].point);
  for (size_t i=2;i<1<<STRAUS_C;++i)
    cache->multiples[i].resize(std::max(offset, N));
  for (size_t j=offset;j<N;++j)
  {
    for (size_t i=2;i<1<<STRAUS_C;++i)
    {
      ge_add(&p1, &data[j].point, &cache->multiples[i-1][j]);
      ge_p1p1_to_p3(&p3, &p1);
      ge_p3_to_cached(&cache->multiples[i][j], &p3);
    }
  }
#endif
#endif
  MULTIEXP_PERF(PERF_TIMER_STOP(multiples));

  return cache;
}

size_t straus_get_cache_size(const std::shared_ptr<straus_cached_data> &cache)
{
  size_t sz = 0;
#ifdef RAW_MEMORY_BLOCK
  sz += cache->size * sizeof(ge_cached) * ((1<<STRAUS_C)-1);
#else
  for (const auto &e0: cache->multiples)
    sz += e0.size() * sizeof(ge_cached);
#endif
  return sz;
}

rct::key straus(const std::vector<MultiexpData> &data, const std::shared_ptr<straus_cached_data> &cache, size_t STEP)
{
  CHECK_AND_ASSERT_THROW_MES(cache == NULL || cache->size >= data.size(), "Cache is too small");
  MULTIEXP_PERF(PERF_TIMER_UNIT(straus, 1000000));
  bool HiGi = cache != NULL;
  STEP = STEP ? STEP : 192;

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
  static constexpr unsigned int mask = (1<<STRAUS_C)-1;
  std::shared_ptr<straus_cached_data> local_cache = cache == NULL ? straus_init_cache(data) : cache;
  ge_cached cached;
  ge_p1p1 p1;
  ge_p3 p3;

#ifdef TRACK_STRAUS_ZERO_IDENTITY
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(skip, 1000000));
  std::vector<uint8_t> skip(data.size());
  for (size_t i = 0; i < data.size(); ++i)
    skip[i] = data[i].scalar == rct::zero() || ge_p3_is_point_at_infinity(&data[i].point);
  MULTIEXP_PERF(PERF_TIMER_STOP(skip));
#endif

  MULTIEXP_PERF(PERF_TIMER_START_UNIT(digits, 1000000));
  std::unique_ptr<uint8_t[]> digits{new uint8_t[256 * data.size()]};
  for (size_t j = 0; j < data.size(); ++j)
  {
    unsigned char bytes33[33];
    memcpy(bytes33,  data[j].scalar.bytes, 32);
    bytes33[32] = 0;
    const unsigned char *bytes = bytes33;
#if 1
    static_assert(STRAUS_C == 4, "optimized version needs STRAUS_C == 4");
    unsigned int i;
    for (i = 0; i < 256; i += 8, bytes++)
    {
      digits[j*256+i] = bytes[0] & 0xf;
      digits[j*256+i+1] = (bytes[0] >> 1) & 0xf;
      digits[j*256+i+2] = (bytes[0] >> 2) & 0xf;
      digits[j*256+i+3] = (bytes[0] >> 3) & 0xf;
      digits[j*256+i+4] = ((bytes[0] >> 4) | (bytes[1]<<4)) & 0xf;
      digits[j*256+i+5] = ((bytes[0] >> 5) | (bytes[1]<<3)) & 0xf;
      digits[j*256+i+6] = ((bytes[0] >> 6) | (bytes[1]<<2)) & 0xf;
      digits[j*256+i+7] = ((bytes[0] >> 7) | (bytes[1]<<1)) & 0xf;
    }
#elif 1
    for (size_t i = 0; i < 256; ++i)
      digits[j*256+i] = ((bytes[i>>3] | (bytes[(i>>3)+1]<<8)) >> (i&7)) & mask;
#else
    rct::key shifted = data[j].scalar;
    for (size_t i = 0; i < 256; ++i)
    {
      digits[j*256+i] = shifted.bytes[0] & 0xf;
      shifted = div2(shifted, (256-i)>>3);
    }
#endif
  }
  MULTIEXP_PERF(PERF_TIMER_STOP(digits));

  rct::key maxscalar = rct::zero();
  for (size_t i = 0; i < data.size(); ++i)
    if (maxscalar < data[i].scalar)
      maxscalar = data[i].scalar;
  size_t start_i = 0;
  while (start_i < 256 && !(maxscalar < pow2(start_i)))
    start_i += STRAUS_C;
  MULTIEXP_PERF(PERF_TIMER_STOP(setup));

  ge_p3 res_p3 = ge_p3_identity;

  for (size_t start_offset = 0; start_offset < data.size(); start_offset += STEP)
  {
    const size_t num_points = std::min(data.size() - start_offset, STEP);

    ge_p3 band_p3 = ge_p3_identity;
    size_t i = start_i;
    if (!(i < STRAUS_C))
      goto skipfirst;
    while (!(i < STRAUS_C))
    {
      ge_p2 p2;
      ge_p3_to_p2(&p2, &band_p3);
      for (size_t j = 0; j < STRAUS_C; ++j)
      {
        ge_p2_dbl(&p1, &p2);
        if (j == STRAUS_C - 1)
          ge_p1p1_to_p3(&band_p3, &p1);
        else
          ge_p1p1_to_p2(&p2, &p1);
      }
skipfirst:
      i -= STRAUS_C;
      for (size_t j = start_offset; j < start_offset + num_points; ++j)
      {
#ifdef TRACK_STRAUS_ZERO_IDENTITY
        if (skip[j])
          continue;
#endif
        const uint8_t digit = digits[j*256+i];
        if (digit)
        {
          ge_add(&p1, &band_p3, &CACHE_OFFSET(local_cache, j, digit));
          ge_p1p1_to_p3(&band_p3, &p1);
        }
      }
    }

    ge_p3_to_cached(&cached, &band_p3);
    ge_add(&p1, &res_p3, &cached);
    ge_p1p1_to_p3(&res_p3, &p1);
  }

  rct::key res;
  ge_p3_tobytes(res.bytes, &res_p3);
  return res;
}

size_t get_pippenger_c(size_t N)
{
// uncached: 2:1, 4:2, 8:2, 16:3, 32:4, 64:4, 128:5, 256:6, 512:7, 1024:7, 2048:8, 4096:9
//   cached: 2:1, 4:2, 8:2, 16:3, 32:4, 64:4, 128:5, 256:6, 512:7, 1024:7, 2048:8, 4096:9
  if (N <= 2) return 1;
  if (N <= 8) return 2;
  if (N <= 16) return 3;
  if (N <= 64) return 4;
  if (N <= 128) return 5;
  if (N <= 256) return 6;
  if (N <= 1024) return 7;
  if (N <= 2048) return 8;
  return 9;
}

struct pippenger_cached_data
{
  size_t size;
  ge_cached *cached;
  pippenger_cached_data(): size(0), cached(NULL) {}
  ~pippenger_cached_data() { aligned_free(cached); }
};

std::shared_ptr<pippenger_cached_data> pippenger_init_cache(const std::vector<MultiexpData> &data, size_t N)
{
  MULTIEXP_PERF(PERF_TIMER_START_UNIT(pippenger_init_cache, 1000000));
  if (N == 0)
    N = data.size();
  CHECK_AND_ASSERT_THROW_MES(N <= data.size(), "Bad cache base data");
  ge_cached cached;
  std::shared_ptr<pippenger_cached_data> cache(new pippenger_cached_data());

  cache->size = N;
  cache->cached = (ge_cached*)aligned_realloc(cache->cached, N * sizeof(ge_cached), 4096);
  CHECK_AND_ASSERT_THROW_MES(cache->cached, "Out of memory");
  for (size_t i = 0; i < N; ++i)
    ge_p3_to_cached(&cache->cached[i], &data[i].point);

  MULTIEXP_PERF(PERF_TIMER_STOP(pippenger_init_cache));
  return cache;
}

size_t pippenger_get_cache_size(const std::shared_ptr<pippenger_cached_data> &cache)
{
  return cache->size * sizeof(*cache->cached);
}

rct::key pippenger(const std::vector<MultiexpData> &data, const std::shared_ptr<pippenger_cached_data> &cache, size_t c)
{
  CHECK_AND_ASSERT_THROW_MES(cache == NULL || cache->size >= data.size(), "Cache is too small");
  if (c == 0)
    c = get_pippenger_c(data.size());
  CHECK_AND_ASSERT_THROW_MES(c <= 9, "c is too large");

  ge_p3 result = ge_p3_identity;
  std::unique_ptr<ge_p3[]> buckets{new ge_p3[1<<c]};
  std::shared_ptr<pippenger_cached_data> local_cache = cache == NULL ? pippenger_init_cache(data) : cache;

  rct::key maxscalar = rct::zero();
  for (size_t i = 0; i < data.size(); ++i)
  {
    if (maxscalar < data[i].scalar)
      maxscalar = data[i].scalar;
  }
  size_t groups = 0;
  while (groups < 256 && !(maxscalar < pow2(groups)))
    ++groups;
  groups = (groups + c - 1) / c;

  for (size_t k = groups; k-- > 0; )
  {
    if (!ge_p3_is_point_at_infinity(&result))
    {
      ge_p2 p2;
      ge_p3_to_p2(&p2, &result);
      for (size_t i = 0; i < c; ++i)
      {
        ge_p1p1 p1;
        ge_p2_dbl(&p1, &p2);
        if (i == c - 1)
          ge_p1p1_to_p3(&result, &p1);
        else
          ge_p1p1_to_p2(&p2, &p1);
      }
    }
    for (size_t i = 0; i < (1u<<c); ++i)
      buckets[i] = ge_p3_identity;

    // partition scalars into buckets
    for (size_t i = 0; i < data.size(); ++i)
    {
      unsigned int bucket = 0;
      for (size_t j = 0; j < c; ++j)
        if (test(data[i].scalar, k*c+j))
          bucket |= 1<<j;
      if (bucket == 0)
        continue;
      CHECK_AND_ASSERT_THROW_MES(bucket < (1u<<c), "bucket overflow");
      if (!ge_p3_is_point_at_infinity(&buckets[bucket]))
      {
        add(buckets[bucket], local_cache->cached[i]);
      }
      else
        buckets[bucket] = data[i].point;
    }

    // sum the buckets
    ge_p3 pail = ge_p3_identity;
    for (size_t i = (1<<c)-1; i > 0; --i)
    {
      if (!ge_p3_is_point_at_infinity(&buckets[i]))
        add(pail, buckets[i]);
      if (!ge_p3_is_point_at_infinity(&pail))
        add(result, pail);
    }
  }

  rct::key res;
  ge_p3_tobytes(res.bytes, &result);
  return res;
}

}
