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
#include <limits.h>

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
  char rs_hash[HASH_SIZE];
  uint64_t  rs_height;
  randomx_cache *rs_cache;
} rx_state;

static CTHR_MUTEX_TYPE rx_mutex = CTHR_MUTEX_INIT;
static CTHR_MUTEX_TYPE rx_dataset_mutex = CTHR_MUTEX_INIT;

static rx_state rx_s[2] = {{CTHR_MUTEX_INIT,{0},0,0},{CTHR_MUTEX_INIT,{0},0,0}};

static randomx_dataset *rx_dataset;
static int rx_dataset_nomem;
static uint64_t rx_dataset_height;
static THREADV randomx_vm *rx_vm = NULL;

static void local_abort(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
  _exit(1);
#else
  abort();
#endif
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

void rx_reorg(const uint64_t split_height) {
  int i;
  CTHR_MUTEX_LOCK(rx_mutex);
  for (i=0; i<2; i++) {
    if (split_height <= rx_s[i].rs_height) {
      if (rx_s[i].rs_height == rx_dataset_height)
        rx_dataset_height = 1;
      rx_s[i].rs_height = 1;	/* set to an invalid seed height */
    }
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
  int toggle = (s_height & SEEDHASH_EPOCH_BLOCKS) != 0;
  randomx_flags flags = enabled_flags() & ~disabled_flags();
  rx_state *rx_sp;
  randomx_cache *cache;

  CTHR_MUTEX_LOCK(rx_mutex);

  /* if alt block but with same seed as mainchain, no need for alt cache */
  if (is_alt) {
    if (s_height == seedheight && !memcmp(rx_s[toggle].rs_hash, seedhash, HASH_SIZE))
      is_alt = 0;
  } else {
  /* RPC could request an earlier block on mainchain */
    if (s_height > seedheight)
      is_alt = 1;
    /* miner can be ahead of mainchain */
    else if (s_height < seedheight)
      toggle ^= 1;
  }

  toggle ^= (is_alt != 0);

  rx_sp = &rx_s[toggle];
  CTHR_MUTEX_LOCK(rx_sp->rs_mutex);
  CTHR_MUTEX_UNLOCK(rx_mutex);

  cache = rx_sp->rs_cache;
  if (cache == NULL) {
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
  if (rx_sp->rs_height != seedheight || rx_sp->rs_cache == NULL || memcmp(seedhash, rx_sp->rs_hash, HASH_SIZE)) {
    randomx_init_cache(cache, seedhash, HASH_SIZE);
    rx_sp->rs_cache = cache;
    rx_sp->rs_height = seedheight;
    memcpy(rx_sp->rs_hash, seedhash, HASH_SIZE);
  }
  if (rx_vm == NULL) {
    if ((flags & RANDOMX_FLAG_JIT) && !miners) {
        flags |= RANDOMX_FLAG_SECURE & ~disabled_flags();
    }
    if (miners && (disabled_flags() & RANDOMX_FLAG_FULL_MEM)) {
      miners = 0;
    }
    if (miners) {
      CTHR_MUTEX_LOCK(rx_dataset_mutex);
      if (!rx_dataset_nomem) {
        if (rx_dataset == NULL) {
          rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_LARGE_PAGES);
          if (rx_dataset == NULL) {
            mdebug(RX_LOGCAT, "Couldn't use largePages for RandomX dataset");
            rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
          }
          if (rx_dataset != NULL)
            rx_initdata(rx_sp->rs_cache, miners, seedheight);
        }
      }
      if (rx_dataset != NULL)
        flags |= RANDOMX_FLAG_FULL_MEM;
      else {
        miners = 0;
        if (!rx_dataset_nomem) {
          rx_dataset_nomem = 1;
          mwarning(RX_LOGCAT, "Couldn't allocate RandomX dataset for miner");
        }
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
    else if (rx_dataset == NULL) {
      /* this is a no-op if the cache hasn't changed */
      randomx_vm_set_cache(rx_vm, rx_sp->rs_cache);
    }
    CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
  } else {
    /* this is a no-op if the cache hasn't changed */
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
  rx_dataset_nomem = 0;
  CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
}
