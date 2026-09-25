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

#include "verify.h"

#include "fcmp_pp_crypto.h"
#include "fcmp_pp_rust/fcmp++.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "fcmp_pp"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool verify(const std::vector<fcmp_pp::FcmpPpVerifyInput> &fcmp_pp_verify_inputs)
{
    MAKE_TEMP_FFI_SLICE(FcmpPpVerifyInput, fcmp_pp_verify_inputs, fcmp_pp_verify_inputs_slice);
    return ::fcmp_pp_verify(fcmp_pp_verify_inputs_slice);
}
//----------------------------------------------------------------------------------------------------------------------
bool verify(const crypto::hash &signable_tx_hash,
    const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRootShared &tree_root,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images)
{
    auto fcmp_pp_verify_input = fcmp_pp::fcmp_pp_verify_input_new(
            signable_tx_hash,
            fcmp_pp_proof,
            n_tree_layers,
            tree_root,
            pseudo_outs,
            key_images
        );
    CHECK_AND_ASSERT_MES(fcmp_pp_verify_input != nullptr, false, "Failed to construct FCMP++ verify input");
    std::vector<fcmp_pp::FcmpPpVerifyInput> fcmp_pp_verify_inputs;
    fcmp_pp_verify_inputs.emplace_back(std::move(fcmp_pp_verify_input));
    return verify(fcmp_pp_verify_inputs);
}
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
