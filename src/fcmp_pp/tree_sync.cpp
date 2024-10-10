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

#include "tree_sync.h"

#include "misc_log_ex.h"
#include "string_tools.h"


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
static bool assign_new_output(const OutputPair &output_pair,
    const LeafIdx leaf_idx,
    RegisteredOutputs &registered_outputs_inout)
{
    const auto output_ref = get_output_ref(output_pair);

    auto registered_output_it = registered_outputs_inout.find(output_ref);
    if (registered_output_it == registered_outputs_inout.end())
        return false;

    // If it's already assigned a leaf idx, then it must be a duplicate and we only care about the earliest one
    // TODO: test this circumstance
    if (registered_output_it->second.assigned_leaf_idx)
        return false;

    MDEBUG("Starting to keep track of leaf_idx: " << leaf_idx);
    registered_output_it->second.assign_leaf(leaf_idx);

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_leaf_chunk(const ChildChunkIdx chunk_idx,
    const std::size_t leaf_parent_chunk_width,
    const typename CurveTrees<C1, C2>::Leaves &leaves,
    const bool bump_ref_count,
    LeafCache &leaf_cache_inout)
{
    const uint64_t n_leaf_tuples  = leaves.start_leaf_tuple_idx + leaves.tuples.size();

    const LeafIdx start_leaf_idx = chunk_idx * leaf_parent_chunk_width;
    const LeafIdx end_leaf_idx   = std::min(start_leaf_idx + leaf_parent_chunk_width, n_leaf_tuples);

    MDEBUG("Caching leaves at chunk_idx: " << chunk_idx
        << " , start_leaf_idx: "           << start_leaf_idx
        << " , end_leaf_idx: "             << end_leaf_idx
        << " , bump_ref_count: "           << bump_ref_count
        << " , start_leaf_tuple_idx: "     << leaves.start_leaf_tuple_idx);

    // If the leaf's chunk isn't present in this leaf extension, there are no new leaves we need to cache
    if (leaves.start_leaf_tuple_idx >= end_leaf_idx)
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

    // See what we expect the leaf chunk size to be
    CHECK_AND_ASSERT_THROW_MES(end_leaf_idx > start_leaf_idx, "start_leaf_idx is too high");
    const uint64_t expected_chunk_size = end_leaf_idx - start_leaf_idx;
    CHECK_AND_ASSERT_THROW_MES(expected_chunk_size >= cached_chunk_size, "cached leaf chunk is too big");
    const uint64_t new_leaves_needed = expected_chunk_size - cached_chunk_size;

    // If we already have all the leaves, we're done, we've already bumped the ref count if needed
    if (new_leaves_needed == 0)
        return;

    // Add the new leaves in the leaf's chunk to the cache
    const LeafIdx end_tuple_idx = end_leaf_idx - leaves.start_leaf_tuple_idx;
    CHECK_AND_ASSERT_THROW_MES(leaves.tuples.size() >= end_tuple_idx, "high end_tuple_idx");
    CHECK_AND_ASSERT_THROW_MES(end_tuple_idx >= new_leaves_needed, "low end_tuple_idx");
    const LeafIdx start_tuple_idx = end_tuple_idx - new_leaves_needed;

    std::vector<OutputPair> new_leaves;
    for (LeafIdx i = start_tuple_idx; i < end_tuple_idx; ++i)
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
static void remove_leaf_chunk_ref(const ChildChunkIdx chunk_idx, LeafCache &leaf_cache_inout)
{
    auto leaf_chunk_it = leaf_cache_inout.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != leaf_cache_inout.end(), "cache is missing leaf chunk");
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it->second.ref_count != 0, "leaf chunk has 0 ref count");

    leaf_chunk_it->second.ref_count -= 1;
    MDEBUG("Removing leaf chunk " << chunk_idx << " , updated ref count: " << leaf_chunk_it->second.ref_count);

    // If the ref count is 0, garbage collect it
    if (leaf_chunk_it->second.ref_count == 0)
        leaf_cache_inout.erase(leaf_chunk_it);
}
//----------------------------------------------------------------------------------------------------------------------
static void pop_leaves_from_cached_last_chunk(const uint64_t new_n_leaf_tuples,
    const std::size_t leaf_parent_chunk_width,
    LeafCache &leaf_cache_inout)
{
    // If the offset is 0, the last chunk is full and we're supposed to keep all elems in it
    const uint64_t offset = new_n_leaf_tuples % leaf_parent_chunk_width;
    if (offset == 0)
        return;

    const ChildChunkIdx chunk_idx = new_n_leaf_tuples / leaf_parent_chunk_width;
    auto leaf_chunk_it = leaf_cache_inout.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != leaf_cache_inout.end(), "cache is missing leaf chunk");

    // The last chunk should have at least offset leaves
    CHECK_AND_ASSERT_THROW_MES((uint64_t)leaf_chunk_it->second.leaves.size() >= offset,
        "unexpected n leaves in cached chunk");

    // Pop until the expected offset
    while ((uint64_t)leaf_chunk_it->second.leaves.size() > offset)
        leaf_chunk_it->second.leaves.pop_back();
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void cache_path_elem(const std::unique_ptr<C> &curve,
    const std::size_t child_width,
    const std::size_t parent_width,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const bool newly_registered_output,
    ChildChunkIdx &start_child_chunk_idx_inout,
    ChildChunkIdx &end_child_chunk_idx_inout,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "high layer_ext_idx");
    auto &layer_ext = layer_exts[layer_ext_idx];

    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "empty layer ext");
    const uint64_t n_layer_elems = layer_ext.start_idx + layer_ext.hashes.size();

    // TODO: clean this up following cache_last_chunk approach
    end_child_chunk_idx_inout = std::min(end_child_chunk_idx_inout, n_layer_elems);

    MDEBUG("Caching path elems from start_child_chunk_idx: " << start_child_chunk_idx_inout << " to end_child_chunk_idx: " << end_child_chunk_idx_inout);

    // Collect the path elems in the tree extension
    for (ChildChunkIdx child_chunk_idx = start_child_chunk_idx_inout; child_chunk_idx < end_child_chunk_idx_inout; ++child_chunk_idx)
    {
        // TODO: separate function
        auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
        if (child_chunk_idx < layer_ext.start_idx)
        {
            // We expect we already have the tree elem cached, since it should be part of the last chunk
            CHECK_AND_ASSERT_THROW_MES(cached_layer_it != cached_tree_elems_inout.end(), "missing layer from last chunk");
            auto cached_tree_elem_it = cached_layer_it->second.find(child_chunk_idx);
            CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing tree elem from last chunk");

            // We only bump the ref count for tree elems not in this tree extension if we're caching path elems for a
            // newly registered output. This tells the cache to keep the elem cached, don't prune it.
            if (newly_registered_output)
                cached_tree_elem_it->second.ref_count += 1;

            continue;
        }

        CHECK_AND_ASSERT_THROW_MES(child_chunk_idx >= layer_ext.start_idx, "low child_chunk_Idx");
        const ChildChunkIdx ext_hash_idx = child_chunk_idx - layer_ext.start_idx;

        // Check if the layer exists
        if (cached_layer_it == cached_tree_elems_inout.end())
        {
            cached_tree_elems_inout[layer_idx] = {{ child_chunk_idx, CachedTreeElem{
                    .tree_elem = curve->to_bytes(layer_ext.hashes[ext_hash_idx]),
                    .ref_count = 1,
                }}};
            continue;
        }

        // Check if we're keeping track of this tree elem already
        auto cached_tree_elem_it = cached_layer_it->second.find(child_chunk_idx);
        if (cached_tree_elem_it == cached_layer_it->second.end())
        {
            cached_tree_elems_inout[layer_idx][child_chunk_idx] = CachedTreeElem{
                    .tree_elem = curve->to_bytes(layer_ext.hashes[ext_hash_idx]),
                    .ref_count = 1,
                };
            continue;
        }

        // We only need to bump the ref count for *new* path elems in this tree extension, or for elems in the
        // path of a newly registered output. Otherwise we're duplicating refs to an output's path elems that won't get
        // purged.
        // TODO: when implementing reorg, see how this logic can be simplified
        const bool updating_existing_last_hash = ext_hash_idx == 0 && layer_ext.update_existing_last_hash;
        if (newly_registered_output || !updating_existing_last_hash)
            cached_tree_elem_it->second.ref_count += 1;
    }

    start_child_chunk_idx_inout /= parent_width;
    start_child_chunk_idx_inout = start_child_chunk_idx_inout - (start_child_chunk_idx_inout % child_width);
    end_child_chunk_idx_inout = start_child_chunk_idx_inout + child_width;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_registered_path_chunks(const LeafIdx leaf_idx,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const std::vector<LayerExtension<C1>> &c1_layer_exts,
    const std::vector<LayerExtension<C2>> &c2_layer_exts,
    const bool newly_assigned_output,
    TreeElemCache &tree_elem_cache_inout)
{
    const ChildChunkIdx child_chunk_idx = leaf_idx / curve_trees->m_c2_width;
    ChildChunkIdx start_child_chunk_idx = child_chunk_idx - (child_chunk_idx % curve_trees->m_c1_width);
    ChildChunkIdx end_child_chunk_idx = start_child_chunk_idx + curve_trees->m_c1_width;

    std::size_t c1_idx = 0, c2_idx = 0;
    bool parent_is_c1 = true;
    const std::size_t n_layers = c1_layer_exts.size() + c2_layer_exts.size();
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        MDEBUG("Caching tree elems from layer_idx " << layer_idx);
        if (parent_is_c1)
        {
            cache_path_elem(curve_trees->m_c2,
                    curve_trees->m_c2_width,
                    curve_trees->m_c1_width,
                    c2_layer_exts,
                    c2_idx,
                    layer_idx,
                    newly_assigned_output,
                    start_child_chunk_idx,
                    end_child_chunk_idx,
                    tree_elem_cache_inout
                );
            ++c2_idx;
        }
        else
        {
            cache_path_elem(curve_trees->m_c1,
                    curve_trees->m_c1_width,
                    curve_trees->m_c2_width,
                    c1_layer_exts,
                    c1_idx,
                    layer_idx,
                    newly_assigned_output,
                    start_child_chunk_idx,
                    end_child_chunk_idx,
                    tree_elem_cache_inout
                );
            ++c1_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_last_chunk_leaves(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const typename CurveTrees<C1, C2>::Leaves &leaves,
    LeafCache &leaf_cache_inout)
{
    const uint64_t n_leaf_tuples = leaves.start_leaf_tuple_idx + leaves.tuples.size();
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx last_leaf_idx = n_leaf_tuples - 1;
    const ChildChunkIdx chunk_idx = last_leaf_idx / curve_trees->m_c2_width;

    // Always bump the ref count for last chunk of leaves so that it sticks around until pruned
    const bool bump_ref_count = true;

    cache_leaf_chunk<C1, C2>(chunk_idx,
        curve_trees->m_c2_width,
        leaves,
        bump_ref_count,
        leaf_cache_inout);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void cache_last_chunk(const std::unique_ptr<C> &curve,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const std::size_t parent_width,
    TreeElemCache &cached_tree_elems_inout,
    ChildChunkIdxSet &prunable_child_chunks_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "unexpected high layer_ext_idx");

    const auto &layer_ext = layer_exts[layer_ext_idx];
    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "unexpected empty layer ext");

    // First, update the existing last hash if it updated
    if (layer_ext.update_existing_last_hash)
    {
        auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != cached_tree_elems_inout.end(), "missing layer");

        auto cached_tree_elem_it = cached_layer_it->second.find(layer_ext.start_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing cached elem");

        auto tree_elem = curve->to_bytes(layer_ext.hashes.front());
        cached_tree_elem_it->second.tree_elem = std::move(tree_elem);
    }

    // Second, cache the new last chunk values
    const ChildChunkIdx end_child_chunk_idx = layer_ext.start_idx + layer_ext.hashes.size();

    const ChildChunkIdx offset = end_child_chunk_idx % parent_width;
    const ChildChunkIdx end_offset = (offset > 0) ? offset : parent_width;
    CHECK_AND_ASSERT_THROW_MES(end_child_chunk_idx >= end_offset, "high end_offset");

    const ChildChunkIdx start_child_chunk_idx = end_child_chunk_idx - end_offset;

    MDEBUG("Caching start_child_chunk_idx " << start_child_chunk_idx << " to end_child_chunk_idx " << end_child_chunk_idx
        << " (layer start idx " << layer_ext.start_idx << " , parent_width " << parent_width << " , end_offset " << end_offset << ")");

    // TODO: this code is *mostly* duplicated above with subtle diffs
    for (ChildChunkIdx child_chunk_idx = start_child_chunk_idx; child_chunk_idx < end_child_chunk_idx; ++child_chunk_idx)
    {
        prunable_child_chunks_inout.insert(child_chunk_idx);

        auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
        if (child_chunk_idx < layer_ext.start_idx)
        {
            // We expect we already have the tree elem cached, since it should be part of the last chunk
            CHECK_AND_ASSERT_THROW_MES(cached_layer_it != cached_tree_elems_inout.end(), "missing layer from last chunk");
            auto cached_tree_elem_it = cached_layer_it->second.find(child_chunk_idx);
            CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing tree elem from last chunk");

            cached_tree_elem_it->second.ref_count += 1;
            continue;
        }

        // TODO: separate function
        CHECK_AND_ASSERT_THROW_MES(child_chunk_idx >= layer_ext.start_idx, "low child_chunk_Idx");
        const ChildChunkIdx ext_hash_idx = child_chunk_idx - layer_ext.start_idx;

        if (cached_layer_it == cached_tree_elems_inout.end())
        {
            cached_tree_elems_inout[layer_idx] = {{ child_chunk_idx, CachedTreeElem {
                    .tree_elem = curve->to_bytes(layer_ext.hashes[ext_hash_idx]),
                    .ref_count = 1
                }}};
            continue;
        }

        auto cached_tree_elem_it = cached_layer_it->second.find(child_chunk_idx);
        if (cached_tree_elem_it == cached_layer_it->second.end())
        {
            cached_tree_elems_inout[layer_idx][child_chunk_idx] = CachedTreeElem{
                    .tree_elem = curve->to_bytes(layer_ext.hashes[ext_hash_idx]),
                    .ref_count = 1,
                };
            continue;
        }

        // We're already keeping track of this elem, so bump the ref count
        cached_tree_elem_it->second.ref_count += 1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static PrunableChunks cache_last_chunks_from_every_layer(
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const std::vector<LayerExtension<C1>> &c1_layer_exts,
    const std::vector<LayerExtension<C2>> &c2_layer_exts,
    TreeElemCache &cached_tree_elems_inout)
{
    PrunableChunks prunable_tree_elems_out;

    bool use_c2 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    const std::size_t n_layers = c1_layer_exts.size() + c2_layer_exts.size();
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        MDEBUG("Caching the last chunk from layer " << layer_idx+1 << " / " << n_layers);
        ChildChunkIdxSet prunable_child_chunks;
        if (use_c2)
        {
            cache_last_chunk(
                curve_trees->m_c2,
                c2_layer_exts,
                c2_idx,
                layer_idx,
                curve_trees->m_c1_width,
                cached_tree_elems_inout,
                prunable_child_chunks);

            ++c2_idx;
        }
        else
        {
            cache_last_chunk(
                curve_trees->m_c1,
                c1_layer_exts,
                c1_idx,
                layer_idx,
                curve_trees->m_c2_width,
                cached_tree_elems_inout,
                prunable_child_chunks);

            ++c1_idx;
        }

        prunable_tree_elems_out[layer_idx] = std::move(prunable_child_chunks);
        use_c2 = !use_c2;
    }

    return prunable_tree_elems_out;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_CHILD, typename C_PARENT>
static std::vector<typename C_PARENT::Scalar> get_layer_last_chunk_children_to_regrow(
    const std::unique_ptr<C_CHILD> &c_child,
    const ChildChunkCache &child_chunk_cache,
    const ChildChunkIdx start_idx,
    const ChildChunkIdx end_idx)
{
    std::vector<typename C_PARENT::Scalar> children_to_regrow_out;
    if (end_idx > start_idx)
    {
        ChildChunkIdx idx = start_idx;
        do
        {
            const auto cached_chunk_it = child_chunk_cache.find(idx);
            CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != child_chunk_cache.end(), "missing child chunk for regrow");

            auto child_point = c_child->from_bytes(cached_chunk_it->second.tree_elem);
            auto child_scalar = c_child->point_to_cycle_scalar(child_point);

            MDEBUG("Re-growing with child chunk idx: " << idx << " , elem: " << c_child->to_string(child_point));

            children_to_regrow_out.push_back(std::move(child_scalar));

            ++idx;
        }
        while (idx < end_idx);
    }

    return children_to_regrow_out;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSync<C1, C2>::register_output(const OutputPair &output, const uint64_t unlock_block_idx)
{
    if (!m_cached_blocks.empty())
    {
        const auto &top_synced_block = m_cached_blocks.back();

        // If the output is already unlocked, we won't be able to tell the output's position in the tree
        CHECK_AND_ASSERT_THROW_MES(unlock_block_idx > top_synced_block.blk_idx,
            "already synced block in which output unlocked");
    }

    auto output_ref = get_output_ref(output);

    // Return false if already registered
    if (m_registered_outputs.find(output_ref) != m_registered_outputs.end())
        return false;

    // Add to registered outputs container
    m_registered_outputs.insert({ std::move(output_ref), AssignedLeafIdx{} });

    return true;
}

// Explicit instantiation
template bool TreeSync<Helios, Selene>::register_output(const OutputPair &output, const uint64_t unlock_block_idx);
//----------------------------------------------------------------------------------------------------------------------
// TODO: change all code to be more precise: I should know exactly which tree elems I need. Don't go by what's stored
template<typename C1, typename C2>
void TreeSync<C1, C2>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    std::vector<OutputContext> &&new_leaf_tuples)
{
    // Pre-checks
    std::size_t n_leaf_tuples = 0;
    if (m_cached_blocks.empty())
    {
        // TODO: if block_idx > 0, we need the tree's last chunk elems and old_n_leaf_tuples
        CHECK_AND_ASSERT_THROW_MES(block_idx == 0, "syncing first block_idx > 0 not yet implemented");

        // Make sure all blockchain containers are empty
        CHECK_AND_ASSERT_THROW_MES(m_cached_blocks.empty(),   "expected empty cached blocks");
        CHECK_AND_ASSERT_THROW_MES(m_leaf_cache.empty(),      "expected empty cached leaves");
        CHECK_AND_ASSERT_THROW_MES(m_tree_elem_cache.empty(), "expected empty cached tree elems");
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(block_idx > 0, "expected block_idx > 0");

        // Make sure provided block is contiguous to prior synced block
        const auto &prev_block = m_cached_blocks.back();
        CHECK_AND_ASSERT_THROW_MES(prev_block.blk_idx == (block_idx - 1), "failed contiguity idx check");
        CHECK_AND_ASSERT_THROW_MES(prev_block.blk_hash == prev_block_hash, "failed contiguity hash check");

        n_leaf_tuples = prev_block.n_leaf_tuples;
    }

    // Get the tree extension using existing tree data. We'll use the tree extension to update registered output paths
    // in the tree and cache the data necessary to build the next block's tree extension.
    auto tree_extension = m_curve_trees->get_tree_extension(
        n_leaf_tuples,
        this->get_last_hashes(n_leaf_tuples),
        std::move(new_leaf_tuples));

    // Check if any registered outputs are present in the tree extension. If so, we assign the output its leaf idx and
    // start keeping track of the output's path elems
    std::unordered_set<LeafIdx> new_assigned_outputs;
    for (uint64_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &output_pair = tree_extension.leaves.tuples[i].output_pair;
        const LeafIdx leaf_idx = tree_extension.leaves.start_leaf_tuple_idx + i;

        if (assign_new_output(output_pair, leaf_idx, m_registered_outputs))
            new_assigned_outputs.insert(leaf_idx);
    }

    // Cache tree elems from the tree extension needed in order to keep track of registered output paths in the tree
    const auto &c1_layer_exts = tree_extension.c1_layer_extensions;
    const auto &c2_layer_exts = tree_extension.c2_layer_extensions;
    for (const auto &registered_o : m_registered_outputs)
    {
        // Skip all registered outputs which have not been included in the tree yet
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        const LeafIdx leaf_idx = registered_o.second.leaf_idx;

        // We only need to bump the ref count on this registered output's leaf chunk if it was just included in the tree
        const bool bump_ref_count = new_assigned_outputs.find(leaf_idx) != new_assigned_outputs.end();

        // Cache registered leaf's chunk
        cache_leaf_chunk<C1, C2>(leaf_idx / m_curve_trees->m_c2_width,
            m_curve_trees->m_c2_width,
            tree_extension.leaves,
            bump_ref_count,
            m_leaf_cache);

        // Now cache the rest of the path elems for each registered output
        cache_registered_path_chunks<C1, C2>(leaf_idx,
            m_curve_trees,
            c1_layer_exts,
            c2_layer_exts,
            bump_ref_count,
            m_tree_elem_cache);
    }

    // Cache the last chunk of leaves, so if a registered output appears in the first chunk next block, we'll have all
    // prior leaves from that output's chunk
    cache_last_chunk_leaves<C1, C2>(m_curve_trees, tree_extension.leaves, m_leaf_cache);

    // Cache the last chunk of hashes from every layer. We need to do this to handle all of the following:
    //   1) So we can use the tree's last hashes to grow the tree from here next block.
    //   2) In case a registered output appears in the first chunk next block, we'll have all its path elems cached.
    //   3) To trim the tree on reorg using the last children from each chunk
    auto prunable_tree_elems = cache_last_chunks_from_every_layer<C1, C2>(m_curve_trees,
        c1_layer_exts,
        c2_layer_exts,
        m_tree_elem_cache);
    m_prunable_tree_elems_by_block[block_hash] = std::move(prunable_tree_elems);

    // Update cached blocks
    const uint64_t new_total_n_leaf_tuples = n_leaf_tuples + tree_extension.leaves.tuples.size();
    auto blk_meta = BlockMeta {
            .blk_idx = block_idx,
            .blk_hash = block_hash,
            .n_leaf_tuples = new_total_n_leaf_tuples,
        };
    m_cached_blocks.push_back(std::move(blk_meta));

    // Deque the oldest cached block
    // TODO: separate function
    if (m_cached_blocks.size() > m_max_reorg_depth)
    {
        CHECK_AND_ASSERT_THROW_MES(!m_cached_blocks.empty(), "empty cached blocks");
        const BlockMeta &oldest_block = m_cached_blocks.front();

        this->deque_block(oldest_block.blk_hash, oldest_block.n_leaf_tuples);

        // Prune the block
        m_cached_blocks.pop_front();

        // Keep in mind: the registered output path should remain untouched, chain state isn't changing. We're only
        // purging refs to last chunks from the cache.
    }

    CHECK_AND_ASSERT_THROW_MES(m_max_reorg_depth >= m_cached_blocks.size(), "cached blocks exceeded max reorg depth");
}

// Explicit instantiation
template void TreeSync<Helios, Selene>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    std::vector<OutputContext> &&new_leaf_tuples);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSync<C1, C2>::pop_block()
{
    if (m_cached_blocks.empty())
        return false;

    // Pop the top block off the back of the cache
    const uint64_t  old_n_leaf_tuples = m_cached_blocks.back().n_leaf_tuples;
    const BlockHash old_block_hash    = m_cached_blocks.back().blk_hash;
    this->deque_block(old_block_hash, old_n_leaf_tuples);
    m_cached_blocks.pop_back();

    // Determine how many leaves we need to trim
    uint64_t new_n_leaf_tuples = 0;
    if (!m_cached_blocks.empty())
    {
        // Trim to the new top block
        const BlockMeta &new_top_block = m_cached_blocks.back();
        new_n_leaf_tuples = new_top_block.n_leaf_tuples;
    }
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples >= new_n_leaf_tuples, "expected old_n_leaf_tuples >= new_n_leaf_tuples");
    const uint64_t trim_n_leaf_tuples = old_n_leaf_tuples - new_n_leaf_tuples;

    // We're going to trim the tree as the node would to see exactly how the tree elems we know about need to change.
    // First get trim instructions
    const auto trim_instructions = m_curve_trees->get_trim_instructions(old_n_leaf_tuples,
        trim_n_leaf_tuples,
        true/*always_regrow_with_remaining*/);
    MDEBUG("Acquired trim instructions for " << trim_instructions.size() << " layers");

    // Do initial tree reads using trim instructions
    const auto last_chunk_children_to_regrow = this->get_last_chunk_children_to_regrow(trim_instructions);
    const auto last_hashes_to_trim = this->get_last_hashes_to_trim(trim_instructions);

    // Get the new hashes, wrapped in a simple struct we can use to trim the tree
    const auto tree_reduction = m_curve_trees->get_tree_reduction(
        trim_instructions,
        last_chunk_children_to_regrow,
        last_hashes_to_trim);

    const auto &c1_layer_reductions = tree_reduction.c1_layer_reductions;
    const auto &c2_layer_reductions = tree_reduction.c2_layer_reductions;
    const std::size_t new_n_layers = c2_layer_reductions.size() + c1_layer_reductions.size();

    // Pop leaves from the current last chunk
    // NOTE: deque_block above already removed refs to any last chunk leaves that weren't tied to registered outputs,
    // so we don't need to remove refs here.
    pop_leaves_from_cached_last_chunk(new_n_leaf_tuples, m_curve_trees->m_c2_width, m_leaf_cache);

    // Use the tree reduction to update ref'd last hashes
    // NOTE: deque_block above already removed refs to any last chunk hashes and last chunk leaves that weren't tied
    // to registered outputs, so we don't need to remove anything here.
    // TODO: separate function
    {
        bool use_c2 = true;
        std::size_t c2_idx = 0;
        std::size_t c1_idx = 0;
        for (LayerIdx i = 0; i < new_n_layers; ++i)
        {
            auto cached_layer_it = m_tree_elem_cache.find(i);
            CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing cached layer");

            // TODO: templated function
            if (use_c2)
            {
                CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layer_reductions.size(), "unexpected c2 layer reduction");
                const auto &c2_reduction = c2_layer_reductions[c2_idx];

                // We updated the last hash
                if (c2_reduction.update_existing_last_hash)
                {
                    MDEBUG("layer: " << i << " updating last hash to " << m_curve_trees->m_c2->to_string(c2_reduction.new_last_hash));

                    CHECK_AND_ASSERT_THROW_MES(c2_reduction.new_total_parents > 0, "unexpected 0 new_total_parents");
                    auto cached_tree_elem_it = cached_layer_it->second.find(c2_reduction.new_total_parents - 1);
                    CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing cached new last hash");

                    cached_tree_elem_it->second.tree_elem = m_curve_trees->m_c2->to_bytes(c2_reduction.new_last_hash);
                }

                ++c2_idx;
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layer_reductions.size(), "unexpected c1 layer reduction");
                const auto &c1_reduction = c1_layer_reductions[c1_idx];

                // We updated the last hash
                if (c1_reduction.update_existing_last_hash)
                {
                    MDEBUG("layer: " << i << " updating last hash to " << m_curve_trees->m_c1->to_string(c1_reduction.new_last_hash));

                    CHECK_AND_ASSERT_THROW_MES(c1_reduction.new_total_parents > 0, "unexpected 0 new_total_parents");
                    auto cached_tree_elem_it = cached_layer_it->second.find(c1_reduction.new_total_parents - 1);
                    CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing cached new last hash");

                    cached_tree_elem_it->second.tree_elem = m_curve_trees->m_c1->to_bytes(c1_reduction.new_last_hash);
                }

                ++c1_idx;
            }

            use_c2 = !use_c2;
        }
    }

    // Use the tree reduction to update registered output paths
    for (auto &registered_o : m_registered_outputs)
    {
        // If the output isn't in the tree, it has no path elems we need to change in the cache 
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        // TODO: below should all be a separate function
        // Get the output's cached path indexes in the tree
        const LeafIdx leaf_idx = registered_o.second.leaf_idx;
        const auto old_path_idxs = m_curve_trees->get_path_indexes(old_n_leaf_tuples, leaf_idx);

        const bool output_removed_from_tree = leaf_idx >= tree_reduction.new_total_leaf_tuples;

        // Use the tree reduction to update the cached leaves and path elems
        // First, remove ref to the leaf's chunk if we're removing the leaf from the tree
        if (output_removed_from_tree)
        {
            const ChildChunkIdx leaf_chunk_idx = leaf_idx / m_curve_trees->m_c2_width;
            remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);
        }

        // Second, remove or update any cached path elems if necessary
        bool use_c2 = true;
        std::size_t c2_idx = 0;
        std::size_t c1_idx = 0;
        for (LayerIdx i = 0; i < new_n_layers; ++i)
        {
            auto cached_layer_it = m_tree_elem_cache.find(i);
            CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing cached layer");

            uint64_t new_total_parents = 0;
            uint64_t old_chunk_start = 0;
            uint64_t old_chunk_end = 0;
            // TODO: templated function
            if (use_c2)
            {
                CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layer_reductions.size(), "unexpected c2 layer reduction");
                const auto &c2_reduction = c2_layer_reductions[c2_idx];

                new_total_parents = c2_reduction.new_total_parents;

                CHECK_AND_ASSERT_THROW_MES(old_path_idxs.c2_layers.size() > c2_idx, "unexpected c2 path idxs");
                old_chunk_start = old_path_idxs.c2_layers[c2_idx].first;
                old_chunk_end = old_path_idxs.c2_layers[c2_idx].second;

                ++c2_idx;
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layer_reductions.size(), "unexpected c1 layer reduction");
                const auto &c1_reduction = c1_layer_reductions[c1_idx];

                new_total_parents = c1_reduction.new_total_parents;

                CHECK_AND_ASSERT_THROW_MES(old_path_idxs.c1_layers.size() > c1_idx, "unexpected c1 path idxs");
                old_chunk_start = old_path_idxs.c1_layers[c1_idx].first;
                old_chunk_end = old_path_idxs.c1_layers[c1_idx].second;

                ++c1_idx;
            }

            MDEBUG("layer: " << i
                << " , old_chunk_start: "   << old_chunk_start
                << " , old_chunk_end: "     << old_chunk_end
                << " , new_total_parents: " << new_total_parents);

            // Remove cached elems if necessary
            if (output_removed_from_tree || old_chunk_end > new_total_parents)
            {
                // Remove refs to stale path elems from the cache
                const ChildChunkIdx start_idx = output_removed_from_tree ? old_chunk_start : new_total_parents;
                const ChildChunkIdx end_idx = old_chunk_end;

                MDEBUG("Removing in layer " << i << ": start_idx: " << start_idx << " , end_idx: " << end_idx);

                // TODO: separate static function, duplicated above
                for (ChildChunkIdx j = start_idx; j < end_idx; ++j)
                {
                    auto cached_tree_elem_it = cached_layer_it->second.find(j);
                    CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing cached tree elem");
                    CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it->second.ref_count != 0, "cached elem has 0 ref count");

                    cached_tree_elem_it->second.ref_count -= 1;

                    // If the ref count is 0, garbage collect it
                    if (cached_tree_elem_it->second.ref_count == 0)
                        cached_layer_it->second.erase(cached_tree_elem_it);
                }

                if (cached_layer_it->second.empty())
                    m_tree_elem_cache.erase(cached_layer_it);
            }

            use_c2 = !use_c2;
        }

        if (output_removed_from_tree)
        {
            MDEBUG("Un-assigning leaf idx " << leaf_idx);
            registered_o.second.unassign_leaf();
        }
    }

    // Check if there are any remaining layers that need to be removed
    // NOTE: this should only be useful for removing excess layers from registered outputs
    // TODO: de-dup this code
    LayerIdx layer_idx = new_n_layers;
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
template bool TreeSync<Helios, Selene>::pop_block();
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSync<C1, C2>::get_output_path(const OutputPair &output, typename CurveTrees<C1, C2>::Path &path_out) const
{
    // TODO: use n leaf tuples + leaf idx in tree to know exactly which elems from the tree we expect

    path_out.clear();

    // Return false if the output isn't registered
    auto registered_output_it = m_registered_outputs.find(get_output_ref(output));
    if (registered_output_it == m_registered_outputs.end())
        return false;

    // Return empty path if the output is registered but isn't in the tree
    if (!registered_output_it->second.assigned_leaf_idx)
        return true;

    const LeafIdx leaf_idx = registered_output_it->second.leaf_idx;
    ChildChunkIdx child_chunk_idx = leaf_idx / m_curve_trees->m_c2_width;
    const LeafIdx start_leaf_idx = child_chunk_idx * m_curve_trees->m_c2_width;

    MDEBUG("Getting output path at leaf_idx: " << leaf_idx << " , start_leaf_idx: " << start_leaf_idx);

    // Collect cached leaves from the leaf chunk this leaf is in
    const auto leaf_chunk_it = m_leaf_cache.find(child_chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != m_leaf_cache.end(), "missing cached leaf chunk");
    for (const auto &leaf : leaf_chunk_it->second.leaves)
        path_out.leaves.push_back(output_to_tuple(leaf));

    CHECK_AND_ASSERT_THROW_MES((start_leaf_idx + path_out.leaves.size()) > leaf_idx, "leaves path missing leaf_idx");

    // Collect cached tree elems in the leaf's path
    LayerIdx layer_idx = 0;
    child_chunk_idx /= m_curve_trees->m_c1_width;
    ChildChunkIdx start_child_chunk_idx = child_chunk_idx * m_curve_trees->m_c1_width;
    ChildChunkIdx end_child_chunk_idx = start_child_chunk_idx + m_curve_trees->m_c1_width;
    bool parent_is_c1 = true;
    while (1)
    {
        auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        if (cached_layer_it == m_tree_elem_cache.end())
            break;

        MDEBUG("Getting output path at layer_idx " << layer_idx             << ", " <<
            "child_chunk_idx "                     << child_chunk_idx       << ", " <<
            "start_child_chunk_idx "               << start_child_chunk_idx << ", " <<
            "end_child_chunk_idx "                 << end_child_chunk_idx);

        if (parent_is_c1)
            path_out.c2_layers.emplace_back();
        else
            path_out.c1_layers.emplace_back();

        for (ChildChunkIdx i = start_child_chunk_idx; i < end_child_chunk_idx; ++i)
        {
            const auto cached_tree_elem_it = cached_layer_it->second.find(i);
            if (cached_tree_elem_it == cached_layer_it->second.end())
            {
                CHECK_AND_ASSERT_THROW_MES(i > start_child_chunk_idx, "missing cached tree elem");
                break;
            }

            auto &tree_elem = cached_tree_elem_it->second.tree_elem;
            MDEBUG("Found child chunk idx: " << i << " elem: " << epee::string_tools::pod_to_hex(tree_elem));
            if (parent_is_c1)
            {
                path_out.c2_layers.back().push_back(m_curve_trees->m_c2->from_bytes(tree_elem));
            }
            else
            {
                path_out.c1_layers.back().push_back(m_curve_trees->m_c1->from_bytes(tree_elem));
            }
        }

        parent_is_c1 = !parent_is_c1;
        const std::size_t width = parent_is_c1 ? m_curve_trees->m_c1_width : m_curve_trees->m_c2_width;

        child_chunk_idx /= width;
        start_child_chunk_idx = child_chunk_idx * width;
        end_child_chunk_idx = start_child_chunk_idx + width;

        ++layer_idx;
    }

    return true;
}

// Explicit instantiation
template bool TreeSync<Helios, Selene>::get_output_path(const OutputPair &output,
    CurveTrees<Helios, Selene>::Path &path_out) const;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::LastHashes TreeSync<C1, C2>::get_last_hashes(const std::size_t n_leaf_tuples) const
{
    MDEBUG("Getting last hashes on tree with " << n_leaf_tuples << " leaf tuples");

    typename CurveTrees<C1, C2>::LastHashes last_hashes;
    if (n_leaf_tuples == 0)
        return last_hashes;

    std::size_t n_children = n_leaf_tuples;
    bool use_c2 = true;
    LayerIdx layer_idx = 0;
    do
    {
        const std::size_t width = use_c2 ? m_curve_trees->m_c2_width : m_curve_trees->m_c1_width;
        const ChildChunkIdx last_child_chunk_idx = (n_children - 1) / width;

        MDEBUG("Getting last hash at layer_idx " << layer_idx << " and last_child_chunk_idx " << last_child_chunk_idx);

        auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing cached last hash layer");

        auto cached_tree_elem_it = cached_layer_it->second.find(last_child_chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_tree_elem_it != cached_layer_it->second.end(), "missing cached last hash");

        const auto &tree_elem = cached_tree_elem_it->second.tree_elem;
        if (use_c2)
            last_hashes.c2_last_hashes.push_back(m_curve_trees->m_c2->from_bytes(tree_elem));
        else
            last_hashes.c1_last_hashes.push_back(m_curve_trees->m_c1->from_bytes(tree_elem));

        ++layer_idx;
        n_children = last_child_chunk_idx + 1;
        use_c2 = !use_c2;
    }
    while (n_children > 1);

    return last_hashes;
}

// Explicit instantiation
template CurveTrees<Helios, Selene>::LastHashes TreeSync<Helios, Selene>::get_last_hashes(
    const std::size_t n_leaf_tuples) const;
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LastChunkChildrenToTrim TreeSync<Helios, Selene>::get_last_chunk_children_to_regrow(
    const std::vector<TrimLayerInstructions> &trim_instructions) const
{
    CurveTrees<Helios, Selene>::LastChunkChildrenToTrim all_children_to_regrow;

    if (trim_instructions.empty())
        return all_children_to_regrow;

    // Leaf layer
    const auto &trim_leaf_layer_instructions = trim_instructions[0];
    std::vector<Selene::Scalar> leaves_to_regrow;
    const std::size_t LEAF_TUPLE_SIZE = CurveTrees<Helios, Selene>::LEAF_TUPLE_SIZE;
    // TODO: separate function
    if (trim_leaf_layer_instructions.end_trim_idx > trim_leaf_layer_instructions.start_trim_idx)
    {
        LeafIdx idx = trim_leaf_layer_instructions.start_trim_idx;
        CHECK_AND_ASSERT_THROW_MES(idx % LEAF_TUPLE_SIZE == 0, "expected divisble by leaf tuple size");
        const ChildChunkIdx chunk_idx = idx / m_curve_trees->m_leaf_layer_chunk_width;

        const auto leaf_chunk_it = m_leaf_cache.find(chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != m_leaf_cache.end(), "missing cached leaf chunk");
        auto leaf_it = leaf_chunk_it->second.leaves.begin();

        do
        {
            const LeafIdx leaf_idx = idx / LEAF_TUPLE_SIZE;
            MDEBUG("Re-growing with leaf idx " << leaf_idx);
            CHECK_AND_ASSERT_THROW_MES(leaf_it != leaf_chunk_it->second.leaves.end(), "missing cached leaf");

            const auto leaf_tuple = m_curve_trees->leaf_tuple(*leaf_it);

            leaves_to_regrow.push_back(leaf_tuple.O_x);
            leaves_to_regrow.push_back(leaf_tuple.I_x);
            leaves_to_regrow.push_back(leaf_tuple.C_x);

            idx += LEAF_TUPLE_SIZE;
            ++leaf_it;
        }
        while (idx < trim_leaf_layer_instructions.end_trim_idx);
    }

    all_children_to_regrow.c2_children.emplace_back(std::move(leaves_to_regrow));

    bool parent_is_c2 = false;
    for (std::size_t i = 1; i < trim_instructions.size(); ++i)
    {
        MDEBUG("Getting last chunk children to re-grow layer " << i);

        const auto &trim_layer_instructions = trim_instructions[i];

        const ChildChunkIdx start_idx = trim_layer_instructions.start_trim_idx;
        const ChildChunkIdx end_idx   = trim_layer_instructions.end_trim_idx;

        const LayerIdx layer_idx = i - 1;
        const auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing layer for trim");

        if (parent_is_c2)
        {
            auto children_to_regrow = get_layer_last_chunk_children_to_regrow<Helios, Selene>(
                m_curve_trees->m_c1,
                cached_layer_it->second,
                start_idx,
                end_idx);

            all_children_to_regrow.c2_children.emplace_back(std::move(children_to_regrow));
        }
        else
        {
            auto children_to_regrow = get_layer_last_chunk_children_to_regrow<Selene, Helios>(
                m_curve_trees->m_c2,
                cached_layer_it->second,
                start_idx,
                end_idx);

            all_children_to_regrow.c1_children.emplace_back(std::move(children_to_regrow));
        }

        parent_is_c2 = !parent_is_c2;
    }

    return all_children_to_regrow;
}
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LastHashes TreeSync<Helios, Selene>::get_last_hashes_to_trim(
    const std::vector<TrimLayerInstructions> &trim_instructions) const
{
    CurveTrees<Helios, Selene>::LastHashes last_hashes;

    if (trim_instructions.empty())
        return last_hashes;

    bool parent_is_c2 = true;
    for (LayerIdx i = 0; i < trim_instructions.size(); ++i)
    {
        const auto &trim_layer_instructions = trim_instructions[i];

        const std::size_t new_total_parents = trim_layer_instructions.new_total_parents;
        CHECK_AND_ASSERT_THROW_MES(new_total_parents > 0, "no new parents");
        const ChildChunkIdx last_parent_idx = new_total_parents - 1;

        const auto cached_layer_it = m_tree_elem_cache.find(i);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing layer for trim");

        auto cached_chunk_it = cached_layer_it->second.find(last_parent_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached chunk");

        if (parent_is_c2)
        {
            auto c2_point = m_curve_trees->m_c2->from_bytes(cached_chunk_it->second.tree_elem);

            MDEBUG("Last hash at layer: " << i << " , last_parent_idx: " << last_parent_idx
                << " , hash: " << m_curve_trees->m_c2->to_string(c2_point));

            last_hashes.c2_last_hashes.push_back(std::move(c2_point));
        }
        else
        {
            auto c1_point = m_curve_trees->m_c1->from_bytes(cached_chunk_it->second.tree_elem);

            MDEBUG("Last hash at layer: " << i << " , last_parent_idx: " << last_parent_idx
                << " , hash: " << m_curve_trees->m_c1->to_string(c1_point));

            last_hashes.c1_last_hashes.push_back(std::move(c1_point));
        }

        parent_is_c2 = !parent_is_c2;
    }

    return last_hashes;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeSync<C1, C2>::deque_block(const BlockHash &block_hash, const uint64_t old_n_leaf_tuples)
{
    // Remove ref to last chunk leaves from the cache
    const uint64_t offset = old_n_leaf_tuples % m_curve_trees->m_c2_width;
    const uint64_t old_last_chunk_start_leaf_idx = offset == 0
        ? (old_n_leaf_tuples - m_curve_trees->m_c2_width)
        : (old_n_leaf_tuples - offset);
    const ChildChunkIdx leaf_chunk_idx = old_last_chunk_start_leaf_idx / m_curve_trees->m_c2_width;
    remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);

    // Remove refs to prunable tree elems in the cache
    auto prunable_tree_elems_it = m_prunable_tree_elems_by_block.find(block_hash);
    CHECK_AND_ASSERT_THROW_MES(prunable_tree_elems_it != m_prunable_tree_elems_by_block.end(),
        "missing block of prunable tree elems");
    for (const auto &tree_elem : prunable_tree_elems_it->second)
    {
        const LayerIdx layer_idx = tree_elem.first;
        const ChildChunkIdxSet &child_chunk_idx_set = tree_elem.second;
        if (child_chunk_idx_set.empty())
            continue;

        auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing prunable cached layer");

        for (const auto &child_chunk_idx : child_chunk_idx_set)
        {
            auto cached_chunk_it = cached_layer_it->second.find(child_chunk_idx);
            CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(),
                "missing prunable cached chunk");
            CHECK_AND_ASSERT_THROW_MES(cached_chunk_it->second.ref_count != 0, "prunable chunk has 0 ref count");

            cached_chunk_it->second.ref_count -= 1;

            // If the ref count is 0, garbage collect it
            if (cached_chunk_it->second.ref_count == 0)
                m_tree_elem_cache[layer_idx].erase(cached_chunk_it);
        }

        // If the layer is empty, garbage collect it
        if (m_tree_elem_cache[layer_idx].empty())
            m_tree_elem_cache.erase(layer_idx);
    }
    m_prunable_tree_elems_by_block.erase(block_hash);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
