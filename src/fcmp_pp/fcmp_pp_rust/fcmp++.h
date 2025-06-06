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
#define FCMP_STATIC_ASSERT static_assert
#else
#include <assert.h>
#define FCMP_STATIC_ASSERT _Static_assert
#endif

#include <stdbool.h>
#include <stdint.h>

// https://github.com/kayabaNerve/fcmp-plus-plus/blob/78754718faa21f0a5751fbd30c9495d7f7f5c2b1/networks/monero/ringct/fcmp%2B%2B/src/lib.rs#L273-L274
#define FCMP_PP_SAL_PROOF_SIZE_V1 (12*32)
#define FCMP_PP_INPUT_TUPLE_SIZE_V1 (3*32)


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

typedef struct CResult {
  void* value;
  void* err;
} CResult;

struct OutputBytes {
  const uint8_t *O_bytes;
  const uint8_t *I_bytes;
  const uint8_t *C_bytes;
};

struct FcmpInputCompressed
{
  uint8_t O_tilde[32];
  uint8_t I_tilde[32];
  uint8_t R[32];
  uint8_t C_tilde[32];
};
FCMP_STATIC_ASSERT(sizeof(struct FcmpInputCompressed) == 4 * 32,
  "FcmpInputCompressed has padding and thus cannot be treated as a byte buffer");

struct FcmpRerandomizedOutputCompressed
{
  struct FcmpInputCompressed input;

  uint8_t r_o[32];
  uint8_t r_i[32];
  uint8_t r_r_i[32];
  uint8_t r_c[32];
};
FCMP_STATIC_ASSERT(sizeof(struct FcmpRerandomizedOutputCompressed) == 8 * 32,
  "RerandomizedOutputCompressed has padding and thus cannot be treated as a byte buffer");

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

struct OutputSlice
{
  const struct OutputBytes *buf;
  uintptr_t len;
};

struct InputSlice
{
  const struct FcmpInputCompressed *buf;
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

struct ObjectSlice
{
  const uint8_t * const *buf;
  uintptr_t len;
};

struct HeliosBranchBlindUnsafe;
struct SeleneBranchBlindUnsafe;

struct BlindedOBlindUnsafe;
struct BlindedIBlindUnsafe;
struct BlindedIBlindBlindUnsafe;
struct BlindedCBlindUnsafe;

struct HeliosBranchBlindSliceUnsafe
{
  const struct HeliosBranchBlindUnsafe * const *buf;
  uintptr_t len;
};

struct SeleneBranchBlindSliceUnsafe
{
  const struct SeleneBranchBlindUnsafe * const *buf;
  uintptr_t len;
};

#ifdef __cplusplus
extern "C" {
#endif

struct HeliosPoint helios_hash_init_point(void);

struct SelenePoint selene_hash_init_point(void);

void helios_scalar_to_bytes(const struct HeliosScalar *helios_scalar, uint8_t bytes_out[32]);

void selene_scalar_to_bytes(const struct SeleneScalar *selene_scalar, uint8_t bytes_out[32]);

void helios_point_to_bytes(const struct HeliosPoint *helios_point, uint8_t bytes_out[32]);

void selene_point_to_bytes(const struct SelenePoint *selene_point, uint8_t bytes_out[32]);

struct HeliosPoint helios_point_from_bytes(const uint8_t *helios_point_bytes);

struct SelenePoint selene_point_from_bytes(const uint8_t *selene_point_bytes);

struct SeleneScalar selene_scalar_from_bytes(const uint8_t *selene_scalar_bytes);

struct HeliosScalar selene_point_to_helios_scalar(struct SelenePoint selene_point);

struct SeleneScalar helios_point_to_selene_scalar(struct HeliosPoint helios_point);

struct HeliosScalar helios_zero_scalar(void);

struct SeleneScalar selene_zero_scalar(void);

uint8_t *selene_tree_root(struct SelenePoint selene_point);

uint8_t *helios_tree_root(struct HeliosPoint helios_point);

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

CResult path_new(struct OutputSlice leaves,
                                             uintptr_t output_idx,
                                             struct HeliosScalarChunks helios_layer_chunks,
                                             struct SeleneScalarChunks selene_layer_chunks);

int rerandomize_output(struct OutputBytes output,
                                            struct FcmpRerandomizedOutputCompressed *rerandomized_output_out);

int o_blind(const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
  struct SeleneScalar *o_blind_out);
int i_blind(const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
  struct SeleneScalar *i_blind_out);
int i_blind_blind(const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
  struct SeleneScalar *i_blind_blind_out);
int c_blind(const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
  struct SeleneScalar *c_blind_out);

int blind_o_blind(const struct SeleneScalar *o_blind, struct BlindedOBlindUnsafe **blinded_o_blind_out);
int blind_i_blind(const struct SeleneScalar *i_blind, struct BlindedIBlindUnsafe **blinded_i_blind_out);
int blind_i_blind_blind(const struct SeleneScalar *i_blind_blind, struct BlindedIBlindBlindUnsafe **blinded_i_blind_blind_out);
int blind_c_blind(const struct SeleneScalar *c_blind, struct BlindedCBlindUnsafe **blinded_c_blind_out);

void destroy_blinded_o_blind(struct BlindedOBlindUnsafe *blinded_o_blind);
void destroy_blinded_i_blind(struct BlindedIBlindUnsafe *blinded_i_blind);
void destroy_blinded_i_blind_blind(struct BlindedIBlindBlindUnsafe *blinded_i_blind_blind);
void destroy_blinded_c_blind(struct BlindedCBlindUnsafe *blinded_c_blind);

CResult output_blinds_new(const uint8_t *o_blind,
                                             const uint8_t *i_blind,
                                             const uint8_t *i_blind_blind,
                                             const uint8_t *c_blind);

int generate_helios_branch_blind(struct HeliosBranchBlindUnsafe **branch_blind_out);
int generate_selene_branch_blind(struct SeleneBranchBlindUnsafe **branch_blind_out);

void destroy_helios_branch_blind(struct HeliosBranchBlindUnsafe *helios_branch_blind);
void destroy_selene_branch_blind(struct SeleneBranchBlindUnsafe *selene_branch_blind);

CResult fcmp_prove_input_new(const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
                                        const uint8_t *path,
                                        const uint8_t *output_blinds,
                                        struct SeleneBranchBlindSliceUnsafe selene_branch_blinds,
                                        struct HeliosBranchBlindSliceUnsafe helios_branch_blinds);

CResult fcmp_pp_prove_input_new(const uint8_t *x,
                                             const uint8_t *y,
                                             const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
                                             const uint8_t *path,
                                             const uint8_t *output_blinds,
                                             struct SeleneBranchBlindSliceUnsafe selene_branch_blinds,
                                             struct HeliosBranchBlindSliceUnsafe helios_branch_blinds);

CResult balance_last_pseudo_out(const uint8_t *sum_input_masks,
                                             const uint8_t *sum_output_masks,
                                             struct ObjectSlice fcmp_prove_inputs);

uint8_t *read_input_pseudo_out(const uint8_t *fcmp_prove_input);

CResult prove(const uint8_t *signable_tx_hash,
                                             struct ObjectSlice fcmp_prove_inputs,
                                             uintptr_t n_tree_layers);

/**
 * brief: fcmp_pp_prove_sal - Make a FCMP++ spend auth & linkability proof
 * param: signable_tx_hash - message to sign
 * param: x - ed25519 scalar s.t. O~ = x G + y T
 * param: y - ed25519 scalar s.t. O~ = x G + y T
 * param: rerandomized_output - used for input tuple, r_i, and r_r_i
 * outparam: sal_proof_out - a buffer of size FCMP_PP_SAL_PROOF_SIZE_V1 where resultant SAL proof is stored
 * return: 0 on success, a negative value on failure
 * 
 * note: This call can technically be stripped down even more because `rerandomized_output` contains
 *       more information than we need: we can discard r_o and r_c. However, in practice, these
 *       values will always be known before a call to this function since O~ and C~ are added to the
 *       challenge transcript, so passing `rerandomized_output` is more ergonomic.
 */
int fcmp_pp_prove_sal(const uint8_t signable_tx_hash[32],
                                             const uint8_t x[32],
                                             const uint8_t y[32],
                                             const struct FcmpRerandomizedOutputCompressed *rerandomized_output,
                                             uint8_t sal_proof_out[FCMP_PP_SAL_PROOF_SIZE_V1],
                                             uint8_t key_image_out[32]);

/**
 * brief: fcmp_pp_prove_membership - Make a FCMP++ membership proof for N inputs
 * param: inputs - a slice of FCMP provable inputs returned from fcmp_prove_input_new()
 * param: n_tree_layers -
 * param: proof_len -
 * outparam: fcmp_proof_out - a buffer where the FCMP proof will be written to
 * outparam: fcmp_proof_out_size - the max length of the buffer fcmp_proof_out, is set to written proof size
 * return: an error on failure, nothing otherwise
 */
CResult fcmp_pp_prove_membership(struct ObjectSlice fcmp_prove_inputs,
                                             uintptr_t n_tree_layers,
                                             uintptr_t proof_len,
                                             uint8_t fcmp_proof_out[],
                                             uintptr_t *fcmp_proof_out_len);

// The following proof_size functions are tabled through proof_len.h. Use
// those functions instead.
uintptr_t _slow_membership_proof_size(uintptr_t n_inputs, uintptr_t n_tree_layers);

uintptr_t _slow_fcmp_pp_proof_size(uintptr_t n_inputs, uintptr_t n_tree_layers);

CResult fcmp_pp_verify_input_new(const uint8_t *signable_tx_hash,
                                             const uint8_t *fcmp_pp_proof,
                                             uintptr_t fcmp_pp_proof_len,
                                             uintptr_t n_tree_layers,
                                             const uint8_t *tree_root,
                                             struct ObjectSlice pseudo_outs,
                                             struct ObjectSlice key_images);

bool verify(const uint8_t *signable_tx_hash,
                                             const uint8_t *fcmp_pp_proof,
                                             uintptr_t fcmp_pp_proof_len,
                                             uintptr_t n_tree_layers,
                                             const uint8_t *tree_root,
                                             struct ObjectSlice pseudo_outs,
                                             struct ObjectSlice key_images);
/**
 * brief: fcmp_pp_verify_sal - Verify a FCMP++ spend auth & linkability proof
 * param: signable_tx_hash - message to verify
 * param: input - (O~, I~, C~, R) tuple
 * param: L - L = x Hp(O), AKA key image
 * param: sal_proof - SAL proof to verify
 * return: true on verification success, false otherwise
 */
bool fcmp_pp_verify_sal(const uint8_t signable_tx_hash[32],
                                             const struct FcmpInputCompressed *input,
                                             const uint8_t L[32],
                                             const uint8_t sal_proof[FCMP_PP_SAL_PROOF_SIZE_V1]);
/**
 * brief: fcmp_pp_verify_membership - Verify a FCMP++ membership proof
 * param: inputs - a slice of fcmp_input_ref pointers
 * param: tree_root -
 * param: n_tree_layers -
 * param: fcmp_proof - FCMP proof bytes to verify
 * param: fcmp_proof_len - length of fcmp_proof buffer
 * return: true on verification success, false otherwise
 */
bool fcmp_pp_verify_membership(struct InputSlice inputs,
  const uint8_t *tree_root,
  const uintptr_t n_tree_layers,
  const uint8_t fcmp_proof[],
  const uintptr_t fcmp_proof_len);

bool fcmp_pp_batch_verify(struct ObjectSlice fcmp_pp_verify_inputs);

#ifdef __cplusplus
} //extern "C"
#endif
