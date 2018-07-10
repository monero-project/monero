// Copyright (c) 2017-2018, The Monero Project
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
// Parts of this file Copyright (c) 2009-2015 The Bitcoin Core developers

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_EXPLICIT_BZERO
#include <strings.h>
#endif
#include "memwipe.h"

#if defined(_MSC_VER)
#define SCARECROW \
    __asm;
#else
#define SCARECROW \
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif

#ifdef HAVE_MEMSET_S

void *memwipe(void *ptr, size_t n)
{
  if (n > 0 && memset_s(ptr, n, 0, n))
  {
#ifdef NDEBUG
    fprintf(stderr, "Error: memset_s failed\n");
    _exit(1);
#else
    abort();
#endif
  }
  SCARECROW // might as well...
  return ptr;
}

#elif defined HAVE_EXPLICIT_BZERO

void *memwipe(void *ptr, size_t n)
{
  if (n > 0)
    explicit_bzero(ptr, n);
  SCARECROW
  return ptr;
}

#else

/* The memory_cleanse implementation is taken from Bitcoin */

/* Compilers have a bad habit of removing "superfluous" memset calls that
 * are trying to zero memory. For example, when memset()ing a buffer and
 * then free()ing it, the compiler might decide that the memset is
 * unobservable and thus can be removed.
 *
 * Previously we used OpenSSL which tried to stop this by a) implementing
 * memset in assembly on x86 and b) putting the function in its own file
 * for other platforms.
 *
 * This change removes those tricks in favour of using asm directives to
 * scare the compiler away. As best as our compiler folks can tell, this is
 * sufficient and will continue to be so.
 *
 * Adam Langley <agl@google.com>
 * Commit: ad1907fe73334d6c696c8539646c21b11178f20f
 * BoringSSL (LICENSE: ISC)
 */
static void memory_cleanse(void *ptr, size_t len)
{
    memset(ptr, 0, len);

    /* As best as we can tell, this is sufficient to break any optimisations that
       might try to eliminate "superfluous" memsets. If there's an easy way to
       detect memset_s, it would be better to use that. */
    SCARECROW
}

void *memwipe(void *ptr, size_t n)
{
  if (n > 0)
    memory_cleanse(ptr, n);
  SCARECROW
  return ptr;
}

#endif
