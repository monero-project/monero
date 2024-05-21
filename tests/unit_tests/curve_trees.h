// Copyright (c) 2014, The Monero Project
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

#include "fcmp/curve_trees.h"
#include "fcmp/tower_cycle.h"
#include "misc_log_ex.h"

using Helios = fcmp::tower_cycle::helios::Helios;
using Selene = fcmp::tower_cycle::selene::Selene;

// TODO: make this the instantiation in curve_trees.h/.cpp
using CurveTreesV1 = fcmp::CurveTrees<Helios, Selene>;

class CurveTreesUnitTest
{
public:
    CurveTreesUnitTest(CurveTreesV1 &curve_trees): m_curve_trees(curve_trees) {};

//member structs
public:
    template<typename C>
    using Layer = std::vector<typename C::Point>;

    // A complete tree, useful for testing (don't want to keep the whole tree in memory during normal operation)
    struct Tree final
    {
        std::vector<CurveTreesV1::LeafTuple> leaves;
        std::vector<Layer<Helios>> c1_layers;
        std::vector<Layer<Selene>> c2_layers;
    };

//public member functions
public:
    // Read the in-memory tree and get data from last chunks from each layer
    CurveTreesV1::LastChunks get_last_chunks(const Tree &tree);

    // Use the tree extension to extend the in-memory tree
    void extend_tree(const CurveTreesV1::TreeExtension &tree_extension, Tree &tree_inout);

    // Validate the in-memory tree by re-hashing every layer, starting from root and working down to leaf layer
    bool validate_tree(const Tree &tree);

//private member functions
private:
    template<typename C_PARENT>
    bool validate_layer(const C_PARENT &c_parent,
        const Layer<C_PARENT> &parents,
        const std::vector<typename C_PARENT::Scalar> &child_scalars,
        const std::size_t max_chunk_size);

private:
    CurveTreesV1 &m_curve_trees;
};
