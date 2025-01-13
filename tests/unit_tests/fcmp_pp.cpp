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
#include "fcmp_pp/tower_cycle.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

#include "crypto/crypto.h"

struct OutputContextsAndKeys
{
    std::vector<crypto::secret_key> x_vec;
    std::vector<crypto::secret_key> y_vec;
    std::vector<fcmp_pp::curve_trees::OutputContext> outputs;
};

const OutputContextsAndKeys generate_random_outputs(const CurveTreesV1 &curve_trees,
    const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples)
{
    OutputContextsAndKeys outs;
    outs.x_vec.reserve(new_n_leaf_tuples);
    outs.y_vec.reserve(new_n_leaf_tuples);
    outs.outputs.reserve(new_n_leaf_tuples);

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

        // Output pubkey O = xG + yT
        // In this test, x is o, y is zero
        crypto::secret_key x = std::move(o);
        crypto::secret_key y;
        sc_0((unsigned char *)y.data);

        outs.x_vec.emplace_back(std::move(x));
        outs.y_vec.emplace_back(std::move(y));
        outs.outputs.emplace_back(std::move(output_context));
    }

    return outs;
}

//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, prove)
{
    static const std::size_t helios_chunk_width = fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH;
    static const std::size_t selene_chunk_width = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH;

    static const std::size_t tree_depth = 3;

    LOG_PRINT_L1("Test prove with helios chunk width " << helios_chunk_width
        << ", selene chunk width " << selene_chunk_width << ", tree depth " << tree_depth);

    uint64_t min_leaves_needed_for_tree_depth = 0;
    const auto curve_trees = test::init_curve_trees_test(helios_chunk_width,
        selene_chunk_width,
        tree_depth,
        min_leaves_needed_for_tree_depth);

    LOG_PRINT_L1("Initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    // Init tree in memory
    CurveTreesGlobalTree global_tree(*curve_trees);
    const auto new_outputs = generate_random_outputs(*curve_trees, 0, min_leaves_needed_for_tree_depth);
    ASSERT_TRUE(global_tree.grow_tree(0, min_leaves_needed_for_tree_depth, new_outputs.outputs));

    LOG_PRINT_L1("Finished initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    const auto tree_root = global_tree.get_tree_root();

    // Create proof for every leaf in the tree
    for (std::size_t leaf_idx = 0; leaf_idx < global_tree.get_n_leaf_tuples(); ++leaf_idx)
    {
        const auto path = global_tree.get_path_at_leaf_idx(leaf_idx);
        const std::size_t output_idx = leaf_idx % curve_trees->m_c2_width;

        const auto x = (fcmp_pp::tower_cycle::Ed25519Scalar) new_outputs.x_vec[leaf_idx].data;
        const auto y = (fcmp_pp::tower_cycle::Ed25519Scalar) new_outputs.y_vec[leaf_idx].data;

        // Leaves
        std::vector<fcmp_pp::tower_cycle::OutputBytes> output_bytes;
        output_bytes.reserve(path.leaves.size());
        for (const auto &leaf : path.leaves)
        {
            output_bytes.push_back({
                    .O_bytes = (uint8_t *)&leaf.O.bytes,
                    .I_bytes = (uint8_t *)&leaf.I.bytes,
                    .C_bytes = (uint8_t *)&leaf.C.bytes,
                });
        }
        const fcmp_pp::tower_cycle::OutputChunk leaves{output_bytes.data(), output_bytes.size()};

        // c1 scalars from c2 points
        std::vector<std::vector<fcmp_pp::tower_cycle::HeliosScalar>> c1_layer_scalars;
        std::vector<fcmp_pp::tower_cycle::Helios::Chunk> c1_chunks;
        for (const auto &c2_points : path.c2_layers)
        {
            c1_layer_scalars.emplace_back();
            auto &c1_layer = c1_layer_scalars.back();
            c1_layer.reserve(c2_points.size());
            for (const auto &c2_point : c2_points)
                c1_layer.emplace_back(curve_trees->m_c2->point_to_cycle_scalar(c2_point));
            // Padding with 0's
            for (std::size_t i = c2_points.size(); i < curve_trees->m_c2_width; ++i)
                c1_layer.emplace_back(curve_trees->m_c1->zero_scalar());
            c1_chunks.emplace_back(fcmp_pp::tower_cycle::Helios::Chunk{c1_layer.data(), c1_layer.size()});
        }
        const Helios::ScalarChunks c1_layer_chunks{c1_chunks.data(), c1_chunks.size()};

        // c2 scalars from c1 points
        std::vector<std::vector<fcmp_pp::tower_cycle::SeleneScalar>> c2_layer_scalars;
        std::vector<fcmp_pp::tower_cycle::Selene::Chunk> c2_chunks;
        for (const auto &c1_points : path.c1_layers)
        {
            c2_layer_scalars.emplace_back();
            auto &c2_layer = c2_layer_scalars.back();
            c2_layer.reserve(c1_points.size());
            for (const auto &c1_point : c1_points)
                c2_layer.emplace_back(curve_trees->m_c1->point_to_cycle_scalar(c1_point));
            // Padding with 0's
            for (std::size_t i = c1_points.size(); i < curve_trees->m_c1_width; ++i)
                c2_layer.emplace_back(curve_trees->m_c2->zero_scalar());
            c2_chunks.emplace_back(fcmp_pp::tower_cycle::Selene::Chunk{c2_layer.data(), c2_layer.size()});
        }
        const Selene::ScalarChunks c2_layer_chunks{c2_chunks.data(), c2_chunks.size()};

        const auto rerandomized_output = fcmp_pp::tower_cycle::rerandomize_output(output_bytes[output_idx]);

        // Collect blinds for rerandomized output
        const auto o_blind = fcmp_pp::tower_cycle::o_blind(rerandomized_output);
        const auto i_blind = fcmp_pp::tower_cycle::i_blind(rerandomized_output);
        const auto i_blind_blind = fcmp_pp::tower_cycle::i_blind_blind(rerandomized_output);
        const auto c_blind = fcmp_pp::tower_cycle::c_blind(rerandomized_output);

        const auto blinded_o_blind = fcmp_pp::tower_cycle::blind_o_blind(o_blind);
        const auto blinded_i_blind = fcmp_pp::tower_cycle::blind_i_blind(i_blind);
        const auto blinded_i_blind_blind = fcmp_pp::tower_cycle::blind_i_blind_blind(i_blind_blind);
        const auto blinded_c_blind = fcmp_pp::tower_cycle::blind_c_blind(c_blind);

        const auto output_blinds = fcmp_pp::tower_cycle::output_blinds_new(blinded_o_blind,
            blinded_i_blind,
            blinded_i_blind_blind,
            blinded_c_blind);

        // Collect new branch blinds
        const std::size_t n_helios_blinds = fcmp_pp::n_necessary_helios_blinds(1 + tree_depth);
        const std::size_t n_selene_blinds = fcmp_pp::n_necessary_selene_blinds(1 + tree_depth);

        std::vector<fcmp_pp::tower_cycle::BranchBlind> helios_branch_blinds;
        for (std::size_t i = 0; i < n_helios_blinds; ++i)
            helios_branch_blinds.emplace_back(fcmp_pp::tower_cycle::helios_branch_blind());

        std::vector<fcmp_pp::tower_cycle::BranchBlind> selene_branch_blinds;
        for (std::size_t i = 0; i < n_selene_blinds; ++i)
            selene_branch_blinds.emplace_back(fcmp_pp::tower_cycle::selene_branch_blind());

        fcmp_pp::tower_cycle::prove(x,
                y,
                output_idx,
                leaves,
                c1_layer_chunks,
                c2_layer_chunks,
                rerandomized_output,
                output_blinds,
                {helios_branch_blinds.data(), helios_branch_blinds.size()},
                {selene_branch_blinds.data(), selene_branch_blinds.size()},
                tree_root
            );
    }
}
//----------------------------------------------------------------------------------------------------------------------
