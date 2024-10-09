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
#include "ringct/rctTypes.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

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

using OutputRef = crypto::hash;
inline OutputRef get_output_ref(const OutputPair &o)
{
    static_assert(sizeof(o.output_pubkey) == sizeof(o.commitment), "unexpected size of output pubkey & commitment");

    static const std::size_t N_ELEMS = 2;
    static_assert(sizeof(o) == (N_ELEMS * sizeof(crypto::public_key)), "unexpected size of output pair");

    const crypto::public_key data[N_ELEMS] = {o.output_pubkey, rct::rct2pk(o.commitment)};
    crypto::hash h;
    crypto::cn_fast_hash(data, N_ELEMS * sizeof(crypto::public_key), h);
    return h;
};

struct BlockMeta final
{
    BlockIdx blk_idx;
    BlockHash blk_hash;
    uint64_t n_leaf_tuples;
};

// TODO: we only need to ref count by chunks, not by individual records
// TODO: consider using additional bool path_member. Purge from cache on dequeue if path_member is false && ref_count == 0.
//       This could simplify the ref counting logic when adding path leaves and elems.
struct CachedTreeElem final
{
    std::array<uint8_t, 32UL> tree_elem;
    uint64_t ref_count;
};

struct CachedLeafTuple final
{
    OutputPair output;
    uint64_t ref_count;
};

struct AssignedLeafIdx final
{
    bool assigned_leaf_idx{false};
    LeafIdx leaf_idx{0};

    void assign_leaf(const LeafIdx idx) { leaf_idx = idx; assigned_leaf_idx = true; }
    void unassign_leaf() { leaf_idx = 0; assigned_leaf_idx = false; }
};

struct RegisteredOutputContext final
{
    OutputRef output_ref;
    bool included_in_tree{false};
};

using ChildChunkCache  = std::unordered_map<ChildChunkIdx, CachedTreeElem>;

// TODO: technically this can be a vector. There should *always* be at least 1 entry for every layer
using TreeElemCache    = std::unordered_map<LayerIdx, ChildChunkCache>;
using LeavesSet        = std::unordered_set<LeafIdx>;
using ChildChunkIdxSet = std::unordered_set<ChildChunkIdx>;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Syncs the tree, keeping track of known output paths
// - Wallets can use this object to sync the tree locally, making sure they can construct fcmp++'s for received outputs
//   using the outputs' latest paths in the tree, without revealing which output is being spent to the daemon.
// - The object does not store the entire tree locally. The object only stores what it needs in order to update paths
//   of known received outputs as it syncs.
// - The caller first calls register_output for any known received outputs.
// - The caller then calls sync_block, which identifies and updates known output paths in the tree.
// - The caller can get an output's latest path in the tree via get_output_path.
// - If there's a reorg, the caller can use pop_block, which trims the locally synced tree and updates paths as needed.
// - The memory footprint of the TreeSync object is roughly all known output paths and the last chunk of tree elems in
//   every layer of the tree for the last N blocks. The latter is required to handle reorgs up to N blocks deep.
// - WARNING: the implementation is not thread safe, it expects synchronous calls.
//   TODO: use a mutex to enforce thread safety.
template<typename C1, typename C2>
class TreeSync
{
public:
    TreeSync(std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
        const std::size_t max_reorg_depth = ORPHANED_BLOCKS_MAX_COUNT):
            m_curve_trees{curve_trees},
            m_max_reorg_depth{max_reorg_depth}
    {};

    // Registers an output with the TreeSync object so that syncing will keep track of the output's path in the tree
    // - Returns true on successful new insertion
    // - Returns false if the output is already registered
    // - Throws if the TreeSync object has already synced the block in which the output unlocks. The scanner would not
    //   be able to determine the output's position in the tree in this case
    bool register_output(const OutputPair &output, const uint64_t unlock_block_idx);

    // TODO: bool cancel_output_registration

    // Sync the leaf tuples from the provided block
    // - The block must be contiguous to the most recently synced block
    // - If any registered outputs are present in the new leaf tuples, keeps track of their paths in the tree
    // - Uses the new leaf tuples to update any existing known output paths in the tree
    void sync_block(const uint64_t block_idx,
        const crypto::hash &block_hash,
        const crypto::hash &prev_block_hash,
        std::vector<OutputContext> &&new_leaf_tuples);

    // Trim from the locally synced tree and update any paths as necesary
    // - Returns false if we cannot pop any more blocks (if the max reorg depth is reached, or no more blocks to pop)
    bool pop_block();

    // Get a registered output's path in the tree
    // - Returns false if the output is not registered
    // - Returns true with empty path_out if the output is registered but not yet included in the tree
    bool get_output_path(const OutputPair &output, typename CurveTrees<C1, C2>::Path &path_out) const;

// Internal helper functions
private:
    typename CurveTrees<C1, C2>::LastHashes get_last_hashes(const std::size_t n_leaf_tuples) const;

    typename CurveTrees<C1, C2>::LastChunkChildrenToTrim get_last_chunk_children_to_regrow(
        const std::vector<TrimLayerInstructions> &trim_instructions) const;

    typename CurveTrees<C1, C2>::LastHashes get_last_hashes_to_trim(
        const std::vector<TrimLayerInstructions> &trim_instructions) const;

    void deque_block(const BlockHash &block_hash);

// Internal member variables
private:
    std::shared_ptr<CurveTrees<C1, C2>> m_curve_trees;
    const std::size_t m_max_reorg_depth;

    // The outputs that TreeSync should keep track of while syncing
    std::unordered_map<OutputRef, AssignedLeafIdx> m_registered_outputs;

    // Cached leaves and tree elems
    std::unordered_map<LeafIdx, CachedLeafTuple> m_cached_leaves;
    TreeElemCache m_tree_elem_cache;

    // Keep track of cached tree elems that are not needed for path data and can be pruned from the cache once the cache
    // reaches m_max_reorg_depth
    std::unordered_map<BlockHash, LeavesSet> m_prunable_leaves_by_block;
    std::unordered_map<BlockHash, std::unordered_map<LayerIdx, ChildChunkIdxSet>> m_prunable_tree_elems_by_block;

    // Used for getting tree extensions and reductions when growing and trimming respectively
    // - These are unspecific to the wallet's registered outputs. These are strictly necessary to ensure we can rebuild
    //   the tree extensions and reductions for each block correctly locally when syncing.
    std::deque<BlockMeta> m_cached_blocks;

// TODO: serialization
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
