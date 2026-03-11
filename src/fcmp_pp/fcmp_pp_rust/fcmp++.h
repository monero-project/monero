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

#include <stdint.h>


// ----- deps C bindings -----

/// A constant-time implementation of the Ed25519 field.
struct SeleneScalar {
  uintptr_t _0[32 / sizeof(uintptr_t)];
};

/// The field novel to Helios/Selene.
struct HeliosScalar {
  uintptr_t _0[32 / sizeof(uintptr_t)];
};

struct HeliosPoint {
  struct SeleneScalar x;
  struct SeleneScalar y;
  struct SeleneScalar z;
};

struct SelenePoint {
  struct HeliosScalar x;
  struct HeliosScalar y;
  struct HeliosScalar z;
};

// ----- End deps C bindings -----

struct OutputTuple
{
  uint8_t O[32];
  uint8_t I[32];
  uint8_t C[32];
};

struct HeliosScalarSlice
{
  const struct HeliosScalar *buf;
  uintptr_t len;
};

struct SeleneScalarSlice
{
  const struct SeleneScalar *buf;
  uintptr_t len;
};

struct HeliosScalarChunks
{
  const struct HeliosScalarSlice *buf;
  uintptr_t len;
};

struct SeleneScalarChunks
{
  const struct SeleneScalarSlice *buf;
  uintptr_t len;
};

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
