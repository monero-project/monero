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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "fcmp_pp/prove.h"
#include "unit_tests_utils.h"

#include <iosfwd>


static std::string get_fcmp_pp_filename(const std::size_t n_inputs)
{
    return "../data/fcmp_pp_verify_inputs_" + std::to_string(n_inputs) + "in.bin";
}

template<std::size_t n_inputs>
class test_fcmp_pp_verify
{
public:
    static constexpr size_t loop_count =
        n_inputs < 2 ? 1000 :
        n_inputs < 16 ? 100 :
        /*n_inputs >= 16*/ 8;

    bool init()
    {
        return unit_test::read_fcmp_pp_verify_input_from_file(get_fcmp_pp_filename(n_inputs),
                n_inputs,
                signable_tx_hash,
                fcmp_pp_proof,
                n_layers,
                tree_root,
                pseudo_outs,
                key_images
            );
    }

    bool test()
    {
        return fcmp_pp::verify(
                signable_tx_hash,
                fcmp_pp_proof,
                n_layers,
                tree_root,
                pseudo_outs,
                key_images
            );
    }

private:
    crypto::hash signable_tx_hash;
    std::vector<uint8_t> fcmp_pp_proof;
    uint8_t n_layers;
    fcmp_pp::TreeRootShared tree_root;
    std::vector<crypto::ec_point> pseudo_outs;
    std::vector<crypto::key_image> key_images;
};
