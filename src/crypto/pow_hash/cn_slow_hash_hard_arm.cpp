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
	return (saes_sbox[key >> 24] << 24) | (saes_sbox[(key >> 16) & 0xff] << 16) |
		   (saes_sbox[(key >> 8) & 0xff] << 8) | saes_sbox[key & 0xff];
}

inline uint32_t rotr(uint32_t value, uint32_t amount)
{
	return (value >> amount) | (value << ((32 - amount) & 31));
}

template <uint8_t rcon>
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

inline void mem_load(cn_sptr& lpad, size_t i, uint8x16_t& x0, uint8x16_t& x1, uint8x16_t& x2, uint8x16_t& x3, uint8x16_t& x4, uint8x16_t& x5, uint8x16_t& x6, uint8x16_t& x7)
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

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::implode_scratchpad_hard()
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

	for(size_t i = 0; i < MEMORY; i += 128)
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

		if(VERSION == 2)
			xor_shift(x0, x1, x2, x3, x4, x5, x6, x7);
	}

	for(size_t i = 0; VERSION == 2 && i < MEMORY; i += 128)
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

	for(size_t i = 0; VERSION == 2 && i < 16; i++)
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

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::explode_scratchpad_hard()
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

inline uint8x16_t _mm_set_epi64x(const uint64_t a, const uint64_t b)
{
	return vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(b), vcreate_u64(a)));
}

inline void cryptonight_monero_tweak(uint64_t* mem_out, uint8x16_t tmp)
{
	mem_out[0] = vgetq_lane_u64(vreinterpretq_u64_u8(tmp), 0);
	uint64_t vh = vgetq_lane_u64(vreinterpretq_u64_u8(tmp), 1);

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
	uint8x16_t bx0 = _mm_set_epi64x(h0[3] ^ h0[7], h0[2] ^ h0[6]);

	uint64_t idx0 = h0[0] ^ h0[4];

	const uint8x16_t zero = vdupq_n_u8(0);
	// Optim - 90% time boundary
	for(size_t i = 0; i < ITER; i++)
	{
		uint8x16_t cx;
		cx = vld1q_u8(scratchpad_ptr(idx0).as_byte());

		cx = vaesmcq_u8(vaeseq_u8(cx, zero)) ^ _mm_set_epi64x(ah0, al0);

		bx0 ^= cx;
		if(VERSION == 1) 
			cryptonight_monero_tweak(scratchpad_ptr(idx0).template as_ptr<uint64_t>(), bx0); 
		else 
			vst1q_u8(scratchpad_ptr(idx0).as_byte(), bx0);

		idx0 = vgetq_lane_u64(vreinterpretq_u64_u8(cx), 0);
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

#endif // HAS_ARM_HW

#ifdef HAS_ARM

inline void prep_dv(cn_sptr& idx, int32x4_t& v, float32x4_t& n)
{
	v = vld1q_s32((int32_t*)idx.as_void());
	n = vcvtq_f32_s32(v);
}

// 14
inline void sub_round(const float32x4_t& n0, const float32x4_t& n1, const float32x4_t& n2, const float32x4_t& n3,
					  const float32x4_t& rnd_c, float32x4_t& n, float32x4_t& d, float32x4_t& c)
{
	float32x4_t ln1 = vaddq_f32(n1, c);
	float32x4_t nn = vmulq_f32(n0, c);
	nn = vmulq_f32(ln1, vmulq_f32(nn, nn));
	vandq_f32(nn, 0xFEFFFFFF);
	vorq_f32(nn, 0x00800000);
	n = vaddq_f32(n, nn);

	float32x4_t ln3 = vsubq_f32(n3, c);
	float32x4_t dd = vmulq_f32(n2, c);
	dd = vmulq_f32(ln3, vmulq_f32(dd, dd));
	vandq_f32(dd, 0xFEFFFFFF);
	vorq_f32(dd, 0x00800000);
	d = vaddq_f32(d, dd);

	//Constant feedback
	c = vaddq_f32(c, rnd_c);
	c = vaddq_f32(c, vdupq_n_f32(0.734375f));
	float32x4_t r = vaddq_f32(nn, dd);
	vandq_f32(r, 0x807FFFFF);
	vorq_f32(r, 0x40000000);
	c = vaddq_f32(c, r);
}

inline void round_compute(const float32x4_t& n0, const float32x4_t& n1, const float32x4_t& n2, const float32x4_t& n3,
						  const float32x4_t& rnd_c, float32x4_t& c, float32x4_t& r)
{
	float32x4_t n = vdupq_n_f32(0.0f), d = vdupq_n_f32(0.0f);

	sub_round(n0, n1, n2, n3, rnd_c, n, d, c);
	sub_round(n1, n2, n3, n0, rnd_c, n, d, c);
	sub_round(n2, n3, n0, n1, rnd_c, n, d, c);
	sub_round(n3, n0, n1, n2, rnd_c, n, d, c);
	sub_round(n3, n2, n1, n0, rnd_c, n, d, c);
	sub_round(n2, n1, n0, n3, rnd_c, n, d, c);
	sub_round(n1, n0, n3, n2, rnd_c, n, d, c);
	sub_round(n0, n3, n2, n1, rnd_c, n, d, c);

	// Make sure abs(d) > 2.0 - this prevents division by zero and accidental overflows by division by < 1.0
	vandq_f32(d, 0xFF7FFFFF);
	vorq_f32(d, 0x40000000);
	r = vaddq_f32(r, vdivq_f32(n, d));
}

template <bool add>
inline int32x4_t single_comupte(const float32x4_t& n0, const float32x4_t& n1, const float32x4_t& n2, const float32x4_t& n3,
								float cnt, const float32x4_t& rnd_c, float32x4_t& sum)
{
	float32x4_t c = vdupq_n_f32(cnt);
	float32x4_t r = vdupq_n_f32(0.0f);

	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);
	round_compute(n0, n1, n2, n3, rnd_c, c, r);

	// do a quick fmod by setting exp to 2
	vandq_f32(r, 0x807FFFFF);
	vorq_f32(r, 0x40000000);

	if(add)
		sum = vaddq_f32(sum, r);
	else
		sum = r;

	const float32x4_t cc2 = vdupq_n_f32(536870880.0f);
	r = vmulq_f32(r, cc2); // 35
	return vcvtq_s32_f32(r);
}

template <size_t rot>
inline void single_comupte_wrap(const float32x4_t& n0, const float32x4_t& n1, const float32x4_t& n2, const float32x4_t& n3,
								float cnt, const float32x4_t& rnd_c, float32x4_t& sum, int32x4_t& out)
{
	int32x4_t r = single_comupte<rot % 2 != 0>(n0, n1, n2, n3, cnt, rnd_c, sum);
	vrot_si32<rot>(r);
	out = veorq_s32(out, r);
}

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::inner_hash_3()
{
	uint32_t s = spad.as_dword(0) >> 8;
	cn_sptr idx0 = scratchpad_ptr(s, 0);
	cn_sptr idx1 = scratchpad_ptr(s, 1);
	cn_sptr idx2 = scratchpad_ptr(s, 2);
	cn_sptr idx3 = scratchpad_ptr(s, 3);
	float32x4_t sum0 = vdupq_n_f32(0.0f);

	for(size_t i = 0; i < ITER; i++)
	{
		float32x4_t n0, n1, n2, n3;
		int32x4_t v0, v1, v2, v3;
		float32x4_t suma, sumb, sum1, sum2, sum3;

		prep_dv(idx0, v0, n0);
		prep_dv(idx1, v1, n1);
		prep_dv(idx2, v2, n2);
		prep_dv(idx3, v3, n3);
		float32x4_t rc = sum0;

		int32x4_t out, out2;
		out = vdupq_n_s32(0);
		single_comupte_wrap<0>(n0, n1, n2, n3, 1.3437500f, rc, suma, out);
		single_comupte_wrap<1>(n0, n2, n3, n1, 1.2812500f, rc, suma, out);
		single_comupte_wrap<2>(n0, n3, n1, n2, 1.3593750f, rc, sumb, out);
		single_comupte_wrap<3>(n0, n3, n2, n1, 1.3671875f, rc, sumb, out);
		sum0 = vaddq_f32(suma, sumb);
		vst1q_s32((int32_t*)idx0.as_void(), veorq_s32(v0, out));
		out2 = out;

		out = vdupq_n_s32(0);
		single_comupte_wrap<0>(n1, n0, n2, n3, 1.4296875f, rc, suma, out);
		single_comupte_wrap<1>(n1, n2, n3, n0, 1.3984375f, rc, suma, out);
		single_comupte_wrap<2>(n1, n3, n0, n2, 1.3828125f, rc, sumb, out);
		single_comupte_wrap<3>(n1, n3, n2, n0, 1.3046875f, rc, sumb, out);
		sum1 = vaddq_f32(suma, sumb);
		vst1q_s32((int32_t*)idx1.as_void(), veorq_s32(v1, out));
		out2 = veorq_s32(out2, out);

		out = vdupq_n_s32(0);
		single_comupte_wrap<0>(n2, n1, n0, n3, 1.4140625f, rc, suma, out);
		single_comupte_wrap<1>(n2, n0, n3, n1, 1.2734375f, rc, suma, out);
		single_comupte_wrap<2>(n2, n3, n1, n0, 1.2578125f, rc, sumb, out);
		single_comupte_wrap<3>(n2, n3, n0, n1, 1.2890625f, rc, sumb, out);
		sum2 = vaddq_f32(suma, sumb);
		vst1q_s32((int32_t*)idx2.as_void(), veorq_s32(v2, out));
		out2 = veorq_s32(out2, out);

		out = vdupq_n_s32(0);
		single_comupte_wrap<0>(n3, n1, n2, n0, 1.3203125f, rc, suma, out);
		single_comupte_wrap<1>(n3, n2, n0, n1, 1.3515625f, rc, suma, out);
		single_comupte_wrap<2>(n3, n0, n1, n2, 1.3359375f, rc, sumb, out);
		single_comupte_wrap<3>(n3, n0, n2, n1, 1.4609375f, rc, sumb, out);
		sum3 = vaddq_f32(suma, sumb);
		vst1q_s32((int32_t*)idx3.as_void(), veorq_s32(v3, out));
		out2 = veorq_s32(out2, out);
		sum0 = vaddq_f32(sum0, sum1);
		sum2 = vaddq_f32(sum2, sum3);
		sum0 = vaddq_f32(sum0, sum2);

		const float32x4_t cc1 = vdupq_n_f32(16777216.0f);
		const float32x4_t cc2 = vdupq_n_f32(64.0f);
		vandq_f32(sum0, 0x7fffffff); // take abs(va) by masking the float sign bit
		// vs range 0 - 64
		n0 = vmulq_f32(sum0, cc1);
		v0 = vcvtq_s32_f32(n0);
		v0 = veorq_s32(v0, out2);
		uint32_t n = vheor_s32(v0);

		// vs is now between 0 and 1
		sum0 = vdivq_f32(sum0, cc2);
		idx0 = scratchpad_ptr(n, 0);
		idx1 = scratchpad_ptr(n, 1);
		idx2 = scratchpad_ptr(n, 2);
		idx3 = scratchpad_ptr(n, 3);
	}
}

#if defined(__aarch64__)
template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::hardware_hash_3(const void* in, size_t len, void* pout)
{
	keccak((const uint8_t*)in, len, spad.as_byte(), 200);

	explode_scratchpad_3();
	inner_hash_3();
	implode_scratchpad_hard();

	keccakf(spad.as_uqword());
	memcpy(pout, spad.as_byte(), 32);
}
#endif

template <size_t MEMORY, size_t ITER, size_t VERSION>
void cn_slow_hash<MEMORY, ITER, VERSION>::software_hash_3(const void* in, size_t len, void* pout)
{
	keccak((const uint8_t*)in, len, spad.as_byte(), 200);

	explode_scratchpad_3();
	inner_hash_3();
	implode_scratchpad_soft();

	keccakf(spad.as_uqword());
	memcpy(pout, spad.as_byte(), 32);
}

template class cn_v1_hash_t;
template class cn_v7l_hash_t;
template class cn_gpu_hash_t;
#endif