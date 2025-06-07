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

#include <cstdint>
#include <vector>

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "fcmp_pp_rust/fcmp++.h"
#include "fcmp_pp_types.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------

FcmpRerandomizedOutputCompressed rerandomize_output(const OutputBytes output);
FcmpRerandomizedOutputCompressed rerandomize_output(const crypto::public_key &onetime_address,
    const crypto::ec_point &amount_commitment);

FcmpInputCompressed calculate_fcmp_input_for_rerandomizations(const crypto::public_key &onetime_address,
    const crypto::ec_point &amount_commitment,
    const crypto::secret_key &r_o,
    const crypto::secret_key &r_i,
    const crypto::secret_key &r_r_i,
    const crypto::secret_key &r_c);

void make_balanced_rerandomized_output_set(
    const std::vector<crypto::public_key> &input_onetime_addresses,
    const std::vector<crypto::ec_point> &input_amount_commitments,
    const std::vector<crypto::secret_key> &input_amount_blinding_factors,
    const std::vector<crypto::secret_key> &r_o,
    const crypto::secret_key &output_amount_blinding_factor_sum,
    std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out);

SeleneScalar o_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output);
SeleneScalar i_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output);
SeleneScalar i_blind_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output);
SeleneScalar c_blind(const FcmpRerandomizedOutputCompressed &rerandomized_output);

std::pair<FcmpPpSalProof, crypto::key_image> prove_sal(const crypto::hash &signable_tx_hash,
    const crypto::secret_key &x,
    const crypto::secret_key &y,
    const FcmpRerandomizedOutputCompressed &rerandomized_output);

FcmpMembershipProof prove_membership(const std::vector<FcmpPpProveInput> &fcmp_pp_prove_inputs,
    const std::size_t n_tree_layers);

bool verify_sal(const crypto::hash &signable_tx_hash,
    const FcmpInputCompressed &input,
    const crypto::key_image &key_image,
    const FcmpPpSalProof &sal_proof);

bool verify_membership(const FcmpMembershipProof &fcmp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRoot &tree_root,
    const std::vector<FcmpInputCompressed> &inputs);

bool verify(const std::vector<fcmp_pp::FcmpPpVerifyInput> &fcmp_pp_verify_inputs);

bool verify(const crypto::hash &signable_tx_hash,
    const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    const std::size_t n_tree_layers,
    const fcmp_pp::TreeRoot &tree_root,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images);
}//namespace fcmp_pp
