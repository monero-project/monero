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
#include "fcmp_pp/fcmp_pp_crypto.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

#include <algorithm>


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test helpers
//----------------------------------------------------------------------------------------------------------------------
namespace test
{
const std::vector<fcmp_pp::curve_trees::OutputContext> generate_random_outputs(const CurveTreesV1 &curve_trees,
    const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples)
{
    std::vector<fcmp_pp::curve_trees::OutputContext> outs;
    outs.reserve(new_n_leaf_tuples);

    for (std::size_t i = 0; i < new_n_leaf_tuples; ++i)
    {
        const std::uint64_t output_id = old_n_leaf_tuples + i;

        // Generate random output tuple
        crypto::secret_key o,c;
        crypto::public_key O,C;
        crypto::generate_keys(O, o, o, false);
        crypto::generate_keys(C, c, c, false);

        rct::key C_key = rct::pk2rct(C);
        auto output_pair = fcmp_pp::curve_trees::OutputPair{
                .output_pubkey = std::move(O),
                .commitment    = std::move(C_key)
            };

        auto output_context = fcmp_pp::curve_trees::OutputContext{
                .output_id   = output_id,
                .output_pair = std::move(output_pair)
            };

        outs.emplace_back(std::move(output_context));
    }

    return outs;
}
//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<CurveTreesV1> init_curve_trees_test(const std::size_t selene_chunk_width,
    const std::size_t helios_chunk_width,
    const std::size_t tree_depth,
    uint64_t &n_leaves_out)
{
    CHECK_AND_ASSERT_THROW_MES(selene_chunk_width > 1, "selene width must be > 1");
    CHECK_AND_ASSERT_THROW_MES(helios_chunk_width > 1, "helios width must be > 1");
    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1(selene_chunk_width, helios_chunk_width);

    /* Number of leaves required for tree to reach given depth */
    n_leaves_out = (uint64_t) (tree_depth > 1 ? selene_chunk_width : 0);
    for (uint64_t i = 1; i < tree_depth; ++i)
    {
        const std::size_t width = i % 2 == 0 ? selene_chunk_width : helios_chunk_width;
        n_leaves_out *= width;
    }

    /* Increment to test for off-by-1 */
    ++n_leaves_out;

    return curve_trees;
}
}//namespace test
//----------------------------------------------------------------------------------------------------------------------
static const Selene::Scalar generate_random_selene_scalar()
{
    crypto::secret_key s;
    crypto::public_key S;

    crypto::generate_keys(S, s, s, false);

    rct::key S_x;
    CHECK_AND_ASSERT_THROW_MES(fcmp_pp::point_to_wei_x(rct::pk2rct(S), S_x), "failed to convert to wei x");
    return fcmp_pp::tower_cycle::selene_scalar_from_bytes(S_x);
}
//----------------------------------------------------------------------------------------------------------------------
static bool grow_tree_db(const uint64_t block_idx,
    const std::size_t expected_old_n_leaf_tuples,
    const std::size_t n_leaves,
    std::shared_ptr<CurveTreesV1> curve_trees,
    unit_test::BlockchainLMDBTest &test_db)
{
    cryptonote::db_wtxn_guard guard(test_db.m_db);

    LOG_PRINT_L1("Growing " << n_leaves << " leaves on tree with " << expected_old_n_leaf_tuples << " leaves in db");

    CHECK_AND_ASSERT_MES(test_db.m_db->get_n_leaf_tuples() == (uint64_t)(expected_old_n_leaf_tuples),
        false, "unexpected starting n leaf tuples in db");

    auto new_outputs = test::generate_random_outputs(*curve_trees, 0, n_leaves);

    test_db.m_db->grow_tree(block_idx, std::move(new_outputs));

    return test_db.m_db->audit_tree(expected_old_n_leaf_tuples + n_leaves);
}
//----------------------------------------------------------------------------------------------------------------------
static bool trim_tree_db(const uint64_t block_idx,
    const std::size_t expected_old_n_leaf_tuples,
    const std::size_t trim_leaves,
    unit_test::BlockchainLMDBTest &test_db)
{
    cryptonote::db_wtxn_guard guard(test_db.m_db);

    CHECK_AND_ASSERT_THROW_MES(expected_old_n_leaf_tuples >= trim_leaves, "cannot trim more leaves than exist");
    CHECK_AND_ASSERT_THROW_MES(trim_leaves > 0, "must be trimming some leaves");

    LOG_PRINT_L1("Trimming " << trim_leaves << " leaf tuples from tree with "
        << expected_old_n_leaf_tuples << " leaves in db");

    CHECK_AND_ASSERT_MES(test_db.m_db->get_n_leaf_tuples() == (uint64_t)(expected_old_n_leaf_tuples),
        false, "trimming unexpected starting n leaf tuples in db");

    const uint64_t new_n_leaf_tuples = expected_old_n_leaf_tuples - trim_leaves;
    test_db.m_db->trim_tree(new_n_leaf_tuples, block_idx);

    CHECK_AND_ASSERT_MES(test_db.m_db->audit_tree(new_n_leaf_tuples), false,
        "failed to trim tree in db");

    MDEBUG("Successfully trimmed tree in db by " << trim_leaves << " leaves");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
#define INIT_CURVE_TREES_TEST                                                     \
    static_assert(selene_chunk_width > 1, "selene width must be > 1");            \
    static_assert(helios_chunk_width > 1, "helios width must be > 1");            \
                                                                                  \
    /* Number of leaves required for tree to reach given depth */                 \
    uint64_t min_leaves_needed_for_tree_depth = 0;                                \
    const auto curve_trees = test::init_curve_trees_test(selene_chunk_width,      \
        helios_chunk_width,                                                       \
        tree_depth,                                                               \
        min_leaves_needed_for_tree_depth);                                        \
                                                                                  \
    unit_test::BlockchainLMDBTest test_db;                                        \
//----------------------------------------------------------------------------------------------------------------------
#define BEGIN_INIT_TREE_ITER(curve_trees, init_mem_tree)                                              \
    for (std::size_t init_leaves = 1; init_leaves <= min_leaves_needed_for_tree_depth; ++init_leaves) \
    {                                                                                                 \
        LOG_PRINT_L1("Initializing tree with " << init_leaves << " leaves");                          \
                                                                                                      \
        /* Init tree in memory */                                                                     \
        CurveTreesGlobalTree global_tree(*curve_trees);                                               \
        if (init_mem_tree)                                                                            \
        {                                                                                             \
            const auto init_outputs = test::generate_random_outputs(*curve_trees, 0, init_leaves);    \
            ASSERT_TRUE(global_tree.grow_tree(0, init_leaves, init_outputs));                         \
        }                                                                                             \
                                                                                                      \
        /* Init tree in db */                                                                         \
        INIT_BLOCKCHAIN_LMDB_TEST_DB(test_db, curve_trees);                                           \
        ASSERT_TRUE(grow_tree_db(0, 0, init_leaves, curve_trees, test_db));                           \
//----------------------------------------------------------------------------------------------------------------------
#define END_INIT_TREE_ITER(curve_trees) \
    };                                  \
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree helpers
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static bool validate_layer(const std::unique_ptr<C> &curve,
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
            MDEBUG("Hashing " << curve->to_string(chunk_start[i]));

        const typename C::Point chunk_hash = fcmp_pp::curve_trees::get_new_parent(curve, chunk);

        MDEBUG("chunk_start_idx: " << chunk_start_idx << " , chunk_size: " << chunk_size << " , chunk_hash: " << curve->to_string(chunk_hash));

        const auto actual = curve->to_string(parent);
        const auto expected = curve->to_string(chunk_hash);
        CHECK_AND_ASSERT_MES(actual == expected, false, "unexpected hash");

        chunk_start_idx += chunk_size;
    }

    CHECK_AND_ASSERT_THROW_MES(chunk_start_idx == child_scalars.size(), "unexpected ending chunk start idx");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree public implementations
//----------------------------------------------------------------------------------------------------------------------
std::size_t CurveTreesGlobalTree::get_n_leaf_tuples() const
{
    return m_tree.leaves.size();
}
//----------------------------------------------------------------------------------------------------------------------
bool CurveTreesGlobalTree::grow_tree(const std::size_t expected_old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples,
    const std::vector<fcmp_pp::curve_trees::OutputContext> &new_outputs,
    const bool audit_tree)
{
    CHECK_AND_ASSERT_MES(new_outputs.size() == new_n_leaf_tuples, false, "unexpected n new outputs");

    // Do initial tree reads
    const std::size_t old_n_leaf_tuples = this->get_n_leaf_tuples();
    CHECK_AND_ASSERT_MES(old_n_leaf_tuples == expected_old_n_leaf_tuples, false, "unexpected old_n_leaf_tuples");
    const CurveTreesV1::LastHashes last_hashes = this->get_last_hashes();

    this->log_last_hashes(last_hashes);

    // Get a tree extension object to the existing tree using randomly generated leaves
    // - The tree extension includes all elements we'll need to add to the existing tree when adding the new leaves
    const auto tree_extension = m_curve_trees.get_tree_extension(old_n_leaf_tuples,
        last_hashes,
        {std::move(new_outputs)},
        true/*use_fast_torsion_check*/);

    this->log_tree_extension(tree_extension);

    // Use the tree extension to extend the existing tree
    this->extend_tree(tree_extension);

    this->log_tree();

    if (!audit_tree)
        return true;

    // Validate tree structure and all hashes
    const std::size_t expected_n_leaf_tuples = old_n_leaf_tuples + new_n_leaf_tuples;
    return this->audit_tree(expected_n_leaf_tuples);
}
//----------------------------------------------------------------------------------------------------------------------
bool CurveTreesGlobalTree::audit_tree(const std::size_t expected_n_leaf_tuples) const
{
    MDEBUG("Auditing global tree");

    auto leaves = m_tree.leaves;
    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    CHECK_AND_ASSERT_MES(leaves.size() == expected_n_leaf_tuples, false, "unexpected num leaves");

    if (leaves.empty())
    {
        CHECK_AND_ASSERT_MES(c1_layers.empty() && c2_layers.empty(), false, "expected empty tree");
        return true;
    }

    CHECK_AND_ASSERT_MES(!c1_layers.empty(), false, "must have at least 1 c1 layer in tree");
    CHECK_AND_ASSERT_MES(c1_layers.size() == c2_layers.size() || c1_layers.size() == (c2_layers.size() + 1),
        false, "unexpected mismatch of c1 and c2 layers");

    const std::size_t n_layers = c1_layers.size() + c2_layers.size();
    CHECK_AND_ASSERT_MES(n_layers == m_curve_trees.n_layers(leaves.size()), false, "unexpected n_layers");

    // Verify root has 1 member in it
    const bool c1_is_root = c1_layers.size() > c2_layers.size();
    CHECK_AND_ASSERT_MES(c1_is_root ? c1_layers.back().size() == 1 : c2_layers.back().size() == 1, false,
        "root must have 1 member in it");

    // Iterate from root down to layer above leaves, and check hashes match up correctly
    bool parent_is_c1 = c1_is_root;
    std::size_t c1_idx = c1_layers.size() - 1;
    std::size_t c2_idx = c2_layers.empty() ? 0 : (c2_layers.size() - 1);
    for (std::size_t i = 1; i < n_layers; ++i)
    {
        // TODO: implement templated function for below if statement
        if (parent_is_c1)
        {
            MDEBUG("Validating parent c1 layer " << c1_idx << " , child c2 layer " << c2_idx);

            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");

            const Layer<Selene> &parents  = c1_layers[c1_idx];
            const Layer<Helios> &children = c2_layers[c2_idx];

            CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c1_idx " + std::to_string(c1_idx));
            CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c2_idx " + std::to_string(c2_idx));

            std::vector<Selene::Scalar> child_scalars;
            fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Helios, Selene>(m_curve_trees.m_c2,
                children,
                child_scalars);

            const bool valid = validate_layer<Selene>(
                m_curve_trees.m_c1,
                parents,
                child_scalars,
                m_curve_trees.m_c1_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c1_idx " + std::to_string(c1_idx));

            --c1_idx;
        }
        else
        {
            MDEBUG("Validating parent c2 layer " << c2_idx << " , child c1 layer " << c1_idx);

            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");

            const Layer<Helios> &parents  = c2_layers[c2_idx];
            const Layer<Selene> &children = c1_layers[c1_idx];

            CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c2_idx " + std::to_string(c2_idx));
            CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c1_idx " + std::to_string(c1_idx));

            std::vector<Helios::Scalar> child_scalars;
            fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Selene, Helios>(m_curve_trees.m_c1,
                children,
                child_scalars);

            const bool valid = validate_layer<Helios>(m_curve_trees.m_c2,
                parents,
                child_scalars,
                m_curve_trees.m_c2_width);

            CHECK_AND_ASSERT_MES(valid, false, "failed to validate c2_idx " + std::to_string(c2_idx));

            --c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    MDEBUG("Validating leaves");

    // Convert output pairs to leaf tuples
    std::vector<CurveTreesV1::LeafTuple> leaf_tuples;
    leaf_tuples.reserve(leaves.size());
    for (const auto &leaf : leaves)
    {
        auto leaf_tuple = m_curve_trees.leaf_tuple(leaf);
        leaf_tuples.emplace_back(std::move(leaf_tuple));
    }

    // Now validate leaves
    return validate_layer<Selene>(m_curve_trees.m_c1,
        c1_layers[0],
        m_curve_trees.flatten_leaves(std::move(leaf_tuples)),
        m_curve_trees.m_leaf_layer_chunk_width);
}
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::Path CurveTreesGlobalTree::get_path_at_leaf_idx(const std::size_t leaf_idx) const
{
    CurveTreesV1::Path path_out;

    const std::size_t n_leaf_tuples = get_n_leaf_tuples();
    CHECK_AND_ASSERT_THROW_MES(n_leaf_tuples > leaf_idx, "too high leaf idx");
    const auto path_idxs = m_curve_trees.get_path_indexes(n_leaf_tuples, leaf_idx);

    // Get leaves
    const auto &leaf_range = path_idxs.leaf_range;
    for (std::size_t i = leaf_range.first; i < leaf_range.second; ++i)
    {
        const auto &output_pair = m_tree.leaves[i];

        const crypto::public_key &output_pubkey = output_pair.output_pubkey;
        const rct::key &commitment              = output_pair.commitment;

        crypto::ec_point I;
        crypto::derive_key_image_generator(output_pubkey, I);

        rct::key O = rct::pk2rct(output_pubkey);
        rct::key C = commitment;

        auto output_tuple = fcmp_pp::curve_trees::OutputTuple{
            .O = std::move(O),
            .I = std::move(rct::pt2rct(I)),
            .C = std::move(C)
        };

        path_out.leaves.emplace_back(std::move(output_tuple));
    }

    // Get parents
    const auto &layer_ranges = path_idxs.layers;
    std::size_t c1_idx = 0, c2_idx = 0;
    bool parent_is_c1 = true;
    for (const auto &layer_range : layer_ranges)
    {
        if (parent_is_c1)
        {
            path_out.c1_layers.emplace_back();
            auto &layer_out = path_out.c1_layers.back();

            CHECK_AND_ASSERT_THROW_MES(m_tree.c1_layers.size() > c1_idx, "too high c1_idx");
            const std::size_t n_layer_elems = m_tree.c1_layers[c1_idx].size();
            CHECK_AND_ASSERT_THROW_MES(n_layer_elems >= layer_range.second, "too high parent idx");

            for (std::size_t j = layer_range.first; j < layer_range.second; ++j)
            {
                layer_out.emplace_back(m_tree.c1_layers[c1_idx][j]);
            }

            ++c1_idx;
        }
        else
        {
            path_out.c2_layers.emplace_back();
            auto &layer_out = path_out.c2_layers.back();

            CHECK_AND_ASSERT_THROW_MES(m_tree.c2_layers.size() > c2_idx, "too high c2_idx");
            const std::size_t n_layer_elems = m_tree.c2_layers[c2_idx].size();
            CHECK_AND_ASSERT_THROW_MES(n_layer_elems >= layer_range.second, "too high parent idx");

            for (std::size_t j = layer_range.first; j < layer_range.second; ++j)
            {
                layer_out.emplace_back(m_tree.c2_layers[c2_idx][j]);
            }

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    return path_out;
}
//----------------------------------------------------------------------------------------------------------------------
std::set<std::size_t> CurveTreesGlobalTree::get_leaf_idxs_with_output_pair(
    const fcmp_pp::curve_trees::OutputPair &output_pair) const
{
    std::set<std::size_t> leaf_idx;
    for (std::size_t i = 0; i < m_tree.leaves.size(); ++i)
    {
        if (output_pair == m_tree.leaves.at(i))
            leaf_idx.insert(i);
    }
    return leaf_idx;
}
//----------------------------------------------------------------------------------------------------------------------
fcmp_pp::TreeRootShared CurveTreesGlobalTree::get_tree_root() const
{
    const std::size_t n_layers = m_tree.c1_layers.size() + m_tree.c2_layers.size();

    if (n_layers == 0)
        return nullptr;

    if ((n_layers % 2) == 0)
    {
        CHECK_AND_ASSERT_THROW_MES(!m_tree.c2_layers.empty(), "missing c2 layers");
        const auto &last_layer = m_tree.c2_layers.back();
        CHECK_AND_ASSERT_THROW_MES(last_layer.size() == 1, "unexpected n elems in c2 root");
        return fcmp_pp::helios_tree_root(last_layer.back());
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(!m_tree.c1_layers.empty(), "missing c1 layers");
        const auto &last_layer = m_tree.c1_layers.back();
        CHECK_AND_ASSERT_THROW_MES(last_layer.size() == 1, "unexpected n elems in c1 root");
        return fcmp_pp::selene_tree_root(last_layer.back());
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTreesGlobalTree private implementations
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::extend_tree(const CurveTreesV1::TreeExtension &tree_extension)
{
    // Add the leaves
    CHECK_AND_ASSERT_THROW_MES(m_tree.leaves.size() == tree_extension.leaves.start_leaf_tuple_idx,
        "unexpected leaf start idx");

    m_tree.leaves.reserve(m_tree.leaves.size() + tree_extension.leaves.tuples.size());
    for (const auto &o : tree_extension.leaves.tuples)
    {
        m_tree.leaves.emplace_back(o.output_pair);
    }

    // Add the layers
    const auto &c1_extensions = tree_extension.c1_layer_extensions;
    const auto &c2_extensions = tree_extension.c2_layer_extensions;
    CHECK_AND_ASSERT_THROW_MES(!c1_extensions.empty(), "empty c1 extensions");

    bool parent_is_c1 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    for (std::size_t i = 0; i < (c1_extensions.size() + c2_extensions.size()); ++i)
    {
        // TODO: template below if statement
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer extension");
            const fcmp_pp::curve_trees::LayerExtension<Selene> &c1_ext = c1_extensions[c1_idx];

            CHECK_AND_ASSERT_THROW_MES(!c1_ext.hashes.empty(), "empty c1 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c1_idx <= m_tree.c1_layers.size(), "missing c1 layer");
            if (m_tree.c1_layers.size() == c1_idx)
                m_tree.c1_layers.emplace_back(Layer<Selene>{});

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
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_extensions.size(), "unexpected c2 layer extension");
            const fcmp_pp::curve_trees::LayerExtension<Helios> &c2_ext = c2_extensions[c2_idx];

            CHECK_AND_ASSERT_THROW_MES(!c2_ext.hashes.empty(), "empty c2 layer extension");

            CHECK_AND_ASSERT_THROW_MES(c2_idx <= m_tree.c2_layers.size(), "missing c2 layer");
            if (m_tree.c2_layers.size() == c2_idx)
                m_tree.c2_layers.emplace_back(Layer<Helios>{});

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

        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
CurveTreesV1::LastHashes CurveTreesGlobalTree::get_last_hashes() const
{
    CurveTreesV1::LastHashes last_hashes_out;
    auto &c1_last_hashes_out = last_hashes_out.c1_last_hashes;
    auto &c2_last_hashes_out = last_hashes_out.c2_last_hashes;

    const auto &c1_layers = m_tree.c1_layers;
    const auto &c2_layers = m_tree.c2_layers;

    // We started with c1 and then alternated, so c1 is the same size or 1 higher than c2
    CHECK_AND_ASSERT_THROW_MES(c1_layers.size() == c2_layers.size() || c1_layers.size() == (c2_layers.size() + 1),
        "unexpected number of curve layers");

    c1_last_hashes_out.reserve(c1_layers.size());
    c2_last_hashes_out.reserve(c2_layers.size());

    if (c1_layers.empty())
        return last_hashes_out;

    // Next parents will be c1
    bool parent_is_c1 = true;

    // Then get last chunks up until the root
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    while (c1_last_hashes_out.size() < c1_layers.size() || c2_last_hashes_out.size() < c2_layers.size())
    {
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c1_layers.size() > c1_idx, "missing c1 layer");
            c1_last_hashes_out.push_back(c1_layers[c1_idx].back());
            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c2_layers.size() > c2_idx, "missing c2 layer");
            c2_last_hashes_out.push_back(c2_layers[c2_idx].back());
            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    return last_hashes_out;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Logging helpers
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_last_hashes(const CurveTreesV1::LastHashes &last_hashes)
{
    if (!el::Loggers::allowed(el::Level::Debug, "serialization"))
        return;

    const auto &c1_last_hashes = last_hashes.c1_last_hashes;
    const auto &c2_last_hashes = last_hashes.c2_last_hashes;

    MDEBUG("Total of " << c1_last_hashes.size() << " Helios layers and " << c2_last_hashes.size() << " Selene layers");

    bool parent_is_c1 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (c1_last_hashes.size() + c2_last_hashes.size()); ++i)
    {
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_last_hashes.size(), "unexpected c1 layer");

            const auto &last_hash = c1_last_hashes[c1_idx];
            MDEBUG("c1_idx: " << c1_idx << " , last_hash: " << m_curve_trees.m_c1->to_string(last_hash));

            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_last_hashes.size(), "unexpected c2 layer");

            const auto &last_hash = c2_last_hashes[c2_idx];
            MDEBUG("c2_idx: " << c2_idx << " , last_hash: " << m_curve_trees.m_c2->to_string(last_hash));

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_tree_extension(const CurveTreesV1::TreeExtension &tree_extension)
{
    if (!el::Loggers::allowed(el::Level::Debug, "serialization"))
        return;

    const auto &c1_extensions = tree_extension.c1_layer_extensions;
    const auto &c2_extensions = tree_extension.c2_layer_extensions;

    MDEBUG("Tree extension has " << tree_extension.leaves.tuples.size() << " leaves, "
        << c1_extensions.size() << " helios layers, " <<  c2_extensions.size() << " selene layers");

    MDEBUG("Leaf start idx: " << tree_extension.leaves.start_leaf_tuple_idx);
    for (std::size_t i = 0; i < tree_extension.leaves.tuples.size(); ++i)
    {
        const auto &output_pair = tree_extension.leaves.tuples[i].output_pair;
        const auto leaf = m_curve_trees.leaf_tuple(output_pair);

        const auto O_x = m_curve_trees.m_c1->to_string(leaf.O_x);
        const auto O_y = m_curve_trees.m_c1->to_string(leaf.O_y);

        const auto I_x = m_curve_trees.m_c1->to_string(leaf.I_x);
        const auto I_y = m_curve_trees.m_c1->to_string(leaf.I_y);

        const auto C_x = m_curve_trees.m_c1->to_string(leaf.C_x);
        const auto C_y = m_curve_trees.m_c1->to_string(leaf.C_y);

        MDEBUG("Leaf tuple idx " << (tree_extension.leaves.start_leaf_tuple_idx + (i * CurveTreesV1::LEAF_TUPLE_SIZE))
            << " : { O_x: " << O_x << " , O_y: " << O_y << " , I_x: " << I_x << " , I_y: " << I_y
            << " , C_x: " << C_x << " , C_y: " << C_y << " }");
    }

    bool parent_is_c1 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (c1_extensions.size() + c2_extensions.size()); ++i)
    {
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer");

            const fcmp_pp::curve_trees::LayerExtension<Selene> &c1_layer = c1_extensions[c1_idx];
            MDEBUG("Selene tree extension start idx: " << c1_layer.start_idx);

            for (std::size_t j = 0; j < c1_layer.hashes.size(); ++j)
                MDEBUG("Child chunk start idx: " << (j + c1_layer.start_idx) << " , hash: "
                    << m_curve_trees.m_c1->to_string(c1_layer.hashes[j]));

            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_extensions.size(), "unexpected c2 layer");

            const fcmp_pp::curve_trees::LayerExtension<Helios> &c2_layer = c2_extensions[c2_idx];
            MDEBUG("Helios tree extension start idx: " << c2_layer.start_idx);

            for (std::size_t j = 0; j < c2_layer.hashes.size(); ++j)
                MDEBUG("Child chunk start idx: " << (j + c2_layer.start_idx) << " , hash: "
                    << m_curve_trees.m_c2->to_string(c2_layer.hashes[j]));

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void CurveTreesGlobalTree::log_tree()
{
    if (!el::Loggers::allowed(el::Level::Debug, "serialization"))
        return;

    MDEBUG("Tree has " << m_tree.leaves.size() << " leaves, "
        << m_tree.c1_layers.size() << " selene layers, " <<  m_tree.c2_layers.size() << " helios layers");

    for (std::size_t i = 0; i < m_tree.leaves.size(); ++i)
    {
        const auto leaf = m_curve_trees.leaf_tuple(m_tree.leaves[i]);

        const auto O_x = m_curve_trees.m_c1->to_string(leaf.O_x);
        const auto O_y = m_curve_trees.m_c1->to_string(leaf.O_y);

        const auto I_x = m_curve_trees.m_c1->to_string(leaf.I_x);
        const auto I_y = m_curve_trees.m_c1->to_string(leaf.I_y);

        const auto C_x = m_curve_trees.m_c1->to_string(leaf.C_x);
        const auto C_y = m_curve_trees.m_c1->to_string(leaf.C_y);

        MDEBUG("Leaf idx " << i << " : { O_x: " << O_x << " , O_y: " << O_y << " , I_x: " << I_x << " , I_y: " << I_y
            << " , C_x: " << C_x << " , C_y: " << C_y << " }");
    }

    bool parent_is_c1 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;
    for (std::size_t i = 0; i < (m_tree.c1_layers.size() + m_tree.c2_layers.size()); ++i)
    {
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c1_idx < m_tree.c1_layers.size(), "unexpected c1 layer");

            const CurveTreesGlobalTree::Layer<Selene> &c1_layer = m_tree.c1_layers[c1_idx];
            MDEBUG("Selene layer size: " << c1_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c1_layer.size(); ++j)
                MDEBUG("Child chunk start idx: " << j << " , hash: " << m_curve_trees.m_c1->to_string(c1_layer[j]));

            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c2_idx < m_tree.c2_layers.size(), "unexpected c2 layer");

            const CurveTreesGlobalTree::Layer<Helios> &c2_layer = m_tree.c2_layers[c2_idx];
            MDEBUG("Helios layer size: " << c2_layer.size() << " , tree layer: " << i);

            for (std::size_t j = 0; j < c2_layer.size(); ++j)
                MDEBUG("Child chunk start idx: " << j << " , hash: " << m_curve_trees.m_c2->to_string(c2_layer[j]));

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Test
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, grow_tree)
{
    // Use lower values for chunk width than prod so that we can quickly test a many-layer deep tree
    static const std::size_t selene_chunk_width = 2;
    static const std::size_t helios_chunk_width = 3;
    static const std::size_t tree_depth = 4;

    LOG_PRINT_L1("Test grow tree with selene chunk width " << selene_chunk_width
        << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth);

    INIT_CURVE_TREES_TEST;

    // First initialize the tree with init_leaves
    BEGIN_INIT_TREE_ITER(curve_trees, true)

    // Then extend the tree with ext_leaves
    for (std::size_t ext_leaves = 1; (init_leaves + ext_leaves) <= min_leaves_needed_for_tree_depth; ++ext_leaves)
    {
        // Tree in memory
        // Copy the already existing global tree
        CurveTreesGlobalTree tree_copy(global_tree);
        const auto new_outputs = test::generate_random_outputs(*curve_trees, init_leaves, ext_leaves);
        ASSERT_TRUE(tree_copy.grow_tree(init_leaves, ext_leaves, new_outputs));

        // Tree in db
        // Copy the already existing db
        unit_test::BlockchainLMDBTest copy_db = *test_db.copy_db(curve_trees);
        INIT_BLOCKCHAIN_LMDB_TEST_DB(copy_db, nullptr);
        ASSERT_TRUE(grow_tree_db(1/*block_idx*/, init_leaves, ext_leaves, curve_trees, copy_db));
    }

    END_INIT_TREE_ITER()
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, trim_tree)
{
    // Use lower values for chunk width than prod so that we can quickly test a many-layer deep tree
    static const std::size_t selene_chunk_width = 2;
    static const std::size_t helios_chunk_width = 3;
    static const std::size_t tree_depth = 4;

    LOG_PRINT_L1("Test trim tree with selene chunk width " << selene_chunk_width
        << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth);

    INIT_CURVE_TREES_TEST;

    // First initialize the tree with init_leaves
    BEGIN_INIT_TREE_ITER(curve_trees, false)

    // Then grow a block with trim_leaves, then trim that block
    for (std::size_t trim_leaves = 1; trim_leaves <= min_leaves_needed_for_tree_depth; ++trim_leaves)
    {
        if (trim_leaves > init_leaves)
            continue;

        // Tree in db
        // Copy the already existing db
        unit_test::BlockchainLMDBTest copy_db = *test_db.copy_db(curve_trees);
        INIT_BLOCKCHAIN_LMDB_TEST_DB(copy_db, nullptr);
        const uint64_t block_idx = 1;
        ASSERT_TRUE(grow_tree_db(block_idx, init_leaves, trim_leaves, curve_trees, copy_db));
        ASSERT_TRUE(trim_tree_db(block_idx, init_leaves + trim_leaves, trim_leaves, copy_db));
    }

    END_INIT_TREE_ITER()
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, trim_tree_then_grow)
{
    // Use lower values for chunk width than prod so that we can quickly test a many-layer deep tree
    static const std::size_t selene_chunk_width = 3;
    static const std::size_t helios_chunk_width = 3;
    static const std::size_t tree_depth = 2;

    static const std::size_t grow_after_trim = 1;

    LOG_PRINT_L1("Test trim tree with selene chunk width " << selene_chunk_width
        << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth
        << ", then grow " << grow_after_trim << " leaf/leaves");

    INIT_CURVE_TREES_TEST;

    // First initialize the tree with init_leaves
    BEGIN_INIT_TREE_ITER(curve_trees, false)

    // Then trim by trim_leaves
    for (std::size_t trim_leaves = 1; trim_leaves <= min_leaves_needed_for_tree_depth; ++trim_leaves)
    {
        if (trim_leaves > init_leaves)
            continue;

        // Tree in db
        // Copy the already existing db
        unit_test::BlockchainLMDBTest copy_db = *test_db.copy_db(curve_trees);
        INIT_BLOCKCHAIN_LMDB_TEST_DB(copy_db, nullptr);
        const uint64_t block_idx = 1;
        ASSERT_TRUE(grow_tree_db(block_idx, init_leaves, trim_leaves, curve_trees, copy_db));
        ASSERT_TRUE(trim_tree_db(block_idx, init_leaves + trim_leaves, trim_leaves, copy_db));
        ASSERT_TRUE(grow_tree_db(block_idx, init_leaves, grow_after_trim, curve_trees, copy_db));
    }

    END_INIT_TREE_ITER()
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, hash_grow)
{
    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();

    // Start by hashing: {selene_scalar_0, selene_scalar_1}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2}
    // Then grow 1:      {selene_scalar_0, selene_scalar_1, selene_scalar_2, selene_scalar_3}
    const auto selene_scalar_0 = generate_random_selene_scalar();
    const auto selene_scalar_1 = generate_random_selene_scalar();

    // Get the initial hash of the 2 selene scalars
    std::vector<Selene::Scalar> all_children{selene_scalar_0, selene_scalar_1};
    const auto init_hash = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});

    // Extend with a new child
    const auto selene_scalar_2 = generate_random_selene_scalar();
    std::vector<Selene::Scalar> new_children{selene_scalar_2};
    const auto ext_hash = curve_trees->m_c1->hash_grow(
        init_hash,
        all_children.size(),
        curve_trees->m_c1->zero_scalar(),
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_str = curve_trees->m_c1->to_string(ext_hash);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_2}
    all_children.push_back(selene_scalar_2);
    const auto grow_res = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_str = curve_trees->m_c1->to_string(grow_res);

    ASSERT_EQ(ext_hash_str, grow_res_str);

    // Extend again with a new child
    const auto selene_scalar_3 = generate_random_selene_scalar();
    new_children.clear();
    new_children = {selene_scalar_3};
    const auto ext_hash2 = curve_trees->m_c1->hash_grow(
        ext_hash,
        all_children.size(),
        curve_trees->m_c1->zero_scalar(),
        Selene::Chunk{new_children.data(), new_children.size()});
    const auto ext_hash_str2 = curve_trees->m_c1->to_string(ext_hash2);

    // Now compare to calling hash_grow{selene_scalar_0, selene_scalar_1, selene_scalar_2, selene_scalar_3}
    all_children.push_back(selene_scalar_3);
    const auto grow_res2 = curve_trees->m_c1->hash_grow(
        /*existing_hash*/            curve_trees->m_c1->hash_init_point(),
        /*offset*/                   0,
        /*existing_child_at_offset*/ curve_trees->m_c1->zero_scalar(),
        /*children*/                 Selene::Chunk{all_children.data(), all_children.size()});
    const auto grow_res_str2 = curve_trees->m_c1->to_string(grow_res2);

    ASSERT_EQ(ext_hash_str2, grow_res_str2);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(curve_trees, path_for_proof_inner_layer_single_elem)
{
    /*
        Assume the path has the following composition:

        Root:            layer4_0
        Layer3: layer3_0 layer3_1 layer3_2
        Layer2:                   layer2_9
        Layer1:                   layer1_14
        Leaves:          leaf_100 leaf_101 leaf_102

        Direct parents for the leaf chunk are layer1_14 -> layer2_9 -> layer3_2 -> layer4_0

        Note that Layer1 and Layer2 only have 1 element each, but are not the root layer.
        We need to make sure that the path for proof still captures that layer.
    */

    // TODO: construct a path in place with the exact composition we want so this test executes faster
    const uint64_t N_LEAF_TUPLES = 52000;
    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    CurveTreesGlobalTree global_tree(*curve_trees);
    const auto init_outputs = test::generate_random_outputs(*curve_trees, 0, N_LEAF_TUPLES);
    ASSERT_TRUE(global_tree.grow_tree(0, init_outputs.size(), init_outputs));

    const auto path = global_tree.get_path_at_leaf_idx(init_outputs.size() - 1);
    ASSERT_TRUE(curve_trees->audit_path(path, init_outputs.back().output_pair, N_LEAF_TUPLES));

    // Make sure path has expected composition we're trying to test
    ASSERT_EQ(path.c1_layers.size(), 2);
    ASSERT_EQ(path.c2_layers.size(), 2);

    ASSERT_EQ(path.c1_layers.at(0).size(), 1);
    ASSERT_EQ(path.c1_layers.at(1).size(), 3);

    ASSERT_EQ(path.c2_layers.at(0).size(), 1);
    ASSERT_EQ(path.c2_layers.at(1).size(), 1);

    // Get the Rust-FFI friendly path
    const auto output_tuple = fcmp_pp::curve_trees::output_to_tuple(init_outputs.back().output_pair);
    const auto path_for_proof = curve_trees->path_for_proof(path, output_tuple);

    // Make sure Rust-FFI path has expected composition
    ASSERT_EQ(path_for_proof.c1_scalar_chunks.size(), 1);
    ASSERT_EQ(path_for_proof.c2_scalar_chunks.size(), 2);
}
//----------------------------------------------------------------------------------------------------------------------
