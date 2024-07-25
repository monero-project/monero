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
template<typename C_CHILD, typename C_PARENT>
static std::vector<typename C_PARENT::Scalar> get_last_chunk_children_to_trim(const C_CHILD &c_child,
    const CurveTreesGlobalTree::Layer<C_CHILD> &child_layer,
    const bool need_last_chunk_children_to_trim,
    const bool need_last_chunk_remaining_children,
    const std::size_t start_trim_idx,
    const std::size_t end_trim_idx)
{
    std::vector<typename C_PARENT::Scalar> children_to_trim_out;
    if (end_trim_idx > start_trim_idx)
    {
        std::size_t idx = start_trim_idx;
        MDEBUG("Start trim from idx: " << idx << " , ending trim at: " << end_trim_idx);
        do
        {
            CHECK_AND_ASSERT_THROW_MES(child_layer.size() > idx, "idx too high");
            const auto &child_point = child_layer[idx];

            auto child_scalar = c_child.point_to_cycle_scalar(child_point);
            children_to_trim_out.push_back(std::move(child_scalar));

            ++idx;
        }
        while (idx < end_trim_idx);
    }

    return children_to_trim_out;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree implementations
//----------------------------------------------------------------------------------------------------------------------
std::size_t CurveTreesGlobalTree::get_num_leaf_tuples() const
{
    return m_tree.leaves.size();
}
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::LastHashes CurveTreesGlobalTree::get_last_hashes() const
{
    CurveTreesV1::LastHashes last_hashes_out;
    auto &c1_last_hashes_out = last_hashes_out.c1_last_hashes;
    auto &c2_last_hashes_out = last_hashes_out.c2_last_hashes;

    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    // We started with c2 and then alternated, so c2 is the same size or 1 higher than c1
    CHECK_AND_ASSERT_THROW_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
        "unexpected number of curve layers");

    c1_last_hashes_out.reserve(c1_layers.size());
    c2_last_hashes_out.reserve(c2_layers.size());

    if (c2_layers.empty())
        return last_hashes_out;

    // Next parents will be c2
    bool use_c2 = true;

    // Then get last chunks up until the root
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    while (c1_last_hashes_out.size() < c1_layers.size() || c2_last_hashes_out.size() < c2_layers.size())
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_layers.size() > c2_idx, "missing c2 layer");
            c2_last_hashes_out.push_back(c2_layers[c2_idx].back());
            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_layers.size() > c1_idx, "missing c1 layer");
            c1_last_hashes_out.push_back(c1_layers[c1_idx].back());
            ++c1_idx;
        }

        use_c2 = !use_c2;
    }

    return last_hashes_out;
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::extend_tree(const CurveTreesV1::TreeExtension &tree_extension)
{
    // Add the leaves
    CHECK_AND_ASSERT_THROW_MES(m_tree.leaves.size() == tree_extension.leaves.start_leaf_tuple_idx,
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
            {
                CHECK_AND_ASSERT_THROW_MES(c2_ext.update_existing_last_hash, "expect to be updating last hash");
                c2_inout.back() = c2_ext.hashes.front();
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(!c2_ext.update_existing_last_hash, "unexpected last hash update");
            }

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
            {
                CHECK_AND_ASSERT_THROW_MES(c1_ext.update_existing_last_hash, "expect to be updating last hash");
                c1_inout.back() = c1_ext.hashes.front();
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(!c1_ext.update_existing_last_hash, "unexpected last hash update");
            }

            for (std::size_t i = started_at_tip ? 1 : 0; i < c1_ext.hashes.size(); ++i)
                c1_inout.emplace_back(c1_ext.hashes[i]);

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::reduce_tree(const CurveTreesV1::TreeReduction &tree_reduction)
{
    // Trim the leaves
    CHECK_AND_ASSERT_THROW_MES(m_tree.leaves.size() > tree_reduction.new_total_leaf_tuples,
        "expected fewer new total leaves");
    while (m_tree.leaves.size() > tree_reduction.new_total_leaf_tuples)
        m_tree.leaves.pop_back();

    // Trim the layers
    const auto &c2_layer_reductions = tree_reduction.c2_layer_reductions;
    const auto &c1_layer_reductions = tree_reduction.c1_layer_reductions;
    CHECK_AND_ASSERT_THROW_MES(!c2_layer_reductions.empty(), "empty c2 layer reductions");

    bool use_c2 = true;
    std::size_t c2_idx = 0;
    std::size_t c1_idx = 0;
    for (std::size_t i = 0; i < (c2_layer_reductions.size() + c1_layer_reductions.size()); ++i)
    {
        // TODO: template below if statement
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layer_reductions.size(), "unexpected c2 layer reduction");
            const auto &c2_reduction = c2_layer_reductions[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(c2_idx < m_tree.c2_layers.size(), "missing c2 layer");
            auto &c2_inout = m_tree.c2_layers[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(c2_reduction.new_total_parents <= c2_inout.size(),
                "unexpected c2 new total parents");

            c2_inout.resize(c2_reduction.new_total_parents);
            c2_inout.shrink_to_fit();

            // We updated the last hash
            if (c2_reduction.update_existing_last_hash)
            {
                c2_inout.back() = c2_reduction.new_last_hash;
            }

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layer_reductions.size(), "unexpected c1 layer reduction");
            const auto &c1_reduction = c1_layer_reductions[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(c1_idx < m_tree.c1_layers.size(), "missing c1 layer");
            auto &c1_inout = m_tree.c1_layers[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(c1_reduction.new_total_parents <= c1_inout.size(),
                "unexpected c1 new total parents");

            c1_inout.resize(c1_reduction.new_total_parents);
            c1_inout.shrink_to_fit();

            // We updated the last hash
            if (c1_reduction.update_existing_last_hash)
            {
                c1_inout.back() = c1_reduction.new_last_hash;
            }

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }

    // Delete remaining layers
    m_tree.c1_layers.resize(c1_layer_reductions.size());
    m_tree.c2_layers.resize(c2_layer_reductions.size());

    m_tree.c1_layers.shrink_to_fit();
    m_tree.c2_layers.shrink_to_fit();
}
//----------------------------------------------------------------------------------------------------------------------
// TODO: template
CurveTreesV1::LastChunkChildrenToTrim CurveTreesGlobalTree::get_all_last_chunk_children_to_trim(
    const std::vector<fcmp::curve_trees::TrimLayerInstructions> &trim_instructions)
{
    CurveTreesV1::LastChunkChildrenToTrim all_children_to_trim;

    // Leaf layer
    CHECK_AND_ASSERT_THROW_MES(!trim_instructions.empty(), "no instructions");
    const auto &trim_leaf_layer_instructions = trim_instructions[0];

    std::vector<Selene::Scalar> leaves_to_trim;

    // TODO: separate function
    if (trim_leaf_layer_instructions.end_trim_idx > trim_leaf_layer_instructions.start_trim_idx)
    {
        std::size_t idx = trim_leaf_layer_instructions.start_trim_idx;
        MDEBUG("Start trim from idx: " << idx);
        do
        {
            CHECK_AND_ASSERT_THROW_MES(idx % CurveTreesV1::LEAF_TUPLE_SIZE == 0, "expected divisble by leaf tuple size");
            const std::size_t leaf_tuple_idx = idx / CurveTreesV1::LEAF_TUPLE_SIZE;

            CHECK_AND_ASSERT_THROW_MES(m_tree.leaves.size() > leaf_tuple_idx, "leaf_tuple_idx too high");
            const auto &leaf_tuple = m_tree.leaves[leaf_tuple_idx];

            leaves_to_trim.push_back(leaf_tuple.O_x);
            leaves_to_trim.push_back(leaf_tuple.I_x);
            leaves_to_trim.push_back(leaf_tuple.C_x);

            idx += CurveTreesV1::LEAF_TUPLE_SIZE;
        }
        while (idx < trim_leaf_layer_instructions.end_trim_idx);
    }

    all_children_to_trim.c2_children.emplace_back(std::move(leaves_to_trim));

    bool parent_is_c2 = false;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 1; i < trim_instructions.size(); ++i)
    {
        MDEBUG("Getting trim instructions for layer " << i);

        const auto &trim_layer_instructions = trim_instructions[i];

        const bool need_last_chunk_children_to_trim   = trim_layer_instructions.need_last_chunk_children_to_trim;
        const bool need_last_chunk_remaining_children = trim_layer_instructions.need_last_chunk_remaining_children;
        const std::size_t start_trim_idx              = trim_layer_instructions.start_trim_idx;
        const std::size_t end_trim_idx                = trim_layer_instructions.end_trim_idx;

        if (parent_is_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(m_tree.c1_layers.size() > c1_idx, "c1_idx too high");

            auto children_to_trim = get_last_chunk_children_to_trim<Helios, Selene>(
                m_curve_trees.m_c1,
                m_tree.c1_layers[c1_idx],
                need_last_chunk_children_to_trim,
                need_last_chunk_remaining_children,
                start_trim_idx,
                end_trim_idx);

            all_children_to_trim.c2_children.emplace_back(std::move(children_to_trim));
            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(m_tree.c2_layers.size() > c2_idx, "c2_idx too high");

            auto children_to_trim = get_last_chunk_children_to_trim<Selene, Helios>(
                m_curve_trees.m_c2,
                m_tree.c2_layers[c2_idx],
                need_last_chunk_children_to_trim,
                need_last_chunk_remaining_children,
                start_trim_idx,
                end_trim_idx);

            all_children_to_trim.c1_children.emplace_back(std::move(children_to_trim));
            ++c2_idx;
        }

        parent_is_c2 = !parent_is_c2;
    }

    return all_children_to_trim;
}
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::LastHashes CurveTreesGlobalTree::get_last_hashes_to_trim(
    const std::vector<fcmp::curve_trees::TrimLayerInstructions> &trim_instructions) const
{
    CurveTreesV1::LastHashes last_hashes;
    CHECK_AND_ASSERT_THROW_MES(!trim_instructions.empty(), "no instructions");

    bool parent_is_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (const auto &trim_layer_instructions : trim_instructions)
    {
        const std::size_t new_total_parents = trim_layer_instructions.new_total_parents;
        CHECK_AND_ASSERT_THROW_MES(new_total_parents > 0, "no new parents");

        if (parent_is_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(m_tree.c2_layers.size() > c2_idx, "c2_idx too high");
            const auto &c2_layer = m_tree.c2_layers[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(c2_layer.size() >= new_total_parents, "not enough c2 parents");

            last_hashes.c2_last_hashes.push_back(c2_layer[new_total_parents - 1]);
            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(m_tree.c1_layers.size() > c1_idx, "c1_idx too high");
            const auto &c1_layer = m_tree.c1_layers[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(c1_layer.size() >= new_total_parents, "not enough c1 parents");

            last_hashes.c1_last_hashes.push_back(c1_layer[new_total_parents - 1]);
            ++c1_idx;
        }

        parent_is_c2 = !parent_is_c2;
    }

    return last_hashes;
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::trim_tree(const std::size_t trim_n_leaf_tuples)
{
    const std::size_t old_n_leaf_tuples = this->get_num_leaf_tuples();
    MDEBUG(old_n_leaf_tuples << " leaves in the tree, trimming " << trim_n_leaf_tuples);

    // Get trim instructions
    const auto trim_instructions = m_curve_trees.get_trim_instructions(old_n_leaf_tuples, trim_n_leaf_tuples);
    MDEBUG("Acquired trim instructions for " << trim_instructions.size() << " layers");

    // Do initial tree reads
    const auto last_chunk_children_to_trim = this->get_all_last_chunk_children_to_trim(trim_instructions);
    const auto last_hashes_to_trim = this->get_last_hashes_to_trim(trim_instructions);

    // Get the new hashes, wrapped in a simple struct we can use to trim the tree
    const auto tree_reduction = m_curve_trees.get_tree_reduction(
        trim_instructions,
        last_chunk_children_to_trim,
        last_hashes_to_trim);

    // Use tree reduction to trim tree
    this->reduce_tree(tree_reduction);

    const std::size_t new_n_leaf_tuples = this->get_num_leaf_tuples();
    CHECK_AND_ASSERT_THROW_MES((new_n_leaf_tuples + trim_n_leaf_tuples) == old_n_leaf_tuples,
        "unexpected num leaves after trim");
}
//----------------------------------------------------------------------------------------------------------------------
bool CurveTreesGlobalTree::audit_tree(const std::size_t expected_n_leaf_tuples)
{
    MDEBUG("Auditing global tree");

    const auto &leaves = m_tree.leaves;
    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    CHECK_AND_ASSERT_MES(!leaves.empty(), false, "must have at least 1 leaf in tree");
    CHECK_AND_ASSERT_MES(leaves.size() == expected_n_leaf_tuples, false, "unexpected num leaves");

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
void CurveTreesGlobalTree::log_last_hashes(const CurveTreesV1::LastHashes &last_hashes)
{
    const auto &c1_last_hashes = last_hashes.c1_last_hashes;
    const auto &c2_last_hashes = last_hashes.c2_last_hashes;

    MDEBUG("Total of " << c1_last_hashes.size() << " Helios layers and " << c2_last_hashes.size() << " Selene layers");

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (c1_last_hashes.size() + c2_last_hashes.size()); ++i)
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_last_hashes.size(), "unexpected c2 layer");

            const auto &last_hash = c2_last_hashes[c2_idx];
            MDEBUG("c2_idx: " << c2_idx << " , last_hash: " << m_curve_trees.m_c2.to_string(last_hash));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_last_hashes.size(), "unexpected c1 layer");

            const auto &last_hash = c1_last_hashes[c1_idx];
            MDEBUG("c1_idx: " << c1_idx << " , last_hash: " << m_curve_trees.m_c1.to_string(last_hash));

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

    MDEBUG("Leaf start idx: " << tree_extension.leaves.start_leaf_tuple_idx);
    for (std::size_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &leaf = tree_extension.leaves.tuples[i];

        const auto O_x = m_curve_trees.m_c2.to_string(leaf.O_x);
        const auto I_x = m_curve_trees.m_c2.to_string(leaf.I_x);
        const auto C_x = m_curve_trees.m_c2.to_string(leaf.C_x);

        MDEBUG("Leaf tuple idx " << (tree_extension.leaves.start_leaf_tuple_idx)
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
static const std::vector<fcmp::curve_trees::CurveTreesV1::LeafTupleContext> generate_random_leaves(const CurveTreesV1 &curve_trees,
    const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples)
{
    std::vector<CurveTreesV1::LeafTupleContext> tuples;
    tuples.reserve(new_n_leaf_tuples);

    for (std::size_t i = 0; i < new_n_leaf_tuples; ++i)
    {
        // Generate random output tuple
        crypto::secret_key o,c;
        crypto::public_key O,C;
        crypto::generate_keys(O, o, o, false);
        crypto::generate_keys(C, c, c, false);

        auto leaf_tuple = curve_trees.output_to_leaf_tuple(O, C);

        tuples.emplace_back(fcmp::curve_trees::CurveTreesV1::LeafTupleContext{
                .output_id  = old_n_leaf_tuples + i,
                .leaf_tuple = std::move(leaf_tuple),
            });
    }

    return tuples;
}
//----------------------------------------------------------------------------------------------------------------------
static const Selene::Scalar generate_random_selene_scalar()
{
    crypto::secret_key s;
    crypto::public_key S;

    crypto::generate_keys(S, s, s, false);
    return fcmp::tower_cycle::ed_25519_point_to_scalar(S);
}
//----------------------------------------------------------------------------------------------------------------------
static bool grow_tree(CurveTreesV1 &curve_trees,
    CurveTreesGlobalTree &global_tree,
    const std::size_t new_n_leaf_tuples)
{
    // Do initial tree reads
    const std::size_t old_n_leaf_tuples = global_tree.get_num_leaf_tuples();
    const CurveTreesV1::LastHashes last_hashes = global_tree.get_last_hashes();

    global_tree.log_last_hashes(last_hashes);

    auto new_leaf_tuples = generate_random_leaves(curve_trees, old_n_leaf_tuples, new_n_leaf_tuples);

    // Get a tree extension object to the existing tree using randomly generated leaves
    // - The tree extension includes all elements we'll need to add to the existing tree when adding the new leaves
    const auto tree_extension = curve_trees.get_tree_extension(old_n_leaf_tuples,
        last_hashes,
        std::move(new_leaf_tuples));

    global_tree.log_tree_extension(tree_extension);

    // Use the tree extension to extend the existing tree
    global_tree.extend_tree(tree_extension);

    global_tree.log_tree();

    // Validate tree structure and all hashes
    const std::size_t expected_n_leaf_tuples = old_n_leaf_tuples + new_n_leaf_tuples;
    return global_tree.audit_tree(expected_n_leaf_tuples);
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
static bool trim_tree_in_memory(const std::size_t trim_n_leaf_tuples,
    CurveTreesGlobalTree &&global_tree)
{
    const std::size_t old_n_leaf_tuples = global_tree.get_num_leaf_tuples();
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples > trim_n_leaf_tuples, "cannot trim more leaves than exist");
    CHECK_AND_ASSERT_THROW_MES(trim_n_leaf_tuples > 0, "must be trimming some leaves");

    // Trim the global tree by `trim_n_leaf_tuples`
    LOG_PRINT_L1("Trimming " << trim_n_leaf_tuples << " leaf tuples from tree with "
        << old_n_leaf_tuples << " leaves in memory");

    global_tree.trim_tree(trim_n_leaf_tuples);

    MDEBUG("Finished trimming " << trim_n_leaf_tuples << " leaf tuples from tree");

    global_tree.log_tree();

    const std::size_t expected_n_leaf_tuples = old_n_leaf_tuples - trim_n_leaf_tuples;
    bool res = global_tree.audit_tree(expected_n_leaf_tuples);
    CHECK_AND_ASSERT_MES(res, false, "failed to trim tree in memory");

    MDEBUG("Successfully trimmed " << trim_n_leaf_tuples << " leaves in memory");
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

        auto init_leaf_tuples = generate_random_leaves(curve_trees, 0, init_leaves);

        test_db.m_db->grow_tree(curve_trees, std::move(init_leaf_tuples));
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees, init_leaves), false,
            "failed to add initial leaves to db");

        MDEBUG("Successfully added initial " << init_leaves << " leaves to db, extending by "
            << ext_leaves << " leaves");

        auto ext_leaf_tuples = generate_random_leaves(curve_trees, init_leaves, ext_leaves);

        test_db.m_db->grow_tree(curve_trees, std::move(ext_leaf_tuples));
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees, init_leaves + ext_leaves), false,
            "failed to extend tree in db");

        MDEBUG("Successfully extended tree in db by " << ext_leaves << " leaves");
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
static bool trim_tree_db(const std::size_t init_leaves,
    const std::size_t trim_leaves,
    CurveTreesV1 &curve_trees,
    unit_test::BlockchainLMDBTest &test_db)
{
    INIT_BLOCKCHAIN_LMDB_TEST_DB();

    {
        cryptonote::db_wtxn_guard guard(test_db.m_db);

        LOG_PRINT_L1("Adding " << init_leaves << " leaves to db, then trimming by " << trim_leaves << " leaves");

        auto init_leaf_tuples = generate_random_leaves(curve_trees, 0, init_leaves);

        test_db.m_db->grow_tree(curve_trees, std::move(init_leaf_tuples));
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees, init_leaves), false,
            "failed to add initial leaves to db");

        MDEBUG("Successfully added initial " << init_leaves << " leaves to db, trimming by "
            << trim_leaves << " leaves");

        test_db.m_db->trim_tree(curve_trees, trim_leaves);
        CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(curve_trees, init_leaves - trim_leaves), false,
            "failed to trim tree in db");

        MDEBUG("Successfully trimmed tree in db by " << trim_leaves << " leaves");
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

    // Use lower values for chunk width than prod so that we can quickly test a many-layer deep tree
    static const std::size_t helios_chunk_width = 3;
    static const std::size_t selene_chunk_width = 2;

    static_assert(helios_chunk_width > 1, "helios width must be > 1");
    static_assert(selene_chunk_width > 1, "selene width must be > 1");

    LOG_PRINT_L1("Test grow tree with helios chunk width " << helios_chunk_width
        << ", selene chunk width " << selene_chunk_width);

    // Constant for how deep we want the tree
    static const std::size_t TEST_N_LAYERS = 4;

    // Number of leaves for which x number of layers is required
    std::size_t leaves_needed_for_n_layers = selene_chunk_width;
    for (std::size_t i = 1; i < TEST_N_LAYERS; ++i)
    {
        const std::size_t width = i % 2 == 0 ? selene_chunk_width : helios_chunk_width;
        leaves_needed_for_n_layers *= width;
    }

    auto curve_trees = CurveTreesV1(
        helios,
        selene,
        helios_chunk_width,
        selene_chunk_width);

    unit_test::BlockchainLMDBTest test_db;

    // Increment to test for off-by-1
    ++leaves_needed_for_n_layers;

    // First initialize the tree with init_leaves
    for (std::size_t init_leaves = 1; init_leaves < leaves_needed_for_n_layers; ++init_leaves)
    {
        // TODO: init tree once, then extend a copy of that tree
        // Then extend the tree with ext_leaves
        for (std::size_t ext_leaves = 1; (init_leaves + ext_leaves) <= leaves_needed_for_n_layers; ++ext_leaves)
        {
            ASSERT_TRUE(grow_tree_in_memory(init_leaves, ext_leaves, curve_trees));
            ASSERT_TRUE(grow_tree_db(init_leaves, ext_leaves, curve_trees, test_db));
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, trim_tree)
{
    // TODO: consolidate code from grow_tree test
    Helios helios;
    Selene selene;

    // Use lower values for chunk width than prod so that we can quickly test a many-layer deep tree
    static const std::size_t helios_chunk_width = 3;
    static const std::size_t selene_chunk_width = 3;

    static_assert(helios_chunk_width > 1, "helios width must be > 1");
    static_assert(selene_chunk_width > 1, "selene width must be > 1");

    LOG_PRINT_L1("Test trim tree with helios chunk width " << helios_chunk_width
        << ", selene chunk width " << selene_chunk_width);

    // Constant for how deep we want the tree
    static const std::size_t TEST_N_LAYERS = 4;

    // Number of leaves for which x number of layers is required
    std::size_t leaves_needed_for_n_layers = selene_chunk_width;
    for (std::size_t i = 1; i < TEST_N_LAYERS; ++i)
    {
        const std::size_t width = i % 2 == 0 ? selene_chunk_width : helios_chunk_width;
        leaves_needed_for_n_layers *= width;
    }

    auto curve_trees = CurveTreesV1(
        helios,
        selene,
        helios_chunk_width,
        selene_chunk_width);

    unit_test::BlockchainLMDBTest test_db;

    // Increment to test for off-by-1
    ++leaves_needed_for_n_layers;

    // First initialize the tree with init_leaves
    for (std::size_t init_leaves = 2; init_leaves <= leaves_needed_for_n_layers; ++init_leaves)
    {
        LOG_PRINT_L1("Initializing tree with " << init_leaves << " leaves in memory");
        CurveTreesGlobalTree global_tree(curve_trees);

        ASSERT_TRUE(grow_tree(curve_trees, global_tree, init_leaves));

        // Then extend the tree with ext_leaves
        for (std::size_t trim_leaves = 1; trim_leaves < leaves_needed_for_n_layers; ++trim_leaves)
        {
            if (trim_leaves >= init_leaves)
                continue;

            // Copy the already existing global tree
            CurveTreesGlobalTree tree_copy(global_tree);

            ASSERT_TRUE(trim_tree_in_memory(trim_leaves, std::move(tree_copy)));
            ASSERT_TRUE(trim_tree_db(init_leaves, trim_leaves, curve_trees, test_db));
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
// Make sure the result of hash_trim is the same as the equivalent hash_grow excluding the trimmed children
TEST(curve_trees, hash_trim)
{
    // 1. Trim 1
    {
        // Start by hashing: {selene_scalar_0, selene_scalar_1}
        // Then trim to:     {selene_scalar_0}
        const auto selene_scalar_0 = generate_random_selene_scalar();
        const auto selene_scalar_1 = generate_random_selene_scalar();

        // Get the initial hash of the 2 scalars
        std::vector<Selene::Scalar> init_children{selene_scalar_0, selene_scalar_1};
        const auto init_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{init_children.data(), init_children.size()});

        // Trim selene_scalar_1
        const auto &trimmed_children = Selene::Chunk{init_children.data() + 1, 1};
        const auto trim_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_trim(
            init_hash,
            1,
            trimmed_children,
            fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar());
        const auto trim_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(trim_res);

        // Now compare to calling hash_grow{selene_scalar_0}
        std::vector<Selene::Scalar> remaining_children{selene_scalar_0};
        const auto grow_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{remaining_children.data(), remaining_children.size()});
        const auto grow_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res);

        ASSERT_EQ(trim_res_bytes, grow_res_bytes);
    }

    // 2. Trim 2
    {
        // Start by hashing: {selene_scalar_0, selene_scalar_1, selene_scalar_2}
        // Then trim to:     {selene_scalar_0}
        const auto selene_scalar_0 = generate_random_selene_scalar();
        const auto selene_scalar_1 = generate_random_selene_scalar();
        const auto selene_scalar_2 = generate_random_selene_scalar();

        // Get the initial hash of the 3 selene scalars
        std::vector<Selene::Scalar> init_children{selene_scalar_0, selene_scalar_1, selene_scalar_2};
        const auto init_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{init_children.data(), init_children.size()});

        // Trim the initial result by 2 children
        const auto &trimmed_children = Selene::Chunk{init_children.data() + 1, 2};
        const auto trim_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_trim(
            init_hash,
            1,
            trimmed_children,
            fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar());
        const auto trim_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(trim_res);

        // Now compare to calling hash_grow{selene_scalar_0}
        std::vector<Selene::Scalar> remaining_children{selene_scalar_0};
        const auto grow_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{remaining_children.data(), remaining_children.size()});
        const auto grow_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res);

        ASSERT_EQ(trim_res_bytes, grow_res_bytes);
    }

    // 3. Change 1
    {
        // Start by hashing:  {selene_scalar_0, selene_scalar_1}
        // Then change to:    {selene_scalar_0, selene_scalar_2}
        const auto selene_scalar_0 = generate_random_selene_scalar();
        const auto selene_scalar_1 = generate_random_selene_scalar();

        // Get the initial hash of the 2 selene scalars
        std::vector<Selene::Scalar> init_children{selene_scalar_0, selene_scalar_1};
        const auto init_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{init_children.data(), init_children.size()});

        const auto selene_scalar_2 = generate_random_selene_scalar();

        // Trim the 2nd child and grow with new child
        const auto &trimmed_children = Selene::Chunk{init_children.data() + 1, 1};
        const auto trim_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_trim(
            init_hash,
            1,
            trimmed_children,
            selene_scalar_2);
        const auto trim_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(trim_res);

        // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_2}
        std::vector<Selene::Scalar> remaining_children{selene_scalar_0, selene_scalar_2};
        const auto grow_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{remaining_children.data(), remaining_children.size()});
        const auto grow_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res);

        ASSERT_EQ(trim_res_bytes, grow_res_bytes);
    }

    // 4. Trim 2 and grow back by 1
    {
        // Start by hashing:  {selene_scalar_0, selene_scalar_1, selene_scalar_2}
        // Then trim+grow to: {selene_scalar_0, selene_scalar_3}
        const auto selene_scalar_0 = generate_random_selene_scalar();
        const auto selene_scalar_1 = generate_random_selene_scalar();
        const auto selene_scalar_2 = generate_random_selene_scalar();

        // Get the initial hash of the 3 selene scalars
        std::vector<Selene::Scalar> init_children{selene_scalar_0, selene_scalar_1, selene_scalar_2};
        const auto init_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{init_children.data(), init_children.size()});

        const auto selene_scalar_3 = generate_random_selene_scalar();

        // Trim the initial result by 2 children+grow by 1
        const auto &trimmed_children = Selene::Chunk{init_children.data() + 1, 2};
        const auto trim_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_trim(
            init_hash,
            1,
            trimmed_children,
            selene_scalar_3);
        const auto trim_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(trim_res);

        // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_3}
        std::vector<Selene::Scalar> remaining_children{selene_scalar_0, selene_scalar_3};
        const auto grow_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
            /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
            /*offset*/                   0,
            /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
            /*children*/                 Selene::Chunk{remaining_children.data(), remaining_children.size()});
        const auto grow_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res);

        ASSERT_EQ(trim_res_bytes, grow_res_bytes);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, hash_grow)
{
    // Start by hashing: {selene_scalar_0, selene_scalar_1}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2, selene_scalar_3}
    const auto selene_scalar_0 = generate_random_selene_scalar();
    const auto selene_scalar_1 = generate_random_selene_scalar();

    // Get the initial hash of the 2 selene scalars
    std::vector<Selene::Scalar> all_children{selene_scalar_0, selene_scalar_1};
    const auto init_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
        /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
        /*offset*/                   0,
        /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});

    // Extend with a new child
    const auto selene_scalar_2 = generate_random_selene_scalar();
    std::vector<Selene::Scalar> new_children{selene_scalar_2};
    const auto ext_hash = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
        init_hash,
        all_children.size(),
        fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(ext_hash);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_2}
    all_children.push_back(selene_scalar_2);
    const auto grow_res = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
        /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
        /*offset*/                   0,
        /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_bytes = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res);

    ASSERT_EQ(ext_hash_bytes, grow_res_bytes);

    // Extend again with a new child
    const auto selene_scalar_3 = generate_random_selene_scalar();
    new_children.clear();
    new_children = {selene_scalar_3};
    const auto ext_hash2 = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
        ext_hash,
        all_children.size(),
        fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_bytes2 = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(ext_hash2);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_2, selene_scalar_3}
    all_children.push_back(selene_scalar_3);
    const auto grow_res2 = fcmp::curve_trees::curve_trees_v1.m_c2.hash_grow(
        /*existing_hash*/            fcmp::curve_trees::curve_trees_v1.m_c2.m_hash_init_point,
        /*offset*/                   0,
        /*existing_child_at_offset*/ fcmp::curve_trees::curve_trees_v1.m_c2.zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_bytes2 = fcmp::curve_trees::curve_trees_v1.m_c2.to_bytes(grow_res2);

    ASSERT_EQ(ext_hash_bytes2, grow_res_bytes2);
}
