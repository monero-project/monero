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

#include "../keccak.h"
#include "aux_hash.h"
#include "cn_slow_hash.hpp"

#ifdef HAS_INTEL_HW
// sl_xor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
inline __m128i sl_xor(__m128i tmp1)
{
	__m128i tmp4;
	tmp4 = _mm_slli_si128(tmp1, 0x04);
	tmp1 = _mm_xor_si128(tmp1, tmp4);
	tmp4 = _mm_slli_si128(tmp4, 0x04);
	tmp1 = _mm_xor_si128(tmp1, tmp4);
	tmp4 = _mm_slli_si128(tmp4, 0x04);
	tmp1 = _mm_xor_si128(tmp1, tmp4);
	return tmp1;
}

template <uint8_t rcon>
inline void aes_genkey_sub(__m128i& xout0, __m128i& xout2)
{
	__m128i xout1 = _mm_aeskeygenassist_si128(xout2, rcon);
	xout1 = _mm_shuffle_epi32(xout1, 0xFF);
	xout0 = sl_xor(xout0);
	xout0 = _mm_xor_si128(xout0, xout1);
	xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);
	xout1 = _mm_shuffle_epi32(xout1, 0xAA);
	xout2 = sl_xor(xout2);
	xout2 = _mm_xor_si128(xout2, xout1);
}

inline void aes_genkey(const __m128i* memory, __m128i& k0, __m128i& k1, __m128i& k2, __m128i& k3, __m128i& k4,
					   __m128i& k5, __m128i& k6, __m128i& k7, __m128i& k8, __m128i& k9)
{
	__m128i xout0, xout2;

	xout0 = _mm_load_si128(memory);
	xout2 = _mm_load_si128(memory + 1);
	k0 = xout0;
	k1 = xout2;

	aes_genkey_sub<0x01>(xout0, xout2);
	k2 = xout0;
	k3 = xout2;

	aes_genkey_sub<0x02>(xout0, xout2);
	k4 = xout0;
	k5 = xout2;

	aes_genkey_sub<0x04>(xout0, xout2);
	k6 = xout0;
	k7 = xout2;

	aes_genkey_sub<0x08>(xout0, xout2);
	k8 = xout0;
	k9 = xout2;
}

inline void aes_round8(const __m128i& key, __m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3, __m128i& x4, __m128i& x5, __m128i& x6, __m128i& x7)
{
	x0 = _mm_aesenc_si128(x0, key);
	x1 = _mm_aesenc_si128(x1, key);
	x2 = _mm_aesenc_si128(x2, key);
	x3 = _mm_aesenc_si128(x3, key);
	x4 = _mm_aesenc_si128(x4, key);
	x5 = _mm_aesenc_si128(x5, key);
	x6 = _mm_aesenc_si128(x6, key);
	x7 = _mm_aesenc_si128(x7, key);
}

inline void xor_shift(__m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3, __m128i& x4, __m128i& x5, __m128i& x6, __m128i& x7)
{
	__m128i tmp0 = x0;
	x0 = _mm_xor_si128(x0, x1);
	x1 = _mm_xor_si128(x1, x2);
	x2 = _mm_xor_si128(x2, x3);
	x3 = _mm_xor_si128(x3, x4);
	x4 = _mm_xor_si128(x4, x5);
	x5 = _mm_xor_si128(x5, x6);
	x6 = _mm_xor_si128(x6, x7);
	x7 = _mm_xor_si128(x7, tmp0);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::implode_scratchpad_hard()
{
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey(spad.as_ptr<__m128i>() + 2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = _mm_load_si128(spad.as_ptr<__m128i>() + 4);
	x1 = _mm_load_si128(spad.as_ptr<__m128i>() + 5);
	x2 = _mm_load_si128(spad.as_ptr<__m128i>() + 6);
	x3 = _mm_load_si128(spad.as_ptr<__m128i>() + 7);
	x4 = _mm_load_si128(spad.as_ptr<__m128i>() + 8);
	x5 = _mm_load_si128(spad.as_ptr<__m128i>() + 9);
	x6 = _mm_load_si128(spad.as_ptr<__m128i>() + 10);
	x7 = _mm_load_si128(spad.as_ptr<__m128i>() + 11);

	for(size_t i = 0; i < MEMORY / sizeof(__m128i); i += 8)
	{
		x0 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 0), x0);
		x1 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 1), x1);
		x2 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 2), x2);
		x3 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 3), x3);
		x4 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 4), x4);
		x5 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 5), x5);
		x6 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 6), x6);
		x7 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 7), x7);
		
		aes_round8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

		if(VERSION == 2)
			xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for(size_t i = 0; VERSION == 2 && i < MEMORY / sizeof(__m128i); i += 8)
	{
		x0 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 0), x0);
		x1 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 1), x1);
		x2 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 2), x2);
		x3 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 3), x3);
		x4 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 4), x4);
		x5 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 5), x5);
		x6 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 6), x6);
		x7 = _mm_xor_si128(_mm_load_si128(lpad.as_ptr<__m128i>() + i + 7), x7);

		aes_round8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k9, x0, x1, x2, x3, x4, x5, x6, x7);
		
		xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for(size_t i = 0; VERSION == 2 && i < 16; i++)
	{
		aes_round8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

		xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	_mm_store_si128(spad.as_ptr<__m128i>() + 4, x0);
	_mm_store_si128(spad.as_ptr<__m128i>() + 5, x1);
	_mm_store_si128(spad.as_ptr<__m128i>() + 6, x2);
	_mm_store_si128(spad.as_ptr<__m128i>() + 7, x3);
	_mm_store_si128(spad.as_ptr<__m128i>() + 8, x4);
	_mm_store_si128(spad.as_ptr<__m128i>() + 9, x5);
	_mm_store_si128(spad.as_ptr<__m128i>() + 10, x6);
	_mm_store_si128(spad.as_ptr<__m128i>() + 11, x7);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::explode_scratchpad_hard()
{
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey(spad.as_ptr<__m128i>(), k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = _mm_load_si128(spad.as_ptr<__m128i>() + 4);
	x1 = _mm_load_si128(spad.as_ptr<__m128i>() + 5);
	x2 = _mm_load_si128(spad.as_ptr<__m128i>() + 6);
	x3 = _mm_load_si128(spad.as_ptr<__m128i>() + 7);
	x4 = _mm_load_si128(spad.as_ptr<__m128i>() + 8);
	x5 = _mm_load_si128(spad.as_ptr<__m128i>() + 9);
	x6 = _mm_load_si128(spad.as_ptr<__m128i>() + 10);
	x7 = _mm_load_si128(spad.as_ptr<__m128i>() + 11);

	for(size_t i = 0; i < MEMORY / sizeof(__m128i); i += 8)
	{
		aes_round8(k0, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k1, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k2, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k3, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k4, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k5, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k6, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k7, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k8, x0, x1, x2, x3, x4, x5, x6, x7);
		aes_round8(k9, x0, x1, x2, x3, x4, x5, x6, x7);

		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 0, x0);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 1, x1);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 2, x2);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 3, x3);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 4, x4);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 5, x5);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 6, x6);
		_mm_store_si128(lpad.as_ptr<__m128i>() + i + 7, x7);
	}
}

#ifdef BUILD32
inline uint64_t _umul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi)
{
	// multiplier   = ab = a * 2^32 + b
	// multiplicand = cd = c * 2^32 + d
	// ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
	uint64_t a = multiplier >> 32;
	uint64_t b = multiplier & 0xFFFFFFFF;
	uint64_t c = multiplicand >> 32;
	uint64_t d = multiplicand & 0xFFFFFFFF;

	uint64_t ac = a * c;
	uint64_t ad = a * d;
	uint64_t bc = b * c;
	uint64_t bd = b * d;

	uint64_t adbc = ad + bc;
	uint64_t adbc_carry = adbc < ad ? 1 : 0;

	// multiplier * multiplicand = product_hi * 2^64 + product_lo
	uint64_t product_lo = bd + (adbc << 32);
	uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
	*product_hi = ac + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

	return product_lo;
}
#else
#if !defined(HAS_WIN_INTRIN_API)
inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
	unsigned __int128 r = (unsigned __int128)a * (unsigned __int128)b;
	*hi = r >> 64;
	return (uint64_t)r;
}
#endif
#endif

inline uint64_t xmm_extract_64(__m128i x)
{
#ifdef BUILD32
	uint64_t r = uint32_t(_mm_cvtsi128_si32(_mm_shuffle_epi32(x, _MM_SHUFFLE(1, 1, 1, 1))));
	r <<= 32;
	r |= uint32_t(_mm_cvtsi128_si32(x));
	return r;
#else
	return _mm_cvtsi128_si64(x);
#endif
}

inline void cryptonight_monero_tweak(uint64_t* mem_out, __m128i tmp)
{
	mem_out[0] = xmm_extract_64(tmp);
	
	tmp = (__m128i)_mm_movehl_ps((__m128)tmp, (__m128)tmp);
	uint64_t vh = xmm_extract_64(tmp);
	
	uint8_t x = vh >> 24;
	static const uint16_t table = 0x7531;
	const uint8_t index = (((x >> 3) & 6) | (x & 1)) << 1;
	vh ^= ((table >> index) & 0x3) << 28;
	
	mem_out[1] = vh;
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::hardware_hash(const void* in, size_t len, void* out)
{
	if(VERSION == 1 && len < 43)
	{
		memset(out, 0, 32);
		return;
	}

	keccak((const uint8_t*)in, len, spad.as_byte(), 200);

	uint64_t mc0;
	if(VERSION == 1)
	{
		mc0  =  *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(in) + 35);
		mc0 ^=  *(spad.as_uqword()+24);
	}

	explode_scratchpad_hard();

	uint64_t* h0 = spad.as_uqword();

	uint64_t al0 = h0[0] ^ h0[4];
	uint64_t ah0 = h0[1] ^ h0[5];
	__m128i bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);

	uint64_t idx0 = h0[0] ^ h0[4];

	// Optim - 90% time boundary
	for(size_t i = 0; i < ITER; i++)
	{
		__m128i cx;
		cx = _mm_load_si128(scratchpad_ptr(idx0).template as_ptr<__m128i>());

		cx = _mm_aesenc_si128(cx, _mm_set_epi64x(ah0, al0));

		if(VERSION == 1)
			cryptonight_monero_tweak(scratchpad_ptr(idx0).template as_ptr<uint64_t>(), _mm_xor_si128(bx0, cx));
		else
			_mm_store_si128(scratchpad_ptr(idx0).template as_ptr<__m128i>(), _mm_xor_si128(bx0, cx));

		idx0 = xmm_extract_64(cx);
		bx0 = cx;

		uint64_t hi, lo, cl, ch;
		cl = scratchpad_ptr(idx0).as_uqword(0);
		ch = scratchpad_ptr(idx0).as_uqword(1);

		lo = _umul128(idx0, cl, &hi);

		al0 += hi;
		ah0 += lo;
		scratchpad_ptr(idx0).as_uqword(0) = al0;

		if(VERSION == 1)
			scratchpad_ptr(idx0).as_uqword(1) = ah0 ^ mc0;
		else
			scratchpad_ptr(idx0).as_uqword(1) = ah0;

		ah0 ^= ch;
		al0 ^= cl;
		idx0 = al0;
	}

	implode_scratchpad_hard();

	keccakf(spad.as_uqword());

	switch(spad.as_byte(0) & 3)
	{
	case 0:
		blake256_hash(spad.as_byte(), (uint8_t*)out);
		break;
	case 1:
		groestl_hash(spad.as_byte(), (uint8_t*)out);
		break;
	case 2:
		jh_hash(spad.as_byte(), (uint8_t*)out);
		break;
	case 3:
		skein_hash(spad.as_byte(), (uint8_t*)out);
		break;
	}
}

inline void prep_dv(cn_sptr& idx, __m128i& v, __m128& n)
{
	v = _mm_load_si128(idx.as_ptr<__m128i>());
	n = _mm_cvtepi32_ps(v);
}

inline __m128 _mm_set1_ps_epi32(uint32_t x)
{
	return _mm_castsi128_ps(_mm_set1_epi32(x));
}

inline __m128 fma_break(__m128 x)
{
	// Break the dependency chain by setitng the exp to ?????01
	__m128 xx = _mm_and_ps(_mm_set1_ps_epi32(0xFEFFFFFF), x);
	return _mm_or_ps(_mm_set1_ps_epi32(0x00800000), xx);
}

// 14
inline void sub_round(__m128 n0, __m128 n1, __m128 n2, __m128 n3, __m128 rnd_c, __m128& n, __m128& d, __m128& c)
{
	n1 = _mm_add_ps(n1, c);
	__m128 nn = _mm_mul_ps(n0, c);
	nn = _mm_mul_ps(n1, _mm_mul_ps(nn, nn));
	nn = fma_break(nn);
	n = _mm_add_ps(n, nn);

	n3 = _mm_sub_ps(n3, c);
	__m128 dd = _mm_mul_ps(n2, c);
	dd = _mm_mul_ps(n3, _mm_mul_ps(dd, dd));
	dd = fma_break(dd);
	d = _mm_add_ps(d, dd);

	//Constant feedback
	c = _mm_add_ps(c, rnd_c);
	c = _mm_add_ps(c, _mm_set1_ps(0.734375f));
	__m128 r = _mm_add_ps(nn, dd);
	r = _mm_and_ps(_mm_set1_ps_epi32(0x807FFFFF), r);
	r = _mm_or_ps(_mm_set1_ps_epi32(0x40000000), r);
	c = _mm_add_ps(c, r);
}

// 14*8 + 2 = 112
inline void round_compute(__m128 n0, __m128 n1, __m128 n2, __m128 n3, __m128 rnd_c, __m128& c, __m128& r)
{
	__m128 n = _mm_setzero_ps(), d = _mm_setzero_ps();

	sub_round(n0, n1, n2, n3, rnd_c, n, d, c);
	sub_round(n1, n2, n3, n0, rnd_c, n, d, c);
	sub_round(n2, n3, n0, n1, rnd_c, n, d, c);
	sub_round(n3, n0, n1, n2, rnd_c, n, d, c);
	sub_round(n3, n2, n1, n0, rnd_c, n, d, c);
	sub_round(n2, n1, n0, n3, rnd_c, n, d, c);
	sub_round(n1, n0, n3, n2, rnd_c, n, d, c);
	sub_round(n0, n3, n2, n1, rnd_c, n, d, c);

	// Make sure abs(d) > 2.0 - this prevents division by zero and accidental overflows by division by < 1.0
	d = _mm_and_ps(_mm_set1_ps_epi32(0xFF7FFFFF), d);
	d = _mm_or_ps(_mm_set1_ps_epi32(0x40000000), d);
	r = _mm_add_ps(r, _mm_div_ps(n, d));
}

// 112×4 = 448
template <bool add>
inline __m128i single_comupte(__m128 n0, __m128 n1, __m128 n2, __m128 n3, float cnt, __m128 rnd_c, __m128& sum)
{
	__m128 c = _mm_set1_ps(cnt);
	__m128 r = _mm_setzero_ps();

	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);

	// do a quick fmod by setting exp to 2
	r = _mm_and_ps(_mm_set1_ps_epi32(0x807FFFFF), r);
	r = _mm_or_ps(_mm_set1_ps_epi32(0x40000000), r);

	if(add)
		sum = _mm_add_ps(sum, r);
	else
		sum = r;

	r = _mm_mul_ps(r, _mm_set1_ps(536870880.0f)); // 35
	return _mm_cvttps_epi32(r);
}

template <size_t rot>
inline void single_comupte_wrap(__m128 n0, __m128 n1, __m128 n2, __m128 n3, float cnt, __m128 rnd_c, __m128& sum, __m128i& out)
{
	__m128i r = single_comupte<rot % 2 != 0>(n0, n1, n2, n3, cnt, rnd_c, sum);
	if(rot != 0)
		r = _mm_or_si128(_mm_bslli_si128(r, 16 - rot), _mm_bsrli_si128(r, rot));
	out = _mm_xor_si128(out, r);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::inner_hash_3()
{
	uint32_t s = spad.as_dword(0) >> 8;
	cn_sptr idx0 = scratchpad_ptr(s, 0);
	cn_sptr idx1 = scratchpad_ptr(s, 1);
	cn_sptr idx2 = scratchpad_ptr(s, 2);
	cn_sptr idx3 = scratchpad_ptr(s, 3);
	__m128 sum0 = _mm_setzero_ps();

	for(size_t i = 0; i < ITER; i++)
	{
		__m128 n0, n1, n2, n3;
		__m128i v0, v1, v2, v3;
		__m128 suma, sumb, sum1, sum2, sum3;

		prep_dv(idx0, v0, n0);
		prep_dv(idx1, v1, n1);
		prep_dv(idx2, v2, n2);
		prep_dv(idx3, v3, n3);
		__m128 rc = sum0;

		__m128i out, out2;
		out = _mm_setzero_si128();
		single_comupte_wrap<0>(n0, n1, n2, n3, 1.3437500f, rc, suma, out);
		single_comupte_wrap<1>(n0, n2, n3, n1, 1.2812500f, rc, suma, out);
		single_comupte_wrap<2>(n0, n3, n1, n2, 1.3593750f, rc, sumb, out);
		single_comupte_wrap<3>(n0, n3, n2, n1, 1.3671875f, rc, sumb, out);
		sum0 = _mm_add_ps(suma, sumb);
		_mm_store_si128(idx0.as_ptr<__m128i>(), _mm_xor_si128(v0, out));
		out2 = out;

		out = _mm_setzero_si128();
		single_comupte_wrap<0>(n1, n0, n2, n3, 1.4296875f, rc, suma, out);
		single_comupte_wrap<1>(n1, n2, n3, n0, 1.3984375f, rc, suma, out);
		single_comupte_wrap<2>(n1, n3, n0, n2, 1.3828125f, rc, sumb, out);
		single_comupte_wrap<3>(n1, n3, n2, n0, 1.3046875f, rc, sumb, out);
		sum1 = _mm_add_ps(suma, sumb);
		_mm_store_si128(idx1.as_ptr<__m128i>(), _mm_xor_si128(v1, out));
		out2 = _mm_xor_si128(out2, out);

		out = _mm_setzero_si128();
		single_comupte_wrap<0>(n2, n1, n0, n3, 1.4140625f, rc, suma, out);
		single_comupte_wrap<1>(n2, n0, n3, n1, 1.2734375f, rc, suma, out);
		single_comupte_wrap<2>(n2, n3, n1, n0, 1.2578125f, rc, sumb, out);
		single_comupte_wrap<3>(n2, n3, n0, n1, 1.2890625f, rc, sumb, out);
		sum2 = _mm_add_ps(suma, sumb);
		_mm_store_si128(idx2.as_ptr<__m128i>(), _mm_xor_si128(v2, out));
		out2 = _mm_xor_si128(out2, out);

		out = _mm_setzero_si128();
		single_comupte_wrap<0>(n3, n1, n2, n0, 1.3203125f, rc, suma, out);
		single_comupte_wrap<1>(n3, n2, n0, n1, 1.3515625f, rc, suma, out);
		single_comupte_wrap<2>(n3, n0, n1, n2, 1.3359375f, rc, sumb, out);
		single_comupte_wrap<3>(n3, n0, n2, n1, 1.4609375f, rc, sumb, out);
		sum3 = _mm_add_ps(suma, sumb);
		_mm_store_si128(idx3.as_ptr<__m128i>(), _mm_xor_si128(v3, out));
		out2 = _mm_xor_si128(out2, out);
		sum0 = _mm_add_ps(sum0, sum1);
		sum2 = _mm_add_ps(sum2, sum3);
		sum0 = _mm_add_ps(sum0, sum2);

		sum0 = _mm_and_ps(_mm_set1_ps_epi32(0x7fffffff), sum0); // take abs(va) by masking the float sign bit
		// vs range 0 - 64
		n0 = _mm_mul_ps(sum0, _mm_set1_ps(16777216.0f));
		v0 = _mm_cvttps_epi32(n0);
		v0 = _mm_xor_si128(v0, out2);
		v1 = _mm_shuffle_epi32(v0, _MM_SHUFFLE(0, 1, 2, 3));
		v0 = _mm_xor_si128(v0, v1);
		v1 = _mm_shuffle_epi32(v0, _MM_SHUFFLE(0, 1, 0, 1));
		v0 = _mm_xor_si128(v0, v1);

		// vs is now between 0 and 1
		sum0 = _mm_div_ps(sum0, _mm_set1_ps(64.0f));
		uint32_t n = _mm_cvtsi128_si32(v0);
		idx0 = scratchpad_ptr(n, 0);
		idx1 = scratchpad_ptr(n, 1);
		idx2 = scratchpad_ptr(n, 2);
		idx3 = scratchpad_ptr(n, 3);
	}
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::hardware_hash_3(const void* in, size_t len, void* pout)
{
	keccak((const uint8_t*)in, len, spad.as_byte(), 200);

	explode_scratchpad_3();
	if(check_avx2())
		inner_hash_3_avx();
	else
		inner_hash_3();
	implode_scratchpad_hard();

	keccakf(spad.as_uqword());
	memcpy(pout, spad.as_byte(), 32);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::software_hash_3(const void* in, size_t len, void* pout)
{
	keccak((const uint8_t*)in, len, spad.as_byte(), 200);

	explode_scratchpad_3();
	if(check_avx2())
		inner_hash_3_avx();
	else
		inner_hash_3();
	implode_scratchpad_soft();

	keccakf(spad.as_uqword());
	memcpy(pout, spad.as_byte(), 32);
}

template class cn_gpu_hash_t;
template class cn_v1_hash_t;
template class cn_v7l_hash_t;

#endif