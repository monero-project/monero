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

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "curve_trees.h"
#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/tree_cache.h"
#include "serialization/binary_utils.h"
#include "string_tools.h"

//----------------------------------------------------------------------------------------------------------------------
#define INIT_SYNC_TEST(tree_depth)                                                                            \
    static const std::size_t selene_chunk_width = 2;                                                          \
    static const std::size_t helios_chunk_width = 3;                                                          \
                                                                                                              \
    uint64_t n_leaves_needed = 0;                                                                             \
    auto curve_trees = test::init_curve_trees_test(selene_chunk_width,                                        \
        helios_chunk_width,                                                                                   \
        tree_depth,                                                                                           \
        n_leaves_needed);                                                                                     \
                                                                                                              \
    /* Each block, we'll sync a max of just over 2 full chunks, to make sure we're saving all path elems even \
       when the data we need is not in the last chunk */                                                      \
    static const std::size_t max_outputs_per_block = (2*selene_chunk_width)+1;                                \
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, register_output)
{
    // 1. Init
    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

    const std::size_t INIT_LEAVES = 10;
    auto outputs = test::generate_random_outputs(*curve_trees, 0, INIT_LEAVES);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == INIT_LEAVES, "unexpected size of outputs");

    // Mock values
    const uint64_t last_locked_block_idx = 1;
    const auto output = outputs[0].output_pair;

    // 2. Register output - valid
    ASSERT_TRUE(tree_cache->register_output(output));

    // 3. Register same output again - already registered
    ASSERT_FALSE(tree_cache->register_output(output));

    // 4. Register another output with the same output pubkey as existing, different commitment - valid
    auto output_new_commitment = output;
    output_new_commitment.commitment = outputs[1].output_pair.commitment;

    ASSERT_EQ(output_new_commitment.output_pubkey, output.output_pubkey);
    ASSERT_NE(output_new_commitment.commitment, output.commitment);

    ASSERT_TRUE(tree_cache->register_output(output_new_commitment));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, sync_block_simple)
{
    // 1. Init
    static const std::size_t INIT_LEAVES = 10;

    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

    auto outputs = test::generate_random_outputs(*curve_trees, 0, INIT_LEAVES);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == INIT_LEAVES, "unexpected size of outputs");

    // Mock values
    const uint64_t last_locked_block_idx = 1;
    const auto output = outputs[0].output_pair;

    // 2. Register output
    ASSERT_TRUE(tree_cache->register_output(output));

    // 3. Sync the block of outputs
    crypto::hash block_hash{0x01};
    crypto::hash prev_block_hash{};
    tree_cache->sync_block(0, block_hash, prev_block_hash, {{ last_locked_block_idx, outputs }});

    // 4. Sync 1 more block so the outputs unlock and enter the tree
    prev_block_hash = block_hash;
    block_hash = crypto::hash{0x02};
    tree_cache->sync_block(last_locked_block_idx, block_hash, prev_block_hash, {});

    // 5. Get the output's path in the tree
    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_cache->get_output_path(output, output_path));

    // If the c1 layer chunk width < INIT_LEAVES, the test won't use expected values below
    ASSERT_GE(curve_trees->m_c1_width, INIT_LEAVES);
    ASSERT_EQ(output_path.leaves.size(), INIT_LEAVES);
    ASSERT_TRUE(curve_trees->audit_path(output_path, output, INIT_LEAVES));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, sync_n_chunks_of_blocks)
{
    // 1. Init
    static const std::size_t INIT_LEAVES = 10;
    static const std::size_t N_CHUNKS = 10;
    static const std::size_t N_BLOCKS_PER_CHUNK = 100;

    const std::size_t rand_chunk = crypto::rand_idx(N_CHUNKS);
    const std::size_t rand_block = crypto::rand_idx(N_BLOCKS_PER_CHUNK - 1);
    const std::size_t rand_leaf = crypto::rand_idx(INIT_LEAVES);

    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

    const crypto::hash mock_block_hash{};

    // 2. Make chunks of blocks
    std::vector<std::vector<crypto::hash>> chunks_of_block_hashes;
    std::vector<std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock>> chunks_of_outputs;
    std::size_t leaf_count = 0;
    uint64_t last_locked_block_idx = 0;
    fcmp_pp::curve_trees::OutputPair output;
    for (std::size_t i = 0; i < N_CHUNKS; ++i)
    {
        std::vector<crypto::hash> block_hashes;
        std::vector<fcmp_pp::curve_trees::OutsByLastLockedBlock> outs;
        for (std::size_t j = 0; j < N_BLOCKS_PER_CHUNK; ++j)
        {
            auto outputs = test::generate_random_outputs(*curve_trees, leaf_count, INIT_LEAVES);
            leaf_count += INIT_LEAVES;
            const auto last_locked_block = 1 + i * N_BLOCKS_PER_CHUNK + j;

            if (i == rand_chunk && j == rand_block)
            {
                last_locked_block_idx = last_locked_block;
                output = outputs[rand_leaf].output_pair;
            }

            outs.push_back({{ last_locked_block, std::move(outputs) }});
            block_hashes.push_back(mock_block_hash);
        }
        chunks_of_block_hashes.push_back(std::move(block_hashes));
        chunks_of_outputs.push_back(std::move(outs));
    }
    ASSERT_TRUE(last_locked_block_idx > 0);

    // 3. Register output
    ASSERT_TRUE(tree_cache->register_output(output));

    // 4. Sync the chunks of blocks
    for (std::size_t i = 0; i < N_CHUNKS; ++i)
    {
        const uint64_t start_block_idx = i * N_BLOCKS_PER_CHUNK;

        fcmp_pp::curve_trees::TreeCache<Selene, Helios>::CacheStateChange cache_state_change;

        tree_cache->prepare_to_grow_cache(start_block_idx,
            mock_block_hash,
            chunks_of_block_hashes[i],
            chunks_of_outputs[i],
            cache_state_change);

        tree_cache->grow_cache(start_block_idx,
            chunks_of_block_hashes[i],
            std::move(cache_state_change));

        // Audit the output's path in the tree
        CurveTreesV1::Path output_path;
        ASSERT_TRUE(tree_cache->get_output_path(output, output_path));
        if (!output_path.empty())
        {
            ASSERT_TRUE(curve_trees->audit_path(output_path, output, tree_cache->get_n_leaf_tuples()));
        }
    }

    // 5. Make sure the output was actually added to the tree
    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_cache->get_output_path(output, output_path));
    ASSERT_TRUE(!output_path.empty());
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, sync_n_blocks_register_n_outputs)
{
    // Init
    static const std::size_t tree_depth = 6;
    INIT_SYNC_TEST(tree_depth);

    // Sync until we've synced all the leaves needed to get to the desired tree depth
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);
    uint64_t block_idx = 0;
    uint64_t n_outputs = 0;
    uint64_t n_unlocked_outputs = 0;
    crypto::hash prev_block_hash = crypto::hash{};

    // Keep track of all registered outputs so that we can make sure ALL output paths update correctly every block
    std::vector<fcmp_pp::curve_trees::OutputPair> registered_outputs;
    registered_outputs.reserve(n_leaves_needed);

    while (n_unlocked_outputs < n_leaves_needed)
    {
        const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
        LOG_PRINT_L1("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
            << " (" << n_unlocked_outputs << " unlocked / " << n_leaves_needed << " outputs)");

        auto outputs = test::generate_random_outputs(*curve_trees, n_outputs, sync_n_outputs);
        CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

        // Pick an output to register
        auto output_to_register = block_idx % sync_n_outputs;
        const auto output = outputs[output_to_register].output_pair;
        MDEBUG("Registering output " << n_outputs + output_to_register);

        // Register the output
        const uint64_t last_locked_block_idx = block_idx + 1;
        ASSERT_TRUE(tree_cache->register_output(output));

        // Sync the outputs generated above
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(std::size_t), block_hash);
        tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {{ last_locked_block_idx, outputs }});

        n_unlocked_outputs = n_outputs;
        n_outputs += sync_n_outputs;

        // Audit all registered output paths
        for (const auto &o : registered_outputs)
        {
            CurveTreesV1::Path output_path;
            ASSERT_TRUE(tree_cache->get_output_path(o, output_path));
            ASSERT_TRUE(curve_trees->audit_path(output_path, o, n_unlocked_outputs));
        }

        // Update for next iteration
        registered_outputs.push_back(output);
        prev_block_hash = block_hash;
        ++block_idx;
    }

    // Sync 1 more empty block so all outputs unlock
    crypto::hash block_hash;
    crypto::cn_fast_hash(&block_idx, sizeof(std::size_t), block_hash);
    tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {});

    // Check all registered output paths
    for (const auto &o : registered_outputs)
    {
        CurveTreesV1::Path output_path;
        ASSERT_TRUE(tree_cache->get_output_path(o, output_path));
        ASSERT_TRUE(curve_trees->audit_path(output_path, o, n_unlocked_outputs));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, sync_n_blocks_register_one_output)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);

    // For every output, sync until the tree reaches the expected tree depth, registering 1 unique output each separate
    // sync. We audit the output path every block while syncing
    for (std::size_t i = 0; i < n_leaves_needed; ++i)
    {
        LOG_PRINT_L1("Register output " << (i+1) << " / " << n_leaves_needed);
        auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;

        crypto::hash prev_block_hash = crypto::hash{};

        uint64_t block_idx = 0;
        uint64_t n_outputs = 0;
        uint64_t n_unlocked_outputs = 0;
        while (n_unlocked_outputs < n_leaves_needed)
        {
            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
                << " (" << n_unlocked_outputs << " unlocked / " << n_leaves_needed << " outputs)");

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Check if this chunk includes the output we're supposed to register
            const uint64_t last_locked_block_idx = block_idx + 1;
            bool just_registered = n_outputs <= i && i < (n_outputs + sync_n_outputs);
            if (just_registered)
            {
                MDEBUG("Registering output");
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_cache->register_output(output));

                registered = true;
                registered_output = output;
            }

            // Sync the outputs generated above
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);
            tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {{ last_locked_block_idx, outputs }});

            n_unlocked_outputs = n_outputs;
            n_outputs += sync_n_outputs;

            // Audit registered output path
            if (registered && !just_registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_unlocked_outputs));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);

        // Sync 1 more empty block so all outputs unlock
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(std::size_t), block_hash);
        tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {});

        CurveTreesV1::Path output_path;
        ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
        ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_unlocked_outputs));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, sync_past_max_reorg_depth)
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
        auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees, max_reorg_depth);

        uint64_t block_idx = 0;
        uint64_t n_outputs = 0;
        uint64_t n_unlocked_outputs = 0;
        crypto::hash prev_block_hash = crypto::hash{};

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;

        while (n_unlocked_outputs < n_leaves_needed || block_idx <= max_reorg_depth)
        {
            // TODO: de-dup this code with above test
            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << block_idx);

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Check if this chunk includes the output we're supposed to register
            const uint64_t last_locked_block_idx = block_idx + 1;
            const bool just_registered = n_outputs <= i && i < (n_outputs + sync_n_outputs);
            if (just_registered)
            {
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_cache->register_output(output));

                registered = true;
                registered_output = output;
            }

            // Sync the outputs generated above
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);
            tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {{ last_locked_block_idx, outputs }});

            n_unlocked_outputs = n_outputs;
            n_outputs += sync_n_outputs;

            // Audit registered output path
            if (registered && !just_registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_unlocked_outputs));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);

        // Sync 1 more empty block so all outputs unlock
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(std::size_t), block_hash);
        tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {});

        CurveTreesV1::Path output_path;
        ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
        ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_unlocked_outputs));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, reorg_after_register)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);

    // For every output, sync until the tree reaches the expected tree depth, registering 1 unique output each separate
    // sync. We audit the output path every block while syncing
    for (std::size_t i = 0; i < n_leaves_needed; ++i)
    {
        LOG_PRINT_L1("Register output " << (i+1) << " / " << n_leaves_needed);
        auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

        fcmp_pp::curve_trees::OutputPair registered_output;
        bool registered = false;
        uint64_t last_locked_block_idx = 0;
        crypto::hash last_locked_block_hash;

        crypto::hash prev_block_hash = crypto::hash{};

        uint64_t block_idx = 0;
        uint64_t n_outputs = 0;
        uint64_t n_unlocked_outputs = 0;

        std::vector<uint64_t> n_outputs_synced_by_block;
        while (n_unlocked_outputs < n_leaves_needed)
        {
            if (registered && block_idx > (last_locked_block_idx + 1))
            {
                uint64_t cur_block_idx = block_idx;
                ASSERT_EQ(n_outputs_synced_by_block.size(), block_idx);

                LOG_PRINT_L1("Popping blocks back to block " << last_locked_block_idx + 1 << " , then re-syncing");

                auto n_outputs_unlocked = [&n_outputs_synced_by_block](uint64_t blk_idx) -> uint64_t
                    { return blk_idx < 2 ? 0 : n_outputs_synced_by_block[blk_idx - 2]; };

                // Pop blocks until the included block is the top block
                while (cur_block_idx > (last_locked_block_idx + 1))
                {
                    ASSERT_TRUE(tree_cache->pop_block());
                    --cur_block_idx;

                    MDEBUG("cur_block_idx: " << cur_block_idx
                        << " , n_outputs_unlocked(cur_block_idx): " << n_outputs_unlocked(cur_block_idx));

                    CurveTreesV1::Path output_path;
                    ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
                    ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_outputs_unlocked(cur_block_idx)));
                }

                // Sync back up again until cur_block_idx == block_idx
                prev_block_hash = last_locked_block_hash;
                while (cur_block_idx < block_idx)
                {
                    const auto sync_n_outputs = (cur_block_idx % max_outputs_per_block) + 1;
                    MDEBUG("Re-syncing "<< sync_n_outputs << " outputs in block " << (cur_block_idx+1)
                        << " (" << (n_outputs_unlocked(cur_block_idx)) << " unlocked / " << n_leaves_needed << " outputs)");

                    auto outputs = test::generate_random_outputs(*curve_trees, n_outputs_unlocked(cur_block_idx+1), sync_n_outputs);
                    CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

                    // Sync the outputs generated above
                    crypto::hash block_hash;
                    crypto::cn_fast_hash(&cur_block_idx, sizeof(uint64_t), block_hash);
                    tree_cache->sync_block(cur_block_idx,
                        block_hash,
                        prev_block_hash,
                        {{ cur_block_idx + 1, outputs }});
                    ++cur_block_idx;

                    CurveTreesV1::Path output_path;
                    ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
                    ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_outputs_unlocked(cur_block_idx)));

                    prev_block_hash = block_hash;
                }
            }

            const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
            MDEBUG("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
                << " (" << n_unlocked_outputs << " unlocked / " << n_leaves_needed << " outputs)");

            auto outputs = test::generate_random_outputs(*curve_trees, n_outputs, sync_n_outputs);
            CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

            // Block metadata
            crypto::hash block_hash;
            crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

            // Check if this chunk includes the output we're supposed to register
            const bool just_registered = n_outputs <= i && i < (n_outputs + sync_n_outputs);
            if (just_registered)
            {
                ASSERT_FALSE(registered);

                auto output_to_register = i - n_outputs;
                const auto output = outputs[output_to_register].output_pair;

                ASSERT_TRUE(tree_cache->register_output(output));

                registered = true;
                registered_output = output;
                last_locked_block_idx = block_idx + 1;
            }

            if (registered && block_idx == last_locked_block_idx)
                last_locked_block_hash = block_hash;

            // Sync the outputs generated above
            tree_cache->sync_block(block_idx,
                block_hash,
                prev_block_hash,
                {{ block_idx + 1, outputs }});

            n_unlocked_outputs = n_outputs;
            n_outputs += sync_n_outputs;
            n_outputs_synced_by_block.push_back(n_outputs);

            // Audit registered output path
            if (registered && !just_registered)
            {
                CurveTreesV1::Path output_path;
                ASSERT_TRUE(tree_cache->get_output_path(registered_output, output_path));
                ASSERT_TRUE(curve_trees->audit_path(output_path, registered_output, n_unlocked_outputs));
            }

            // Update for next iteration
            prev_block_hash = block_hash;
            ++block_idx;
        }

        ASSERT_TRUE(registered);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, register_after_reorg)
{
    // Init
    static const std::size_t tree_depth = 5;
    INIT_SYNC_TEST(tree_depth);
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);

    // Sync until we reach expected tree depth
    uint64_t block_idx = 0;
    uint64_t n_outputs = 0;
    uint64_t n_unlocked_outputs = 0;
    std::vector<uint64_t> n_outputs_synced_by_block;
    std::vector<crypto::hash> block_hashes;
    while (n_unlocked_outputs < n_leaves_needed)
    {
        const auto sync_n_outputs = (block_idx % max_outputs_per_block) + 1;
        LOG_PRINT_L1("Syncing "<< sync_n_outputs << " outputs in block " << (block_idx+1)
            << " (" << n_unlocked_outputs << " unlocked / " << n_leaves_needed << " outputs)");

        auto outputs = test::generate_random_outputs(*curve_trees, n_outputs, sync_n_outputs);
        CHECK_AND_ASSERT_THROW_MES(outputs.size() == sync_n_outputs, "unexpected size of outputs");

        // Block metadata
        crypto::hash block_hash;
        crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

        // Sync the outputs generated above
        const uint64_t last_locked_block_idx = block_idx + 1;
        tree_cache->sync_block(block_idx,
            block_hash,
            block_hashes.empty() ? crypto::hash{} : block_hashes.back(),
            {{ last_locked_block_idx, outputs }});

        n_unlocked_outputs = n_outputs;
        n_outputs += sync_n_outputs;
        n_outputs_synced_by_block.push_back(n_outputs);
        block_hashes.push_back(block_hash);

        // Update for next iteration
        ++block_idx;
    }

    // Reorg 1 block
    // TODO: test reorg 1 block to block_idx blocks
    LOG_PRINT_L1("Popping 1 block");
    ASSERT_TRUE(tree_cache->pop_block());
    --block_idx;
    block_hashes.pop_back();
    n_outputs_synced_by_block.pop_back();
    n_outputs = n_outputs_synced_by_block.back();
    n_unlocked_outputs = n_outputs_synced_by_block[n_outputs_synced_by_block.size() - 2];

    // Register output and sync it in the next block
    LOG_PRINT_L1("Registering 1 output and syncing in next block");
    auto outputs = test::generate_random_outputs(*curve_trees, n_unlocked_outputs, 1);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == 1, "unexpected size of outputs");

    const auto output = outputs[0].output_pair;
    ASSERT_TRUE(tree_cache->register_output(output));

    // Block metadata
    crypto::hash block_hash;
    crypto::cn_fast_hash(&block_idx, sizeof(uint64_t), block_hash);

    // Sync the output generated above
    const uint64_t last_locked_block_idx = block_idx + 1;
    tree_cache->sync_block(block_idx,
        block_hash,
        block_hashes.empty() ? crypto::hash{} : block_hashes.back(),
        {{ last_locked_block_idx, outputs }});
    block_hashes.push_back(block_hash);
    n_unlocked_outputs = n_outputs;

    // Output is not expected to enter the tree until next block
    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_cache->get_output_path(output, output_path));
    ASSERT_TRUE(output_path.leaves.empty() && output_path.c1_layers.empty() && output_path.c2_layers.empty());

    // Sync 1 more block so the output unlocks and enters the tree
    crypto::cn_fast_hash(&last_locked_block_idx, sizeof(uint64_t), block_hash);
    tree_cache->sync_block(last_locked_block_idx, block_hash, block_hashes.back(), {});
    n_unlocked_outputs += outputs.size();

    ASSERT_TRUE(tree_cache->get_output_path(output, output_path));
    ASSERT_TRUE(curve_trees->audit_path(output_path, output, n_unlocked_outputs));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(tree_cache, serialization)
{
    // 1. Grow the tree with a registered output
    static const std::size_t INIT_LEAVES = 10;
    auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    auto tree_cache = new fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);
    auto outputs = test::generate_random_outputs(*curve_trees, 0, INIT_LEAVES);
    CHECK_AND_ASSERT_THROW_MES(outputs.size() == INIT_LEAVES, "unexpected size of outputs");

    const uint64_t block_idx = 0;
    const uint64_t last_locked_block_idx = 1;
    const auto output = outputs[0].output_pair;
    ASSERT_TRUE(tree_cache->register_output(output));

    crypto::hash block_hash{0x01};
    crypto::hash prev_block_hash{};
    tree_cache->sync_block(block_idx, block_hash, prev_block_hash, {{ last_locked_block_idx, outputs }});

    // 2. Serialize the tree_cache object
    std::string blob;
    ASSERT_TRUE(serialization::dump_binary(*tree_cache, blob));

    // 3. Make sure the output is present in the serialized string
    const std::string blob_hex = epee::string_tools::buff_to_hex_nodelimer(blob);
    ASSERT_TRUE(blob_hex.find(epee::string_tools::pod_to_hex(output.output_pubkey)) != std::string::npos);
    ASSERT_TRUE(blob_hex.find(epee::string_tools::pod_to_hex(output.commitment)) != std::string::npos);

    // 4. De-serialize the string into a new tree_cache2 object
    auto tree_cache2 = fcmp_pp::curve_trees::TreeCache<Selene, Helios>(curve_trees);
    ASSERT_TRUE(serialization::parse_binary(blob, tree_cache2));

    // 5. Sync 1 more block in tree_cache and tree_cache2 so the init outputs unlock and enter the trees
    prev_block_hash = block_hash;
    block_hash = crypto::hash{0x02};
    tree_cache->sync_block(last_locked_block_idx, block_hash, prev_block_hash, {});
    tree_cache2.sync_block(last_locked_block_idx, block_hash, prev_block_hash, {});

    // 6. Make sure output's path is the same across both tree_cache and tree_cache2
    CurveTreesV1::Path output_path;
    ASSERT_TRUE(tree_cache->get_output_path(output, output_path));
    ASSERT_TRUE(curve_trees->audit_path(output_path, output, INIT_LEAVES));

    CurveTreesV1::Path output_path2;
    ASSERT_TRUE(tree_cache2.get_output_path(output, output_path2));
    // Checking the roots match and auditing path suffices for now, TODO: implement constant time eq
    // ASSERT_EQ(output_path, output_path2);
    auto get_root_str = [&curve_trees](const CurveTreesV1::Path &path) -> std::string
    {
        return path.c1_layers.size() > path.c2_layers.size()
            ? curve_trees->m_c1->to_string(path.c1_layers.back().front())
            : curve_trees->m_c2->to_string(path.c2_layers.back().front());
    };
    const auto root_str = get_root_str(output_path);
    const auto root_str2 = get_root_str(output_path2);
    ASSERT_EQ(root_str, root_str2);
    ASSERT_TRUE(curve_trees->audit_path(output_path2, output, INIT_LEAVES));

    // 7. Validate output counts
    ASSERT_EQ(tree_cache->get_output_count(), INIT_LEAVES);
    ASSERT_EQ(tree_cache2.get_output_count(), INIT_LEAVES);
}
//----------------------------------------------------------------------------------------------------------------------
// TODO: test edge cases: duplicate output when syncing, mismatched prev block hash in sync_block
// TODO: clean up code
// TODO: test that there are no dangling refs, refs are gc'd as expected
//----------------------------------------------------------------------------------------------------------------------
