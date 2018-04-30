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

#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#include <intrin.h>
#define HAS_WIN_INTRIN_API
#endif

// Note HAS_INTEL_HW and HAS_ARM_HW only mean we can emit the AES instructions
// check CPU support for the hardware AES encryption has to be done at runtime
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X86) || defined(_M_X64)
#ifdef __GNUC__
#include <x86intrin.h>
#pragma GCC target ("aes")
#if !defined(HAS_WIN_INTRIN_API)
#include <cpuid.h>
#endif // !defined(HAS_WIN_INTRIN_API)
#endif // __GNUC__
#define HAS_INTEL_HW
#endif

#if defined(__aarch64__)
#pragma GCC target ("+crypto")
#include <sys/auxv.h>
#include <asm/hwcap.h>
#include <arm_neon.h>
#define HAS_ARM_HW
#endif

#ifdef HAS_INTEL_HW
inline void cpuid(uint32_t eax, int32_t ecx, int32_t val[4])
{
	val[0] = 0;
	val[1] = 0;
	val[2] = 0;
	val[3] = 0;

#if defined(HAS_WIN_INTRIN_API)
	__cpuidex(val, eax, ecx);
#else
	__cpuid_count(eax, ecx, val[0], val[1], val[2], val[3]);
#endif
}

inline bool hw_check_aes()
{
	int32_t cpu_info[4];
	cpuid(1, 0, cpu_info);
	return (cpu_info[2] & (1 << 25)) != 0;
}
#endif

#ifdef HAS_ARM_HW
inline bool hw_check_aes()
{
	return (getauxval(AT_HWCAP) & HWCAP_AES) != 0;
}
#endif

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
inline bool hw_check_aes()
{
	return false;
}
#endif

// This cruft avoids casting-galore and allows us not to worry about sizeof(void*)
class cn_sptr
{
public:
	cn_sptr() : base_ptr(nullptr) {}
	cn_sptr(uint64_t* ptr) { base_ptr = ptr; }
	cn_sptr(uint32_t* ptr) { base_ptr = ptr; }
	cn_sptr(uint8_t* ptr) { base_ptr = ptr; }
#ifdef HAS_INTEL_HW
	cn_sptr(__m128i* ptr) { base_ptr = ptr; }
#endif

	inline void set(void* ptr) { base_ptr = ptr; }
	inline cn_sptr offset(size_t i) { return reinterpret_cast<uint8_t*>(base_ptr)+i; }
	inline const cn_sptr offset(size_t i) const { return reinterpret_cast<uint8_t*>(base_ptr)+i; }

	inline void* as_void() { return base_ptr; }
	inline uint8_t& as_byte(size_t i) { return *(reinterpret_cast<uint8_t*>(base_ptr)+i); }
	inline uint8_t* as_byte() { return reinterpret_cast<uint8_t*>(base_ptr); }
	inline uint64_t& as_uqword(size_t i) { return *(reinterpret_cast<uint64_t*>(base_ptr)+i); }
	inline const uint64_t& as_uqword(size_t i) const { return *(reinterpret_cast<uint64_t*>(base_ptr)+i); } 
	inline uint64_t* as_uqword() { return reinterpret_cast<uint64_t*>(base_ptr); }
	inline const uint64_t* as_uqword() const { return reinterpret_cast<uint64_t*>(base_ptr); }
	inline int64_t& as_qword(size_t i) { return *(reinterpret_cast<int64_t*>(base_ptr)+i); }
	inline int32_t& as_dword(size_t i) { return *(reinterpret_cast<int32_t*>(base_ptr)+i); }
	inline uint32_t& as_udword(size_t i) { return *(reinterpret_cast<uint32_t*>(base_ptr)+i); }
	inline const uint32_t& as_udword(size_t i) const { return *(reinterpret_cast<uint32_t*>(base_ptr)+i); }
#ifdef HAS_INTEL_HW
	inline __m128i* as_xmm() { return reinterpret_cast<__m128i*>(base_ptr); }
#endif
private:
	void* base_ptr;
};

template<size_t MEMORY, size_t ITER, size_t VERSION> class cn_slow_hash;
using cn_pow_hash_v1 = cn_slow_hash<2*1024*1024, 0x80000, 0>;
using cn_pow_hash_v2 = cn_slow_hash<4*1024*1024, 0x40000, 1>;

template<size_t MEMORY, size_t ITER, size_t VERSION>
class cn_slow_hash
{
public:
	cn_slow_hash() : borrowed_pad(false)
	{
#if !defined(HAS_WIN_INTRIN_API)
		lpad.set(aligned_alloc(4096, MEMORY));
		spad.set(aligned_alloc(4096, 4096));
#else
		lpad.set(_aligned_malloc(MEMORY, 4096));
		spad.set(_aligned_malloc(4096, 4096));
#endif
	}

	cn_slow_hash (cn_slow_hash&& other) noexcept : lpad(other.lpad.as_byte()), spad(other.spad.as_byte()), borrowed_pad(other.borrowed_pad)
	{
		other.lpad.set(nullptr);
		other.spad.set(nullptr);
	}

	// Factory function enabling to temporaliy turn v2 object into v1
	// It is caller's responsibility to ensure that v2 object is not hashing at the same time!!
	static cn_pow_hash_v1 make_borrowed(cn_pow_hash_v2& t)
	{
		return cn_pow_hash_v1(t.lpad.as_void(), t.spad.as_void());
	}

	cn_slow_hash& operator= (cn_slow_hash&& other) noexcept
    {
		if(this == &other)
			return *this;

		free_mem();
		lpad.set(other.lpad.as_void());
		spad.set(other.spad.as_void());
		borrowed_pad = other.borrowed_pad;
		return *this;
	}

	// Copying is going to be really inefficient
	cn_slow_hash(const cn_slow_hash& other) = delete;
	cn_slow_hash& operator= (const cn_slow_hash& other) = delete;

	~cn_slow_hash()
	{
		free_mem();
	}

	void hash(const void* in, size_t len, void* out, bool prehashed=false)
	{
		if(hw_check_aes() && !check_override())
			hardware_hash(in, len, out, prehashed);
		else
			software_hash(in, len, out, prehashed);
	}

	void software_hash(const void* in, size_t len, void* out, bool prehashed);
	
#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
	inline void hardware_hash(const void* in, size_t len, void* out, bool prehashed) { assert(false); }
#else
	void hardware_hash(const void* in, size_t len, void* out, bool prehashed);
#endif

private:
	static constexpr size_t MASK = ((MEMORY-1) >> 4) << 4;
	friend cn_pow_hash_v1;
	friend cn_pow_hash_v2;

	// Constructor enabling v1 hash to borrow v2's buffer
	cn_slow_hash(void* lptr, void* sptr)
	{
		lpad.set(lptr);
		spad.set(sptr);
		borrowed_pad = true;
	}

	inline bool check_override()
	{
		const char *env = getenv("LOKI_USE_SOFTWARE_AES");
		if (!env) {
			return false;
		}
		else if (!strcmp(env, "0") || !strcmp(env, "no")) {
			return false;
		}
		else {
			return true;
		}
	}

	inline void free_mem()
	{
		if(!borrowed_pad)
		{
#if !defined(HAS_WIN_INTRIN_API)
			if(lpad.as_void() != nullptr)
				free(lpad.as_void());
			if(lpad.as_void() != nullptr)
				free(spad.as_void());
#else
			if(lpad.as_void() != nullptr)
				_aligned_free(lpad.as_void());
			if(lpad.as_void() != nullptr)
				_aligned_free(spad.as_void());
#endif
		}

		lpad.set(nullptr);
		spad.set(nullptr);
	}

	inline cn_sptr scratchpad_ptr(uint32_t idx) { return lpad.as_byte() + (idx & MASK); }

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
	inline void explode_scratchpad_hard() { assert(false); }
	inline void implode_scratchpad_hard() { assert(false); }
#else
	void explode_scratchpad_hard();
	void implode_scratchpad_hard();
#endif

	void explode_scratchpad_soft();
	void implode_scratchpad_soft();

	cn_sptr lpad;
	cn_sptr spad;
	bool borrowed_pad;
};

extern template class cn_slow_hash<2*1024*1024, 0x80000, 0>;
extern template class cn_slow_hash<4*1024*1024, 0x40000, 1>;
