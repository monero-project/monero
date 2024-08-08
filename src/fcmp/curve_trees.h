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
#include "cryptonote_basic/cryptonote_basic.h"
#include "misc_log_ex.h"
#include "tower_cycle.h"

#include <map>
#include <memory>
#include <vector>


namespace fcmp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Hash a chunk of new children
template<typename C>
typename C::Point get_new_parent(const std::unique_ptr<C> &curve, const typename C::Chunk &new_children);
//----------------------------------------------------------------------------------------------------------------------
// A layer of contiguous hashes starting from a specific start_idx in the tree
template<typename C>
struct LayerExtension final
{
    uint64_t                       start_idx{0};
    bool                           update_existing_last_hash;
    std::vector<typename C::Point> hashes;
};

// A struct useful to trim a layer and update its last hash if necessary
template<typename C>
struct LayerReduction final
{
    uint64_t          new_total_parents{0};
    bool              update_existing_last_hash;
    typename C::Point new_last_hash;
};

// Useful metadata for growing a layer
struct GrowLayerInstructions final
{
    // The max chunk width of children used to hash into a parent
    std::size_t parent_chunk_width;

    // Total children refers to the total number of elements in a layer
    uint64_t old_total_children;
    uint64_t new_total_children;

    // Total parents refers to the total number of hashes of chunks of children
    uint64_t old_total_parents;
    uint64_t new_total_parents;

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
    uint64_t next_parent_start_index;
};

// Useful metadata for trimming a layer
struct TrimLayerInstructions final
{
    // The max chunk width of children used to hash into a parent
    std::size_t parent_chunk_width;

    // Total children refers to the total number of elements in a layer
    uint64_t old_total_children;
    uint64_t new_total_children;

    // Total parents refers to the total number of hashes of chunks of children
    uint64_t old_total_parents;
    uint64_t new_total_parents;

    // True if the new last chunk's existing parent hash will need to be updated
    bool update_existing_last_hash;

    // Whether we need to explicitly trim children from the new last chunk
    bool need_last_chunk_children_to_trim;
    // Whether we need to trim by growing using the remaining children from the new last chunk
    bool need_last_chunk_remaining_children;
    // Whether we need the new last chunk's existing parent hash in order to complete the trim
    bool need_existing_last_hash;
    // Whether we need the new last child from the new last chunk in order to complete the trim
    bool need_new_last_child;

    // The offset to use when hashing the last chunk
    std::size_t hash_offset;

    // The starting and ending indexes of the children we're going to need to trim the last chunk
    uint64_t start_trim_idx;
    uint64_t end_trim_idx;
};

// Output pub key and commitment, ready to be converted into a leaf tuple (from {O,C} -> {O.x, I.x, C.x})
struct PreprocessedLeafTuple final
{
    // Output pubkey that has been checked valid and torsion cleared
    rct::key O;
    // Commitment that has been checked valid and torsion cleared
    rct::key C;
};
static_assert(sizeof(PreprocessedLeafTuple) == (32+32), "db expects 64 bytes for pre-processed leaf tuples");

// Contextual wrapper for a pre-processed leaf tuple
struct LeafTupleContext final
{
    // Global output ID useful to order the leaf tuple for insertion into the tree
    uint64_t              output_id;
    PreprocessedLeafTuple preprocessed_leaf_tuple;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// This class is useful to help update the curve trees merkle tree without needing to keep the entire tree in memory
// - It requires instantiation with the C1 and C2 curve classes and widths, hardening the tree structure
// - It ties the C2 curve in the tree to the leaf layer (the leaf layer is composed of C2 scalars)
template<typename C1, typename C2>
class CurveTrees
{
public:
    CurveTrees(std::unique_ptr<C1> &&c1, std::unique_ptr<C2> &&c2, const uint64_t c1_width, const uint64_t c2_width):
        m_c1{std::move(c1)},
        m_c2{std::move(c2)},
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

    // Contiguous leaves in the tree, starting a specified start_idx in the leaf layer
    struct Leaves final
    {
        // Starting leaf tuple index in the leaf layer
        uint64_t                           start_leaf_tuple_idx{0};
        // Contiguous leaves in a tree that start at the start_idx
        std::vector<PreprocessedLeafTuple> tuples;
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

    // A struct useful to reduce the number of leaves in an existing tree
    // - layers alternate between C1 and C2
    // - c2_layer_reductions[0] is first layer after leaves, then c1_layer_reductions[0], c2_layer_reductions[1], etc
    struct TreeReduction final
    {
        uint64_t                        new_total_leaf_tuples;
        std::vector<LayerReduction<C1>> c1_layer_reductions;
        std::vector<LayerReduction<C2>> c2_layer_reductions;
    };

    // Last hashes from each layer in the tree
    // - layers alternate between C1 and C2
    // - c2_last_hashes[0] refers to the layer after leaves, then c1_last_hashes[0], then c2_last_hashes[1], etc
    struct LastHashes final
    {
        std::vector<typename C1::Point> c1_last_hashes;
        std::vector<typename C2::Point> c2_last_hashes;
    };

    // The children we'll trim from each last chunk in the tree
    // - layers alternate between C1 and C2
    // - c2_children[0] refers to the layer after leaves, then c1_children[0], then c2_children[1], etc
    struct LastChunkChildrenToTrim final
    {
        std::vector<std::vector<typename C1::Scalar>> c1_children;
        std::vector<std::vector<typename C2::Scalar>> c2_children;
    };

//member functions
public:
    // Convert cryptonote output pub key and commitment to a pre-processed leaf tuple ready for insertion to the tree
    LeafTupleContext output_to_leaf_context(const std::uint64_t output_id,
        const crypto::public_key &output_pubkey,
        const rct::key &C) const;

    // Convert cryptonote tx outs to contexts ready to be converted to leaf tuples, grouped by unlock height
    void tx_outs_to_leaf_tuple_contexts(const cryptonote::transaction &tx,
        const std::vector<uint64_t> &output_ids,
        const uint64_t tx_height,
        const bool miner_tx,
        std::multimap<uint64_t, LeafTupleContext> &leaf_tuples_by_unlock_block_inout) const;

    // Derive a leaf tuple from a pre-processed leaf tuple {O,C} -> {O.x,I.x,C.x}
    LeafTuple leaf_tuple(const PreprocessedLeafTuple &preprocessed_leaf_tuple) const;

    // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [O.x, I.x, C.x, O.x, I.x, C.x...]
    std::vector<typename C2::Scalar> flatten_leaves(std::vector<LeafTuple> &&leaves) const;

    // Take in the existing number of leaf tuples and the existing last hash in each layer in the tree, as well as new
    // leaves to add to the tree, and return a tree extension struct that can be used to extend a tree
    TreeExtension get_tree_extension(const uint64_t old_n_leaf_tuples,
        const LastHashes &existing_last_hashes,
        std::vector<LeafTupleContext> &&new_leaf_tuples) const;

    // Get instructions useful for trimming all existing layers in the tree
    std::vector<TrimLayerInstructions> get_trim_instructions(
        const uint64_t old_n_leaf_tuples,
        const uint64_t trim_n_leaf_tuples) const;

    // Take in the instructions useful for trimming all existing layers in the tree, all children to be trimmed from
    // each last chunk, and the existing last hash in what will become the new last parent of each layer, and return
    // a tree reduction struct that can be used to trim a tree
    TreeReduction get_tree_reduction(
        const std::vector<TrimLayerInstructions> &trim_instructions,
        const LastChunkChildrenToTrim &children_to_trim,
        const LastHashes &last_hashes) const;

private:
    // Helper function used to set the next layer extension used to grow the next layer in the tree
    // - for example, if we just grew the parent layer after the leaf layer, the "next layer" would be the grandparent
    //   layer of the leaf layer
    GrowLayerInstructions set_next_layer_extension(
        const GrowLayerInstructions &prev_layer_instructions,
        const bool parent_is_c1,
        const LastHashes &last_hashes,
        std::size_t &c1_last_idx_inout,
        std::size_t &c2_last_idx_inout,
        TreeExtension &tree_extension_inout) const;

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
using Helios       = tower_cycle::Helios;
using Selene       = tower_cycle::Selene;
using CurveTreesV1 = CurveTrees<Helios, Selene>;

// https://github.com/kayabaNerve/fcmp-plus-plus/blob
//  /b2742e86f3d18155fd34dd1ed69cb8f79b900fce/crypto/fcmps/src/tests.rs#L81-L82
const std::size_t HELIOS_CHUNK_WIDTH = 38;
const std::size_t SELENE_CHUNK_WIDTH = 18;

std::shared_ptr<CurveTreesV1> curve_trees_v1(
    const std::size_t helios_chunk_width = HELIOS_CHUNK_WIDTH,
    const std::size_t selene_chunk_width = SELENE_CHUNK_WIDTH);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp
