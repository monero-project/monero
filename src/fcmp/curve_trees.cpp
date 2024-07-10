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


namespace fcmp
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
typename C::Point get_new_parent(const C &curve, const typename C::Chunk &new_children)
{
    return curve.hash_grow(
            curve.m_hash_init_point,
            0,/*offset*/
            curve.zero_scalar(),
            new_children
        );
};
template Helios::Point get_new_parent<Helios>(const Helios &curve, const typename Helios::Chunk &new_children);
template Selene::Point get_new_parent<Selene>(const Selene &curve, const typename Selene::Chunk &new_children);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Static functions
//----------------------------------------------------------------------------------------------------------------------
// After hashing a layer of children points, convert those children x-coordinates into their respective cycle
// scalars, and prepare them to be hashed for the next layer
template<typename C_CHILD, typename C_PARENT>
static std::vector<typename C_PARENT::Scalar> next_child_scalars_from_children(const C_CHILD &c_child,
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
            child_scalars_out.emplace_back(c_child.point_to_cycle_scalar(*last_root));
        }
    }

    // Convert child points to scalars
    tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child, children.hashes, child_scalars_out);

    return child_scalars_out;
};
//----------------------------------------------------------------------------------------------------------------------
// Hash chunks of a layer of new children, outputting the next layer's parents
template<typename C>
static LayerExtension<C> hash_children_chunks(const C &curve,
    const typename C::Scalar *old_last_child,
    const typename C::Point *old_last_parent,
    const std::size_t start_offset,
    const std::size_t next_parent_start_index,
    const std::vector<typename C::Scalar> &new_child_scalars,
    const std::size_t chunk_width)
{
    LayerExtension<C> parents_out;
    parents_out.start_idx                 = next_parent_start_index;
    parents_out.update_existing_last_hash = old_last_parent != nullptr;
    parents_out.hashes.reserve(1 + (new_child_scalars.size() / chunk_width));

    CHECK_AND_ASSERT_THROW_MES(!new_child_scalars.empty(), "empty child scalars");
    CHECK_AND_ASSERT_THROW_MES(chunk_width > start_offset, "start_offset must be smaller than chunk_width");

    // See how many children we need to fill up the existing last chunk
    std::size_t chunk_size = std::min(new_child_scalars.size(), chunk_width - start_offset);

    MDEBUG("First chunk_size: " << chunk_size << " , num new child scalars: " << new_child_scalars.size()
        << " , start_offset: " << start_offset << " , parent layer start idx: " << parents_out.start_idx);

    // Hash the first chunk
    // TODO: separate function
    {
        // Prepare to hash
        const auto &existing_hash = old_last_parent != nullptr
            ? *old_last_parent
            : curve.m_hash_init_point;

        const auto &prior_child_after_offset = old_last_child != nullptr
            ? *old_last_child
            : curve.zero_scalar();

        const auto chunk_start = new_child_scalars.data();
        const typename C::Chunk chunk{chunk_start, chunk_size};

        MDEBUG("existing_hash: " << curve.to_string(existing_hash) << " , start_offset: " << start_offset
            << " , prior_child_after_offset: " << curve.to_string(prior_child_after_offset));

        for (std::size_t i = 0; i < chunk_size; ++i)
            MDEBUG("Hashing child " << curve.to_string(chunk_start[i]));

        // Do the hash
        auto chunk_hash = curve.hash_grow(
                existing_hash,
                start_offset,
                prior_child_after_offset,
                chunk
            );

        MDEBUG("Child chunk_start_idx " << 0 << " result: " << curve.to_string(chunk_hash)
            << " , chunk_size: " << chunk_size);

        // We've got our hash
        parents_out.hashes.emplace_back(std::move(chunk_hash));
    }

    // Hash chunks of child scalars to create the parent hashes
    std::size_t chunk_start_idx = chunk_size;
    while (chunk_start_idx < new_child_scalars.size())
    {
        chunk_size = std::min(chunk_width, new_child_scalars.size() - chunk_start_idx);

        const auto chunk_start = new_child_scalars.data() + chunk_start_idx;
        const typename C::Chunk chunk{chunk_start, chunk_size};

        for (std::size_t i = 0; i < chunk_size; ++i)
            MDEBUG("Hashing child " << curve.to_string(chunk_start[i]));

        auto chunk_hash = get_new_parent(curve, chunk);

        MDEBUG("Child chunk_start_idx " << chunk_start_idx << " result: " << curve.to_string(chunk_hash)
            << " , chunk_size: " << chunk_size);

        // We've got our hash
        parents_out.hashes.emplace_back(std::move(chunk_hash));

        // Advance to the next chunk
        chunk_start_idx += chunk_size;

        // Fill a complete chunk, or add the remaining new children to the last chunk
        CHECK_AND_ASSERT_THROW_MES(chunk_start_idx <= new_child_scalars.size(), "unexpected chunk start idx");
    }

    return parents_out;
};
//----------------------------------------------------------------------------------------------------------------------
static GrowLayerInstructions get_grow_layer_instructions(const std::size_t old_total_children,
    const std::size_t new_total_children,
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
    const std::size_t old_total_parents = old_total_children > 1
        ? (1 + ((old_total_children - 1) / parent_chunk_width))
        : 0;
    const std::size_t new_total_parents = 1 + ((new_total_children - 1) / parent_chunk_width);

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
    std::size_t next_parent_start_index = old_total_parents;
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
static GrowLayerInstructions get_leaf_layer_grow_instructions(const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples,
    const std::size_t leaf_tuple_size,
    const std::size_t leaf_layer_chunk_width)
{
    // TODO: comments

    // The leaf layer can never be the root layer
    const bool setting_next_layer_after_old_root = false;

    const std::size_t old_total_children = old_n_leaf_tuples * leaf_tuple_size;
    const std::size_t new_total_children = (old_n_leaf_tuples + new_n_leaf_tuples) * leaf_tuple_size;

    const std::size_t old_total_parents = old_total_children > 0
        ? (1 + ((old_total_children - 1) / leaf_layer_chunk_width))
        : 0;
    const std::size_t new_total_parents = 1 + ((new_total_children - 1) / leaf_layer_chunk_width);

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

    std::size_t next_parent_start_index = old_total_parents;
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
static LayerExtension<C_PARENT> get_next_layer_extension(const C_CHILD &c_child,
    const C_PARENT &c_parent,
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
        last_child_scalar = c_child.point_to_cycle_scalar(*child_last_hash);
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
    const std::size_t old_total_children,
    const std::size_t new_total_children,
    const std::size_t parent_chunk_width,
    const bool last_child_will_change)
{
    CHECK_AND_ASSERT_THROW_MES(new_total_children > 0, "new total children must be > 0");
    CHECK_AND_ASSERT_THROW_MES(old_total_children >= new_total_children,
        "old_total_children must be >= new_total_children");

    // Calculate old and new total number of parents using totals for children
    const std::size_t old_total_parents = 1 + ((old_total_children - 1) / parent_chunk_width);
    const std::size_t new_total_parents = 1 + ((new_total_children - 1) / parent_chunk_width);

    CHECK_AND_ASSERT_THROW_MES(old_total_parents >= new_total_parents,
        "old_total_parents must be >= new_total_parents");
    CHECK_AND_ASSERT_THROW_MES(new_total_children > new_total_parents,
        "new_total_children must be > new_total_parents");

    const std::size_t old_offset = old_total_children % parent_chunk_width;
    std::size_t new_offset = new_total_children % parent_chunk_width;

    // Get the number of existing children in what will become the new last chunk after trimming
    const std::size_t new_last_chunk_old_num_children = (old_total_parents > new_total_parents || old_offset == 0)
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

    // TODO: cleaner conditional approach
    // TODO: comments
    const bool need_last_chunk_parent = !need_last_chunk_remaining_children &&
        (need_last_chunk_children_to_trim || last_child_will_change);

    const bool update_existing_last_hash = need_last_chunk_remaining_children || need_last_chunk_parent;

    std::size_t hash_offset = new_offset;
    if (last_child_will_change)
    {
        hash_offset = hash_offset == 0 ? (parent_chunk_width - 1) : (hash_offset - 1);

        if (need_last_chunk_children_to_trim || need_last_chunk_remaining_children)
            --new_offset;
    }

    if (need_last_chunk_remaining_children)
    {
        hash_offset = 0;
    }

    std::size_t start_trim_idx = 0;
    std::size_t end_trim_idx = 0;
    if (need_last_chunk_children_to_trim)
    {
        const std::size_t chunk_boundary_start = (new_total_parents - 1) * parent_chunk_width;
        const std::size_t chunk_boundary_end   = chunk_boundary_start + parent_chunk_width;

        start_trim_idx = chunk_boundary_start + new_offset;
        end_trim_idx   = std::min(chunk_boundary_end, old_total_children);
    }
    else if (need_last_chunk_remaining_children && new_offset > 0)
    {
        start_trim_idx = new_total_children - new_offset;
        end_trim_idx   = new_total_children;
    }

    MDEBUG("parent_chunk_width: "                    << parent_chunk_width
        << " , old_total_children: "                 << old_total_children
        << " , new_total_children: "                 << new_total_children
        << " , old_total_parents: "                  << old_total_parents
        << " , new_total_parents: "                  << new_total_parents
        << " , need_last_chunk_children_to_trim: "   << need_last_chunk_children_to_trim
        << " , need_last_chunk_remaining_children: " << need_last_chunk_remaining_children
        << " , need_last_chunk_parent: "             << need_last_chunk_parent
        << " , need_new_last_child: "                << last_child_will_change
        << " , update_existing_last_hash: "          << update_existing_last_hash
        << " , new_offset: "                         << new_offset
        << " , hash_offset: "                        << hash_offset
        << " , start_trim_idx: "                     << start_trim_idx
        << " , end_trim_idx: "                       << end_trim_idx);

    return TrimLayerInstructions{
            .parent_chunk_width                 = parent_chunk_width,
            .old_total_children                 = old_total_children,
            .new_total_children                 = new_total_children,
            .old_total_parents                  = old_total_parents,
            .new_total_parents                  = new_total_parents,
            .need_last_chunk_children_to_trim   = need_last_chunk_children_to_trim,
            .need_last_chunk_remaining_children = need_last_chunk_remaining_children,
            .need_last_chunk_parent             = need_last_chunk_parent,
            .need_new_last_child                = last_child_will_change,
            .update_existing_last_hash          = update_existing_last_hash,
            .new_offset                         = new_offset,
            .hash_offset                        = hash_offset,
            .start_trim_idx                     = start_trim_idx,
            .end_trim_idx                       = end_trim_idx,
        };
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C_CHILD, typename C_PARENT>
static typename fcmp::curve_trees::LayerReduction<C_PARENT> get_next_layer_reduction(
    const C_CHILD &c_child,
    const C_PARENT &c_parent,
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

    typename C_PARENT::Point existing_hash = c_parent.m_hash_init_point;
    if (trim_layer_instructions.need_last_chunk_parent)
    {
        CHECK_AND_ASSERT_THROW_MES(parent_last_hashes.size() > parent_layer_idx, "missing last parent hash");
        existing_hash = parent_last_hashes[parent_layer_idx];
    }

    std::vector<typename C_PARENT::Scalar> child_scalars;
    if (trim_layer_instructions.need_last_chunk_children_to_trim
        || trim_layer_instructions.need_last_chunk_remaining_children)
    {
        // TODO: a clean way to do this without copying
        CHECK_AND_ASSERT_THROW_MES(children_to_trim.size() > parent_layer_idx, "missing children to trim");
        child_scalars = children_to_trim[parent_layer_idx];
    }

    typename C_PARENT::Scalar new_last_child_scalar = c_parent.zero_scalar();
    if (trim_layer_instructions.need_new_last_child)
    {
        CHECK_AND_ASSERT_THROW_MES(child_layer_idx > 0, "child index cannot be 0 here");
        CHECK_AND_ASSERT_THROW_MES(child_reductions.size() == child_layer_idx, "unexpected child layer idx");
        CHECK_AND_ASSERT_THROW_MES(child_reductions.back().update_existing_last_hash, "expected new last child");

        const typename C_CHILD::Point &new_last_child = child_reductions.back().new_last_hash;
        new_last_child_scalar = c_child.point_to_cycle_scalar(new_last_child);

        if (trim_layer_instructions.need_last_chunk_remaining_children)
        {
            child_scalars.emplace_back(std::move(new_last_child_scalar));
        }
        else if (!trim_layer_instructions.need_last_chunk_children_to_trim)
        {
            // TODO: cleaner conditional for this case
            const std::size_t last_child_layer_idx = child_layer_idx - 1;
            CHECK_AND_ASSERT_THROW_MES(child_last_hashes.size() > last_child_layer_idx, "missing last child hash");

            const typename C_CHILD::Point &old_last_child = child_last_hashes[last_child_layer_idx];
            auto old_last_child_scalar = c_child.point_to_cycle_scalar(old_last_child);

            child_scalars.emplace_back(std::move(old_last_child_scalar));
        }
    }

    for (std::size_t i = 0; i < child_scalars.size(); ++i)
        MDEBUG("Hashing child " << c_parent.to_string(child_scalars[i]));

    if (trim_layer_instructions.need_last_chunk_remaining_children)
    {
        MDEBUG("hash_grow: existing_hash: " << c_parent.to_string(existing_hash)
            << " , hash_offset: " << trim_layer_instructions.hash_offset);

        layer_reduction_out.new_last_hash = c_parent.hash_grow(
            existing_hash,
            trim_layer_instructions.hash_offset,
            c_parent.zero_scalar(),
            typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()});
    }
    else
    {
        MDEBUG("hash_trim: existing_hash: " << c_parent.to_string(existing_hash)
            << " , hash_offset: "           << trim_layer_instructions.hash_offset
            << " , child_to_grow_back: "    << c_parent.to_string(new_last_child_scalar));

        layer_reduction_out.new_last_hash = c_parent.hash_trim(
            existing_hash,
            trim_layer_instructions.hash_offset,
            typename C_PARENT::Chunk{child_scalars.data(), child_scalars.size()},
            new_last_child_scalar);
    }

    MDEBUG("Result hash: " << c_parent.to_string(layer_reduction_out.new_last_hash));

    return layer_reduction_out;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees public member functions
//----------------------------------------------------------------------------------------------------------------------
template<>
CurveTrees<Helios, Selene>::LeafTuple CurveTrees<Helios, Selene>::output_to_leaf_tuple(
    const crypto::public_key &O,
    const crypto::public_key &C) const
{
    crypto::ec_point I;
    crypto::derive_key_image_generator(O, I);

    return LeafTuple{
        .O_x = tower_cycle::ed_25519_point_to_scalar(O),
        .I_x = tower_cycle::ed_25519_point_to_scalar(I),
        .C_x = tower_cycle::ed_25519_point_to_scalar(C)
    };
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<typename C2::Scalar> CurveTrees<C1, C2>::flatten_leaves(const std::vector<LeafTuple> &leaves) const
{
    std::vector<typename C2::Scalar> flattened_leaves;
    flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

    for (const auto &l : leaves)
    {
        // TODO: implement without cloning
        flattened_leaves.emplace_back(l.O_x);
        flattened_leaves.emplace_back(l.I_x);
        flattened_leaves.emplace_back(l.C_x);
    }

    return flattened_leaves;
};
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeExtension CurveTrees<C1, C2>::get_tree_extension(
    const std::size_t old_n_leaf_tuples,
    const LastHashes &existing_last_hashes,
    const std::vector<LeafTuple> &new_leaf_tuples) const
{
    TreeExtension tree_extension;

    if (new_leaf_tuples.empty())
        return tree_extension;

    auto grow_layer_instructions = get_leaf_layer_grow_instructions(
        old_n_leaf_tuples,
        new_leaf_tuples.size(),
        LEAF_TUPLE_SIZE,
        m_leaf_layer_chunk_width);

    tree_extension.leaves.start_leaf_tuple_idx = grow_layer_instructions.old_total_children / LEAF_TUPLE_SIZE;

    // Copy the leaves
    // TODO: don't copy here
    tree_extension.leaves.tuples.reserve(new_leaf_tuples.size());
    for (const auto &leaf : new_leaf_tuples)
    {
        tree_extension.leaves.tuples.emplace_back(LeafTuple{
            .O_x = leaf.O_x,
            .I_x = leaf.I_x,
            .C_x = leaf.C_x
        });
    }

    if (grow_layer_instructions.need_old_last_parent)
        CHECK_AND_ASSERT_THROW_MES(!existing_last_hashes.c2_last_hashes.empty(), "missing last c2 parent");

    // Hash the leaf layer
    auto leaf_parents = hash_children_chunks(m_c2,
            nullptr, // We never need the old last child from leaf layer because the leaf layer is always append-only
            grow_layer_instructions.need_old_last_parent ? &existing_last_hashes.c2_last_hashes[0] : nullptr,
            grow_layer_instructions.start_offset,
            grow_layer_instructions.next_parent_start_index,
            this->flatten_leaves(new_leaf_tuples),
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

        const std::size_t new_total_children = grow_layer_instructions.new_total_parents;

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
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
std::vector<TrimLayerInstructions> CurveTrees<C1, C2>::get_trim_instructions(
    const std::size_t old_n_leaf_tuples,
    const std::size_t trim_n_leaf_tuples) const
{
    CHECK_AND_ASSERT_THROW_MES(old_n_leaf_tuples > trim_n_leaf_tuples, "cannot trim more leaves than exist");
    CHECK_AND_ASSERT_THROW_MES(trim_n_leaf_tuples > 0, "must be trimming some leaves");

    std::vector<TrimLayerInstructions> trim_instructions;

    // Get trim instructions for the leaf layer
    {
        const std::size_t old_total_leaves = old_n_leaf_tuples * LEAF_TUPLE_SIZE;
        const std::size_t new_total_leaves = (old_n_leaf_tuples - trim_n_leaf_tuples) * LEAF_TUPLE_SIZE;

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
//----------------------------------------------------------------------------------------------------------------------
template<typename C1, typename C2>
typename CurveTrees<C1, C2>::TreeReduction CurveTrees<C1, C2>::get_tree_reduction(
    const std::vector<TrimLayerInstructions> &trim_instructions,
    const LastChunkChildrenToTrim &children_to_trim,
    const LastHashes &last_hashes) const
{
    TreeReduction tree_reduction_out;

    CHECK_AND_ASSERT_THROW_MES((trim_instructions[0].new_total_children % LEAF_TUPLE_SIZE) == 0,
        "unexpected new total leaves");
    const std::size_t new_total_leaf_tuples = trim_instructions[0].new_total_children / LEAF_TUPLE_SIZE;
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
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees private member functions
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
} //namespace fcmp
