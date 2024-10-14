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

#include "tree_sync_memory.h"

#include "misc_log_ex.h"
#include "string_tools.h"


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
OutputRef get_output_ref(const OutputPair &o)
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

    MDEBUG("Starting to keep track of leaf_idx: " << leaf_idx);
    registered_output_it->second.assign_leaf(leaf_idx);

    return;
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
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx start_leaf_idx = chunk_idx * leaf_parent_chunk_width;
    const LeafIdx end_leaf_idx   = std::min(start_leaf_idx + leaf_parent_chunk_width, n_leaf_tuples);

    CHECK_AND_ASSERT_THROW_MES(end_leaf_idx > start_leaf_idx, "start_leaf_idx is too high");

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

    // See how many new leaves we need to add to the chunk
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
template<typename C>
static void cache_path_chunk(const std::unique_ptr<C> &curve,
    const std::size_t parent_width,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const bool bump_ref_count,
    const ChildChunkIdx parent_idx,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "high layer_ext_idx");
    auto &layer_ext = layer_exts[layer_ext_idx];

    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "empty layer ext");
    const uint64_t n_layer_elems = layer_ext.start_idx + layer_ext.hashes.size();

    const ChildChunkIdx start_chunk_idx = parent_idx * parent_width;
    const ChildChunkIdx end_chunk_idx   = std::min(start_chunk_idx + parent_width, n_layer_elems);
    CHECK_AND_ASSERT_THROW_MES(end_chunk_idx > start_chunk_idx, "start_chunk_idx is too high");

    MDEBUG("Caching path elems at layer_idx: "  << layer_idx
        << " , parent_idx: "                    << parent_idx
        << " , start_chunk_idx: "               << start_chunk_idx
        << " , end_chunk_idx: "                 << end_chunk_idx
        << " , bump_ref_count: "                << bump_ref_count
        << " , start_idx: "                     << layer_ext.start_idx);

    // If the chunk isn't in this tree extension at all, there are no elems we need to cache
    if (layer_ext.start_idx >= end_chunk_idx)
        return;

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

    // See how many new elems we need to add to the chunk
    const uint64_t expected_chunk_size = end_chunk_idx - start_chunk_idx;
    CHECK_AND_ASSERT_THROW_MES(expected_chunk_size >= cached_chunk_size, "cached chunk is too big");
    const uint64_t new_elems_needed = expected_chunk_size - cached_chunk_size;

    MDEBUG("layer_cache_hit: "        << layer_cache_hit
        << " , cache_hit: "           << cache_hit
        << " , cached_chunk_size: "   << cached_chunk_size
        << " , expected_chunk_size: " << expected_chunk_size
        << " , new_elems_needed: "    << new_elems_needed);

    // If we already have all the latest elems, we're done, we've already bumped the ref count if needed
    if (new_elems_needed == 0)
        return;

    // Add the new elems in the chunk to the cache
    const ChildChunkIdx end_i = end_chunk_idx - layer_ext.start_idx;
    CHECK_AND_ASSERT_THROW_MES(layer_ext.hashes.size() >= end_i, "high end_i");
    CHECK_AND_ASSERT_THROW_MES(end_i >= new_elems_needed, "low end_i");
    const ChildChunkIdx start_i = end_i - new_elems_needed;

    // Collect the new elems into cache
    std::vector<std::array<uint8_t, 32UL>> new_elems;
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

    CHECK_AND_ASSERT_THROW_MES(cached_tree_elems_inout[layer_idx][parent_idx].tree_elems.size() == expected_chunk_size,
        "unexpected chunk size");
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

    // Make sure the layer is already cached
    auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
    CHECK_AND_ASSERT_THROW_MES(cached_layer_it != cached_tree_elems_inout.end(), "missing cached last layer");

    // Make sure the chunk is cached
    auto cached_chunk_it = cached_layer_it->second.find(last_parent_idx);
    CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached last chunk");

    cached_chunk_it->second.tree_elems.back() = curve->to_bytes(layer_ext.hashes.front());
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void cache_path_chunks(const LeafIdx leaf_idx,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const std::vector<LayerExtension<C1>> &c1_layer_exts,
    const std::vector<LayerExtension<C2>> &c2_layer_exts,
    const bool bump_ref_count,
    TreeElemCache &tree_elem_cache_inout)
{
    const ChildChunkIdx child_chunk_idx = leaf_idx / curve_trees->m_c2_width;
    ChildChunkIdx parent_idx = child_chunk_idx / curve_trees->m_c1_width;

    std::size_t c1_idx = 0, c2_idx = 0;
    bool parent_is_c1 = true;
    const std::size_t n_layers = c1_layer_exts.size() + c2_layer_exts.size();
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        MDEBUG("Caching tree elems from layer_idx " << layer_idx << " parent_idx " << parent_idx);
        if (parent_is_c1)
        {
            cache_path_chunk(curve_trees->m_c2,
                    curve_trees->m_c1_width,
                    c2_layer_exts,
                    c2_idx,
                    layer_idx,
                    bump_ref_count,
                    parent_idx,
                    tree_elem_cache_inout
                );

            parent_idx /= curve_trees->m_c2_width;
            ++c2_idx;
        }
        else
        {
            cache_path_chunk(curve_trees->m_c1,
                    curve_trees->m_c2_width,
                    c1_layer_exts,
                    c1_idx,
                    layer_idx,
                    bump_ref_count,
                    parent_idx,
                    tree_elem_cache_inout
                );

            parent_idx /= curve_trees->m_c1_width;
            ++c1_idx;
        }

        parent_is_c1 = !parent_is_c1;
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

    const ChildChunkIdx child_chunk_idx = old_n_leaf_tuples / curve_trees->m_c2_width;
    ChildChunkIdx last_parent_idx = child_chunk_idx / curve_trees->m_c1_width;

    std::size_t c1_idx = 0, c2_idx = 0;
    bool parent_is_c1 = true;
    const std::size_t n_layers = c1_layer_exts.size() + c2_layer_exts.size();
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        MDEBUG("Updating existing last hash from layer_idx " << layer_idx << " last_parent_idx " << last_parent_idx);
        if (parent_is_c1)
        {
            update_last_hash(curve_trees->m_c2,
                    c2_layer_exts,
                    c2_idx,
                    layer_idx,
                    last_parent_idx,
                    tree_elem_cache_inout
                );

            last_parent_idx /= curve_trees->m_c2_width;
            ++c2_idx;
        }
        else
        {
            update_last_hash(curve_trees->m_c1,
                    c1_layer_exts,
                    c1_idx,
                    layer_idx,
                    last_parent_idx,
                    tree_elem_cache_inout
                );

            last_parent_idx /= curve_trees->m_c1_width;
            ++c1_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
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
    MDEBUG("Removing ref to chunk " << chunk_idx << " in layer " << layer_idx
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

    const ChildChunkIdx child_chunk_idx = leaf_idx / curve_trees->m_c2_width;
    ChildChunkIdx parent_idx = child_chunk_idx / curve_trees->m_c1_width;

    const std::size_t n_layers = curve_trees->n_layers(n_leaf_tuples);

    bool parent_is_c1 = true;
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        remove_path_chunk_ref(layer_idx, parent_idx, tree_elem_cache_inout);
        parent_is_c1 = !parent_is_c1;
        parent_idx /= parent_is_c1 ? curve_trees->m_c1_width : curve_trees->m_c2_width;
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
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != leaf_cache_inout.end(), "cache is missing leaf chunk");

    // The last chunk should have at least offset leaves
    const std::size_t n_leaves_last_chunk = leaf_chunk_it->second.leaves.size();
    CHECK_AND_ASSERT_THROW_MES(n_leaves_last_chunk >= offset, "unexpected n leaves in cached last chunk");

    leaf_chunk_it->second.leaves.resize(offset);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void reduce_cached_last_chunks(
    const typename CurveTrees<C1, C2>::TreeReduction &tree_reduction,
    const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    TreeElemCache &tree_elem_cache_inout)
{
    const uint64_t n_leaf_tuples = tree_reduction.new_total_leaf_tuples;
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx last_leaf_idx = n_leaf_tuples - 1;
    ChildChunkIdx last_chunk_idx = last_leaf_idx / curve_trees->m_c2_width;

    const auto &c1_layer_reductions = tree_reduction.c1_layer_reductions;
    const auto &c2_layer_reductions = tree_reduction.c2_layer_reductions;

    std::size_t c1_idx = 0, c2_idx = 0;
    bool parent_is_c1 = true;
    const std::size_t n_layers = c2_layer_reductions.size() + c1_layer_reductions.size();
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        // TODO: separate templated function
        const std::size_t parent_width = parent_is_c1 ? curve_trees->m_c1_width : curve_trees->m_c2_width;
        const ChildChunkIdx parent_idx = last_chunk_idx / parent_width;

        // Get the layer
        auto cached_layer_it = tree_elem_cache_inout.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != tree_elem_cache_inout.end(), "missing cached layer");

        // Get the chunk
        auto cached_chunk_it = cached_layer_it->second.find(parent_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached last chunk");

        // Shrink the chunk to the expected size
        const uint64_t n_layer_elems = last_chunk_idx + 1;
        const std::size_t chunk_offset = n_layer_elems % parent_width;
        const std::size_t new_chunk_size = chunk_offset == 0 ? parent_width : chunk_offset;

        MDEBUG("Reducing cached last chunk in layer_idx: " << layer_idx
            << " , parent_idx: "                           << parent_idx
            << " , last_chunk_idx: "                       << last_chunk_idx
            << " , new_chunk_size: "                       << new_chunk_size);

        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it->second.tree_elems.size() >= new_chunk_size, "chunk is too small");
        cached_chunk_it->second.tree_elems.resize(new_chunk_size);

        // Update the last hash in the chunk if necessary
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_layer_reductions.size() > c2_idx, "missing c2 layer reduction");
            const auto &c2_reduction = c2_layer_reductions[c2_idx];

            if (c2_reduction.update_existing_last_hash)
            {
                auto tree_elem = curve_trees->m_c2->to_bytes(c2_reduction.new_last_hash);
                cached_chunk_it->second.tree_elems.back() = std::move(tree_elem);
            }

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_layer_reductions.size() > c1_idx, "missing c1 layer reduction");
            const auto &c1_reduction = c1_layer_reductions[c1_idx];

            if (c1_reduction.update_existing_last_hash)
            {
                auto tree_elem = curve_trees->m_c1->to_bytes(c1_reduction.new_last_hash);
                cached_chunk_it->second.tree_elems.back() = std::move(tree_elem);
            }

            ++c1_idx;
        }

        last_chunk_idx = parent_idx;
        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
static void update_registered_path(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const LeafIdx leaf_idx,
    const typename CurveTrees<C1, C2>::TreeExtension &tree_extension,
    LeafCache &leaf_cache_inout,
    TreeElemCache &tree_elem_cach_inout)
{
    // We only need to bump the ref count on this registered output's leaf chunk if it was just included in the tree
    const bool bump_ref_count = leaf_idx >= tree_extension.leaves.start_leaf_tuple_idx;

    // Cache registered leaf's chunk
    cache_leaf_chunk<C1, C2>(leaf_idx / curve_trees->m_c2_width,
        curve_trees->m_c2_width,
        tree_extension.leaves,
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
        bump_ref_count,
        tree_elem_cach_inout);
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
template<typename C1, typename C2>
static void cache_last_chunks(const std::shared_ptr<CurveTrees<C1, C2>> &curve_trees,
    const typename CurveTrees<C1, C2>::TreeExtension &tree_extension,
    TreeElemCache &tree_elem_cache_inout)
{
    const uint64_t n_leaf_tuples = tree_extension.leaves.start_leaf_tuple_idx + tree_extension.leaves.tuples.size();
    if (n_leaf_tuples == 0)
        return;

    const LeafIdx last_leaf_idx = n_leaf_tuples - 1;

    // Always bump the ref count for last chunk of hashes so that it sticks around until pruned
    const bool bump_ref_count = true;

    cache_path_chunks<C1, C2>(last_leaf_idx,
        curve_trees,
        tree_extension.c1_layer_extensions,
        tree_extension.c2_layer_extensions,
        bump_ref_count,
        tree_elem_cache_inout);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_CHILD, typename C_PARENT>
static std::vector<typename C_PARENT::Scalar> get_layer_last_chunk_children_to_regrow(
    const std::unique_ptr<C_CHILD> &c_child,
    const ChildChunkCache &child_chunk_cache,
    const ChildChunkIdx start_idx,
    const ChildChunkIdx end_idx,
    const std::size_t parent_width)
{
    std::vector<typename C_PARENT::Scalar> children_to_regrow_out;
    if (end_idx > start_idx)
    {
        const uint64_t n_elems = end_idx - start_idx;

        const ChildChunkIdx chunk_idx = start_idx / parent_width;

        const auto cached_chunk_it = child_chunk_cache.find(chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != child_chunk_cache.end(), "missing child chunk for regrow");

        children_to_regrow_out.reserve(n_elems);
        for (uint64_t i = 0; i < n_elems; ++i)
        {
            auto child_point  = c_child->from_bytes(cached_chunk_it->second.tree_elems[i]);
            auto child_scalar = c_child->point_to_cycle_scalar(child_point);

            MDEBUG("Re-growing child chunk idx: " << start_idx+i << " , elem: " << c_child->to_string(child_point));

            children_to_regrow_out.emplace_back(std::move(child_scalar));
        }
    }

    return children_to_regrow_out;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSyncMemory<C1, C2>::register_output(const OutputPair &output, const uint64_t unlock_block_idx)
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
template bool TreeSyncMemory<Helios, Selene>::register_output(const OutputPair &output, const uint64_t unlock_block_idx);
//----------------------------------------------------------------------------------------------------------------------
// TODO: change all code to be more precise: I should know exactly which tree elems I need. Don't go by what's stored
template<typename C1, typename C2>
void TreeSyncMemory<C1, C2>::sync_block(const uint64_t block_idx,
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
    // in the tree and cache the data necessary to either build the next block's tree extension or pop the block.
    const auto tree_extension = TreeSync<C1, C2>::m_curve_trees->get_tree_extension(
        n_leaf_tuples,
        this->get_last_hashes(n_leaf_tuples),
        std::move(new_leaf_tuples));

    // Update the existing last hashes in the cache using the tree extension
    update_existing_last_hashes<C1, C2>(TreeSync<C1, C2>::m_curve_trees, tree_extension, m_tree_elem_cache);

    // Check if any registered outputs are present in the tree extension. If so, we assign the output its leaf idx and
    // start keeping track of the output's path elems
    for (uint64_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &output_pair = tree_extension.leaves.tuples[i].output_pair;
        const LeafIdx leaf_idx = tree_extension.leaves.start_leaf_tuple_idx + i;
        assign_new_output(output_pair, leaf_idx, m_registered_outputs);
    }

    // Cache tree elems from the tree extension needed in order to keep track of registered output paths in the tree
    for (const auto &registered_o : m_registered_outputs)
    {
        // Skip all registered outputs which have not been included in the tree yet
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        update_registered_path<C1, C2>(TreeSync<C1, C2>::m_curve_trees,
            registered_o.second.leaf_idx,
            tree_extension,
            m_leaf_cache,
            m_tree_elem_cache);
    }

    // Cache the last chunk of leaves, so if a registered output appears in the first chunk next block, we'll have all
    // prior leaves from that output's chunk already saved
    cache_last_chunk_leaves<C1, C2>(TreeSync<C1, C2>::m_curve_trees, tree_extension.leaves, m_leaf_cache);

    // Cache the last chunk of hashes from every layer. We need to do this to handle all of the following:
    //   1) So we can use the tree's last hashes to grow the tree from here next block.
    //   2) In case a registered output appears in the first chunk next block, we'll have all its path elems cached.
    //   3) To trim the tree on reorg by re-growing with the children in each last chunk.
    cache_last_chunks<C1, C2>(TreeSync<C1, C2>::m_curve_trees, tree_extension, m_tree_elem_cache);

    // Update cached blocks
    const uint64_t new_total_n_leaf_tuples = n_leaf_tuples + tree_extension.leaves.tuples.size();
    auto blk_meta = BlockMeta {
            .blk_idx = block_idx,
            .blk_hash = block_hash,
            .n_leaf_tuples = new_total_n_leaf_tuples,
        };
    m_cached_blocks.push_back(std::move(blk_meta));

    // Deque the oldest cached block upon reaching the max reorg depth
    if (m_cached_blocks.size() > TreeSync<C1, C2>::m_max_reorg_depth)
    {
        CHECK_AND_ASSERT_THROW_MES(!m_cached_blocks.empty(), "empty cached blocks");
        this->deque_block(m_cached_blocks.front());
        m_cached_blocks.pop_front();
    }

    CHECK_AND_ASSERT_THROW_MES((TreeSync<C1, C2>::m_max_reorg_depth >= m_cached_blocks.size()),
        "cached blocks exceeded max reorg depth");
}

// Explicit instantiation
template void TreeSyncMemory<Helios, Selene>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    std::vector<OutputContext> &&new_leaf_tuples);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSyncMemory<C1, C2>::pop_block()
{
    if (m_cached_blocks.empty())
        return false;

    // Pop the top block off the cache
    const uint64_t old_n_leaf_tuples = m_cached_blocks.back().n_leaf_tuples;
    this->deque_block(m_cached_blocks.back());
    m_cached_blocks.pop_back();

    // Determine how many leaves we need to trim
    uint64_t new_n_leaf_tuples = 0;
    if (!m_cached_blocks.empty())
    {
        // We're trimming to the new top block
        const BlockMeta &new_top_block = m_cached_blocks.back();
        new_n_leaf_tuples = new_top_block.n_leaf_tuples;
    }
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples >= new_n_leaf_tuples, "expected old_n_leaf_tuples >= new_n_leaf_tuples");
    const uint64_t trim_n_leaf_tuples = old_n_leaf_tuples - new_n_leaf_tuples;

    // We're going to trim the tree as the node would to see exactly how the tree elems we know about need to change.
    // First get trim instructions
    const auto trim_instructions = TreeSync<C1, C2>::m_curve_trees->get_trim_instructions(old_n_leaf_tuples,
        trim_n_leaf_tuples,
        true/*always_regrow_with_remaining, since we don't save all new tree elems in every chunk*/);
    MDEBUG("Acquired trim instructions for " << trim_instructions.size() << " layers");

    // Do initial tree reads using trim instructions
    const auto last_chunk_children_to_regrow = this->get_last_chunk_children_to_regrow(trim_instructions);
    const auto last_hashes_to_trim = this->get_last_hashes_to_trim(trim_instructions);

    // Get the new hashes, wrapped in a simple struct we can use to trim the tree
    const auto tree_reduction = TreeSync<C1, C2>::m_curve_trees->get_tree_reduction(
        trim_instructions,
        last_chunk_children_to_regrow,
        last_hashes_to_trim);

    const auto &c1_layer_reductions = tree_reduction.c1_layer_reductions;
    const auto &c2_layer_reductions = tree_reduction.c2_layer_reductions;
    const std::size_t new_n_layers = c2_layer_reductions.size() + c1_layer_reductions.size();

    // Shrink the current last chunk if some of the leaves in it got cut off
    shrink_cached_last_leaf_chunk(new_n_leaf_tuples, TreeSync<C1, C2>::m_curve_trees->m_c2_width, m_leaf_cache);

    // Use the tree reduction to update ref'd last hashes and shrink current last chunks as necessary
    reduce_cached_last_chunks<C1, C2>(tree_reduction, TreeSync<C1, C2>::m_curve_trees, m_tree_elem_cache);

    // Use the tree reduction to update registered output paths
    for (auto &registered_o : m_registered_outputs)
    {
        // If the output isn't in the tree, it has no path elems we need to change in the cache 
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        // TODO: below should all be a separate function
        // Get the output's cached path indexes in the tree
        const LeafIdx leaf_idx = registered_o.second.leaf_idx;
        const auto old_path_idxs = TreeSync<C1, C2>::m_curve_trees->get_path_indexes(old_n_leaf_tuples, leaf_idx);

        const bool output_removed_from_tree = leaf_idx >= tree_reduction.new_total_leaf_tuples;

        // Use the tree reduction to update the cached leaves and path elems
        if (output_removed_from_tree)
        {
            const ChildChunkIdx leaf_chunk_idx = leaf_idx / TreeSync<C1, C2>::m_curve_trees->m_c2_width;
            remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);
            remove_path_chunks_refs(leaf_idx, TreeSync<C1, C2>::m_curve_trees, old_n_leaf_tuples, m_tree_elem_cache);
        }

        if (output_removed_from_tree)
        {
            MDEBUG("Un-assigning leaf idx " << leaf_idx);
            registered_o.second.unassign_leaf();
        }
    }

    // Check if there are any remaining layers that need to be removed
    // NOTE: this should only be useful for removing excess layers from registered outputs
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
template bool TreeSyncMemory<Helios, Selene>::pop_block();
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSyncMemory<C1, C2>::get_output_path(const OutputPair &output, typename CurveTrees<C1, C2>::Path &path_out) const
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
    ChildChunkIdx chunk_idx = leaf_idx / TreeSync<C1, C2>::m_curve_trees->m_c2_width;

    MDEBUG("Getting output path at leaf_idx: " << leaf_idx);

    // Collect cached leaves from the leaf chunk this leaf is in
    const auto leaf_chunk_it = m_leaf_cache.find(chunk_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != m_leaf_cache.end(), "missing cached leaf chunk");
    for (const auto &leaf : leaf_chunk_it->second.leaves)
        path_out.leaves.push_back(output_to_tuple(leaf));

    // Collect cached tree elems in the leaf's path
    LayerIdx layer_idx = 0;
    bool parent_is_c1 = true;
    while (1)
    {
        auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        if (cached_layer_it == m_tree_elem_cache.end())
            break;

        chunk_idx /= parent_is_c1 ? TreeSync<C1, C2>::m_curve_trees->m_c1_width : TreeSync<C1, C2>::m_curve_trees->m_c2_width;

        const auto cached_chunk_it = cached_layer_it->second.find(chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached chunk");

        MDEBUG("Getting output path at layer_idx " << layer_idx << " chunk_idx " << chunk_idx);

        if (parent_is_c1)
            path_out.c2_layers.emplace_back();
        else
            path_out.c1_layers.emplace_back();

        for (const auto &tree_elem : cached_chunk_it->second.tree_elems)
        {
            MDEBUG("Found elem: " << epee::string_tools::pod_to_hex(tree_elem));
            if (parent_is_c1)
                path_out.c2_layers.back().push_back(TreeSync<C1, C2>::m_curve_trees->m_c2->from_bytes(tree_elem));
            else
                path_out.c1_layers.back().push_back(TreeSync<C1, C2>::m_curve_trees->m_c1->from_bytes(tree_elem));
        }

        parent_is_c1 = !parent_is_c1;
        ++layer_idx;
    }

    return true;
}

// Explicit instantiation
template bool TreeSyncMemory<Helios, Selene>::get_output_path(const OutputPair &output,
    CurveTrees<Helios, Selene>::Path &path_out) const;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::LastHashes TreeSyncMemory<C1, C2>::get_last_hashes(const uint64_t n_leaf_tuples) const
{
    MDEBUG("Getting last hashes on tree with " << n_leaf_tuples << " leaf tuples");

    typename CurveTrees<C1, C2>::LastHashes last_hashes;
    if (n_leaf_tuples == 0)
        return last_hashes;

    uint64_t n_children = n_leaf_tuples;
    bool use_c2 = true;
    LayerIdx layer_idx = 0;
    do
    {
        const std::size_t width = use_c2 ? TreeSync<C1, C2>::m_curve_trees->m_c2_width : TreeSync<C1, C2>::m_curve_trees->m_c1_width;
        const ChildChunkIdx last_child_chunk_idx = (n_children - 1) / width;

        const std::size_t parent_width = use_c2 ? TreeSync<C1, C2>::m_curve_trees->m_c1_width : TreeSync<C1, C2>::m_curve_trees->m_c2_width;
        const ChildChunkIdx last_parent_idx = last_child_chunk_idx / parent_width;

        MDEBUG("Getting last hash at layer_idx " << layer_idx << " and last_parent_idx " << last_parent_idx);

        auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing cached last hash layer");

        auto cached_chunk_it = cached_layer_it->second.find(last_parent_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached last chunk");

        CHECK_AND_ASSERT_THROW_MES(!cached_chunk_it->second.tree_elems.empty(), "empty cached last chunk");

        const auto &last_hash = cached_chunk_it->second.tree_elems.back();
        if (use_c2)
            last_hashes.c2_last_hashes.push_back(TreeSync<C1, C2>::m_curve_trees->m_c2->from_bytes(last_hash));
        else
            last_hashes.c1_last_hashes.push_back(TreeSync<C1, C2>::m_curve_trees->m_c1->from_bytes(last_hash));

        ++layer_idx;
        n_children = last_child_chunk_idx + 1;
        use_c2 = !use_c2;
    }
    while (n_children > 1);

    return last_hashes;
}

// Explicit instantiation
template CurveTrees<Helios, Selene>::LastHashes TreeSyncMemory<Helios, Selene>::get_last_hashes(
    const uint64_t n_leaf_tuples) const;
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LastChunkChildrenToTrim TreeSyncMemory<Helios, Selene>::get_last_chunk_children_to_regrow(
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
        const ChildChunkIdx chunk_idx = idx / TreeSync<Helios, Selene>::m_curve_trees->m_leaf_layer_chunk_width;

        const auto leaf_chunk_it = m_leaf_cache.find(chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(leaf_chunk_it != m_leaf_cache.end(), "missing cached leaf chunk");
        auto leaf_it = leaf_chunk_it->second.leaves.begin();

        do
        {
            const LeafIdx leaf_idx = idx / LEAF_TUPLE_SIZE;
            MDEBUG("Re-growing with leaf idx " << leaf_idx);
            CHECK_AND_ASSERT_THROW_MES(leaf_it != leaf_chunk_it->second.leaves.end(), "missing cached leaf");

            const auto leaf_tuple = TreeSync<Helios, Selene>::m_curve_trees->leaf_tuple(*leaf_it);

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
        const std::size_t parent_width = parent_is_c2
            ? TreeSync<Helios, Selene>::m_curve_trees->m_c2_width
            : TreeSync<Helios, Selene>::m_curve_trees->m_c1_width;

        const LayerIdx layer_idx = i - 1;
        const auto cached_layer_it = m_tree_elem_cache.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing layer for trim");

        if (parent_is_c2)
        {
            auto children_to_regrow = get_layer_last_chunk_children_to_regrow<Helios, Selene>(
                TreeSync<Helios, Selene>::m_curve_trees->m_c1,
                cached_layer_it->second,
                start_idx,
                end_idx,
                parent_width);

            all_children_to_regrow.c2_children.emplace_back(std::move(children_to_regrow));
        }
        else
        {
            auto children_to_regrow = get_layer_last_chunk_children_to_regrow<Selene, Helios>(
                TreeSync<Helios, Selene>::m_curve_trees->m_c2,
                cached_layer_it->second,
                start_idx,
                end_idx,
                parent_width);

            all_children_to_regrow.c1_children.emplace_back(std::move(children_to_regrow));
        }

        parent_is_c2 = !parent_is_c2;
    }

    return all_children_to_regrow;
}
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LastHashes TreeSyncMemory<Helios, Selene>::get_last_hashes_to_trim(
    const std::vector<TrimLayerInstructions> &trim_instructions) const
{
    CurveTrees<Helios, Selene>::LastHashes last_hashes;

    if (trim_instructions.empty())
        return last_hashes;

    bool parent_is_c2 = true;
    for (LayerIdx i = 0; i < trim_instructions.size(); ++i)
    {
        const auto &trim_layer_instructions = trim_instructions[i];

        const uint64_t new_total_parents = trim_layer_instructions.new_total_parents;
        CHECK_AND_ASSERT_THROW_MES(new_total_parents > 0, "no new parents");
        const ChildChunkIdx new_last_idx = new_total_parents - 1;

        const std::size_t grandparent_width = parent_is_c2
            ? TreeSync<Helios, Selene>::m_curve_trees->m_c1_width
            : TreeSync<Helios, Selene>::m_curve_trees->m_c2_width;
        const ChildChunkIdx chunk_idx = new_last_idx / grandparent_width;

        const auto cached_layer_it = m_tree_elem_cache.find(i);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_tree_elem_cache.end(), "missing layer for trim");

        auto cached_chunk_it = cached_layer_it->second.find(chunk_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it != cached_layer_it->second.end(), "missing cached chunk");

        const std::size_t new_offset = new_last_idx % grandparent_width;

        MDEBUG("Getting last hash to trim at layer " << i
            << " , new_total_parents: " << new_total_parents
            << " , grandparent_width: " << grandparent_width
            << " , chunk_idx: " << chunk_idx
            << " , new_offset: " << new_offset
            << " , existing chunk size: " << cached_chunk_it->second.tree_elems.size());

        CHECK_AND_ASSERT_THROW_MES(cached_chunk_it->second.tree_elems.size() > new_offset, "small cached chunk");
        auto &last_hash = cached_chunk_it->second.tree_elems[new_offset];

        if (parent_is_c2)
        {
            auto c2_point = TreeSync<Helios, Selene>::m_curve_trees->m_c2->from_bytes(last_hash);

            MDEBUG("Last hash at layer: " << i << " , new_last_idx: " << new_last_idx
                << " , hash: " << (TreeSync<Helios, Selene>::m_curve_trees->m_c2->to_string(c2_point)));

            last_hashes.c2_last_hashes.push_back(std::move(c2_point));
        }
        else
        {
            auto c1_point = TreeSync<Helios, Selene>::m_curve_trees->m_c1->from_bytes(last_hash);

            MDEBUG("Last hash at layer: " << i << " , new_last_idx: " << new_last_idx
                << " , hash: " << (TreeSync<Helios, Selene>::m_curve_trees->m_c1->to_string(c1_point)));

            last_hashes.c1_last_hashes.push_back(std::move(c1_point));
        }

        parent_is_c2 = !parent_is_c2;
    }

    return last_hashes;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void TreeSyncMemory<C1, C2>::deque_block(const BlockMeta &block)
{
    if (block.n_leaf_tuples == 0)
        return;

    // Remove ref to last chunk leaves from the cache
    const LeafIdx old_last_leaf_idx = block.n_leaf_tuples - 1;
    const ChildChunkIdx leaf_chunk_idx = old_last_leaf_idx / TreeSync<C1, C2>::m_curve_trees->m_c2_width;
    remove_leaf_chunk_ref(leaf_chunk_idx, m_leaf_cache);

    // Remove refs to last chunk in every layer
    remove_path_chunks_refs(old_last_leaf_idx, TreeSync<C1, C2>::m_curve_trees, block.n_leaf_tuples, m_tree_elem_cache);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
