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

#include "curve_trees.h"

namespace fcmp
{
namespace curve_trees
{

LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C)
{
    crypto::ec_point I;
    crypto::derive_key_image_generator(O, I);

    return LeafTuple{
        .O_x = tower_cycle::selene::SELENE.ed_25519_point_to_scalar(O),
        .I_x = tower_cycle::selene::SELENE.ed_25519_point_to_scalar(I),
        .C_x = tower_cycle::selene::SELENE.ed_25519_point_to_scalar(C)
    };
}

// TODO: move into curves tree file
std::vector<tower_cycle::selene::Selene::Scalar> flatten_leaves(const std::vector<LeafTuple> &leaves)
{
    std::vector<tower_cycle::selene::Selene::Scalar> flattened_leaves;
    flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

    for (const auto &l : leaves)
    {
        // TODO: implement without cloning
        flattened_leaves.emplace_back(tower_cycle::selene::SELENE.clone(l.O_x));
        flattened_leaves.emplace_back(tower_cycle::selene::SELENE.clone(l.I_x));
        flattened_leaves.emplace_back(tower_cycle::selene::SELENE.clone(l.C_x));
    }

    return flattened_leaves;
};

} //namespace curve_trees
} //namespace fcmp
