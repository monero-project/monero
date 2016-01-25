// Copyright (c) 2014-2016, The Monero Project
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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "hash-ops.h"

#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
 #include <alloca.h>
#else
 #include <stdlib.h>
#endif

/// Quick check if this is power of two (use on unsigned types; in this case for size_t only)
bool ispowerof2_size_t(size_t x) {
	return x && !(x & (x - 1));
}

/*** 
* Round to power of two, for count>=3 and for count being not too large (as reasonable for tree hash calculations)
*/
size_t tree_hash_cnt(size_t count) {
	assert( count >= 3); // cases for 0,1,2 are handled elsewhere
	// Round down the count size: fun(2**n)= 2**(n-1) to round down to power of two
	size_t tmp = count - 1;
	size_t jj = 1;
	for (jj=1 ; tmp != 0 ; ++jj) {
		tmp /= 2; // dividing by 2 until to get how many powers of 2 fits size_to tmp
	}
	size_t cnt = 1 << (jj-2); // cnt is the count, but rounded down to power of two
	// printf("count=%zu cnt=%zu jj=%zu tmp=%zu \n" , count,cnt,jj,tmp);
	assert( cnt > 0 );	assert( cnt >= count/2 ); 	assert( cnt <= count );
	assert( ispowerof2_size_t( cnt ));
	return cnt;
}

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash) {
// The blockchain block at height 202612 http://monerochain.info/block/bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698
// contained 514 transactions, that triggered bad calculation of variable "cnt" in the original version of this function
// as from CryptoNote code.
//
// This bug applies to all CN altcoins.
//
// Mathematical bug here was first published on 14:45:34 (GMT+2) 2014-09-04 by Rafal Freeman <rfree>
// https://github.com/rfree2monero/bitmonero/commit/b417abfb7a297d09f1bbb6de29030f8de9952ac8
// and soon also applied to CryptoNote (15:10 GMT+2), and BoolBerry used not fully correct work around:
// the work around of sizeof(size_t)*8 or <<3 as used before in 2 coins and in BBL later was blocking
// exploitation on normal platforms, how ever we strongly recommend the following fix because it removes
// mistake in mathematical formula.

  assert(count > 0);
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
  } else if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
  } else {
    size_t i, j;

    size_t cnt = tree_hash_cnt( count );
    size_t max_size_t = (size_t) -1; // max allowed value of size_t 
    assert( cnt < max_size_t/2 ); // reasonable size to avoid any overflows. /2 is extra; Anyway should be limited much stronger by logical code 
    // as we have sane limits on transactions counts in blockchain rules

    char (*ints)[HASH_SIZE];
    size_t ints_size = cnt * HASH_SIZE;
    ints = alloca(ints_size); 	memset( ints , 0 , ints_size);  // allocate, and zero out as extra protection for using uninitialized mem

    memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);

    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      cn_fast_hash(hashes[i], 64, ints[j]);
    }
    assert(i == count);

    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        cn_fast_hash(ints[i], 64, ints[j]);
      }
    }

    cn_fast_hash(ints[0], 64, root_hash);
  }
}
