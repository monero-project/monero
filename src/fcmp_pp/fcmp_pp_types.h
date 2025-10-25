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

#include <memory>
#include <vector>

#include "crypto/crypto.h"
#include "fcmp_pp_rust/fcmp++.h"

// TODO: consolidate more FCMP++ types into this file

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
    using ScalarChunks = ::SeleneScalarChunks;
};
//----------------------------------------------------------------------------------------------------------------------
struct HeliosT final
{
    using Scalar       = HeliosScalar;
    using Point        = ::HeliosPoint;
    using Chunk        = ::HeliosScalarSlice;
    using CycleScalar  = SeleneScalar;
    using ScalarChunks = ::HeliosScalarChunks;
};
//----------------------------------------------------------------------------------------------------------------------
using OutputBytes = ::OutputBytes;
using OutputChunk = ::OutputSlice;
//----------------------------------------------------------------------------------------------------------------------
// Define C++ type here so it can be used in FFI types
using FcmpPpProof = std::vector<uint8_t>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// FFI types
//----------------------------------------------------------------------------------------------------------------------
// FFI types instantiated on the Rust side must be destroyed back on the Rust side. We wrap them in a unique ptr with a
// custom deleter that calls the respective Rust destroy fn.
#define DEFINE_FCMP_FFI_TYPE(raw_t, cpp_fn)                                      \
    struct raw_t##Deleter { void operator()(raw_t##Unsafe *p) const noexcept; }; \
    using raw_t = std::unique_ptr<raw_t##Unsafe, raw_t##Deleter>;                \
    raw_t cpp_fn;

// Macro to instantiate an FFI-compatible slice from a vector of FCMP FFI type. Instantiates a vector in local scope
// so it remains in scope while the slice points to it, making sure memory addresses remain contiguous. The slice is
// only usable within local scope, hence "TEMP".
#define MAKE_TEMP_FFI_SLICE(raw_t, vec, slice_name)                              \
    std::vector<const raw_t##Unsafe *> raw_t##Vector;                            \
    raw_t##Vector.reserve(vec.size());                                           \
    for (const raw_t &elem : vec)                                                \
        raw_t##Vector.push_back(elem.get());                                     \
    ::raw_t##SliceUnsafe slice_name{raw_t##Vector.data(), raw_t##Vector.size()};

DEFINE_FCMP_FFI_TYPE(HeliosBranchBlind, gen_helios_branch_blind());
DEFINE_FCMP_FFI_TYPE(SeleneBranchBlind, gen_selene_branch_blind());

DEFINE_FCMP_FFI_TYPE(BlindedOBlind, blind_o_blind(const SeleneScalar &));
DEFINE_FCMP_FFI_TYPE(BlindedIBlind, blind_i_blind(const SeleneScalar &));
DEFINE_FCMP_FFI_TYPE(BlindedIBlindBlind, blind_i_blind_blind(const SeleneScalar &));
DEFINE_FCMP_FFI_TYPE(BlindedCBlind, blind_c_blind(const SeleneScalar &));

DEFINE_FCMP_FFI_TYPE(OutputBlinds,
    output_blinds_new(const BlindedOBlind &, const BlindedIBlind &, const BlindedIBlindBlind &, const BlindedCBlind &));

// Use a shared pointer so we can reference the same underlying tree root in multiple places
using TreeRootShared = std::shared_ptr<TreeRootUnsafe>;
TreeRootShared helios_tree_root(const HeliosPoint &);
TreeRootShared selene_tree_root(const SelenePoint &);

DEFINE_FCMP_FFI_TYPE(Path,
    path_new(const OutputChunk &, std::size_t, const HeliosT::ScalarChunks &, const SeleneT::ScalarChunks &));

DEFINE_FCMP_FFI_TYPE(FcmpPpProveMembershipInput,
    fcmp_pp_prove_input_new(const Path &,
        const OutputBlinds &,
        const std::vector<SeleneBranchBlind> &,
        const std::vector<HeliosBranchBlind> &));

DEFINE_FCMP_FFI_TYPE(FcmpPpVerifyInput,
    fcmp_pp_verify_input_new(const crypto::hash &signable_tx_hash,
        const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
        const std::size_t n_tree_layers,
        const fcmp_pp::TreeRootShared &tree_root,
        const std::vector<crypto::ec_point> &pseudo_outs,
        const std::vector<crypto::key_image> &key_images));
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// C++ types
//----------------------------------------------------------------------------------------------------------------------
// Byte buffer containing the fcmp++ proof
using FcmpPpSalProof = std::vector<uint8_t>;
using FcmpMembershipProof = std::vector<uint8_t>;

struct ProofInput final
{
    Path path;
    OutputBlinds output_blinds;
    std::vector<SeleneBranchBlind> selene_branch_blinds;
    std::vector<HeliosBranchBlind> helios_branch_blinds;
};

struct ProofParams final
{
    uint64_t reference_block;
    std::vector<ProofInput> proof_inputs;
};

struct FcmpVerifyHelperData final
{
    TreeRootShared tree_root;
    std::vector<crypto::key_image> key_images;
};

// Serialize types into a single byte buffer
FcmpPpProof fcmp_pp_proof_from_parts_v1(const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const std::vector<FcmpPpSalProof> &sal_proofs,
    const FcmpMembershipProof &membership_proof,
    const std::uint8_t n_tree_layers);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
