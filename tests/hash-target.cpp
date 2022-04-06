// Copyright (c) 2014-2022, The Monero Project
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

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "misc_log_ex.h"
#include "crypto/hash.h"
#include "cryptonote_basic/difficulty.h"

using namespace std;
using cryptonote::check_hash;

int main(int argc, char *argv[]) {
  TRY_ENTRY();
  crypto::hash h;
  for (cryptonote::difficulty_type diff = 1;; diff += 1 + (diff >> 8)) {
    for (uint16_t b = 0; b < 256; b++) {
      memset(&h, b, sizeof(crypto::hash));
      if (check_hash(h, diff) != (b == 0 || diff <= 255 / b)) {
        return 1;
      }
      if (b > 0) {
        memset(&h, 0, sizeof(crypto::hash));
        ((char *) &h)[31] = b;
        if (check_hash(h, diff) != (diff <= 255 / b)) {
          return 2;
        }
      }
    }
    if (diff < numeric_limits<uint64_t>::max() / 256) {
      uint64_t val = 0;
      for (int i = 31; i >= 0; i--) {
        val = val * 256 + 255;
        ((char *) &h)[i] = static_cast<char>(static_cast<uint64_t>(val / diff));
        val %= (diff & 0xffffffffffffffff).convert_to<uint64_t>();
      }
      if (check_hash(h, diff) != true) {
        return 3;
      }
      if (diff > 1) {
        for (int i = 0;; i++) {
          if (i >= 32) {
            abort();
          }
          if (++((char *) &h)[i] != 0) {
            break;
          }
        }
        if (check_hash(h, diff) != false) {
          return 4;
        }
      }
    }
    if (diff + 1 + (diff >> 8) < diff) {
      break;
    }
  }
  return 0;
  CATCH_ENTRY_L0("main", 1);
}
