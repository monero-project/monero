// Copyright (c) 2014, The Monero Project
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

#include "gtest/gtest.h"

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "curve_trees.h"
#include "misc_log_ex.h"
#include "unit_tests_utils.h"

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree helpers
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static fcmp::curve_trees::LastChunkData<C> get_last_child_layer_chunk(const bool update_last_parent,
    const std::size_t parent_layer_size,
    const typename C::Point &last_parent,
    const typename C::Scalar &last_child)
{
    if (update_last_parent)
        CHECK_AND_ASSERT_THROW_MES(parent_layer_size > 0, "empty parent layer");

    // If updating last parent, the next start will be the last parent's index, else we start at the tip
    const std::size_t next_start_child_chunk_index = update_last_parent
        ? (parent_layer_size - 1)
        : parent_layer_size;

    return fcmp::curve_trees::LastChunkData<C>{
        .next_start_child_chunk_index = next_start_child_chunk_index,
        .last_parent                  = last_parent,
        .update_last_parent           = update_last_parent,
        .last_child                   = last_child
    };
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static bool validate_layer(const C &curve,
    const CurveTreesGlobalTree::Layer<C> &parents,
    const std::vector<typename C::Scalar> &child_scalars,
    const std::size_t max_chunk_size)
{
    // Hash chunk of children scalars, then see if the hash matches up to respective parent
    std::size_t chunk_start_idx = 0;
    for (std::size_t i = 0; i < parents.size(); ++i)
    {
        CHECK_AND_ASSERT_MES(child_scalars.size() > chunk_start_idx, false, "chunk start too high");
        const std::size_t chunk_size = std::min(child_scalars.size() - chunk_start_idx, max_chunk_size);
        CHECK_AND_ASSERT_MES(child_scalars.size() >= (chunk_start_idx + chunk_size), false, "chunk size too large");

        const typename C::Point &parent = parents[i];

        const auto chunk_start = child_scalars.data() + chunk_start_idx;
        const typename C::Chunk chunk{chunk_start, chunk_size};

        for (std::size_t i = 0; i < chunk_size; ++i)
            MDEBUG("Hashing " << curve.to_string(chunk_start[i]));

        const typename C::Point chunk_hash = fcmp::curve_trees::get_new_parent(curve, chunk);

        MDEBUG("chunk_start_idx: " << chunk_start_idx << " , chunk_size: " << chunk_size << " , chunk_hash: " << curve.to_string(chunk_hash));

        const auto actual_bytes = curve.to_bytes(parent);
        const auto expected_bytes = curve.to_bytes(chunk_hash);
        CHECK_AND_ASSERT_MES(actual_bytes == expected_bytes, false, "unexpected hash");

        chunk_start_idx += chunk_size;
    }

    CHECK_AND_ASSERT_THROW_MES(chunk_start_idx == child_scalars.size(), "unexpected ending chunk start idx");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree implementations
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::LastChunks CurveTreesGlobalTree::get_last_chunks()
{
    const auto &leaves    = m_tree.leaves;
    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    // We started with c2 and then alternated, so c2 is the same size or 1 higher than c1
    CHECK_AND_ASSERT_THROW_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
        "unexpected number of curve layers");

    CurveTreesV1::LastChunks last_chunks;

    // Since leaf layer is append-only, we know the next start will be right after all existing leaf tuple
    const std::size_t num_leaf_tuples = leaves.size() * CurveTreesV1::LEAF_TUPLE_SIZE;
    last_chunks.next_start_leaf_index = num_leaf_tuples;

    if (c2_layers.empty())
        return last_chunks;

    auto &c1_last_chunks_out = last_chunks.c1_last_chunks;
    auto &c2_last_chunks_out = last_chunks.c2_last_chunks;

    c1_last_chunks_out.reserve(c1_layers.size());
    c2_last_chunks_out.reserve(c2_layers.size());

    // First push the last leaf chunk data into c2 chunks
    const bool update_last_parent = (num_leaf_tuples % m_curve_trees.m_leaf_layer_chunk_width) > 0;
    auto last_leaf_chunk = get_last_child_layer_chunk<Selene>(
        /*update_last_parent*/ update_last_parent,
        /*parent_layer_size */ c2_layers[0].size(),
        /*last_parent       */ c2_layers[0].back(),
        // Since the leaf layer is append-only, we'll never need access to the last child
        /*last_child        */ m_curve_trees.m_c2.zero_scalar());

    c2_last_chunks_out.push_back(std::move(last_leaf_chunk));

    // If there are no c1 layers, we're done
    if (c1_layers.empty())
        return last_chunks;

    // Next parents will be c1
    bool parent_is_c1 = true;

    // Then get last chunks up until the root
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    while (c1_last_chunks_out.size() < c1_layers.size() || c2_last_chunks_out.size() < c2_layers.size())
    {
        CHECK_AND_ASSERT_THROW_MES(c1_layers.size() > c1_idx, "missing c1 layer");
        CHECK_AND_ASSERT_THROW_MES(c2_layers.size() > c2_idx, "missing c2 layer");

        // TODO: template the below if statement into another function
        if (parent_is_c1)
        {
            const Layer<Selene> &child_layer = c2_layers[c2_idx];
            CHECK_AND_ASSERT_THROW_MES(!child_layer.empty(), "child layer is empty");

            const Layer<Helios> &parent_layer = c1_layers[c1_idx];
            CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "parent layer is empty");

            const auto &last_child = m_curve_trees.m_c2.point_to_cycle_scalar(child_layer.back());

            auto last_parent_chunk = get_last_child_layer_chunk<Helios>(update_last_parent,
                parent_layer.size(),
                parent_layer.back(),
                last_child);

            c1_last_chunks_out.push_back(std::move(last_parent_chunk));

            ++c2_idx;
        }
        else
        {
            const Layer<Helios> &child_layer = c1_layers[c1_idx];
            CHECK_AND_ASSERT_THROW_MES(!child_layer.empty(), "child layer is empty");

            const Layer<Selene> &parent_layer = c2_layers[c2_idx];
            CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "parent layer is empty");

            const auto &last_child = m_curve_trees.m_c1.point_to_cycle_scalar(child_layer.back());

            auto last_parent_chunk = get_last_child_layer_chunk<Selene>(update_last_parent,
                parent_layer.size(),
                parent_layer.back(),
                last_child);

            c2_last_chunks_out.push_back(std::move(last_parent_chunk));

            ++c1_idx;
        }

        // Alternate curves every iteration
        parent_is_c1 = !parent_is_c1;
    }

    CHECK_AND_ASSERT_THROW_MES(c1_last_chunks_out.size() == c1_layers.size(), "unexpected c1 last chunks");
    CHECK_AND_ASSERT_THROW_MES(c2_last_chunks_out.size() == c2_layers.size(), "unexpected c2 last chunks");

    return last_chunks;
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::extend_tree(const CurveTreesV1::TreeExtension &tree_extension)
{
    // Add the leaves
    const std::size_t init_num_leaves = m_tree.leaves.size() * m_curve_trees.LEAF_TUPLE_SIZE;
    CHECK_AND_ASSERT_THROW_MES(init_num_leaves == tree_extension.leaves.start_idx,
        "unexpected leaf start idx");

    m_tree.leaves.reserve(m_tree.leaves.size() + tree_extension.leaves.tuples.size());
    for (const auto &leaf : tree_extension.leaves.tuples)
    {
        m_tree.leaves.emplace_back(CurveTreesV1::LeafTuple{
            .O_x = leaf.O_x,
            .I_x = leaf.I_x,
            .C_x = leaf.C_x
        });
    }

    // Add the layers
    const auto &c2_extensions = tree_extension.c2_layer_extensions;
    const auto &c1_extensions = tree_extension.c1_layer_extensions;
    CHECK_AND_ASSERT_THROW_MES(!c2_extensions.empty(), "empty c2 extensions");

    bool use_c2 = true;
    std::size_t c2_idx = 0;
    std::size_t c1_idx = 0;
    for (std::size_t i = 0; i < (c2_extensions.size() + c1_extensions.size()); ++i)
    {
        // TODO: template below if statement
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_extensions.size(), "unexpected c2 layer extension");
            const fcmp::curve_trees::LayerExtension<Selene> &c2_ext = c2_extensions[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(!c2_ext.hashes.empty(), "empty c2 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c2_idx <= m_tree.c2_layers.size(), "missing c2 layer");
            if (m_tree.c2_layers.size() == c2_idx)
                m_tree.c2_layers.emplace_back(Layer<Selene>{});

            auto &c2_inout = m_tree.c2_layers[c2_idx];

            const bool started_after_tip = (c2_inout.size() == c2_ext.start_idx);
            const bool started_at_tip    = (c2_inout.size() == (c2_ext.start_idx + 1));
            CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c2 layer start");

            // We updated the last hash
            if (started_at_tip)
                c2_inout.back() = c2_ext.hashes.front();

            for (std::size_t i = started_at_tip ? 1 : 0; i < c2_ext.hashes.size(); ++i)
                c2_inout.emplace_back(c2_ext.hashes[i]);

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer extension");
            const fcmp::curve_trees::LayerExtension<Helios> &c1_ext = c1_extensions[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(!c1_ext.hashes.empty(), "empty c1 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c1_idx <= m_tree.c1_layers.size(), "missing c1 layer");
            if (m_tree.c1_layers.size() == c1_idx)
                m_tree.c1_layers.emplace_back(Layer<Helios>{});

            auto &c1_inout = m_tree.c1_layers[c1_idx];

            const bool started_after_tip = (c1_inout.size() == c1_ext.start_idx);
            const bool started_at_tip    = (c1_inout.size() == (c1_ext.start_idx + 1));
            CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c1 layer start");

            // We updated the last hash
            if (started_at_tip)
                c1_inout.back() = c1_ext.hashes.front();

            for (std::size_t i = started_at_tip ? 1 : 0; i < c1_ext.hashes.size(); ++i)
                c1_inout.emplace_back(c1_ext.hashes[i]);

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
// If we reached the new root, then clear all remaining elements in the tree above the root. Otherwise continue
template <typename C>
static bool handle_root_after_trim(const std::size_t num_parents,
    const std::size_t c1_expected_n_layers,
    const std::size_t c2_expected_n_layers,
    CurveTreesGlobalTree::Layer<C> &parents_inout,
    std::vector<CurveTreesGlobalTree::Layer<Helios>> &c1_layers_inout,
    std::vector<CurveTreesGlobalTree::Layer<Selene>> &c2_layers_inout)
{
    // We're at the root if there should only be 1 element in the layer
    if (num_parents > 1)
        return false;

    MDEBUG("We have encountered the root, clearing remaining elements in the tree");

    // Clear all parents after root
    while (parents_inout.size() > 1)
        parents_inout.pop_back();

    // Clear all remaining layers, if any
    while (c1_layers_inout.size() > c1_expected_n_layers)
        c1_layers_inout.pop_back();

    while (c2_layers_inout.size() > c2_expected_n_layers)
        c2_layers_inout.pop_back();

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
// Trims the child layer and caches values needed to update and trim the child's parent layer
// TODO: work on consolidating this function with the leaf layer logic and simplifying edge case handling
template <typename C_CHILD, typename C_PARENT>
static typename C_PARENT::Point trim_children(const C_CHILD &c_child,
    const C_PARENT &c_parent,
    const std::size_t parent_width,
    const CurveTreesGlobalTree::Layer<C_PARENT> &parents,
    const typename C_CHILD::Point &old_last_child_hash,
    CurveTreesGlobalTree::Layer<C_CHILD> &children_inout,
    std::size_t &last_parent_idx_inout,
    typename C_PARENT::Point &old_last_parent_hash_out)
{
    const std::size_t old_num_children    = children_inout.size();
    const std::size_t old_last_parent_idx = (old_num_children - 1) / parent_width;
    const std::size_t old_last_offset     = old_num_children % parent_width;

    const std::size_t new_num_children    = last_parent_idx_inout + 1;
    const std::size_t new_last_parent_idx = (new_num_children - 1) / parent_width;
    const std::size_t new_last_offset     = new_num_children % parent_width;

    CHECK_AND_ASSERT_THROW_MES(old_num_children >= new_num_children, "unexpected new_num_children");

    last_parent_idx_inout = new_last_parent_idx;
    old_last_parent_hash_out = parents[new_last_parent_idx];

    MDEBUG("old_num_children: "         << old_num_children <<
        " , old_last_parent_idx: "      << old_last_parent_idx <<
        " , old_last_offset: "          << old_last_offset <<
        " , old_last_parent_hash_out: " << c_parent.to_string(old_last_parent_hash_out) <<
        " , new_num_children: "         << new_num_children <<
        " , new_last_parent_idx: "      << new_last_parent_idx <<
        " , new_last_offset: "          << new_last_offset);

    // TODO: consolidate logic handling this function with the edge case at the end of this function
    if (old_num_children == new_num_children)
    {
        // No new children means we only updated the last child, so use it to get the new last parent
        const auto new_last_child = c_child.point_to_cycle_scalar(children_inout.back());
        std::vector<typename C_PARENT::Scalar> new_child_v{new_last_child};
        const auto &chunk = typename C_PARENT::Chunk{new_child_v.data(), new_child_v.size()};

        const auto new_last_parent = c_parent.hash_grow(
            /*existing_hash*/            old_last_parent_hash_out,
            /*offset*/                   (new_num_children - 1) % parent_width,
            /*first_child_after_offset*/ c_child.point_to_cycle_scalar(old_last_child_hash),
            /*children*/                 chunk);

        MDEBUG("New last parent using updated last child " << c_parent.to_string(new_last_parent));
        return new_last_parent;
    }

    // Get the number of existing children in what will become the new last chunk after trimming
    const std::size_t new_last_chunk_old_num_children = (old_last_parent_idx > new_last_parent_idx
        || old_last_offset == 0)
            ? parent_width
            : old_last_offset;

    CHECK_AND_ASSERT_THROW_MES(new_last_chunk_old_num_children > new_last_offset,
        "unexpected new_last_chunk_old_num_children");

    // Get the number of children we'll be trimming from the new last chunk
    const std::size_t trim_n_children_from_new_last_chunk = new_last_offset == 0
        ? 0 // it wil remain full
        : new_last_chunk_old_num_children - new_last_offset;

    // We use hash trim if we're removing fewer elems in the last chunk than the number of elems remaining
    const bool last_chunk_use_hash_trim = trim_n_children_from_new_last_chunk > 0
        && trim_n_children_from_new_last_chunk < new_last_offset;

    MDEBUG("new_last_chunk_old_num_children: "     << new_last_chunk_old_num_children <<
        " , trim_n_children_from_new_last_chunk: " << trim_n_children_from_new_last_chunk <<
        " , last_chunk_use_hash_trim: "            << last_chunk_use_hash_trim);

    // If we're using hash_trim for the last chunk, we'll need to collect the children we're removing
    // TODO: use a separate function to handle last_chunk_use_hash_trim case
    std::vector<typename C_CHILD::Point> new_last_chunk_children_to_trim;
    if (last_chunk_use_hash_trim)
        new_last_chunk_children_to_trim.reserve(trim_n_children_from_new_last_chunk);

    // Trim the children starting at the back of the child layer
    MDEBUG("Trimming " << (old_num_children - new_num_children) << " children");
    while (children_inout.size() > new_num_children)
    {
        // If we're using hash_trim for the last chunk, collect children from the last chunk
        if (last_chunk_use_hash_trim)
        {
            const std::size_t cur_last_parent_idx = (children_inout.size() - 1) / parent_width;
            if (cur_last_parent_idx == new_last_parent_idx)
                new_last_chunk_children_to_trim.emplace_back(std::move(children_inout.back()));
        }

        children_inout.pop_back();
    }
    CHECK_AND_ASSERT_THROW_MES(children_inout.size() == new_num_children, "unexpected new children");
    // We're done trimming the children

    // If we're not using hash_trim for the last chunk, and we will be trimming from the new last chunk, then
    // we'll need to collect the new last chunk's remaining children for hash_grow
    // TODO: use a separate function to handle last_chunk_remaining_children case
    std::vector<typename C_CHILD::Point> last_chunk_remaining_children;
    if (!last_chunk_use_hash_trim && new_last_offset > 0)
    {
        last_chunk_remaining_children.reserve(new_last_offset);

        const std::size_t start_child_idx = new_last_parent_idx * parent_width;

        CHECK_AND_ASSERT_THROW_MES((start_child_idx + new_last_offset) == children_inout.size(),
            "unexpected start_child_idx");

        for (std::size_t i = start_child_idx; i < children_inout.size(); ++i)
        {
            CHECK_AND_ASSERT_THROW_MES(i < children_inout.size(), "unexpected child idx");
            last_chunk_remaining_children.push_back(children_inout[i]);
        }
    }

    CHECK_AND_ASSERT_THROW_MES(!parents.empty(), "empty parent layer");
    CHECK_AND_ASSERT_THROW_MES(new_last_parent_idx < parents.size(), "unexpected new_last_parent_idx");

    // Set the new last chunk's parent hash
    if (last_chunk_use_hash_trim)
    {
        CHECK_AND_ASSERT_THROW_MES(new_last_chunk_children_to_trim.size() == trim_n_children_from_new_last_chunk,
            "unexpected size of last child chunk");

        // We need to reverse the order in order to match the order the children were initially inserted into the tree
        std::reverse(new_last_chunk_children_to_trim.begin(), new_last_chunk_children_to_trim.end());

        // Check if the last child changed
        const auto &old_last_child = old_last_child_hash;
        const auto &new_last_child = children_inout.back();

        if (c_child.to_bytes(old_last_child) == c_child.to_bytes(new_last_child))
        {
            // If the last child didn't change, then simply trim the collected children
            std::vector<typename C_PARENT::Scalar> child_scalars;
            fcmp::tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child,
                new_last_chunk_children_to_trim,
                child_scalars);

            for (std::size_t i = 0; i < child_scalars.size(); ++i)
                MDEBUG("Trimming child " << c_parent.to_string(child_scalars[i]));

            const auto &chunk = typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()};

            const auto new_last_parent = c_parent.hash_trim(
                old_last_parent_hash_out,
                new_last_offset,
                chunk);

            MDEBUG("New last parent using simple hash_trim " << c_parent.to_string(new_last_parent));
            return new_last_parent;
        }

        // The last child changed, so trim the old child, then grow the chunk by 1 with the new child
        // TODO: implement prior_child_at_offset in hash_trim
        new_last_chunk_children_to_trim.insert(new_last_chunk_children_to_trim.begin(), old_last_child);

        std::vector<typename C_PARENT::Scalar> child_scalars;
        fcmp::tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child,
            new_last_chunk_children_to_trim,
            child_scalars);

        for (std::size_t i = 0; i < child_scalars.size(); ++i)
            MDEBUG("Trimming child " << c_parent.to_string(child_scalars[i]));

        const auto &chunk = typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()};

        CHECK_AND_ASSERT_THROW_MES(new_last_offset > 0, "new_last_offset must be >0");
        auto new_last_parent = c_parent.hash_trim(
            old_last_parent_hash_out,
            new_last_offset - 1,
            chunk);

        std::vector<typename C_PARENT::Scalar> new_last_child_scalar{c_child.point_to_cycle_scalar(new_last_child)};
        const auto &new_last_child_chunk = typename C_PARENT::Chunk{
            new_last_child_scalar.data(),
            new_last_child_scalar.size()};

        MDEBUG("Growing with new child: " << c_parent.to_string(new_last_child_scalar[0]));

        new_last_parent = c_parent.hash_grow(
            new_last_parent,
            new_last_offset - 1,
            c_parent.zero_scalar(),
            new_last_child_chunk);

        MDEBUG("New last parent using hash_trim AND updated last child " << c_parent.to_string(new_last_parent));
        return new_last_parent;
    }
    else if (!last_chunk_remaining_children.empty())
    {
        // If we have reamining children in the new last chunk, and some children were trimmed from the chunk, then
        // use hash_grow to calculate the new hash
        std::vector<typename C_PARENT::Scalar> child_scalars;
        fcmp::tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child,
            last_chunk_remaining_children,
            child_scalars);

        const auto &chunk = typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()};

        auto new_last_parent = c_parent.hash_grow(
            /*existing_hash*/            c_parent.m_hash_init_point,
            /*offset*/                   0,
            /*first_child_after_offset*/ c_parent.zero_scalar(),
            /*children*/                 chunk);

        MDEBUG("New last parent from re-growing last chunk " << c_parent.to_string(new_last_parent));
        return new_last_parent;
    }

    // Check if the last child updated
    const auto &old_last_child = old_last_child_hash;
    const auto &new_last_child = children_inout.back();
    const auto old_last_child_bytes = c_child.to_bytes(old_last_child);
    const auto new_last_child_bytes = c_child.to_bytes(new_last_child);

    if (old_last_child_bytes == new_last_child_bytes)
    {
        MDEBUG("The last child didn't update, nothing left to do");
        return old_last_parent_hash_out;
    }

    // TODO: try to consolidate handling this edge case with the case of old_num_children == new_num_children
    MDEBUG("The last child changed, updating last chunk parent hash");

    CHECK_AND_ASSERT_THROW_MES(new_last_offset == 0, "unexpected new last offset");

    const auto old_last_child_scalar = c_child.point_to_cycle_scalar(old_last_child);
    auto new_last_child_scalar = c_child.point_to_cycle_scalar(new_last_child);

    std::vector<typename C_PARENT::Scalar> child_scalars{std::move(new_last_child_scalar)};
    const auto &chunk = typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()};

    auto new_last_parent = c_parent.hash_grow(
        /*existing_hash*/            old_last_parent_hash_out,
        /*offset*/                   parent_width - 1,
        /*first_child_after_offset*/ old_last_child_scalar,
        /*children*/                 chunk);

    MDEBUG("New last parent from updated last child " << c_parent.to_string(new_last_parent));
    return new_last_parent;
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::trim_tree(const std::size_t new_num_leaves)
{
    // TODO: consolidate below logic with trim_children above
    CHECK_AND_ASSERT_THROW_MES(new_num_leaves >= CurveTreesV1::LEAF_TUPLE_SIZE,
        "tree must have at least 1 leaf tuple in it");
    CHECK_AND_ASSERT_THROW_MES(new_num_leaves % CurveTreesV1::LEAF_TUPLE_SIZE == 0,
        "num leaves must be divisible by leaf tuple size");

    auto &leaves_out = m_tree.leaves;
    auto &c1_layers_out = m_tree.c1_layers;
    auto &c2_layers_out = m_tree.c2_layers;

    const std::size_t old_num_leaves = leaves_out.size() * CurveTreesV1::LEAF_TUPLE_SIZE;
    CHECK_AND_ASSERT_THROW_MES(old_num_leaves > new_num_leaves, "unexpected new num leaves");

    const std::size_t old_last_leaf_parent_idx = (old_num_leaves - CurveTreesV1::LEAF_TUPLE_SIZE)
        / m_curve_trees.m_leaf_layer_chunk_width;
    const std::size_t old_last_leaf_offset = old_num_leaves % m_curve_trees.m_leaf_layer_chunk_width;

    const std::size_t new_last_leaf_parent_idx = (new_num_leaves - CurveTreesV1::LEAF_TUPLE_SIZE)
        / m_curve_trees.m_leaf_layer_chunk_width;
    const std::size_t new_last_leaf_offset = new_num_leaves % m_curve_trees.m_leaf_layer_chunk_width;

    MDEBUG("old_num_leaves: "          << old_num_leaves <<
        ", old_last_leaf_parent_idx: " << old_last_leaf_parent_idx <<
        ", old_last_leaf_offset: "     << old_last_leaf_offset <<
        ", new_num_leaves: "           << new_num_leaves <<
        ", new_last_leaf_parent_idx: " << new_last_leaf_parent_idx <<
        ", new_last_leaf_offset: "     << new_last_leaf_offset);

    // Get the number of existing leaves in what will become the new last chunk after trimming
    const std::size_t new_last_chunk_old_num_leaves = (old_last_leaf_parent_idx > new_last_leaf_parent_idx
        || old_last_leaf_offset == 0)
            ? m_curve_trees.m_leaf_layer_chunk_width
            : old_last_leaf_offset;

    CHECK_AND_ASSERT_THROW_MES(new_last_chunk_old_num_leaves > new_last_leaf_offset,
        "unexpected last_chunk_old_num_leaves");

    // Get the number of leaves we'll be trimming from the new last chunk
    const std::size_t n_leaves_trim_from_new_last_chunk = new_last_leaf_offset == 0
        ? 0 // the last chunk wil remain full
        : new_last_chunk_old_num_leaves - new_last_leaf_offset;

    // We use hash trim if we're removing fewer elems in the last chunk than the number of elems remaining
    const bool last_chunk_use_hash_trim = n_leaves_trim_from_new_last_chunk > 0
        && n_leaves_trim_from_new_last_chunk < new_last_leaf_offset;

    MDEBUG("new_last_chunk_old_num_leaves: "    << new_last_chunk_old_num_leaves <<
        ", n_leaves_trim_from_new_last_chunk: " << n_leaves_trim_from_new_last_chunk <<
        ", last_chunk_use_hash_trim: "          << last_chunk_use_hash_trim);

    // If we're using hash_trim for the last chunk, we'll need to collect the leaves we're trimming from that chunk
    std::vector<Selene::Scalar> new_last_chunk_leaves_to_trim;
    if (last_chunk_use_hash_trim)
        new_last_chunk_leaves_to_trim.reserve(n_leaves_trim_from_new_last_chunk);

    // Trim the leaves starting at the back of the leaf layer
    const std::size_t new_num_leaf_tuples = new_num_leaves / CurveTreesV1::LEAF_TUPLE_SIZE;
    while (leaves_out.size() > new_num_leaf_tuples)
    {
        // If we're using hash_trim for the last chunk, collect leaves from the last chunk to use later
        if (last_chunk_use_hash_trim)
        {
            // Check if we're now trimming leaves from what will be the new last chunk
            const std::size_t num_leaves_remaining = (leaves_out.size() - 1) * CurveTreesV1::LEAF_TUPLE_SIZE;
            const std::size_t cur_last_leaf_parent_idx = num_leaves_remaining / m_curve_trees.m_leaf_layer_chunk_width;

            if (cur_last_leaf_parent_idx == new_last_leaf_parent_idx)
            {
                // Add leaves in reverse order, because we're going to reverse the entire vector later on to get the
                // correct trim order
                new_last_chunk_leaves_to_trim.emplace_back(std::move(leaves_out.back().C_x));
                new_last_chunk_leaves_to_trim.emplace_back(std::move(leaves_out.back().I_x));
                new_last_chunk_leaves_to_trim.emplace_back(std::move(leaves_out.back().O_x));
            }
        }

        leaves_out.pop_back();
    }
    CHECK_AND_ASSERT_THROW_MES(leaves_out.size() == new_num_leaf_tuples, "unexpected size of new leaves");

    const std::size_t cur_last_leaf_parent_idx = ((leaves_out.size() - 1) * CurveTreesV1::LEAF_TUPLE_SIZE)
        / m_curve_trees.m_leaf_layer_chunk_width;
    CHECK_AND_ASSERT_THROW_MES(cur_last_leaf_parent_idx == new_last_leaf_parent_idx, "unexpected last leaf parent idx");

    // If we're not using hash_trim for the last chunk, and the new last chunk is not full already, we'll need to
    // collect the existing leaves to get the hash using hash_grow
    std::vector<Selene::Scalar> last_chunk_remaining_leaves;
    if (!last_chunk_use_hash_trim && new_last_leaf_offset > 0)
    {
        last_chunk_remaining_leaves.reserve(new_last_leaf_offset);

        const std::size_t start_leaf_idx = new_last_leaf_parent_idx * m_curve_trees.m_leaf_layer_chunk_width;
        MDEBUG("start_leaf_idx: " << start_leaf_idx << ", leaves_out.size(): " << leaves_out.size());

        CHECK_AND_ASSERT_THROW_MES((start_leaf_idx + new_last_leaf_offset) == new_num_leaves,
            "unexpected start_leaf_idx");

        for (std::size_t i = (start_leaf_idx / CurveTreesV1::LEAF_TUPLE_SIZE); i < leaves_out.size(); ++i)
        {
            CHECK_AND_ASSERT_THROW_MES(i < leaves_out.size(), "unexpected leaf idx");
            last_chunk_remaining_leaves.push_back(leaves_out[i].O_x);
            last_chunk_remaining_leaves.push_back(leaves_out[i].I_x);
            last_chunk_remaining_leaves.push_back(leaves_out[i].C_x);
        }
    }

    CHECK_AND_ASSERT_THROW_MES(!c2_layers_out.empty(), "empty leaf parent layer");
    CHECK_AND_ASSERT_THROW_MES(cur_last_leaf_parent_idx < c2_layers_out[0].size(),
        "unexpected cur_last_leaf_parent_idx");

    // Set the new last leaf parent
    Selene::Point old_last_c2_hash = std::move(c2_layers_out[0][cur_last_leaf_parent_idx]);
    if (last_chunk_use_hash_trim)
    {
        CHECK_AND_ASSERT_THROW_MES(new_last_chunk_leaves_to_trim.size() == n_leaves_trim_from_new_last_chunk,
            "unexpected size of last leaf chunk");

        // We need to reverse the order in order to match the order the leaves were initially inserted into the tree
        std::reverse(new_last_chunk_leaves_to_trim.begin(), new_last_chunk_leaves_to_trim.end());

        const Selene::Chunk trim_leaves{new_last_chunk_leaves_to_trim.data(), new_last_chunk_leaves_to_trim.size()};

        for (std::size_t i = 0; i < new_last_chunk_leaves_to_trim.size(); ++i)
            MDEBUG("Trimming leaf " << m_curve_trees.m_c2.to_string(new_last_chunk_leaves_to_trim[i]));

        auto new_last_leaf_parent = m_curve_trees.m_c2.hash_trim(
            old_last_c2_hash,
            new_last_leaf_offset,
            trim_leaves);

        MDEBUG("New hash " << m_curve_trees.m_c2.to_string(new_last_leaf_parent));

        c2_layers_out[0][cur_last_leaf_parent_idx] = std::move(new_last_leaf_parent);
    }
    else if (new_last_leaf_offset > 0)
    {
        for (std::size_t i = 0; i < last_chunk_remaining_leaves.size(); ++i)
            MDEBUG("Hashing leaf " << m_curve_trees.m_c2.to_string(last_chunk_remaining_leaves[i]));

        const auto &leaves = Selene::Chunk{last_chunk_remaining_leaves.data(), last_chunk_remaining_leaves.size()};

        auto new_last_leaf_parent = m_curve_trees.m_c2.hash_grow(
            /*existing_hash*/            m_curve_trees.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*first_child_after_offset*/ m_curve_trees.m_c2.zero_scalar(),
            /*children*/                 leaves);

        MDEBUG("Result hash " << m_curve_trees.m_c2.to_string(new_last_leaf_parent));

        c2_layers_out[0][cur_last_leaf_parent_idx] = std::move(new_last_leaf_parent);
    }

    if (handle_root_after_trim<Selene>(
        /*num_parents*/          cur_last_leaf_parent_idx + 1,
        /*c1_expected_n_layers*/ 0,
        /*c2_expected_n_layers*/ 1,
        /*parents_inout*/        c2_layers_out[0],
        /*c1_layers_inout*/      c1_layers_out,
        /*c2_layers_inout*/      c2_layers_out))
    {
        return;
    }

    // Go layer-by-layer starting by trimming the c2 layer we just set, and updating the parent layer hashes
    bool trim_c1 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    std::size_t last_parent_idx = cur_last_leaf_parent_idx;
    Helios::Point old_last_c1_hash;
    for (std::size_t i = 0; i < (c1_layers_out.size() + c2_layers_out.size()); ++i)
    {
        MDEBUG("Trimming layer " << i);

        CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers_out.size(), "unexpected c1 layer");
        CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers_out.size(), "unexpected c2 layer");

        auto &c1_layer_out = c1_layers_out[c1_idx];
        auto &c2_layer_out = c2_layers_out[c2_idx];

        if (trim_c1)
        {
            // TODO: fewer params
            auto new_last_parent = trim_children(m_curve_trees.m_c2,
                m_curve_trees.m_c1,
                m_curve_trees.m_c1_width,
                c1_layer_out,
                old_last_c2_hash,
                c2_layer_out,
                last_parent_idx,
                old_last_c1_hash);

            // Update the last parent
            c1_layer_out[last_parent_idx] = std::move(new_last_parent);

            if (handle_root_after_trim<Helios>(last_parent_idx + 1,
                c1_idx + 1,
                c2_idx + 1,
                c1_layer_out,
                c1_layers_out,
                c2_layers_out))
            {
                return;
            }

            ++c2_idx;
        }
        else
        {
            // TODO: fewer params
            auto new_last_parent = trim_children(m_curve_trees.m_c1,
                m_curve_trees.m_c2,
                m_curve_trees.m_c2_width,
                c2_layer_out,
                old_last_c1_hash,
                c1_layer_out,
                last_parent_idx,
                old_last_c2_hash);

            // Update the last parent
            c2_layer_out[last_parent_idx] = std::move(new_last_parent);

            if (handle_root_after_trim<Selene>(last_parent_idx + 1,
                c1_idx + 1,
                c2_idx + 1,
                c2_layer_out,
                c1_layers_out,
                c2_layers_out))
            {
                return;
            }

            ++c1_idx;
        }

        trim_c1 = !trim_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
bool CurveTreesGlobalTree::audit_tree()
{
    const auto &leaves = m_tree.leaves;
    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    CHECK_AND_ASSERT_MES(!leaves.empty(), false, "must have at least 1 leaf in tree");
    CHECK_AND_ASSERT_MES(!c2_layers.empty(), false, "must have at least 1 c2 layer in tree");
    CHECK_AND_ASSERT_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
        false, "unexpected mismatch of c2 and c1 layers");

    // Verify root has 1 member in it
    const bool c2_is_root = c2_layers.size() > c1_layers.size();
    CHECK_AND_ASSERT_MES(c2_is_root ? c2_layers.back().size() == 1 : c1_layers.back().size() == 1, false,
        "root must have 1 member in it");

    // Iterate from root down to layer above leaves, and check hashes match up correctly
    bool parent_is_c2 = c2_is_root;
    std::size_t c2_idx = c2_layers.size() - 1;
    std::size_t c1_idx = c1_layers.empty() ? 0 : (c1_layers.size() - 1);
    for (std::size_t i = 1; i < (c2_layers.size() + c1_layers.size()); ++i)
    {
        // TODO: implement templated function for below if statement
        if (parent_is_c2)
        {
            MDEBUG("Validating parent c2 layer " << c2_idx << " , child c1 layer " << c1_idx);

            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");

            const Layer<Selene> &parents  = c2_layers[c2_idx];
            const Layer<Helios> &children = c1_layers[c1_idx];

            CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c2_idx " + std::to_string(c2_idx));
            CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c1_idx " + std::to_string(c1_idx));

            std::vector<Selene::Scalar> child_scalars;
            fcmp::tower_cycle::extend_scalars_from_cycle_points<Helios, Selene>(m_curve_trees.m_c1,
                children,
                child_scalars);

            const bool valid = validate_layer<Selene>(m_curve_trees.m_c2,
                parents,
                child_scalars,
                m_curve_trees.m_c2_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c2_idx " + std::to_string(c2_idx));

            --c2_idx;
        }
        else
        {
            MDEBUG("Validating parent c1 layer " << c1_idx << " , child c2 layer " << c2_idx);

            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");

            const Layer<Helios> &parents  = c1_layers[c1_idx];
            const Layer<Selene> &children = c2_layers[c2_idx];

            CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c1_idx " + std::to_string(c1_idx));
            CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c2_idx " + std::to_string(c2_idx));

            std::vector<Helios::Scalar> child_scalars;
            fcmp::tower_cycle::extend_scalars_from_cycle_points<Selene, Helios>(m_curve_trees.m_c2,
                children,
                child_scalars);

            const bool valid = validate_layer<Helios>(
                m_curve_trees.m_c1,
                parents,
                child_scalars,
                m_curve_trees.m_c1_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c1_idx " + std::to_string(c1_idx));

            --c1_idx;
        }

        parent_is_c2 = !parent_is_c2;
    }

    MDEBUG("Validating leaves");

    // Now validate leaves
    return validate_layer<Selene>(m_curve_trees.m_c2,
        c2_layers[0],
        m_curve_trees.flatten_leaves(leaves),
        m_curve_trees.m_leaf_layer_chunk_width);
}
//----------------------------------------------------------------------------------------------------------------------
// Logging helpers
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_last_chunks(const CurveTreesV1::LastChunks &last_chunks)
{
    const auto &c1_last_chunks = last_chunks.c1_last_chunks;
    const auto &c2_last_chunks = last_chunks.c2_last_chunks;

    MDEBUG("Total of " << c1_last_chunks.size() << " Helios last chunks and "
        << c2_last_chunks.size() << " Selene last chunks");

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (c1_last_chunks.size() + c2_last_chunks.size()); ++i)
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_last_chunks.size(), "unexpected c2 layer");

            const fcmp::curve_trees::LastChunkData<Selene> &last_chunk = c2_last_chunks[c2_idx];

            MDEBUG("next_start_child_chunk_index: " << last_chunk.next_start_child_chunk_index
                << " , last_parent: "               << m_curve_trees.m_c2.to_string(last_chunk.last_parent)
                << " , update_last_parent: "        << last_chunk.update_last_parent
                << " , last_child: "                << m_curve_trees.m_c2.to_string(last_chunk.last_child));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_last_chunks.size(), "unexpected c1 layer");

            const fcmp::curve_trees::LastChunkData<Helios> &last_chunk = c1_last_chunks[c1_idx];

            MDEBUG("next_start_child_chunk_index: " << last_chunk.next_start_child_chunk_index
                << " , last_parent: "               << m_curve_trees.m_c1.to_string(last_chunk.last_parent)
                << " , update_last_parent: "        << last_chunk.update_last_parent
                << " , last_child: "                << m_curve_trees.m_c1.to_string(last_chunk.last_child));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_tree_extension(const CurveTreesV1::TreeExtension &tree_extension)
{
    const auto &c1_extensions = tree_extension.c1_layer_extensions;
    const auto &c2_extensions = tree_extension.c2_layer_extensions;

    MDEBUG("Tree extension has " << tree_extension.leaves.tuples.size() << " leaves, "
        << c1_extensions.size() << " helios layers, " <<  c2_extensions.size() << " selene layers");

    MDEBUG("Leaf start idx: " << tree_extension.leaves.start_idx);
    for (std::size_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &leaf = tree_extension.leaves.tuples[i];

        const auto O_x = m_curve_trees.m_c2.to_string(leaf.O_x);
        const auto I_x = m_curve_trees.m_c2.to_string(leaf.I_x);
        const auto C_x = m_curve_trees.m_c2.to_string(leaf.C_x);

        MDEBUG("Leaf idx " << ((i*CurveTreesV1::LEAF_TUPLE_SIZE) + tree_extension.leaves.start_idx)
            << " : { O_x: " << O_x << " , I_x: " << I_x << " , C_x: " << C_x << " }");
    }

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (c1_extensions.size() + c2_extensions.size()); ++i)
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_extensions.size(), "unexpected c2 layer");

            const fcmp::curve_trees::LayerExtension<Selene> &c2_layer = c2_extensions[c2_idx];
            MDEBUG("Selene tree extension start idx: " << c2_layer.start_idx);

            for (std::size_t j = 0; j < c2_layer.hashes.size(); ++j)
                MDEBUG("Child chunk start idx: " << (j + c2_layer.start_idx) << " , hash: "
                    << m_curve_trees.m_c2.to_string(c2_layer.hashes[j]));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer");

            const fcmp::curve_trees::LayerExtension<Helios> &c1_layer = c1_extensions[c1_idx];
            MDEBUG("Helios tree extension start idx: " << c1_layer.start_idx);

            for (std::size_t j = 0; j < c1_layer.hashes.size(); ++j)
                MDEBUG("Child chunk start idx: " << (j + c1_layer.start_idx) << " , hash: "
                    << m_curve_trees.m_c1.to_string(c1_layer.hashes[j]));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_tree()
{
    MDEBUG("Tree has " << m_tree.leaves.size() << " leaves, "
        << m_tree.c1_layers.size() << " helios layers, " <<  m_tree.c2_layers.size() << " selene layers");

    for (std::size_t i = 0; i < m_tree.leaves.size(); ++i)
    {
        const auto &leaf = m_tree.leaves[i];

        const auto O_x = m_curve_trees.m_c2.to_string(leaf.O_x);
        const auto I_x = m_curve_trees.m_c2.to_string(leaf.I_x);
        const auto C_x = m_curve_trees.m_c2.to_string(leaf.C_x);

        MDEBUG("Leaf idx " << i << " : { O_x: " << O_x << " , I_x: " << I_x << " , C_x: " << C_x << " }");
    }

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (m_tree.c1_layers.size() + m_tree.c2_layers.size()); ++i)
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < m_tree.c2_layers.size(), "unexpected c2 layer");

            const CurveTreesGlobalTree::Layer<Selene> &c2_layer = m_tree.c2_layers[c2_idx];
            MDEBUG("Selene layer size: " << c2_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c2_layer.size(); ++j)
                MDEBUG("Child chunk start idx: " << j << " , hash: " << m_curve_trees.m_c2.to_string(c2_layer[j]));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < m_tree.c1_layers.size(), "unexpected c1 layer");

            const CurveTreesGlobalTree::Layer<Helios> &c1_layer = m_tree.c1_layers[c1_idx];
            MDEBUG("Helios layer size: " << c1_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c1_layer.size(); ++j)
                MDEBUG("Child chunk start idx: " << j << " , hash: " << m_curve_trees.m_c1.to_string(c1_layer[j]));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test helpers
//----------------------------------------------------------------------------------------------------------------------
const std::vector<CurveTreesV1::LeafTuple> generate_random_leaves(const CurveTreesV1 &curve_trees,
    const std::size_t num_leaves)
{
    std::vector<CurveTreesV1::LeafTuple> tuples;
    tuples.reserve(num_leaves);

    for (std::size_t i = 0; i < num_leaves; ++i)
    {
        // Generate random output tuple
        crypto::secret_key o,c;
        crypto::public_key O,C;
        crypto::generate_keys(O, o, o, false);
        crypto::generate_keys(C, c, c, false);

        auto leaf_tuple = curve_trees.output_to_leaf_tuple(O, C);

        tuples.emplace_back(std::move(leaf_tuple));
    }

    return tuples;
}
//----------------------------------------------------------------------------------------------------------------------
static bool grow_tree(CurveTreesV1 &curve_trees,
    CurveTreesGlobalTree &global_tree,
    const std::size_t num_leaves)
{
    // Get the last chunk from each layer in the tree; empty if tree is empty
    const auto last_chunks = global_tree.get_last_chunks();

    global_tree.log_last_chunks(last_chunks);

    // Get a tree extension object to the existing tree using randomly generated leaves
    // - The tree extension includes all elements we'll need to add to the existing tree when adding the new leaves
    const auto tree_extension = curve_trees.get_tree_extension(last_chunks,
        generate_random_leaves(curve_trees, num_leaves));

    global_tree.log_tree_extension(tree_extension);

    // Use the tree extension to extend the existing tree
    global_tree.extend_tree(tree_extension);

    global_tree.log_tree();

    // Validate tree structure and all hashes
    return global_tree.audit_tree();
}
//----------------------------------------------------------------------------------------------------------------------
static bool grow_tree_in_memory(const std::size_t init_leaves,
    const std::size_t ext_leaves,
    CurveTreesV1 &curve_trees)
{
    LOG_PRINT_L1("Adding " << init_leaves << " leaves to tree in memory, then extending by "
        << ext_leaves << " leaves");

    CurveTreesGlobalTree global_tree(curve_trees);

    // Initialize global tree with `init_leaves`
    MDEBUG("Adding " << init_leaves << " leaves to tree");

    bool res = grow_tree(curve_trees, global_tree, init_leaves);
    CHECK_AND_ASSERT_MES(res, false, "failed to add inital leaves to tree in memory");

    MDEBUG("Successfully added initial " << init_leaves << " leaves to tree in memory");

    // Then extend the global tree by `ext_leaves`
    MDEBUG("Extending tree by " << ext_leaves << " leaves");

    res = grow_tree(curve_trees, global_tree, ext_leaves);
    CHECK_AND_ASSERT_MES(res, false, "failed to extend tree in memory");

    MDEBUG("Successfully extended by " << ext_leaves << " leaves in memory");
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
static bool trim_tree_in_memory(const std::size_t init_leaves,
    const std::size_t trim_leaves,
    CurveTreesGlobalTree &&global_tree)
{
    // Trim the global tree by `trim_leaves`
    LOG_PRINT_L1("Trimming " << trim_leaves << " leaves from tree");

    CHECK_AND_ASSERT_MES(init_leaves > trim_leaves, false, "trimming too many leaves");
    const std::size_t new_num_leaves = init_leaves - trim_leaves;
    global_tree.trim_tree(new_num_leaves * CurveTreesV1::LEAF_TUPLE_SIZE);

    MDEBUG("Finished trimming " << trim_leaves << " leaves from tree");

    global_tree.log_tree();

    bool res = global_tree.audit_tree();
    CHECK_AND_ASSERT_MES(res, false, "failed to trim tree in memory");

    MDEBUG("Successfully trimmed " << trim_leaves << " leaves in memory");
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
static bool grow_tree_db(const std::size_t init_leaves,
    const std::size_t ext_leaves,
    CurveTreesV1 &curve_trees,
    unit_test::BlockchainLMDBTest &test_db)
{
    INIT_BLOCKCHAIN_LMDB_TEST_DB();

    {
        cryptonote::db_wtxn_guard guard(test_db.m_db);

        LOG_PRINT_L1("Adding " << init_leaves << " leaves to db, then extending by " << ext_leaves << " leaves");

        test_db.m_db->grow_tree(curve_trees, generate_random_leaves(curve_trees, init_leaves));
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees), false, "failed to add initial leaves to db");

        MDEBUG("Successfully added initial " << init_leaves << " leaves to db, extending by "
            << ext_leaves << " leaves");

        test_db.m_db->grow_tree(curve_trees, generate_random_leaves(curve_trees, ext_leaves));
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees), false, "failed to extend tree in db");

        MDEBUG("Successfully extended tree in db by " << ext_leaves << " leaves");
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, grow_tree)
{
    Helios helios;
    Selene selene;

    LOG_PRINT_L1("Test grow tree with helios chunk width " << HELIOS_CHUNK_WIDTH
        << ", selene chunk width " << SELENE_CHUNK_WIDTH);

    auto curve_trees = CurveTreesV1(
        helios,
        selene,
        HELIOS_CHUNK_WIDTH,
        SELENE_CHUNK_WIDTH);

    unit_test::BlockchainLMDBTest test_db;

    static_assert(HELIOS_CHUNK_WIDTH > 1, "helios width must be > 1");
    static_assert(SELENE_CHUNK_WIDTH > 1, "selene width must be > 1");

    // Number of leaves for which x number of layers is required
    const std::size_t NEED_1_LAYER  = SELENE_CHUNK_WIDTH;
    const std::size_t NEED_2_LAYERS = NEED_1_LAYER  * HELIOS_CHUNK_WIDTH;
    const std::size_t NEED_3_LAYERS = NEED_2_LAYERS * SELENE_CHUNK_WIDTH;

    const std::vector<std::size_t> N_LEAVES{
        // Basic tests
        1,
        2,

        // Test with number of leaves {-1,0,+1} relative to chunk width boundaries
        NEED_1_LAYER-1,
        NEED_1_LAYER,
        NEED_1_LAYER+1,

        NEED_2_LAYERS-1,
        NEED_2_LAYERS,
        NEED_2_LAYERS+1,

        NEED_3_LAYERS,
    };

    for (const std::size_t init_leaves : N_LEAVES)
    {
        // TODO: init tree once, then extend a copy of that tree

        for (const std::size_t ext_leaves : N_LEAVES)
        {
            // Only test 3rd layer once because it's a huge test
            if (init_leaves > 1 && ext_leaves == NEED_3_LAYERS)
                continue;
            if (ext_leaves > 1 && init_leaves == NEED_3_LAYERS)
                continue;

            ASSERT_TRUE(grow_tree_in_memory(init_leaves, ext_leaves, curve_trees));
            ASSERT_TRUE(grow_tree_db(init_leaves, ext_leaves, curve_trees, test_db));
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, trim_tree)
{
    Helios helios;
    Selene selene;

    LOG_PRINT_L1("Test trim tree with helios chunk width " << HELIOS_CHUNK_WIDTH
        << ", selene chunk width " << SELENE_CHUNK_WIDTH);

    auto curve_trees = CurveTreesV1(
        helios,
        selene,
        HELIOS_CHUNK_WIDTH,
        SELENE_CHUNK_WIDTH);

    unit_test::BlockchainLMDBTest test_db;

    static_assert(HELIOS_CHUNK_WIDTH > 1, "helios width must be > 1");
    static_assert(SELENE_CHUNK_WIDTH > 1, "selene width must be > 1");

    // Number of leaves for which x number of layers is required
    const std::size_t NEED_1_LAYER  = SELENE_CHUNK_WIDTH;
    const std::size_t NEED_2_LAYERS = NEED_1_LAYER  * HELIOS_CHUNK_WIDTH;
    const std::size_t NEED_3_LAYERS = NEED_2_LAYERS * SELENE_CHUNK_WIDTH;

    const std::vector<std::size_t> N_LEAVES{
        // Basic tests
        1,
        2,

        // Test with number of leaves {-1,0,+1} relative to chunk width boundaries
        NEED_1_LAYER-1,
        NEED_1_LAYER,
        NEED_1_LAYER+1,

        NEED_2_LAYERS-1,
        NEED_2_LAYERS,
        NEED_2_LAYERS+1,

        NEED_3_LAYERS-1,
        NEED_3_LAYERS,
        NEED_3_LAYERS+1,
    };

    for (const std::size_t init_leaves : N_LEAVES)
    {
        if (init_leaves == 1)
            continue;

        CurveTreesGlobalTree global_tree(curve_trees);

        // Initialize global tree with `init_leaves`
        LOG_PRINT_L1("Initializing tree with " << init_leaves << " leaves in memory");
        ASSERT_TRUE(grow_tree(curve_trees, global_tree, init_leaves));
        MDEBUG("Successfully added initial " << init_leaves << " leaves to tree in memory");

        for (const std::size_t trim_leaves : N_LEAVES)
        {
            // Can't trim more leaves than exist in tree, and tree must always have at least 1 leaf in it
            if (trim_leaves >= init_leaves)
                continue;

            // Copy the already initialized tree
            CurveTreesGlobalTree tree_copy(global_tree);
            ASSERT_TRUE(trim_tree_in_memory(init_leaves, trim_leaves, std::move(tree_copy)));
        }
    }
}
// TODO: write tests with more layers, but smaller widths so the tests run in a reasonable amount of time
//----------------------------------------------------------------------------------------------------------------------
// Make sure the result of hash_trim is the same as the equivalent hash_grow excluding the trimmed children
TEST(curve_trees, hash_trim)
{
    Helios helios;
    Selene selene;
    auto curve_trees = CurveTreesV1(
        helios,
        selene,
        HELIOS_CHUNK_WIDTH,
        SELENE_CHUNK_WIDTH);

    // Selene
    // Generate 3 random leaf tuples
    const std::size_t NUM_LEAF_TUPLES = 3;
    const std::size_t NUM_LEAVES = NUM_LEAF_TUPLES * CurveTreesV1::LEAF_TUPLE_SIZE;
    const auto grow_leaves = generate_random_leaves(curve_trees, NUM_LEAF_TUPLES);
    const auto grow_children = curve_trees.flatten_leaves(grow_leaves);
    const auto &grow_chunk = Selene::Chunk{grow_children.data(), grow_children.size()};

    // Hash the leaves
    const auto init_grow_result = curve_trees.m_c2.hash_grow(
        /*existing_hash*/            curve_trees.m_c2.m_hash_init_point,
        /*offset*/                   0,
        /*first_child_after_offset*/ curve_trees.m_c2.zero_scalar(),
        /*children*/                 grow_chunk);

    // Trim the initial result
    const std::size_t trim_offset = NUM_LEAVES - CurveTreesV1::LEAF_TUPLE_SIZE;
    const auto &trimmed_child = Selene::Chunk{grow_children.data() + trim_offset, CurveTreesV1::LEAF_TUPLE_SIZE};
    const auto trim_result = curve_trees.m_c2.hash_trim(
        init_grow_result,
        trim_offset,
        trimmed_child);
    const auto trim_res_bytes = curve_trees.m_c2.to_bytes(trim_result);

    // Now compare to calling hash_grow with the remaining children, excluding the trimmed child
    const auto &remaining_children = Selene::Chunk{grow_children.data(), trim_offset};
    const auto remaining_children_hash = curve_trees.m_c2.hash_grow(
        /*existing_hash*/            curve_trees.m_c2.m_hash_init_point,
        /*offset*/                   0,
        /*first_child_after_offset*/ curve_trees.m_c2.zero_scalar(),
        /*children*/                 remaining_children);
    const auto grow_res_bytes = curve_trees.m_c2.to_bytes(remaining_children_hash);

    ASSERT_EQ(trim_res_bytes, grow_res_bytes);

    // Helios
    // Get 2 helios scalars
    std::vector<Helios::Scalar> grow_helios_scalars;
    fcmp::tower_cycle::extend_scalars_from_cycle_points<Selene, Helios>(curve_trees.m_c2,
        {init_grow_result, trim_result},
        grow_helios_scalars);
    const auto &grow_helios_chunk = Helios::Chunk{grow_helios_scalars.data(), grow_helios_scalars.size()};

    // Get the initial hash of the 2 helios scalars
    const auto helios_grow_result = curve_trees.m_c1.hash_grow(
        /*existing_hash*/            curve_trees.m_c1.m_hash_init_point,
        /*offset*/                   0,
        /*first_child_after_offset*/ curve_trees.m_c1.zero_scalar(),
        /*children*/                 grow_helios_chunk);

    // Trim the initial result by 1 child
    const auto &trimmed_helios_child = Helios::Chunk{grow_helios_scalars.data() + 1, 1};
    const auto trim_helios_result = curve_trees.m_c1.hash_trim(
        helios_grow_result,
        1,
        trimmed_helios_child);
    const auto trim_helios_res_bytes = curve_trees.m_c1.to_bytes(trim_helios_result);

    // Now compare to calling hash_grow with the remaining children, excluding the trimmed child
    const auto &remaining_helios_children = Helios::Chunk{grow_helios_scalars.data(), 1};
    const auto remaining_helios_children_hash = curve_trees.m_c1.hash_grow(
        /*existing_hash*/            curve_trees.m_c1.m_hash_init_point,
        /*offset*/                   0,
        /*first_child_after_offset*/ curve_trees.m_c1.zero_scalar(),
        /*children*/                 remaining_helios_children);
    const auto grow_helios_res_bytes = curve_trees.m_c1.to_bytes(remaining_helios_children_hash);

    ASSERT_EQ(trim_helios_res_bytes, grow_helios_res_bytes);
}
