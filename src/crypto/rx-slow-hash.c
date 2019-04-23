// Copyright (c) 2019, The Monero Project
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

#include "randomx.h"
#include "c_threads.h"
#include "hash-ops.h"
#include "misc_log_ex.h"

#define RX_LOGCAT	"randomx"

#if defined(_MSC_VER)
#define THREADV __declspec(thread)
#else
#define THREADV __thread
#endif

typedef struct rx_state {
  CTHR_MUTEX_TYPE rs_mutex;
  char rs_hash[32];
  uint64_t  rs_height;
  randomx_cache *rs_cache;
} rx_state;

static CTHR_MUTEX_TYPE rx_mutex = CTHR_MUTEX_INIT;
static CTHR_MUTEX_TYPE rx_dataset_mutex = CTHR_MUTEX_INIT;

static rx_state rx_s[2] = {{CTHR_MUTEX_INIT,{0},0,0},{CTHR_MUTEX_INIT,{0},0,0}};

static randomx_dataset *rx_dataset;
static uint64_t rx_dataset_height;
static THREADV randomx_vm *rx_vm = NULL;
static THREADV int rx_toggle;

static void local_abort(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
  _exit(1);
#else
  abort();
#endif
}

/**
 * @brief uses cpuid to determine if the CPU supports the AES instructions
 * @return true if the CPU supports AES, false otherwise
 */

static inline int force_software_aes(void)
{
  static int use = -1;

  if (use != -1)
    return use;

  const char *env = getenv("MONERO_USE_SOFTWARE_AES");
  if (!env) {
    use = 0;
  }
  else if (!strcmp(env, "0") || !strcmp(env, "no")) {
    use = 0;
  }
  else {
    use = 1;
  }
  return use;
}

static void cpuid(int CPUInfo[4], int InfoType)
{
#if defined(__x86_64__)
    __asm __volatile__
    (
    "cpuid":
        "=a" (CPUInfo[0]),
        "=b" (CPUInfo[1]),
        "=c" (CPUInfo[2]),
        "=d" (CPUInfo[3]) :
            "a" (InfoType), "c" (0)
        );
#endif
}
static inline int check_aes_hw(void)
{
#if defined(__x86_64__)
    int cpuid_results[4];
    static int supported = -1;

    if(supported >= 0)
        return supported;

    cpuid(cpuid_results,1);
    return supported = cpuid_results[2] & (1 << 25);
#else
    return 0;
#endif
}

static volatile int use_rx_jit_flag = -1;

static inline int use_rx_jit(void)
{
#if defined(__x86_64__)

  if (use_rx_jit_flag != -1)
    return use_rx_jit_flag;

  const char *env = getenv("MONERO_USE_RX_JIT");
  if (!env) {
    use_rx_jit_flag = 1;
  }
  else if (!strcmp(env, "0") || !strcmp(env, "no")) {
    use_rx_jit_flag = 0;
  }
  else {
    use_rx_jit_flag = 1;
  }
  return use_rx_jit_flag;
#else
  return 0;
#endif
}

#define SEEDHASH_EPOCH_BLOCKS	2048	/* Must be same as BLOCKS_SYNCHRONIZING_MAX_COUNT in cryptonote_config.h */
#define SEEDHASH_EPOCH_LAG		64

void rx_reorg(const uint64_t split_height) {
  int i;
  CTHR_MUTEX_LOCK(rx_mutex);
  for (i=0; i<2; i++) {
    if (split_height < rx_s[i].rs_height)
      rx_s[i].rs_height = 1;	/* set to an invalid seed height */
  }
  CTHR_MUTEX_UNLOCK(rx_mutex);
}

uint64_t rx_seedheight(const uint64_t height) {
  uint64_t s_height =  (height <= SEEDHASH_EPOCH_BLOCKS+SEEDHASH_EPOCH_LAG) ? 0 :
                       (height - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS-1);
  return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seedheight, uint64_t *nextheight) {
  *seedheight = rx_seedheight(height);
  *nextheight = rx_seedheight(height + SEEDHASH_EPOCH_LAG);
}

typedef struct seedinfo {
  randomx_cache *si_cache;
  unsigned long si_start;
  unsigned long si_count;
} seedinfo;

static CTHR_THREAD_RTYPE rx_seedthread(void *arg) {
  seedinfo *si = arg;
  randomx_init_dataset(rx_dataset, si->si_cache, si->si_start, si->si_count);
  CTHR_THREAD_RETURN;
}

static void rx_initdata(randomx_cache *rs_cache, const int miners, const uint64_t seedheight) {
  if (miners > 1) {
    unsigned long delta = randomx_dataset_item_count() / miners;
    unsigned long start = 0;
    int i;
    seedinfo *si;
    CTHR_THREAD_TYPE *st;
    si = malloc(miners * sizeof(seedinfo));
    if (si == NULL)
      local_abort("Couldn't allocate RandomX mining threadinfo");
    st = malloc(miners * sizeof(CTHR_THREAD_TYPE));
    if (st == NULL) {
      free(si);
      local_abort("Couldn't allocate RandomX mining threadlist");
    }
    for (i=0; i<miners-1; i++) {
      si[i].si_cache = rs_cache;
      si[i].si_start = start;
      si[i].si_count = delta;
      start += delta;
    }
    si[i].si_cache = rs_cache;
    si[i].si_start = start;
    si[i].si_count = randomx_dataset_item_count() - start;
    for (i=1; i<miners; i++) {
      CTHR_THREAD_CREATE(st[i], rx_seedthread, &si[i]);
    }
    randomx_init_dataset(rx_dataset, rs_cache, 0, si[0].si_count);
    for (i=1; i<miners; i++) {
      CTHR_THREAD_JOIN(st[i]);
    }
    free(st);
    free(si);
  } else {
    randomx_init_dataset(rx_dataset, rs_cache, 0, randomx_dataset_item_count());
  }
  rx_dataset_height = seedheight;
}

void rx_slow_hash(const uint64_t mainheight, const uint64_t seedheight, const char *seedhash, const void *data, size_t length,
  char *hash, int miners, int is_alt) {
  uint64_t s_height = rx_seedheight(mainheight);
  int changed = 0;
  int toggle = is_alt ? s_height : seedheight;
  randomx_flags flags = RANDOMX_FLAG_DEFAULT;
  rx_state *rx_sp;
  randomx_cache *cache;

  toggle = (toggle & SEEDHASH_EPOCH_BLOCKS) != 0;
  CTHR_MUTEX_LOCK(rx_mutex);

  /* if alt block but with same seed as mainchain, no need for alt cache */
  if (is_alt && s_height == seedheight && !memcmp(rx_s[toggle].rs_hash, seedhash, sizeof(rx_s[toggle].rs_hash)))
    is_alt = 0;

  /* RPC could request an earlier block on mainchain */
  if (!is_alt && s_height > seedheight)
    is_alt = 1;

  toggle ^= (is_alt != 0);
  if (toggle != rx_toggle)
    changed = 1;
  rx_toggle = toggle;

  rx_sp = &rx_s[toggle];
  CTHR_MUTEX_LOCK(rx_sp->rs_mutex);
  CTHR_MUTEX_UNLOCK(rx_mutex);

  cache = rx_sp->rs_cache;
  if (cache == NULL) {
    if (use_rx_jit())
      flags |= RANDOMX_FLAG_JIT;
    if (cache == NULL) {
      cache = randomx_alloc_cache(flags | RANDOMX_FLAG_LARGE_PAGES);
      if (cache == NULL) {
        mdebug(RX_LOGCAT, "Couldn't use largePages for RandomX cache");
        cache = randomx_alloc_cache(flags);
      }
      if (cache == NULL)
        local_abort("Couldn't allocate RandomX cache");
    }
  }
  if (rx_sp->rs_height != seedheight || rx_sp->rs_cache == NULL || memcmp(seedhash, rx_sp->rs_hash, sizeof(rx_sp->rs_hash))) {
    randomx_init_cache(cache, seedhash, 32);
    rx_sp->rs_cache = cache;
    rx_sp->rs_height = seedheight;
    memcpy(rx_sp->rs_hash, seedhash, sizeof(rx_sp->rs_hash));
    changed = 1;
  }
  if (rx_vm == NULL) {
    randomx_flags flags = RANDOMX_FLAG_DEFAULT;
    if (use_rx_jit()) {
      flags |= RANDOMX_FLAG_JIT;
      if (!miners)
          flags |= RANDOMX_FLAG_SECURE;
    }
    if(!force_software_aes() && check_aes_hw())
      flags |= RANDOMX_FLAG_HARD_AES;
    if (miners) {
      CTHR_MUTEX_LOCK(rx_dataset_mutex);
      if (rx_dataset == NULL) {
        rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_LARGE_PAGES);
        if (rx_dataset == NULL) {
          mdebug(RX_LOGCAT, "Couldn't use largePages for RandomX dataset");
          rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
        }
        if (rx_dataset != NULL)
          rx_initdata(rx_sp->rs_cache, miners, seedheight);
      }
      if (rx_dataset != NULL)
        flags |= RANDOMX_FLAG_FULL_MEM;
      else {
        miners = 0;
        mwarning(RX_LOGCAT, "Couldn't allocate RandomX dataset for miner");
      }
      CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
    }
    rx_vm = randomx_create_vm(flags | RANDOMX_FLAG_LARGE_PAGES, rx_sp->rs_cache, rx_dataset);
    if(rx_vm == NULL) { //large pages failed
      mdebug(RX_LOGCAT, "Couldn't use largePages for RandomX VM");
      rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
    }
    if(rx_vm == NULL) {//fallback if everything fails
      flags = RANDOMX_FLAG_DEFAULT | (miners ? RANDOMX_FLAG_FULL_MEM : 0);
      rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
    }
    if (rx_vm == NULL)
      local_abort("Couldn't allocate RandomX VM");
  } else if (miners) {
    CTHR_MUTEX_LOCK(rx_dataset_mutex);
    if (rx_dataset != NULL && rx_dataset_height != seedheight)
      rx_initdata(cache, miners, seedheight);
    CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
  } else if (changed) {
    randomx_vm_set_cache(rx_vm, rx_sp->rs_cache);
  }
  /* mainchain users can run in parallel */
  if (!is_alt)
    CTHR_MUTEX_UNLOCK(rx_sp->rs_mutex);
  randomx_calculate_hash(rx_vm, data, length, hash);
  /* altchain slot users always get fully serialized */
  if (is_alt)
    CTHR_MUTEX_UNLOCK(rx_sp->rs_mutex);
}

void rx_slow_hash_allocate_state(void) {
}

void rx_slow_hash_free_state(void) {
  if (rx_vm != NULL) {
    randomx_destroy_vm(rx_vm);
    rx_vm = NULL;
  }
}

void rx_stop_mining(void) {
  CTHR_MUTEX_LOCK(rx_dataset_mutex);
  if (rx_dataset != NULL) {
    randomx_dataset *rd = rx_dataset;
    rx_dataset = NULL;
    randomx_release_dataset(rd);
  }
  CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
}
