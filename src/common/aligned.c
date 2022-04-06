// Copyright (c) 2017-2022, The Monero Project
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "aligned.h"

static inline int is_power_of_2(size_t n) { return n && (n & (n-1)) == 0; }

#define MAGIC 0xaa0817161500ff81
#define MAGIC_FREED 0xaa0817161500ff82

static void local_abort(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
  _exit(1);
#else
  abort();
#endif
}

typedef struct
{
  uint64_t magic;
  void *raw;
  size_t bytes;
  size_t align;
} control;

void *aligned_malloc(size_t bytes, size_t align)
{
  void *raw, *ptr;
  control *ctrl;

  if (!is_power_of_2(align))
    return NULL;
  if (bytes > (size_t)-1 - align)
    return NULL;
  if (bytes + align > (size_t)-1 - sizeof(control))
    return NULL;

  raw = malloc(bytes + sizeof(control) + align);
  if (!raw)
    return NULL;
  ptr = (void*)(((uintptr_t)raw + align + sizeof(control) - 1) & ~(align-1));
  ctrl = ((control*)ptr) - 1;
  ctrl->magic = MAGIC;
  ctrl->raw = raw;
  ctrl->bytes = bytes;
  ctrl->align = align;
  return ptr;
}

void *aligned_realloc(void *ptr, size_t bytes, size_t align)
{
  void *raw, *ptr2;
  control *ctrl, *ctrl2;

  if (!ptr)
    return aligned_malloc(bytes, align);
  if (!bytes)
  {
     aligned_free(ptr);
     return NULL;
  }
  if (!is_power_of_2(align))
    return NULL;

  ctrl = ((control*)ptr) - 1;
  if (ctrl->magic == MAGIC_FREED)
    local_abort("Double free detected");
  if (ctrl->magic != MAGIC)
    local_abort("Freeing unallocated memory");
  if (ctrl->align != align)
    return NULL;
  if (ctrl->bytes >= bytes)
    return ptr;

  if (ctrl->bytes > (size_t)-1 - ctrl->align)
    return NULL;
  if (ctrl->bytes + ctrl->align > (size_t)-1 - sizeof(control))
    return NULL;

  raw = malloc(bytes + sizeof(control) + ctrl->align);
  if (!raw)
    return NULL;
  ptr2 = (void*)(((uintptr_t)raw + ctrl->align + sizeof(control) - 1) & ~(ctrl->align-1));
  memcpy(ptr2, ptr, ctrl->bytes);
  ctrl2 = ((control*)ptr2) - 1;
  ctrl2->magic = MAGIC;
  ctrl2->raw = raw;
  ctrl2->bytes = bytes;
  ctrl2->align = ctrl->align;
  ctrl->magic = MAGIC_FREED;
  free(ctrl->raw);
  return ptr2;
}

void aligned_free(void *ptr)
{
  if (!ptr)
    return;
  control *ctrl = ((control*)ptr) - 1;
  if (ctrl->magic == MAGIC_FREED)
    local_abort("Double free detected");
  if (ctrl->magic != MAGIC)
    local_abort("Freeing unallocated memory");
  ctrl->magic = MAGIC_FREED;
  free(ctrl->raw);
}
