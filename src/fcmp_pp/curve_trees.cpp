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
#include "ringct/rctOps.h"

#include <stdlib.h>

namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Instantiate the tower cycle types
template class CurveTrees<Helios, Selene>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Public helper functions
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
typename C::Point get_new_parent(const std::unique_ptr<C> &curve, const typename C::Chunk &new_children)
{
    return curve->hash_grow(
            curve->hash_init_point(),
            0,/*offset*/
            curve->zero_scalar(),
            new_children
        );
};
template Helios::Point get_new_parent<Helios>(const std::unique_ptr<Helios> &curve,
    const typename Helios::Chunk &new_children);
template Selene::Point get_new_parent<Selene>(const std::unique_ptr<Selene> &curve,
    const typename Selene::Chunk &new_children);
//----------------------------------------------------------------------------------------------------------------------
std::shared_ptr<CurveTreesV1> curve_trees_v1(const std::size_t helios_chunk_width, const std::size_t selene_chunk_width)
{
    std::unique_ptr<Helios> helios(new Helios());
    std::unique_ptr<Selene> selene(new Selene());
    return std::shared_ptr<CurveTreesV1>(
            new CurveTreesV1(
                std::move(helios),
                std::move(selene),
                helios_chunk_width,
                selene_chunk_width
            )
        );
};
//----------------------------------------------------------------------------------------------------------------------
OutputTuple output_to_tuple(const OutputPair &output_pair)
{
    const crypto::public_key &output_pubkey = output_pair.output_pubkey;
    const rct::key &commitment              = output_pair.commitment;

    rct::key O, C;
    if (!fcmp_pp::clear_torsion(rct::pk2rct(output_pubkey), O))
        throw std::runtime_error("output pubkey is invalid");
    if (!fcmp_pp::clear_torsion(commitment, C))
        throw std::runtime_error("commitment is invalid");

    if (O == rct::I)
        throw std::runtime_error("O cannot equal identity");
    if (C == rct::I)
        throw std::runtime_error("C cannot equal identity");

    // Must use the original output pubkey to derive I to prevent double spends, since torsioned outputs yield a
    // a distinct I and key image from their respective torsion cleared output (and torsioned outputs are spendable
    // before fcmp++)
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, I);

    rct::key I_rct = rct::pt2rct(I);

    return OutputTuple{
        .O = std::move(O),
        .I = std::move(I_rct),
        .C = std::move(C),
    };
}
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
            MDEBUG("Updating root layer and including the existing root in next children");
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

    MDEBUG("First chunk existing_hash: " << curve->to_string(existing_hash) << " , start_offset: " << start_offset
        << " , prior_child_after_offset: " << curve->to_string(prior_child_after_offset));

    for (std::size_t i = 0; i < chunk_size; ++i)
        MDEBUG("Hashing child in first chunk " << curve->to_string(chunk_start[i]));

    // Do the hash
    auto chunk_hash = curve->hash_grow(
            existing_hash,
            start_offset,
            prior_child_after_offset,
            chunk
        );

    MDEBUG("First chunk result: " << curve->to_string(chunk_hash) << " , chunk_size: " << chunk_size);

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
        MDEBUG("Child chunk_start_idx " << chunk_start_idx << " hashing child " << curve->to_string(chunk_start[i]));

    auto chunk_hash = get_new_parent(curve, chunk);

    MDEBUG("Child chunk_start_idx " << chunk_start_idx << " result: " << curve->to_string(chunk_hash)
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
    std::size_t chunk_size = std::min(new_child_scalars.size(), chunk_width - start_offset);

    CHECK_AND_ASSERT_THROW_MES(new_child_scalars.size() >= chunk_size, "unexpected first chunk size");

    const std::size_t n_chunks = 1 // first chunk
        + (new_child_scalars.size() - chunk_size) / chunk_width // middle chunks
        + (((new_child_scalars.size() - chunk_size) % chunk_width > 0) ? 1 : 0); // final chunk

    parents_out.hashes.resize(n_chunks);

    MDEBUG("First chunk_size: " << chunk_size << " , num new child scalars: " << new_child_scalars.size()
        << " , start_offset: " << start_offset << " , parent layer start idx: " << parents_out.start_idx);

    // Hash all chunks in parallel
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    // Hash the first chunk
    tpool.submit(&waiter,
            [
                &curve,
                &old_last_child,
                &old_last_parent,
                &new_child_scalars,
                &parents_out,
                start_offset,
                chunk_size
            ]()
            {
                auto &hash_out = parents_out.hashes[0];
                hash_first_chunk(curve,
                    old_last_child,
                    old_last_parent,
                    start_offset,
                    new_child_scalars,
                    chunk_size,
                    hash_out);
            },
            true
        );

    // Hash chunks of child scalars to create the parent hashes
    std::size_t chunk_start_idx = chunk_size;
    std::size_t chunk_idx = 1;
    while (chunk_start_idx < new_child_scalars.size())
    {
        // Fill a complete chunk, or add the remaining new children to the last chunk
        chunk_size = std::min(chunk_width, new_child_scalars.size() - chunk_start_idx);

        CHECK_AND_ASSERT_THROW_MES(chunk_idx < parents_out.hashes.size(), "unexpected chunk_idx");

        tpool.submit(&waiter,
                [
                    &curve,
                    &new_child_scalars,
                    &parents_out,
                    chunk_start_idx,
                    chunk_size,
                    chunk_idx
                ]()
                {
                    auto &hash_out = parents_out.hashes[chunk_idx];
                    hash_next_chunk(curve, chunk_start_idx, new_child_scalars, chunk_size, hash_out);
                },
                true
            );

        // Advance to the next chunk
        chunk_start_idx += chunk_size;

        CHECK_AND_ASSERT_THROW_MES(chunk_start_idx <= new_child_scalars.size(), "unexpected chunk start idx");

        ++chunk_idx;
    }

    CHECK_AND_ASSERT_THROW_MES(chunk_idx == n_chunks, "unexpected n chunks");
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
    MDEBUG("parent_chunk_width: "                   << parent_chunk_width
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
            .old_total_children                = old_total_children,
            .new_total_children                = new_total_children,
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

    MDEBUG("parent_chunk_width: "                   << leaf_layer_chunk_width
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
            .old_total_children                = old_total_children,
            .new_total_children                = new_total_children,
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
static TrimLayerInstructions get_trim_layer_instructions(
    const uint64_t old_total_children,
    const uint64_t new_total_children,
    const std::size_t parent_chunk_width,
    const bool last_child_will_change)
{
    CHECK_AND_ASSERT_THROW_MES(new_total_children > 0, "new total children must be > 0");
    CHECK_AND_ASSERT_THROW_MES(old_total_children >= new_total_children,
        "old_total_children must be >= new_total_children");

    // Calculate old and new total number of parents using totals for children
    const uint64_t old_total_parents = 1 + ((old_total_children - 1) / parent_chunk_width);
    const uint64_t new_total_parents = 1 + ((new_total_children - 1) / parent_chunk_width);

    CHECK_AND_ASSERT_THROW_MES(old_total_parents >= new_total_parents,
        "old_total_parents must be >= new_total_parents");
    CHECK_AND_ASSERT_THROW_MES(new_total_children > new_total_parents,
        "new_total_children must be > new_total_parents");

    const std::size_t old_offset = old_total_children % parent_chunk_width;
    const std::size_t new_offset = new_total_children % parent_chunk_width;

    // Get the number of existing children in what will become the new last chunk after trimming
    const uint64_t new_last_chunk_old_num_children = (old_total_parents > new_total_parents || old_offset == 0)
        ? parent_chunk_width
        : old_offset;

    MDEBUG("new_last_chunk_old_num_children: " << new_last_chunk_old_num_children << ", new_offset: " << new_offset);

    CHECK_AND_ASSERT_THROW_MES(new_last_chunk_old_num_children >= new_offset,
        "unexpected new_last_chunk_old_num_children");

    // Get the number of children we'll be trimming from the new last chunk
    const std::size_t trim_n_children = new_offset == 0
        ? 0 // The last chunk wil remain full when the new_offset == 0
        : new_last_chunk_old_num_children - new_offset;

    // We use hash trim if we're trimming fewer elems in the last chunk than the number of elems remaining
    const bool need_last_chunk_children_to_trim = trim_n_children > 0 && trim_n_children <= new_offset;

    // Otherwise we use hash_grow
    const bool need_last_chunk_remaining_children = trim_n_children > 0 && trim_n_children > new_offset;

    CHECK_AND_ASSERT_THROW_MES(!(need_last_chunk_children_to_trim && need_last_chunk_remaining_children),
        "cannot both need last children to trim and need the remaining children");

    // If we're trimming from the new last chunk OR an element in the new last chunk will change, then we're going to
    // update the existing last hash, since its children are changing
    const bool update_existing_last_hash = trim_n_children > 0 || last_child_will_change;

    // If we're trimming using remaining children, then we're just going to call hash_grow as if the chunk is being
    // hashed for the first time, and so we don't need the existing last hash in that case, even if the hash is updating
    const bool need_existing_last_hash = update_existing_last_hash && !need_last_chunk_remaining_children;

    // We need to decrement the offset we use to hash the chunk if the last child is changing
    std::size_t hash_offset = new_offset;
    if (last_child_will_change)
    {
        hash_offset = hash_offset == 0
            ? (parent_chunk_width - 1) // chunk is full, so decrement full width by 1
            : (hash_offset - 1);
    }

    // Set the child index range so the caller knows which children to read from the tree
    uint64_t start_trim_idx = 0;
    uint64_t end_trim_idx = 0;
    if (need_last_chunk_children_to_trim)
    {
        // We'll call hash_trim to trim the children between [offset, last chunk end]
        const uint64_t chunk_boundary_start = (new_total_parents - 1) * parent_chunk_width;
        const uint64_t chunk_boundary_end   = chunk_boundary_start + parent_chunk_width;

        start_trim_idx = chunk_boundary_start + hash_offset;
        end_trim_idx   = std::min(chunk_boundary_end, old_total_children);
    }
    else if (need_last_chunk_remaining_children)
    {
        // We'll call hash_grow with the remaining children between [0, offset]
        CHECK_AND_ASSERT_THROW_MES(new_total_children >= hash_offset, "hash_offset is unexpectedly high");
        start_trim_idx = new_total_children - hash_offset;
        end_trim_idx   = new_total_children;
    }

    // If we're trimming using remaining children, then we're just going to call hash_grow with offset 0
    if (need_last_chunk_remaining_children)
    {
        hash_offset = 0;
    }

    MDEBUG("parent_chunk_width: "                    << parent_chunk_width
        << " , old_total_children: "                 << old_total_children
        << " , new_total_children: "                 << new_total_children
        << " , old_total_parents: "                  << old_total_parents
        << " , new_total_parents: "                  << new_total_parents
        << " , need_last_chunk_children_to_trim: "   << need_last_chunk_children_to_trim
        << " , need_last_chunk_remaining_children: " << need_last_chunk_remaining_children
        << " , need_existing_last_hash: "            << need_existing_last_hash
        << " , need_new_last_child: "                << last_child_will_change
        << " , update_existing_last_hash: "          << update_existing_last_hash
        << " , hash_offset: "                        << hash_offset
        << " , start_trim_idx: "                     << start_trim_idx
        << " , end_trim_idx: "                       << end_trim_idx);

    return TrimLayerInstructions{
            .parent_chunk_width                 = parent_chunk_width,
            .old_total_children                 = old_total_children,
            .new_total_children                 = new_total_children,
            .old_total_parents                  = old_total_parents,
            .new_total_parents                  = new_total_parents,
            .update_existing_last_hash          = update_existing_last_hash,
            .need_last_chunk_children_to_trim   = need_last_chunk_children_to_trim,
            .need_last_chunk_remaining_children = need_last_chunk_remaining_children,
            .need_existing_last_hash            = need_existing_last_hash,
            .need_new_last_child                = last_child_will_change,
            .hash_offset                        = hash_offset,
            .start_trim_idx                     = start_trim_idx,
            .end_trim_idx                       = end_trim_idx,
        };
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_CHILD, typename C_PARENT>
static typename fcmp_pp::curve_trees::LayerReduction<C_PARENT> get_next_layer_reduction(
    const std::unique_ptr<C_CHILD> &c_child,
    const std::unique_ptr<C_PARENT> &c_parent,
    const TrimLayerInstructions &trim_layer_instructions,
    const std::vector<typename C_PARENT::Point> &parent_last_hashes,
    const std::vector<std::vector<typename C_PARENT::Scalar>> &children_to_trim,
    const std::vector<typename C_CHILD::Point> &child_last_hashes,
    const std::size_t parent_layer_idx,
    const std::size_t child_layer_idx,
    const std::vector<LayerReduction<C_CHILD>> &child_reductions)
{
    LayerReduction<C_PARENT> layer_reduction_out;

    layer_reduction_out.new_total_parents         = trim_layer_instructions.new_total_parents;
    layer_reduction_out.update_existing_last_hash = trim_layer_instructions.update_existing_last_hash;

    if (trim_layer_instructions.need_existing_last_hash)
        CHECK_AND_ASSERT_THROW_MES(parent_last_hashes.size() > parent_layer_idx, "missing last parent hash");

    const typename C_PARENT::Point &existing_hash = trim_layer_instructions.need_existing_last_hash
        ? parent_last_hashes[parent_layer_idx]
        : c_parent->hash_init_point();

    std::vector<typename C_PARENT::Scalar> child_scalars;
    if (trim_layer_instructions.need_last_chunk_children_to_trim
        || trim_layer_instructions.need_last_chunk_remaining_children)
    {
        CHECK_AND_ASSERT_THROW_MES(children_to_trim.size() > parent_layer_idx, "missing children to trim");
        child_scalars = children_to_trim[parent_layer_idx];
    }

    typename C_PARENT::Scalar new_last_child_scalar = c_parent->zero_scalar();
    if (trim_layer_instructions.need_new_last_child)
    {
        CHECK_AND_ASSERT_THROW_MES(child_layer_idx > 0, "child index cannot be 0 here");
        CHECK_AND_ASSERT_THROW_MES(child_reductions.size() == child_layer_idx, "unexpected child layer idx");
        CHECK_AND_ASSERT_THROW_MES(child_reductions.back().update_existing_last_hash, "expected new last child");

        const typename C_CHILD::Point &new_last_child = child_reductions.back().new_last_hash;
        new_last_child_scalar = c_child->point_to_cycle_scalar(new_last_child);

        if (trim_layer_instructions.need_last_chunk_remaining_children)
        {
            child_scalars.emplace_back(std::move(new_last_child_scalar));
        }
        else if (!trim_layer_instructions.need_last_chunk_children_to_trim)
        {
            // Falling to this conditional means we're not trimming at all, just updating the old last child
            const std::size_t last_child_layer_idx = child_layer_idx - 1;
            CHECK_AND_ASSERT_THROW_MES(child_last_hashes.size() > last_child_layer_idx, "missing last child hash");

            const typename C_CHILD::Point &old_last_child = child_last_hashes[last_child_layer_idx];
            auto old_last_child_scalar = c_child->point_to_cycle_scalar(old_last_child);

            child_scalars.emplace_back(std::move(old_last_child_scalar));
        }
    }

    for (std::size_t i = 0; i < child_scalars.size(); ++i)
        MDEBUG("Hashing child " << c_parent->to_string(child_scalars[i]));

    if (trim_layer_instructions.need_last_chunk_remaining_children)
    {
        MDEBUG("hash_grow: existing_hash: " << c_parent->to_string(existing_hash)
            << " , hash_offset: " << trim_layer_instructions.hash_offset);

        layer_reduction_out.new_last_hash = c_parent->hash_grow(
            existing_hash,
            trim_layer_instructions.hash_offset,
            c_parent->zero_scalar(),
            typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()});
    }
    else
    {
        MDEBUG("hash_trim: existing_hash: " << c_parent->to_string(existing_hash)
            << " , hash_offset: "           << trim_layer_instructions.hash_offset
            << " , child_to_grow_back: "    << c_parent->to_string(new_last_child_scalar));

        layer_reduction_out.new_last_hash = c_parent->hash_trim(
            existing_hash,
            trim_layer_instructions.hash_offset,
            typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()},
            new_last_child_scalar);
    }

    MDEBUG("Result hash: " << c_parent->to_string(layer_reduction_out.new_last_hash));

    return layer_reduction_out;
}
//----------------------------------------------------------------------------------------------------------------------
static PreLeafTuple output_tuple_to_pre_leaf_tuple(const OutputTuple &o)
{
    PreLeafTuple plt;
    if (!fcmp_pp::point_to_ed_y_derivatives(o.O, plt.O_pre_x))
        throw std::runtime_error("failed to get ed y derivatives from O");
    if (!fcmp_pp::point_to_ed_y_derivatives(o.I, plt.I_pre_x))
        throw std::runtime_error("failed to get ed y derivatives from I");
    if (!fcmp_pp::point_to_ed_y_derivatives(o.C, plt.C_pre_x))
        throw std::runtime_error("failed to get ed y derivatives from C");

    return plt;
}
//----------------------------------------------------------------------------------------------------------------------
static PreLeafTuple output_to_pre_leaf_tuple(const OutputPair &output_pair)
{
    const auto o = output_to_tuple(output_pair);
    return output_tuple_to_pre_leaf_tuple(o);
}
//----------------------------------------------------------------------------------------------------------------------
static CurveTrees<Helios, Selene>::LeafTuple pre_leaf_tuple_to_leaf_tuple(const PreLeafTuple &plt)
{
    rct::key O_x, I_x, C_x;
    fcmp_pp::ed_y_derivatives_to_wei_x(plt.O_pre_x, O_x);
    fcmp_pp::ed_y_derivatives_to_wei_x(plt.I_pre_x, I_x);
    fcmp_pp::ed_y_derivatives_to_wei_x(plt.C_pre_x, C_x);

    return CurveTrees<Helios, Selene>::LeafTuple{
        .O_x = tower_cycle::selene_scalar_from_bytes(O_x),
        .I_x = tower_cycle::selene_scalar_from_bytes(I_x),
        .C_x = tower_cycle::selene_scalar_from_bytes(C_x)
    };
}
//----------------------------------------------------------------------------------------------------------------------
static CurveTrees<Helios, Selene>::LeafTuple output_tuple_to_leaf_tuple(const OutputTuple &output_tuple)
{
    const auto plt = output_tuple_to_pre_leaf_tuple(output_tuple);
    return pre_leaf_tuple_to_leaf_tuple(plt);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees public member functions
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LeafTuple CurveTrees<Helios, Selene>::leaf_tuple(const OutputPair &output_pair) const
{
    const auto plt = output_to_pre_leaf_tuple(output_pair);
    return pre_leaf_tuple_to_leaf_tuple(plt);
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<typename C2::Scalar> CurveTrees<C1, C2>::flatten_leaves(std::vector<LeafTuple> &&leaves) const
{
    std::vector<typename C2::Scalar> flattened_leaves;
    flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

    for (auto &l : leaves)
    {
        flattened_leaves.emplace_back(std::move(l.O_x));
        flattened_leaves.emplace_back(std::move(l.I_x));
        flattened_leaves.emplace_back(std::move(l.C_x));
    }

    return flattened_leaves;
};

// Explicit instantiation
template std::vector<Selene::Scalar> CurveTrees<Helios, Selene>::flatten_leaves(
    std::vector<LeafTuple> &&leaves) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeExtension CurveTrees<C1, C2>::get_tree_extension(
    const uint64_t old_n_leaf_tuples,
    const LastHashes &existing_last_hashes,
    std::vector<OutputContext> &&new_outputs) const
{
    TreeExtension tree_extension;
    tree_extension.leaves.start_leaf_tuple_idx = old_n_leaf_tuples;

    if (new_outputs.empty())
        return tree_extension;

    // Sort the outputs by order they appear in the chain
    const auto sort_fn = [](const OutputContext &a, const OutputContext &b) { return a.output_id < b.output_id; };
    std::sort(new_outputs.begin(), new_outputs.end(), sort_fn);

    // Convert sorted outputs into leaf tuples, place each element of each leaf tuple in a flat vector to be hashed,
    // and place the outputs in a tree extension struct for insertion into the db. We ignore invalid outputs, since
    // they cannot be inserted to the tree.
    std::vector<typename C2::Scalar> flattened_leaves;
    this->set_valid_leaves(flattened_leaves, tree_extension.leaves.tuples, std::move(new_outputs));

    if (flattened_leaves.empty())
        return tree_extension;

    MDEBUG("Getting extension for layer 0");
    auto grow_layer_instructions = get_leaf_layer_grow_instructions(
        old_n_leaf_tuples,
        tree_extension.leaves.tuples.size(),
        LEAF_TUPLE_SIZE,
        m_leaf_layer_chunk_width);

    if (grow_layer_instructions.need_old_last_parent)
        CHECK_AND_ASSERT_THROW_MES(!existing_last_hashes.c2_last_hashes.empty(), "missing last c2 parent");

    // Hash the leaf layer
    auto leaf_parents = hash_children_chunks(m_c2,
            nullptr, // We never need the old last child from leaf layer because the leaf layer is always append-only
            grow_layer_instructions.need_old_last_parent ? &existing_last_hashes.c2_last_hashes[0] : nullptr,
            grow_layer_instructions.start_offset,
            grow_layer_instructions.next_parent_start_index,
            flattened_leaves,
            m_leaf_layer_chunk_width
        );

    CHECK_AND_ASSERT_THROW_MES(
        (leaf_parents.start_idx + leaf_parents.hashes.size()) == grow_layer_instructions.new_total_parents,
        "unexpected num leaf parents extended");

    tree_extension.c2_layer_extensions.emplace_back(std::move(leaf_parents));

    // Alternate between hashing c2 children, c1 children, c2, c1, ...
    bool parent_is_c1 = true;

    std::size_t c1_last_idx = 0;
    std::size_t c2_last_idx = 0;
    while (grow_layer_instructions.new_total_parents > 1)
    {
        MDEBUG("Getting extension for layer " << (c1_last_idx + c2_last_idx + 1));

        const uint64_t new_total_children = grow_layer_instructions.new_total_parents;

        grow_layer_instructions = this->set_next_layer_extension(
                grow_layer_instructions,
                parent_is_c1,
                existing_last_hashes,
                c1_last_idx,
                c2_last_idx,
                tree_extension
            );

        // Sanity check to make sure we're making progress to exit the while loop
        CHECK_AND_ASSERT_THROW_MES(grow_layer_instructions.new_total_parents < new_total_children,
            "expect fewer parents than children in every layer");

        parent_is_c1 = !parent_is_c1;
    }

    return tree_extension;
};

// Explicit instantiation
template CurveTrees<Helios, Selene>::TreeExtension CurveTrees<Helios, Selene>::get_tree_extension(
    const uint64_t old_n_leaf_tuples,
    const LastHashes &existing_last_hashes,
    std::vector<OutputContext> &&new_outputs) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<TrimLayerInstructions> CurveTrees<C1, C2>::get_trim_instructions(
    const uint64_t old_n_leaf_tuples,
    const uint64_t trim_n_leaf_tuples) const
{
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples >= trim_n_leaf_tuples, "cannot trim more leaves than exist");
    CHECK_AND_ASSERT_THROW_MES(trim_n_leaf_tuples > 0, "must be trimming some leaves");

    std::vector<TrimLayerInstructions> trim_instructions;

    if (old_n_leaf_tuples == trim_n_leaf_tuples)
        return trim_instructions;

    // Get trim instructions for the leaf layer
    {
        const uint64_t old_total_leaves = old_n_leaf_tuples * LEAF_TUPLE_SIZE;
        const uint64_t new_total_leaves = (old_n_leaf_tuples - trim_n_leaf_tuples) * LEAF_TUPLE_SIZE;

        const std::size_t parent_chunk_width = m_leaf_layer_chunk_width;

        // Leaf layer's last child never changes since leaf layer is pop-/append-only
        const bool last_child_will_change = false;

        auto trim_leaf_layer_instructions = get_trim_layer_instructions(
            old_total_leaves,
            new_total_leaves,
            parent_chunk_width,
            last_child_will_change);

        trim_instructions.emplace_back(std::move(trim_leaf_layer_instructions));
    }

    bool use_c2 = false;
    while (trim_instructions.back().new_total_parents > 1)
    {
        auto trim_layer_instructions = get_trim_layer_instructions(
            trim_instructions.back().old_total_parents,
            trim_instructions.back().new_total_parents,
            use_c2 ? m_c2_width : m_c1_width,
            trim_instructions.back().update_existing_last_hash);

        trim_instructions.emplace_back(std::move(trim_layer_instructions));
        use_c2 = !use_c2;
    }

    return trim_instructions;
}

// Explicit instantiation
template std::vector<TrimLayerInstructions> CurveTrees<Helios, Selene>::get_trim_instructions(
    const uint64_t old_n_leaf_tuples,
    const uint64_t trim_n_leaf_tuples) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeReduction CurveTrees<C1, C2>::get_tree_reduction(
    const std::vector<TrimLayerInstructions> &trim_instructions,
    const LastChunkChildrenToTrim &children_to_trim,
    const LastHashes &last_hashes) const
{
    TreeReduction tree_reduction_out;

    if (trim_instructions.empty())
    {
        tree_reduction_out.new_total_leaf_tuples = 0;
        return tree_reduction_out;
    }

    CHECK_AND_ASSERT_THROW_MES((trim_instructions[0].new_total_children % LEAF_TUPLE_SIZE) == 0,
        "unexpected new total leaves");
    const uint64_t new_total_leaf_tuples = trim_instructions[0].new_total_children / LEAF_TUPLE_SIZE;
    tree_reduction_out.new_total_leaf_tuples = new_total_leaf_tuples;

    bool use_c2 = true;
    std::size_t c1_idx = 0;
    std::size_t c2_idx = 0;

    for (const auto &trim_layer_instructions : trim_instructions)
    {
        MDEBUG("Trimming layer " << (c1_idx + c2_idx) << " (c1_idx: " << c1_idx << " , c2_idx: " << c2_idx << ")");

        if (use_c2)
        {
            auto c2_layer_reduction_out = get_next_layer_reduction(
                    m_c1,
                    m_c2,
                    trim_layer_instructions,
                    last_hashes.c2_last_hashes,
                    children_to_trim.c2_children,
                    last_hashes.c1_last_hashes,
                    c2_idx,
                    c1_idx,
                    tree_reduction_out.c1_layer_reductions
                );

            tree_reduction_out.c2_layer_reductions.emplace_back(std::move(c2_layer_reduction_out));
            ++c2_idx;
        }
        else
        {
            auto c1_layer_reduction_out = get_next_layer_reduction(
                    m_c2,
                    m_c1,
                    trim_layer_instructions,
                    last_hashes.c1_last_hashes,
                    children_to_trim.c1_children,
                    last_hashes.c2_last_hashes,
                    c1_idx,
                    c2_idx,
                    tree_reduction_out.c2_layer_reductions
                );

            tree_reduction_out.c1_layer_reductions.emplace_back(std::move(c1_layer_reduction_out));
            ++c1_idx;
        }

        use_c2 = !use_c2;
    }

    return tree_reduction_out;
};

// Explicit instantiation
template CurveTrees<Helios, Selene>::TreeReduction CurveTrees<Helios, Selene>::get_tree_reduction(
    const std::vector<TrimLayerInstructions> &trim_instructions,
    const LastChunkChildrenToTrim &children_to_trim,
    const LastHashes &last_hashes) const;
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::size_t CurveTrees<C1, C2>::n_layers(const uint64_t n_leaf_tuples) const
{
    if (n_leaf_tuples == 0)
        return 0;

    uint64_t n_children = n_leaf_tuples;
    std::size_t n_layers = 0;
    bool use_c2 = true;
    do
    {
        const std::size_t parent_chunk_width = use_c2 ? m_c2_width : m_c1_width;
        const uint64_t n_parents = ((n_children - 1) / parent_chunk_width) + 1;
        n_children = n_parents;
        use_c2 = !use_c2;
        ++n_layers;
    }
    while (n_children > 1);

    return n_layers;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::PathIndexes CurveTrees<C1, C2>::get_path_indexes(const uint64_t n_leaf_tuples,
    const uint64_t leaf_tuple_idx) const
{
    CurveTrees<C1, C2>::PathIndexes path_indexes_out;

    if (n_leaf_tuples <= leaf_tuple_idx)
        return path_indexes_out;

    MERROR("n_leaf_tuples: " << n_leaf_tuples << " , leaf_tuple_idx: " << leaf_tuple_idx);

    uint64_t child_idx = leaf_tuple_idx;
    uint64_t n_children = n_leaf_tuples;
    bool leaf_layer = true;
    bool parent_is_c2 = true;
    do
    {
        const std::size_t parent_chunk_width = parent_is_c2 ? m_c2_width : m_c1_width;
        const uint64_t parent_idx = child_idx / parent_chunk_width;

        const uint64_t start_range = parent_idx * parent_chunk_width;
        const uint64_t end_range = std::min(n_children, start_range + parent_chunk_width);

        const uint64_t n_parents = (leaf_layer || n_children > 1)
            ? (((n_children - 1) / parent_chunk_width) + 1)
            : 0;

        MDEBUG("start_range: " << start_range
            << " ,  end_range: " << end_range
            << " ,  parent_chunk_width: " << parent_chunk_width
            << " ,  n_parents: " << n_parents
            << " ,  parent_idx: " << parent_idx);

        std::pair<uint64_t, uint64_t> range = { start_range, end_range };
        if (leaf_layer)
            path_indexes_out.leaf_range = std::move(range);
        else if (parent_is_c2)
            path_indexes_out.c1_layers.emplace_back(std::move(range));
        else
            path_indexes_out.c2_layers.emplace_back(std::move(range));

        child_idx = parent_idx;
        n_children = n_parents;

        leaf_layer = false;
        parent_is_c2 = !parent_is_c2;
    }
    while (n_children > 0);

    return path_indexes_out;
}
//----------------------------------------------------------------------------------------------------------------------
template<>
bool CurveTrees<Helios, Selene>::audit_path(const CurveTrees<Helios, Selene>::Path &path,
    const OutputPair &output,
    const std::size_t n_leaf_tuples_in_tree) const
{
    // TODO: use the leaf idx to know exactly which parent indexes we expect and how many parent hashes we expect at each layer

    // Cleaner refs
    const auto &leaves = path.leaves;
    const auto &c1_layers = path.c1_layers;
    const auto &c2_layers = path.c2_layers;

    // Initial checks
    CHECK_AND_ASSERT_MES(!leaves.empty(),    false, "empty leaves");
    CHECK_AND_ASSERT_MES(!c2_layers.empty(), false, "empty c2 layers");
    CHECK_AND_ASSERT_MES(leaves.size() <= m_c2_width, false, "too many leaves");

    const std::size_t n_layers = c1_layers.size() + c2_layers.size();
    CHECK_AND_ASSERT_MES(n_layers == this->n_layers(n_leaf_tuples_in_tree), false, "unexpected n_layers");

    // Make sure output tuple is present in leaves
    const auto output_tuple = output_to_tuple(output);
    bool found = false;
    for (std::size_t i = 0; !found && i < leaves.size(); ++i)
        found = (output_tuple.O == leaves[i].O && output_tuple.I == leaves[i].I && output_tuple.C == leaves[i].C);
    CHECK_AND_ASSERT_MES(found, false, "did not find output in chunk of leaves");

    // Collect leaves so we can hash them
    std::vector<Selene::Scalar> leaf_scalars;
    leaf_scalars.reserve(leaves.size() * LEAF_TUPLE_SIZE);
    for (auto &l : leaves)
    {
        auto leaf_tuple = output_tuple_to_leaf_tuple(l);
        leaf_scalars.emplace_back(std::move(leaf_tuple.O_x));
        leaf_scalars.emplace_back(std::move(leaf_tuple.I_x));
        leaf_scalars.emplace_back(std::move(leaf_tuple.C_x));
    }

    // Hash the leaf chunk
    MDEBUG("Path contains " << leaves.size() << " leaf tuples");
    const Selene::Chunk leaf_chunk{leaf_scalars.data(), leaf_scalars.size()};
    const Selene::Point leaf_parent_hash = get_new_parent<Selene>(m_c2, leaf_chunk);
    const auto leaf_parent_bytes = m_c2->to_bytes(leaf_parent_hash);

    // Make sure leaf chunk hash is present in first c2_layer
    const auto &first_c2_layer = c2_layers.front();
    found = false;
    MDEBUG("Looking for leaf chunk hash: " << m_c2->to_string(leaf_parent_hash) << " among " << first_c2_layer.size() << " hashes");
    for (std::size_t i = 0; !found && i < first_c2_layer.size(); ++i)
    {
        MDEBUG("Reading: " << m_c2->to_string(first_c2_layer[i]));
        found = (leaf_parent_bytes == m_c2->to_bytes(first_c2_layer[i]));
    }
    CHECK_AND_ASSERT_MES(found, false, "did not find leaf chunk hash");

    // If there are no more layers to audit, we're done
    if (c2_layers.size() == 1 && c1_layers.empty())
    {
        // We must have encountered the root
        CHECK_AND_ASSERT_MES(c2_layers.front().size() == 1, false, "expected to encounter c2 root");
        return true;
    }

    // Continue hashing every layer chunk until there are no more layers
    bool parent_is_c1 = true;
    std::size_t c1_idx = 0, c2_idx = 0;
    for (std::size_t i = 0; i < n_layers; ++i)
    {
        CHECK_AND_ASSERT_MES(c1_layers.size() > c1_idx, false, "low c1_idx");
        CHECK_AND_ASSERT_MES(c2_layers.size() > c2_idx, false, "low c2_idx");

        auto &c1_layer = c1_layers[c1_idx];
        auto &c2_layer = c2_layers[c2_idx];

        // TODO: template
        if (parent_is_c1)
        {
            // Collect c1 scalars so we can hash them
            std::vector<Helios::Scalar> c1_scalars;
            c1_scalars.reserve(c2_layer.size());
            for (auto &c2_point : c2_layer)
            {
                c1_scalars.emplace_back(m_c2->point_to_cycle_scalar(c2_point));
                MDEBUG("Path hashing " << m_c1->to_string(c1_scalars.back()));
            }

            // Hash c1 scalars
            const Helios::Chunk chunk{c1_scalars.data(), c1_scalars.size()};
            const Helios::Point hash = get_new_parent<Helios>(m_c1, chunk);
            const auto bytes = m_c1->to_bytes(hash);

            // Make sure hash is present in c1 layer
            MDEBUG("Looking for c1 hash: " << m_c1->to_string(hash) << " among " << c1_layer.size() << " hashes");
            found = false;
            for (std::size_t j = 0; !found && j < c1_layer.size(); ++j)
            {
                MDEBUG("Reading: " << m_c1->to_string(c1_layer[j]));
                found = (bytes == m_c1->to_bytes(c1_layer[j]));
            }
            CHECK_AND_ASSERT_MES(found, false, "did not find c1 hash");

            // Check if we have encountered the root
            if (c1_layer.size() == 1 && (c1_idx + 1) >= c1_layers.size() && (c2_idx + 1) >= c2_layers.size())
                break;

            ++c2_idx;
        }
        else
        {
            // Collect c2 scalars so we can hash them
            std::vector<Selene::Scalar> c2_scalars;
            c2_scalars.reserve(c1_layer.size());
            for (auto &c1_point : c1_layer)
                c2_scalars.emplace_back(m_c1->point_to_cycle_scalar(c1_point));

            // Hash c2 scalars
            const Selene::Chunk chunk{c2_scalars.data(), c2_scalars.size()};
            const Selene::Point hash = get_new_parent<Selene>(m_c2, chunk);
            const auto bytes = m_c2->to_bytes(hash);

            // Make sure hash is present in c2 layer
            found = false;
            for (std::size_t j = 0; !found && j < c2_layer.size(); ++j)
                found = (bytes == m_c2->to_bytes(c2_layer[j]));
            CHECK_AND_ASSERT_MES(found, false, "did not find c2 hash");

            // Check if we have encountered the root
            if (c2_layer.size() == 1 && (c1_idx + 1) >= c1_layers.size() && (c2_idx + 1) >= c2_layers.size())
                break;

            ++c1_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees private member functions
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
void CurveTrees<C1, C2>::set_valid_leaves(
    std::vector<typename C2::Scalar> &flattened_leaves_out,
    std::vector<OutputContext> &tuples_out,
    std::vector<OutputContext> &&new_outputs) const
{
    // Keep track of valid outputs to make sure we only use leaves from valid outputs. Can't use std::vector<bool>
    // because std::vector<bool> concurrent access is not thread safe.
    enum Boolean : uint8_t {
        False = 0,
        True = 1,
    };
    std::vector<Boolean> valid_outputs(new_outputs.size(), False);

    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    tools::threadpool::waiter waiter(tpool);

    // Step 1. Multithreaded convert valid outputs into Edwards y derivatives needed to get Wei x coordinates
    // TODO: investigate batched threading (as opposed to small tasks)
    std::vector<PreLeafTuple> pre_leaves;
    pre_leaves.resize(new_outputs.size());
    for (std::size_t i = 0; i < new_outputs.size(); ++i)
    {
        tpool.submit(&waiter,
                [
                    &new_outputs,
                    &valid_outputs,
                    &pre_leaves,
                    i
                ]()
                {
                    CHECK_AND_ASSERT_THROW_MES(valid_outputs.size() > i, "unexpected valid outputs size");
                    CHECK_AND_ASSERT_THROW_MES(!valid_outputs[i], "unexpected valid output");
                    CHECK_AND_ASSERT_THROW_MES(pre_leaves.size() > i, "unexpected pre_leaves size");

                    const auto &output_pair = new_outputs[i].output_pair;

                    try { pre_leaves[i] = output_to_pre_leaf_tuple(output_pair); }
                    catch(...) { /* Invalid outputs can't be added to the tree */ return; }

                    valid_outputs[i] = True;
                },
                true
            );
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "failed to convert outputs to ed y derivatives");

    // Step 2. Collect valid Edwards y derivatives
    const std::size_t n_valid_outputs = std::count(valid_outputs.begin(), valid_outputs.end(), True);
    const std::size_t n_valid_leaf_elems = n_valid_outputs * LEAF_TUPLE_SIZE;

    // Collecting (1+y)'s
    fe *one_plus_y_vec = (fe *) malloc(n_valid_leaf_elems * sizeof(fe));
    CHECK_AND_ASSERT_THROW_MES(one_plus_y_vec, "failed malloc one_plus_y_vec");

    // Collecting (1-y)'s
    fe *one_minus_y_vec = (fe *) malloc(n_valid_leaf_elems * sizeof(fe));
    CHECK_AND_ASSERT_THROW_MES(one_minus_y_vec, "failed malloc one_minus_y_vec");

    std::size_t valid_i = 0;
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        CHECK_AND_ASSERT_THROW_MES(pre_leaves.size() > i, "unexpected size of pre_leaves");
        CHECK_AND_ASSERT_THROW_MES(n_valid_leaf_elems > valid_i, "unexpected valid_i");

        auto &pl = pre_leaves[i];

        auto &O_pre_x = pl.O_pre_x;
        auto &I_pre_x = pl.I_pre_x;
        auto &C_pre_x = pl.C_pre_x;

        static_assert(LEAF_TUPLE_SIZE == 3, "unexpected leaf tuple size");

        // TODO: avoid copying underlying (tried using pointer to pointers, but wasn't clean)
        memcpy(&one_plus_y_vec[valid_i], &O_pre_x.one_plus_y, sizeof(fe));
        memcpy(&one_plus_y_vec[valid_i+1], &I_pre_x.one_plus_y, sizeof(fe));
        memcpy(&one_plus_y_vec[valid_i+2], &C_pre_x.one_plus_y, sizeof(fe));

        memcpy(&one_minus_y_vec[valid_i], &O_pre_x.one_minus_y, sizeof(fe));
        memcpy(&one_minus_y_vec[valid_i+1], &I_pre_x.one_minus_y, sizeof(fe));
        memcpy(&one_minus_y_vec[valid_i+2], &C_pre_x.one_minus_y, sizeof(fe));

        valid_i += LEAF_TUPLE_SIZE;
    }

    CHECK_AND_ASSERT_THROW_MES(n_valid_leaf_elems == valid_i, "unexpected end valid_i");

    // Step 3. Get batch inverse of all valid (1-y)'s
    // - Batch inversion is significantly faster than inverting 1 at a time
    fe *inv_one_minus_y_vec = (fe *) malloc(n_valid_leaf_elems * sizeof(fe));
    CHECK_AND_ASSERT_THROW_MES(inv_one_minus_y_vec, "failed malloc inv_one_minus_y_vec");
    CHECK_AND_ASSERT_THROW_MES(fe_batch_invert(inv_one_minus_y_vec, one_minus_y_vec, n_valid_leaf_elems) == 0,
        "failed to batch invert");

    // Step 4. Multithreaded get Wei x's and convert to Selene scalars
    // TODO: investigate batched threading (as opposed to small tasks)
    flattened_leaves_out.resize(n_valid_leaf_elems);
    for (std::size_t i = 0; i < n_valid_leaf_elems; ++i)
    {
        tpool.submit(&waiter,
                [
                    &inv_one_minus_y_vec,
                    &one_plus_y_vec,
                    &flattened_leaves_out,
                    i
                ]()
                {
                    rct::key wei_x;
                    fe_ed_y_derivatives_to_wei_x(wei_x.bytes, inv_one_minus_y_vec[i], one_plus_y_vec[i]);
                    flattened_leaves_out[i] = tower_cycle::selene_scalar_from_bytes(wei_x);
                },
                true
            );
    }

    CHECK_AND_ASSERT_THROW_MES(waiter.wait(), "failed to convert outputs to wei x coords");

    // Step 5. Set valid tuples to be stored in the db
    tuples_out.clear();
    tuples_out.reserve(n_valid_outputs);
    for (std::size_t i = 0; i < valid_outputs.size(); ++i)
    {
        if (!valid_outputs[i])
            continue;

        CHECK_AND_ASSERT_THROW_MES(new_outputs.size() > i, "unexpected size of valid outputs");

        // We can derive {O.x,I.x,C.x} from output pairs, so we store just the output context in the db to save 32 bytes
        tuples_out.emplace_back(std::move(new_outputs[i]));
    }

    // Step 6. Clean up
    free(one_plus_y_vec);
    free(one_minus_y_vec);
    free(inv_one_minus_y_vec);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
GrowLayerInstructions CurveTrees<C1, C2>::set_next_layer_extension(
    const GrowLayerInstructions &prev_layer_instructions,
    const bool parent_is_c1,
    const LastHashes &last_hashes,
    std::size_t &c1_last_idx_inout,
    std::size_t &c2_last_idx_inout,
    TreeExtension &tree_extension_inout) const
{
    const auto &c1_last_hashes = last_hashes.c1_last_hashes;
    const auto &c2_last_hashes = last_hashes.c2_last_hashes;

    auto &c1_layer_extensions_out = tree_extension_inout.c1_layer_extensions;
    auto &c2_layer_extensions_out = tree_extension_inout.c2_layer_extensions;

    const std::size_t parent_chunk_width = parent_is_c1 ? m_c1_width : m_c2_width;

    const auto grow_layer_instructions = get_grow_layer_instructions(
            prev_layer_instructions.old_total_parents,
            prev_layer_instructions.new_total_parents,
            parent_chunk_width,
            prev_layer_instructions.need_old_last_parent
        );

    if (parent_is_c1)
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
    else
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

    return grow_layer_instructions;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp_pp
