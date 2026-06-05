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
#include "crypto/crypto.h"
#include "fcmp_pp_crypto.h"
#include "fcmp_pp_types.h"
#include "profile_tools.h"

#include <cstring>

namespace
{
    // Struct composed of ec elems needed to get a full-fledged leaf tuple
    struct PreLeafTuple final
    {
        fcmp_pp::EdDerivatives O_derivatives;
        fcmp_pp::EdDerivatives I_derivatives;
        fcmp_pp::EdDerivatives C_derivatives;
    };
}

namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Public helper functions
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
OutputTuple output_to_tuple(const OutputPair &output_pair)
{
    const crypto::public_key &output_pubkey = output_pubkey_cref(output_pair);
    const crypto::ec_point &commitment      = commitment_cref(output_pair);

    crypto::ec_point O = output_pubkey;
    crypto::ec_point C = commitment;

    // If the output has already been checked for torsion, then we don't need to clear torsion here
    if (!output_checked_for_torsion(output_pair))
    {
        TIME_MEASURE_NS_START(clear_torsion_ns);

        if (!fcmp_pp::get_valid_torsion_cleared_point(output_pubkey, O))
            throw std::runtime_error("O is invalid for insertion to tree");
        if (!fcmp_pp::get_valid_torsion_cleared_point(commitment, C))
            throw std::runtime_error("C is invalid for insertion to tree");

        if (O != output_pubkey)
            LOG_PRINT_L2("Output pubkey has torsion: " << output_pubkey);
        if (C != commitment)
            LOG_PRINT_L2("Commitment has torsion: " << commitment);

        TIME_MEASURE_NS_FINISH(clear_torsion_ns);

        LOG_PRINT_L3("clear_torsion_ns: " << clear_torsion_ns);
    }

#if !defined(NDEBUG)
    {
        // Debug build safety checks
        crypto::ec_point O_debug;
        crypto::ec_point C_debug;
        assert(fcmp_pp::get_valid_torsion_cleared_point(output_pubkey, O_debug));
        assert(fcmp_pp::get_valid_torsion_cleared_point(commitment, C_debug));
        assert(O == O_debug);
        assert(C == C_debug);
    }
#endif

    // Redundant check for safety
    if (O == crypto::EC_I)
        throw std::runtime_error("O cannot equal identity");
    if (C == crypto::EC_I)
        throw std::runtime_error("C cannot equal identity");

    TIME_MEASURE_NS_START(derive_key_image_generator_ns);

    // Derive key image generator using original output pubkey
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, use_biased_hash_to_point(output_pair), I);

    TIME_MEASURE_NS_FINISH(derive_key_image_generator_ns);

    LOG_PRINT_L3("derive_key_image_generator_ns: " << derive_key_image_generator_ns);

    return output_tuple_from_bytes(O, I, C);
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
    const std::size_t n_threads = std::max<std::size_t>(1, tpool.get_max_concurrency());

    const std::size_t HASH_BATCH_SIZE = std::max<std::size_t>(1, (n_chunks / n_threads));
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

    // 5. Check if the last chunk is full
    const bool last_chunk_is_full = old_total_parents > 0 && offset == 0;

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

    const bool last_chunk_is_full                    = old_total_parents > 0 && offset == 0;
    const bool adding_members_to_existing_last_chunk = old_total_parents > 0 && !last_chunk_is_full
        && new_total_children > old_total_children;
    const bool need_old_last_parent                  = adding_members_to_existing_last_chunk;

    uint64_t next_parent_start_index = old_total_parents;
    if (need_old_last_parent)
    {
        // If we're updating the last parent, we need to bring the starting parent index back 1
        CHECK_AND_ASSERT_THROW_MES(old_total_parents > 0, "No old parents but last parent is supposed to change");
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

    const crypto::ec_point &O = (crypto::ec_point&) o.O;
    const crypto::ec_point &I = (crypto::ec_point&) o.I;
    const crypto::ec_point &C = (crypto::ec_point&) o.C;

    // TODO: we end up decompressing O and C twice, both in here and when checking the points for torsion. It's worth
    // checking how much of an impact that has on performance.
    PreLeafTuple plt;
    if (!fcmp_pp::point_to_ed_derivatives(O, plt.O_derivatives))
        throw std::runtime_error("failed to get ed derivatives from O");
    if (!fcmp_pp::point_to_ed_derivatives(I, plt.I_derivatives))
        throw std::runtime_error("failed to get ed derivatives from I");
    if (!fcmp_pp::point_to_ed_derivatives(C, plt.C_derivatives))
        throw std::runtime_error("failed to get ed derivatives from C");

    TIME_MEASURE_NS_FINISH(point_to_ed_derivatives_ns);

    LOG_PRINT_L3("point_to_ed_derivatives_ns: " << point_to_ed_derivatives_ns);

    return plt;
}
//----------------------------------------------------------------------------------------------------------------------
static PreLeafTuple output_to_pre_leaf_tuple(const OutputPair &output_pair)
{
    const auto o = output_to_tuple(output_pair);
    return output_tuple_to_pre_leaf_tuple(o);
}
//----------------------------------------------------------------------------------------------------------------------
static CurveTrees<Selene, Helios>::LeafTuple pre_leaf_tuple_to_leaf_tuple(const PreLeafTuple &plt)
{
    crypto::ec_coord O_x, O_y, I_x, I_y, C_x, C_y;
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
    std::vector<std::vector<UnifiedOutput>> &&new_outputs) const
{
    TreeExtension tree_extension;
    tree_extension.leaves.start_idx = old_n_leaf_tuples;

    if (new_outputs.empty())
        return tree_extension;

    TIME_MEASURE_START(sorting_outputs);

    // Sort the outputs by order they appear in the chain
    // Note: the outputs are expected to be grouped by last locked block
    std::vector<UnifiedOutput> flat_sorted_outputs;
    for (auto &unsorted_outputs : new_outputs)
    {
        const auto sort_fn = [](const UnifiedOutput &a, const UnifiedOutput &b){return a.unified_id < b.unified_id;};
        std::sort(unsorted_outputs.begin(), unsorted_outputs.end(), sort_fn);

        // No duplicates allowed
        const auto dup_check = [](const UnifiedOutput &a, const UnifiedOutput &b){return a.unified_id == b.unified_id;};
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
    this->outputs_to_leaves(std::move(flat_sorted_outputs),
        flattened_leaves,
        tree_extension.leaves.tuples);

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
    std::vector<std::vector<UnifiedOutput>> &&new_outputs) const;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees private member functions
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void CurveTrees<C1, C2>::outputs_to_leaves(std::vector<UnifiedOutput> &&new_outputs,
    std::vector<typename C1::Scalar> &flattened_leaves_out,
    std::vector<UnifiedOutput> &valid_outputs_out) const
{
    flattened_leaves_out.clear();
    valid_outputs_out.clear();

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
    const std::size_t n_threads = std::max<std::size_t>(1, tpool.get_max_concurrency());

    TIME_MEASURE_START(convert_valid_leaves);
    // Step 1. Multithreaded convert valid outputs into Edwards derivatives needed to get Wei coordinates
    std::vector<PreLeafTuple> pre_leaves;
    pre_leaves.resize(new_outputs.size());
    const std::size_t LEAF_CONVERT_BATCH_SIZE = std::max<std::size_t>(1, (new_outputs.size() / n_threads));
    for (std::size_t i = 0; i < new_outputs.size(); i += LEAF_CONVERT_BATCH_SIZE)
    {
        const std::size_t end = std::min(i + LEAF_CONVERT_BATCH_SIZE, new_outputs.size());
        tpool.submit(&waiter,
                [
                    &new_outputs,
                    &valid_outputs,
                    &pre_leaves,
                    i,
                    end
                ]()
                {
                    for (std::size_t j = i; j < end; ++j)
                    {
                        CHECK_AND_ASSERT_THROW_MES(!valid_outputs.at(j), "unexpected already valid output");

                        const auto &output_pair = new_outputs.at(j).output_pair;
                        try
                        {
                            pre_leaves.at(j) = output_to_pre_leaf_tuple(output_pair);
                        }
                        catch(...)
                        {
                            /* Invalid outputs can't be added to the tree */
                            LOG_PRINT_L2("Output " << new_outputs.at(j).unified_id << " is invalid (out pubkey "
                                << output_pubkey_cref(output_pair)
                                << " , commitment " << commitment_cref(output_pair) << ")");
                            continue;
                        }

                        valid_outputs.at(j) = True;
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

    // Collecting [(1+y),(1-y),((1-y)*x)] for batch inversion
    std::unique_ptr<fe[]> one_plus_y_vec = std::make_unique<fe[]>(n_valid_leaf_points);
    std::unique_ptr<fe[]> fe_batch       = std::make_unique<fe[]>(n_valid_leaf_points * 2);
    std::unique_ptr<fe[]> batch_inv_res  = std::make_unique<fe[]>(n_valid_leaf_points * 2);

    std::size_t valid_i = 0, batch_i = 0;
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        CHECK_AND_ASSERT_THROW_MES(n_valid_leaf_points > valid_i, "unexpected valid_i");

        auto &pl = pre_leaves.at(i);

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
    // FIXME: use latest fe_batch_invert and check return value: https://github.com/j-berman/monero/pull/18
    fe_batch_invert(batch_inv_res.get(), fe_batch.get(), n_valid_leaf_points * 2);
    TIME_MEASURE_FINISH(batch_invert);

    TIME_MEASURE_START(get_selene_scalars);
    // Step 4. Multithreaded get Wei coordinates and convert to Selene scalars
    const std::size_t n_valid_leaf_elems = n_valid_outputs * LEAF_TUPLE_SIZE;
    flattened_leaves_out.resize(n_valid_leaf_elems);
    CHECK_AND_ASSERT_THROW_MES(flattened_leaves_out.size() == (2 * n_valid_leaf_points),
        "unexpected size of flattened leaves");

    const std::size_t DERIVATION_BATCH_SIZE = std::max<std::size_t>(1, (n_valid_leaf_points / n_threads));
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
                        crypto::ec_coord wei_x;
                        crypto::ec_coord wei_y;
                        fe_ed_derivatives_to_wei_x_y(
                                to_bytes(wei_x),
                                to_bytes(wei_y),
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
    valid_outputs_out.reserve(n_valid_outputs);
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        // We can derive leaf tuples from output pairs, so we store just the unified output in the db to save 32 bytes
        valid_outputs_out.emplace_back(std::move(new_outputs.at(i)));
    }
    CHECK_AND_ASSERT_THROW_MES(valid_outputs_out.size() == n_valid_outputs, "unexpected size of valid_outputs_out");

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
} //namespace curve_trees
} //namespace fcmp_pp
