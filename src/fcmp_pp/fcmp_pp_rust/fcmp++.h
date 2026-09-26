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
#include <stdbool.h>
#include <stdint.h>

// https://github.com/monero-oxide/monero-oxide/blob/77788c368145127f2dde2ac3e2ddce919f3ddd01/monero-oxide/ringct/fcmp%2B%2B/src/lib.rs#L188-L189
#define FCMP_PP_SAL_PROOF_SIZE_V1 (12*32)
#define FCMP_PP_INPUT_TUPLE_SIZE_V1 (3*32)

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

struct ObjectSlice
{
  const uint8_t * const *buf;
  uintptr_t len;
};

// Tiny types that are expected to be a Rust Box of the underlying object, allocated on the Rust side.
// These must be destroyed manually on the Rust side of the boundary.
struct TreeRootUnsafe;

struct FcmpPpVerifyInputUnsafe;
struct FcmpPpVerifyInputSliceUnsafe
{
  const struct FcmpPpVerifyInputUnsafe * const *buf;
  uintptr_t len;
};

// ----- End deps C bindings -----

#ifdef __cplusplus
extern "C" {
#endif

int selene_scalar_from_bytes(const uint8_t *selene_scalar_bytes, struct SeleneScalar *selene_scalar_out);

struct HeliosPoint helios_hash_init_point(void);

struct SelenePoint selene_hash_init_point(void);

int selene_point_to_helios_scalar(struct SelenePoint selene_point, struct HeliosScalar *helios_scalar_out);

int helios_point_to_selene_scalar(struct HeliosPoint helios_point, struct SeleneScalar *selene_scalar_out);

struct HeliosScalar helios_zero_scalar(void);

struct SeleneScalar selene_zero_scalar(void);

int selene_tree_root(struct SelenePoint selene_point, struct TreeRootUnsafe **tree_root_out);
int helios_tree_root(struct HeliosPoint helios_point, struct TreeRootUnsafe **tree_root_out);

void destroy_tree_root(struct TreeRootUnsafe *tree_root);

void helios_scalar_to_bytes(const struct HeliosScalar *helios_scalar, uint8_t bytes_out[32]);

void selene_scalar_to_bytes(const struct SeleneScalar *selene_scalar, uint8_t bytes_out[32]);

void helios_point_to_bytes(const struct HeliosPoint *helios_point, uint8_t bytes_out[32]);

void selene_point_to_bytes(const struct SelenePoint *selene_point, uint8_t bytes_out[32]);

int helios_point_from_bytes(const uint8_t *helios_point_bytes, struct HeliosPoint *helios_point_out);

int selene_point_from_bytes(const uint8_t *selene_point_bytes, struct SelenePoint *selene_point_out);

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

uintptr_t membership_proof_size(uintptr_t n_inputs, uintptr_t n_tree_layers);

uintptr_t fcmp_pp_proof_size(uintptr_t n_inputs, uintptr_t n_tree_layers);

int fcmp_pp_verify_input_new(const uint8_t *signable_tx_hash,
                                             const uint8_t *fcmp_pp_proof,
                                             uintptr_t fcmp_pp_proof_len,
                                             uintptr_t n_tree_layers,
                                             const struct TreeRootUnsafe *tree_root,
                                             struct ObjectSlice pseudo_outs,
                                             struct ObjectSlice key_images,
                                             struct FcmpPpVerifyInputUnsafe **fcmp_pp_verify_input_out);

void destroy_fcmp_pp_verify_input(struct FcmpPpVerifyInputUnsafe *fcmp_pp_verify_input);

uintptr_t fcmp_pp_n_inputs(const struct FcmpPpVerifyInputUnsafe *fcmp_pp_verify_input);

bool fcmp_pp_verify(const struct FcmpPpVerifyInputSliceUnsafe fcmp_pp_verify_inputs);

#ifdef __cplusplus
} //extern "C"
#endif
