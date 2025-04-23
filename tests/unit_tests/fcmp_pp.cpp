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

#include "common/container_helpers.h"
#include "common/threadpool.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "curve_trees.h"
#include "fcmp_pp/proof_len.h"
#include "fcmp_pp/prove.h"
#include "fcmp_pp/tower_cycle.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

#include "crypto/crypto.h"
#include "crypto/generators.h"

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct OutputContextsAndKeys
{
    std::vector<crypto::secret_key> x_vec;
    std::vector<crypto::secret_key> y_vec;
    std::vector<fcmp_pp::curve_trees::OutputContext> outputs;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static rct::key derive_key_image_generator(const rct::key O)
{
    crypto::public_key I;
    crypto::derive_key_image_generator(rct::rct2pk(O), I);
    return rct::pk2rct(I);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void store_key(uint8_t b[32], const rct::key &k)
{
    memcpy(b, k.bytes, 32);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static rct::key load_key(const uint8_t b[32])
{
    rct::key k;
    memcpy(k.bytes, b, sizeof(k));
    return k;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static crypto::secret_key load_sk(const uint8_t b[32])
{
    crypto::secret_key sk;
    memcpy(sk.data, b, sizeof(sk));
    return sk;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static FcmpRerandomizedOutputCompressed rerandomize_output_manual(const rct::key &O, const rct::key &C)
{
    FcmpRerandomizedOutputCompressed res;

    // sample random r_o, r_i, r_r_i, r_c in [0, l)
    rct::key r_o = rct::skGen();
    rct::key r_i = rct::skGen();
    rct::key r_r_i = rct::skGen();
    rct::key r_c = rct::skGen();

    store_key(res.r_o, r_o);
    store_key(res.r_i, r_i);
    store_key(res.r_r_i, r_r_i);
    store_key(res.r_c, r_c);

    // O~ = O + r_o T
    rct::key O_tilde = rct::scalarmultKey(rct::pk2rct(crypto::get_T()), r_o);
    O_tilde = rct::addKeys(O_tilde, O);

    store_key(res.input.O_tilde, O_tilde);

    // I = Hp(O)
    // I~ = I + r_i U
    const rct::key I = derive_key_image_generator(O);
    rct::key I_tilde = rct::scalarmultKey(rct::pk2rct(crypto::get_U()), r_i);
    I_tilde = rct::addKeys(I_tilde, I);

    store_key(res.input.I_tilde, I_tilde);

    // precomp T
    const ge_p3 T_p3 = crypto::get_T_p3();
    ge_dsmp T_dsmp;
    ge_dsm_precomp(T_dsmp, &T_p3);

    // R = r_i V + r_r_i T
    rct::key R;
    rct::addKeys3(R, r_i, rct::pk2rct(crypto::get_V()), r_r_i, T_dsmp);

    store_key(res.input.R, R);

    // C~ = C + r_c G
    rct::key C_tilde;
    rct::addKeys1(C_tilde, r_c, C);

    store_key(res.input.C_tilde, C_tilde);

    return res;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static const OutputContextsAndKeys generate_random_outputs(const CurveTreesV1 &curve_trees,
    const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples)
{
    OutputContextsAndKeys outs;
    outs.x_vec.resize(new_n_leaf_tuples);
    outs.y_vec.resize(new_n_leaf_tuples);
    outs.outputs.resize(new_n_leaf_tuples);

    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    const std::size_t batch_size = 1 + (new_n_leaf_tuples / (std::size_t)tpool.get_max_concurrency());
    for (std::size_t i = 0; i < new_n_leaf_tuples; i+= batch_size)
    {
        const std::size_t end = std::min(i + batch_size, new_n_leaf_tuples);
        tpool.submit(&waiter, [&outs, i, end, old_n_leaf_tuples]()
        {
            for (std::size_t j = i; j < end; ++j)
            {
                const std::uint64_t output_id = old_n_leaf_tuples + j;

                // Generate random output tuple
                // Note: lock contention slows this down quite a bit
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

                outs.x_vec[j] = std::move(x);
                outs.y_vec[j] = std::move(y);
                outs.outputs[j] = std::move(output_context);
            }
        });
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "Failed generating random outputs");

    return outs;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, prove)
{
    static const std::size_t selene_chunk_width = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH;
    static const std::size_t helios_chunk_width = fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH;
    static const std::size_t tree_depth = 4;

    LOG_PRINT_L1("Test prove with selene chunk width " << selene_chunk_width
        << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth);

    uint64_t min_leaves_needed_for_tree_depth = 0;
    const auto curve_trees = test::init_curve_trees_test(selene_chunk_width,
        helios_chunk_width,
        tree_depth,
        min_leaves_needed_for_tree_depth);

    LOG_PRINT_L1("Initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    // Init tree in memory
    CurveTreesGlobalTree global_tree(*curve_trees);
    const auto new_outputs = generate_random_outputs(*curve_trees, 0, min_leaves_needed_for_tree_depth);
    LOG_PRINT_L1("Finished generating random outputs");
    ASSERT_TRUE(global_tree.grow_tree(0, min_leaves_needed_for_tree_depth, new_outputs.outputs, false/*audit_tree*/));

    LOG_PRINT_L1("Finished initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    const auto tree_root = global_tree.get_tree_root();

    // Keep them cached across runs
    std::vector<const uint8_t *> selene_branch_blinds;
    std::vector<const uint8_t *> helios_branch_blinds;

    CHECK_AND_ASSERT_THROW_MES(global_tree.get_n_leaf_tuples() >= FCMP_PLUS_PLUS_MAX_INPUTS, "too few leaves");

    // Create proofs with random leaf idxs for txs with [1..FCMP_PLUS_PLUS_MAX_INPUTS] inputs
    for (std::size_t n_inputs = 1; n_inputs <= FCMP_PLUS_PLUS_MAX_INPUTS; ++n_inputs)
    {
        std::vector<const uint8_t *> fcmp_prove_inputs;
        std::vector<crypto::key_image> key_images;
        std::vector<crypto::ec_point> pseudo_outs;

        std::unordered_set<std::size_t> selected_indices;

        while (fcmp_prove_inputs.size() < n_inputs)
        {
            // Generate a random unique leaf tuple index within the tree
            const size_t leaf_idx = crypto::rand_idx(global_tree.get_n_leaf_tuples());
            if (selected_indices.count(leaf_idx))
                continue;
            else
                selected_indices.insert(leaf_idx);

            LOG_PRINT_L1("Constructing proof inputs for leaf idx " << leaf_idx);

            const auto path = global_tree.get_path_at_leaf_idx(leaf_idx);
            const std::size_t output_idx = leaf_idx % curve_trees->m_c1_width;

            const fcmp_pp::curve_trees::OutputPair output_pair = {rct::rct2pk(path.leaves[output_idx].O), path.leaves[output_idx].C};
            const auto output_tuple = fcmp_pp::curve_trees::output_to_tuple(output_pair);

            // ASSERT_TRUE(curve_trees->audit_path(path, output_pair, global_tree.get_n_leaf_tuples()));
            // LOG_PRINT_L1("Passed the audit...\n");

            const auto x = (uint8_t *) new_outputs.x_vec[leaf_idx].data;
            const auto y = (uint8_t *) new_outputs.y_vec[leaf_idx].data;

            // Leaves
            const auto path_for_proof = curve_trees->path_for_proof(path, output_tuple);

            const FcmpRerandomizedOutputCompressed rerandomized_output = fcmp_pp::rerandomize_output(OutputBytes{
                .O_bytes = output_tuple.O.bytes,
                .I_bytes = output_tuple.I.bytes,
                .C_bytes = output_tuple.C.bytes
            });

            pseudo_outs.emplace_back(rct::rct2pt(load_key(rerandomized_output.input.C_tilde)));

            key_images.emplace_back();
            crypto::generate_key_image(rct::rct2pk(path.leaves[output_idx].O),
                new_outputs.x_vec[leaf_idx],
                key_images.back());

            // Set path
            const auto helios_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::HeliosT>(
                path_for_proof.c2_scalar_chunks);
            const auto selene_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::SeleneT>(
                path_for_proof.c1_scalar_chunks);

            const auto path_rust = fcmp_pp::path_new({path_for_proof.leaves.data(), path_for_proof.leaves.size()},
                path_for_proof.output_idx,
                {helios_scalar_chunks.data(), helios_scalar_chunks.size()},
                {selene_scalar_chunks.data(), selene_scalar_chunks.size()});

            // Collect blinds for rerandomized output
            const auto o_blind = fcmp_pp::o_blind(rerandomized_output);
            const auto i_blind = fcmp_pp::i_blind(rerandomized_output);
            const auto i_blind_blind = fcmp_pp::i_blind_blind(rerandomized_output);
            const auto c_blind = fcmp_pp::c_blind(rerandomized_output);

            const auto blinded_o_blind = fcmp_pp::blind_o_blind(o_blind);
            const auto blinded_i_blind = fcmp_pp::blind_i_blind(i_blind);
            const auto blinded_i_blind_blind = fcmp_pp::blind_i_blind_blind(i_blind_blind);
            const auto blinded_c_blind = fcmp_pp::blind_c_blind(c_blind);

            const auto output_blinds = fcmp_pp::output_blinds_new(blinded_o_blind,
                blinded_i_blind,
                blinded_i_blind_blind,
                blinded_c_blind);

            // Cache branch blinds
            if (selene_branch_blinds.empty())
                for (std::size_t i = 0; i < helios_scalar_chunks.size(); ++i)
                    selene_branch_blinds.emplace_back(fcmp_pp::selene_branch_blind());

            if (helios_branch_blinds.empty())
                for (std::size_t i = 0; i < selene_scalar_chunks.size(); ++i)
                    helios_branch_blinds.emplace_back(fcmp_pp::helios_branch_blind());

            auto fcmp_prove_input = fcmp_pp::fcmp_pp_prove_input_new(x,
                y,
                rerandomized_output,
                path_rust,
                output_blinds,
                selene_branch_blinds,
                helios_branch_blinds);

            fcmp_prove_inputs.emplace_back(std::move(fcmp_prove_input));

            // This test does not have outputs, but this is where this would go if it did
            // fcmp_pp::balance_last_pseudo_out(sum_output_masks, fcmp_prove_inputs);
        }

        LOG_PRINT_L1("Constructing proof and verifying (n_inputs=" << n_inputs << ")");
        const crypto::hash tx_hash{};
        const std::size_t n_layers = curve_trees->n_layers(global_tree.get_n_leaf_tuples());
        const auto proof = fcmp_pp::prove(
                tx_hash,
                fcmp_prove_inputs,
                n_layers
            );

        bool verify = fcmp_pp::verify(
                tx_hash,
                proof,
                n_layers,
                tree_root,
                pseudo_outs,
                key_images
            );
        ASSERT_TRUE(verify);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, sal_completeness)
{
    // O, I, C, L
    const rct::key x = rct::skGen();
    const rct::key y = rct::skGen();
    rct::key O;
    rct::addKeys2(O, x, y, rct::pk2rct(crypto::get_T())); // O = x G + y T
    const rct::key I = derive_key_image_generator(O);
    const rct::key C = rct::pkGen();
    crypto::key_image L;
    crypto::generate_key_image(rct::rct2pk(O), rct::rct2sk(x), L);

    // Rerandomize
    const FcmpRerandomizedOutputCompressed rerandomized_output{fcmp_pp::rerandomize_output(fcmp_pp::OutputBytes{
        .O_bytes = O.bytes,
        .I_bytes = I.bytes,
        .C_bytes = C.bytes
    })};

    // Generate signable_tx_hash
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // Prove
    const std::pair<fcmp_pp::FcmpPpSalProof, crypto::key_image> sal_proof = fcmp_pp::prove_sal(
        signable_tx_hash,
        rct::rct2sk(x),
        rct::rct2sk(y),
        rerandomized_output);

    EXPECT_EQ(L, sal_proof.second);

    // Verify
    const bool ver = fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_output.input,
        L,
        sal_proof.first);

    EXPECT_TRUE(ver);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, membership_completeness)
{
    static const std::size_t MAX_NUM_INPUTS = 8;

    static const std::size_t selene_chunk_width = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH;
    static const std::size_t helios_chunk_width = fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH;
    static const std::size_t tree_depth = 3;
    static const std::size_t n_layers = 1 + tree_depth;

    LOG_PRINT_L1("Test prove with selene chunk width " << selene_chunk_width
        << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth);

    uint64_t min_leaves_needed_for_tree_depth = 0;
    const auto curve_trees = test::init_curve_trees_test(selene_chunk_width,
        helios_chunk_width,
        tree_depth,
        min_leaves_needed_for_tree_depth);

    LOG_PRINT_L1("Initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    // Init tree in memory
    CurveTreesGlobalTree global_tree(*curve_trees);
    const auto new_outputs = generate_random_outputs(*curve_trees, 0, min_leaves_needed_for_tree_depth);
    ASSERT_TRUE(global_tree.grow_tree(0, min_leaves_needed_for_tree_depth, new_outputs.outputs));

    LOG_PRINT_L1("Finished initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    const size_t num_tree_leaves = global_tree.get_n_leaf_tuples();

    // Make branch blinds once purely for performance reasons (DO NOT DO THIS IN PRODUCTION)
    const size_t expected_num_selene_branch_blinds = (tree_depth + 1) / 2;
    LOG_PRINT_L1("Calculating " << expected_num_selene_branch_blinds << " Selene branch blinds");
    std::vector<const uint8_t *> selene_branch_blinds;
    for (size_t i = 0; i < expected_num_selene_branch_blinds; ++i)
        selene_branch_blinds.emplace_back(fcmp_pp::selene_branch_blind());

    const size_t expected_num_helios_branch_blinds = tree_depth / 2;
    LOG_PRINT_L1("Calculating " << expected_num_helios_branch_blinds << " Helios branch blinds");
    std::vector<const uint8_t *> helios_branch_blinds;
    for (size_t i = 0; i < expected_num_helios_branch_blinds; ++i)
        helios_branch_blinds.emplace_back(fcmp_pp::helios_branch_blind());

    // For every supported input size...
    for (size_t num_inputs = 1; num_inputs <= MAX_NUM_INPUTS; ++num_inputs)
    {
        LOG_PRINT_L1("Starting " << num_inputs << "-in " << n_layers << "-layer test case");

        // Build up a set of `num_inputs` inputs to prove membership on
        ASSERT_LE(num_inputs, num_tree_leaves);
        std::set<size_t> selected_indices;
        std::vector<FcmpInputCompressed> fcmp_raw_inputs;
        fcmp_raw_inputs.reserve(num_inputs);
        std::vector<uint8_t*> fcmp_provable_inputs;
        fcmp_provable_inputs.reserve(num_inputs);
        while (selected_indices.size() < num_inputs)
        {
            // Generate a random unique leaf tuple index within the tree
            const size_t leaf_idx = crypto::rand_idx(num_tree_leaves);
            if (selected_indices.count(leaf_idx))
                continue;
            else
                selected_indices.insert(leaf_idx);

            // Fetch path
            const auto path = global_tree.get_path_at_leaf_idx(leaf_idx);
            const std::size_t output_idx = leaf_idx % curve_trees->m_c1_width;

            // Collect leaves in this path
            std::vector<fcmp_pp::OutputBytes> output_bytes;
            output_bytes.reserve(path.leaves.size());
            for (const auto &leaf : path.leaves)
            {
                output_bytes.push_back({
                        .O_bytes = (uint8_t *)&leaf.O.bytes,
                        .I_bytes = (uint8_t *)&leaf.I.bytes,
                        .C_bytes = (uint8_t *)&leaf.C.bytes,
                    });
            }
            const fcmp_pp::OutputChunk leaves{output_bytes.data(), output_bytes.size()};

            // selene scalars from helios points
            std::vector<std::vector<fcmp_pp::tower_cycle::Selene::Scalar>> selene_scalars;
            std::vector<fcmp_pp::tower_cycle::Selene::Chunk> selene_chunks;
            for (const auto &helios_points : path.c2_layers)
            {
                // Exclude the root
                if (helios_points.size() == 1)
                    break;
                selene_scalars.emplace_back();
                auto &selene_layer = selene_scalars.back();
                selene_layer.reserve(helios_points.size());
                for (const auto &c2_point : helios_points)
                    selene_layer.emplace_back(curve_trees->m_c2->point_to_cycle_scalar(c2_point));
                // Padding with 0's
                for (std::size_t i = helios_points.size(); i < curve_trees->m_c1_width; ++i)
                    selene_layer.emplace_back(curve_trees->m_c1->zero_scalar());
                selene_chunks.emplace_back(fcmp_pp::tower_cycle::Selene::Chunk{selene_layer.data(), selene_layer.size()});
            }
            const Selene::ScalarChunks selene_scalar_chunks{selene_chunks.data(), selene_chunks.size()};

            // helios scalars from selene points
            std::vector<std::vector<fcmp_pp::tower_cycle::Helios::Scalar>> helios_scalars;
            std::vector<fcmp_pp::tower_cycle::Helios::Chunk> helios_chunks;
            for (const auto &selene_points : path.c1_layers)
            {
                // Exclude the root
                if (selene_points.size() == 1)
                    break;
                helios_scalars.emplace_back();
                auto &helios_layer = helios_scalars.back();
                helios_layer.reserve(selene_points.size());
                for (const auto &c1_point : selene_points)
                    helios_layer.emplace_back(curve_trees->m_c1->point_to_cycle_scalar(c1_point));
                // Padding with 0's
                for (std::size_t i = selene_points.size(); i < curve_trees->m_c2_width; ++i)
                    helios_layer.emplace_back(curve_trees->m_c2->zero_scalar());
                helios_chunks.emplace_back(fcmp_pp::tower_cycle::Helios::Chunk{helios_layer.data(), helios_layer.size()});
            }
            const Helios::ScalarChunks helios_scalar_chunks{helios_chunks.data(), helios_chunks.size()};

            const auto path_rust = fcmp_pp::path_new(leaves,
                output_idx,
                helios_scalar_chunks,
                selene_scalar_chunks);

            // Rerandomize output. We use rerandomize_output_manual() here just to test out the U, V
            // generators and manually constructing a FcmpRerandomizedOutputCompressed. But
            // fcmp_pp::rerandomize_output() would work perfectly fine here as well, especially
            // since we're not balancing C~ and thus don't need to modify it.
            const FcmpRerandomizedOutputCompressed rerandomized_output = rerandomize_output_manual(
                path.leaves.at(output_idx).O,
                path.leaves.at(output_idx).C);

            // check the size of our precalculated branch blind cache
            ASSERT_EQ(helios_scalars.size(), expected_num_selene_branch_blinds);
            ASSERT_EQ(selene_scalars.size(), expected_num_helios_branch_blinds);

            // Calculate output blinds for rerandomized output
            LOG_PRINT_L1("Calculating output blind");
            const SeleneScalar o_blind = fcmp_pp::o_blind(rerandomized_output);
            const SeleneScalar i_blind = fcmp_pp::i_blind(rerandomized_output);
            const SeleneScalar i_blind_blind = fcmp_pp::i_blind_blind(rerandomized_output);
            const SeleneScalar c_blind = fcmp_pp::c_blind(rerandomized_output);

            const auto blinded_o_blind = fcmp_pp::blind_o_blind(o_blind);
            const auto blinded_i_blind = fcmp_pp::blind_i_blind(i_blind);
            const auto blinded_i_blind_blind = fcmp_pp::blind_i_blind_blind(i_blind_blind);
            const auto blinded_c_blind = fcmp_pp::blind_c_blind(c_blind);

            const auto output_blinds = fcmp_pp::output_blinds_new(blinded_o_blind,
                blinded_i_blind,
                blinded_i_blind_blind,
                blinded_c_blind);
            
            // make provable FCMP input
            fcmp_provable_inputs.push_back(fcmp_pp::fcmp_prove_input_new(rerandomized_output,
                path_rust,
                output_blinds,
                selene_branch_blinds,
                helios_branch_blinds));

            // get FCMP input
            fcmp_raw_inputs.push_back(rerandomized_output.input);

            // Dealloc
            free(blinded_o_blind);
            free(blinded_i_blind);
            free(blinded_i_blind_blind);
            free(blinded_c_blind);
            free(output_blinds);
        }

        ASSERT_EQ(fcmp_raw_inputs.size(), fcmp_provable_inputs.size());

        // Create FCMP proof
        LOG_PRINT_L1("Proving " << num_inputs << "-in " << n_layers << "-layer FCMP");
        const fcmp_pp::FcmpMembershipProof proof = fcmp_pp::prove_membership(fcmp_provable_inputs,
            n_layers);

        // Verify
        LOG_PRINT_L1("Verifying " << num_inputs << "-in " << n_layers << "-layer FCMP");
        EXPECT_TRUE(fcmp_pp::verify_membership(proof, n_layers, global_tree.get_tree_root(), fcmp_raw_inputs));

        // Dealloc
        for (uint8_t *input : fcmp_provable_inputs)
            free(input);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, force_init_gen_u_v)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Generator reproduction assert statements don't trigger on Release builds";
#endif

    const ge_p3 U_p3 = crypto::get_U_p3();
    const ge_p3 V_p3 = crypto::get_V_p3();
    const ge_cached U_cached = crypto::get_U_cached();
    const ge_cached V_cached = crypto::get_V_cached();

    (void) U_p3, (void) V_p3, (void) U_cached, (void) V_cached;
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, calculate_fcmp_input_for_rerandomizations_convergence)
{
    const crypto::public_key onetime_address = rct::rct2pk(rct::pkGen());
    const crypto::ec_point amount_commitment = rct::rct2pt(rct::pkGen());
    const rct::key I = derive_key_image_generator(rct::pk2rct(onetime_address));

    const FcmpRerandomizedOutputCompressed rerandomized_output = fcmp_pp::rerandomize_output(OutputBytes{
        .O_bytes = to_bytes(onetime_address),
        .I_bytes = I.bytes,
        .C_bytes = to_bytes(amount_commitment)
    });

    const FcmpInputCompressed recomputed_input = fcmp_pp::calculate_fcmp_input_for_rerandomizations(
        onetime_address,
        amount_commitment,
        load_sk(rerandomized_output.r_o),
        load_sk(rerandomized_output.r_i),
        load_sk(rerandomized_output.r_r_i),
        load_sk(rerandomized_output.r_c));

    EXPECT_EQ(0, memcmp(&rerandomized_output.input, &recomputed_input, sizeof(FcmpInputCompressed)));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, proof_size_table)
{
    for (std::size_t i = 1; i <= FCMP_PLUS_PLUS_MAX_INPUTS; ++i)
    {
        // Uncomment the prints to construct the table
        // printf("{");
        for (std::size_t j = 1; j <= FCMP_PLUS_PLUS_MAX_LAYERS; ++j)
        {
            const std::size_t membership_proof_len = ::_slow_membership_proof_size(i, j);
            const std::size_t fcmp_pp_proof_len = ::_slow_fcmp_pp_proof_size(i, j);

            EXPECT_EQ(fcmp_pp::membership_proof_len(i, j), membership_proof_len);
            EXPECT_EQ(fcmp_pp::fcmp_pp_proof_len(i, j), fcmp_pp_proof_len);

            // printf("%lu, ", membership_proof_len);
        }
        // printf("},\n");
    }
}
//----------------------------------------------------------------------------------------------------------------------
