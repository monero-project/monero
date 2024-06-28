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



namespace fcmp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Hash a chunk of new children
template<typename C>
typename C::Point get_new_parent(const C &curve, const typename C::Chunk &new_children);
//----------------------------------------------------------------------------------------------------------------------
// A layer of contiguous hashes starting from a specific start_idx in the tree
template<typename C>
struct LayerExtension final
{
    std::size_t                    start_idx{0};
    bool                           update_existing_last_hash;
    std::vector<typename C::Point> hashes;
};

// Useful metadata for updating a layer
struct UpdateLayerMetadata final
{
    // The max chunk width of children used to hash into a parent
    std::size_t parent_chunk_width;

    // Total children refers to the total number of elements in a layer
    std::size_t old_total_children;
    std::size_t new_total_children;

    // Total parents refers to the total number of hashes of chunks of children
    std::size_t old_total_parents;
    std::size_t new_total_parents;

    // When updating the tree, we use this boolean to know when we'll need to use the tree's existing old root in order
    // to set a new layer after that root
    // - We'll need to be sure the old root gets hashed when setting the next layer
    bool setting_next_layer_after_old_root;
    // When the last child in the child layer changes, we'll need to use its old value to update its parent hash
    bool need_old_last_child;
    // When the last parent in the layer changes, we'll need to use its old value to update itself
    bool need_old_last_parent;

    // The first chunk that needs to be updated's first child's offset within that chunk
    std::size_t start_offset;
    // The parent's starting index in the layer
    std::size_t next_parent_start_index;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// This class is useful help update the curve trees tree without needing to keep the entire tree in memory
// - It requires instantiation with the C1 and C2 curve classes and widths, hardening the tree structure
// - It ties the C2 curve in the tree to the leaf layer
template<typename C1, typename C2>
class CurveTrees
{
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
        const typename C2::Scalar O_x;
        // Key image generator x-coordinate
        const typename C2::Scalar I_x;
        // Commitment x-coordinate
        const typename C2::Scalar C_x;
    };
    static const std::size_t LEAF_TUPLE_SIZE = 3;
    static_assert(sizeof(LeafTuple) == (sizeof(typename C2::Scalar) * LEAF_TUPLE_SIZE), "unexpected LeafTuple size");

    // Contiguous leaves in the tree, starting a specified start_idx in the leaf layer
    struct Leaves final
    {
        // Starting index in the leaf layer
        std::size_t            start_idx{0};
        // Contiguous leaves in a tree that start at the start_idx
        std::vector<LeafTuple> tuples;
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

    // Last hashes from each layer in the tree
    // - layers alternate between C1 and C2
    // - c2_last_hashes[0] refers to the layer after leaves, then c1_last_hashes[0], then c2_last_hashes[1], etc
    struct LastHashes final
    {
        std::vector<typename C1::Point> c1_last_hashes;
        std::vector<typename C2::Point> c2_last_hashes;
    };

//member functions
public:
    // Convert cryptonote output pub key and commitment to a leaf tuple for the curve trees tree
    LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C) const;

    // Take in the existing number of leaf tuples and the existing last hashes of each layer in the tree, as well as new
    // leaves to add to the tree, and return a tree extension struct that can be used to extend a global tree
    TreeExtension get_tree_extension(const std::size_t old_n_leaf_tuples,
        const LastHashes &existing_last_hashes,
        const std::vector<LeafTuple> &new_leaf_tuples) const;

    // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [scalar,scalar,scalar,scalar,scalar,scalar,...]
    std::vector<typename C2::Scalar> flatten_leaves(const std::vector<LeafTuple> &leaves) const;

private:
    // Helper function used to set the next layer extension used to grow the next layer in the tree
    // - for example, if we just grew the parent layer after the leaf layer, the "next layer" would be the grandparent
    //   layer of the leaf layer
    UpdateLayerMetadata set_next_layer_extension(
        const UpdateLayerMetadata &prev_layer_metadata,
        const bool parent_is_c1,
        const LastHashes &last_hashes,
        std::size_t &c1_last_idx_inout,
        std::size_t &c2_last_idx_inout,
        TreeExtension &tree_extension_inout) const;

//public member variables
public:
    // The curve interfaces
    const C1 &m_c1;
    const C2 &m_c2;

    // The leaf layer has a distinct chunk width than the other layers
    // TODO: public function for update_last_parent, and make this private
    const std::size_t m_leaf_layer_chunk_width;

    // The chunk widths of the layers in the tree tied to each curve
    const std::size_t m_c1_width;
    const std::size_t m_c2_width;
};
//----------------------------------------------------------------------------------------------------------------------
using Helios       = tower_cycle::Helios;
using Selene       = tower_cycle::Selene;
using CurveTreesV1 = CurveTrees<Helios, Selene>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp
