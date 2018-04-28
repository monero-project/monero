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

#ifdef HAS_ARM_HW

extern const uint8_t saes_sbox[256];

struct aeskeydata
{
	uint32_t x0;
	uint32_t x1;
	uint32_t x2;
	uint32_t x3;
	
	aeskeydata(const uint8_t* memory)
	{
		const uint32_t* mem = reinterpret_cast<const uint32_t*>(memory);
		x0 = mem[0];
		x1 = mem[1];
		x2 = mem[2];
		x3 = mem[3];
	}

	inline uint8x16_t store()
	{
		uint32x4_t tmp = {x0, x1, x2, x3};
		return vreinterpretq_u8_u32(tmp);
	}

	inline aeskeydata& operator^=(uint32_t rhs) noexcept
	{
		x0 ^= rhs;
		x1 ^= rhs;
		x2 ^= rhs;
		x3 ^= rhs;
		return *this;
	}
};

// sl_xor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
inline void sl_xor(aeskeydata& x)
{
	x.x1 ^= x.x0;
	x.x2 ^= x.x1;
	x.x3 ^= x.x2;
}

inline uint32_t sub_word(uint32_t key)
{
	return (saes_sbox[key >> 24 ] << 24)   | (saes_sbox[(key >> 16) & 0xff] << 16 ) | 
		(saes_sbox[(key >> 8)  & 0xff] << 8  ) | saes_sbox[key & 0xff];
}

inline uint32_t rotr(uint32_t value, uint32_t amount)
{
	return (value >> amount) | (value << ((32 - amount) & 31));
}

template<uint8_t rcon>
inline void soft_aes_genkey_sub(aeskeydata& xout0, aeskeydata& xout2)
{
	uint32_t tmp;
	sl_xor(xout0);
	xout0 ^= rotr(sub_word(xout2.x3), 8) ^ rcon;
	sl_xor(xout2);
	xout2 ^= sub_word(xout0.x3);
}

inline void aes_genkey(const uint8_t* memory, uint8x16_t& k0, uint8x16_t& k1, uint8x16_t& k2, uint8x16_t& k3, uint8x16_t& k4, 
	uint8x16_t& k5, uint8x16_t& k6, uint8x16_t& k7, uint8x16_t& k8, uint8x16_t& k9)
{
	aeskeydata xout0(memory);
	aeskeydata xout2(memory + 16);

	k0 = xout0.store();
	k1 = xout2.store();

	soft_aes_genkey_sub<0x01>(xout0, xout2);
	k2 = xout0.store();
	k3 = xout2.store();

	soft_aes_genkey_sub<0x02>(xout0, xout2);
	k4 = xout0.store();
	k5 = xout2.store();

	soft_aes_genkey_sub<0x04>(xout0, xout2);
	k6 = xout0.store();
	k7 = xout2.store();

	soft_aes_genkey_sub<0x08>(xout0, xout2);
	k8 = xout0.store();
	k9 = xout2.store();
}

inline void aes_round10(uint8x16_t& x, const uint8x16_t& k0, const uint8x16_t& k1, const uint8x16_t& k2, const uint8x16_t& k3, 
				const uint8x16_t& k4, const uint8x16_t& k5, const uint8x16_t& k6, const uint8x16_t& k7, const uint8x16_t& k8, const uint8x16_t& k9)
{
	x = vaesmcq_u8(vaeseq_u8(x, vdupq_n_u8(0)));
	x = vaesmcq_u8(vaeseq_u8(x, k0));
	x = vaesmcq_u8(vaeseq_u8(x, k1));
	x = vaesmcq_u8(vaeseq_u8(x, k2));
	x = vaesmcq_u8(vaeseq_u8(x, k3));
	x = vaesmcq_u8(vaeseq_u8(x, k4));
	x = vaesmcq_u8(vaeseq_u8(x, k5));
	x = vaesmcq_u8(vaeseq_u8(x, k6));
	x = vaesmcq_u8(vaeseq_u8(x, k7));
	x = vaesmcq_u8(vaeseq_u8(x, k8));
	x = veorq_u8(x, k9);
}

inline void xor_shift(uint8x16_t& x0, uint8x16_t& x1, uint8x16_t& x2, uint8x16_t& x3, uint8x16_t& x4, uint8x16_t& x5, uint8x16_t& x6, uint8x16_t& x7)
{
	uint8x16_t tmp0 = x0;
	x0 ^= x1;
	x1 ^= x2;
	x2 ^= x3;
	x3 ^= x4;
	x4 ^= x5;
	x5 ^= x6;
	x6 ^= x7;
	x7 ^= tmp0;
}

inline void mem_load(cn_sptr& lpad, size_t i,uint8x16_t& x0, uint8x16_t& x1, uint8x16_t& x2, uint8x16_t& x3, uint8x16_t& x4, uint8x16_t& x5, uint8x16_t& x6, uint8x16_t& x7)
{
	x0 ^= vld1q_u8(lpad.as_byte() + i);
	x1 ^= vld1q_u8(lpad.as_byte() + i + 16);
	x2 ^= vld1q_u8(lpad.as_byte() + i + 32);
	x3 ^= vld1q_u8(lpad.as_byte() + i + 48);
	x4 ^= vld1q_u8(lpad.as_byte() + i + 64);
	x5 ^= vld1q_u8(lpad.as_byte() + i + 80);
	x6 ^= vld1q_u8(lpad.as_byte() + i + 96);
	x7 ^= vld1q_u8(lpad.as_byte() + i + 112);
}

template<size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY,ITER,VERSION>::implode_scratchpad_hard()
{
	uint8x16_t x0, x1, x2, x3, x4, x5, x6, x7;
	uint8x16_t k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey(spad.as_byte() + 32, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = vld1q_u8(spad.as_byte() + 64);
	x1 = vld1q_u8(spad.as_byte() + 80);
	x2 = vld1q_u8(spad.as_byte() + 96);
	x3 = vld1q_u8(spad.as_byte() + 112);
	x4 = vld1q_u8(spad.as_byte() + 128);
	x5 = vld1q_u8(spad.as_byte() + 144);
	x6 = vld1q_u8(spad.as_byte() + 160);
	x7 = vld1q_u8(spad.as_byte() + 176);

	for (size_t i = 0; i < MEMORY; i += 128)
	{
		mem_load(lpad, i, x0, x1, x2, x3, x4, x5, x6, x7);

		aes_round10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

		if(VERSION > 0)
			xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for (size_t i = 0; VERSION > 0 && i < MEMORY; i += 128)
	{
		mem_load(lpad, i, x0, x1, x2, x3, x4, x5, x6, x7);

		aes_round10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

		xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for (size_t i = 0; VERSION > 0 && i < 16; i++)
	{
		aes_round10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

		xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	vst1q_u8(spad.as_byte() + 64, x0);
	vst1q_u8(spad.as_byte() + 80, x1);
	vst1q_u8(spad.as_byte() + 96, x2);
	vst1q_u8(spad.as_byte() + 112, x3);
	vst1q_u8(spad.as_byte() + 128, x4);
	vst1q_u8(spad.as_byte() + 144, x5);
	vst1q_u8(spad.as_byte() + 160, x6);
	vst1q_u8(spad.as_byte() + 176, x7);
}

template<size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY,ITER,VERSION>::explode_scratchpad_hard()
{
	uint8x16_t x0, x1, x2, x3, x4, x5, x6, x7;
	uint8x16_t k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

	aes_genkey(spad.as_byte(), k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

	x0 = vld1q_u8(spad.as_byte() + 64);
	x1 = vld1q_u8(spad.as_byte() + 80);
	x2 = vld1q_u8(spad.as_byte() + 96);
	x3 = vld1q_u8(spad.as_byte() + 112);
	x4 = vld1q_u8(spad.as_byte() + 128);
	x5 = vld1q_u8(spad.as_byte() + 144);
	x6 = vld1q_u8(spad.as_byte() + 160);
	x7 = vld1q_u8(spad.as_byte() + 176);

	for (size_t i = 0; VERSION > 0 && i < 16; i++)
	{
		aes_round10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

		xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for(size_t i = 0; i < MEMORY; i += 128)
	{
		aes_round10(x0, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x1, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x2, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x3, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x4, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x5, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x6, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
		aes_round10(x7, k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);

		vst1q_u8(lpad.as_byte() + i, x0);
		vst1q_u8(lpad.as_byte() + i + 16, x1);
		vst1q_u8(lpad.as_byte() + i + 32, x2);
		vst1q_u8(lpad.as_byte() + i + 48, x3);
		vst1q_u8(lpad.as_byte() + i + 64, x4);
		vst1q_u8(lpad.as_byte() + i + 80, x5);
		vst1q_u8(lpad.as_byte() + i + 96, x6);
		vst1q_u8(lpad.as_byte() + i + 112, x7);
	}
}

inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
	unsigned __int128 r = (unsigned __int128)a * (unsigned __int128)b;
	*hi = r >> 64;
	return (uint64_t)r;
}

extern "C" void blake256_hash(uint8_t*, const uint8_t*, uint64_t);
extern "C" void groestl(const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t jh_hash(int, const unsigned char*, unsigned long long, unsigned char*);
extern "C" size_t skein_hash(int, const unsigned char*, size_t, unsigned char*);

inline uint8x16_t _mm_set_epi64x(const uint64_t a, const uint64_t b)
{
    return vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(b), vcreate_u64(a)));
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
	uint8x16_t bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);

	uint64_t idx0 = h0[0] ^ h0[4];

	const uint8x16_t zero = vdupq_n_u8(0);
	// Optim - 90% time boundary
	for(size_t i = 0; i < ITER; i++)
	{
		uint8x16_t cx;
		cx = vld1q_u8(scratchpad_ptr(idx0).as_byte());

		cx = vaesmcq_u8(vaeseq_u8(cx, zero)) ^ _mm_set_epi64x(ah0, al0);

		vst1q_u8(scratchpad_ptr(idx0).as_byte(), bx0 ^ cx);

		idx0 = vgetq_lane_u64(vreinterpretq_u64_u8(cx), 0);
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
