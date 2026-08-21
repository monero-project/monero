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

struct OutputTuple
{
  uint8_t O[32];
  uint8_t I[32];
  uint8_t C[32];
};
FFI_STATIC_ASSERT(sizeof(struct OutputTuple) == 32*3, "OutputTuple FFI type unexpected size");
FFI_STATIC_ASSERT(alignof(struct OutputTuple) == 1, "OutputTuple FFI type unexpected alignment");

// ----- End deps C bindings -----

#ifdef __cplusplus
extern "C" {
#endif

int selene_scalar_from_bytes(const uint8_t *selene_scalar_bytes, struct SeleneScalar *selene_scalar_out);

#ifdef __cplusplus
} //extern "C"
#endif
