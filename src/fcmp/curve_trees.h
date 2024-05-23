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
#include "misc_log_ex.h"
#include "tower_cycle.h"

#include <vector>

// forward declarations
class CurveTreesUnitTest;

namespace fcmp
{

// TODO: longer descriptions
template<typename C1, typename C2>
class CurveTrees
{
    friend class ::CurveTreesUnitTest;
public:
    CurveTrees(const C1 &c1, const C2 &c2, const std::size_t c1_width, const std::size_t c2_width):
        m_c1{c1},
        m_c2{c2},
        m_c1_width{c1_width},
        m_c2_width{c2_width},
        m_leaf_layer_chunk_width{LEAF_TUPLE_SIZE * c2_width}
    {
        assert(c1_width > 0);
        assert(c2_width > 0);
    };

//member structs
public:
    // Tuple that composes a single leaf in the tree
    struct LeafTuple final
    {
        // Output ed25519 point x-coordinate
        typename C2::Scalar O_x;
        // Key image generator x-coordinate
        typename C2::Scalar I_x;
        // Commitment x-coordinate
        typename C2::Scalar C_x;
    };
    static const std::size_t LEAF_TUPLE_SIZE = 3;
    static_assert(sizeof(LeafTuple) == (sizeof(typename C2::Scalar) * LEAF_TUPLE_SIZE), "unexpected LeafTuple size");

    // Leaves in the tree
    struct Leaves final
    {
        // Starting index in the leaf layer
        std::size_t            start_idx;
        // Contiguous leaves in a tree that start at the start_idx
        std::vector<LeafTuple> tuples;
    };

    // A layer of contiguous hashes starting from a specific start_idx in the tree
    template<typename C>
    struct LayerExtension final
    {
        std::size_t                    start_idx;
        std::vector<typename C::Point> hashes;
    };

    // A struct useful to extend an existing tree
    // - layers alternate between C1 and C2
    // - c2_layer_extensions[0] is first layer after leaves, then c1_layer_extensions[0], c2_layer_extensions[1], etc
    struct TreeExtension final
    {
        Leaves                          leaves;
        std::vector<LayerExtension<C1>> c1_layer_extensions;
        std::vector<LayerExtension<C2>> c2_layer_extensions;
    };

    // Useful data from the last chunk in a layer
    template<typename C>
    struct LastChunkData final
    {
        // The total number of children % child layer chunk width
        /*TODO: const*/ std::size_t                     child_offset;
        // The last child in the chunk (and therefore the last child in the child layer)
        /*TODO: const*/ typename C::Scalar              last_child;
        // The hash of the last chunk of child scalars
        /*TODO: const*/ typename C::Point               last_parent;
        // Total number of children in the child layer
        /*TODO: const*/ std::size_t                     child_layer_size;
        // Total number of hashes in the parent layer
        /*TODO: const*/ std::size_t                     parent_layer_size;
    };

    // Last chunk data from each layer in the tree
    // - layers alternate between C1 and C2
    // - c2_last_chunks[0] is first layer after leaves, then c1_last_chunks[0], then c2_last_chunks[1], etc
    struct LastChunks final
    {
        std::vector<LastChunkData<C1>> c1_last_chunks;
        std::vector<LastChunkData<C2>> c2_last_chunks;
    };

//member functions
public:
    // Convert cryptonote output pub key and commitment to a leaf tuple for the curve trees tree
    LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C) const;

    // Take in the existing last chunks of each layer in the tree, as well as new leaves to add to the tree,
    // and return a tree extension struct that can be used to extend a global tree
    TreeExtension get_tree_extension(const LastChunks &existing_last_chunks,
        const std::vector<LeafTuple> &new_leaf_tuples);

//private member functions
private:
    // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [scalar,scalar,scalar,scalar,scalar,scalar,...]
    std::vector<typename C2::Scalar> flatten_leaves(const std::vector<LeafTuple> &leaves) const;

    // TODO: make below functions static functions inside curve_trees.cpp
    // Hash a chunk of new children
    template <typename C>
    typename C::Point get_new_parent(const C &curve, const typename C::Chunk &new_children) const;

    // Hash the first chunk of children being added to a layer
    template <typename C>
    typename C::Point get_first_parent(const C &curve,
        const typename C::Chunk &new_children,
        const std::size_t chunk_width,
        const bool child_layer_last_hash_updated,
        const LastChunkData<C> *last_chunk_ptr,
        const std::size_t offset) const;

    // After hashing a layer of children points, convert those children x-coordinates into their respective cycle
    // scalars, and prepare them to be hashed for the next layer
    template<typename C_CHILD, typename C_PARENT>  
    std::vector<typename C_PARENT::Scalar> next_child_scalars_from_children(const C_CHILD &c_child,
        const LastChunkData<C_CHILD> *last_child_chunk_ptr,
        const LastChunkData<C_PARENT> *last_parent_chunk_ptr,
        const LayerExtension<C_CHILD> &children);

    // Hash chunks of a layer of new children, outputting the next layer's parents
    template<typename C>
    void hash_layer(const C &curve,
        const LastChunkData<C> *last_parent_chunk_ptr,
        const std::vector<typename C::Scalar> &child_scalars,
        const std::size_t children_start_idx,
        const std::size_t chunk_width,
        LayerExtension<C> &parents_out);

//member variables
private:
    const C1 &m_c1;
    const C2 &m_c2;

    const std::size_t m_c1_width;
    const std::size_t m_c2_width;

    const std::size_t m_leaf_layer_chunk_width;
};

} //namespace fcmp
