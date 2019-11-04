// Copyright (c) 2019, Ryo Currency Project
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
// Parts of this file are originally copyright (c) 2014-2017, SUMOKOIN
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#define CN_ADD_TARGETS_AND_HEADERS
#define INTEL_AVX2

#include "../keccak.h"
#include "aux_hash.h"
#include "cn_slow_hash.hpp"

#ifdef HAS_INTEL_HW

inline void prep_dv_avx(cn_sptr& idx, __m256i& v, __m256& n01)
{
	v = _mm256_load_si256(idx.as_ptr<__m256i>());
	n01 = _mm256_cvtepi32_ps(v);
}

inline __m256 _mm256_set1_ps_epi32(uint32_t x)
{
	return _mm256_castsi256_ps(_mm256_set1_epi32(x));
}

inline __m128 _mm_set1_ps_epi32(uint32_t x)
{
	return _mm_castsi128_ps(_mm_set1_epi32(x));
}

inline __m256 fma_break(const __m256& x) 
{ 
	// Break the dependency chain by setitng the exp to ?????01 
	__m256 xx = _mm256_and_ps(_mm256_set1_ps_epi32(0xFEFFFFFF), x); 
	return _mm256_or_ps(_mm256_set1_ps_epi32(0x00800000), xx); 
}

// 14
inline void sub_round(const __m256& n0, const __m256& n1, const __m256& n2, const __m256& n3, const __m256& rnd_c, __m256& n, __m256& d, __m256& c)
{
	__m256 nn = _mm256_mul_ps(n0, c);
	nn = _mm256_mul_ps(_mm256_add_ps(n1, c), _mm256_mul_ps(nn, nn));
	nn = fma_break(nn);
	n = _mm256_add_ps(n, nn);

	__m256 dd = _mm256_mul_ps(n2, c);
	dd = _mm256_mul_ps(_mm256_sub_ps(n3, c), _mm256_mul_ps(dd, dd));
	dd = fma_break(dd);
	d = _mm256_add_ps(d, dd);

	//Constant feedback
	c = _mm256_add_ps(c, rnd_c);
	c = _mm256_add_ps(c, _mm256_set1_ps(0.734375f));
	__m256 r = _mm256_add_ps(nn, dd);
	r = _mm256_and_ps(_mm256_set1_ps_epi32(0x807FFFFF), r);
	r = _mm256_or_ps(_mm256_set1_ps_epi32(0x40000000), r);
	c = _mm256_add_ps(c, r);
}

// 14*8 + 2 = 112
inline void round_compute(const __m256& n0, const __m256& n1, const __m256& n2, const __m256& n3, const __m256& rnd_c, __m256& c, __m256& r)
{
	__m256 n = _mm256_setzero_ps(), d = _mm256_setzero_ps();

	sub_round(n0, n1, n2, n3, rnd_c, n, d, c);
	sub_round(n1, n2, n3, n0, rnd_c, n, d, c);
	sub_round(n2, n3, n0, n1, rnd_c, n, d, c);
	sub_round(n3, n0, n1, n2, rnd_c, n, d, c);
	sub_round(n3, n2, n1, n0, rnd_c, n, d, c);
	sub_round(n2, n1, n0, n3, rnd_c, n, d, c);
	sub_round(n1, n0, n3, n2, rnd_c, n, d, c);
	sub_round(n0, n3, n2, n1, rnd_c, n, d, c);

	// Make sure abs(d) > 2.0 - this prevents division by zero and accidental overflows by division by < 1.0
	d = _mm256_and_ps(_mm256_set1_ps_epi32(0xFF7FFFFF), d);
	d = _mm256_or_ps(_mm256_set1_ps_epi32(0x40000000), d);
	r = _mm256_add_ps(r, _mm256_div_ps(n, d));
}

// 112×4 = 448
template <bool add>
inline __m256i double_comupte(const __m256& n0, const __m256& n1, const __m256& n2, const __m256& n3, 
							  float lcnt, float hcnt, const __m256& rnd_c, __m256& sum)
{
	__m256 c = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_set1_ps(lcnt)), _mm_set1_ps(hcnt), 1);
	__m256 r = _mm256_setzero_ps();

	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);

	// do a quick fmod by setting exp to 2
	r = _mm256_and_ps(_mm256_set1_ps_epi32(0x807FFFFF), r);
	r = _mm256_or_ps(_mm256_set1_ps_epi32(0x40000000), r);

	if(add)
		sum = _mm256_add_ps(sum, r);
	else
		sum = r;

	r = _mm256_mul_ps(r, _mm256_set1_ps(536870880.0f)); // 35
	return _mm256_cvttps_epi32(r);
}

template <size_t rot>
inline void double_comupte_wrap(const __m256& n0, const __m256& n1, const __m256& n2, const __m256& n3, 
								float lcnt, float hcnt, const __m256& rnd_c, __m256& sum, __m256i& out)
{
	__m256i r = double_comupte<rot % 2 != 0>(n0, n1, n2, n3, lcnt, hcnt, rnd_c, sum);
	if(rot != 0)
		r = _mm256_or_si256(_mm256_bslli_epi128(r, 16 - rot), _mm256_bsrli_epi128(r, rot));

	out = _mm256_xor_si256(out, r);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::inner_hash_3_avx()
{
	uint32_t s = spad.as_dword(0) >> 8;
	cn_sptr idx0 = scratchpad_ptr(s, 0);
	cn_sptr idx2 = scratchpad_ptr(s, 2);
	__m256 sum0 = _mm256_setzero_ps();

	for(size_t i = 0; i < ITER; i++)
	{
		__m256i v01, v23;
		__m256 suma, sumb, sum1;
		__m256 rc = sum0;

		__m256 n01, n23;
		prep_dv_avx(idx0, v01, n01);
		prep_dv_avx(idx2, v23, n23);

		__m256i out, out2;
		__m256 n10, n22, n33;
		n10 = _mm256_permute2f128_ps(n01, n01, 0x01);
		n22 = _mm256_permute2f128_ps(n23, n23, 0x00);
		n33 = _mm256_permute2f128_ps(n23, n23, 0x11);

		out = _mm256_setzero_si256();
		double_comupte_wrap<0>(n01, n10, n22, n33, 1.3437500f, 1.4296875f, rc, suma, out);
		double_comupte_wrap<1>(n01, n22, n33, n10, 1.2812500f, 1.3984375f, rc, suma, out);
		double_comupte_wrap<2>(n01, n33, n10, n22, 1.3593750f, 1.3828125f, rc, sumb, out);
		double_comupte_wrap<3>(n01, n33, n22, n10, 1.3671875f, 1.3046875f, rc, sumb, out);
		_mm256_store_si256(idx0.as_ptr<__m256i>(), _mm256_xor_si256(v01, out));
		sum0 = _mm256_add_ps(suma, sumb);
		out2 = out;

		__m256 n11, n02, n30;
		n11 = _mm256_permute2f128_ps(n01, n01, 0x11);
		n02 = _mm256_permute2f128_ps(n01, n23, 0x20);
		n30 = _mm256_permute2f128_ps(n01, n23, 0x03);

		out = _mm256_setzero_si256();
		double_comupte_wrap<0>(n23, n11, n02, n30, 1.4140625f, 1.3203125f, rc, suma, out);
		double_comupte_wrap<1>(n23, n02, n30, n11, 1.2734375f, 1.3515625f, rc, suma, out);
		double_comupte_wrap<2>(n23, n30, n11, n02, 1.2578125f, 1.3359375f, rc, sumb, out);
		double_comupte_wrap<3>(n23, n30, n02, n11, 1.2890625f, 1.4609375f, rc, sumb, out);
		_mm256_store_si256(idx2.as_ptr<__m256i>(), _mm256_xor_si256(v23, out));
		sum1 = _mm256_add_ps(suma, sumb);

		out2 = _mm256_xor_si256(out2, out);
		out2 = _mm256_xor_si256(_mm256_permute2x128_si256(out2, out2, 0x41), out2);
		suma = _mm256_permute2f128_ps(sum0, sum1, 0x30);
		sumb = _mm256_permute2f128_ps(sum0, sum1, 0x21);
		sum0 = _mm256_add_ps(suma, sumb);
		sum0 = _mm256_add_ps(sum0, _mm256_permute2f128_ps(sum0, sum0, 0x41));

		// Clear the high 128 bits
		__m128 sum = _mm256_castps256_ps128(sum0);

		sum = _mm_and_ps(_mm_set1_ps_epi32(0x7fffffff), sum); // take abs(va) by masking the float sign bit
		// vs range 0 - 64
		__m128i v0 = _mm_cvttps_epi32(_mm_mul_ps(sum, _mm_set1_ps(16777216.0f)));
		v0 = _mm_xor_si128(v0, _mm256_castsi256_si128(out2));
		__m128i v1 = _mm_shuffle_epi32(v0, _MM_SHUFFLE(0, 1, 2, 3));
		v0 = _mm_xor_si128(v0, v1);
		v1 = _mm_shuffle_epi32(v0, _MM_SHUFFLE(0, 1, 0, 1));
		v0 = _mm_xor_si128(v0, v1);

		// vs is now between 0 and 1
		sum = _mm_div_ps(sum, _mm_set1_ps(64.0f));
		sum0 = _mm256_insertf128_ps(_mm256_castps128_ps256(sum), sum, 1);
		uint32_t n = _mm_cvtsi128_si32(v0);
		idx0 = scratchpad_ptr(n, 0);
		idx2 = scratchpad_ptr(n, 2);
	}
}

template class cn_v1_hash_t;
template class cn_v7l_hash_t;
template class cn_gpu_hash_t;
#endif