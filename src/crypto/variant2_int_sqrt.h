#ifndef VARIANT2_INT_SQRT_H
#define VARIANT2_INT_SQRT_H

#include <math.h>
#include <float.h>

#define VARIANT2_INTEGER_MATH_SQRT_STEP_SSE2() \
  do { \
    const __m128i exp_double_bias = _mm_set_epi64x(0, 1023ULL << 52); \
    __m128d x = _mm_castsi128_pd(_mm_add_epi64(_mm_cvtsi64_si128(sqrt_input >> 12), exp_double_bias)); \
    x = _mm_sqrt_sd(_mm_setzero_pd(), x); \
    sqrt_result = (uint64_t)(_mm_cvtsi128_si64(_mm_sub_epi64(_mm_castpd_si128(x), exp_double_bias))) >> 19; \
  } while(0)

#define VARIANT2_INTEGER_MATH_SQRT_STEP_FP64() \
  do { \
    sqrt_result = sqrt(sqrt_input + 18446744073709551616.0) * 2.0 - 8589934592.0; \
  } while(0)

#define VARIANT2_INTEGER_MATH_SQRT_STEP_REF() \
  sqrt_result = integer_square_root_v2(sqrt_input)

// Reference implementation of the integer square root for Cryptonight variant 2
// Computes integer part of "sqrt(2^64 + n) * 2 - 2^33"
//
// In other words, given 64-bit unsigned integer n:
// 1) Write it as x = 1.NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN000... in binary (1 <= x < 2, all 64 bits of n are used)
// 2) Calculate sqrt(x) = 1.0RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR... (1 <= sqrt(x) < sqrt(2), so it will always start with "1.0" in binary)
// 3) Take 32 bits that come after "1.0" and return them as a 32-bit unsigned integer, discard all remaining bits
//
// Some sample inputs and outputs:
//
// Input            | Output     | Exact value of "sqrt(2^64 + n) * 2 - 2^33"
// -----------------|------------|-------------------------------------------
// 0                | 0          | 0
// 2^32             | 0          | 0.99999999994179233909330885695244...
// 2^32 + 1         | 1          | 1.0000000001746229827200734316305...
// 2^50             | 262140     | 262140.00012206565608606978175873...
// 2^55 + 20963331  | 8384515    | 8384515.9999999997673963974959744...
// 2^55 + 20963332  | 8384516    | 8384516
// 2^62 + 26599786  | 1013904242 | 1013904242.9999999999479374853545...
// 2^62 + 26599787  | 1013904243 | 1013904243.0000000001561875439364...
// 2^64 - 1         | 3558067407 | 3558067407.9041987696409179931096...

// The reference implementation as it is now uses only unsigned int64 arithmetic, so it can't have undefined behavior
// It was tested once for all edge cases and confirmed correct
static inline uint32_t integer_square_root_v2(uint64_t n)
{
  uint64_t r = 1ULL << 63;

  for (uint64_t bit = 1ULL << 60; bit; bit >>= 2)
  {
    const bool b = (n < r + bit);
    const uint64_t n_next = n - (r + bit);
    const uint64_t r_next = r + bit * 2;
    n = b ? n : n_next;
    r = b ? r : r_next;
    r >>= 1;
  }

  return r * 2 + ((n > r) ? 1 : 0);
}

/*
VARIANT2_INTEGER_MATH_SQRT_FIXUP checks that "r" is an integer part of "sqrt(2^64 + sqrt_input) * 2 - 2^33" and adds or subtracts 1 if needed
It's hard to understand how it works, so here is a full calculation of formulas used in VARIANT2_INTEGER_MATH_SQRT_FIXUP

The following inequalities must hold for r if it's an integer part of "sqrt(2^64 + sqrt_input) * 2 - 2^33":
1) r <= sqrt(2^64 + sqrt_input) * 2 - 2^33
2) r + 1 > sqrt(2^64 + sqrt_input) * 2 - 2^33

We need to check them using only unsigned integer arithmetic to avoid rounding errors and undefined behavior

First inequality: r <= sqrt(2^64 + sqrt_input) * 2 - 2^33
-----------------------------------------------------------------------------------
r <= sqrt(2^64 + sqrt_input) * 2 - 2^33
r + 2^33 <= sqrt(2^64 + sqrt_input) * 2
r/2 + 2^32 <= sqrt(2^64 + sqrt_input)
(r/2 + 2^32)^2 <= 2^64 + sqrt_input

Rewrite r as r = s * 2 + b (s = trunc(r/2), b is 0 or 1)

((s*2+b)/2 + 2^32)^2 <= 2^64 + sqrt_input
(s*2+b)^2/4 + 2*2^32*(s*2+b)/2 + 2^64 <= 2^64 + sqrt_input
(s*2+b)^2/4 + 2*2^32*(s*2+b)/2 <= sqrt_input
(s*2+b)^2/4 + 2^32*r <= sqrt_input
(s^2*4+2*s*2*b+b^2)/4 + 2^32*r <= sqrt_input
s^2+s*b+b^2/4 + 2^32*r <= sqrt_input
s*(s+b) + b^2/4 + 2^32*r <= sqrt_input

Let r2 = s*(s+b) + r*2^32
r2 + b^2/4 <= sqrt_input

If this inequality doesn't hold, then we must decrement r: IF "r2 + b^2/4 > sqrt_input" THEN r = r - 1

b can be 0 or 1
If b is 0 then we need to compare "r2 > sqrt_input"
If b is 1 then b^2/4 = 0.25, so we need to compare "r2 + 0.25 > sqrt_input"
Since both r2 and sqrt_input are integers, we can safely replace it with "r2 + 1 > sqrt_input"
-----------------------------------------------------------------------------------
Both cases can be merged to a single expression "r2 + b > sqrt_input"
-----------------------------------------------------------------------------------
There will be no overflow when calculating "r2 + b", so it's safe to compare with sqrt_input:
r2 + b = s*(s+b) + r*2^32 + b
The largest value s, b and r can have is s = 1779033703, b = 1, r = 3558067407 when sqrt_input = 2^64 - 1
r2 + b <= 1779033703*1779033704 + 3558067407*2^32 + 1 = 18446744068217447385 < 2^64

Second inequality: r + 1 > sqrt(2^64 + sqrt_input) * 2 - 2^33
-----------------------------------------------------------------------------------
r + 1 > sqrt(2^64 + sqrt_input) * 2 - 2^33
r + 1 + 2^33 > sqrt(2^64 + sqrt_input) * 2
((r+1)/2 + 2^32)^2 > 2^64 + sqrt_input

Rewrite r as r = s * 2 + b (s = trunc(r/2), b is 0 or 1)

((s*2+b+1)/2 + 2^32)^2 > 2^64 + sqrt_input
(s*2+b+1)^2/4 + 2*(s*2+b+1)/2*2^32 + 2^64 > 2^64 + sqrt_input
(s*2+b+1)^2/4 + (s*2+b+1)*2^32 > sqrt_input
(s*2+b+1)^2/4 + (r+1)*2^32 > sqrt_input
(s*2+(b+1))^2/4 + r*2^32 + 2^32 > sqrt_input
(s^2*4+2*s*2*(b+1)+(b+1)^2)/4 + r*2^32 + 2^32 > sqrt_input
s^2+s*(b+1)+(b+1)^2/4 + r*2^32 + 2^32 > sqrt_input
s*(s+b) + s + (b+1)^2/4 + r*2^32 + 2^32 > sqrt_input

Let r2 = s*(s+b) + r*2^32

r2 + s + (b+1)^2/4 + 2^32 > sqrt_input
r2 + 2^32 + (b+1)^2/4 > sqrt_input - s

If this inequality doesn't hold, then we must decrement r: IF "r2 + 2^32 + (b+1)^2/4 <= sqrt_input - s" THEN r = r - 1
b can be 0 or 1
If b is 0 then we need to compare "r2 + 2^32 + 1/4 <= sqrt_input - s" which is equal to "r2 + 2^32 < sqrt_input - s" because all numbers here are integers
If b is 1 then (b+1)^2/4 = 1, so we need to compare "r2 + 2^32 + 1 <= sqrt_input - s" which is also equal to "r2 + 2^32 < sqrt_input - s"
-----------------------------------------------------------------------------------
Both cases can be merged to a single expression "r2 + 2^32 < sqrt_input - s"
-----------------------------------------------------------------------------------
There will be no overflow when calculating "r2 + 2^32":
r2 + 2^32 = s*(s+b) + r*2^32 + 2^32 = s*(s+b) + (r+1)*2^32
The largest value s, b and r can have is s = 1779033703, b = 1, r = 3558067407 when sqrt_input = 2^64 - 1
r2 + b <= 1779033703*1779033704 + 3558067408*2^32 = 18446744072512414680 < 2^64

There will be no integer overflow when calculating "sqrt_input - s", i.e. "sqrt_input >= s" at all times:
s = trunc(r/2) = trunc(sqrt(2^64 + sqrt_input) - 2^32) < sqrt(2^64 + sqrt_input) - 2^32 + 1
sqrt_input > sqrt(2^64 + sqrt_input) - 2^32 + 1
sqrt_input + 2^32 - 1 > sqrt(2^64 + sqrt_input)
(sqrt_input + 2^32 - 1)^2 > sqrt_input + 2^64
sqrt_input^2 + 2*sqrt_input*(2^32 - 1) + (2^32-1)^2 > sqrt_input + 2^64
sqrt_input^2 + sqrt_input*(2^33 - 2) + (2^32-1)^2 > sqrt_input + 2^64
sqrt_input^2 + sqrt_input*(2^33 - 3) + (2^32-1)^2 > 2^64
sqrt_input^2 + sqrt_input*(2^33 - 3) + 2^64-2^33+1 > 2^64
sqrt_input^2 + sqrt_input*(2^33 - 3) - 2^33 + 1 > 0
This inequality is true if sqrt_input > 1 and it's easy to check that s = 0 if sqrt_input is 0 or 1, so there will be no integer overflow
*/

#define VARIANT2_INTEGER_MATH_SQRT_FIXUP(r) \
  do { \
    const uint64_t s = r >> 1; \
    const uint64_t b = r & 1; \
    const uint64_t r2 = (uint64_t)(s) * (s + b) + (r << 32); \
    r += ((r2 + b > sqrt_input) ? -1 : 0) + ((r2 + (1ULL << 32) < sqrt_input - s) ? 1 : 0); \
  } while(0)

#endif
