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

#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "curve_trees.h"
#include "fcmp_pp_types.h"
#include "ringct/rctTypes.h"
#include "serialization/containers.h"
#include "serialization/crypto.h"
#include "serialization/pair.h"
#include "serialization/serialization.h"
#include "tree_sync.h"

#include <deque>
#include <memory>
#include <unordered_map>


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
using BlockIdx  = uint64_t;
using BlockHash = crypto::hash;

using LeafIdx       = uint64_t;
using LayerIdx      = std::size_t;
using ChildChunkIdx = uint64_t;

using LastLockedBlockIdx = BlockIdx;
using CreatedBlockIdx = BlockIdx;
using NumOutputs      = std::size_t;

using OutputRef = crypto::hash;

struct BlockMeta final
{
    BlockIdx blk_idx;
    BlockHash blk_hash;
    uint64_t n_leaf_tuples;

    BEGIN_SERIALIZE_OBJECT()
        FIELD(blk_idx)
        FIELD(blk_hash)
        FIELD(n_leaf_tuples)
    END_SERIALIZE()
};

// We need to use a ref count on all individual elems in the cache because it's possible for:
//  a) multiple blocks to share path elems that need to remain after pruning a block past the max reorg depth.
//  b) multiple registered outputs to share the same path elems.
// We can't remove a cached elem unless we know it's ref'd 0 times.
struct CachedLeafChunk final
{
    std::vector<OutputPair> leaves;
    uint64_t ref_count;

    BEGIN_SERIALIZE_OBJECT()
        FIELD(leaves)
        FIELD(ref_count)
    END_SERIALIZE()
};

struct CachedTreeElemChunk final
{
    std::vector<crypto::ec_point> tree_elems;
    uint64_t ref_count;

    BEGIN_SERIALIZE_OBJECT()
        FIELD(tree_elems)
        FIELD(ref_count)
    END_SERIALIZE()
};

struct AssignedLeafIdx final
{
    bool assigned_leaf_idx{false};
    LeafIdx leaf_idx{0};

    void assign_leaf(const LeafIdx idx) { leaf_idx = idx; assigned_leaf_idx = true; }
    void unassign_leaf() { leaf_idx = 0; assigned_leaf_idx = false; }

    BEGIN_SERIALIZE_OBJECT()
        FIELD(assigned_leaf_idx)
        FIELD(leaf_idx)
    END_SERIALIZE()
};

using LockedOutsByLastLockedBlock = std::unordered_map<LastLockedBlockIdx, std::vector<OutputContext>>;
using LockedOutputRefs       = std::unordered_map<LastLockedBlockIdx, NumOutputs>;
using LockedOutputsByCreated = std::unordered_map<CreatedBlockIdx, LockedOutputRefs>;

using RegisteredOutputs = std::unordered_map<OutputRef, AssignedLeafIdx>;
using LeafCache         = std::unordered_map<ChildChunkIdx, CachedLeafChunk>;
using ChildChunkCache   = std::unordered_map<ChildChunkIdx, CachedTreeElemChunk>;

// TODO: technically this can be a vector. There should *always* be at least 1 entry for every layer
using TreeElemCache     = std::unordered_map<LayerIdx, ChildChunkCache>;

static constexpr int TREE_CACHE_VERSION = 0;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Syncs the tree and keeps a user's known received outputs up to date, all saved in memory.
// - The object does not store the entire tree locally. The object only stores what it needs in order to update paths
//   of known received outputs as it syncs.
// - The memory footprint of the object is roughly ALL locked outputs in the chain, all known output paths, and the last
//   chunk of tree elems at every layer of the tree the last N blocks. The latter is required to handle reorgs up to
//   N blocks deep.
// - WARNING: the implementation is not thread safe, it expects synchronous calls.
//   TODO: use a mutex to enforce thread safety.
template<typename C1, typename C2>
class TreeCache final : public TreeSync<C1, C2>
{
public:
    // A helpful struct useful to prepare a state change to the cache, and then use it to change the cache
    struct CacheStateChange final
    {
        uint64_t n_outputs_observed{0};
        LockedOutsByLastLockedBlock locked_outputs;
        LockedOutputsByCreated locked_output_refs;
        typename fcmp_pp::curve_trees::CurveTrees<C1, C2>::TreeExtension tree_extension;
        std::vector<uint64_t> n_new_leaf_tuples_per_block;
    };

public:
    TreeCache(std::shared_ptr<CurveTrees<C1, C2>> curve_trees,
        const uint64_t max_reorg_depth = ORPHANED_BLOCKS_MAX_COUNT):
            TreeSync<C1, C2>(curve_trees, max_reorg_depth)
    {};

    bool register_output(const OutputPair &output) override;

    // TODO: bool cancel_output_registration

    void sync_block(const uint64_t block_idx,
        const crypto::hash &block_hash,
        const crypto::hash &prev_block_hash,
        const OutsByLastLockedBlock &outs_by_last_locked_block) override;

    bool pop_block() override;

    bool get_output_path(const OutputPair &output, typename CurveTrees<C1, C2>::Path &path_out) const override;

// Public functions not part of TreeSync interface
public:
    // Note: it's possible that the cache already contains force added paths from force_add_output_path. Calling init
    // does not get rid of those already added paths.
    void init(const uint64_t start_block_idx,
        const crypto::hash &start_block_hash,
        const uint64_t n_leaf_tuples,
        const fcmp_pp::curve_trees::PathBytes &last_path,
        const OutsByLastLockedBlock &timelocked_outputs);

    // TODO: make this part of the TreeSync interface
    uint64_t get_n_leaf_tuples() const noexcept;
    bool get_top_block(BlockMeta &top_block_out) const
    {
        CHECK_AND_ASSERT_MES(!m_cached_blocks.empty(), false, "get_top_block: empty cached blocks");
        BlockMeta top_block = m_cached_blocks.back();
        top_block_out = std::move(top_block);
        return true;
    };

    bool get_front_block(BlockMeta &front_block_out) const
    {
        CHECK_AND_ASSERT_MES(!m_cached_blocks.empty(), false, "get_front_block: empty cached blocks");
        BlockMeta front_block = m_cached_blocks.front();
        front_block_out = std::move(front_block);
        return true;
    };

    uint64_t n_synced_blocks() const { return m_cached_blocks.empty() ? 0 : (m_cached_blocks.back().blk_idx + 1); }

    uint64_t get_output_count() const { return m_output_count; }

    // Gets the tree root and n_tree_layers for the tree currently cached in the TreeCache. If the TreeCache's tip is
    // block index n, then this will return the tree root and n_tree_layers for the tree composed of all valid spendable
    // outputs in the chain when the chain tip is block index n.
    uint8_t get_tree_root(crypto::ec_point &tree_root_out) const;

    // Build the tree extension and all other types needed to grow the cache, returning the state change by ref
    void prepare_to_grow_cache(const uint64_t start_block_idx,
        const crypto::hash &prev_block_hash,
        const std::vector<crypto::hash> &new_block_hashes,
        const std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> &outs_by_last_locked_blocks,
        CacheStateChange &cache_state_change) const;

    // Advance the cache state, processing the state change prepared above
    void grow_cache(const uint64_t start_block_idx,
        const std::vector<crypto::hash> &new_block_hashes,
        CacheStateChange &&cache_state_change,
        const bool skip_shrink_to_reorg_depth = false);

    // The pruning feature of the cache gets rid of all refs we don't need anymore
    void shrink_to_reorg_depth();

    // Enable popping back to a specific block efficiently
    bool pop_to_block(const uint64_t new_top_blk_idx, const crypto::hash &new_top_hash);

    // Clear all state
    void clear();

    // Force add a path to the cache without re-constructing it via sync
    void force_add_output_path(const OutputPair &output,
        const LeafIdx leaf_idx,
        const PathBytes &path_bytes,
        const uint64_t n_leaf_tuples);

// Internal helper functions
private:
    typename CurveTrees<C1, C2>::LastHashes get_last_hashes() const;

    bool get_leaf_path(const uint64_t n_leaf_tuples,
        const LeafIdx leaf_idx,
        typename CurveTrees<C1, C2>::Path &path_out) const;

    void deque_block(const uint64_t old_n_leaf_tuples);

    std::vector<crypto::ec_point> get_tree_edge(const uint64_t n_leaf_tuples) const;

// State held in memory
private:
    // Locked outputs in the chain that we use to grow the tree with internally upon unlock
    LockedOutsByLastLockedBlock m_locked_outputs;
    LockedOutputsByCreated m_locked_output_refs;

    // Keep a global output counter so the caller knows how output id's should be set
    uint64_t m_output_count{0};

    // The outputs that TreeCache should keep track of while syncing
    RegisteredOutputs m_registered_outputs;

    // Cached leaves and tree elems
    LeafCache m_leaf_cache;
    TreeElemCache m_tree_elem_cache;

    // Used for getting tree extensions when growing and for trimming
    // - These are unspecific to the wallet's registered outputs. These are strictly necessary to ensure we can rebuild
    //   the tree extensions (and trim backwards) for each block correctly locally when syncing.
    std::deque<BlockMeta> m_cached_blocks;

    mutable uint64_t m_getting_unlocked_outs_ms{0};
    mutable uint64_t m_getting_tree_extension_ms{0};
    mutable uint64_t m_updating_cache_values_ms{0};

// Serialization
public:
    BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(TREE_CACHE_VERSION)
        FIELD(m_locked_outputs)
        FIELD(m_locked_output_refs)
        FIELD(m_output_count)
        FIELD(m_registered_outputs)
        FIELD(m_leaf_cache)
        FIELD(m_tree_elem_cache)
        FIELD(m_cached_blocks)
        // It's possible for m_cached_blocks.size() > m_max_reorg_depth if the max reorg depth changes across runs.
        // This is ok as implemented. m_cached_blocks.size() will stay constant while syncing in this case.
    END_SERIALIZE()
};

using TreeCacheV1 = TreeCache<Selene, Helios>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
