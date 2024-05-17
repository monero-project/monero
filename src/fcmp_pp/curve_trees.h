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
#include "fcmp_pp_crypto.h"
#include "fcmp_pp_types.h"
#include "misc_log_ex.h"
#include "serialization/keyvalue_serialization.h"
#include "tower_cycle.h"

#include <map>
#include <memory>
#include <vector>


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// A layer of contiguous hashes starting from a specific start_idx in the tree
template<typename C>
struct LayerExtension final
{
    uint64_t                       start_idx{0};
    bool                           update_existing_last_hash;
    std::vector<typename C::Point> hashes;
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

// Output pub key and commitment, ready to be converted to a leaf tuple
// - From {output_pubkey,commitment} -> {O,C} -> {O.x,I.x,C.x}
// - Output pairs do NOT necessarily have torsion cleared. We need the output pubkey as it exists in the chain in order
//   to derive the correct I (when deriving {O.x, I.x, C.x}). Torsion clearing O before deriving I from O would enable
//   spending a torsioned output once before the fcmp++ fork and again with a different key image via fcmp++.
#pragma pack(push, 1)
struct OutputPair final
{
    crypto::public_key output_pubkey;
    rct::key           commitment;

    bool operator==(const OutputPair &other) const { return output_pubkey == other.output_pubkey && commitment == other.commitment; }

    template <class Archive>
    inline void serialize(Archive &a, const unsigned int ver)
    {
        a & output_pubkey;
        a & commitment;
    }

    BEGIN_SERIALIZE_OBJECT()
        FIELD(output_pubkey)
        FIELD(commitment)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(output_pubkey)
        KV_SERIALIZE_VAL_POD_AS_BLOB(commitment)
    END_KV_SERIALIZE_MAP()
};

// Contextual wrapper for the output
struct OutputContext final
{
    // Output's global id in the chain, used to insert the output in the tree in the order it entered the chain
    uint64_t output_id;
    OutputPair output_pair;

    bool operator==(const OutputContext &other) const { return output_id == other.output_id && output_pair == other.output_pair; }

    template <class Archive>
    inline void serialize(Archive &a, const unsigned int ver)
    {
        a & output_id;
        a & output_pair;
    }

    BEGIN_SERIALIZE_OBJECT()
        FIELD(output_id)
        FIELD(output_pair)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(output_id)
        KV_SERIALIZE(output_pair)
    END_KV_SERIALIZE_MAP()
};
#pragma pack(pop)

static_assert(sizeof(OutputPair)    == (32+32),   "db expects 64 bytes for output pairs");
static_assert(sizeof(OutputContext) == (8+32+32), "db expects 72 bytes for output context");

using OutsByLastLockedBlock = std::unordered_map<uint64_t, std::vector<OutputContext>>;

// Ed25519 points (can go from OutputTuple -> LeafTuple)
struct OutputTuple final
{
    rct::key O;
    rct::key I;
    rct::key C;

    const OutputBytes to_output_bytes() const { return OutputBytes { O.bytes, I.bytes, C.bytes }; }
};

// Struct composed of ec elems needed to get a full-fledged leaf tuple
struct PreLeafTuple final
{
    fcmp_pp::EdYDerivatives O_pre_x;
    fcmp_pp::EdYDerivatives I_pre_x;
    fcmp_pp::EdYDerivatives C_pre_x;
};

struct ChunkBytes final
{
    std::vector<crypto::ec_point> chunk_bytes;

    BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(chunk_bytes)
    END_KV_SERIALIZE_MAP()
};

struct PathBytes final
{
    std::vector<OutputContext> leaves;
    std::vector<ChunkBytes> layer_chunks;

    BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(leaves)
        KV_SERIALIZE(layer_chunks)
    END_KV_SERIALIZE_MAP()
};

// The indexes in the tree of a leaf's path elems containing whole chunks at each layer
// - leaf_range refers to a complete chunk of leaves
struct PathIndexes final
{
    using StartIdx = uint64_t;
    using EndIdxExclusive = uint64_t;
    using Range = std::pair<StartIdx, EndIdxExclusive>;

    Range leaf_range;
    std::vector<Range> layers;
};

//----------------------------------------------------------------------------------------------------------------------
// Hash a chunk of new children
template<typename C>
typename C::Point get_new_parent(const std::unique_ptr<C> &curve, const typename C::Chunk &new_children);
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
        // Output ed25519 point x-coordinate
        typename C1::Scalar O_x;
        // Key image generator x-coordinate
        typename C1::Scalar I_x;
        // Commitment x-coordinate
        typename C1::Scalar C_x;
    };
    static const std::size_t LEAF_TUPLE_SIZE = 3;
    static_assert(sizeof(LeafTuple) == (sizeof(typename C1::Scalar) * LEAF_TUPLE_SIZE), "unexpected LeafTuple size");

    // Contiguous leaves in the tree, starting a specified start_idx in the leaf layer
    struct Leaves final
    {
        // Starting leaf tuple index in the leaf layer
        uint64_t                   start_leaf_tuple_idx{0};
        // Contiguous leaves in a tree that start at the start_idx
        std::vector<OutputContext> tuples;
    };

    // A struct useful to extend an existing tree
    // - layers alternate between C1 and C2
    // - c1_layer_extensions[0] is first layer after leaves, then c2_layer_extensions[0], c1_layer_extensions[1], etc
    struct TreeExtension final
    {
        Leaves                          leaves;
        std::vector<LayerExtension<C1>> c1_layer_extensions;
        std::vector<LayerExtension<C2>> c2_layer_extensions;
    };

    // Last hashes from each layer in the tree
    // - layers alternate between C1 and C2
    // - c1_last_hashes[0] refers to the layer after leaves, then c2_last_hashes[0], then c1_last_hashes[1], etc
    struct LastHashes final
    {
        std::vector<typename C1::Point> c1_last_hashes;
        std::vector<typename C2::Point> c2_last_hashes;
    };

    // A path in the tree containing whole chunks at each layer
    // - leaves contain a complete chunk of leaves, encoded as compressed ed25519 points
    // - c1_layers[0] refers to the chunk of elems in the tree in the layer after leaves. The hash of the chunk of
    //   leaves is 1 member of the c1_layers[0] chunk. The rest of c1_layers[0] is the chunk of elems that hash is in.
    // - layers alternate between C1 and C2
    // - c2_layers[0] refers to the chunk of elems in the tree in the layer after c1_layers[0]. The hash of the chunk
    //   of c1_layers[0] is 1 member of the c2_layers[0] chunk. The rest of c2_layers[0] is the chunk of elems that hash
    //   is in.
    // - c1_layers[1] refers to the chunk of elems in the tree in the layer after c2_layers[0] etc.
    struct Path final
    {
        std::vector<OutputTuple> leaves;
        // TODO: std::size_t idx_in_leaves;
        std::vector<std::vector<typename C1::Point>> c1_layers;
        std::vector<std::vector<typename C2::Point>> c2_layers;

        void clear()
        {
            leaves.clear();
            c1_layers.clear();
            c2_layers.clear();
        }

        bool empty() { return leaves.empty() && c1_layers.empty() && c2_layers.empty(); }
    };

    // A path ready to be used to construct an FCMP++ proof
    struct PathForProof final
    {
        std::vector<fcmp_pp::OutputBytes> leaves;
        std::size_t output_idx;
        std::vector<std::vector<typename C2::Scalar>> c2_scalar_chunks;
        std::vector<std::vector<typename C1::Scalar>> c1_scalar_chunks;
    };
//member functions
public:
    // Convert output pairs into leaf tuples, from {output pubkey,commitment} -> {O,C} -> {O.x,I.x,C.x}
    LeafTuple leaf_tuple(const OutputPair &output_pair) const;

    // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [O.x, I.x, C.x, O.x, I.x, C.x...]
    std::vector<typename C1::Scalar> flatten_leaves(std::vector<LeafTuple> &&leaves) const;

    // Take in the existing number of leaf tuples and the existing last hash in each layer in the tree, as well as new
    // outputs to add to the tree, and return a tree extension struct that can be used to extend a tree
    TreeExtension get_tree_extension(const uint64_t old_n_leaf_tuples,
        const LastHashes &existing_last_hashes,
        std::vector<std::vector<OutputContext>> &&new_outputs);

    // Calculate the number of elems in each layer of the tree based on the number of leaf tuples
    std::vector<uint64_t> n_elems_per_layer(const uint64_t n_leaf_tuples) const;

    // Calculate how many layers in the tree there are based on the number of leaf tuples
    std::size_t n_layers(const uint64_t n_leaf_tuples) const;

    // Get path indexes for the provided leaf tuple
    // - Returns empty path indexes if leaf is not in the tree (if n_leaf_tuples <= leaf_tuple_idx)
    PathIndexes get_path_indexes(const uint64_t n_leaf_tuples, const uint64_t leaf_tuple_idx) const;

    // Get child chunk indexes for the provided leaf tuple
    // - Returns empty if leaf is not in the tree (if n_leaf_tuples <= leaf_tuple_idx)
    std::vector<uint64_t> get_child_chunk_indexes(const uint64_t n_leaf_tuples, const uint64_t leaf_tuple_idx) const;

    LastHashes tree_edge_to_last_hashes(const std::vector<crypto::ec_point> &tree_edge_to_last_hashes) const;

    // Audit the provided path
    bool audit_path(const Path &path, const OutputPair &output, const uint64_t n_leaf_tuples_in_tree) const;

    uint8_t *get_tree_root_from_bytes(const std::size_t n_layers, const crypto::ec_point &tree_root) const;

    PathForProof path_for_proof(const Path &path, const OutputTuple &output_tuple) const;

    std::vector<crypto::ec_point> calc_hashes_from_path(const Path &path, const bool replace_last_hash = false) const;
private:
    // Multithreaded helper function to convert outputs to leaf tuples and set leaves on tree extension
    void set_valid_leaves(
        std::vector<typename C1::Scalar> &flattened_leaves_out,
        std::vector<OutputContext> &tuples_out,
        std::vector<OutputContext> &&new_outputs);

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

//private state
private:
    uint64_t m_set_valid_leaves_ms{0};
    uint64_t m_get_selene_scalars_ms{0};
    uint64_t m_batch_invert_ms{0};
    uint64_t m_collect_derivatives_ms{0};
    uint64_t m_convert_valid_leaves_ms{0};

    uint64_t m_sorting_outputs_ms{0};
    uint64_t m_hash_leaves_ms{0};
    uint64_t m_hash_layers_ms{0};

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
