// Copyright (c) 2019-2024, The Monero Project
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

#include <boost/multiprecision/cpp_int.hpp>

void div128_64(uint64_t dividend_hi, uint64_t dividend_lo, uint64_t divisor, uint64_t* quotient_hi, uint64_t *quotient_lo, uint64_t *remainder_hi, uint64_t *remainder_lo)
{
  typedef boost::multiprecision::uint128_t uint128_t;

  uint128_t dividend = dividend_hi;
  dividend <<= 64;
  dividend |= dividend_lo;

  uint128_t q, r;
  divide_qr(dividend, uint128_t(divisor), q, r);

  *quotient_hi = ((q >> 64) & 0xffffffffffffffffull).convert_to<uint64_t>();
  *quotient_lo = (q & 0xffffffffffffffffull).convert_to<uint64_t>();
  if (remainder_hi)
    *remainder_hi = ((r >> 64) & 0xffffffffffffffffull).convert_to<uint64_t>();
  if (remainder_lo)
    *remainder_lo = (r & 0xffffffffffffffffull).convert_to<uint64_t>();
}
