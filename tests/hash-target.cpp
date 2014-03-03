// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "crypto/hash.h"
#include "cryptonote_core/difficulty.h"

using namespace std;
using cryptonote::check_hash;

int main(int argc, char *argv[]) {
  crypto::hash h;
  for (uint64_t diff = 1;; diff += 1 + (diff >> 8)) {
    for (int b = 0; b < 256; b++) {
      memset(&h, b, sizeof(crypto::hash));
      if (check_hash(h, diff) != (b == 0 || diff <= 255 / b)) {
        return 1;
      }
      if (b > 0) {
        memset(&h, 0, sizeof(crypto::hash));
        ((char *) &h)[31] = b;
        if (check_hash(h, diff) != (diff <= 255 / b)) {
          return 1;
        }
      }
    }
    if (diff < numeric_limits<uint64_t>::max() / 256) {
      uint64_t val = 0;
      for (int i = 31; i >= 0; i--) {
        val = val * 256 + 255;
        ((char *) &h)[i] = static_cast<char>(val / diff);
        val %= diff;
      }
      if (check_hash(h, diff) != true) {
        return 1;
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
          return 1;
        }
      }
    }
    if (diff + 1 + (diff >> 8) < diff) {
      break;
    }
  }
  return 0;
}
