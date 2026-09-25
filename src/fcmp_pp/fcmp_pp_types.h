// Copyright (c) 2024, The Monero Project
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

#include <cstring>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "crypto/crypto.h"
#include "fcmp_pp_rust/fcmp++.h"


namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Rust types
//----------------------------------------------------------------------------------------------------------------------
using SeleneScalar = ::SeleneScalar;
static_assert(sizeof(SeleneScalar) == 32, "unexpected size of selene scalar");
using HeliosScalar = ::HeliosScalar;
static_assert(sizeof(HeliosScalar) == 32, "unexpected size of helios scalar");
//----------------------------------------------------------------------------------------------------------------------
struct SeleneT final
{
    using Scalar       = SeleneScalar;
    using Point        = ::SelenePoint;
    using Chunk        = ::SeleneScalarSlice;
    using CycleScalar  = HeliosScalar;
};
//----------------------------------------------------------------------------------------------------------------------
struct HeliosT final
{
    using Scalar       = HeliosScalar;
    using Point        = ::HeliosPoint;
    using Chunk        = ::HeliosScalarSlice;
    using CycleScalar  = SeleneScalar;
};
//----------------------------------------------------------------------------------------------------------------------
using OutputTuple = ::OutputTuple;
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_tuple_from_bytes(const crypto::ec_point &O, const crypto::ec_point &I, const crypto::ec_point &C);
//----------------------------------------------------------------------------------------------------------------------
// Define FCMP++ prove/verify C++ type here so it can be used in FFI types
using FcmpPpProof = std::vector<uint8_t>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// FFI types
//----------------------------------------------------------------------------------------------------------------------
// Macro to instantiate an FFI-compatible slice from a vector of FCMP FFI type. Instantiates a vector in local scope
// so it remains in scope while the slice points to it, making sure memory addresses remain contiguous. The slice is
// only usable within local scope, hence "TEMP".
#define MAKE_TEMP_FFI_SLICE(raw_t, vec, slice_name)                              \
    std::vector<const raw_t##Unsafe *> raw_t##Vector;                            \
    raw_t##Vector.reserve(vec.size());                                           \
    for (const raw_t &elem : vec)                                                \
        raw_t##Vector.push_back(elem.get());                                     \
    ::raw_t##SliceUnsafe slice_name{raw_t##Vector.data(), raw_t##Vector.size()};

// Use a shared pointer so we can reference the same underlying tree root in multiple places
using TreeRootShared = std::shared_ptr<TreeRootUnsafe>;
TreeRootShared helios_tree_root(const HeliosPoint &);
TreeRootShared selene_tree_root(const SelenePoint &);

struct FcmpPpVerifyInputDeleter { void operator()(FcmpPpVerifyInputUnsafe *p) const noexcept; };
using FcmpPpVerifyInput = std::unique_ptr<FcmpPpVerifyInputUnsafe, FcmpPpVerifyInputDeleter>;
FcmpPpVerifyInput fcmp_pp_verify_input_new(const crypto::hash &signable_tx_hash,
        const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
        const std::size_t n_tree_layers,
        const fcmp_pp::TreeRootShared &tree_root,
        const std::vector<crypto::ec_point> &pseudo_outs,
        const std::vector<crypto::key_image> &key_images);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// C++ types
//----------------------------------------------------------------------------------------------------------------------
//   Curve trees types
//----------------------------------------------------------------------------------------------------------------------
// Output pubkey and commitment, ready to be converted to a leaf tuple
// - From {output_pubkey,commitment} -> {O,C} -> {O,I,C} -> {O.x,O.y,I.x,I.y,C.x,C.y}
// - Output pairs do NOT necessarily have torsion cleared. We need the output pubkey as it exists in the chain in order
//   to derive the correct I (when deriving {O.x,O.y,I.x,I.y,C.x,C.y}). Torsion clearing O before deriving I from O
//   would enable spending a torsioned output once before FCMP++ fork and again with a different key image via FCMP++.
template<typename T>
struct OutputPairTemplate
{
    crypto::public_key output_pubkey;
    // Uses the ec_point type to avoid a circular dep to ringct/rctTypes.h, and to differentiate from output_pubkey
    crypto::ec_point commitment;

    OutputPairTemplate(const crypto::public_key &_output_pubkey, const crypto::ec_point &_commitment):
        output_pubkey(_output_pubkey),
        commitment(_commitment)
    {};

    OutputPairTemplate():
        output_pubkey{},
        commitment{}
    {};

    // WARNING: not constant time
    bool operator==(const OutputPairTemplate &other) const
    {
        return output_pubkey == other.output_pubkey
            && commitment == other.commitment;
    }
};

// May have torsion, use biased key image generator for I
struct LegacyOutputPair : public OutputPairTemplate<LegacyOutputPair>{};
// No torsion, use unbiased key image generator for I
struct CarrotOutputPairV1 : public OutputPairTemplate<CarrotOutputPairV1>{};

static_assert(sizeof(LegacyOutputPair)   == (32+32), "sizeof LegacyOutputPair unexpected");
static_assert(sizeof(CarrotOutputPairV1) == (32+32), "sizeof CarrotOutputPairV1 unexpected");

static_assert(std::has_unique_object_representations_v<LegacyOutputPair>);
static_assert(std::has_unique_object_representations_v<CarrotOutputPairV1>);

using OutputPair = std::variant<LegacyOutputPair, CarrotOutputPairV1>;

const crypto::public_key &output_pubkey_cref(const OutputPair &output_pair);
const crypto::ec_point &commitment_cref(const OutputPair &output_pair);

bool output_checked_for_torsion(const OutputPair &output_pair);
bool use_biased_hash_to_point(const OutputPair &output_pair);

// Wrapper for outputs with context to insert the output into the FCMP++ curve tree
struct UnifiedOutput final
{
    // Output's unique id in the chain.
    // When a batch of outputs is added to the tree, the batch
    // is internally sorted by unique ids, which correspond with
    // the order the outputs entered the chain. Batch-added outputs
    // may not be globally ordered because e.g. timelocked
    // outputs were added to 'late batches'. Batch refers to
    // outputs that become unlocked/spendable at the same time.
    uint64_t unified_id{0};
    OutputPair output_pair;

    bool operator==(const UnifiedOutput &other) const
    {
        return unified_id == other.unified_id && output_pair == other.output_pair;
    }
};

#define SIZEOF_SERIALIZED_UNIFIED_OUTPUT 73 // 8+1+32+32

// Contiguous leaves in the tree, starting at a specified start_idx in the leaf layer
struct ContiguousLeaves final
{
    // Starting leaf tuple index in the leaf layer
    uint64_t                   start_idx{0};
    // Contiguous leaves in a tree that start at the start_idx
    std::vector<UnifiedOutput> tuples;
};
//----------------------------------------------------------------------------------------------------------------------
//   FCMP++ prove/verify types
//----------------------------------------------------------------------------------------------------------------------
// Size of the membership proof alone
std::size_t membership_proof_len(const std::size_t n_inputs, const uint8_t n_layers);

// Size of the FCMP++ proof (membership proof + spend-auth + linkability proofs & input tuples)
std::size_t fcmp_pp_proof_len(const std::size_t n_inputs, const uint8_t n_layers);

// Get the number of inputs included in the FCMP++ verify input
std::size_t n_inputs_in_fcmp_pp(const FcmpPpVerifyInput &fcmp_pp_verify_input);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp

// WARNING: not constant time
inline bool operator==(const fcmp_pp::OutputTuple &a, const fcmp_pp::OutputTuple &b)
{
    static_assert(sizeof(fcmp_pp::OutputTuple) == (sizeof(a.O) + sizeof(a.I) + sizeof(a.C)),
        "unexpected sizeof OutputTuple for == implementation");
    return
        (memcmp(a.O, b.O, sizeof(a.O)) == 0) &&
        (memcmp(a.I, b.I, sizeof(a.I)) == 0) &&
        (memcmp(a.C, b.C, sizeof(a.C)) == 0);
}
