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

#include "curve_trees.h"

#include <cmath>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesUnitTest helpers
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static CurveTreesV1::LastChunkData<C> get_last_child_layer_chunk(const C &curve,
    const std::size_t child_layer_size,
    const std::size_t parent_layer_size,
    const std::size_t chunk_width,
    const typename C::Scalar &last_child,
    const typename C::Point &last_parent)
{
    CHECK_AND_ASSERT_THROW_MES(child_layer_size > 0, "empty child layer");
    CHECK_AND_ASSERT_THROW_MES(parent_layer_size > 0, "empty parent layer");

    const std::size_t child_offset = child_layer_size % chunk_width;

    return CurveTreesV1::LastChunkData<C>{
        .child_offset      = child_offset,
        .last_child        = curve.clone(last_child),
        .last_parent       = curve.clone(last_parent),
        .child_layer_size  = child_layer_size,
        .parent_layer_size = parent_layer_size
    };
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesUnitTest implementations
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::LastChunks CurveTreesUnitTest::get_last_chunks(const CurveTreesUnitTest::Tree &tree)
{
    const auto &leaves    = tree.leaves;
    const auto &c1_layers = tree.c1_layers;
    const auto &c2_layers = tree.c2_layers;

    // We started with c2 and then alternated, so c2 is the same size or 1 higher than c1
    CHECK_AND_ASSERT_THROW_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
        "unexpected number of curve layers");

    CurveTreesV1::LastChunks last_chunks;

    if (c2_layers.empty())
        return last_chunks;

    auto &c1_last_chunks_out = last_chunks.c1_last_chunks;
    auto &c2_last_chunks_out = last_chunks.c2_last_chunks;

    c1_last_chunks_out.reserve(c1_layers.size());
    c2_last_chunks_out.reserve(c2_layers.size());

    // First push the last leaf chunk data into c2 chunks
    auto last_leaf_chunk = get_last_child_layer_chunk<Selene>(m_curve_trees.m_c2,
        /*child_layer_size */ leaves.size() * CurveTreesV1::LEAF_TUPLE_SIZE,
        /*parent_layer_size*/ c2_layers[0].size(),
        /*chunk_width      */ m_curve_trees.m_leaf_layer_chunk_width,
        /*last_child       */ leaves.back().C_x,
        /*last_parent      */ c2_layers[0].back());

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

            auto last_parent_chunk = get_last_child_layer_chunk<Helios>(m_curve_trees.m_c1,
                child_layer.size(),
                parent_layer.size(),
                m_curve_trees.m_c1_width,
                last_child,
                parent_layer.back());

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

            auto last_parent_chunk = get_last_child_layer_chunk<Selene>(m_curve_trees.m_c2,
                child_layer.size(),
                parent_layer.size(),
                m_curve_trees.m_c2_width,
                last_child,
                parent_layer.back());

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
void CurveTreesUnitTest::extend_tree(const CurveTreesV1::TreeExtension &tree_extension,
    CurveTreesUnitTest::Tree &tree_inout)
{
    // Add the leaves
    const std::size_t init_num_leaves = tree_inout.leaves.size() * m_curve_trees.LEAF_TUPLE_SIZE;
    CHECK_AND_ASSERT_THROW_MES(init_num_leaves == tree_extension.leaves.start_idx,
        "unexpected leaf start idx");

    tree_inout.leaves.reserve(tree_inout.leaves.size() + tree_extension.leaves.tuples.size());
    for (const auto &leaf : tree_extension.leaves.tuples)
    {
        tree_inout.leaves.emplace_back(CurveTreesV1::LeafTuple{
            .O_x = m_curve_trees.m_c2.clone(leaf.O_x),
            .I_x = m_curve_trees.m_c2.clone(leaf.I_x),
            .C_x = m_curve_trees.m_c2.clone(leaf.C_x)
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
            const CurveTreesV1::LayerExtension<Selene> &c2_ext = c2_extensions[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(!c2_ext.hashes.empty(), "empty c2 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c2_idx <= tree_inout.c2_layers.size(), "missing c2 layer");
            if (tree_inout.c2_layers.size() == c2_idx)
                tree_inout.c2_layers.emplace_back(Layer<Selene>{});

            auto &c2_inout = tree_inout.c2_layers[c2_idx];

            const bool started_after_tip = (c2_inout.size() == c2_ext.start_idx);
            const bool started_at_tip    = (c2_inout.size() == (c2_ext.start_idx + 1));
            CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c2 layer start");

            // We updated the last hash
            if (started_at_tip)
                c2_inout.back() = m_curve_trees.m_c2.clone(c2_ext.hashes.front());

            for (std::size_t i = started_at_tip ? 1 : 0; i < c2_ext.hashes.size(); ++i)
                c2_inout.emplace_back(m_curve_trees.m_c2.clone(c2_ext.hashes[i]));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer extension");
            const CurveTreesV1::LayerExtension<Helios> &c1_ext = c1_extensions[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(!c1_ext.hashes.empty(), "empty c1 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c1_idx <= tree_inout.c1_layers.size(), "missing c1 layer");
            if (tree_inout.c1_layers.size() == c1_idx)
                tree_inout.c1_layers.emplace_back(Layer<Helios>{});

            auto &c1_inout = tree_inout.c1_layers[c1_idx];

            const bool started_after_tip = (c1_inout.size() == c1_ext.start_idx);
            const bool started_at_tip    = (c1_inout.size() == (c1_ext.start_idx + 1));
            CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c1 layer start");

            // We updated the last hash
            if (started_at_tip)
                c1_inout.back() = m_curve_trees.m_c1.clone(c1_ext.hashes.front());

            for (std::size_t i = started_at_tip ? 1 : 0; i < c1_ext.hashes.size(); ++i)
                c1_inout.emplace_back(m_curve_trees.m_c1.clone(c1_ext.hashes[i]));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_PARENT>
bool CurveTreesUnitTest::validate_layer(const C_PARENT &c_parent,
    const CurveTreesUnitTest::Layer<C_PARENT> &parents,
    const std::vector<typename C_PARENT::Scalar> &child_scalars,
    const std::size_t max_chunk_size)
{
    // Hash chunk of children scalars, then see if the hash matches up to respective parent
    std::size_t chunk_start_idx = 0;
    for (std::size_t i = 0; i < parents.size(); ++i)
    {
        CHECK_AND_ASSERT_MES(child_scalars.size() > chunk_start_idx, false, "chunk start too high");
        const std::size_t chunk_size = std::min(child_scalars.size() - chunk_start_idx, max_chunk_size);
        CHECK_AND_ASSERT_MES(child_scalars.size() >= (chunk_start_idx + chunk_size), false, "chunk size too large");

        const typename C_PARENT::Point &parent = parents[i];

        const auto chunk_start = child_scalars.data() + chunk_start_idx;
        const typename C_PARENT::Chunk chunk{chunk_start, chunk_size};

        const typename C_PARENT::Point chunk_hash = m_curve_trees.get_new_parent(c_parent, chunk);

        const auto actual_bytes = c_parent.to_bytes(parent);
        const auto expected_bytes = c_parent.to_bytes(chunk_hash);
        CHECK_AND_ASSERT_MES(actual_bytes == expected_bytes, false, "unexpected hash");

        chunk_start_idx += chunk_size;
    }

    CHECK_AND_ASSERT_THROW_MES(chunk_start_idx == child_scalars.size(), "unexpected ending chunk start idx");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool CurveTreesUnitTest::validate_tree(const CurveTreesUnitTest::Tree &tree)
{
    const auto &leaves = tree.leaves;
    const auto &c1_layers = tree.c1_layers;
    const auto &c2_layers = tree.c2_layers;

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

            const bool valid = this->validate_layer<Selene>(m_curve_trees.m_c2,
                parents,
                child_scalars,
                m_curve_trees.m_c2_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c2_idx " + std::to_string(c2_idx));

            --c2_idx;
        }
        else
        {
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

            const bool valid = this->validate_layer<Helios>(
                m_curve_trees.m_c1,
                parents,
                child_scalars,
                m_curve_trees.m_c1_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c1_idx " + std::to_string(c1_idx));

            --c1_idx;
        }

        parent_is_c2 = !parent_is_c2;
    }

    // Now validate leaves
    return this->validate_layer<Selene>(m_curve_trees.m_c2,
        c2_layers[0],
        m_curve_trees.flatten_leaves(leaves),
        m_curve_trees.m_leaf_layer_chunk_width);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Logging helpers
//----------------------------------------------------------------------------------------------------------------------
static void log_last_chunks(const CurveTreesV1::LastChunks &last_chunks)
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

            const CurveTreesV1::LastChunkData<Selene> &last_chunk = c2_last_chunks[c2_idx];

            MDEBUG("child_offset: "         << last_chunk.child_offset
                << " , last_child: "        << fcmp::tower_cycle::selene::SELENE.to_string(last_chunk.last_child)
                << " , last_parent: "       << fcmp::tower_cycle::selene::SELENE.to_string(last_chunk.last_parent)
                << " , child_layer_size: "  << last_chunk.child_layer_size
                << " , parent_layer_size: " << last_chunk.parent_layer_size);

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_last_chunks.size(), "unexpected c1 layer");

            const CurveTreesV1::LastChunkData<Helios> &last_chunk = c1_last_chunks[c1_idx];

            MDEBUG("child_offset: "         << last_chunk.child_offset
                << " , last_child: "        << fcmp::tower_cycle::helios::HELIOS.to_string(last_chunk.last_child)
                << " , last_parent: "       << fcmp::tower_cycle::helios::HELIOS.to_string(last_chunk.last_parent)
                << " , child_layer_size: "  << last_chunk.child_layer_size
                << " , parent_layer_size: " << last_chunk.parent_layer_size);

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void log_tree_extension(const CurveTreesV1::TreeExtension &tree_extension)
{
    const auto &c1_extensions = tree_extension.c1_layer_extensions;
    const auto &c2_extensions = tree_extension.c2_layer_extensions;

    MDEBUG("Tree extension has " << tree_extension.leaves.tuples.size() << " leaves, "
        << c1_extensions.size() << " helios layers, " <<  c2_extensions.size() << " selene layers");

    MDEBUG("Leaf start idx: " << tree_extension.leaves.start_idx);
    for (std::size_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &leaf = tree_extension.leaves.tuples[i];

        const auto O_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.O_x);
        const auto I_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.I_x);
        const auto C_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.C_x);

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

            const CurveTreesV1::LayerExtension<Selene> &c2_layer = c2_extensions[c2_idx];
            MDEBUG("Selene tree extension start idx: " << c2_layer.start_idx);

            for (std::size_t j = 0; j < c2_layer.hashes.size(); ++j)
                MDEBUG("Hash idx: " << (j + c2_layer.start_idx) << " , hash: "
                    << fcmp::tower_cycle::selene::SELENE.to_string(c2_layer.hashes[j]));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer");

            const CurveTreesV1::LayerExtension<Helios> &c1_layer = c1_extensions[c1_idx];
            MDEBUG("Helios tree extension start idx: " << c1_layer.start_idx);

            for (std::size_t j = 0; j < c1_layer.hashes.size(); ++j)
                MDEBUG("Hash idx: " << (j + c1_layer.start_idx) << " , hash: "
                    << fcmp::tower_cycle::helios::HELIOS.to_string(c1_layer.hashes[j]));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void log_tree(const CurveTreesUnitTest::Tree &tree)
{
    MDEBUG("Tree has " << tree.leaves.size() << " leaves, "
        << tree.c1_layers.size() << " helios layers, " <<  tree.c2_layers.size() << " selene layers");

    for (std::size_t i = 0; i < tree.leaves.size(); ++i)
    {
        const auto &leaf = tree.leaves[i];

        const auto O_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.O_x);
        const auto I_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.I_x);
        const auto C_x = fcmp::tower_cycle::selene::SELENE.to_string(leaf.C_x);

        MDEBUG("Leaf idx " << i << " : { O_x: " << O_x << " , I_x: " << I_x << " , C_x: " << C_x << " }");
    }

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (tree.c1_layers.size() + tree.c2_layers.size()); ++i)
    {
        if (use_c2)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < tree.c2_layers.size(), "unexpected c2 layer");

            const CurveTreesUnitTest::Layer<Selene> &c2_layer = tree.c2_layers[c2_idx];
            MDEBUG("Selene layer size: " << c2_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c2_layer.size(); ++j)
                MDEBUG("Hash idx: " << j << " , hash: " << fcmp::tower_cycle::selene::SELENE.to_string(c2_layer[j]));

            ++c2_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < tree.c1_layers.size(), "unexpected c1 layer");

            const CurveTreesUnitTest::Layer<Helios> &c1_layer = tree.c1_layers[c1_idx];
            MDEBUG("Helios layer size: " << c1_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c1_layer.size(); ++j)
                MDEBUG("Hash idx: " << j << " , hash: " << fcmp::tower_cycle::helios::HELIOS.to_string(c1_layer[j]));

            ++c1_idx;
        }

        use_c2 = !use_c2;
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test helpers
//----------------------------------------------------------------------------------------------------------------------
static const CurveTreesV1::Leaves generate_leaves(const CurveTreesV1 &curve_trees, const std::size_t num_leaves)
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

    return CurveTreesV1::Leaves{
        .start_idx = 0,
        .tuples    = std::move(tuples)
    };
}
//----------------------------------------------------------------------------------------------------------------------
static void grow_tree_test(CurveTreesV1 &curve_trees,
    CurveTreesUnitTest &curve_trees_accessor,
    const std::size_t num_leaves,
    CurveTreesUnitTest::Tree &tree_inout)
{
    const auto last_chunks = curve_trees_accessor.get_last_chunks(tree_inout);
    log_last_chunks(last_chunks);

    const auto tree_extension = curve_trees.get_tree_extension(
        last_chunks,
        generate_leaves(curve_trees, num_leaves));
    log_tree_extension(tree_extension);

    curve_trees_accessor.extend_tree(tree_extension, tree_inout);
    log_tree(tree_inout);

    ASSERT_TRUE(curve_trees_accessor.validate_tree(tree_inout));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, grow_tree)
{
    // TODO: test varying widths
    const std::size_t HELIOS_CHUNK_WIDTH = 5;
    const std::size_t SELENE_CHUNK_WIDTH = 5;

    auto curve_trees = CurveTreesV1(
        fcmp::tower_cycle::helios::HELIOS,
        fcmp::tower_cycle::selene::SELENE,
        HELIOS_CHUNK_WIDTH,
        SELENE_CHUNK_WIDTH);

    CurveTreesUnitTest curve_trees_accesor{curve_trees};

    const std::vector<std::size_t> N_LEAVES{
        1,
        2,
        3,
        SELENE_CHUNK_WIDTH - 1,
        SELENE_CHUNK_WIDTH,
        SELENE_CHUNK_WIDTH + 1,
        (std::size_t)std::pow(SELENE_CHUNK_WIDTH, 2) - 1,
        (std::size_t)std::pow(SELENE_CHUNK_WIDTH, 2),
        (std::size_t)std::pow(SELENE_CHUNK_WIDTH, 2) + 1,
        (std::size_t)std::pow(SELENE_CHUNK_WIDTH, 3),
        (std::size_t)std::pow(SELENE_CHUNK_WIDTH, 4)
    };

    for (const std::size_t init_leaves : N_LEAVES)
    {
        for (const std::size_t ext_leaves : N_LEAVES)
        {
            MDEBUG("Adding " << init_leaves << " leaves to tree, then extending by " << ext_leaves << " leaves");

            CurveTreesUnitTest::Tree global_tree;

            // Initialize global tree with `init_leaves`
            {
                MDEBUG("Adding " << init_leaves << " leaves to tree");

                grow_tree_test(curve_trees,
                    curve_trees_accesor,
                    init_leaves,
                    global_tree);

                MDEBUG("Successfully added initial " << init_leaves << " leaves to tree");
            }

            // Then extend the global tree by `ext_leaves`
            {
                MDEBUG("Extending tree by " << ext_leaves << " leaves");

                grow_tree_test(curve_trees,
                    curve_trees_accesor,
                    ext_leaves,
                    global_tree);

                MDEBUG("Successfully extended by " << ext_leaves << " leaves");
            }
        }
    }
}
