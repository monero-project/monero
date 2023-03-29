// Copyright (c) 2019-2022, The Monero Project
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "randomx.h"
#include "c_threads.h"
#include "hash-ops.h"
#include "misc_log_ex.h"

#define RX_LOGCAT	"randomx"

// Report large page allocation failures as debug messages
#define alloc_err_msg(x) mdebug(RX_LOGCAT, x);

static CTHR_RWLOCK_TYPE main_dataset_lock = CTHR_RWLOCK_INIT;
static CTHR_RWLOCK_TYPE main_cache_lock = CTHR_RWLOCK_INIT;

static randomx_dataset *main_dataset = NULL;
static randomx_cache *main_cache = NULL;
static char main_seedhash[HASH_SIZE];
static int main_seedhash_set = 0;

static CTHR_RWLOCK_TYPE secondary_cache_lock = CTHR_RWLOCK_INIT;

static randomx_cache *secondary_cache = NULL;
static char secondary_seedhash[HASH_SIZE];
static int secondary_seedhash_set = 0;

#if defined(_MSC_VER)
#define THREADV __declspec(thread)
#else
#define THREADV __thread
#endif

static THREADV randomx_vm *main_vm_full = NULL;
static THREADV randomx_vm *main_vm_light = NULL;
static THREADV randomx_vm *secondary_vm_light = NULL;

static THREADV uint32_t miner_thread = 0;

static bool is_main(const char* seedhash) { return main_seedhash_set && (memcmp(seedhash, main_seedhash, HASH_SIZE) == 0); }
static bool is_secondary(const char* seedhash) { return secondary_seedhash_set && (memcmp(seedhash, secondary_seedhash, HASH_SIZE) == 0); }

static void local_abort(const char *msg)
{
  merror(RX_LOGCAT, "%s", msg);
  fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
  _exit(1);
#else
  abort();
#endif
}

static void hash2hex(const char* hash, char* hex) {
  const char* d = "0123456789abcdef";
  for (int i = 0; i < HASH_SIZE; ++i) {
    const uint8_t b = hash[i];
    hex[i * 2 + 0] = d[b >> 4];
    hex[i * 2 + 1] = d[b & 15];
  }
  hex[HASH_SIZE * 2] = '\0';
}

static inline int disabled_flags(void) {
  static int flags = -1;

  if (flags != -1) {
    return flags;
  }

  const char *env = getenv("MONERO_RANDOMX_UMASK");
  if (!env) {
    flags = 0;
  }
  else {
    char* endptr;
    long int value = strtol(env, &endptr, 0);
    if (endptr != env && value >= 0 && value < INT_MAX) {
      flags = value;
    }
    else {
      flags = 0;
    }
  }

  return flags;
}

static inline int enabled_flags(void) {
  static int flags = -1;

  if (flags != -1) {
    return flags;
  }

  flags = randomx_get_flags();

  return flags;
}

#define SEEDHASH_EPOCH_BLOCKS	2048	/* Must be same as BLOCKS_SYNCHRONIZING_MAX_COUNT in cryptonote_config.h */
#define SEEDHASH_EPOCH_LAG		64

static inline int is_power_of_2(uint64_t n) { return n && (n & (n-1)) == 0; }

static int get_seedhash_epoch_lag(void)
{
  static unsigned int lag = (unsigned int)-1;
  if (lag != (unsigned int)-1)
    return lag;
  const char *e = getenv("SEEDHASH_EPOCH_LAG");
  if (e)
  {
    lag = atoi(e);
    if (lag > SEEDHASH_EPOCH_LAG || !is_power_of_2(lag))
      lag = SEEDHASH_EPOCH_LAG;
  }
  else
  {
    lag = SEEDHASH_EPOCH_LAG;
  }
  return lag;
}

static unsigned int get_seedhash_epoch_blocks(void)
{
  static unsigned int blocks = (unsigned int)-1;
  if (blocks != (unsigned int)-1)
    return blocks;
  const char *e = getenv("SEEDHASH_EPOCH_BLOCKS");
  if (e)
  {
    blocks = atoi(e);
    if (blocks < 2 || blocks > SEEDHASH_EPOCH_BLOCKS || !is_power_of_2(blocks))
      blocks = SEEDHASH_EPOCH_BLOCKS;
  }
  else
  {
    blocks = SEEDHASH_EPOCH_BLOCKS;
  }
  return blocks;
}

uint64_t rx_seedheight(const uint64_t height) {
  const uint64_t seedhash_epoch_lag = get_seedhash_epoch_lag();
  const uint64_t seedhash_epoch_blocks = get_seedhash_epoch_blocks();
  uint64_t s_height =  (height <= seedhash_epoch_blocks+seedhash_epoch_lag) ? 0 :
                       (height - seedhash_epoch_lag - 1) & ~(seedhash_epoch_blocks-1);
  return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seedheight, uint64_t *nextheight) {
  *seedheight = rx_seedheight(height);
  *nextheight = rx_seedheight(height + get_seedhash_epoch_lag());
}

static void rx_alloc_dataset(randomx_flags flags, randomx_dataset** dataset, int ignore_env)
{
  if (*dataset) {
    return;
  }

  if (disabled_flags() & RANDOMX_FLAG_FULL_MEM) {
    static int shown = 0;
    if (!shown) {
      shown = 1;
      minfo(RX_LOGCAT, "RandomX dataset is disabled by MONERO_RANDOMX_UMASK environment variable.");
    }
    return;
  }

  if (!ignore_env && !getenv("MONERO_RANDOMX_FULL_MEM")) {
    static int shown = 0;
    if (!shown) {
      shown = 1;
      minfo(RX_LOGCAT, "RandomX dataset is not enabled by default. Use MONERO_RANDOMX_FULL_MEM environment variable to enable it.");
    }
    return;
  }

  *dataset = randomx_alloc_dataset((flags | RANDOMX_FLAG_LARGE_PAGES) & ~disabled_flags());
  if (!*dataset) {
    alloc_err_msg("Couldn't allocate RandomX dataset using large pages");
    *dataset = randomx_alloc_dataset(flags & ~disabled_flags());
    if (!*dataset) {
      merror(RX_LOGCAT, "Couldn't allocate RandomX dataset");
    }
  }
}

static void rx_alloc_cache(randomx_flags flags, randomx_cache** cache)
{
  if (*cache) {
    return;
  }

  *cache = randomx_alloc_cache((flags | RANDOMX_FLAG_LARGE_PAGES) & ~disabled_flags());
  if (!*cache) {
    alloc_err_msg("Couldn't allocate RandomX cache using large pages");
    *cache = randomx_alloc_cache(flags & ~disabled_flags());
    if (!*cache) local_abort("Couldn't allocate RandomX cache");
  }
}

static void rx_init_full_vm(randomx_flags flags, randomx_vm** vm)
{
  if (*vm || !main_dataset || (disabled_flags() & RANDOMX_FLAG_FULL_MEM)) {
    return;
  }

  if ((flags & RANDOMX_FLAG_JIT) && !miner_thread) {
    flags |= RANDOMX_FLAG_SECURE;
  }

  *vm = randomx_create_vm((flags | RANDOMX_FLAG_LARGE_PAGES | RANDOMX_FLAG_FULL_MEM) & ~disabled_flags(), NULL, main_dataset);
  if (!*vm) {
    static int shown = 0;
    if (!shown) {
        shown = 1;
        alloc_err_msg("Couldn't allocate RandomX full VM using large pages (will print only once)");
    }
    *vm = randomx_create_vm((flags | RANDOMX_FLAG_FULL_MEM) & ~disabled_flags(), NULL, main_dataset);
    if (!*vm) {
      merror(RX_LOGCAT, "Couldn't allocate RandomX full VM");
    }
  }
}

static void rx_init_light_vm(randomx_flags flags, randomx_vm** vm, randomx_cache* cache)
{
  if (*vm) {
    randomx_vm_set_cache(*vm, cache);
    return;
  }

  if ((flags & RANDOMX_FLAG_JIT) && !miner_thread) {
    flags |= RANDOMX_FLAG_SECURE;
  }

  flags &= ~RANDOMX_FLAG_FULL_MEM;

  *vm = randomx_create_vm((flags | RANDOMX_FLAG_LARGE_PAGES) & ~disabled_flags(), cache, NULL);
  if (!*vm) {
    static int shown = 0;
    if (!shown) {
        shown = 1;
        alloc_err_msg("Couldn't allocate RandomX light VM using large pages (will print only once)");
    }
    *vm = randomx_create_vm(flags & ~disabled_flags(), cache, NULL);
    if (!*vm) local_abort("Couldn't allocate RandomX light VM");
  }
}

typedef struct seedinfo {
  randomx_cache *si_cache;
  unsigned long si_start;
  unsigned long si_count;
} seedinfo;

static CTHR_THREAD_RTYPE rx_seedthread(void *arg) {
  seedinfo *si = arg;
  randomx_init_dataset(main_dataset, si->si_cache, si->si_start, si->si_count);
  CTHR_THREAD_RETURN;
}

static void rx_init_dataset(size_t max_threads) {
  if (!main_dataset) {
    return;
  }

  // leave 2 CPU cores for other tasks
  const size_t num_threads = (max_threads < 4) ? 1 : (max_threads - 2);
  seedinfo* si = malloc(num_threads * sizeof(seedinfo));
  if (!si) local_abort("Couldn't allocate RandomX mining threadinfo");

  const uint32_t delta = randomx_dataset_item_count() / num_threads;
  uint32_t start = 0;

  const size_t n1 = num_threads - 1;
  for (size_t i = 0; i < n1; ++i) {
    si[i].si_cache = main_cache;
    si[i].si_start = start;
    si[i].si_count = delta;
    start += delta;
  }

  si[n1].si_cache = main_cache;
  si[n1].si_start = start;
  si[n1].si_count = randomx_dataset_item_count() - start;

  CTHR_THREAD_TYPE *st = malloc(num_threads * sizeof(CTHR_THREAD_TYPE));
  if (!st) local_abort("Couldn't allocate RandomX mining threadlist");

  CTHR_RWLOCK_LOCK_READ(main_cache_lock);
  for (size_t i = 0; i < n1; ++i) {
    if (!CTHR_THREAD_CREATE(st[i], rx_seedthread, &si[i])) {
      local_abort("Couldn't start RandomX seed thread");
    }
  }
  randomx_init_dataset(main_dataset, si[n1].si_cache, si[n1].si_start, si[n1].si_count);
  for (size_t i = 0; i < n1; ++i) CTHR_THREAD_JOIN(st[i]);
  CTHR_RWLOCK_UNLOCK_READ(main_cache_lock);

  free(st);
  free(si);

  minfo(RX_LOGCAT, "RandomX dataset initialized");
}

typedef struct thread_info {
  char seedhash[HASH_SIZE];
  size_t max_threads;
} thread_info;

static CTHR_THREAD_RTYPE rx_set_main_seedhash_thread(void *arg) {
  thread_info* info = arg;

  CTHR_RWLOCK_LOCK_WRITE(main_dataset_lock);
  CTHR_RWLOCK_LOCK_WRITE(main_cache_lock);

  // Double check that seedhash wasn't already updated
  if (is_main(info->seedhash)) {
    CTHR_RWLOCK_UNLOCK_WRITE(main_cache_lock);
    CTHR_RWLOCK_UNLOCK_WRITE(main_dataset_lock);
    free(info);
    CTHR_THREAD_RETURN;
  }
  memcpy(main_seedhash, info->seedhash, HASH_SIZE);
  main_seedhash_set = 1;

  char buf[HASH_SIZE * 2 + 1];
  hash2hex(main_seedhash, buf);
  minfo(RX_LOGCAT, "RandomX new main seed hash is %s", buf);

  const randomx_flags flags = enabled_flags() & ~disabled_flags();
  rx_alloc_dataset(flags, &main_dataset, 0);
  rx_alloc_cache(flags, &main_cache);

  randomx_init_cache(main_cache, info->seedhash, HASH_SIZE);
  minfo(RX_LOGCAT, "RandomX main cache initialized");

  CTHR_RWLOCK_UNLOCK_WRITE(main_cache_lock);

  // From this point, rx_slow_hash can calculate hashes in light mode, but dataset is not initialized yet
  rx_init_dataset(info->max_threads);

  CTHR_RWLOCK_UNLOCK_WRITE(main_dataset_lock);

  free(info);
  CTHR_THREAD_RETURN;
}

void rx_set_main_seedhash(const char *seedhash, size_t max_dataset_init_threads) {
  // Early out if seedhash didn't change
  if (is_main(seedhash)) {
    return;
  }

  // Update main cache and dataset in the background
  thread_info* info = malloc(sizeof(thread_info));
  if (!info) local_abort("Couldn't allocate RandomX mining threadinfo");

  memcpy(info->seedhash, seedhash, HASH_SIZE);
  info->max_threads = max_dataset_init_threads;

  CTHR_THREAD_TYPE t;
  if (!CTHR_THREAD_CREATE(t, rx_set_main_seedhash_thread, info)) {
    local_abort("Couldn't start RandomX seed thread");
  }
  CTHR_THREAD_CLOSE(t);
}

void rx_slow_hash(const char *seedhash, const void *data, size_t length, char *result_hash) {
  const randomx_flags flags = enabled_flags() & ~disabled_flags();
  int success = 0;

  // Fast path (seedhash == main_seedhash)
  // Multiple threads can run in parallel in fast or light mode, 1-2 ms or 10-15 ms per hash per thread
  if (is_main(seedhash)) {
    // If CTHR_RWLOCK_TRYLOCK_READ fails it means dataset is being initialized now, so use the light mode
    if (main_dataset && CTHR_RWLOCK_TRYLOCK_READ(main_dataset_lock)) {
      // Double check that main_seedhash didn't change
      if (is_main(seedhash)) {
        rx_init_full_vm(flags, &main_vm_full);
        if (main_vm_full) {
          randomx_calculate_hash(main_vm_full, data, length, result_hash);
          success = 1;
        }
      }
      CTHR_RWLOCK_UNLOCK_READ(main_dataset_lock);
    } else {
      CTHR_RWLOCK_LOCK_READ(main_cache_lock);
      // Double check that main_seedhash didn't change
      if (is_main(seedhash)) {
        rx_init_light_vm(flags, &main_vm_light, main_cache);
        randomx_calculate_hash(main_vm_light, data, length, result_hash);
        success = 1;
      }
      CTHR_RWLOCK_UNLOCK_READ(main_cache_lock);
    }
  }

  if (success) {
    return;
  }

  char buf[HASH_SIZE * 2 + 1];

  // Slow path (seedhash != main_seedhash, but seedhash == secondary_seedhash)
  // Multiple threads can run in parallel in light mode, 10-15 ms per hash per thread
  if (!secondary_cache) {
    CTHR_RWLOCK_LOCK_WRITE(secondary_cache_lock);
    if (!secondary_cache) {
      hash2hex(seedhash, buf);
      minfo(RX_LOGCAT, "RandomX new secondary seed hash is %s", buf);

      rx_alloc_cache(flags, &secondary_cache);
      randomx_init_cache(secondary_cache, seedhash, HASH_SIZE);
      minfo(RX_LOGCAT, "RandomX secondary cache updated");
      memcpy(secondary_seedhash, seedhash, HASH_SIZE);
      secondary_seedhash_set = 1;
    }
    CTHR_RWLOCK_UNLOCK_WRITE(secondary_cache_lock);
  }

  CTHR_RWLOCK_LOCK_READ(secondary_cache_lock);
  if (is_secondary(seedhash)) {
    rx_init_light_vm(flags, &secondary_vm_light, secondary_cache);
    randomx_calculate_hash(secondary_vm_light, data, length, result_hash);
    success = 1;
  }
  CTHR_RWLOCK_UNLOCK_READ(secondary_cache_lock);

  if (success) {
    return;
  }

  // Slowest path (seedhash != main_seedhash, seedhash != secondary_seedhash)
  // Only one thread runs at a time and updates secondary_seedhash if needed, up to 200-500 ms per hash
  CTHR_RWLOCK_LOCK_WRITE(secondary_cache_lock);
  if (!is_secondary(seedhash)) {
    hash2hex(seedhash, buf);
    minfo(RX_LOGCAT, "RandomX new secondary seed hash is %s", buf);

    randomx_init_cache(secondary_cache, seedhash, HASH_SIZE);
    minfo(RX_LOGCAT, "RandomX secondary cache updated");
    memcpy(secondary_seedhash, seedhash, HASH_SIZE);
    secondary_seedhash_set = 1;
  }
  rx_init_light_vm(flags, &secondary_vm_light, secondary_cache);
  randomx_calculate_hash(secondary_vm_light, data, length, result_hash);
  CTHR_RWLOCK_UNLOCK_WRITE(secondary_cache_lock);
}

void rx_set_miner_thread(uint32_t value, size_t max_dataset_init_threads) {
  miner_thread = value;

  // If dataset is not allocated yet, try to allocate and initialize it
  CTHR_RWLOCK_LOCK_WRITE(main_dataset_lock);
  if (main_dataset) {
    CTHR_RWLOCK_UNLOCK_WRITE(main_dataset_lock);
    return;
  }

  const randomx_flags flags = enabled_flags() & ~disabled_flags();
  rx_alloc_dataset(flags, &main_dataset, 1);
  rx_init_dataset(max_dataset_init_threads);

  CTHR_RWLOCK_UNLOCK_WRITE(main_dataset_lock);
}

uint32_t rx_get_miner_thread() {
  return miner_thread;
}

void rx_slow_hash_allocate_state() {}

static void rx_destroy_vm(randomx_vm** vm) {
  if (*vm) {
    randomx_destroy_vm(*vm);
    *vm = NULL;
  }
}

void rx_slow_hash_free_state() {
  rx_destroy_vm(&main_vm_full);
  rx_destroy_vm(&main_vm_light);
  rx_destroy_vm(&secondary_vm_light);
}
