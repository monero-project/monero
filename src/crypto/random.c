// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "hash-ops.h"
#include "initializer.h"
#include "random.h"

static void generate_system_random_bytes(size_t n, void *result);

#if defined(_WIN32)

#include <windows.h>
#include <wincrypt.h>

static void generate_system_random_bytes(size_t n, void *result) {
  HCRYPTPROV prov;
#define must_succeed(x) do if (!(x)) assert(0); while (0)
  must_succeed(CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT));
  must_succeed(CryptGenRandom(prov, (DWORD)n, result));
  must_succeed(CryptReleaseContext(prov, 0));
#undef must_succeed
}

#else

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void generate_system_random_bytes(size_t n, void *result) {
  int fd;
  if ((fd = open("/dev/urandom", O_RDONLY | O_NOCTTY | O_CLOEXEC)) < 0) {
    err(EXIT_FAILURE, "open /dev/urandom");
  }
  for (;;) {
    ssize_t res = read(fd, result, n);
    if ((size_t) res == n) {
      break;
    }
    if (res < 0) {
      if (errno != EINTR) {
        err(EXIT_FAILURE, "read /dev/urandom");
      }
    } else if (res == 0) {
      errx(EXIT_FAILURE, "read /dev/urandom: end of file");
    } else {
      result = padd(result, (size_t) res);
      n -= (size_t) res;
    }
  }
  if (close(fd) < 0) {
    err(EXIT_FAILURE, "close /dev/urandom");
  }
}

#endif

static union hash_state state;

#if !defined(NDEBUG)
static volatile int curstate; /* To catch thread safety problems. */
#endif

FINALIZER(deinit_random) {
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 0;
#endif
  memset(&state, 0, sizeof(union hash_state));
}

INITIALIZER(init_random) {
  generate_system_random_bytes(32, &state);
  REGISTER_FINALIZER(deinit_random);
#if !defined(NDEBUG)
  assert(curstate == 0);
  curstate = 1;
#endif
}

void generate_random_bytes(size_t n, void *result) {
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 2;
#endif
  if (n == 0) {
#if !defined(NDEBUG)
    assert(curstate == 2);
    curstate = 1;
#endif
    return;
  }
  for (;;) {
    hash_permutation(&state);
    if (n <= HASH_DATA_AREA) {
      memcpy(result, &state, n);
#if !defined(NDEBUG)
      assert(curstate == 2);
      curstate = 1;
#endif
      return;
    } else {
      memcpy(result, &state, HASH_DATA_AREA);
      result = padd(result, HASH_DATA_AREA);
      n -= HASH_DATA_AREA;
    }
  }
}
