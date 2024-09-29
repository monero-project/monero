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


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void cache_path_elem(const std::unique_ptr<C> &curve,
    const std::size_t child_width,
    const std::size_t parent_width,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    ChildChunkIdx &start_child_chunk_idx_inout,
    ChildChunkIdx &end_child_chunk_idx_inout,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "high layer_ext_idx");
    auto &layer_ext = layer_exts[layer_ext_idx];

    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "empty layer ext");
    const std::size_t n_layer_elems = layer_ext.start_idx + layer_ext.hashes.size();

    // TODO: clean this up following cache_last_chunk approach
    start_child_chunk_idx_inout = std::max(start_child_chunk_idx_inout, layer_ext.start_idx);
    end_child_chunk_idx_inout = std::min(end_child_chunk_idx_inout, n_layer_elems);

    MDEBUG("Caching path elems from start_child_chunk_idx: " << start_child_chunk_idx_inout << " to end_child_chunk_idx: " << end_child_chunk_idx_inout);

    // Collect the path elems in the tree extension
    for (ChildChunkIdx child_chunk_idx = start_child_chunk_idx_inout; child_chunk_idx < end_child_chunk_idx_inout; ++child_chunk_idx)
    {
        // TODO: separate function
        CHECK_AND_ASSERT_THROW_MES(child_chunk_idx >= layer_ext.start_idx, "low child_chunk_Idx");
        const ChildChunkIdx ext_hash_idx = child_chunk_idx - layer_ext.start_idx;

        MDEBUG("ext_hash_idx: " << ext_hash_idx
            << " , hash: " << curve->to_string(layer_ext.hashes[ext_hash_idx])
            << " , update_existing_last_hash: " << layer_ext.update_existing_last_hash
            << " , child_chunk_idx: " << child_chunk_idx);

        // Check if the layer exists
        auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
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

        // We're already keeping track of this elem, so bump the ref count
        cached_tree_elem_it->second.ref_count += 1;

        // If the tree extension is updating an existing value, we need to update it in our cache too
        if (ext_hash_idx == 0 && layer_ext.update_existing_last_hash)
        {
            auto tree_elem = curve->to_bytes(layer_ext.hashes.front());
            cached_tree_elem_it->second.tree_elem = std::move(tree_elem);
        }
    }

    start_child_chunk_idx_inout /= parent_width;
    start_child_chunk_idx_inout = start_child_chunk_idx_inout - (start_child_chunk_idx_inout % child_width);
    end_child_chunk_idx_inout = start_child_chunk_idx_inout + child_width;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void cache_last_chunk(const std::unique_ptr<C> &curve,
    const std::vector<LayerExtension<C>> &layer_exts,
    const std::size_t layer_ext_idx,
    const LayerIdx layer_idx,
    const std::size_t parent_width,
    TreeElemCache &cached_tree_elems_inout)
{
    CHECK_AND_ASSERT_THROW_MES(layer_exts.size() > layer_ext_idx, "unexpected high layer_ext_idx");

    const auto &layer_ext = layer_exts[layer_ext_idx];
    CHECK_AND_ASSERT_THROW_MES(!layer_ext.hashes.empty(), "unexpected empty layer ext");

    const ChildChunkIdx end_child_chunk_idx = layer_ext.start_idx + layer_ext.hashes.size();
    const ChildChunkIdx start_child_chunk_idx = std::max(layer_ext.start_idx,
        end_child_chunk_idx - (end_child_chunk_idx % parent_width));

    MDEBUG("Caching start_child_chunk_idx " << start_child_chunk_idx << " to end_child_chunk_idx " << end_child_chunk_idx
        << " (layer start idx " << layer_ext.start_idx << ")");

    // TODO: this code is duplicated above
    for (ChildChunkIdx child_chunk_idx = start_child_chunk_idx; child_chunk_idx < end_child_chunk_idx; ++child_chunk_idx)
    {
        // TODO: separate function
        CHECK_AND_ASSERT_THROW_MES(child_chunk_idx >= layer_ext.start_idx, "low child_chunk_Idx");
        const ChildChunkIdx ext_hash_idx = child_chunk_idx - layer_ext.start_idx;

        auto cached_layer_it = cached_tree_elems_inout.find(layer_idx);
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

        // If the tree extension is updating an existing value, we need to update it in our cache too. Note that only the
        // first hash in the given layer extension can update (when update_existing_last_hash is true, the first hash is the
        // "existing last hash" before the tree extension is used to grow the tree).
        if (ext_hash_idx == 0 && layer_ext.update_existing_last_hash)
        {
            auto tree_elem = curve->to_bytes(layer_ext.hashes.front());
            cached_tree_elem_it->second.tree_elem = std::move(tree_elem);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
bool TreeSync<C1, C2>::register_output(const uint64_t block_idx_included_in_chain,
    const crypto::hash &block_hash_included_in_chain,
    const uint64_t unlock_block_idx,
    const OutputPair &output)
{
    if (!m_cached_blocks.empty())
    {
        CHECK_AND_ASSERT_THROW_MES(unlock_block_idx > m_cached_blocks.back().blk_idx,
            "already synced block in which output unlocked");
    }

    auto output_ref = get_output_ref(output);

    // Return false if already registered
    if (m_registered_outputs.find(output_ref) != m_registered_outputs.end())
        return false;

    // Add to registered outputs container
    m_registered_outputs.insert({ std::move(output_ref), AssignedLeafIdx{} });

    // Add to registered outputs by block container
    const RegisteredOutputContext registered_output{
            .output_ref = output_ref,
            .included_in_tree = false,
        };
    m_registered_outputs_by_block.insert({ block_hash_included_in_chain, registered_output });

    return true;
}

// Explicit instantiation
template bool TreeSync<Helios, Selene>::register_output(const uint64_t block_idx_included_in_chain,
    const crypto::hash &block_hash_included_in_chain,
    const uint64_t unlock_block_idx,
    const OutputPair &output);
//----------------------------------------------------------------------------------------------------------------------
// TODO: change all code to be more precise: I should know exactly which tree elems I need. Don't go by what's stored
template<typename C1, typename C2>
void TreeSync<C1, C2>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    std::vector<OutputContext> &&new_leaf_tuples)
{
    std::size_t n_leaf_tuples = 0;
    if (m_cached_blocks.empty())
    {
        // TODO: if block_idx > 0, we need the tree's last chunk elems and old_n_leaf_tuples
        CHECK_AND_ASSERT_THROW_MES(block_idx == 0, "syncing first block_idx > 0 not yet implemented");

        // Make sure all blockchain containers are empty
        CHECK_AND_ASSERT_THROW_MES(m_cached_blocks.empty(),     "expected empty cached blocks");
        CHECK_AND_ASSERT_THROW_MES(m_cached_leaves.empty(),     "expected empty cached leaves");
        CHECK_AND_ASSERT_THROW_MES(m_cached_tree_elems.empty(), "expected empty cached tree elems");
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
    const CurveTreesV1::LastHashes last_hashes = this->get_last_hashes(n_leaf_tuples);
    auto tree_extension = m_curve_trees->get_tree_extension(n_leaf_tuples,
        last_hashes,
        std::move(new_leaf_tuples));

    // Check if any registered outputs are present in the tree extension. If so, we assign the output its leaf idx and
    // start keeping track of the output's path elems
    // TODO: separate function
    std::unordered_set<LeafIdx> new_assigned_outputs;
    for (std::size_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &output_pair = tree_extension.leaves.tuples[i].output_pair;
        const auto output_ref = get_output_ref(output_pair);

        auto registered_output_it = m_registered_outputs.find(output_ref);
        if (registered_output_it == m_registered_outputs.end())
            continue;

        // If it's already assigned a leaf idx, then it must be a duplicate and we only care about the earliest one
        // TODO: test this circumstance
        if (registered_output_it->second.assigned_leaf_idx)
            continue;

        // Assign the leaf idx
        const LeafIdx leaf_idx = tree_extension.leaves.start_leaf_tuple_idx + i;
        registered_output_it->second = AssignedLeafIdx{ true, leaf_idx };

        MDEBUG("Starting to keep track of leaf_idx: " << leaf_idx);
        new_assigned_outputs.insert(leaf_idx);
    }

    // Cache tree elems from the tree extension needed in order to keep track of registered output paths in the tree
    const auto &c1_layer_exts = tree_extension.c1_layer_extensions;
    const auto &c2_layer_exts = tree_extension.c2_layer_extensions;
    const std::size_t n_layers = c1_layer_exts.size() + c2_layer_exts.size();
    for (const auto &registered_o : m_registered_outputs)
    {
        // Skip all registered outputs which have not been included in the tree yet
        if (!registered_o.second.assigned_leaf_idx)
            continue;

        MDEBUG("Caching tree elems for leaf idx: " << registered_o.second.leaf_idx);

        // Cache leaves
        // TODO: separate function
        const LeafIdx leaf_idx = registered_o.second.leaf_idx;
        const LeafIdx start_leaf_idx = (leaf_idx / m_curve_trees->m_c2_width) * m_curve_trees->m_c2_width;
        const LeafIdx end_leaf_idx = std::min(start_leaf_idx + m_curve_trees->m_c2_width,
            tree_extension.leaves.start_leaf_tuple_idx + tree_extension.leaves.tuples.size());

        // If the registered output's chunk isn't present in this tree extension, we have no leaves to cache
        if (end_leaf_idx > tree_extension.leaves.start_leaf_tuple_idx)
        {
            MDEBUG("Caching leaves for leaf_idx: " << leaf_idx << " , start_leaf_idx: " << start_leaf_idx << " , end_leaf_idx: " << end_leaf_idx);

            CHECK_AND_ASSERT_THROW_MES(end_leaf_idx > start_leaf_idx, "unexpected end_leaf_idx > start_leaf_idx");

            // Cache the leaf elems from this leaf's chunk
            for (std::size_t j = start_leaf_idx; j < end_leaf_idx; ++j)
            {
                auto leaf_it = m_cached_leaves.find(j);
                if (leaf_it != m_cached_leaves.end())
                {
                    // We only need to bump the ref count for new outputs included in this tree extension, or for
                    // outputs in the chunk of a newly registered output
                    const bool new_leaf = j >= tree_extension.leaves.start_leaf_tuple_idx;
                    const bool newly_assigned_output = new_assigned_outputs.find(j) != new_assigned_outputs.end();

                    if (newly_assigned_output || new_leaf)
                        leaf_it->second.ref_count += 1;

                    continue;
                }

                CHECK_AND_ASSERT_THROW_MES(j >= tree_extension.leaves.start_leaf_tuple_idx, "low j");
                const uint64_t tuple_idx = j - tree_extension.leaves.start_leaf_tuple_idx;

                CHECK_AND_ASSERT_THROW_MES(tree_extension.leaves.tuples.size() > tuple_idx, "high tuple_idx");
                auto tuple = std::move(tree_extension.leaves.tuples[tuple_idx]);

                m_cached_leaves[j] = CachedLeafTuple { .output = std::move(tuple.output_pair), .ref_count = 1 };
            }
        }
        // Done caching leaves

        // Now cache the rest of the path elems for each registered output
        // TODO: separate function
        const ChildChunkIdx child_chunk_idx = leaf_idx / m_curve_trees->m_c2_width;
        ChildChunkIdx start_child_chunk_idx = child_chunk_idx - (child_chunk_idx % m_curve_trees->m_c1_width);
        ChildChunkIdx end_child_chunk_idx = start_child_chunk_idx + m_curve_trees->m_c1_width;

        std::size_t c1_idx = 0, c2_idx = 0;
        bool parent_is_c1 = true;
        for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
        {
            MDEBUG("Caching tree elems from layer_idx " << layer_idx);
            if (parent_is_c1)
            {
                cache_path_elem(m_curve_trees->m_c2,
                        m_curve_trees->m_c2_width,
                        m_curve_trees->m_c1_width,
                        c2_layer_exts,
                        c2_idx,
                        layer_idx,
                        start_child_chunk_idx,
                        end_child_chunk_idx,
                        m_cached_tree_elems
                    );
                ++c2_idx;
            }
            else
            {
                cache_path_elem(m_curve_trees->m_c1,
                        m_curve_trees->m_c1_width,
                        m_curve_trees->m_c2_width,
                        c1_layer_exts,
                        c1_idx,
                        layer_idx,
                        start_child_chunk_idx,
                        end_child_chunk_idx,
                        m_cached_tree_elems
                    );
                ++c1_idx;
            }

            parent_is_c1 = !parent_is_c1;
        }
    }

    // Update cached blocks
    const uint64_t new_total_n_leaf_tuples = n_leaf_tuples + tree_extension.leaves.tuples.size();
    auto blk_meta = BlockMeta {
            .blk_idx = block_idx,
            .blk_hash = block_hash,
            .n_leaf_tuples = new_total_n_leaf_tuples,
        };
    m_cached_blocks.push(std::move(blk_meta));

    // Cache the last chunk of leaves, so if a registered output appears in the first chunk next block, we'll have all
    // prior leaves from that output's chunk
    // TODO: keep track of which leaves these are by block, so we can delete upon de-queing block from the cache
    // TODO: separate function
    const LeafIdx start_leaf_idx_last_chunk = new_total_n_leaf_tuples - (new_total_n_leaf_tuples % m_curve_trees->m_c2_width);
    const LeafIdx end_leaf_idx_last_chunk = std::min(start_leaf_idx_last_chunk + m_curve_trees->m_c2_width, new_total_n_leaf_tuples);
    for (LeafIdx i = start_leaf_idx_last_chunk; i < end_leaf_idx_last_chunk; ++i)
    {
        // Bump the ref count if it's already cached
        auto leaf_it = m_cached_leaves.find(i);
        if (leaf_it != m_cached_leaves.end())
        {
            leaf_it->second.ref_count += 1;
            continue;
        }

        // The leaf is not cached, so cache it
        CHECK_AND_ASSERT_THROW_MES(i >= tree_extension.leaves.start_leaf_tuple_idx,
            "the leaf isn't in this tree extension, expected the leaf to be cached already");
        const auto ext_idx = i - tree_extension.leaves.start_leaf_tuple_idx;
        auto &output = tree_extension.leaves.tuples[ext_idx].output_pair;
        m_cached_leaves[i] = CachedLeafTuple {
                .output = std::move(output),
                .ref_count = 1,
            };
    }

    // Cache the last chunk of hashes from every layer. We need to do this to handle all of the following:
    //   1) So we can use the tree's last hashes to grow the tree from here next block.
    //   2) In case a registered output appears in the first chunk next block, we'll have all its path elems cached.
    //   3) To trim the tree on reorg using the last children from each chunk
    // TODO: keep track of which hashes we add by block, so we can delete upon de-queing block from the cache
    // TODO: separate function
    bool use_c2 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    for (LayerIdx layer_idx = 0; layer_idx < n_layers; ++layer_idx)
    {
        MDEBUG("Caching the last chunk from layer " << layer_idx+1 << " / " << n_layers);
        if (use_c2)
        {
            cache_last_chunk(m_curve_trees->m_c2, c2_layer_exts, c2_idx, layer_idx, m_curve_trees->m_c1_width, m_cached_tree_elems);
            ++c2_idx;
        }
        else
        {
            cache_last_chunk(m_curve_trees->m_c1, c1_layer_exts, c1_idx, layer_idx, m_curve_trees->m_c2_width, m_cached_tree_elems);
            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}

// Explicit instantiation
template void TreeSync<Helios, Selene>::sync_block(const uint64_t block_idx,
    const crypto::hash &block_hash,
    const crypto::hash &prev_block_hash,
    std::vector<OutputContext> &&new_leaf_tuples);
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
    const LeafIdx end_leaf_idx = start_leaf_idx + m_curve_trees->m_c2_width;

    MDEBUG("Getting output path at leaf_idx: " << leaf_idx << " , start_leaf_idx: " << start_leaf_idx << " , end_leaf_idx: " << end_leaf_idx);

    CHECK_AND_ASSERT_THROW_MES(start_leaf_idx <= leaf_idx && leaf_idx < end_leaf_idx, "unexpected leaf idx range");

    // Collect cached leaves from the leaf chunk this leaf is in
    for (LeafIdx i = start_leaf_idx; i < end_leaf_idx; ++i)
    {
        auto it = m_cached_leaves.find(i);
        if (it == m_cached_leaves.end())
            break;

        MDEBUG("Found leaf idx " << i);
        path_out.leaves.push_back(output_to_tuple(it->second.output));
    }

    CHECK_AND_ASSERT_THROW_MES((start_leaf_idx + path_out.leaves.size()) > leaf_idx, "leaves path missing leaf_idx");

    // Collect cached tree elems in the leaf's path
    LayerIdx layer_idx = 0;
    child_chunk_idx /= m_curve_trees->m_c1_width;
    ChildChunkIdx start_child_chunk_idx = child_chunk_idx * m_curve_trees->m_c1_width;
    ChildChunkIdx end_child_chunk_idx = start_child_chunk_idx + m_curve_trees->m_c1_width;
    bool parent_is_c1 = true;
    while (1)
    {
        auto cached_layer_it = m_cached_tree_elems.find(layer_idx);
        if (cached_layer_it == m_cached_tree_elems.end())
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

            MDEBUG("Seeking child chunk idx: " << i);
            auto &tree_elem = cached_tree_elem_it->second.tree_elem;
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

        auto cached_layer_it = m_cached_tree_elems.find(layer_idx);
        CHECK_AND_ASSERT_THROW_MES(cached_layer_it != m_cached_tree_elems.end(), "missing cached last hash layer");

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
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp