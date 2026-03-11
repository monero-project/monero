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

#include "crypto/crypto.h"
#include "fcmp_pp_types.h"
#include "tower_cycle.h"
#include "misc_log_ex.h"

#include <memory>
#include <vector>

namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_to_tuple(const OutputPair &output_pair);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// This class is useful to help update the curve trees merkle tree without needing to keep the entire tree in memory
// - It requires instantiation with the C1 and C2 curve classes and widths, hardening the tree structure
// - It ties the C1 curve in the tree to the leaf layer (the leaf layer is composed of C1 scalars)
template<typename C1, typename C2>
class CurveTrees
{
public:
    CurveTrees(std::unique_ptr<C1> &&c1,
        std::unique_ptr<C2> &&c2,
        const std::size_t c1_width,
        const std::size_t c2_width):
            m_c1{std::move(c1)},
            m_c2{std::move(c2)},
            m_c1_width{c1_width},
            m_c2_width{c2_width},
            m_leaf_layer_chunk_width{LEAF_TUPLE_SIZE * c1_width}
    {
        assert(c1_width > 0);
        assert(c2_width > 0);
    };

//member structs
public:
    // Tuple that composes a single leaf in the tree
    struct LeafTuple final
    {
        // Output ed25519 point wei x and y coordinates
        typename C1::Scalar O_x;
        typename C1::Scalar O_y;
        // Key image generator wei x and y coordinates
        typename C1::Scalar I_x;
        typename C1::Scalar I_y;
        // Commitment wei x and y coordinates
        typename C1::Scalar C_x;
        typename C1::Scalar C_y;
    };

    static const std::size_t LEAF_TUPLE_POINTS = 3;
    static constexpr std::size_t LEAF_TUPLE_SIZE = LEAF_TUPLE_POINTS * 2;

    static_assert(sizeof(LeafTuple) == (sizeof(typename C1::Scalar) * LEAF_TUPLE_SIZE), "unexpected LeafTuple size");

//member functions
private:
    // Multithreaded helper function to convert valid outputs to leaf tuples ready for insertion to the tree & db
    void outputs_to_leaves(std::vector<UnifiedOutput> &&new_outputs,
        std::vector<typename C1::Scalar> &flattened_leaves_out,
        std::vector<UnifiedOutput> &valid_outputs_out);

//private state
private:
    uint64_t m_set_valid_leaves_ms{0};
    uint64_t m_get_selene_scalars_ms{0};
    uint64_t m_batch_invert_ms{0};
    uint64_t m_collect_derivatives_ms{0};
    uint64_t m_convert_valid_leaves_ms{0};

//public member variables
public:
    // The curve interfaces
    const std::unique_ptr<C1> m_c1;
    const std::unique_ptr<C2> m_c2;

    // The leaf layer has a distinct chunk width than the other layers
    const std::size_t m_leaf_layer_chunk_width;

    // The chunk widths of the layers in the tree tied to each curve
    const std::size_t m_c1_width;
    const std::size_t m_c2_width;
};
//----------------------------------------------------------------------------------------------------------------------
using Selene       = tower_cycle::Selene;
using Helios       = tower_cycle::Helios;
using CurveTreesV1 = CurveTrees<Selene, Helios>;

// https://github.com/kayabaNerve/fcmp-plus-plus/blob
//  /b2742e86f3d18155fd34dd1ed69cb8f79b900fce/crypto/fcmps/src/tests.rs#L81-L82
const std::size_t SELENE_CHUNK_WIDTH = 38;
const std::size_t HELIOS_CHUNK_WIDTH = 18;

std::shared_ptr<CurveTreesV1> curve_trees_v1(
    const std::size_t selene_chunk_width = SELENE_CHUNK_WIDTH,
    const std::size_t helios_chunk_width = HELIOS_CHUNK_WIDTH);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp_pp
