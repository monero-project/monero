// Copyright (c) 2019 The Circle Foundation
// Copyright (c) 2019 fireice-uk
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string.h>
#include <fenv.h>


#include "hash.h"
#include "cn_aux.hpp"
#include "aux_hash.h"

#if !defined(_LP64) && !defined(_WIN64)
#error You are trying to do a 32-bit build. This will all end in tears. I know it.
#endif

namespace crypto {
	  extern "C" {
		  #include "keccak.h"
	  }
	  
	void cn_conceal_slow_hash_v0(const void *data, size_t length, char *hash);

  #define CN_PAGE_SIZE                    2097152
  class cn_context {
	public:

    cn_context()
    {
        long_state = (uint8_t*)boost::alignment::aligned_alloc(4096, CN_PAGE_SIZE);
        hash_state = (uint8_t*)boost::alignment::aligned_alloc(4096, 4096);
    }

    ~cn_context()
    {
        if(long_state != nullptr)
            boost::alignment::aligned_free(long_state);
        if(hash_state != nullptr)
            boost::alignment::aligned_free(hash_state);
    }

    cn_context(const cn_context &) = delete;
    void operator=(const cn_context &) = delete;

     uint8_t* long_state = nullptr;
     uint8_t* hash_state = nullptr;
  };
  
template<bool SOFT_AES>
inline void aes_genkey(const __m128i* memory, __m128i& k0, __m128i& k1, __m128i& k2, __m128i& k3,
	__m128i& k4, __m128i& k5, __m128i& k6, __m128i& k7, __m128i& k8, __m128i& k9)
{
	__m128i xout0, xout1, xout2;

	xout0 = _mm_load_si128(memory);
	xout2 = _mm_load_si128(memory+1);
	k0 = xout0;
	k1 = xout2;

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x01>(xout2);
	else
		xout1 = _mm_aeskeygenassist_si128(xout2, 0x01);

	xout1 = _mm_shuffle_epi32(xout1, 0xFF); // see PSHUFD, set all elems to 4th elem
	xout0 = sl_xor(xout0);
	xout0 = _mm_xor_si128(xout0, xout1);

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x00>(xout0);
	else
		xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);

	xout1 = _mm_shuffle_epi32(xout1, 0xAA); // see PSHUFD, set all elems to 3rd elem
	xout2 = sl_xor(xout2);
	xout2 = _mm_xor_si128(xout2, xout1);
	k2 = xout0;
	k3 = xout2;

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x02>(xout2);
	else
		xout1 = _mm_aeskeygenassist_si128(xout2, 0x02);

	xout1 = _mm_shuffle_epi32(xout1, 0xFF);
	xout0 = sl_xor(xout0);
	xout0 = _mm_xor_si128(xout0, xout1);

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x00>(xout0);
	else
		xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);

	xout1 = _mm_shuffle_epi32(xout1, 0xAA);
	xout2 = sl_xor(xout2);
	xout2 = _mm_xor_si128(xout2, xout1);
	k4 = xout0;
	k5 = xout2;

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x04>(xout2);
	else
		xout1 = _mm_aeskeygenassist_si128(xout2, 0x04);

	xout1 = _mm_shuffle_epi32(xout1, 0xFF);
	xout0 = sl_xor(xout0);
	xout0 = _mm_xor_si128(xout0, xout1);

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x00>(xout0);
	else
		xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);

	xout1 = _mm_shuffle_epi32(xout1, 0xAA);
	xout2 = sl_xor(xout2);
	xout2 = _mm_xor_si128(xout2, xout1);
	k6 = xout0;
	k7 = xout2;

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x08>(xout2);
	else
		xout1 = _mm_aeskeygenassist_si128(xout2, 0x08);

	xout1 = _mm_shuffle_epi32(xout1, 0xFF);
	xout0 = sl_xor(xout0);
	xout0 = _mm_xor_si128(xout0, xout1);

	if(SOFT_AES)
		xout1 = soft_aeskeygenassist<0x00>(xout0);
	else
		xout1 = _mm_aeskeygenassist_si128(xout0, 0x00);

	xout1 = _mm_shuffle_epi32(xout1, 0xAA);
	xout2 = sl_xor(xout2);
	xout2 = _mm_xor_si128(xout2, xout1);
	k8 = xout0;
	k9 = xout2;
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

template<bool SOFT_AES, size_t MEMORY, cryptonight_algo ALGO>
void cn_explode_scratchpad(const __m128i* input, __m128i* output)
{
	// This is more than we have registers, compiler will assign 2 keys on the stack
	__m128i xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey<SOFT_AES>(input, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	xin0 = _mm_load_si128(input + 4);
	xin1 = _mm_load_si128(input + 5);
	xin2 = _mm_load_si128(input + 6);
	xin3 = _mm_load_si128(input + 7);
	xin4 = _mm_load_si128(input + 8);
	xin5 = _mm_load_si128(input + 9);
	xin6 = _mm_load_si128(input + 10);
	xin7 = _mm_load_si128(input + 11);

	for (size_t i = 0; i < MEMORY / sizeof(__m128i); i += 8)
	{
		aes_round<SOFT_AES>(k0, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k1, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k2, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k3, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k4, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k5, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k6, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k7, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k8, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
		aes_round<SOFT_AES>(k9, xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);

		_mm_store_si128(output + i + 0, xin0);
		_mm_store_si128(output + i + 1, xin1);
		_mm_store_si128(output + i + 2, xin2);
		_mm_store_si128(output + i + 3, xin3);
		_mm_store_si128(output + i + 4, xin4);
		_mm_store_si128(output + i + 5, xin5);
		_mm_store_si128(output + i + 6, xin6);
		_mm_store_si128(output + i + 7, xin7);
	}
}

template<bool SOFT_AES, size_t MEMORY, cryptonight_algo ALGO>
void cn_implode_scratchpad(const __m128i* input, __m128i* output)
{
	// This is more than we have registers, compiler will assign 2 keys on the stack
	__m128i xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7;
	__m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey<SOFT_AES>(output + 2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	xout0 = _mm_load_si128(output + 4);
	xout1 = _mm_load_si128(output + 5);
	xout2 = _mm_load_si128(output + 6);
	xout3 = _mm_load_si128(output + 7);
	xout4 = _mm_load_si128(output + 8);
	xout5 = _mm_load_si128(output + 9);
	xout6 = _mm_load_si128(output + 10);
	xout7 = _mm_load_si128(output + 11);

	for (size_t i = 0; i < MEMORY / sizeof(__m128i); i += 8)
	{
		xout0 = _mm_xor_si128(_mm_load_si128(input + i + 0), xout0);
		xout1 = _mm_xor_si128(_mm_load_si128(input + i + 1), xout1);
		xout2 = _mm_xor_si128(_mm_load_si128(input + i + 2), xout2);
		xout3 = _mm_xor_si128(_mm_load_si128(input + i + 3), xout3);
		xout4 = _mm_xor_si128(_mm_load_si128(input + i + 4), xout4);
		xout5 = _mm_xor_si128(_mm_load_si128(input + i + 5), xout5);
		xout6 = _mm_xor_si128(_mm_load_si128(input + i + 6), xout6);
		xout7 = _mm_xor_si128(_mm_load_si128(input + i + 7), xout7);

		aes_round<SOFT_AES>(k0, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k1, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k2, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k3, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k4, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k5, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k6, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k7, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k8, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
		aes_round<SOFT_AES>(k9, xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
	}

	_mm_store_si128(output + 4, xout0);
	_mm_store_si128(output + 5, xout1);
	_mm_store_si128(output + 6, xout2);
	_mm_store_si128(output + 7, xout3);
	_mm_store_si128(output + 8, xout4);
	_mm_store_si128(output + 9, xout5);
	_mm_store_si128(output + 10, xout6);
	_mm_store_si128(output + 11, xout7);
}

inline __m128 _mm_set1_ps_epi32(uint32_t x)
{
	return _mm_castsi128_ps(_mm_set1_epi32(x));
}

template<bool SOFT_AES, cryptonight_algo ALGO>
void cryptonight_hash(const void* input, size_t len, void* output)
{
	constexpr size_t MEMORY = cn_select_memory<ALGO>();
	constexpr uint32_t MASK = cn_select_mask<ALGO>();
	constexpr uint32_t ITER = cn_select_iter<ALGO>();
	constexpr bool CONC_VARIANT = ALGO == CRYPTONIGHT_CONCEAL;
    cn_context ctx0;

	keccak((const uint8_t *)input, len, ctx0.hash_state, 200);

	uint64_t mc0;


	// Optim - 99% time boundary
	cn_explode_scratchpad<SOFT_AES, MEMORY,ALGO>((__m128i*)ctx0.hash_state, (__m128i*)ctx0.long_state);

	uint8_t* l0 = ctx0.long_state;
	uint64_t* h0 = (uint64_t*)ctx0.hash_state;

	uint64_t al0 = h0[0] ^ h0[4];
	uint64_t ah0 = h0[1] ^ h0[5];
	__m128i bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);
	__m128 conc_var = _mm_setzero_ps();

	uint64_t idx0 = h0[0] ^ h0[4];
	// Optim - 90% time boundary
	for(size_t i = 0; i < ITER; i++)
	{
		__m128i cx;
		cx = _mm_load_si128((__m128i *)&l0[idx0 & MASK]);

		if(CONC_VARIANT)
		{
			__m128 r = _mm_cvtepi32_ps(cx);
			__m128 c_old = conc_var;
			r = _mm_add_ps(r, conc_var);
			r = _mm_mul_ps(r, _mm_mul_ps(r, r));
			r = _mm_and_ps(_mm_set1_ps_epi32(0x807FFFFF), r);
			r = _mm_or_ps(_mm_set1_ps_epi32(0x40000000), r);
			conc_var = _mm_add_ps(conc_var, r);

			c_old = _mm_and_ps(_mm_set1_ps_epi32(0x807FFFFF), c_old);
			c_old = _mm_or_ps(_mm_set1_ps_epi32(0x40000000), c_old);
			__m128 nc = _mm_mul_ps(c_old, _mm_set1_ps(536870880.0f));
			cx = _mm_xor_si128(cx, _mm_cvttps_epi32(nc));
		}

		if(SOFT_AES)
			cx = soft_aesenc(cx, _mm_set_epi64x(ah0, al0));
		else
			cx = _mm_aesenc_si128(cx, _mm_set_epi64x(ah0, al0));

		_mm_store_si128((__m128i *)&l0[idx0 & MASK], _mm_xor_si128(bx0, cx));

		idx0 = _mm_cvtsi128_si64(cx);
		bx0 = cx;

		uint64_t hi, lo, cl, ch;
		cl = ((uint64_t*)&l0[idx0 & MASK])[0];
		ch = ((uint64_t*)&l0[idx0 & MASK])[1];

		lo = _umul128(idx0, cl, &hi);
		al0 += hi;
		ah0 += lo;

		((uint64_t*)&l0[idx0 & MASK])[0] = al0;
		((uint64_t*)&l0[idx0 & MASK])[1] = ah0;

		ah0 ^= ch;
		al0 ^= cl;
		idx0 = al0;
	}

	// Optim - 90% time boundary
	cn_implode_scratchpad<SOFT_AES, MEMORY,ALGO>((__m128i*)ctx0.long_state, (__m128i*)ctx0.hash_state);

	// Optim - 99% time boundary

	keccakf((uint64_t*)ctx0.hash_state, 24);

	switch(ctx0.hash_state[0] & 3)
	{
	case 0:
		conceal_blake256_hash(ctx0.hash_state, (uint8_t*)output);
		break;
	case 1:
		conceal_groestl_hash(ctx0.hash_state, (uint8_t*)output);
		break;
	case 2:
		conceal_jh_hash(ctx0.hash_state, (uint8_t*)output);
		break;
	case 3:
		conceal_skein_hash(ctx0.hash_state, (uint8_t*)output);
		break;
	}
}

}