// Copyright (c) 2025, The Monero Project
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

#pragma once

// static assertions
#if defined(__cplusplus) || (__STDC_VERSION__ >= 202311L)
#define FFI_STATIC_ASSERT static_assert
#else
#include <assert.h>
#define FFI_STATIC_ASSERT _Static_assert
#endif

#include <stdalign.h>
#include <stdint.h>


// ----- deps C bindings -----

/// A constant-time implementation of the Ed25519 field.
/// This type is expected to be opaque to the C/C++ side, meaning only the Rust side should read/write
/// its internal represenation. We're using a modified crypto-bigint crate for this type so that we
/// can work with points and scalars across the FFI without tons of byte repr conversions.
struct SeleneScalar {
  uintptr_t _0[32 / sizeof(uintptr_t)];
};
FFI_STATIC_ASSERT(sizeof(struct SeleneScalar) == 32, "SeleneScalar FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct SeleneScalar) == sizeof(uintptr_t), "SeleneScalar FFI type unexpected alignment");

/// The field novel to Helios/Selene.
/// This type is expected to be opaque to the C/C++ side, meaning only the Rust side should read/write
/// its internal represenation. We're using a modified crypto-bigint crate for this type so that we
/// can work with points and scalars across the FFI without tons of byte repr conversions.
struct HeliosScalar {
  uintptr_t _0[32 / sizeof(uintptr_t)];
};
FFI_STATIC_ASSERT(sizeof(struct HeliosScalar) == 32, "HeliosScalar FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct HeliosScalar) == sizeof(uintptr_t), "HeliosScalar FFI type unexpected alignment");

struct HeliosPoint {
  struct SeleneScalar x;
  struct SeleneScalar y;
  struct SeleneScalar z;
};
FFI_STATIC_ASSERT(sizeof(struct HeliosPoint) == 32*3, "HeliosPoint FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct HeliosPoint) == sizeof(uintptr_t), "HeliosPoint FFI type unexpected alignment");

struct SelenePoint {
  struct HeliosScalar x;
  struct HeliosScalar y;
  struct HeliosScalar z;
};
FFI_STATIC_ASSERT(sizeof(struct SelenePoint) == 32*3, "SelenePoint FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct SelenePoint) == sizeof(uintptr_t), "SelenePoint FFI type unexpected alignment");

struct OutputTuple
{
  uint8_t O[32];
  uint8_t I[32];
  uint8_t C[32];
};
FFI_STATIC_ASSERT(sizeof(struct OutputTuple) == 32*3, "OutputTuple FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct OutputTuple) == 1, "OutputTuple FFI type unexpected alignment");

struct HeliosScalarSlice
{
  const struct HeliosScalar *buf;
  uintptr_t len;
};
FFI_STATIC_ASSERT(sizeof(struct HeliosScalarSlice) == sizeof(struct HeliosScalar*)+sizeof(uintptr_t),
    "HeliosScalarSlice FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct HeliosScalarSlice) == sizeof(uintptr_t),
    "HeliosScalarSlice FFI type unexpected alignment");

struct SeleneScalarSlice
{
  const struct SeleneScalar *buf;
  uintptr_t len;
};
FFI_STATIC_ASSERT(sizeof(struct SeleneScalarSlice) == sizeof(struct SeleneScalar*)+sizeof(uintptr_t),
    "SeleneScalarSlice FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct SeleneScalarSlice) == sizeof(uintptr_t),
    "SeleneScalarSlice FFI type unexpected alignment");

// ----- End deps C bindings -----

#ifdef __cplusplus
extern "C" {
#endif

int selene_scalar_from_bytes(const uint8_t *selene_scalar_bytes, struct SeleneScalar *selene_scalar_out);

struct HeliosPoint helios_hash_init_point(void);

struct SelenePoint selene_hash_init_point(void);

struct HeliosScalar helios_zero_scalar(void);

struct SeleneScalar selene_zero_scalar(void);

void helios_scalar_to_bytes(const struct HeliosScalar *helios_scalar, uint8_t bytes_out[32]);

void selene_scalar_to_bytes(const struct SeleneScalar *selene_scalar, uint8_t bytes_out[32]);

void helios_point_to_bytes(const struct HeliosPoint *helios_point, uint8_t bytes_out[32]);

void selene_point_to_bytes(const struct SelenePoint *selene_point, uint8_t bytes_out[32]);

int hash_grow_helios(struct HeliosPoint existing_hash,
                                             uintptr_t offset,
                                             struct HeliosScalar existing_child_at_offset,
                                             struct HeliosScalarSlice new_children,
                                             struct HeliosPoint *hash_out);

int hash_grow_selene(struct SelenePoint existing_hash,
                                             uintptr_t offset,
                                             struct SeleneScalar existing_child_at_offset,
                                             struct SeleneScalarSlice new_children,
                                             struct SelenePoint *hash_out);

#ifdef __cplusplus
} //extern "C"
#endif
