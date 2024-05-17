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

#include "fcmp.h"
#include "misc_log_ex.h"

namespace fcmp
{

// TODO: move into its own fcmp_crypto file
static SeleneScalar ed_25519_point_to_selene_scalar(const crypto::ec_point &point)
{
    static_assert(sizeof(fcmp::RustEd25519Point) == sizeof(crypto::ec_point),
        "expected same size ed25519 point to rust representation");

    // TODO: implement reading just the x coordinate of ed25519 point in C/C++
    fcmp::RustEd25519Point rust_point;
    memcpy(&rust_point, &point, sizeof(fcmp::RustEd25519Point));
    return fcmp_rust::ed25519_point_to_selene_scalar(rust_point);
};

// TODO: move into its own fcmp_crypto file
LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C)
{
    crypto::ec_point I;
    crypto::derive_key_image_generator(O, I);

    return LeafTuple{
        .O_x = ed_25519_point_to_selene_scalar(O),
        .I_x = ed_25519_point_to_selene_scalar(I),
        .C_x = ed_25519_point_to_selene_scalar(C)
    };
}

// TODO: move into its own fcmp_crypto file
std::vector<SeleneScalar> flatten_leaves(const std::vector<LeafTuple> &leaves)
{
    std::vector<SeleneScalar> flattened_leaves;
    flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

    for (const auto &l : leaves)
    {
        // TODO: implement without cloning
        flattened_leaves.emplace_back(fcmp_rust::clone_selene_scalar(l.O_x));
        flattened_leaves.emplace_back(fcmp_rust::clone_selene_scalar(l.I_x));
        flattened_leaves.emplace_back(fcmp_rust::clone_selene_scalar(l.C_x));
    }

    return flattened_leaves;
};

SeleneScalar Helios::point_to_cycle_scalar(const Helios::Point &point) const
{
    return fcmp_rust::helios_point_to_selene_scalar(point);
};

HeliosScalar Selene::point_to_cycle_scalar(const Selene::Point &point) const
{
    return fcmp_rust::selene_point_to_helios_scalar(point);
};
} //namespace fcmp
