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

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// FFI types
//----------------------------------------------------------------------------------------------------------------------
#define IMPLEMENT_FCMP_FFI_TYPE(raw_t, cpp_fn, rust_fn, destroy_fn)                        \
    void raw_t##Deleter::operator()(raw_t##Unsafe *p) const noexcept { ::destroy_fn(p); }; \
                                                                                           \
    raw_t cpp_fn                                                                           \
    {                                                                                      \
        ::raw_t##Unsafe* raw_ptr;                                                          \
        CHECK_AND_ASSERT_THROW_MES(::rust_fn == 0, "Failed " << #rust_fn);                 \
        return raw_t(raw_ptr);                                                             \
    };

#define IMPLEMENT_FCMP_FFI_SHARED_TYPE(raw_t, cpp_fn1, rust_fn1, cpp_fn2, rust_fn2, destroy_fn) \
    raw_t cpp_fn1                                                                               \
    {                                                                                           \
        ::raw_t##Unsafe* raw_ptr;                                                               \
        CHECK_AND_ASSERT_THROW_MES(::rust_fn1 == 0, "Failed " << #rust_fn1);                    \
        return raw_t(raw_ptr, ::destroy_fn);                                                    \
    };                                                                                          \
                                                                                                \
    raw_t cpp_fn2                                                                               \
    {                                                                                           \
        ::raw_t##Unsafe* raw_ptr;                                                               \
        CHECK_AND_ASSERT_THROW_MES(::rust_fn2 == 0, "Failed " << #rust_fn2);                    \
        return raw_t(raw_ptr, ::destroy_fn);                                                    \
    };

// Branch blinds
IMPLEMENT_FCMP_FFI_TYPE(HeliosBranchBlind,
    gen_helios_branch_blind(),
    generate_helios_branch_blind(&raw_ptr),
    destroy_helios_branch_blind);
IMPLEMENT_FCMP_FFI_TYPE(SeleneBranchBlind,
    gen_selene_branch_blind(),
    generate_selene_branch_blind(&raw_ptr),
    destroy_selene_branch_blind);

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

// Tree root
IMPLEMENT_FCMP_FFI_SHARED_TYPE(TreeRoot,
    helios_tree_root(const HeliosPoint &helios_point),
    helios_tree_root(helios_point, &raw_ptr),
    selene_tree_root(const SelenePoint &selene_point),
    selene_tree_root(selene_point, &raw_ptr),
    destroy_tree_root);

// Path
IMPLEMENT_FCMP_FFI_TYPE(Path,
    path_new(const OutputChunk &output_chunk,
        std::size_t output_idx,
        const HeliosT::ScalarChunks &helios_layer_chunks,
        const SeleneT::ScalarChunks &selene_layer_chunks),
    path_new(output_chunk, output_idx, helios_layer_chunks, selene_layer_chunks, &raw_ptr),
    destroy_path);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
