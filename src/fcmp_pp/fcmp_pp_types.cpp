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

#include "fcmp_pp_types.h"

#include "misc_log_ex.h"
#include "proof_len.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// FFI types
//----------------------------------------------------------------------------------------------------------------------
#define CHECK_FFI_RES \
    CHECK_AND_ASSERT_THROW_MES(r == 0, __func__ << " failed with error code " << r);

#define IMPLEMENT_FCMP_FFI_TYPE(raw_t, cpp_fn, rust_fn, destroy_fn)                        \
    void raw_t##Deleter::operator()(raw_t##Unsafe *p) const noexcept { ::destroy_fn(p); }; \
                                                                                           \
    raw_t cpp_fn                                                                           \
    {                                                                                      \
        ::raw_t##Unsafe* raw_ptr;                                                          \
        int r = ::rust_fn;                                                                 \
        CHECK_FFI_RES;                                                                     \
        return raw_t(raw_ptr);                                                             \
    };

#define IMPLEMENT_FCMP_FFI_SHARED_TYPE(raw_t, cpp_fn1, rust_fn1, cpp_fn2, rust_fn2, destroy_fn) \
    raw_t cpp_fn1                                                                               \
    {                                                                                           \
        ::raw_t##Unsafe* raw_ptr;                                                               \
        int r = ::rust_fn1;                                                                     \
        CHECK_FFI_RES;                                                                          \
        return raw_t(raw_ptr, ::destroy_fn);                                                    \
    };                                                                                          \
                                                                                                \
    raw_t cpp_fn2                                                                               \
    {                                                                                           \
        ::raw_t##Unsafe* raw_ptr;                                                               \
        int r = ::rust_fn2;                                                                     \
        CHECK_FFI_RES;                                                                          \
        return raw_t(raw_ptr, ::destroy_fn);                                                    \
    };
//----------------------------------------------------------------------------------------------------------------------
// Branch blinds
IMPLEMENT_FCMP_FFI_TYPE(HeliosBranchBlind,
    gen_helios_branch_blind(),
    generate_helios_branch_blind(&raw_ptr),
    destroy_helios_branch_blind);
IMPLEMENT_FCMP_FFI_TYPE(SeleneBranchBlind,
    gen_selene_branch_blind(),
    generate_selene_branch_blind(&raw_ptr),
    destroy_selene_branch_blind);
//----------------------------------------------------------------------------------------------------------------------
// Blinded blinds
IMPLEMENT_FCMP_FFI_TYPE(BlindedOBlind,
    blind_o_blind(const SeleneScalar &o_blind),
    blind_o_blind(&o_blind, &raw_ptr),
    destroy_blinded_o_blind);
IMPLEMENT_FCMP_FFI_TYPE(BlindedIBlind,
    blind_i_blind(const SeleneScalar &i_blind),
    blind_i_blind(&i_blind, &raw_ptr),
    destroy_blinded_i_blind);
IMPLEMENT_FCMP_FFI_TYPE(BlindedIBlindBlind,
    blind_i_blind_blind(const SeleneScalar &i_blind_blind),
    blind_i_blind_blind(&i_blind_blind, &raw_ptr),
    destroy_blinded_i_blind_blind);
IMPLEMENT_FCMP_FFI_TYPE(BlindedCBlind,
    blind_c_blind(const SeleneScalar &c_blind),
    blind_c_blind(&c_blind, &raw_ptr),
    destroy_blinded_c_blind);
//----------------------------------------------------------------------------------------------------------------------
// Output blinds
IMPLEMENT_FCMP_FFI_TYPE(OutputBlinds,
    output_blinds_new(const BlindedOBlind &blinded_o_blind,
        const BlindedIBlind &blinded_i_blind,
        const BlindedIBlindBlind &blinded_i_blind_blind,
        const BlindedCBlind &blinded_c_blind),
    output_blinds_new(blinded_o_blind.get(),
        blinded_i_blind.get(),
        blinded_i_blind_blind.get(),
        blinded_c_blind.get(),
        &raw_ptr),
    destroy_output_blinds);
//----------------------------------------------------------------------------------------------------------------------
// Tree root
IMPLEMENT_FCMP_FFI_SHARED_TYPE(TreeRoot,
    helios_tree_root(const HeliosPoint &helios_point),
    helios_tree_root(helios_point, &raw_ptr),
    selene_tree_root(const SelenePoint &selene_point),
    selene_tree_root(selene_point, &raw_ptr),
    destroy_tree_root);
//----------------------------------------------------------------------------------------------------------------------
// Path
IMPLEMENT_FCMP_FFI_TYPE(Path,
    path_new(const OutputChunk &output_chunk,
        std::size_t output_idx,
        const HeliosT::ScalarChunks &helios_layer_chunks,
        const SeleneT::ScalarChunks &selene_layer_chunks),
    path_new(output_chunk, output_idx, helios_layer_chunks, selene_layer_chunks, &raw_ptr),
    destroy_path);
//----------------------------------------------------------------------------------------------------------------------
// Implement FCMP++ Prove Input manually because will need temp slices
void FcmpPpProveInputDeleter::operator()(FcmpPpProveInputUnsafe *p) const noexcept { ::destroy_fcmp_pp_prove_input(p); };

FcmpPpProveInput fcmp_pp_prove_input_new(const Path &path,
        const OutputBlinds &output_blinds,
        const std::vector<SeleneBranchBlind> &selene_branch_blinds,
        const std::vector<HeliosBranchBlind> &helios_branch_blinds)
{
    MAKE_TEMP_FFI_SLICE(SeleneBranchBlind, selene_branch_blinds, selene_branch_blind_slice);
    MAKE_TEMP_FFI_SLICE(HeliosBranchBlind, helios_branch_blinds, helios_branch_blind_slice);

    FcmpPpProveInputUnsafe *raw_ptr;
    int r = ::fcmp_pp_prove_input_new(path.get(),
        output_blinds.get(),
        selene_branch_blind_slice,
        helios_branch_blind_slice,
        &raw_ptr);
    CHECK_FFI_RES;

    return FcmpPpProveInput(raw_ptr);
}
//----------------------------------------------------------------------------------------------------------------------
// Implement FCMP++ Verify Input manually because need to type cast
void FcmpPpVerifyInputDeleter::operator()(FcmpPpVerifyInputUnsafe *p) const noexcept { ::destroy_fcmp_pp_verify_input(p); };

FcmpPpVerifyInput fcmp_pp_verify_input_new(const crypto::hash &signable_tx_hash,
        const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
        const std::size_t n_tree_layers,
        const fcmp_pp::TreeRoot &tree_root,
        const std::vector<crypto::ec_point> &pseudo_outs,
        const std::vector<crypto::key_image> &key_images)
{
    // Cast pseudo outs to vector of const uint8_t*
    std::vector<const uint8_t *> pseudo_outs_ptrs;
    pseudo_outs_ptrs.reserve(pseudo_outs.size());
    for (const auto &po : pseudo_outs)
        pseudo_outs_ptrs.emplace_back((const uint8_t *)&po);

    // Cast key images to vector const uint8_t*
    std::vector<const uint8_t *> key_images_ptrs;
    key_images_ptrs.reserve(key_images.size());
    for (const auto &ki : key_images)
        key_images_ptrs.emplace_back((const uint8_t *)&ki.data);

    FcmpPpVerifyInputUnsafe *raw_ptr;
    int r = ::fcmp_pp_verify_input_new(
            reinterpret_cast<const uint8_t*>(&signable_tx_hash),
            fcmp_pp_proof.data(),
            fcmp_pp_proof.size(),
            n_tree_layers,
            tree_root.get(),
            {pseudo_outs_ptrs.data(), pseudo_outs_ptrs.size()},
            {key_images_ptrs.data(), key_images_ptrs.size()},
            &raw_ptr
        );
    CHECK_FFI_RES;

    return FcmpPpVerifyInput(raw_ptr);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
fcmp_pp::FcmpPpProof fcmp_pp_proof_from_parts_v1(
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const std::vector<fcmp_pp::FcmpPpSalProof> &sal_proofs,
    const fcmp_pp::FcmpMembershipProof &membership_proof,
    const std::uint8_t n_tree_layers)
{
    const size_t n_inputs = rerandomized_outputs.size();
    CHECK_AND_ASSERT_THROW_MES(sal_proofs.size() == n_inputs,
        "fcmp_pp_proof_from_parts_v1: wrong number of sal_proofs");
    for (const fcmp_pp::FcmpPpSalProof &sal_proof : sal_proofs)
        CHECK_AND_ASSERT_THROW_MES(sal_proof.size() == FCMP_PP_SAL_PROOF_SIZE_V1,
        "fcmp_pp_proof_from_parts_v1: sal proof is incorrect size");
    CHECK_AND_ASSERT_THROW_MES(membership_proof.size() == fcmp_pp::membership_proof_len(n_inputs, n_tree_layers),
        "fcmp_pp_proof_from_parts_v1: membership proof is incorrect size");
    const size_t actual_proof_size = membership_proof.size() +
        (FCMP_PP_INPUT_TUPLE_SIZE_V1 + FCMP_PP_SAL_PROOF_SIZE_V1) * n_inputs;
    CHECK_AND_ASSERT_THROW_MES(actual_proof_size == fcmp_pp::fcmp_pp_proof_len(n_inputs, n_tree_layers),
        "fcmp_pp_proof_from_parts_v1: bug: bad length calculation");

    // build FCMP++ from FCMP and SA/L parts
    fcmp_pp::FcmpPpProof proof_bytes;
    proof_bytes.reserve(actual_proof_size);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const FcmpInputCompressed &input = rerandomized_outputs.at(i).input;
        const fcmp_pp::FcmpPpSalProof &sal_proof = sal_proofs.at(i);

        // append O~, I~, R                                  (C_tilde not included)
        proof_bytes.insert(proof_bytes.end(), input.O_tilde, input.C_tilde);
        // append SAL proof
        proof_bytes.insert(proof_bytes.end(), sal_proof.cbegin(), sal_proof.cend());
    }
    // append membership proof
    proof_bytes.insert(proof_bytes.end(), membership_proof.cbegin(), membership_proof.cend());

    CHECK_AND_ASSERT_THROW_MES(proof_bytes.size() == actual_proof_size,
        "fcmp_pp_proof_from_parts_v1: bug: bad proof building");

    return proof_bytes;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
