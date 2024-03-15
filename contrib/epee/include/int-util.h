// Copyright (c) 2014-2024, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef _MSC_VER
#include <sys/param.h>
#endif

#if defined(__ANDROID__)
#include <byteswap.h>
#endif

#if defined(__sun) && defined(__SVR4)
#include <endian.h>
#endif

#if defined(_MSC_VER)
#include <stdlib.h>

static inline uint32_t rol32(uint32_t x, int r) {
  static_assert(sizeof(uint32_t) == sizeof(unsigned int), "this code assumes 32-bit integers");
  return _rotl(x, r);
}

static inline uint64_t rol64(uint64_t x, int r) {
  return _rotl64(x, r);
}

#else

static inline uint32_t rol32(uint32_t x, int r) {
  return (x << (r & 31)) | (x >> (-r & 31));
}

static inline uint64_t rol64(uint64_t x, int r) {
  return (x << (r & 63)) | (x >> (-r & 63));
}

#endif

static inline uint64_t hi_dword(uint64_t val) {
  return val >> 32;
}

static inline uint64_t lo_dword(uint64_t val) {
  return val & 0xFFFFFFFF;
}

static inline uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi) {
  // multiplier   = ab = a * 2^32 + b
  // multiplicand = cd = c * 2^32 + d
  // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
  uint64_t a = hi_dword(multiplier);
  uint64_t b = lo_dword(multiplier);
  uint64_t c = hi_dword(multiplicand);
  uint64_t d = lo_dword(multiplicand);

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
  assert(ac <= *product_hi);

  return product_lo;
}

static inline uint64_t div_with_reminder(uint64_t dividend, uint32_t divisor, uint32_t* remainder) {
  dividend |= ((uint64_t)*remainder) << 32;
  *remainder = dividend % divisor;
  return dividend / divisor;
}

// Long division with 2^32 base
static inline uint32_t div128_32(uint64_t dividend_hi, uint64_t dividend_lo, uint32_t divisor, uint64_t* quotient_hi, uint64_t* quotient_lo) {
  uint64_t dividend_dwords[4];
  uint32_t remainder = 0;

  dividend_dwords[3] = hi_dword(dividend_hi);
  dividend_dwords[2] = lo_dword(dividend_hi);
  dividend_dwords[1] = hi_dword(dividend_lo);
  dividend_dwords[0] = lo_dword(dividend_lo);

  *quotient_hi  = div_with_reminder(dividend_dwords[3], divisor, &remainder) << 32;
  *quotient_hi |= div_with_reminder(dividend_dwords[2], divisor, &remainder);
  *quotient_lo  = div_with_reminder(dividend_dwords[1], divisor, &remainder) << 32;
  *quotient_lo |= div_with_reminder(dividend_dwords[0], divisor, &remainder);

  return remainder;
}

// Long divisor with 2^64 base
void div128_64(uint64_t dividend_hi, uint64_t dividend_lo, uint64_t divisor, uint64_t* quotient_hi, uint64_t *quotient_lo, uint64_t *remainder_hi, uint64_t *remainder_lo);

static inline void add64clamp(uint64_t *value, uint64_t add)
{
  static const uint64_t maxval = (uint64_t)-1;
  if (*value > maxval - add)
    *value = maxval;
  else
    *value += add;
}

static inline void sub64clamp(uint64_t *value, uint64_t sub)
{
  if (*value < sub)
    *value = 0;
  else
    *value -= sub;
}

#define IDENT16(x) ((uint16_t) (x))
#define IDENT32(x) ((uint32_t) (x))
#define IDENT64(x) ((uint64_t) (x))

#define SWAP16(x) ((((uint16_t) (x) & 0x00ff) << 8) | \
  (((uint16_t) (x) & 0xff00) >> 8))
#define SWAP32(x) ((((uint32_t) (x) & 0x000000ff) << 24) | \
  (((uint32_t) (x) & 0x0000ff00) <<  8) | \
  (((uint32_t) (x) & 0x00ff0000) >>  8) | \
  (((uint32_t) (x) & 0xff000000) >> 24))
#define SWAP64(x) ((((uint64_t) (x) & 0x00000000000000ff) << 56) | \
  (((uint64_t) (x) & 0x000000000000ff00) << 40) | \
  (((uint64_t) (x) & 0x0000000000ff0000) << 24) | \
  (((uint64_t) (x) & 0x00000000ff000000) <<  8) | \
  (((uint64_t) (x) & 0x000000ff00000000) >>  8) | \
  (((uint64_t) (x) & 0x0000ff0000000000) >> 24) | \
  (((uint64_t) (x) & 0x00ff000000000000) >> 40) | \
  (((uint64_t) (x) & 0xff00000000000000) >> 56))

static inline uint16_t ident16(uint16_t x) { return x; }
static inline uint32_t ident32(uint32_t x) { return x; }
static inline uint64_t ident64(uint64_t x) { return x; }

#ifndef __OpenBSD__
#  if defined(__ANDROID__) && defined(__swap16) && !defined(swap16)
#      define swap16 __swap16
#  elif !defined(swap16)
static inline uint16_t swap16(uint16_t x) {
  return ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8);
}
#  endif
#  if defined(__ANDROID__) && defined(__swap32) && !defined(swap32)
#      define swap32 __swap32
#  elif !defined(swap32)
static inline uint32_t swap32(uint32_t x) {
  x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
  return (x << 16) | (x >> 16);
}
#  endif
#  if defined(__ANDROID__) && defined(__swap64) && !defined(swap64)
#      define swap64 __swap64
#  elif !defined(swap64)
static inline uint64_t swap64(uint64_t x) {
  x = ((x & 0x00ff00ff00ff00ff) <<  8) | ((x & 0xff00ff00ff00ff00) >>  8);
  x = ((x & 0x0000ffff0000ffff) << 16) | ((x & 0xffff0000ffff0000) >> 16);
  return (x << 32) | (x >> 32);
}
#  endif
#endif /* __OpenBSD__ */

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif
static inline void mem_inplace_ident(void *mem UNUSED, size_t n UNUSED) { }
#undef UNUSED

static inline void mem_inplace_swap16(void *mem, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint16_t *) mem)[i] = swap16(((const uint16_t *) mem)[i]);
  }
}
static inline void mem_inplace_swap32(void *mem, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint32_t *) mem)[i] = swap32(((const uint32_t *) mem)[i]);
  }
}
static inline void mem_inplace_swap64(void *mem, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint64_t *) mem)[i] = swap64(((const uint64_t *) mem)[i]);
  }
}

static inline void memcpy_ident16(void *dst, const void *src, size_t n) {
  memcpy(dst, src, 2 * n);
}
static inline void memcpy_ident32(void *dst, const void *src, size_t n) {
  memcpy(dst, src, 4 * n);
}
static inline void memcpy_ident64(void *dst, const void *src, size_t n) {
  memcpy(dst, src, 8 * n);
}

static inline void memcpy_swap16(void *dst, const void *src, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint16_t *) dst)[i] = swap16(((const uint16_t *) src)[i]);
  }
}
static inline void memcpy_swap32(void *dst, const void *src, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint32_t *) dst)[i] = swap32(((const uint32_t *) src)[i]);
  }
}
static inline void memcpy_swap64(void *dst, const void *src, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((uint64_t *) dst)[i] = swap64(((const uint64_t *) src)[i]);
  }
}

#ifdef _MSC_VER
# define LITTLE_ENDIAN	1234
# define BIG_ENDIAN	4321
# define BYTE_ORDER	LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN)
static_assert(false, "BYTE_ORDER is undefined. Perhaps, GNU extensions are not enabled");
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define SWAP16LE IDENT16
#define SWAP16BE SWAP16
#define swap16le ident16
#define swap16be swap16
#define mem_inplace_swap16le mem_inplace_ident
#define mem_inplace_swap16be mem_inplace_swap16
#define memcpy_swap16le memcpy_ident16
#define memcpy_swap16be memcpy_swap16
#define SWAP32LE IDENT32
#define SWAP32BE SWAP32
#define swap32le ident32
#define swap32be swap32
#define mem_inplace_swap32le mem_inplace_ident
#define mem_inplace_swap32be mem_inplace_swap32
#define memcpy_swap32le memcpy_ident32
#define memcpy_swap32be memcpy_swap32
#define SWAP64LE IDENT64
#define SWAP64BE SWAP64
#define swap64le ident64
#define swap64be swap64
#define mem_inplace_swap64le mem_inplace_ident
#define mem_inplace_swap64be mem_inplace_swap64
#define memcpy_swap64le memcpy_ident64
#define memcpy_swap64be memcpy_swap64
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define SWAP16BE IDENT16
#define SWAP16LE SWAP16
#define swap16be ident16
#define swap16le swap16
#define mem_inplace_swap16be mem_inplace_ident
#define mem_inplace_swap16le mem_inplace_swap16
#define memcpy_swap16be memcpy_ident16
#define memcpy_swap16le memcpy_swap16
#define SWAP32BE IDENT32
#define SWAP32LE SWAP32
#define swap32be ident32
#define swap32le swap32
#define mem_inplace_swap32be mem_inplace_ident
#define mem_inplace_swap32le mem_inplace_swap32
#define memcpy_swap32be memcpy_ident32
#define memcpy_swap32le memcpy_swap32
#define SWAP64BE IDENT64
#define SWAP64LE SWAP64
#define swap64be ident64
#define swap64le swap64
#define mem_inplace_swap64be mem_inplace_ident
#define mem_inplace_swap64le mem_inplace_swap64
#define memcpy_swap64be memcpy_ident64
#define memcpy_swap64le memcpy_swap64
#endif
