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

#include "cryptonote_config.h"
#include "misc_log_ex.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_tuple_from_bytes(const crypto::ec_point &O, const crypto::ec_point &I, const crypto::ec_point &C)
{
    OutputTuple output_tuple;

    static_assert(sizeof(output_tuple.O) == sizeof(O), "unexpected sizeof O");
    static_assert(sizeof(output_tuple.I) == sizeof(I), "unexpected sizeof I");
    static_assert(sizeof(output_tuple.C) == sizeof(C), "unexpected sizeof C");

    memcpy(output_tuple.O, &O, sizeof(O));
    memcpy(output_tuple.I, &I, sizeof(I));
    memcpy(output_tuple.C, &C, sizeof(C));

    return output_tuple;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// FFI types
//----------------------------------------------------------------------------------------------------------------------
#define CHECK_FFI_RES(r) \
    CHECK_AND_ASSERT_THROW_MES(r == 0, __func__ << " failed with error code " << r);

//----------------------------------------------------------------------------------------------------------------------
TreeRootShared helios_tree_root(const HeliosPoint &helios_point)
{
    ::TreeRootUnsafe *raw_ptr;
    CHECK_FFI_RES(::helios_tree_root(helios_point, &raw_ptr));
    return TreeRootShared(raw_ptr, ::destroy_tree_root);
}

TreeRootShared selene_tree_root(const SelenePoint &selene_point)
{
    ::TreeRootUnsafe *raw_ptr;
    CHECK_FFI_RES(::selene_tree_root(selene_point, &raw_ptr));
    return TreeRootShared(raw_ptr, ::destroy_tree_root);
}
//----------------------------------------------------------------------------------------------------------------------
void FcmpPpVerifyInputDeleter::operator()(FcmpPpVerifyInputUnsafe *p) const noexcept
{
    ::destroy_fcmp_pp_verify_input(p);
}

FcmpPpVerifyInput fcmp_pp_verify_input_new(const crypto::hash &signable_tx_hash,
        const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
        const std::size_t n_tree_layers,
        const fcmp_pp::TreeRootShared &tree_root,
        const std::vector<crypto::ec_point> &pseudo_outs,
        const std::vector<crypto::key_image> &key_images)
{
    FcmpPpVerifyInputUnsafe *raw_ptr;
    TRY_ENTRY();

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
    CHECK_FFI_RES(r);
    CATCH_ENTRY("fcmp_pp::fcmp_pp_verify_input_new", nullptr);

    return FcmpPpVerifyInput(raw_ptr);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
std::size_t membership_proof_len(const std::size_t n_inputs, const uint8_t n_layers)
{
    CHECK_AND_ASSERT_THROW_MES(n_inputs > 0, "n_inputs must be >0");
    CHECK_AND_ASSERT_THROW_MES(n_layers > 0, "n_layers must be >0");
    CHECK_AND_ASSERT_THROW_MES(n_inputs <= FCMP_PLUS_PLUS_MAX_INPUTS, "n_inputs must be <= FCMP_PLUS_PLUS_MAX_INPUTS");
    CHECK_AND_ASSERT_THROW_MES(n_layers <= FCMP_PLUS_PLUS_MAX_LAYERS, "n_layers must be <= FCMP_PLUS_PLUS_MAX_LAYERS");

    return ::membership_proof_size(n_inputs, n_layers);
};

std::size_t fcmp_pp_proof_len(const std::size_t n_inputs, const uint8_t n_layers)
{
    // https://github.com/monero-oxide/monero-oxide/blob/77788c368145127f2dde2ac3e2ddce919f3ddd01/monero-oxide/ringct/fcmp%2B%2B/src/lib.rs#L189
    return membership_proof_len(n_inputs, n_layers)
        + (n_inputs * (FCMP_PP_INPUT_TUPLE_SIZE_V1 + FCMP_PP_SAL_PROOF_SIZE_V1));
};
//----------------------------------------------------------------------------------------------------------------------
std::size_t n_inputs_in_fcmp_pp(const FcmpPpVerifyInput &fcmp_pp_verify_input)
{
    return ::fcmp_pp_n_inputs(fcmp_pp_verify_input.get());
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
const crypto::public_key &output_pubkey_cref(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        const crypto::public_key &operator()(const LegacyOutputPair &o) const
        { return o.output_pubkey; }
        const crypto::public_key &operator()(const CarrotOutputPairV1 &o) const
        { return o.output_pubkey; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
const crypto::ec_point &commitment_cref(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        const crypto::ec_point &operator()(const LegacyOutputPair &o) const
        { return o.commitment; }
        const crypto::ec_point &operator()(const CarrotOutputPairV1 &o) const
        { return o.commitment; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
bool output_checked_for_torsion(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        bool operator()(const LegacyOutputPair&) const
        { return false; }
        bool operator()(const CarrotOutputPairV1&) const
        { return true; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
bool use_biased_hash_to_point(const OutputPair &output_pair)
{
    struct output_pair_visitor
    {
        bool operator()(const LegacyOutputPair&) const
        { return true; }
        bool operator()(const CarrotOutputPairV1&) const
        { return false; }
    };

    return std::visit(output_pair_visitor{}, output_pair);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
