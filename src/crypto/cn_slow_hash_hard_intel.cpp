// Copyright (c) 2017, SUMOKOIN
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
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#include "cn_slow_hash.hpp"
extern "C" {
#include "../crypto/keccak.h"
}

#ifdef HAS_INTEL_HW

#if !defined(_LP64) && !defined(_WIN64)
#define BUILD32
#endif

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

template<uint8_t rcon>
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

template<size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY,ITER,VERSION>::implode_scratchpad_hard()
{
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;
	
	aes_genkey(spad.as_xmm() + 2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = _mm_load_si128(spad.as_xmm() + 4);
	x1 = _mm_load_si128(spad.as_xmm() + 5);
	x2 = _mm_load_si128(spad.as_xmm() + 6);
	x3 = _mm_load_si128(spad.as_xmm() + 7);
	x4 = _mm_load_si128(spad.as_xmm() + 8);
	x5 = _mm_load_si128(spad.as_xmm() + 9);
	x6 = _mm_load_si128(spad.as_xmm() + 10);
	x7 = _mm_load_si128(spad.as_xmm() + 11);

	for (size_t i = 0; i < MEMORY / sizeof(__m128i); i +=8)
	{
		x0 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 0), x0);
		x1 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 1), x1);
		x2 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 2), x2);
		x3 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 3), x3);
		x4 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 4), x4);
		x5 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 5), x5);
		x6 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 6), x6);
		x7 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 7), x7);

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

		if(VERSION > 0)
			xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for (size_t i = 0; VERSION > 0 && i < MEMORY / sizeof(__m128i); i +=8)
	{
		x0 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 0), x0);
		x1 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 1), x1);
		x2 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 2), x2);
		x3 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 3), x3);
		x4 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 4), x4);
		x5 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 5), x5);
		x6 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 6), x6);
		x7 = _mm_xor_si128(_mm_load_si128(lpad.as_xmm() + i + 7), x7);

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

	for (size_t i = 0; VERSION > 0 && i < 16; i++)
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

	_mm_store_si128(spad.as_xmm() + 4, x0);
	_mm_store_si128(spad.as_xmm() + 5, x1);
	_mm_store_si128(spad.as_xmm() + 6, x2);
	_mm_store_si128(spad.as_xmm() + 7, x3);
	_mm_store_si128(spad.as_xmm() + 8, x4);
	_mm_store_si128(spad.as_xmm() + 9, x5);
	_mm_store_si128(spad.as_xmm() + 10, x6);
	_mm_store_si128(spad.as_xmm() + 11, x7);
}

template<size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY,ITER,VERSION>::explode_scratchpad_hard()
{
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey(spad.as_xmm(), k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = _mm_load_si128(spad.as_xmm() + 4);
	x1 = _mm_load_si128(spad.as_xmm() + 5);
	x2 = _mm_load_si128(spad.as_xmm() + 6);
	x3 = _mm_load_si128(spad.as_xmm() + 7);
	x4 = _mm_load_si128(spad.as_xmm() + 8);
	x5 = _mm_load_si128(spad.as_xmm() + 9);
	x6 = _mm_load_si128(spad.as_xmm() + 10);
	x7 = _mm_load_si128(spad.as_xmm() + 11);

	for (size_t i = 0; VERSION > 0 && i < 16; i++)
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

		_mm_store_si128(lpad.as_xmm() + i + 0, x0);
		_mm_store_si128(lpad.as_xmm() + i + 1, x1);
		_mm_store_si128(lpad.as_xmm() + i + 2, x2);
		_mm_store_si128(lpad.as_xmm() + i + 3, x3);
		_mm_store_si128(lpad.as_xmm() + i + 4, x4);
		_mm_store_si128(lpad.as_xmm() + i + 5, x5);
		_mm_store_si128(lpad.as_xmm() + i + 6, x6);
		_mm_store_si128(lpad.as_xmm() + i + 7, x7);
	}
}

#ifdef BUILD32
inline uint64_t _umul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi)
{
  // multiplier   = ab = a * 2^32 + b
  // multiplicand = cd = c * 2^32 + d
  // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
  uint64_t a = multiplier >> 32;
  uint64_t b = multiplier  & 0xFFFFFFFF;
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

extern "C" void blake256_hash(uint8_t*, const uint8_t*, uint64_t);
extern "C" void groestl(const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t jh_hash(int, const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t skein_hash(int, const unsigned char*, size_t, unsigned char*);

inline uint64_t xmm_extract_64(__m128i x)
{
#ifdef BUILD32
	uint64_t r = uint32_t(_mm_cvtsi128_si32(_mm_shuffle_epi32(x, _MM_SHUFFLE(1,1,1,1))));
	r <<= 32;
	r |= uint32_t(_mm_cvtsi128_si32(x));
	return r;
#else
	return _mm_cvtsi128_si64(x);
#endif
}

template<size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY,ITER,VERSION>::hardware_hash(const void* in, size_t len, void* out, bool prehashed)
{
	if (!prehashed)
		keccak((const uint8_t *)in, len, spad.as_byte(), 200);

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
		cx = _mm_load_si128(scratchpad_ptr(idx0).as_xmm());

		cx = _mm_aesenc_si128(cx, _mm_set_epi64x(ah0, al0));

		_mm_store_si128(scratchpad_ptr(idx0).as_xmm(), _mm_xor_si128(bx0, cx));
		idx0 = xmm_extract_64(cx);
		bx0 = cx;

		uint64_t hi, lo, cl, ch;
		cl = scratchpad_ptr(idx0).as_uqword(0);
		ch = scratchpad_ptr(idx0).as_uqword(1);

		lo = _umul128(idx0, cl, &hi);

		al0 += hi;
		ah0 += lo;
		scratchpad_ptr(idx0).as_uqword(0) = al0;
		scratchpad_ptr(idx0).as_uqword(1) = ah0;
		ah0 ^= ch;
		al0 ^= cl;
		idx0 = al0;
		
		if(VERSION > 0)
		{
			int64_t n  = scratchpad_ptr(idx0).as_qword(0);
			int32_t d  = scratchpad_ptr(idx0).as_dword(2);
			int64_t q = n / (d | 5);
			scratchpad_ptr(idx0).as_qword(0) = n ^ q;
			idx0 = d ^ q;
		}
	}

	implode_scratchpad_hard();

	keccakf(spad.as_uqword(), 24);

	switch(spad.as_byte(0) & 3)
	{
	case 0:
		blake256_hash((uint8_t*)out, spad.as_byte(), 200);
		break;
	case 1:
		groestl(spad.as_byte(), 200 * 8, (uint8_t*)out);
		break;
	case 2:
		jh_hash(32 * 8, spad.as_byte(), 8 * 200, (uint8_t*)out);
		break;
	case 3:
		skein_hash(8 * 32, spad.as_byte(), 8 * 200, (uint8_t*)out);
		break;
	}
}

template class cn_slow_hash<2*1024*1024, 0x80000, 0>;
template class cn_slow_hash<4*1024*1024, 0x40000, 1>;

#endif
