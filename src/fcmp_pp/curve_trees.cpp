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

#include "curve_trees.h"

#include "common/threadpool.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

#include <algorithm>
#include <stdlib.h>

namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Instantiate the tower cycle types
template class CurveTrees<Selene, Helios>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Public helper functions
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
typename C::Point get_new_parent(const std::unique_ptr<C> &curve, const typename C::Chunk &new_children)
{
    for (std::size_t i = 0; i < new_children.len; ++i)
        MTRACE("Hashing " << curve->to_string(new_children.buf[i]));

    return curve->hash_grow(
            curve->hash_init_point(),
            0,/*offset*/
            curve->zero_scalar(),
            new_children
        );
};
template Selene::Point get_new_parent<Selene>(const std::unique_ptr<Selene> &curve,
    const typename Selene::Chunk &new_children);
template Helios::Point get_new_parent<Helios>(const std::unique_ptr<Helios> &curve,
    const typename Helios::Chunk &new_children);
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_to_tuple(const OutputPair &output_pair, bool torsion_checked, bool use_fast_check)
{
    const crypto::public_key &output_pubkey = output_pair.output_pubkey;
    const rct::key &commitment              = output_pair.commitment;

    const rct::key &O_key = rct::pk2rct(output_pubkey);
    const rct::key &C_key = commitment;

    rct::key O = O_key;
    rct::key C = C_key;

    // If the output has already been checked for torsion, then we don't need to clear torsion here
    // TODO: is there a cleaner torsion approach than this nested if statement?
    if (!torsion_checked)
    {
        TIME_MEASURE_NS_START(clear_torsion_ns);

        // Clear torsion on output if it wasn't already checked for torsion
        if (!use_fast_check)
        {
            if (!fcmp_pp::get_valid_torsion_cleared_point(O_key, O))
                throw std::runtime_error("O is invalid for insertion to tree");
            if (!fcmp_pp::get_valid_torsion_cleared_point(C_key, C))
                throw std::runtime_error("C is invalid for insertion to tree");
        }
        else
        {
            if (!fcmp_pp::get_valid_torsion_cleared_point_fast(O_key, O))
                throw std::runtime_error("O is invalid for insertion to tree");
            if (!fcmp_pp::get_valid_torsion_cleared_point_fast(C_key, C))
                throw std::runtime_error("C is invalid for insertion to tree");
        }

        if (O != O_key)
            LOG_PRINT_L2("Output pubkey has torsion: " << O_key);
        if (C != C_key)
            LOG_PRINT_L2("Commitment has torsion: " << C_key);

        TIME_MEASURE_NS_FINISH(clear_torsion_ns);

        LOG_PRINT_L3("clear_torsion_ns: " << clear_torsion_ns);
    }

#if !defined(NDEBUG)
    {
        // Debug build safety checks
        rct::key O_debug;
        rct::key C_debug;
        assert(fcmp_pp::get_valid_torsion_cleared_point(O_key, O_debug));
        assert(fcmp_pp::get_valid_torsion_cleared_point(C_key, C_debug));
        assert(O == O_debug);
        assert(C == C_debug);
    }
#endif

    // Redundant check for safety
    if (O == rct::I)
        throw std::runtime_error("O cannot equal identity");
    if (C == rct::I)
        throw std::runtime_error("C cannot equal identity");

    TIME_MEASURE_NS_START(derive_key_image_generator_ns);

    // Must use the original output pubkey to derive I to prevent double spends, since torsioned outputs yield a
    // a distinct I and key image from their respective torsion cleared output (and torsioned outputs are spendable
    // before fcmp++)
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, I);

    TIME_MEASURE_NS_FINISH(derive_key_image_generator_ns);

    LOG_PRINT_L3("derive_key_image_generator_ns: " << derive_key_image_generator_ns);

    rct::key I_rct = rct::pt2rct(I);

    return OutputTuple{
        .O = std::move(O),
        .I = std::move(I_rct),
        .C = std::move(C),
    };
}
//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<CurveTreesV1> curve_trees_v1(const std::size_t selene_chunk_width, const std::size_t helios_chunk_width)
{
    std::unique_ptr<Selene> selene(new Selene());
    std::unique_ptr<Helios> helios(new Helios());
    return std::shared_ptr<CurveTreesV1>(
            new CurveTreesV1(
                std::move(selene),
                std::move(helios),
                selene_chunk_width,
                helios_chunk_width
            )
        );
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Static functions
//----------------------------------------------------------------------------------------------------------------------
// After hashing a layer of children points, convert those children x-coordinates into their respective cycle
// scalars, and prepare them to be hashed for the next layer
template<typename C_CHILD, typename C_PARENT>
static std::vector<typename C_PARENT::Scalar> next_child_scalars_from_children(const std::unique_ptr<C_CHILD> &c_child,
    const typename C_CHILD::Point *last_root,
    const LayerExtension<C_CHILD> &children)
{
    std::vector<typename C_PARENT::Scalar> child_scalars_out;
    child_scalars_out.reserve(1 + children.hashes.size());

    // If we're creating a *new* root at the existing root layer, we may need to include the *existing* root when
    // hashing the *existing* root layer
    if (last_root != nullptr)
    {
        // If the children don't already include the existing root, then we need to include it to be hashed
        // - the children would include the existing root already if the existing root was updated in the child
        // layer (the start_idx would be 0)
        if (children.start_idx > 0)
        {
            MTRACE("Updating root layer and including the existing root in next children");
            child_scalars_out.emplace_back(c_child->point_to_cycle_scalar(*last_root));
        }
    }

    // Convert child points to scalars
    tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child, children.hashes, child_scalars_out);

    return child_scalars_out;
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void hash_first_chunk(const std::unique_ptr<C> &curve,
    const typename C::Scalar *old_last_child,
    const typename C::Point *old_last_parent,
    const std::size_t start_offset,
    const std::vector<typename C::Scalar> &new_child_scalars,
    const std::size_t chunk_size,
    typename C::Point &hash_out)
{
    // Prepare to hash
    const auto &existing_hash = old_last_parent != nullptr
        ? *old_last_parent
        : curve->hash_init_point();

    const auto &prior_child_after_offset = old_last_child != nullptr
        ? *old_last_child
        : curve->zero_scalar();

    const auto chunk_start = new_child_scalars.data();
    const typename C::Chunk chunk{chunk_start, chunk_size};

    MTRACE("First chunk existing_hash: " << curve->to_string(existing_hash) << " , start_offset: " << start_offset
        << " , prior_child_after_offset: " << curve->to_string(prior_child_after_offset));

    for (std::size_t i = 0; i < chunk_size; ++i)
        MTRACE("Hashing child in first chunk " << curve->to_string(chunk_start[i]));

    // Do the hash
    auto chunk_hash = curve->hash_grow(
            existing_hash,
            start_offset,
            prior_child_after_offset,
            chunk
        );

    MTRACE("First chunk result: " << curve->to_string(chunk_hash) << " , chunk_size: " << chunk_size);

    // We've got our hash
    hash_out = std::move(chunk_hash);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
static void hash_next_chunk(const std::unique_ptr<C> &curve,
    const std::size_t chunk_start_idx,
    const std::vector<typename C::Scalar> &new_child_scalars,
    const std::size_t chunk_size,
    typename C::Point &hash_out)
{
    const auto chunk_start = new_child_scalars.data() + chunk_start_idx;
    const typename C::Chunk chunk{chunk_start, chunk_size};

    for (std::size_t i = 0; i < chunk_size; ++i)
        MTRACE("Child chunk_start_idx " << chunk_start_idx << " hashing child " << curve->to_string(chunk_start[i]));

    auto chunk_hash = get_new_parent(curve, chunk);

    MTRACE("Child chunk_start_idx " << chunk_start_idx << " result: " << curve->to_string(chunk_hash)
        << " , chunk_size: " << chunk_size);

    // We've got our hash
    hash_out = std::move(chunk_hash);
}
//----------------------------------------------------------------------------------------------------------------------
// Hash chunks of a layer of new children, outputting the next layer's parents
template<typename C>
static LayerExtension<C> hash_children_chunks(const std::unique_ptr<C> &curve,
    const typename C::Scalar *old_last_child,
    const typename C::Point *old_last_parent,
    const std::size_t start_offset,
    const uint64_t next_parent_start_index,
    const std::vector<typename C::Scalar> &new_child_scalars,
    const std::size_t chunk_width)
{
    LayerExtension<C> parents_out;
    parents_out.start_idx                 = next_parent_start_index;
    parents_out.update_existing_last_hash = old_last_parent != nullptr;

    CHECK_AND_ASSERT_THROW_MES(!new_child_scalars.empty(), "empty child scalars");
    CHECK_AND_ASSERT_THROW_MES(chunk_width > start_offset, "start_offset must be smaller than chunk_width");

    // See how many children we need to fill up the existing last chunk
    const std::size_t first_chunk_size = std::min(new_child_scalars.size(), chunk_width - start_offset);

    CHECK_AND_ASSERT_THROW_MES(new_child_scalars.size() >= first_chunk_size, "unexpected first chunk size");

    const std::size_t n_chunks = 1 // first chunk
        + (new_child_scalars.size() - first_chunk_size) / chunk_width // middle chunks
        + (((new_child_scalars.size() - first_chunk_size) % chunk_width > 0) ? 1 : 0); // final chunk

    parents_out.hashes.resize(n_chunks);

    MTRACE("First chunk_size: "          << first_chunk_size
        << " , num new child scalars: "  << new_child_scalars.size()
        << " , start_offset: "           << start_offset
        << " , parent layer start idx: " << parents_out.start_idx
        << " , n chunks: "               << n_chunks);

    // Hash batches of chunks in parallel
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    const std::size_t HASH_BATCH_SIZE = 1 + (n_chunks / (std::size_t)tpool.get_max_concurrency());
    for (std::size_t i = 0; i < n_chunks; i += HASH_BATCH_SIZE)
    {
        const std::size_t end = std::min(i + HASH_BATCH_SIZE, n_chunks);
        tpool.submit(&waiter,
                [
                    &curve,
                    &old_last_child,
                    &old_last_parent,
                    &new_child_scalars,
                    &parents_out,
                    start_offset,
                    first_chunk_size,
                    chunk_width,
                    i,
                    end
                ]()
                {
                    for (std::size_t j = i; j < end; ++j)
                    {
                        auto &hash_out = parents_out.hashes[j];

                        // Hash the first chunk
                        if (j == 0)
                        {
                            hash_first_chunk(curve,
                                old_last_child,
                                old_last_parent,
                                start_offset,
                                new_child_scalars,
                                first_chunk_size,
                                hash_out);
                            continue;
                        }

                        const std::size_t chunk_start = j * chunk_width;

                        CHECK_AND_ASSERT_THROW_MES(chunk_start > start_offset, "unexpected small chunk_start");
                        const std::size_t chunk_start_idx = chunk_start - start_offset;

                        const std::size_t chunk_end_idx = std::min(chunk_start_idx + chunk_width, new_child_scalars.size());

                        CHECK_AND_ASSERT_THROW_MES(chunk_end_idx > chunk_start_idx, "unexpected large chunk_start_idx");
                        const std::size_t chunk_size = chunk_end_idx - chunk_start_idx;

                        hash_next_chunk(curve, chunk_start_idx, new_child_scalars, chunk_size, hash_out);
                    }
                },
                true
            );
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "failed to hash chunks");

    return parents_out;
};
//----------------------------------------------------------------------------------------------------------------------
static GrowLayerInstructions get_grow_layer_instructions(const uint64_t old_total_children,
    const uint64_t new_total_children,
    const std::size_t parent_chunk_width,
    const bool last_child_will_change)
{
    // 1. Check pre-conditions on total number of children
    // - If there's only 1 old child, it must be the old root, and we must be setting a new parent layer after old root
    const bool setting_next_layer_after_old_root = old_total_children == 1;
    if (setting_next_layer_after_old_root)
    {
        CHECK_AND_ASSERT_THROW_MES(new_total_children > old_total_children,
            "new_total_children must be > old_total_children when setting next layer after old root");
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(new_total_children >= old_total_children,
            "new_total_children must be >= old_total_children");
    }

    // 2. Calculate old and new total number of parents using totals for children
    // If there's only 1 child, then it must be the old root and thus it would have no old parents
    const uint64_t old_total_parents = old_total_children > 1
        ? (1 + ((old_total_children - 1) / parent_chunk_width))
        : 0;
    const uint64_t new_total_parents = 1 + ((new_total_children - 1) / parent_chunk_width);

    // 3. Check pre-conditions on total number of parents
    CHECK_AND_ASSERT_THROW_MES(new_total_parents >= old_total_parents,
        "new_total_parents must be >= old_total_parents");
    CHECK_AND_ASSERT_THROW_MES(new_total_parents < new_total_children,
        "new_total_parents must be < new_total_children");

    if (setting_next_layer_after_old_root)
    {
        CHECK_AND_ASSERT_THROW_MES(old_total_parents == 0,
            "old_total_parents expected to be 0 when setting next layer after old root");
    }

    // 4. Set the current offset in the last chunk
    // - Note: this value starts at the last child in the last chunk, but it might need to be decremented by 1 if we're
    //   changing that last child
    std::size_t offset = old_total_parents > 0
        ? (old_total_children % parent_chunk_width)
        : 0;

    // 5. Check if the last chunk is full (keep in mind it's also possible it's empty)
    const bool last_chunk_is_full = offset == 0;

    // 6. When the last child changes, we'll need to use its old value to update the parent
    // - We only care if the child has a parent, otherwise we won't need the child's old value to update the parent
    //   (since there is no parent to update)
    const bool need_old_last_child = old_total_parents > 0 && last_child_will_change;

    // 7. If we're changing the last child, we need to subtract the offset by 1 to account for that child
    if (need_old_last_child)
    {
        CHECK_AND_ASSERT_THROW_MES(old_total_children > 0, "no old children but last child is supposed to change");

        // If the chunk is full, must subtract the chunk width by 1
        offset = offset == 0 ? (parent_chunk_width - 1) : (offset - 1);
    }

    // 8. When the last parent changes, we'll need to use its old value to update itself
    const bool adding_members_to_existing_last_chunk = old_total_parents > 0 && !last_chunk_is_full
        && new_total_children > old_total_children;
    const bool need_old_last_parent                  = need_old_last_child || adding_members_to_existing_last_chunk;

    // 9. Set the next parent's start index
    uint64_t next_parent_start_index = old_total_parents;
    if (need_old_last_parent)
    {
        // If we're updating the last parent, we need to bring the starting parent index back 1
        CHECK_AND_ASSERT_THROW_MES(old_total_parents > 0, "no old parents but last parent is supposed to change1");
        --next_parent_start_index;
    }

    // Done
    MTRACE("parent_chunk_width: "                   << parent_chunk_width
        << " , old_total_children: "                << old_total_children
        << " , new_total_children: "                << new_total_children
        << " , old_total_parents: "                 << old_total_parents
        << " , new_total_parents: "                 << new_total_parents
        << " , setting_next_layer_after_old_root: " << setting_next_layer_after_old_root
        << " , need_old_last_child: "               << need_old_last_child
        << " , need_old_last_parent: "              << need_old_last_parent
        << " , start_offset: "                      << offset
        << " , next_parent_start_index: "           << next_parent_start_index);

    return GrowLayerInstructions{
            .parent_chunk_width                = parent_chunk_width,
            .old_total_parents                 = old_total_parents,
            .new_total_parents                 = new_total_parents,
            .setting_next_layer_after_old_root = setting_next_layer_after_old_root,
            .need_old_last_child               = need_old_last_child,
            .need_old_last_parent              = need_old_last_parent,
            .start_offset                      = offset,
            .next_parent_start_index           = next_parent_start_index,
        };

};
//----------------------------------------------------------------------------------------------------------------------
static GrowLayerInstructions get_leaf_layer_grow_instructions(const uint64_t old_n_leaf_tuples,
    const uint64_t new_n_leaf_tuples,
    const std::size_t leaf_tuple_size,
    const std::size_t leaf_layer_chunk_width)
{
    // The leaf layer can never be the root layer
    const bool setting_next_layer_after_old_root = false;

    const uint64_t old_total_children = old_n_leaf_tuples * leaf_tuple_size;
    const uint64_t new_total_children = (old_n_leaf_tuples + new_n_leaf_tuples) * leaf_tuple_size;

    const uint64_t old_total_parents = old_total_children > 0
        ? (1 + ((old_total_children - 1) / leaf_layer_chunk_width))
        : 0;
    const uint64_t new_total_parents = 1 + ((new_total_children - 1) / leaf_layer_chunk_width);

    CHECK_AND_ASSERT_THROW_MES(new_total_children >= old_total_children,
        "new_total_children must be >= old_total_children");
    CHECK_AND_ASSERT_THROW_MES(new_total_parents >= old_total_parents,
        "new_total_parents must be >= old_total_parents");

    // Since leaf layer is append-only, no leaf can ever change and we'll never need an old leaf
    const bool need_old_last_child = false;

    const std::size_t offset = old_total_children % leaf_layer_chunk_width;

    const bool last_chunk_is_full                    = offset == 0;
    const bool adding_members_to_existing_last_chunk = old_total_parents > 0 && !last_chunk_is_full
        && new_total_children > old_total_children;
    const bool need_old_last_parent                  = adding_members_to_existing_last_chunk;

    uint64_t next_parent_start_index = old_total_parents;
    if (need_old_last_parent)
    {
        // If we're updating the last parent, we need to bring the starting parent index back 1
        CHECK_AND_ASSERT_THROW_MES(old_total_parents > 0, "no old parents but last parent is supposed to change2");
        --next_parent_start_index;
    }

    MTRACE("parent_chunk_width: "                   << leaf_layer_chunk_width
        << " , old_total_children: "                << old_total_children
        << " , new_total_children: "                << new_total_children
        << " , old_total_parents: "                 << old_total_parents
        << " , new_total_parents: "                 << new_total_parents
        << " , setting_next_layer_after_old_root: " << setting_next_layer_after_old_root
        << " , need_old_last_child: "               << need_old_last_child
        << " , need_old_last_parent: "              << need_old_last_parent
        << " , start_offset: "                      << offset
        << " , next_parent_start_index: "           << next_parent_start_index);

    return GrowLayerInstructions{
            .parent_chunk_width                = leaf_layer_chunk_width,
            .old_total_parents                 = old_total_parents,
            .new_total_parents                 = new_total_parents,
            .setting_next_layer_after_old_root = setting_next_layer_after_old_root,
            .need_old_last_child               = need_old_last_child,
            .need_old_last_parent              = need_old_last_parent,
            .start_offset                      = offset,
            .next_parent_start_index           = next_parent_start_index,
        };
};
//----------------------------------------------------------------------------------------------------------------------
// Helper function used to get the next layer extension used to grow the next layer in the tree
// - for example, if we just grew the parent layer after the leaf layer, the "next layer" would be the grandparent
//   layer of the leaf layer
template<typename C_CHILD, typename C_PARENT>
static LayerExtension<C_PARENT> get_next_layer_extension(const std::unique_ptr<C_CHILD> &c_child,
    const std::unique_ptr<C_PARENT> &c_parent,
    const GrowLayerInstructions &grow_layer_instructions,
    const std::vector<typename C_CHILD::Point> &child_last_hashes,
    const std::vector<typename C_PARENT::Point> &parent_last_hashes,
    const std::vector<LayerExtension<C_CHILD>> child_layer_extensions,
    const std::size_t last_updated_child_idx,
    const std::size_t last_updated_parent_idx)
{
    // TODO: comments
    const auto *child_last_hash = (last_updated_child_idx >= child_last_hashes.size())
        ? nullptr
        : &child_last_hashes[last_updated_child_idx];

    const auto *parent_last_hash = (last_updated_parent_idx >= parent_last_hashes.size())
        ? nullptr
        : &parent_last_hashes[last_updated_parent_idx];

    // Pre-conditions
    CHECK_AND_ASSERT_THROW_MES(last_updated_child_idx < child_layer_extensions.size(), "missing child layer");
    const auto &child_extension = child_layer_extensions[last_updated_child_idx];

    if (grow_layer_instructions.setting_next_layer_after_old_root)
    {
        CHECK_AND_ASSERT_THROW_MES((last_updated_child_idx + 1) == child_last_hashes.size(),
            "unexpected last updated child idx");
        CHECK_AND_ASSERT_THROW_MES(child_last_hash != nullptr, "missing last child when setting layer after old root");
    }

    const auto child_scalars = next_child_scalars_from_children<C_CHILD, C_PARENT>(c_child,
        grow_layer_instructions.setting_next_layer_after_old_root ? child_last_hash : nullptr,
        child_extension);

    if (grow_layer_instructions.need_old_last_parent)
        CHECK_AND_ASSERT_THROW_MES(parent_last_hash != nullptr, "missing last parent");

    typename C_PARENT::Scalar last_child_scalar;
    if (grow_layer_instructions.need_old_last_child)
    {
        CHECK_AND_ASSERT_THROW_MES(child_last_hash != nullptr, "missing last child");
        last_child_scalar = c_child->point_to_cycle_scalar(*child_last_hash);
    }

    // Do the hashing
    LayerExtension<C_PARENT> layer_extension = hash_children_chunks(
            c_parent,
            grow_layer_instructions.need_old_last_child ? &last_child_scalar : nullptr,
            grow_layer_instructions.need_old_last_parent ? parent_last_hash : nullptr,
            grow_layer_instructions.start_offset,
            grow_layer_instructions.next_parent_start_index,
            child_scalars,
            grow_layer_instructions.parent_chunk_width
        );

    CHECK_AND_ASSERT_THROW_MES((layer_extension.start_idx + layer_extension.hashes.size()) ==
        grow_layer_instructions.new_total_parents,
        "unexpected num parents extended");

    return layer_extension;
}
//----------------------------------------------------------------------------------------------------------------------
static PreLeafTuple output_tuple_to_pre_leaf_tuple(const OutputTuple &o)
{
    TIME_MEASURE_NS_START(point_to_ed_derivatives_ns);

    // TODO: this relatively new point_to_ed_derivatives function introduced a point de-compression in order to get wei
    // y coordinates from the ed x coordinate. It's worth re-investigating the tree building perf hit as a result. The
    // daemon is de-compressing these points twice (once when checking for torsion, and again here).
    PreLeafTuple plt;
    if (!fcmp_pp::point_to_ed_derivatives(o.O, plt.O_derivatives))
        throw std::runtime_error("failed to get ed derivatives from O");
    if (!fcmp_pp::point_to_ed_derivatives(o.I, plt.I_derivatives))
        throw std::runtime_error("failed to get ed derivatives from I");
    if (!fcmp_pp::point_to_ed_derivatives(o.C, plt.C_derivatives))
        throw std::runtime_error("failed to get ed derivatives from C");

    TIME_MEASURE_NS_FINISH(point_to_ed_derivatives_ns);

    LOG_PRINT_L3("point_to_ed_derivatives_ns: " << point_to_ed_derivatives_ns);

    return plt;
}
//----------------------------------------------------------------------------------------------------------------------
static PreLeafTuple output_to_pre_leaf_tuple(const OutputPair &output_pair,
    bool torsion_checked = false,
    bool use_fast_check = false)
{
    const auto o = output_to_tuple(output_pair, torsion_checked, use_fast_check);
    return output_tuple_to_pre_leaf_tuple(o);
}
//----------------------------------------------------------------------------------------------------------------------
static CurveTrees<Selene, Helios>::LeafTuple pre_leaf_tuple_to_leaf_tuple(const PreLeafTuple &plt)
{
    rct::key O_x, O_y, I_x, I_y, C_x, C_y;
    if (!fcmp_pp::ed_derivatives_to_wei_x_y(plt.O_derivatives, O_x, O_y))
        throw std::runtime_error("failed to get wei x y from O derivatives");
    if (!fcmp_pp::ed_derivatives_to_wei_x_y(plt.I_derivatives, I_x, I_y))
        throw std::runtime_error("failed to get wei x y from I derivatives");
    if (!fcmp_pp::ed_derivatives_to_wei_x_y(plt.C_derivatives, C_x, C_y))
        throw std::runtime_error("failed to get wei x y from C derivatives");

    return CurveTrees<Selene, Helios>::LeafTuple{
        .O_x = tower_cycle::selene_scalar_from_bytes(O_x),
        .O_y = tower_cycle::selene_scalar_from_bytes(O_y),

        .I_x = tower_cycle::selene_scalar_from_bytes(I_x),
        .I_y = tower_cycle::selene_scalar_from_bytes(I_y),

        .C_x = tower_cycle::selene_scalar_from_bytes(C_x),
        .C_y = tower_cycle::selene_scalar_from_bytes(C_y)
    };
}
//----------------------------------------------------------------------------------------------------------------------
static CurveTrees<Selene, Helios>::LeafTuple output_tuple_to_leaf_tuple(const OutputTuple &output_tuple)
{
    const auto plt = output_tuple_to_pre_leaf_tuple(output_tuple);
    return pre_leaf_tuple_to_leaf_tuple(plt);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_CHILD, typename C_PARENT>
static typename C_PARENT::Point get_chunk_hash(const std::unique_ptr<C_CHILD> &c_child,
    const std::unique_ptr<C_PARENT> &c_parent,
    const std::vector<std::vector<typename C_CHILD::Point>> &child_layers,
    const bool use_new_last_hash,
    const typename C_CHILD::Point &new_last_hash,
    std::size_t &c_idx_inout)
{
    CHECK_AND_ASSERT_THROW_MES(child_layers.size() > c_idx_inout, "high c_idx");
    const auto &layer = child_layers[c_idx_inout];

    // Collect child scalars so we can hash them
    std::vector<typename C_PARENT::Scalar> scalars;
    scalars.reserve(layer.size());

    CHECK_AND_ASSERT_THROW_MES(!layer.empty(), "empty layer");
    for (std::size_t i = 0; i < (layer.size() - 1); ++i)
        scalars.emplace_back(c_child->point_to_cycle_scalar(layer[i]));

    // Use the newly calculated hash from the preceding layer
    const auto &last_hash = use_new_last_hash ? new_last_hash : layer.back();
    scalars.emplace_back(c_child->point_to_cycle_scalar(last_hash));

    // Hash scalars
    const typename C_PARENT::Chunk chunk{scalars.data(), scalars.size()};
    const typename C_PARENT::Point hash = get_new_parent(c_parent, chunk);

    MTRACE("Hash result: " << c_parent->to_string(hash));

    ++c_idx_inout;
    return hash;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees public member functions
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Selene, Helios>::LeafTuple CurveTrees<Selene, Helios>::leaf_tuple(const OutputPair &output_pair) const
{
    const auto plt = output_to_pre_leaf_tuple(output_pair);
    return pre_leaf_tuple_to_leaf_tuple(plt);
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<typename C1::Scalar> CurveTrees<C1, C2>::flatten_leaves(std::vector<LeafTuple> &&leaves) const
{
    std::vector<typename C1::Scalar> flattened_leaves;
    flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

    for (auto &l : leaves)
    {
        flattened_leaves.emplace_back(std::move(l.O_x));
        flattened_leaves.emplace_back(std::move(l.O_y));

        flattened_leaves.emplace_back(std::move(l.I_x));
        flattened_leaves.emplace_back(std::move(l.I_y));

        flattened_leaves.emplace_back(std::move(l.C_x));
        flattened_leaves.emplace_back(std::move(l.C_y));
    }

    return flattened_leaves;
};

// Explicit instantiation
template std::vector<Selene::Scalar> CurveTrees<Selene, Helios>::flatten_leaves(
    std::vector<LeafTuple> &&leaves) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeExtension CurveTrees<C1, C2>::get_tree_extension(
    const uint64_t old_n_leaf_tuples,
    const LastHashes &existing_last_hashes,
    std::vector<std::vector<OutputContext>> &&new_outputs,
    const bool use_fast_torsion_check)
{
    TreeExtension tree_extension;
    tree_extension.leaves.start_leaf_tuple_idx = old_n_leaf_tuples;

    if (new_outputs.empty())
        return tree_extension;

    TIME_MEASURE_START(sorting_outputs);

    // Sort the outputs by order they appear in the chain
    // Note: the outputs are expected to be grouped by last locked block
    std::vector<OutputContext> flat_sorted_outputs;
    for (auto &unsorted_outputs : new_outputs)
    {
        const auto sort_fn = [](const OutputContext &a, const OutputContext &b){return a.output_id < b.output_id;};
        std::sort(unsorted_outputs.begin(), unsorted_outputs.end(), sort_fn);

        // No duplicates allowed
        const auto dup_check = [](const OutputContext &a, const OutputContext &b){return a.output_id == b.output_id;};
        CHECK_AND_ASSERT_THROW_MES(std::adjacent_find(unsorted_outputs.begin(), unsorted_outputs.end(), dup_check)
            == unsorted_outputs.end(), "get_tree_extension: duplicate output id's");

        flat_sorted_outputs.insert(flat_sorted_outputs.end(),
            std::make_move_iterator(unsorted_outputs.begin()),
            std::make_move_iterator(unsorted_outputs.end()));
    }

    TIME_MEASURE_FINISH(sorting_outputs);

    // Convert sorted outputs into leaf tuples, place each element of each leaf tuple in a flat vector to be hashed,
    // and place the outputs in a tree extension struct for insertion into the db. We ignore invalid outputs, since
    // they cannot be inserted to the tree.
    std::vector<typename C1::Scalar> flattened_leaves;
    this->set_valid_leaves(flattened_leaves,
        tree_extension.leaves.tuples,
        std::move(flat_sorted_outputs),
        use_fast_torsion_check);

    if (flattened_leaves.empty())
        return tree_extension;

    TIME_MEASURE_START(hashing_leaves);

    MTRACE("Getting extension for layer 0");
    auto grow_layer_instructions = get_leaf_layer_grow_instructions(
        old_n_leaf_tuples,
        tree_extension.leaves.tuples.size(),
        LEAF_TUPLE_SIZE,
        m_leaf_layer_chunk_width);

    if (grow_layer_instructions.need_old_last_parent)
        CHECK_AND_ASSERT_THROW_MES(!existing_last_hashes.c1_last_hashes.empty(), "missing last c1 parent");

    // Hash the leaf layer
    auto leaf_parents = hash_children_chunks(m_c1,
            nullptr, // We never need the old last child from leaf layer because the leaf layer is always append-only
            grow_layer_instructions.need_old_last_parent ? &existing_last_hashes.c1_last_hashes[0] : nullptr,
            grow_layer_instructions.start_offset,
            grow_layer_instructions.next_parent_start_index,
            flattened_leaves,
            m_leaf_layer_chunk_width
        );
    TIME_MEASURE_FINISH(hashing_leaves);

    CHECK_AND_ASSERT_THROW_MES(
        (leaf_parents.start_idx + leaf_parents.hashes.size()) == grow_layer_instructions.new_total_parents,
        "unexpected num leaf parents extended");

    tree_extension.c1_layer_extensions.emplace_back(std::move(leaf_parents));

    // Alternate between hashing c1 children, c2 children, c1, c2, ...
    bool parent_is_c2 = true;

    std::size_t c1_last_idx = 0;
    std::size_t c2_last_idx = 0;
    TIME_MEASURE_START(hashing_layers);
    while (grow_layer_instructions.new_total_parents > 1)
    {
        MTRACE("Getting extension for layer " << (c1_last_idx + c2_last_idx + 1));

        const uint64_t new_total_children = grow_layer_instructions.new_total_parents;

        grow_layer_instructions = this->set_next_layer_extension(
                grow_layer_instructions,
                parent_is_c2,
                existing_last_hashes,
                c1_last_idx,
                c2_last_idx,
                tree_extension
            );

        // Sanity check to make sure we're making progress to exit the while loop
        CHECK_AND_ASSERT_THROW_MES(grow_layer_instructions.new_total_parents < new_total_children,
            "expect fewer parents than children in every layer");

        parent_is_c2 = !parent_is_c2;
    }
    TIME_MEASURE_FINISH(hashing_layers);

    m_sorting_outputs_ms += sorting_outputs;
    m_hash_leaves_ms += hashing_leaves;
    m_hash_layers_ms += hashing_layers;

    LOG_PRINT_L2("Total time spent hashing leaves: " << m_hash_leaves_ms / 1000
        << " , hashing layers: "              << m_hash_layers_ms / 1000
        << " , sorting outputs: "             << m_sorting_outputs_ms / 1000);

    return tree_extension;
};

// Explicit instantiation
template CurveTrees<Selene, Helios>::TreeExtension CurveTrees<Selene, Helios>::get_tree_extension(
    const uint64_t old_n_leaf_tuples,
    const LastHashes &existing_last_hashes,
    std::vector<std::vector<OutputContext>> &&new_outputs,
    const bool use_fast_torsion_check);
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<uint64_t> CurveTrees<C1, C2>::n_elems_per_layer(const uint64_t n_leaf_tuples) const
{
    std::vector<uint64_t> n_elems_per_layer;
    if (n_leaf_tuples == 0)
        return n_elems_per_layer;

    uint64_t n_children = n_leaf_tuples;
    bool parent_is_c1 = true;
    do
    {
        const std::size_t parent_chunk_width = parent_is_c1 ? m_c1_width : m_c2_width;
        const uint64_t n_parents = ((n_children - 1) / parent_chunk_width) + 1;
        n_elems_per_layer.push_back(n_parents);
        n_children = n_parents;
        parent_is_c1 = !parent_is_c1;
    }
    while (n_children > 1);

    return n_elems_per_layer;
}

// Explicit instantiation
template std::vector<uint64_t> CurveTrees<Selene, Helios>::n_elems_per_layer(const uint64_t n_leaf_tuples) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::size_t CurveTrees<C1, C2>::n_layers(const uint64_t n_leaf_tuples) const
{
    return this->n_elems_per_layer(n_leaf_tuples).size();
}

// Explicit instantiation
template std::size_t CurveTrees<Selene, Helios>::n_layers(const uint64_t n_leaf_tuples) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
PathIndexes CurveTrees<C1, C2>::get_path_indexes(const uint64_t n_leaf_tuples, const uint64_t leaf_tuple_idx) const
{
    PathIndexes path_indexes_out;

    MTRACE("Getting path indexes for leaf_tuple_idx: " << leaf_tuple_idx << " , n_leaf_tuples: " << n_leaf_tuples);

    const auto child_chunk_indexes = this->get_child_chunk_indexes(n_leaf_tuples, leaf_tuple_idx);
    if (child_chunk_indexes.empty())
        return path_indexes_out;

    const auto n_elems_per_layer = this->n_elems_per_layer(n_leaf_tuples);
    CHECK_AND_ASSERT_THROW_MES(child_chunk_indexes.size() == (n_elems_per_layer.size() + 1),
        "size mismatch n elems per layer <> child_chunk_indexes");

    // Set the leaf range
    {
        const std::size_t parent_chunk_width = m_c1_width;

        const uint64_t n_children = n_leaf_tuples;
        const uint64_t start_range = child_chunk_indexes.front() * parent_chunk_width;
        const uint64_t end_range = std::min(n_children, start_range + parent_chunk_width);

        path_indexes_out.leaf_range = { start_range, end_range };
    }

    // Set ranges on layers above
    bool parent_is_c2 = true;
    for (std::size_t i = 0; i < n_elems_per_layer.size(); ++i)
    {
        const std::size_t parent_chunk_width = parent_is_c2 ? m_c2_width : m_c1_width;

        const uint64_t child_chunk_idx = child_chunk_indexes[i + 1];
        const uint64_t n_children = n_elems_per_layer[i];

        const uint64_t start_range = child_chunk_idx * parent_chunk_width;
        const uint64_t end_range = std::min(n_children, start_range + parent_chunk_width);

        MTRACE("start_range: "           << start_range
            << " , end_range: "          << end_range
            << " , parent_chunk_width: " << parent_chunk_width
            << " , n_children: "         << n_children);

        path_indexes_out.layers.emplace_back(PathIndexes::Range{start_range, end_range});

        parent_is_c2 = !parent_is_c2;
    }

    return path_indexes_out;
}

// Explicit instantiation
template PathIndexes CurveTrees<Selene, Helios>::get_path_indexes(const uint64_t n_leaf_tuples,
    const uint64_t leaf_tuple_idx) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<uint64_t> CurveTrees<C1, C2>::get_child_chunk_indexes(const uint64_t n_leaf_tuples,
    const uint64_t leaf_tuple_idx) const
{
    std::vector<uint64_t> child_chunk_indexes_out;
    if (n_leaf_tuples <= leaf_tuple_idx)
        return child_chunk_indexes_out;

    const std::size_t n_layers = this->n_layers(n_leaf_tuples);

    bool parent_is_c1 = true;
    uint64_t child_idx = leaf_tuple_idx;
    for (std::size_t i = 0; i < n_layers; ++i)
    {
        const std::size_t parent_chunk_width = parent_is_c1 ? m_c1_width : m_c2_width;
        const uint64_t child_chunk_idx = child_idx / parent_chunk_width;

        child_chunk_indexes_out.push_back(child_chunk_idx);

        child_idx = child_chunk_idx;
        parent_is_c1 = !parent_is_c1;
    }

    // Add a 0 for the root (it's its layer above's 0-index child)
    child_chunk_indexes_out.push_back(0);

    return child_chunk_indexes_out;
}

// Explicit instantiation
template std::vector<uint64_t> CurveTrees<Selene, Helios>::get_child_chunk_indexes(const uint64_t n_leaf_tuples,
    const uint64_t leaf_tuple_idx) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::LastHashes CurveTrees<C1, C2>::tree_edge_to_last_hashes(
    const std::vector<crypto::ec_point> &tree_edge) const
{
    typename CurveTrees<C1, C2>::LastHashes last_hashes;

    bool parent_is_c1 = true;
    for (const auto &last_hash : tree_edge)
    {
        if (parent_is_c1)
            last_hashes.c1_last_hashes.push_back(m_c1->from_bytes(last_hash));
        else
            last_hashes.c2_last_hashes.push_back(m_c2->from_bytes(last_hash));
        parent_is_c1 = !parent_is_c1;
    }

    return last_hashes;
}

// Explicit instantiation
template CurveTrees<Selene, Helios>::LastHashes CurveTrees<Selene, Helios>::tree_edge_to_last_hashes(
    const std::vector<crypto::ec_point> &tree_edge) const;
//----------------------------------------------------------------------------------------------------------------------
template<>
bool CurveTrees<Selene, Helios>::audit_path(const CurveTrees<Selene, Helios>::Path &path,
    const OutputPair &output,
    const uint64_t n_leaf_tuples_in_tree) const
{
    // TODO: use the leaf idx to know exactly which parent indexes we expect and how many parent hashes we expect at each layer

    // Cleaner refs
    const auto &leaves = path.leaves;
    const auto &c1_layers = path.c1_layers;
    const auto &c2_layers = path.c2_layers;

    const std::size_t n_layers = c1_layers.size() + c2_layers.size();
    CHECK_AND_ASSERT_MES(n_layers == this->n_layers(n_leaf_tuples_in_tree), false, "unexpected n_layers");

    // Make sure output tuple is present in leaves
    const auto output_tuple = output_to_tuple(output);
    bool found = false;
    for (std::size_t i = 0; !found && i < leaves.size(); ++i)
        found = (output_tuple.O == leaves[i].O && output_tuple.I == leaves[i].I && output_tuple.C == leaves[i].C);
    CHECK_AND_ASSERT_MES(found, false, "did not find output in chunk of leaves");

    // Get all hashes
    const auto hashes = this->calc_hashes_from_path(path);
    CHECK_AND_ASSERT_MES(hashes.size(), false, "empty hashes from path");
    CHECK_AND_ASSERT_MES(n_layers == hashes.size(), false, "hashes <> n_layers mismatch");

    // Make sure each hash is present in each layer above
    bool parent_is_c1 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    for (std::size_t i = 0; i < hashes.size(); ++i)
    {
        MDEBUG("Auditing layer " << i);
        const auto hash_str = epee::string_tools::pod_to_hex(hashes[i]);

        // TODO: template
        if (parent_is_c1)
        {
            // Make sure hash is present in c1 layer
            CHECK_AND_ASSERT_MES(c1_layers.size() > c1_idx, false, "high c1_idx");
            const auto &c1_layer = c1_layers[c1_idx];

            MDEBUG("Looking for c1 hash: " << hash_str << " among " << c1_layer.size() << " hashes");
            found = false;
            for (std::size_t j = 0; !found && j < c1_layer.size(); ++j)
            {
                MDEBUG("Reading: " << m_c1->to_string(c1_layer[j]));
                found = (hash_str == m_c1->to_string(c1_layer[j]));
            }
            CHECK_AND_ASSERT_MES(found, false, "did not find c1 hash");

            ++c1_idx;
        }
        else
        {
            // Make sure hash is present in c2 layer
            CHECK_AND_ASSERT_MES(c2_layers.size() > c2_idx, false, "high c2_idx");
            const auto &c2_layer = c2_layers[c2_idx];

            MDEBUG("Looking for c2 hash: " << hash_str << " among " << c2_layer.size() << " hashes");
            found = false;
            for (std::size_t j = 0; !found && j < c2_layer.size(); ++j)
            {
                MDEBUG("Reading: " << m_c2->to_string(c2_layer[j]));
                found = (hash_str == m_c2->to_string(c2_layer[j]));
            }
            CHECK_AND_ASSERT_MES(found, false, "did not find c2 hash");

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Selene, Helios>::Path CurveTrees<Selene, Helios>::path_bytes_to_path(const PathBytes &path_bytes) const
{
    typename CurveTrees<Selene, Helios>::Path path;

    // Leaves
    path.leaves.reserve(path_bytes.leaves.size());
    for (const auto &leaf : path_bytes.leaves)
        path.leaves.emplace_back(output_to_tuple(leaf.output_pair, leaf.torsion_checked));

    // Layers
    bool parent_is_c1 = true;
    for (const auto &layer : path_bytes.layer_chunks)
    {
        if (parent_is_c1)
        {
            path.c1_layers.emplace_back();
            path.c1_layers.back().reserve(layer.chunk_bytes.size());
            for (const auto &elem : layer.chunk_bytes)
                path.c1_layers.back().emplace_back(m_c1->from_bytes(elem));
        }
        else
        {
            path.c2_layers.emplace_back();
            path.c2_layers.back().reserve(layer.chunk_bytes.size());
            for (const auto &elem : layer.chunk_bytes)
                path.c2_layers.back().emplace_back(m_c2->from_bytes(elem));
        }

        parent_is_c1 = !parent_is_c1;
    }

    return path;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<crypto::ec_point> CurveTrees<C1, C2>::calc_hashes_from_path(
    const typename CurveTrees<C1, C2>::Path &path,
    const bool replace_last_hash) const
{
    std::vector<typename C1::Point> c1_hashes;
    std::vector<typename C2::Point> c2_hashes;

    // Cleaner refs
    const auto &leaves = path.leaves;
    const auto &c1_layers = path.c1_layers;
    const auto &c2_layers = path.c2_layers;

    const std::size_t n_layers = c1_layers.size() + c2_layers.size();
    c1_hashes.reserve(1 + c1_layers.size());
    c2_hashes.reserve(c2_layers.size());

    // Initial checks
    CHECK_AND_ASSERT_THROW_MES(!leaves.empty(),             "empty leaves");
    CHECK_AND_ASSERT_THROW_MES(!c1_layers.empty(),          "empty c1 layers");
    CHECK_AND_ASSERT_THROW_MES(leaves.size() <= m_c1_width, "too many leaves");

    {
        MTRACE("Hashing leaves");

        // Collect leaves so we can hash them
        std::vector<typename C1::Scalar> leaf_scalars;
        leaf_scalars.reserve(leaves.size() * LEAF_TUPLE_SIZE);
        for (auto &l : leaves)
        {
            auto leaf_tuple = output_tuple_to_leaf_tuple(l);
            leaf_scalars.emplace_back(std::move(leaf_tuple.O_x));
            leaf_scalars.emplace_back(std::move(leaf_tuple.O_y));

            leaf_scalars.emplace_back(std::move(leaf_tuple.I_x));
            leaf_scalars.emplace_back(std::move(leaf_tuple.I_y));

            leaf_scalars.emplace_back(std::move(leaf_tuple.C_x));
            leaf_scalars.emplace_back(std::move(leaf_tuple.C_y));
        }

        // Hash the leaf chunk
        const typename C1::Chunk leaf_chunk{leaf_scalars.data(), leaf_scalars.size()};
        const typename C1::Point leaf_parent_hash = get_new_parent<C1>(m_c1, leaf_chunk);

        c1_hashes.push_back(leaf_parent_hash);
        MTRACE("c1 hash result: " << m_c1->to_string(c1_hashes.back()));
    }

    // Continue hashing every layer chunk until there are no more layers
    std::size_t c1_idx = 0, c2_idx = 0;
    for (std::size_t i = 1; i < n_layers; ++i)
    {
        MTRACE("Hashing layer " << i);
        if (c1_idx == c2_idx /*c2 parent*/)
        {
            auto hash = get_chunk_hash(m_c1, m_c2, c1_layers, replace_last_hash, c1_hashes.back(), c1_idx);
            c2_hashes.emplace_back(std::move(hash));
        }
        else
        {
            auto hash = get_chunk_hash(m_c2, m_c1, c2_layers, replace_last_hash, c2_hashes.back(), c2_idx);
            c1_hashes.emplace_back(std::move(hash));
        }
    }

    // Collect hashes
    std::vector<crypto::ec_point> hashes;
    hashes.reserve(n_layers);
    c1_idx = 0, c2_idx = 0;
    for (std::size_t i = 0; i < n_layers; ++i)
    {
        if (c1_idx == c2_idx /*c1 parent*/)
            hashes.emplace_back(m_c1->to_bytes(c1_hashes[c1_idx++]));
        else
            hashes.emplace_back(m_c2->to_bytes(c2_hashes[c2_idx++]));
    }

    return hashes;
}

// Explicit instantiation
template std::vector<crypto::ec_point> CurveTrees<Selene, Helios>::calc_hashes_from_path(
    const typename CurveTrees<Selene, Helios>::Path &path,
    const bool replace_last_hash) const;
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTreesV1::ConsolidatedPaths CurveTrees<Selene, Helios>::get_dummy_paths(
    const std::vector<fcmp_pp::curve_trees::OutputContext> &outputs,
    uint8_t n_layers) const
{
    CHECK_AND_ASSERT_THROW_MES(this->n_layers(outputs.size()) <= n_layers, "n_layers is too low");

    // Require outputs fit into whole chunks to simplify last chunk handling
    CHECK_AND_ASSERT_THROW_MES((outputs.size() % m_c1_width) == 0, "expect outputs to fit into whole chunks");

    CurveTreesV1::ConsolidatedPaths paths;
    if (outputs.empty())
        return paths;

    // Leaves
    for (uint64_t i = 0; i < outputs.size(); ++i)
    {
        auto leaf_tuple = fcmp_pp::curve_trees::output_to_tuple(outputs[i].output_pair);

        const uint64_t leaf_chunk_idx = i / m_c1_width;
        auto it = paths.leaves_by_chunk_idx.find(leaf_chunk_idx);
        if (it == paths.leaves_by_chunk_idx.end())
        {
            paths.leaves_by_chunk_idx[leaf_chunk_idx] = {std::move(leaf_tuple)};
            continue;
        }
        it->second.emplace_back(std::move(leaf_tuple));
    }

    // First c1 layer
    {
        paths.c1_layers.push_back({});
        auto &c1_layer = paths.c1_layers.back();

        std::size_t leaf_chunk_idx = 0;
        std::size_t chunk_n = 0;

        auto leaves_by_chunk_it = paths.leaves_by_chunk_idx.find(leaf_chunk_idx);
        while (leaves_by_chunk_it != paths.leaves_by_chunk_idx.end())
        {
            // Flatten leaves
            std::vector<fcmp_pp::curve_trees::CurveTreesV1::LeafTuple> leaf_tuples;
            for (const auto &leaf : leaves_by_chunk_it->second)
                leaf_tuples.push_back(output_tuple_to_leaf_tuple(leaf));
            const auto flat_leaves = this->flatten_leaves(std::move(leaf_tuples));

            // Prepare to insert hash into the container
            auto c1_layer_it = c1_layer.find(chunk_n);
            if (c1_layer_it == c1_layer.end())
            {
                // Start new chunk if needed
                c1_layer[chunk_n] = {};
                c1_layer_it = c1_layer.find(chunk_n);;
            }

            // Hash the leaves
            c1_layer_it->second.push_back({});
            hash_first_chunk(m_c1, nullptr, nullptr, 0, flat_leaves, flat_leaves.size(), c1_layer_it->second.back());

            // Get ready to hash next chunk of leaves
            if (c1_layer_it->second.size() == m_c2_width)
                ++chunk_n;
            leaves_by_chunk_it = paths.leaves_by_chunk_idx.find(++leaf_chunk_idx);
        }

        // Fill out the last chunk with 0's as needed
        auto last_chunk_it = paths.c1_layers.back().find(chunk_n);
        if (last_chunk_it == paths.c1_layers.back().end())
            last_chunk_it = paths.c1_layers.back().find(chunk_n - 1);
        CHECK_AND_ASSERT_THROW_MES(last_chunk_it != paths.c1_layers.back().end(), "missing last c1 layer");
        while (n_layers > 1 && last_chunk_it->second.size() < m_c2_width)
            last_chunk_it->second.push_back(m_c1->hash_init_point());
    }

    // Rest of the tree
    bool parent_is_c2 = true;
    for (uint8_t l = 1; l < n_layers; ++l)
    {
        std::size_t prev_layer_i = 0;
        std::size_t chunk_n = 0;

        if (parent_is_c2)
        {
            paths.c2_layers.push_back({});
            auto &c2_layer = paths.c2_layers.back();

            auto prev_layer_it = paths.c1_layers.back().find(prev_layer_i);
            auto prev_layer_child_it = prev_layer_it->second.begin();

            while (prev_layer_it != paths.c1_layers.back().end())
            {
                std::vector<Selene::Point> c1_points;
                for (std::size_t j = 0; j < m_c2_width; ++j)
                {
                    c1_points.push_back(*prev_layer_child_it);
                    ++prev_layer_child_it;
                    if (prev_layer_child_it != prev_layer_it->second.end())
                        continue;
                    ++prev_layer_i;
                    prev_layer_it = paths.c1_layers.back().find(prev_layer_i);
                    if (prev_layer_it == paths.c1_layers.back().end())
                        break;
                    prev_layer_child_it = prev_layer_it->second.begin();
                }

                // Convert Selene points to Helios scalars
                std::vector<Helios::Scalar> c2_scalars;
                fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Selene, Helios>(m_c1, c1_points, c2_scalars);

                // Prepare to insert hash into the container
                auto c2_layer_it = c2_layer.find(chunk_n);
                if (c2_layer_it == c2_layer.end())
                {
                    // Start new chunk if needed
                    c2_layer[chunk_n] = {};
                    c2_layer_it = c2_layer.find(chunk_n);;
                }

                // Get hash of prior layer chunk
                c2_layer_it->second.push_back({});
                hash_first_chunk(m_c2, nullptr, nullptr, 0, c2_scalars, c2_scalars.size(), c2_layer_it->second.back());

                if (c2_layer_it->second.size() == m_c1_width)
                    ++chunk_n;
            }

            // Fill out the last chunk with 0's for non-root
            auto last_chunk_it = paths.c2_layers.back().find(chunk_n);
            if (last_chunk_it == paths.c2_layers.back().end())
                last_chunk_it = paths.c2_layers.back().find(chunk_n - 1);
            CHECK_AND_ASSERT_THROW_MES(last_chunk_it != paths.c2_layers.back().end(), "missing last c2 layer");
            while ((l + 1) < n_layers && last_chunk_it->second.size() < m_c1_width)
                last_chunk_it->second.push_back(m_c2->hash_init_point());
        }
        else
        {
            paths.c1_layers.push_back({});
            auto &c1_layer = paths.c1_layers.back();

            auto prev_layer_it = paths.c2_layers.back().find(prev_layer_i);
            auto prev_layer_child_it = prev_layer_it->second.begin();

            while (prev_layer_it != paths.c2_layers.back().end())
            {
                std::vector<Helios::Point> c2_points;
                for (std::size_t j = 0; j < m_c1_width; ++j)
                {
                    c2_points.push_back(*prev_layer_child_it);
                    ++prev_layer_child_it;
                    if (prev_layer_child_it != prev_layer_it->second.end())
                        continue;
                    ++prev_layer_i;
                    prev_layer_it = paths.c2_layers.back().find(prev_layer_i);
                    if (prev_layer_it == paths.c2_layers.back().end())
                        break;
                    prev_layer_child_it = prev_layer_it->second.begin();
                }

                // Convert Helios points to Selene scalars
                std::vector<Selene::Scalar> c1_scalars;
                fcmp_pp::tower_cycle::extend_scalars_from_cycle_points<Helios, Selene>(m_c2, c2_points, c1_scalars);

                // Prepare to insert hash into the container
                auto c1_layer_it = c1_layer.find(chunk_n);
                if (c1_layer_it == c1_layer.end())
                {
                    // Start new chunk if needed
                    c1_layer[chunk_n] = {};
                    c1_layer_it = c1_layer.find(chunk_n);;
                }

                // Get hash of prior layer chunk
                c1_layer_it->second.push_back({});
                hash_first_chunk(m_c1, nullptr, nullptr, 0, c1_scalars, c1_scalars.size(), c1_layer_it->second.back());

                if (c1_layer_it->second.size() == m_c2_width)
                    ++chunk_n;
            }

            // Fill out the last chunk with 0's for non-root layer
            auto last_chunk_it = paths.c1_layers.back().find(chunk_n);
            if (last_chunk_it == paths.c1_layers.back().end())
                last_chunk_it = paths.c1_layers.back().find(chunk_n - 1);
            CHECK_AND_ASSERT_THROW_MES(last_chunk_it != paths.c1_layers.back().end(), "missing last c1 layer");
            while ((l + 1) < n_layers && last_chunk_it->second.size() < m_c2_width)
                last_chunk_it->second.push_back(m_c1->hash_init_point());
        }

        parent_is_c2 = !parent_is_c2;
    }

    return paths;
};
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTreesV1::Path CurveTrees<Selene, Helios>::get_single_dummy_path(
    const CurveTreesV1::ConsolidatedPaths &dummy_paths,
    const uint64_t n_leaf_tuples,
    const uint64_t leaf_tuple_idx) const
{
    CurveTreesV1::Path path;
    const auto path_idxs = this->get_path_indexes(n_leaf_tuples, leaf_tuple_idx);

    // Leaves
    const uint64_t leaf_child_idx = path_idxs.leaf_range.first / m_c1_width;
    const auto leaf_it = dummy_paths.leaves_by_chunk_idx.find(leaf_child_idx);
    CHECK_AND_ASSERT_THROW_MES(leaf_it != dummy_paths.leaves_by_chunk_idx.end(), "dummy missing leaf chunk");
    CHECK_AND_ASSERT_THROW_MES((path_idxs.leaf_range.first + leaf_it->second.size()) == path_idxs.leaf_range.second,
        "dummy leaves size mismatch");
    path.leaves = leaf_it->second;

    // All other layers
    bool parent_is_c1 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    for (uint8_t l = 0; l < path_idxs.layers.size(); ++l)
    {
        const auto start = path_idxs.layers[l].first;
        const auto end = path_idxs.layers[l].second;

        const uint64_t child_idx = start / (parent_is_c1 ? m_c2_width : m_c1_width);
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(dummy_paths.c1_layers.size() > c1_idx, "dummy c1 layers too small");
            const auto chunk_it = dummy_paths.c1_layers[c1_idx].find(child_idx);
            CHECK_AND_ASSERT_THROW_MES((start + chunk_it->second.size()) == end, "dummy c1 layer size mismatch");
            path.c1_layers.push_back(chunk_it->second);

            ++c1_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(dummy_paths.c2_layers.size() > c2_idx, "dummy c2 layers too small");
            const auto chunk_it = dummy_paths.c2_layers[c2_idx].find(child_idx);
            CHECK_AND_ASSERT_THROW_MES((start + chunk_it->second.size()) == end, "dummy c2 layer size mismatch");
            path.c2_layers.push_back(chunk_it->second);

            ++c2_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    return path;
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeExtension CurveTrees<C1, C2>::path_to_tree_extension(const PathBytes &path_bytes,
    const PathIndexes &path_idxs) const
{
    typename CurveTrees<C1, C2>::TreeExtension tree_extension;
    tree_extension.leaves = typename CurveTrees<C1, C2>::Leaves{
            .start_leaf_tuple_idx = path_idxs.leaf_range.first,
            .tuples               = path_bytes.leaves
        };

    uint8_t layer_idx = 0;
    bool parent_is_c1 = true;
    for (const auto &chunk : path_bytes.layer_chunks)
    {
        // Get the start indexes and expected size of the last chunk
        const auto &idx_range = path_idxs.layers.at(layer_idx);
        const uint64_t start_idx = idx_range.first;
        const uint64_t end_idx = idx_range.second;

        CHECK_AND_ASSERT_THROW_MES(end_idx > start_idx,
            "path_to_tree_extension: unexpected end_idx <= start_idx");
        CHECK_AND_ASSERT_THROW_MES(chunk.chunk_bytes.size() == (end_idx - start_idx),
            "path_to_tree_extension: size mismatch last chunk");

        if (parent_is_c1)
        {
            LayerExtension<C1> layer_ext;
            layer_ext.start_idx = start_idx;
            layer_ext.update_existing_last_hash = false;
            for (const auto &child : chunk.chunk_bytes)
                layer_ext.hashes.emplace_back(m_c1->from_bytes(child));
            tree_extension.c1_layer_extensions.emplace_back(std::move(layer_ext));
        }
        else
        {
            LayerExtension<C2> layer_ext;
            layer_ext.start_idx = start_idx;
            layer_ext.update_existing_last_hash = false;
            for (const auto &child : chunk.chunk_bytes)
                layer_ext.hashes.emplace_back(m_c2->from_bytes(child));
            tree_extension.c2_layer_extensions.emplace_back(std::move(layer_ext));
        }

        ++layer_idx;
        parent_is_c1 = !parent_is_c1;
    }

    return tree_extension;
}

// Explicit instantiation
template CurveTrees<Selene, Helios>::TreeExtension CurveTrees<Selene, Helios>::path_to_tree_extension(
    const PathBytes &path_bytes,
    const PathIndexes &path_idxs) const;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees private member functions
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void CurveTrees<C1, C2>::set_valid_leaves(
    std::vector<typename C1::Scalar> &flattened_leaves_out,
    std::vector<OutputContext> &tuples_out,
    std::vector<OutputContext> &&new_outputs,
    const bool use_fast_torsion_check)
{
    TIME_MEASURE_START(set_valid_leaves);

    // Keep track of valid outputs to make sure we only use leaves from valid outputs. Can't use std::vector<bool>
    // because std::vector<bool> concurrent access is not thread safe.
    enum Boolean : uint8_t {
        False = 0,
        True = 1,
    };
    std::vector<Boolean> valid_outputs(new_outputs.size(), False);

    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    TIME_MEASURE_START(convert_valid_leaves);
    // Step 1. Multithreaded convert valid outputs into Edwards derivatives needed to get Wei coordinates
    std::vector<PreLeafTuple> pre_leaves;
    pre_leaves.resize(new_outputs.size());
    const std::size_t LEAF_CONVERT_BATCH_SIZE = 1 + (new_outputs.size() / (std::size_t)tpool.get_max_concurrency());
    for (std::size_t i = 0; i < new_outputs.size(); i += LEAF_CONVERT_BATCH_SIZE)
    {
        const std::size_t end = std::min(i + LEAF_CONVERT_BATCH_SIZE, new_outputs.size());
        tpool.submit(&waiter,
                [
                    &new_outputs,
                    &valid_outputs,
                    &pre_leaves,
                    i,
                    end,
                    use_fast_torsion_check
                ]()
                {
                    for (std::size_t j = i; j < end; ++j)
                    {
                        CHECK_AND_ASSERT_THROW_MES(valid_outputs.size() > j, "unexpected valid outputs size");
                        CHECK_AND_ASSERT_THROW_MES(!valid_outputs[j], "unexpected valid output");
                        CHECK_AND_ASSERT_THROW_MES(pre_leaves.size() > j, "unexpected pre_leaves size");

                        const auto &output_pair = new_outputs[j].output_pair;
                        const bool torsion_checked = new_outputs[j].torsion_checked;

                        try
                        {
                            pre_leaves[j] = output_to_pre_leaf_tuple(output_pair,
                                torsion_checked,
                                use_fast_torsion_check);
                        }
                        catch(...)
                        {
                            /* Invalid outputs can't be added to the tree */
                            LOG_PRINT_L2("Output " << new_outputs[j].output_id << " is invalid (out pubkey " <<
                                output_pair.output_pubkey << " , commitment " << output_pair.commitment << ")");
                            continue;
                        }

                        valid_outputs[j] = True;
                    }
                },
                true
            );
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "failed to convert outputs to ed derivatives");
    TIME_MEASURE_FINISH(convert_valid_leaves);

    TIME_MEASURE_START(collect_derivatives);
    // Step 2. Collect valid Edwards y derivatives
    const std::size_t n_valid_outputs = std::count(valid_outputs.begin(), valid_outputs.end(), True);
    const std::size_t n_valid_leaf_points = n_valid_outputs * LEAF_TUPLE_POINTS;

    // Collecting (1+y)'s
    fe *one_plus_y_vec = (fe *) malloc(n_valid_leaf_points * sizeof(fe));
    CHECK_AND_ASSERT_THROW_MES(one_plus_y_vec, "failed malloc one_plus_y_vec");

    // Collecting (1-y)'s and ((1-y)*x)'s for batch inversion
    fe *fe_batch = (fe *) malloc(n_valid_leaf_points * sizeof(fe) * 2);
    CHECK_AND_ASSERT_THROW_MES(fe_batch, "failed malloc fe_batch");

    fe *batch_inv_res = (fe *) malloc(n_valid_leaf_points * sizeof(fe) * 2);
    CHECK_AND_ASSERT_THROW_MES(batch_inv_res, "failed malloc batch_inv_res");

    std::size_t valid_i = 0, batch_i = 0;
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        CHECK_AND_ASSERT_THROW_MES(pre_leaves.size() > i, "unexpected size of pre_leaves");
        CHECK_AND_ASSERT_THROW_MES(n_valid_leaf_points > valid_i, "unexpected valid_i");

        auto &pl = pre_leaves[i];

        auto &O_derivatives = pl.O_derivatives;
        auto &I_derivatives = pl.I_derivatives;
        auto &C_derivatives = pl.C_derivatives;

        static_assert(LEAF_TUPLE_POINTS == 3, "unexpected n leaf tuple points");

        // TODO: avoid copying underlying (tried using pointer to pointers, but wasn't clean)
        memcpy(&one_plus_y_vec[valid_i++], &O_derivatives.one_plus_y, sizeof(fe));
        memcpy(&one_plus_y_vec[valid_i++], &I_derivatives.one_plus_y, sizeof(fe));
        memcpy(&one_plus_y_vec[valid_i++], &C_derivatives.one_plus_y, sizeof(fe));

        memcpy(&fe_batch[batch_i++], &O_derivatives.one_minus_y, sizeof(fe));
        memcpy(&fe_batch[batch_i++], &O_derivatives.one_minus_y_mul_x, sizeof(fe));

        memcpy(&fe_batch[batch_i++], &I_derivatives.one_minus_y, sizeof(fe));
        memcpy(&fe_batch[batch_i++], &I_derivatives.one_minus_y_mul_x, sizeof(fe));

        memcpy(&fe_batch[batch_i++], &C_derivatives.one_minus_y, sizeof(fe));
        memcpy(&fe_batch[batch_i++], &C_derivatives.one_minus_y_mul_x, sizeof(fe));
    }

    CHECK_AND_ASSERT_THROW_MES(n_valid_leaf_points == valid_i, "unexpected end valid_i");
    CHECK_AND_ASSERT_THROW_MES((n_valid_leaf_points * 2) == batch_i, "unexpected end batch_i");
    TIME_MEASURE_FINISH(collect_derivatives);

    TIME_MEASURE_START(batch_invert);
    // Step 3. Get batch inverse of all valid (1-y)'s and ((1-y)*x)'s
    // - Batch inversion is significantly faster than inverting 1 at a time
    CHECK_AND_ASSERT_THROW_MES(fe_batch_invert(batch_inv_res, fe_batch, n_valid_leaf_points * 2) == 0,
        "failed to batch invert");
    TIME_MEASURE_FINISH(batch_invert);

    TIME_MEASURE_START(get_selene_scalars);
    // Step 4. Multithreaded get Wei coordinates and convert to Selene scalars
    const std::size_t n_valid_leaf_elems = n_valid_outputs * LEAF_TUPLE_SIZE;
    flattened_leaves_out.resize(n_valid_leaf_elems);
    CHECK_AND_ASSERT_THROW_MES(flattened_leaves_out.size() == (2 * n_valid_leaf_points),
        "unexpected size of flattened leaves");

    const std::size_t DERIVATION_BATCH_SIZE = 1 + (n_valid_leaf_points / (std::size_t)tpool.get_max_concurrency());
    for (std::size_t i = 0; i < n_valid_leaf_points; i += DERIVATION_BATCH_SIZE)
    {
        const std::size_t end = std::min(n_valid_leaf_points, i + DERIVATION_BATCH_SIZE);
        tpool.submit(&waiter,
                [
                    &batch_inv_res,
                    &one_plus_y_vec,
                    &flattened_leaves_out,
                    i,
                    end
                ]()
                {
                    std::size_t point_idx = i * 2;
                    for (std::size_t j = i; j < end; ++j)
                    {
                        rct::key wei_x;
                        rct::key wei_y;
                        fe_ed_derivatives_to_wei_x_y(
                                wei_x.bytes,
                                wei_y.bytes,
                                batch_inv_res[point_idx]/*inv_one_minus_y*/,
                                one_plus_y_vec[j],
                                batch_inv_res[point_idx+1]/*inv_one_minus_y_mul_x*/
                            );

                        flattened_leaves_out[point_idx++] = tower_cycle::selene_scalar_from_bytes(wei_x);
                        flattened_leaves_out[point_idx++] = tower_cycle::selene_scalar_from_bytes(wei_y);
                    }
                },
                true
            );
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "failed to convert outputs to wei coords");
    TIME_MEASURE_FINISH(get_selene_scalars);

    // Step 5. Set valid tuples to be stored in the db
    tuples_out.clear();
    tuples_out.reserve(n_valid_outputs);
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        CHECK_AND_ASSERT_THROW_MES(new_outputs.size() > i, "unexpected size of valid outputs");

        // We can derive leaf tuples from output pairs, so we store just the output context in the db to save 32 bytes
        tuples_out.emplace_back(std::move(new_outputs[i]));
    }

    // Step 6. Clean up
    free(one_plus_y_vec);
    free(fe_batch);
    free(batch_inv_res);

    TIME_MEASURE_FINISH(set_valid_leaves);

    m_convert_valid_leaves_ms += convert_valid_leaves;
    m_collect_derivatives_ms  += collect_derivatives;
    m_batch_invert_ms         += batch_invert;
    m_get_selene_scalars_ms   += get_selene_scalars;

    m_set_valid_leaves_ms     += set_valid_leaves;

    LOG_PRINT_L2("Total time spent setting leaves: " << m_set_valid_leaves_ms / 1000
        << " , converting valid leaves: " << m_convert_valid_leaves_ms / 1000
        << " , collecting derivatives: "  << m_collect_derivatives_ms / 1000
        << " , batch invert: "            << m_batch_invert_ms / 1000
        << " , get selene scalars: "      << m_get_selene_scalars_ms / 1000);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
GrowLayerInstructions CurveTrees<C1, C2>::set_next_layer_extension(
    const GrowLayerInstructions &prev_layer_instructions,
    const bool parent_is_c2,
    const LastHashes &last_hashes,
    std::size_t &c1_last_idx_inout,
    std::size_t &c2_last_idx_inout,
    TreeExtension &tree_extension_inout) const
{
    const auto &c1_last_hashes = last_hashes.c1_last_hashes;
    const auto &c2_last_hashes = last_hashes.c2_last_hashes;

    auto &c1_layer_extensions_out = tree_extension_inout.c1_layer_extensions;
    auto &c2_layer_extensions_out = tree_extension_inout.c2_layer_extensions;

    const std::size_t parent_chunk_width = parent_is_c2 ? m_c2_width : m_c1_width;

    const auto grow_layer_instructions = get_grow_layer_instructions(
            prev_layer_instructions.old_total_parents,
            prev_layer_instructions.new_total_parents,
            parent_chunk_width,
            prev_layer_instructions.need_old_last_parent
        );

    if (parent_is_c2)
    {
        auto c2_layer_extension = get_next_layer_extension<C1, C2>(
                m_c1,
                m_c2,
                grow_layer_instructions,
                c1_last_hashes,
                c2_last_hashes,
                c1_layer_extensions_out,
                c1_last_idx_inout,
                c2_last_idx_inout
            );

        c2_layer_extensions_out.emplace_back(std::move(c2_layer_extension));
        ++c1_last_idx_inout;
    }
    else
    {
        auto c1_layer_extension = get_next_layer_extension<C2, C1>(
                m_c2,
                m_c1,
                grow_layer_instructions,
                c2_last_hashes,
                c1_last_hashes,
                c2_layer_extensions_out,
                c2_last_idx_inout,
                c1_last_idx_inout
            );

        c1_layer_extensions_out.emplace_back(std::move(c1_layer_extension));
        ++c2_last_idx_inout;
    }

    return grow_layer_instructions;
};
//----------------------------------------------------------------------------------------------------------------------
template<>
TreeRootShared CurveTrees<Selene, Helios>::get_tree_root_from_bytes(const std::size_t n_layers,
    const crypto::ec_point &tree_root) const
{
    if (n_layers == 0)
        return nullptr;

    if ((n_layers % 2) == 0)
        return fcmp_pp::helios_tree_root(m_c2->from_bytes(tree_root));
    else
        return fcmp_pp::selene_tree_root(m_c1->from_bytes(tree_root));
}
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Selene, Helios>::PathForProof CurveTrees<Selene, Helios>::path_for_proof(
    const CurveTrees<Selene, Helios>::Path &path,
    const OutputTuple &output_tuple) const
{
    // Get output's index in the path
    std::size_t output_idx_in_path = 0;
    {
        bool found = false;
        for (const auto &leaf : path.leaves)
        {
            found = output_tuple.O == leaf.O && output_tuple.I == leaf.I && output_tuple.C == leaf.C;
            if (found)
                break;
            ++output_idx_in_path;
        }
        CHECK_AND_ASSERT_THROW_MES(found, "failed to find output in path");
    }

    // Set up OutputBytes compatible with Rust FFI
    std::vector<fcmp_pp::OutputBytes> output_bytes;
    {
        output_bytes.reserve(path.leaves.size());
        for (const auto &leaf : path.leaves)
            output_bytes.push_back(leaf.to_output_bytes());
    }

    const size_t n_tree_layers = path.c1_layers.size() + path.c2_layers.size();
    const bool root_is_c1 = n_tree_layers % 2 == 1;
    const size_t n_c2_scalar_chunk_layers = path.c1_layers.size() - size_t(root_is_c1);
    const size_t n_c1_scalar_chunk_layers = path.c2_layers.size() - size_t(!root_is_c1);

    // c2 helios scalars from c1 selene points
    std::vector<std::vector<fcmp_pp::HeliosScalar>> c2_scalar_chunks;
    c2_scalar_chunks.reserve(n_c2_scalar_chunk_layers);
    for (std::size_t i = 0; i < n_c2_scalar_chunk_layers; ++i)
    {
        const auto &c1_points = path.c1_layers.at(i);
        auto &c2_scalar_layer = c2_scalar_chunks.emplace_back();
        c2_scalar_layer.reserve(m_c2_width);
        for (const auto &c1_point : c1_points)
            c2_scalar_layer.emplace_back(m_c1->point_to_cycle_scalar(c1_point));
        // Padding with 0's
        for (std::size_t j = c1_points.size(); j < m_c2_width; ++j)
            c2_scalar_layer.emplace_back(m_c2->zero_scalar());
    }

    // c1 selene scalars from c2 helios points
    std::vector<std::vector<fcmp_pp::SeleneScalar>> c1_scalar_chunks;
    c1_scalar_chunks.reserve(n_c1_scalar_chunk_layers);
    for (std::size_t i = 0; i < n_c1_scalar_chunk_layers; ++i)
    {
        const auto &c2_points = path.c2_layers.at(i);
        auto &c1_scalar_layer = c1_scalar_chunks.emplace_back();
        c1_scalar_layer.reserve(m_c1_width);
        for (const auto &c2_point : c2_points)
            c1_scalar_layer.emplace_back(m_c2->point_to_cycle_scalar(c2_point));
        // Padding with 0's
        for (std::size_t j = c2_points.size(); j < m_c1_width; ++j)
            c1_scalar_layer.emplace_back(m_c1->zero_scalar());
    }

    return PathForProof {
            .leaves           = std::move(output_bytes),
            .output_idx       = output_idx_in_path,
            .c2_scalar_chunks = std::move(c2_scalar_chunks),
            .c1_scalar_chunks = std::move(c1_scalar_chunks),
        };
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp_pp
