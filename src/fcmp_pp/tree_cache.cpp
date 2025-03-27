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

#include "tree_cache.h"

#include "common/merge_sorted_vectors.h"
#include "misc_log_ex.h"
#include "profile_tools.h"
#include "string_tools.h"

#include <algorithm>


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
static OutputRef get_output_ref(const OutputPair &o)
{
    static_assert(sizeof(o.output_pubkey) == sizeof(o.commitment), "unexpected size of output pubkey & commitment");

    static const std::size_t N_ELEMS = 2;
    static_assert(sizeof(o) == (N_ELEMS * sizeof(crypto::public_key)), "unexpected size of output pair");

    const crypto::public_key data[N_ELEMS] = {o.output_pubkey, rct::rct2pk(o.commitment)};
    crypto::hash h;
    crypto::cn_fast_hash(data, N_ELEMS * sizeof(crypto::public_key), h);
    return h;
};
//----------------------------------------------------------------------------------------------------------------------
static void assign_new_output(const OutputPair &output_pair,
    const LeafIdx leaf_idx,
    RegisteredOutputs &registered_outputs_inout)
{
    const auto output_ref = get_output_ref(output_pair);

    auto registered_output_it = registered_outputs_inout.find(output_ref);
    if (registered_output_it == registered_outputs_inout.end())
        return;

    // If it's already assigned a leaf idx, then it must be a duplicate and we only care about the earliest one
    // TODO: test this circumstance
    if (registered_output_it->second.assigned_leaf_idx)
        return;

    LOG_PRINT_L1("Found output " << output_pair.output_pubkey << " in curve tree at leaf idx " << leaf_idx);

    registered_output_it->second.assign_leaf(leaf_idx);

    return;
}
//----------------------------------------------------------------------------------------------------------------------
static uint64_t add_to_locked_outputs_cache(const OutsByLastLockedBlock &outs_by_last_locked_block,
    const CreatedBlockIdx created_block_idx,
    LockedOutsByLastLockedBlock &locked_outputs_inout,
    LockedOutputsByCreated &locked_outputs_refs_inout)
{
    uint64_t n_outputs_added = 0;

    LockedOutputRefs locked_output_refs;
    for (const auto &last_locked_block : outs_by_last_locked_block)
    {
        const LastLockedBlockIdx last_locked_block_idx = last_locked_block.first;
        CHECK_AND_ASSERT_THROW_MES(last_locked_block_idx > created_block_idx, "last locked block idx should be > created block");
        const auto &new_locked_outputs = last_locked_block.second;

        // We keep track of the number outputs we're adding to the cache at a specific last locked block, so that we can
        // quickly remove those outputs from the cache upon popping a block.
        const auto n_new_outputs = new_locked_outputs.size();
        locked_output_refs[last_locked_block_idx] = n_new_outputs;

        n_outputs_added += n_new_outputs;

        // Add to locked outputs cache by last locked block, so we can use them to grow the tree upon unlock.
        auto locked_outputs_it = locked_outputs_inout.find(last_locked_block_idx);
        if (locked_outputs_it == locked_outputs_inout.end())
        {
            locked_outputs_inout[last_locked_block_idx] = new_locked_outputs;
            continue;
        }

        // Merge existing sorted locked outputs with new sorted locked outputs
        const auto &locked_outputs = locked_outputs_it->second;
        std::vector<OutputContext> all_locked_outputs;
        const auto is_less = [](const OutputContext &a, const OutputContext &b) { return a.output_id < b.output_id; };
        bool r = tools::merge_sorted_vectors(locked_outputs, new_locked_outputs, is_less, all_locked_outputs);
        CHECK_AND_ASSERT_THROW_MES(r, "failed to merge sorted locked outputs");

        locked_outputs_inout[last_locked_block_idx] = std::move(all_locked_outputs);
    }

    // This is keeping track of locked output refs in the locked outputs cache by their created block. We use this to
    // quickly remove locked outputs form the cache cache upon popping the block from the chain.
    CHECK_AND_ASSERT_THROW_MES(locked_outputs_refs_inout.find(created_block_idx) == locked_outputs_refs_inout.end(),
        "unexpected locked output refs found");
    locked_outputs_refs_inout[created_block_idx] = std::move(locked_output_refs);

    return n_outputs_added;
}
//----------------------------------------------------------------------------------------------------------------------
static uint64_t remove_outputs_created_at_block(const CreatedBlockIdx &created_block_idx,
    LockedOutsByLastLockedBlock &locked_outputs_inout,
    LockedOutputsByCreated &locked_outputs_refs_inout)
{
    uint64_t n_outputs_removed = 0;

    // Get the outputs created at the provided creation block
    auto locked_output_refs_it = locked_outputs_refs_inout.find(created_block_idx);
    CHECK_AND_ASSERT_THROW_MES(locked_output_refs_it != locked_outputs_refs_inout.end(), "missing locked output refs");

    for (const auto &locked_output_refs : locked_output_refs_it->second)
    {
        // The outputs are grouped by last locked block
        const LastLockedBlockIdx last_locked_block_idx = locked_output_refs.first;
        const NumOutputs n_outputs_to_remove = locked_output_refs.second;

        // Find the locked outputs using the last locked block
        const auto locked_outputs_it = locked_outputs_inout.find(last_locked_block_idx);
        CHECK_AND_ASSERT_THROW_MES(locked_outputs_it != locked_outputs_inout.end(), "missing locked outputs");

        const NumOutputs n_cur_outputs = locked_outputs_it->second.size();
        CHECK_AND_ASSERT_THROW_MES(n_cur_outputs >= n_outputs_to_remove, "unexpected n locked outputs");

        // We're removing the number of outputs we originally added upon creation in add_to_locked_outputs_cache
        n_outputs_removed += n_outputs_to_remove;

        // Now remove those outputs from the locked outputs cache
        if (n_cur_outputs == n_outputs_to_remove)
        {
            locked_outputs_inout.erase(locked_outputs_it);
            continue;
        }

        locked_outputs_it->second.resize(n_cur_outputs - n_outputs_to_remove);
    }

    // Don't need the refs anymore, we're done with the outputs created at the given block
    locked_outputs_refs_inout.erase(locked_output_refs_it);

    return n_outputs_removed;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void assert_tuple_slice_is_in_bounds(const typename CurveTrees<C1, C2>::Leaves &leaves,
    const uint64_t start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples)
{
    CHECK_AND_ASSERT_THROW_MES(start_leaf_tuple_idx >= leaves.start_leaf_tuple_idx, "start_leaf_tuple_idx too low");

    const uint64_t n_leaf_tuples_ext = leaves.start_leaf_tuple_idx + leaves.tuples.size();
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples_ext >= n_leaf_tuples, "n_leaf_tuples is larger than leaves extension");

    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples >= start_leaf_tuple_idx,
        "total n leaf tuples must be > start leaf tuple idx");

    const uint64_t tuple_slice_size = n_leaf_tuples - start_leaf_tuple_idx;
    CHECK_AND_ASSERT_THROW_MES(leaves.tuples.size() >= tuple_slice_size, "tuple slice size is too large");
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_leaf_chunk(const ChildChunkIdx chunk_idx,
    const std::size_t leaf_parent_chunk_width,
    const typename CurveTrees<C1, C2>::Leaves &leaves,
    const LeafIdx start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples,
    const bool bump_ref_count,
    LeafCache &leaf_cache_inout)
{
    assert_tuple_slice_is_in_bounds<C1, C2>(leaves, start_leaf_tuple_idx, n_leaf_tuples);
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx start_leaf_idx = chunk_idx * leaf_parent_chunk_width;
    const LeafIdx end_leaf_idx   = std::min(start_leaf_idx + leaf_parent_chunk_width, n_leaf_tuples);

    CHECK_AND_ASSERT_THROW_MES(end_leaf_idx > start_leaf_idx, "start_leaf_idx is too high");

    MTRACE("Caching leaves at chunk_idx: " << chunk_idx
        << " , start_leaf_idx: "           << start_leaf_idx
        << " , end_leaf_idx: "             << end_leaf_idx
        << " , bump_ref_count: "           << bump_ref_count
        << " , start_leaf_tuple_idx: "     << start_leaf_tuple_idx);

    // If the leaf's chunk isn't present in this leaf extension, there are no new leaves we need to cache
    if (start_leaf_tuple_idx >= end_leaf_idx)
        return;

    // Check if the leaf's chunk is already cached
    uint64_t cached_chunk_size = 0;
    auto leaf_chunk_it = leaf_cache_inout.find(chunk_idx);
    const bool cache_hit = leaf_chunk_it != leaf_cache_inout.end();
    if (cache_hit)
    {
        if (bump_ref_count)
            leaf_chunk_it->second.ref_count += 1;

        cached_chunk_size = (uint64_t) leaf_chunk_it->second.leaves.size();
    }

    // Add the *new* elems in the chunk to the cache
    const ChildChunkIdx start_leaf_idx_offset = start_leaf_idx + cached_chunk_size;

    // If we already have all the latest leaves, we're done, we've already bumped the ref count if needed
    if (start_leaf_idx_offset == end_leaf_idx)
        return;
    CHECK_AND_ASSERT_THROW_MES(end_leaf_idx > start_leaf_idx_offset, "high start_leaf_idx_offset comp to end_leaf_idx");

    CHECK_AND_ASSERT_THROW_MES(start_leaf_idx_offset >= leaves.start_leaf_tuple_idx, "high start_leaf_idx_offset");
    const ChildChunkIdx start_i = start_leaf_idx_offset - leaves.start_leaf_tuple_idx;
    const ChildChunkIdx end_i = end_leaf_idx - leaves.start_leaf_tuple_idx;
    CHECK_AND_ASSERT_THROW_MES(leaves.tuples.size() >= end_i, "high end_i");

    std::vector<OutputPair> new_leaves;
    for (LeafIdx i = start_i; i < end_i; ++i)
    {
        const auto &output_pair = leaves.tuples[i].output_pair;
        if (cache_hit)
            leaf_chunk_it->second.leaves.push_back(output_pair);
        else
            new_leaves.push_back(output_pair);
    }

    // Add to the cache
    if (!cache_hit)
        leaf_cache_inout[chunk_idx] = CachedLeafChunk { .leaves = std::move(new_leaves), .ref_count = 1 };
}
//----------------------------------------------------------------------------------------------------------------------
// TODO: fewer params here?
template<typename C>
static void cache_path_chunk(const std::unique_ptr<C> &curve,
    const std::size_t parent_width,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const bool bump_ref_count,
    const ChildChunkIdx parent_idx,
    const uint64_t n_layer_elems,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "high layer_ext_idx");
    auto &layer_ext = layer_exts[layer_ext_idx];

    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "empty layer ext");
    const uint64_t n_layer_elems_ext = layer_ext.start_idx + layer_ext.hashes.size();
    CHECK_AND_ASSERT_THROW_MES(n_layer_elems_ext >= n_layer_elems, "high n_layer_elems");

    const ChildChunkIdx start_chunk_idx = parent_idx * parent_width;
    const ChildChunkIdx end_chunk_idx   = std::min(start_chunk_idx + parent_width, n_layer_elems);
    CHECK_AND_ASSERT_THROW_MES(end_chunk_idx > start_chunk_idx, "end_chunk_idx is too low");

    MTRACE("Caching path elems at layer_idx: " << layer_idx
        << " , parent_idx: "                   << parent_idx
        << " , start_chunk_idx: "              << start_chunk_idx
        << " , end_chunk_idx: "                << end_chunk_idx
        << " , bump_ref_count: "               << bump_ref_count
        << " , n_layer_elems: "                << n_layer_elems
        << " , layer_ext.start_idx: "          << layer_ext.start_idx);

    // Check if the layer is already cached
    auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
    const bool layer_cache_hit = cached_layer_it != cached_tree_elems_inout.end();

    // Check if the path chunk is already cached
    bool cache_hit = false;
    uint64_t cached_chunk_size = 0;
    ChildChunkCache::iterator cached_chunk_it;
    if (layer_cache_hit)
    {
        cached_chunk_it = cached_layer_it->second.find(parent_idx);
        cache_hit = cached_chunk_it != cached_layer_it->second.end();

        if (cache_hit)
        {
            if (bump_ref_count)
                cached_chunk_it->second.ref_count += 1;

            cached_chunk_size = (uint64_t) cached_chunk_it->second.tree_elems.size();
        }
    }

    MTRACE("layer_cache_hit: "        << layer_cache_hit
        << " , cache_hit: "           << cache_hit
        << " , cached_chunk_size: "   << cached_chunk_size);

    // Add the *new* elems in the chunk to the cache
    const ChildChunkIdx start_idx_offset = start_chunk_idx + cached_chunk_size;

    // If we already have all the latest elems, we're done, we've already bumped the ref count if needed
    if (start_idx_offset == end_chunk_idx)
        return;
    CHECK_AND_ASSERT_THROW_MES(end_chunk_idx > start_idx_offset, "high start_idx_offset comp to end_chunk_idx");

    CHECK_AND_ASSERT_THROW_MES(start_idx_offset >= layer_ext.start_idx, "high start_idx_offset");
    const ChildChunkIdx start_i = start_idx_offset - layer_ext.start_idx;
    const ChildChunkIdx end_i = end_chunk_idx - layer_ext.start_idx;
    CHECK_AND_ASSERT_THROW_MES(layer_ext.hashes.size() >= end_i, "high end_i");

    // Collect the new elems into cache
    std::vector<crypto::ec_point> new_elems;
    for (ChildChunkIdx i = start_i; i < end_i; ++i)
    {
        const auto tree_elem_bytes = curve->to_bytes(layer_ext.hashes[i]);
        if (cache_hit)
            cached_chunk_it->second.tree_elems.push_back(tree_elem_bytes);
        else
            new_elems.push_back(tree_elem_bytes);
    }

    // If no cache hit, add collected chunk to the cache
    if (!layer_cache_hit)
    {
        cached_tree_elems_inout[layer_idx] = {{ parent_idx, CachedTreeElemChunk{
                .tree_elems = std::move(new_elems),
                .ref_count  = 1,
            }}};
    }
    else if (!cache_hit)
    {
        cached_tree_elems_inout[layer_idx][parent_idx] = CachedTreeElemChunk{
                .tree_elems = std::move(new_elems),
                .ref_count  = 1,
            };
    }
}
//----------------------------------------------------------------------------------------------------------------------
static ChildChunkCache::const_iterator read_child_chunk(const std::size_t layer_idx,
    const ChildChunkIdx child_chunk_idx,
    const TreeElemCache &tree_elem_cache)
{
    MTRACE("Reading cached layer " << layer_idx << " and child chunk idx " << child_chunk_idx);

    const auto layer_it = tree_elem_cache.find(layer_idx);
    CHECK_AND_ASSERT_THROW_MES(layer_it != tree_elem_cache.end(), "missing cached layer");

    const auto child_chunk_it = layer_it->second.find(child_chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_it != layer_it->second.end(), "missing cached child chunk");

    CHECK_AND_ASSERT_THROW_MES(!child_chunk_it->second.tree_elems.empty(), "empty child chunk cache");
    return child_chunk_it;
}
//----------------------------------------------------------------------------------------------------------------------
static ChildChunkCache::iterator get_child_chunk_it(const std::size_t layer_idx,
    const ChildChunkIdx child_chunk_idx,
    TreeElemCache &tree_elem_cache)
{
    MTRACE("Reading cached layer " << layer_idx << " and child chunk idx " << child_chunk_idx);

    auto layer_it = tree_elem_cache.find(layer_idx);
    CHECK_AND_ASSERT_THROW_MES(layer_it != tree_elem_cache.end(), "missing cached layer");

    auto child_chunk_it = layer_it->second.find(child_chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_it != layer_it->second.end(), "missing cached child chunk");

    CHECK_AND_ASSERT_THROW_MES(!child_chunk_it->second.tree_elems.empty(), "empty child chunk cache");
    return child_chunk_it;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void update_last_hash(const std::unique_ptr<C> &curve,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const ChildChunkIdx last_parent_idx,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "high layer_ext_idx");
    auto &layer_ext = layer_exts[layer_ext_idx];

    if (!layer_ext.update_existing_last_hash)
        return;
    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "empty layer ext");

    auto cached_chunk_it = get_child_chunk_it(layer_idx, last_parent_idx, cached_tree_elems_inout);
    cached_chunk_it->second.tree_elems.back() = curve->to_bytes(layer_ext.hashes.front());
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_path_chunks(const LeafIdx leaf_idx,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const std::vector<LayerExtension<C1>> &c1_layer_exts,
    const std::vector<LayerExtension<C2>> &c2_layer_exts,
    const uint64_t start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples,
    const bool bump_ref_count,
    TreeElemCache &tree_elem_cache_inout)
{
    if (n_leaf_tuples == 0)
        return;
    if (n_leaf_tuples == start_leaf_tuple_idx)
        return;

    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples > leaf_idx, "high leaf_idx");

    // Get the child chunk indexes of the leaf for each layer
    const auto child_chunk_idxs = curve_trees->get_child_chunk_indexes(n_leaf_tuples, leaf_idx);
    const auto n_elems_per_layer = curve_trees->n_elems_per_layer(n_leaf_tuples);
    const std::size_t n_layers = n_elems_per_layer.size();
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (n_layers + 1), "unexpected n child chunk idxs");

    std::size_t c1_idx = 0, c2_idx = 0;
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        const ChildChunkIdx parent_idx = child_chunk_idxs[layer_idx + 1];
        MTRACE("Caching tree elems from layer_idx " << layer_idx << " parent_idx " << parent_idx);

        if (c1_idx == c2_idx /*c2 parent*/)
        {
            cache_path_chunk(curve_trees->m_c1,
                    curve_trees->m_c2_width,
                    c1_layer_exts,
                    c1_idx,
                    layer_idx,
                    bump_ref_count,
                    parent_idx,
                    n_elems_per_layer[layer_idx],
                    tree_elem_cache_inout
                );

            ++c1_idx;
        }
        else
        {
            cache_path_chunk(curve_trees->m_c2,
                    curve_trees->m_c1_width,
                    c2_layer_exts,
                    c2_idx,
                    layer_idx,
                    bump_ref_count,
                    parent_idx,
                    n_elems_per_layer[layer_idx],
                    tree_elem_cache_inout
                );

            ++c2_idx;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void update_existing_last_hashes(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const typename CurveTrees<C1, C2>::TreeExtension &tree_extension,
    TreeElemCache &tree_elem_cache_inout)
{
    const uint64_t old_n_leaf_tuples = tree_extension.leaves.start_leaf_tuple_idx;
    if (old_n_leaf_tuples == 0)
        return;

    const auto &c1_layer_exts = tree_extension.c1_layer_extensions;
    const auto &c2_layer_exts = tree_extension.c2_layer_extensions;

    // Get the child chunk indexes of the last leaf for each layer
    const uint64_t last_leaf_idx = old_n_leaf_tuples - 1;
    const auto child_chunk_idxs = curve_trees->get_child_chunk_indexes(old_n_leaf_tuples, last_leaf_idx);
    const std::size_t n_layers = curve_trees->n_layers(old_n_leaf_tuples);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (n_layers + 1), "unexpected n child chunk idxs");

    std::size_t c1_idx = 0, c2_idx = 0;
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        const ChildChunkIdx last_parent_idx = child_chunk_idxs[layer_idx + 1];
        MTRACE("Updating existing last hash from layer_idx " << layer_idx << " last_parent_idx " << last_parent_idx);

        if (c1_idx == c2_idx /*c2 parent*/)
        {
            update_last_hash(curve_trees->m_c1,
                    c1_layer_exts,
                    c1_idx,
                    layer_idx,
                    last_parent_idx,
                    tree_elem_cache_inout
                );

            ++c1_idx;
        }
        else
        {
            update_last_hash(curve_trees->m_c2,
                    c2_layer_exts,
                    c2_idx,
                    layer_idx,
                    last_parent_idx,
                    tree_elem_cache_inout
                );

            ++c2_idx;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void remove_leaf_chunk_ref(const ChildChunkIdx chunk_idx, LeafCache &leaf_cache_inout)
{
    auto leaf_chunk_it = leaf_cache_inout.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != leaf_cache_inout.end(), "cache is missing leaf chunk");
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it->second.ref_count != 0, "leaf chunk has 0 ref count");

    leaf_chunk_it->second.ref_count -= 1;
    MTRACE("Removing leaf chunk " << chunk_idx << " , updated ref count: " << leaf_chunk_it->second.ref_count);

    // If the ref count is 0, garbage collect it
    if (leaf_chunk_it->second.ref_count == 0)
        leaf_cache_inout.erase(leaf_chunk_it);
}
//----------------------------------------------------------------------------------------------------------------------
static void remove_path_chunk_ref(const LayerIdx layer_idx,
    const ChildChunkIdx chunk_idx,
    TreeElemCache &tree_elem_cache_inout)
{
    // Get the layer
    auto cache_layer_it = tree_elem_cache_inout.find(layer_idx);
    CHECK_AND_ASSERT_THROW_MES(cache_layer_it != tree_elem_cache_inout.end(), "layer is missing");

    // Get the chunk
    auto cache_chunk_it = cache_layer_it->second.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(cache_chunk_it != cache_layer_it->second.end(), "chunk is missing");
    CHECK_AND_ASSERT_THROW_MES(cache_chunk_it->second.ref_count != 0, "chunk has 0 ref count");

    cache_chunk_it->second.ref_count -= 1;
    MTRACE("Removing ref to chunk " << chunk_idx << " in layer " << layer_idx
        << " , updated ref count: " << cache_chunk_it->second.ref_count);

    // If the chunk's ref count is 0, garbage collect it
    if (cache_chunk_it->second.ref_count == 0)
        cache_layer_it->second.erase(cache_chunk_it);

    // If the layer is empty, garbage collect it
    if (cache_layer_it->second.empty())
        tree_elem_cache_inout.erase(cache_layer_it);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void remove_path_chunks_refs(const LeafIdx leaf_idx,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const uint64_t n_leaf_tuples,
    TreeElemCache &tree_elem_cache_inout)
{
    if (n_leaf_tuples == 0)
        return;

    const auto child_chunk_idxs = curve_trees->get_child_chunk_indexes(n_leaf_tuples, leaf_idx);
    const std::size_t n_layers = curve_trees->n_layers(n_leaf_tuples);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (n_layers + 1), "unexpected n child chunk idxs");

    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        const ChildChunkIdx parent_idx = child_chunk_idxs[layer_idx + 1];
        remove_path_chunk_ref(layer_idx, parent_idx, tree_elem_cache_inout);
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void shrink_cached_last_leaf_chunk(const uint64_t new_n_leaf_tuples,
    const std::size_t leaf_parent_chunk_width,
    LeafCache &leaf_cache_inout)
{
    // If the offset is 0, the last chunk is full and we're supposed to keep all elems in it
    const std::size_t offset = new_n_leaf_tuples % leaf_parent_chunk_width;
    if (offset == 0)
        return;

    const LeafIdx last_leaf_idx = new_n_leaf_tuples - 1;
    const ChildChunkIdx chunk_idx = last_leaf_idx / leaf_parent_chunk_width;

    auto leaf_chunk_it = leaf_cache_inout.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != leaf_cache_inout.end(), "cache is missing leaf chunk to shrink");

    // The last chunk should have at least offset leaves
    const std::size_t n_leaves_last_chunk = leaf_chunk_it->second.leaves.size();
    CHECK_AND_ASSERT_THROW_MES(n_leaves_last_chunk >= offset, "unexpected n leaves in cached last chunk");

    leaf_chunk_it->second.leaves.resize(offset);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void reduce_cached_last_chunks(const uint64_t new_n_leaf_tuples,
    const std::vector<crypto::ec_point> &new_tree_edge,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    TreeElemCache &tree_elem_cache_inout)
{
    if (new_n_leaf_tuples == 0)
        return;

    // Get child chunk indexes and layer meta
    const LeafIdx last_leaf_idx = new_n_leaf_tuples - 1;
    const auto child_chunk_idxs = curve_trees->get_child_chunk_indexes(new_n_leaf_tuples, last_leaf_idx);
    const auto n_elems_per_layer = curve_trees->n_elems_per_layer(new_n_leaf_tuples);
    const std::size_t n_layers = n_elems_per_layer.size();
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (n_layers + 1), "unexpected n child chunk idxs");
    CHECK_AND_ASSERT_THROW_MES(new_tree_edge.size() == n_layers, "unexpected tree edge size");

    bool parent_is_c2 = true;
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        const ChildChunkIdx parent_idx = child_chunk_idxs[layer_idx + 1];
        auto cached_chunk_it = get_child_chunk_it(layer_idx, parent_idx, tree_elem_cache_inout);

        // Shrink the chunk to the expected size
        const uint64_t n_layer_elems = n_elems_per_layer[layer_idx];
        const std::size_t parent_width = parent_is_c2 ? curve_trees->m_c2_width : curve_trees->m_c1_width;
        const std::size_t chunk_offset = n_layer_elems % parent_width;
        const std::size_t new_chunk_size = chunk_offset == 0 ? parent_width : chunk_offset;
        CHECK_AND_ASSERT_THROW_MES(new_chunk_size > 0, "new_chunk_size is too small");

        MTRACE("Reducing cached last chunk in layer_idx: " << layer_idx
            << " , parent_idx: "                           << parent_idx
            << " , n_layer_elems: "                        << n_layer_elems
            << " , new_chunk_size: "                       << new_chunk_size);

        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it->second.tree_elems.size() >= new_chunk_size, "chunk is too small");
        cached_chunk_it->second.tree_elems.resize(new_chunk_size);

        // Update the last hash in the chunk
        cached_chunk_it->second.tree_elems.back() = new_tree_edge[layer_idx];

        parent_is_c2 = !parent_is_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void update_registered_path(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const LeafIdx leaf_idx,
    const typename CurveTrees<C1, C2>::TreeExtension &tree_extension,
    const LeafIdx start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples,
    LeafCache &leaf_cache_inout,
    TreeElemCache &tree_elem_cach_inout)
{
    assert_tuple_slice_is_in_bounds<C1, C2>(tree_extension.leaves, start_leaf_tuple_idx, n_leaf_tuples);
    if (n_leaf_tuples == 0)
        return;

    // We only need to bump the ref count on this registered output's leaf chunk if it was just included in the tree
    const bool bump_ref_count = leaf_idx >= start_leaf_tuple_idx && leaf_idx < n_leaf_tuples;

    // Cache registered leaf's chunk
    cache_leaf_chunk<C1, C2>(leaf_idx / curve_trees->m_c1_width,
        curve_trees->m_c1_width,
        tree_extension.leaves,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        bump_ref_count,
        leaf_cache_inout);

    // Now cache the rest of the path elems for each registered output
    // FIXME: 2 registered outputs share a parent chunk. The leaves were **already** in the chain so bump_ref_count is
    // false here, but we're adding a new parent this tree extension, or new members to an existing parent chunk. The
    // ref count on newly included elems will only go up for 1 of those registered outputs.
    cache_path_chunks<C1, C2>(leaf_idx,
        curve_trees,
        tree_extension.c1_layer_extensions,
        tree_extension.c2_layer_extensions,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        bump_ref_count,
        tree_elem_cach_inout);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_last_chunk_leaves(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const typename CurveTrees<C1, C2>::Leaves &leaves,
    const LeafIdx start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples,
    LeafCache &leaf_cache_inout)
{
    assert_tuple_slice_is_in_bounds<C1, C2>(leaves, start_leaf_tuple_idx, n_leaf_tuples);
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx last_leaf_idx = n_leaf_tuples - 1;
    const ChildChunkIdx chunk_idx = last_leaf_idx / curve_trees->m_c1_width;

    // Always bump the ref count for last chunk of leaves so that it sticks around until pruned
    const bool bump_ref_count = true;

    cache_leaf_chunk<C1, C2>(chunk_idx,
        curve_trees->m_c1_width,
        leaves,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        bump_ref_count,
        leaf_cache_inout);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_last_chunks(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const typename CurveTrees<C1, C2>::TreeExtension &tree_extension,
    const LeafIdx start_leaf_tuple_idx,
    const uint64_t n_leaf_tuples,
    TreeElemCache &tree_elem_cache_inout)
{
    assert_tuple_slice_is_in_bounds<C1, C2>(tree_extension.leaves, start_leaf_tuple_idx, n_leaf_tuples);
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx last_leaf_idx = n_leaf_tuples - 1;

    // Always bump the ref count for last chunk of hashes so that it sticks around until pruned
    const bool bump_ref_count = true;

    cache_path_chunks<C1, C2>(last_leaf_idx,
        curve_trees,
        tree_extension.c1_layer_extensions,
        tree_extension.c2_layer_extensions,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        bump_ref_count,
        tree_elem_cache_inout);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeCache<C1, C2>::register_output(const OutputPair &output, const uint64_t last_locked_block_idx)
{
    if (!m_cached_blocks.empty())
    {
        const auto &top_synced_block = m_cached_blocks.back();

        // If the output is already unlocked, we won't be able to tell the output's position in the tree
        CHECK_AND_ASSERT_MES(last_locked_block_idx > top_synced_block.blk_idx, false,
            "already synced output's last locked block");
    }

    auto output_ref = get_output_ref(output);
    CHECK_AND_ASSERT_MES(m_registered_outputs.find(output_ref) == m_registered_outputs.end(), false,
        "output is already registered");

    // Add to registered outputs container
    m_registered_outputs.insert({ std::move(output_ref), AssignedLeafIdx{} });

    MDEBUG("Registered output " << output.output_pubkey << " , commitment " << output.commitment);

    return true;
}

// Explicit instantiation
template bool TreeCache<Selene, Helios>::register_output(const OutputPair &output, const uint64_t last_locked_block_idx);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    const fcmp_pp::curve_trees::OutsByLastLockedBlock &outs_by_last_locked_block)
{
    const std::vector<crypto::hash> new_block_hashes{block_hash};
    const std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> outs{outs_by_last_locked_block};

    CacheStateChange cache_state_change;

    this->prepare_to_grow_cache(block_idx,
        prev_block_hash,
        new_block_hashes,
        outs,
        cache_state_change);

    this->grow_cache(block_idx, new_block_hashes, std::move(cache_state_change));
}

// Explicit instantiation
template void TreeCache<Selene, Helios>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    const fcmp_pp::curve_trees::OutsByLastLockedBlock &outs_by_last_locked_block);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::prepare_to_grow_cache(const uint64_t start_block_idx,
    const crypto::hash &prev_block_hash,
    const std::vector<crypto::hash> &new_block_hashes,
    const std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> &outs_by_last_locked_blocks,
    CacheStateChange &cache_state_change_out) const
{
    CHECK_AND_ASSERT_THROW_MES(new_block_hashes.size() == outs_by_last_locked_blocks.size(), "size mismatch sync_blocks");

    cache_state_change_out = {};

    const uint64_t n_new_blocks = (uint64_t) new_block_hashes.size();
    if (n_new_blocks == 0)
        return;

    // Pre-checks
    if (m_cached_blocks.empty())
    {
        CHECK_AND_ASSERT_THROW_MES(start_block_idx == 0, "must init before sync_blocks");
        CHECK_AND_ASSERT_THROW_MES(prev_block_hash == crypto::null_hash, "expected null prev last hash");

        // Make sure all blockchain containers are empty
        CHECK_AND_ASSERT_THROW_MES(m_cached_blocks.empty(),   "expected empty cached blocks");
        CHECK_AND_ASSERT_THROW_MES(m_leaf_cache.empty(),      "expected empty cached leaves");
        CHECK_AND_ASSERT_THROW_MES(m_tree_elem_cache.empty(), "expected empty cached tree elems");
    }
    else
    {
        // Make sure provided block is contiguous to prior synced block
        const auto &prev_block = m_cached_blocks.back();
        CHECK_AND_ASSERT_THROW_MES((prev_block.blk_idx + 1) == start_block_idx, "failed contiguity idx check");
        CHECK_AND_ASSERT_THROW_MES(prev_block.blk_hash == prev_block_hash, "failed contiguity hash check");
    }

    // Copy the cache's locked outputs and locked output refs so that this function makes no modifications to
    // the existing cache, and its results can be used to update the cache
    // TODO: return state diff instead of copying the whole thing
    TIME_MEASURE_START(getting_unlocked_outputs);
    cache_state_change_out.locked_outputs = m_locked_outputs;
    cache_state_change_out.locked_output_refs = m_locked_output_refs;

    // Update the locked outputs cache with all outputs set to unlock, and collect unlocked outputs and output id's
    std::vector<std::vector<OutputContext>> unlocked_outputs;
    std::vector<std::vector<uint64_t>> unlocked_output_ids_by_block;
    unlocked_outputs.reserve(n_new_blocks);
    unlocked_output_ids_by_block.reserve(n_new_blocks);
    uint64_t n_unlocked_outputs = 0;
    for (uint64_t i = 0; i < n_new_blocks; ++i)
    {
        const BlockIdx blk_idx = start_block_idx + i;

        cache_state_change_out.n_outputs_observed += add_to_locked_outputs_cache(outs_by_last_locked_blocks[i],
                blk_idx,
                cache_state_change_out.locked_outputs,
                cache_state_change_out.locked_output_refs
            );

        // Copy the unlocked outputs in the block. The reason we copy here is to make sure we handle reorgs correctly.
        // We don't need to re-add locked outputs back to the cache upon popping a block this way.
        auto unlocked_outputs_in_blk = cache_state_change_out.locked_outputs[blk_idx];
        const std::size_t n_new_unlocked_outputs = unlocked_outputs_in_blk.size();

        n_unlocked_outputs += n_new_unlocked_outputs;

        // Collect unlock output id's by block
        std::vector<uint64_t> new_unlocked_output_ids;
        new_unlocked_output_ids.reserve(n_new_unlocked_outputs);
        for (const auto &unlocked_output : unlocked_outputs_in_blk)
            new_unlocked_output_ids.push_back(unlocked_output.output_id);

        unlocked_outputs.emplace_back(std::move(unlocked_outputs_in_blk));
        unlocked_output_ids_by_block.emplace_back(std::move(new_unlocked_output_ids));
    }
    TIME_MEASURE_FINISH(getting_unlocked_outputs);

    TIME_MEASURE_START(getting_tree_extension);
    // Get the tree extension using existing tree data. We'll use the tree extension to update registered output paths
    // in the tree and cache the data necessary to either build the next block's tree extension or pop the block.
    cache_state_change_out.tree_extension = TreeSync<C1, C2>::m_curve_trees->get_tree_extension(
        this->get_n_leaf_tuples(),
        this->get_last_hashes(),
        std::move(unlocked_outputs),
        true/*use_fast_torsion_check*/);

    CHECK_AND_ASSERT_THROW_MES(n_unlocked_outputs >= cache_state_change_out.tree_extension.leaves.tuples.size(),
        "unexpected new n tuples");

    TIME_MEASURE_FINISH(getting_tree_extension);

    // Read the tree extension and determine n leaf tuples added per block
    cache_state_change_out.n_new_leaf_tuples_per_block.reserve(n_new_blocks);
    auto new_leaf_tuple_it = cache_state_change_out.tree_extension.leaves.tuples.begin();
    for (uint64_t i = 0; i < n_new_blocks; ++i)
    {
        uint64_t n_leaf_tuples_in_block = 0;

        const auto &unlocked_output_ids = unlocked_output_ids_by_block[i];
        for (const uint64_t output_id : unlocked_output_ids)
        {
            // This expects the unlocked outputs in a block to be inserted to the tree in sorted order
            if (output_id == new_leaf_tuple_it->output_id)
            {
                ++n_leaf_tuples_in_block;
                ++new_leaf_tuple_it;
            }
        }

        cache_state_change_out.n_new_leaf_tuples_per_block.push_back(n_leaf_tuples_in_block);
    }

    CHECK_AND_ASSERT_THROW_MES(new_leaf_tuple_it == cache_state_change_out.tree_extension.leaves.tuples.end(),
        "did not reach all leaf tuples");

    m_getting_unlocked_outs_ms += getting_unlocked_outputs;
    m_getting_tree_extension_ms += getting_tree_extension;

    LOG_PRINT_L1("Total time getting unlocked outs: " << m_getting_unlocked_outs_ms / 1000
        << " , getting tree extension: " << m_getting_tree_extension_ms / 1000);
}

// Explicit instantiation
template void TreeCache<Selene, Helios>::prepare_to_grow_cache(const uint64_t start_block_idx,
    const crypto::hash &prev_block_hash,
    const std::vector<crypto::hash> &new_block_hashes,
    const std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> &outs_by_last_locked_blocks,
    CacheStateChange &cache_state_change_out) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::grow_cache(const uint64_t start_block_idx,
    const std::vector<crypto::hash> &new_block_hashes,
    CacheStateChange &&cache_state_change,
    const bool skip_shrink_to_reorg_depth)
{
    // Pre-checks
    CHECK_AND_ASSERT_THROW_MES(new_block_hashes.size() == cache_state_change.n_new_leaf_tuples_per_block.size(),
        "size mismatch new block hashes <> n new leaf tuples");

    uint64_t n_leaf_tuples = 0;
    if (m_cached_blocks.empty())
    {
        CHECK_AND_ASSERT_THROW_MES(start_block_idx == 0, "must init first");

        // Make sure all blockchain containers are empty
        CHECK_AND_ASSERT_THROW_MES(m_cached_blocks.empty(),   "expected empty cached blocks");
        CHECK_AND_ASSERT_THROW_MES(m_leaf_cache.empty(),      "expected empty cached leaves");
        CHECK_AND_ASSERT_THROW_MES(m_tree_elem_cache.empty(), "expected empty cached tree elems");
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(start_block_idx > 0, "expected start_block_idx > 0");

        // Make sure provided block is contiguous to prior synced block
        const auto &prev_block = m_cached_blocks.back();
        CHECK_AND_ASSERT_THROW_MES((prev_block.blk_idx + 1) == start_block_idx,
            "failed contiguity idx check processing synced blocks");

        n_leaf_tuples = prev_block.n_leaf_tuples;
    }

    // Update the output count
    m_output_count += cache_state_change.n_outputs_observed;

    // Set the next locked outputs and refs
    m_locked_outputs = std::move(cache_state_change.locked_outputs);
    m_locked_output_refs = std::move(cache_state_change.locked_output_refs);

    // Update the existing last hashes in the cache using the tree extension
    const auto &tree_extension = cache_state_change.tree_extension;
    update_existing_last_hashes<C1, C2>(TreeSync<C1, C2>::m_curve_trees, tree_extension, m_tree_elem_cache);

    // Go block-by-block using slices of the tree extension to update values in the cache
    uint64_t tuple_idx_start_slice = 0;
    for (std::size_t i = 0; i < new_block_hashes.size(); ++i)
    {
        const uint64_t n_new_leaf_tuples = cache_state_change.n_new_leaf_tuples_per_block[i];
        n_leaf_tuples += n_new_leaf_tuples;

        const LeafIdx start_leaf_tuple_idx = tree_extension.leaves.start_leaf_tuple_idx + tuple_idx_start_slice;

        MDEBUG("Processing synced block "  << new_block_hashes[i]
            << " , block idx: "            << start_block_idx + i
            << " , n_leaf_tuples: "        << n_leaf_tuples
            << " , start_leaf_tuple_idx: " << start_leaf_tuple_idx);

        // Check if any registered outputs are present in the tree extension. If so, we assign the output its leaf idx
        // and start keeping track of the output's path elems
        for (uint64_t i = 0; i < n_new_leaf_tuples; ++i)
        {
            const LeafIdx tuple_idx = tuple_idx_start_slice + i;
            CHECK_AND_ASSERT_THROW_MES(tree_extension.leaves.tuples.size() > tuple_idx, "unexpected tuple_idx");

            const auto &output_pair = tree_extension.leaves.tuples[tuple_idx].output_pair;
            const LeafIdx leaf_idx = start_leaf_tuple_idx + i;
            assign_new_output(output_pair, leaf_idx, m_registered_outputs);
        }
        tuple_idx_start_slice += n_new_leaf_tuples;

        // Cache tree elems from the tree extension needed in order to keep track of registered output paths in the tree
        for (const auto &registered_o : m_registered_outputs)
        {
            // Skip all registered outputs which have not been included in the tree yet
            if (!registered_o.second.assigned_leaf_idx)
                continue;

            update_registered_path<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
                registered_o.second.leaf_idx,
                cache_state_change.tree_extension,
                start_leaf_tuple_idx,
                n_leaf_tuples,
                m_leaf_cache,
                m_tree_elem_cache);
        }

        // Cache the last chunk of leaves, so if a registered output appears in the first chunk next block, we'll have
        // all prior leaves from that output's chunk already saved
        cache_last_chunk_leaves<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
            tree_extension.leaves,
            start_leaf_tuple_idx,
            n_leaf_tuples,
            m_leaf_cache);

        // Cache the last chunk of hashes from every layer. We do this to handle:
        //   1) So we can use the tree's last hashes to grow the tree from here next block.
        //   2) In case a registered output appears in the first chunk next block, we'll have all its path elems cached.
        cache_last_chunks<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
            tree_extension,
            start_leaf_tuple_idx,
            n_leaf_tuples,
            m_tree_elem_cache);

        // Enqueue block meta
        const BlockIdx blk_idx = start_block_idx + i;
        const auto &blk_hash = new_block_hashes[i];
        auto blk_meta = BlockMeta {
            .blk_idx       = blk_idx,
            .blk_hash      = blk_hash,
            .n_leaf_tuples = n_leaf_tuples,
        };
        m_cached_blocks.push_back(std::move(blk_meta));
    }
    CHECK_AND_ASSERT_THROW_MES(tuple_idx_start_slice == tree_extension.leaves.tuples.size(),
        "did not account for all new leaf tuples");

    if (!skip_shrink_to_reorg_depth)
        this->shrink_to_reorg_depth();
}

template void TreeCache<Selene, Helios>::grow_cache(const uint64_t start_block_idx,
    const std::vector<crypto::hash> &new_block_hashes,
    CacheStateChange &&cache_state_change,
    const bool skip_shrink_to_reorg_depth);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::shrink_to_reorg_depth()
{
    // Deque the oldest cached block upon reaching the max reorg depth
    while ((uint64_t)m_cached_blocks.size() > TreeSync<C1, C2>::m_max_reorg_depth)
    {
        const auto &oldest_block = m_cached_blocks.front();

        // All locked outputs that unlocked in the oldest block idx should already be in the tree. We keep them cached
        // to handle reorgs (in case an output trimmed from the tree is supposed to re-enter the cache). We don't need
        // to keep them past the reorg depth.
        m_locked_outputs.erase(/*LastLockedBlockIdx*/oldest_block.blk_idx);

        // We keep locked output refs around for outputs *created* in the oldest block, so we can quickly remove them
        // from the locked outputs cache upon popping the block. Once the reorg depth is exceeded, we can't remove those
        // outputs anyway, so remove from the cache.
        m_locked_output_refs.erase(/*CreatedBlockIdx*/oldest_block.blk_idx);

        this->deque_block(oldest_block.n_leaf_tuples);
        m_cached_blocks.pop_front();
    }
}

// Explicit instantiation
template void TreeCache<Selene, Helios>::shrink_to_reorg_depth();
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeCache<C1, C2>::pop_block()
{
    if (m_cached_blocks.empty())
        return false;

    // Pop the top block off the cache, removing refs to last chunks
    const uint64_t old_n_leaf_tuples = m_cached_blocks.back().n_leaf_tuples;
    const BlockIdx pop_block_idx = m_cached_blocks.back().blk_idx;

    MDEBUG("Popping block " << pop_block_idx << " , " << m_cached_blocks.back().blk_hash << " from tree cache");

    this->deque_block(old_n_leaf_tuples);
    m_cached_blocks.pop_back();

    // Remove locked outputs from the cache that were created in this block
    const uint64_t n_outputs_removed = remove_outputs_created_at_block(
        pop_block_idx,
        m_locked_outputs,
        m_locked_output_refs);
    CHECK_AND_ASSERT_THROW_MES(m_output_count >= n_outputs_removed, "output count too low");
    m_output_count -= n_outputs_removed;

    // Determine how many leaves we need to trim
    uint64_t new_n_leaf_tuples = 0;
    if (!m_cached_blocks.empty())
    {
        // We're trimming to the new top block
        const BlockMeta &new_top_block = m_cached_blocks.back();
        new_n_leaf_tuples = new_top_block.n_leaf_tuples;
    }
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples >= new_n_leaf_tuples, "expected old_n_leaf_tuples >= new_n_leaf_tuples");
    // No leaves to trim, safe return
    if (old_n_leaf_tuples == new_n_leaf_tuples)
        return true;

    // Shrink the current last chunk if some of the leaves in it got cut off
    shrink_cached_last_leaf_chunk(new_n_leaf_tuples, TreeSync<C1, C2>::m_curve_trees->m_c1_width, m_leaf_cache);

    // Get the tree edge when the top block was the top block
    const auto new_tree_edge = this->get_tree_edge(new_n_leaf_tuples);

    // Update ref'd last hashes and shrink current last chunks as necessary
    reduce_cached_last_chunks<C1, C2>(new_n_leaf_tuples,
        new_tree_edge,
        TreeSync<C1, C2>::m_curve_trees,
        m_tree_elem_cache);

    // Update registered output path refs
    for (auto &registered_o : m_registered_outputs)
    {
        // If the output isn't in the tree, it has no path elems we need to change in the cache 
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        // If the output remains in the tree, its chunk refs remain unchanged
        const LeafIdx leaf_idx = registered_o.second.leaf_idx;
        if (new_n_leaf_tuples > leaf_idx)
            continue;

        // The output was just removed from the tree, so remove its refs
        const ChildChunkIdx leaf_chunk_idx = leaf_idx / TreeSync<C1, C2>::m_curve_trees->m_c1_width;
        remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);
        remove_path_chunks_refs(leaf_idx, TreeSync<C1, C2>::m_curve_trees, old_n_leaf_tuples, m_tree_elem_cache);

        MDEBUG("Un-assigning leaf idx " << leaf_idx);
        registered_o.second.unassign_leaf();
    }

    // Check if there are any remaining layers that need to be removed
    // NOTE: this should only be useful for removing excess layers from registered outputs
    LayerIdx layer_idx = new_tree_edge.size();
    while (1)
    {
        auto cache_layer_it = m_tree_elem_cache.find(layer_idx);
        if (cache_layer_it == m_tree_elem_cache.end())
            break;

        MDEBUG("Removing cached layer " << layer_idx);
        m_tree_elem_cache.erase(cache_layer_it);
        ++layer_idx;
    }

    return true;
}

// Explicit instantiation
template bool TreeCache<Selene, Helios>::pop_block();
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeCache<C1, C2>::get_output_path(const OutputPair &output,
    typename CurveTrees<C1, C2>::Path &path_out) const
{
    path_out.clear();

    // Return false if the output isn't registered
    auto registered_output_it = m_registered_outputs.find(get_output_ref(output));
    if (registered_output_it == m_registered_outputs.end())
        return false;

    // Return empty path if the output is registered but isn't in the tree
    if (!registered_output_it->second.assigned_leaf_idx)
        return true;

    const uint64_t n_leaf_tuples = this->get_n_leaf_tuples();
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples > 0, "n_leaf_tuples must be >0 if leaf is already assigned");

    const LeafIdx leaf_idx = registered_output_it->second.leaf_idx;
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples > leaf_idx, "leaf_idx too high");

    return this->get_leaf_path(n_leaf_tuples, leaf_idx, path_out);
}

// Explicit instantiation
template bool TreeCache<Selene, Helios>::get_output_path(const OutputPair &output,
    CurveTrees<Selene, Helios>::Path &path_out) const;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::init(const uint64_t start_block_idx,
    const crypto::hash &start_block_hash,
    const uint64_t n_leaf_tuples,
    const fcmp_pp::curve_trees::PathBytes &last_path,
    const OutsByLastLockedBlock &timelocked_outputs)
{
    CHECK_AND_ASSERT_THROW_MES(m_cached_blocks.empty(), "expected empty tree cache");
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples >= last_path.leaves.size(), "n_leaf_tuples too small");

    fcmp_pp::curve_trees::BlockMeta init_block{
        .blk_idx       = start_block_idx,
        .blk_hash      = start_block_hash,
        .n_leaf_tuples = n_leaf_tuples,
    };

    m_cached_blocks.push_back(std::move(init_block));

    const uint64_t last_leaf_idx = n_leaf_tuples > 0 ? n_leaf_tuples - 1 : 0;
    const auto last_path_indexes = TreeSync<C1, C2>::m_curve_trees->get_path_indexes(n_leaf_tuples, last_leaf_idx);
    CHECK_AND_ASSERT_THROW_MES(last_path_indexes.layers.size() == last_path.layer_chunks.size(),
        "unexpected size of layer chunks");

    // {n_leaf_tuples, last_path.leaves} -> Leaves
    const uint64_t start_leaf_tuple_idx = n_leaf_tuples - last_path.leaves.size();
    CHECK_AND_ASSERT_THROW_MES(last_path_indexes.leaf_range.first == start_leaf_tuple_idx,
        "unexpected start leaf tuple idx");
    typename CurveTrees<C1, C2>::Leaves leaves{ start_leaf_tuple_idx, last_path.leaves };

    // {leaves, last_path.layer_chunks} -> TreeExtension
    // TODO: {const n_leaf_tuples, const &last_chunks} -> TreeExtension can be implemented in m_curve_trees
    typename CurveTrees<C1, C2>::TreeExtension tree_extension;
    tree_extension.leaves = std::move(leaves);
    bool parent_is_c1 = true;
    LayerIdx layer_idx = 0;
    for (const auto &child_chunk : last_path.layer_chunks)
    {
        // Get the start indexes and expected size of the last chunk
        const ChildChunkIdx start_idx = last_path_indexes.layers[layer_idx].first;
        const ChildChunkIdx end_idx = last_path_indexes.layers[layer_idx].second;
        CHECK_AND_ASSERT_THROW_MES(end_idx > start_idx, "unexpected end_idx <= start_idx");
        CHECK_AND_ASSERT_THROW_MES(child_chunk.chunk_bytes.size() == (end_idx - start_idx), "size mismatch last chunk");

        if (parent_is_c1)
        {
            LayerExtension<C1> layer_ext;
            layer_ext.start_idx = start_idx;
            layer_ext.update_existing_last_hash = false;
            for (const auto &child : child_chunk.chunk_bytes)
                layer_ext.hashes.emplace_back(TreeSync<C1, C2>::m_curve_trees->m_c1->from_bytes(child));
            tree_extension.c1_layer_extensions.emplace_back(std::move(layer_ext));
        }
        else
        {
            LayerExtension<C2> layer_ext;
            layer_ext.start_idx = start_idx;
            layer_ext.update_existing_last_hash = false;
            for (const auto &child : child_chunk.chunk_bytes)
                layer_ext.hashes.emplace_back(TreeSync<C1, C2>::m_curve_trees->m_c2->from_bytes(child));
            tree_extension.c2_layer_extensions.emplace_back(std::move(layer_ext));
        }

        ++layer_idx;
        parent_is_c1 = !parent_is_c1;
    }

    // Cache the last chunk of leaves, so if a registered output appears in the first chunk next block, we'll have
    // all prior leaves from that output's chunk already saved
    cache_last_chunk_leaves<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
        tree_extension.leaves,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        m_leaf_cache);

    // Cache the last chunk of hashes from every layer. We need to do this to handle:
    //   1) So we can use the tree's last hashes to grow the tree from here next block.
    //   2) In case a registered output appears in the first chunk next block, we'll have all its path elems cached.
    //   3) To trim the tree on reorg by re-growing with the children in each last chunk.
    cache_last_chunks<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
        tree_extension,
        start_leaf_tuple_idx,
        n_leaf_tuples,
        m_tree_elem_cache);

    // Add all timelocked outputs created before start_block_idx with last locked block >= start_block_idx so that we
    // grow the tree with those outputs correctly upon unlock.
    // - Assume the created block idx is the genesis block so the outputs won't get pruned.
    const CreatedBlockIdx created_block_idx{0};
    add_to_locked_outputs_cache(timelocked_outputs, created_block_idx, m_locked_outputs, m_locked_output_refs);

    // Set the output count to the max output id + 1
    // WARNING: this is a little hacky because if there are no timelocked outputs provided (which should never be the
    // case), then the output count would be 0 even if initializing at a block index > 0
    for (const auto &bl: timelocked_outputs)
        for (const auto &o : bl.second)
            if (o.output_id >= m_output_count)
                m_output_count = o.output_id + 1;
}

// Explicit instantiation
template void TreeCache<Selene, Helios>::init(const uint64_t start_block_idx,
    const crypto::hash &start_block_hash,
    const uint64_t n_leaf_tuples,
    const fcmp_pp::curve_trees::PathBytes &last_hashes,
    const OutsByLastLockedBlock &timelocked_outputs);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
uint64_t TreeCache<C1, C2>::get_n_leaf_tuples() const noexcept
{
    return m_cached_blocks.empty() ? 0 : m_cached_blocks.back().n_leaf_tuples;
}

// Explicit instantiation
template uint64_t TreeCache<Selene, Helios>::get_n_leaf_tuples() const noexcept;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::clear()
{
    m_locked_outputs.clear();
    m_locked_output_refs.clear();
    m_output_count = 0;
    m_registered_outputs.clear();
    m_leaf_cache.clear();
    m_tree_elem_cache.clear();
    m_cached_blocks.clear();
}
template void TreeCache<Selene, Helios>::clear();
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::LastHashes TreeCache<C1, C2>::get_last_hashes() const
{
    const uint64_t n_leaf_tuples = this->get_n_leaf_tuples();
    MTRACE("Getting last hashes on tree with " << n_leaf_tuples << " leaf tuples");

    typename CurveTrees<C1, C2>::LastHashes last_hashes;
    if (n_leaf_tuples == 0)
        return last_hashes;

    // Get the child chunk indexes of the last leaf for each layer
    const uint64_t last_leaf_idx = n_leaf_tuples - 1;
    const auto child_chunk_idxs = TreeSync<C1, C2>::m_curve_trees->get_child_chunk_indexes(n_leaf_tuples,
        last_leaf_idx);
    const std::size_t n_layers = TreeSync<C1, C2>::m_curve_trees->n_layers(n_leaf_tuples);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (n_layers + 1), "unexpected n child chunk idxs");

    // Read the last elem of each layer, starting at layer above leaf layer
    std::vector<crypto::ec_point> tree_edge;
    tree_edge.reserve(n_layers);
    for (LayerIdx i = 0; i < n_layers; ++i)
    {
        const auto child_chunk_it = read_child_chunk(i, child_chunk_idxs[i + 1], m_tree_elem_cache);
        tree_edge.push_back(child_chunk_it->second.tree_elems.back());
    }

    return TreeSync<C1, C2>::m_curve_trees->tree_edge_to_last_hashes(tree_edge);
}

// Explicit instantiation
template CurveTrees<Selene, Helios>::LastHashes TreeCache<Selene, Helios>::get_last_hashes() const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeCache<C1, C2>::deque_block(const uint64_t old_n_leaf_tuples)
{
    if (old_n_leaf_tuples == 0)
        return;

    // Remove ref to last chunk leaves from the cache
    const LeafIdx old_last_leaf_idx = old_n_leaf_tuples - 1;
    const ChildChunkIdx leaf_chunk_idx = old_last_leaf_idx / TreeSync<C1, C2>::m_curve_trees->m_c1_width;
    remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);

    // Remove refs to last chunk in every layer
    remove_path_chunks_refs(old_last_leaf_idx, TreeSync<C1, C2>::m_curve_trees, old_n_leaf_tuples, m_tree_elem_cache);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeCache<C1, C2>::get_leaf_path(const uint64_t n_leaf_tuples,
    const LeafIdx leaf_idx,
    typename CurveTrees<C1, C2>::Path &path_out) const
{
    path_out.clear();
    if (n_leaf_tuples == 0)
        return true;

    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples <= this->get_n_leaf_tuples(), "n_leaf_tuples is too high");
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples > leaf_idx, "leaf_idx too high");

    MTRACE("Getting path at leaf_idx: " << leaf_idx << " , tree has " << n_leaf_tuples << " leaf tuples");

    const auto path_indexes = TreeSync<C1, C2>::m_curve_trees->get_path_indexes(n_leaf_tuples, leaf_idx);
    const auto child_chunk_idxs = TreeSync<C1, C2>::m_curve_trees->get_child_chunk_indexes(n_leaf_tuples, leaf_idx);
    CHECK_AND_ASSERT_THROW_MES(!child_chunk_idxs.empty(), "empty child chunk indexes");
    CHECK_AND_ASSERT_THROW_MES(child_chunk_idxs.size() == (path_indexes.layers.size() + 1),
        "size mismatch path indexes <> child chunk indexes");

    // Collect cached leaves from the leaf chunk the leaf is in
    {
        const auto leaf_chunk_it = m_leaf_cache.find(child_chunk_idxs.front());
        CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != m_leaf_cache.end(), "missing cached leaf chunk");

        CHECK_AND_ASSERT_THROW_MES(path_indexes.leaf_range.second > path_indexes.leaf_range.first, "bad leaf range");
        const uint64_t n_leaves_in_chunk = path_indexes.leaf_range.second - path_indexes.leaf_range.first;
        CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it->second.leaves.size() >= n_leaves_in_chunk, "leaf chunk is too small");

        for (std::size_t i = 0; i < n_leaves_in_chunk; ++i)
            path_out.leaves.push_back(output_to_tuple(leaf_chunk_it->second.leaves[i]));
    }

    // Read all members of each chunk
    bool parent_is_c2 = true;
    for (LayerIdx i = 0; i < path_indexes.layers.size(); ++i)
    {
        const auto child_chunk_it = read_child_chunk(i, child_chunk_idxs[i + 1], m_tree_elem_cache);
        const auto &tree_elems = child_chunk_it->second.tree_elems;

        const auto &layer_range = path_indexes.layers[i];
        CHECK_AND_ASSERT_THROW_MES(layer_range.second > layer_range.first, "bad layer range");
        const uint64_t n_chunk_elems = layer_range.second - layer_range.first;
        CHECK_AND_ASSERT_THROW_MES(tree_elems.size() >= n_chunk_elems, "layer chunk is too small");

        if (parent_is_c2)
            path_out.c1_layers.emplace_back();
        else
            path_out.c2_layers.emplace_back();

        for (std::size_t i = 0; i < n_chunk_elems; ++i)
        {
            const auto &tree_elem = tree_elems[i];
            if (parent_is_c2)
                path_out.c1_layers.back().push_back(TreeSync<C1, C2>::m_curve_trees->m_c1->from_bytes(tree_elem));
            else
                path_out.c2_layers.back().push_back(TreeSync<C1, C2>::m_curve_trees->m_c2->from_bytes(tree_elem));
        }

        parent_is_c2 = !parent_is_c2;
    }

    return true;
}

// Explicit instantiation
template bool TreeCache<Selene, Helios>::get_leaf_path(const uint64_t n_leaf_tuples,
    const LeafIdx leaf_idx,
    CurveTrees<Selene, Helios>::Path &path_out) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<crypto::ec_point> TreeCache<C1, C2>::get_tree_edge(const uint64_t n_leaf_tuples) const
{
    std::vector<crypto::ec_point> tree_edge_out;
    if (n_leaf_tuples == 0)
        return tree_edge_out;

    typename CurveTrees<C1, C2>::Path last_path;
    CHECK_AND_ASSERT_THROW_MES(this->get_leaf_path(n_leaf_tuples, n_leaf_tuples - 1, last_path),
        "failed to get last leaf path");

    // Re-hash every layer in this last path, starting from leaves to get the
    // tree edge as it was when there were n_leaf_tuples in the chain
    return TreeSync<C1, C2>::m_curve_trees->calc_hashes_from_path(last_path, true/*replace_last_hash*/);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
