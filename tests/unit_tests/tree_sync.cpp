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
#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/tree_sync.h"

//----------------------------------------------------------------------------------------------------------------------
#define INIT_SYNC_TEST(tree_depth)                                                                            \
    static const std::size_t helios_chunk_width = 3;                                                          \
    static const std::size_t selene_chunk_width = 2;                                                          \
                                                                                                              \
    uint64_t n_leaves_needed = 0;                                                                             \
    auto curve_trees = test::init_curve_trees_test(helios_chunk_width,                                        \
        selene_chunk_width,                                                                                   \
        tree_depth,                                                                                           \
        n_leaves_needed);                                                                                     \
                                                                                                              \
    /* Each block, we'll sync a max of just over 2 full chunks, to make sure we're saving all path elems even \
       when the data we need is not in the last chunk */                                                      \
    static const std::size_t max_outputs_per_block = (2*selene_chunk_width)+1;                                \
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, register_output)
{
    // 1. Init
    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);

    const std::size_t INIT_LEAVES = 10;
    auto outputs = test::generate_random_outputs(*curve_trees, 0, INIT_LEAVES);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == INIT_LEAVES, "unexpected size of outputs");

    // Mock values
    const uint64_t block_idx = 0;
    const uint64_t unlock_block_idx = 0;

    const auto output = outputs[0].output_pair;

    // 2. Register output - valid
    ASSERT_TRUE(tree_sync->register_output(output, unlock_block_idx));

    // 3. Register same output again - already registered
    ASSERT_FALSE(tree_sync->register_output(output, unlock_block_idx));

    // 4. Register another output with the same output pubkey as existing, different commitment - valid
    auto output_new_commitment = output;
    output_new_commitment.commitment = outputs[1].output_pair.commitment;

    ASSERT_EQ(output_new_commitment.output_pubkey, output.output_pubkey);
    ASSERT_NE(output_new_commitment.commitment, output.commitment);

    ASSERT_TRUE(tree_sync->register_output(output_new_commitment, unlock_block_idx));

    // 5. Sync the block of outputs
    tree_sync->sync_block(block_idx,
        crypto::hash{0x01}/*block_hash*/,
        crypto::hash{}/*prev_block_hash*/,
        std::move(outputs));

    // 6. Register a new output where we already synced the block output unlocks in - expect throw
    const auto &new_output = test::generate_random_outputs(*curve_trees, INIT_LEAVES, 1).front().output_pair;
    EXPECT_ANY_THROW(tree_sync->register_output(new_output, unlock_block_idx));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, sync_block_simple)
{
    // 1. Init
    static const std::size_t INIT_LEAVES = 10;

    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);

    auto outputs = test::generate_random_outputs(*curve_trees, 0, INIT_LEAVES);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == INIT_LEAVES, "unexpected size of outputs");

    // Mock values
    const uint64_t block_idx = 0;
    const uint64_t unlock_block_idx = 0;

    const auto output = outputs[0].output_pair;

    // 2. Register output
    ASSERT_TRUE(tree_sync->register_output(output, unlock_block_idx));

    // 3. Sync the block of outputs
    tree_sync->sync_block(block_idx,
        crypto::hash{0x01}/*block_hash*/,
        crypto::hash{}/*prev_block_hash*/,
        std::move(outputs));

    // 4. Get the output's path in the tree
    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_sync->get_output_path(output, output_path));

    // If the c2 layer chunk width < INIT_LEAVES, the test won't use expected values below
    ASSERT_GE(curve_trees->m_c2_width, INIT_LEAVES);
    ASSERT_EQ(output_path.leaves.size(), INIT_LEAVES);
    ASSERT_TRUE(curve_trees->audit_path(output_path, output, INIT_LEAVES));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, sync_n_blocks_register_n_outputs)
{
    // Init
    static const std::size_t tree_depth = 6;
    INIT_SYNC_TEST(tree_depth);

    // Sync until we've synced all the leaves needed to get to the desired tree depth
    auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);
    uint64_t block_idx = 0;
    uint64_t n_outputs_synced = 0;
    crypto::hash prev_block_hash = crypto::hash{};

    // Keep track of all registered outputs so that we can make sure ALL output paths update correctly every block
    std::vector<fcmp_pp::curve_trees::OutputPair> registered_outputs;
    registered_outputs.reserve(n_leaves_needed);

    while (n_outputs_synced < n_leaves_needed)
    {
        const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
        LOG_PRINT_L1("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
            << " (" << (n_outputs_synced+sync_n_outputs) << " / " << n_leaves_needed << " outputs)");

        auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, sync_n_outputs);
        CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

        // Pick an output to register
        auto output_to_register = block_idx % sync_n_outputs;
        const auto output = outputs[output_to_register].output_pair;
        MDEBUG("Registering output " << n_outputs_synced + output_to_register);

        // Register the output
        ASSERT_TRUE(tree_sync->register_output(output, block_idx));
        registered_outputs.push_back(output);

        // Block hash
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(std::size_t), block_hash);

        // Sync the outputs generated above
        tree_sync->sync_block(block_idx,
            block_hash,
            prev_block_hash,
            std::move(outputs));

        n_outputs_synced += sync_n_outputs;

        // Audit all registered output paths
        for (const auto &o : registered_outputs)
        {
            CurveTreesV1::Path output_path;
            ASSERT_TRUE(tree_sync->get_output_path(o, output_path));
            ASSERT_TRUE(curve_trees->audit_path(output_path, o, n_outputs_synced));
        }

        // Update for next iteration
        prev_block_hash = block_hash;
        ++block_idx;
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, sync_n_blocks_register_one_output)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);

    // For every output, sync until the tree reaches the expected tree depth, registering 1 unique output each separate
    // sync. We audit the output path every block while syncing
    for (std::size_t i = 0; i < n_leaves_needed; ++i)
    {
        LOG_PRINT_L1("Register output " << (i+1) << " / " << n_leaves_needed);
        auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;

        crypto::hash prev_block_hash = crypto::hash{};

        uint64_t block_idx = 0;
        uint64_t n_outputs_synced = 0;
        while (n_outputs_synced < n_leaves_needed)
        {
            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
                << " (" << (n_outputs_synced+sync_n_outputs) << " / " << n_leaves_needed << " outputs)");

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Check if this chunk includes the output we're supposed to register
            if (n_outputs_synced <= i && i < (n_outputs_synced + sync_n_outputs))
            {
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs_synced;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_sync->register_output(output, block_idx));

                registered = true;
                registered_output = output;
            }

            // Block hash
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

            // Sync the outputs generated above
            tree_sync->sync_block(block_idx,
                block_hash,
                prev_block_hash,
                std::move(outputs));

            n_outputs_synced += sync_n_outputs;

            // Audit registered output path
            if (registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_sync->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_outputs_synced));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, sync_past_max_reorg_depth)
{
    // This test is useful for making sure TreeSync syncs correctly even after syncing past the reorg depth. Internally
    // the class drops cached values from beyond the reorg depth. This test makes sure registered output paths still
    // update correctly even after cached values from the tree get dropped.

    // Init
    static const std::size_t max_reorg_depth = 1;
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);

    // For every output, sync until the tree reaches the expected tree depth AND we sync past the max reorg depth,
    // registering 1 unique output each separate sync. We audit the output path every block while syncing
    for (std::size_t i = 0; i < n_leaves_needed; ++i)
    {
        LOG_PRINT_L1("Register output " << (i+1) << " / " << n_leaves_needed);

        // Sync until we've synced all the leaves needed to get to the desired tree depth
        auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees, max_reorg_depth);

        uint64_t block_idx = 0;
        uint64_t n_outputs_synced = 0;
        crypto::hash prev_block_hash = crypto::hash{};

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;

        while (n_outputs_synced < n_leaves_needed || block_idx <= max_reorg_depth)
        {
            // TODO: de-dup this code with above test
            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << block_idx);

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Check if this chunk includes the output we're supposed to register
            if (n_outputs_synced <= i && i < (n_outputs_synced + sync_n_outputs))
            {
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs_synced;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_sync->register_output(output, block_idx));

                registered = true;
                registered_output = output;
            }

            // Block hash
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

            // Sync the outputs generated above
            tree_sync->sync_block(block_idx,
                block_hash,
                prev_block_hash,
                std::move(outputs));

            n_outputs_synced += sync_n_outputs;

            // Audit registered output path
            if (registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_sync->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_outputs_synced));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, reorg_after_register)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);

    // For every output, sync until the tree reaches the expected tree depth, registering 1 unique output each separate
    // sync. We audit the output path every block while syncing
    for (std::size_t i = 0; i < n_leaves_needed; ++i)
    {
        LOG_PRINT_L1("Register output " << (i+1) << " / " << n_leaves_needed);
        auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;
        uint64_t included_block_idx = 0;
        crypto::hash included_block_hash;

        crypto::hash prev_block_hash = crypto::hash{};

        uint64_t block_idx = 0;
        uint64_t n_outputs_synced = 0;
        std::vector<uint64_t> n_outputs_synced_by_block;
        while (n_outputs_synced < n_leaves_needed)
        {
            if (registered && block_idx > (included_block_idx + 1))
            {
                uint64_t cur_block_idx = block_idx;
                ASSERT_EQ(n_outputs_synced_by_block.size(), block_idx);

                MDEBUG("Popping blocks back to block " << included_block_idx + 1 << " , then re-syncing");

                auto prev_n_outputs_synced = [&n_outputs_synced_by_block](uint64_t blk_idx) -> uint64_t
                    { return blk_idx == 0 ? 0 : n_outputs_synced_by_block[blk_idx - 1]; };

                // Pop blocks until the included block is the top block
                while (cur_block_idx > (included_block_idx + 1))
                {
                    ASSERT_TRUE(tree_sync->pop_block());
                    --cur_block_idx;

                    MDEBUG("cur_block_idx: " << cur_block_idx
                        << " , prev_n_outputs_synced(cur_block_idx): " << prev_n_outputs_synced(cur_block_idx));

                    CurveTreesV1::Path output_path;
                    ASSERT_TRUE(tree_sync->get_output_path(registered_output, output_path));
                    ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, prev_n_outputs_synced(cur_block_idx)));
                }

                // Sync back up again until cur_block_idx == block_idx
                prev_block_hash = included_block_hash;
                while (cur_block_idx < block_idx)
                {
                    const auto sync_n_outputs = (cur_block_idx % max_outputs_per_block) + 1;
                    MDEBUG("Re-syncing "<< sync_n_outputs << " outputs in block " << (cur_block_idx+1)
                        << " (" << (prev_n_outputs_synced(cur_block_idx)+sync_n_outputs) << " / " << n_leaves_needed << " outputs)");

                    auto outputs = test::generate_random_outputs(*curve_trees, prev_n_outputs_synced(cur_block_idx), sync_n_outputs);
                    CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

                    // Block metadata
                    crypto::hash block_hash;
                    crypto::cn_fast_hash(&cur_block_idx, sizeof(uint64_t), block_hash);

                    // Sync the outputs generated above
                    tree_sync->sync_block(cur_block_idx,
                        block_hash,
                        prev_block_hash,
                        std::move(outputs));
                    ++cur_block_idx;

                    CurveTreesV1::Path output_path;
                    ASSERT_TRUE(tree_sync->get_output_path(registered_output, output_path));
                    ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, prev_n_outputs_synced(cur_block_idx)));

                    prev_block_hash = block_hash;
                }
            }

            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
                << " (" << (n_outputs_synced+sync_n_outputs) << " / " << n_leaves_needed << " outputs)");

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Block metadata
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

            // Check if this chunk includes the output we're supposed to register
            if (n_outputs_synced <= i && i < (n_outputs_synced + sync_n_outputs))
            {
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs_synced;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_sync->register_output(output, block_idx));

                registered = true;
                registered_output = output;
                included_block_idx = block_idx;
                included_block_hash = block_hash;
            }

            // Sync the outputs generated above
            tree_sync->sync_block(block_idx,
                block_hash,
                prev_block_hash,
                std::move(outputs));

            n_outputs_synced += sync_n_outputs;
            n_outputs_synced_by_block.push_back(n_outputs_synced);

            // Audit registered output path
            if (registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_sync->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_outputs_synced));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_sync, register_after_reorg)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);
    auto tree_sync = new fcmp_pp::curve_trees::TreeSync<Helios, Selene>(curve_trees);

    // Sync until we reach expected tree depth
    uint64_t block_idx = 0;
    uint64_t n_outputs_synced = 0;
    std::vector<uint64_t> n_outputs_synced_by_block;
    std::vector<crypto::hash> block_hashes;
    while (n_outputs_synced < n_leaves_needed)
    {
        const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
        LOG_PRINT_L1("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
            << " (" << (n_outputs_synced+sync_n_outputs) << " / " << n_leaves_needed << " outputs)");

        auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, sync_n_outputs);
        CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

        // Block metadata
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

        // Sync the outputs generated above
        tree_sync->sync_block(block_idx,
            block_hash,
            block_hashes.empty() ? crypto::hash{} : block_hashes.back(),
            std::move(outputs));

        n_outputs_synced += sync_n_outputs;
        n_outputs_synced_by_block.push_back(n_outputs_synced);
        block_hashes.push_back(block_hash);

        // Update for next iteration
        ++block_idx;
    }

    // Reorg 1 block
    // TODO: test reorg 1 block to block_idx blocks
    LOG_PRINT_L1("Popping 1 block");
    ASSERT_TRUE(tree_sync->pop_block());
    --block_idx;
    block_hashes.pop_back();
    n_outputs_synced_by_block.pop_back();
    n_outputs_synced = n_outputs_synced_by_block.back();

    // Register output and sync it in the next block
    LOG_PRINT_L1("Registering 1 output and syncing in next block");
    auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_synced, 1);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == 1, "unexpected size of outputs");

    const auto output = outputs[0].output_pair;
    ASSERT_TRUE(tree_sync->register_output(output, block_idx));

    // Block metadata
    crypto::hash block_hash;
    crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

    // Sync the output generated above
    tree_sync->sync_block(block_idx,
        block_hash,
        block_hashes.empty() ? crypto::hash{} : block_hashes.back(),
        std::move(outputs));

    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_sync->get_output_path(output, output_path));
    ASSERT_TRUE(curve_trees->audit_path(output_path, output, n_outputs_synced+1));
}
//----------------------------------------------------------------------------------------------------------------------
// TODO: test edge cases: duplicate output when syncing, mismatched prev block hash in sync_block
// TODO: clean up code
//----------------------------------------------------------------------------------------------------------------------
