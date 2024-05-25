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
// Hash the first chunk of the children now being added to a layer
template<typename C>
static typename C::Point get_first_parent(const C &curve,
    const typename C::Chunk &new_children,
    const std::size_t chunk_width,
    const LastChunkData<C> *last_chunk_ptr,
    const std::size_t offset)
{
    // If no last chunk exists, we can get a new parent
    if (last_chunk_ptr == nullptr)
        return get_new_parent<C>(curve, new_children);

    typename C::Scalar prior_child_after_offset;
    if (last_chunk_ptr->update_last_parent)
    {
        // If the last parent has an updated child in it, then we need to get the delta to the old child
        prior_child_after_offset = last_chunk_ptr->last_child;
    }
    else if (offset > 0)
    {
        // If we're not updating the last parent hash and offset is non-zero, then we must be adding new children
        // to the existing last chunk. New children means no prior child after offset exists, use zero scalar
        prior_child_after_offset = curve.zero_scalar();
    }
    else
    {
        // If we're not updating the last parent and the last chunk is already full, we can get a new parent
        return get_new_parent<C>(curve, new_children);
    }

    MDEBUG("Updating existing hash: " << curve.to_string(last_chunk_ptr->last_parent) << " , offset: " << offset
        << ", prior_child_after_offset: " << curve.to_string(prior_child_after_offset));

    return curve.hash_grow(
            last_chunk_ptr->last_parent,
            offset,
            prior_child_after_offset,
            new_children
        );
};
//----------------------------------------------------------------------------------------------------------------------
// After hashing a layer of children points, convert those children x-coordinates into their respective cycle
// scalars, and prepare them to be hashed for the next layer
template<typename C_CHILD, typename C_PARENT>
static std::size_t next_child_scalars_from_children(const C_CHILD &c_child,
    const bool updating_root_layer,
    const LastChunkData<C_CHILD> *last_child_chunk_ptr,
    const LayerExtension<C_CHILD> &children,
    std::vector<typename C_PARENT::Scalar> &child_scalars_out)
{
    child_scalars_out.clear();
    child_scalars_out.reserve(1 + children.hashes.size());

    std::uint64_t next_child_start_index = children.start_idx;

    // If we're creating a *new* root at the existing root layer, we may need to include the *existing* root when
    // hashing the *existing* root layer
    if (updating_root_layer)
    {
        CHECK_AND_ASSERT_THROW_MES(last_child_chunk_ptr != nullptr, "last child chunk does not exist at root");

        // If the children don't already include the existing root, then we need to include it to be hashed
        // - the children would include the existing root already if the existing root was updated in the child
        // layer (the start_idx would be 0)
        if (next_child_start_index > 0)
        {
            MDEBUG("Updating root layer and including the existing root in next children");
            child_scalars_out.emplace_back(c_child.point_to_cycle_scalar(last_child_chunk_ptr->last_parent));
            --next_child_start_index;
        }
    }

    // Convert child points to scalars
    tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child, children.hashes, child_scalars_out);

    return next_child_start_index;
};
//----------------------------------------------------------------------------------------------------------------------
// Hash chunks of a layer of new children, outputting the next layer's parents
template<typename C>
static void hash_layer(const C &curve,
    const LastChunkData<C> *last_chunk_ptr,
    const std::vector<typename C::Scalar> &child_scalars,
    const std::size_t child_start_idx,
    const std::size_t chunk_width,
    LayerExtension<C> &parents_out)
{
    parents_out.start_idx = (last_chunk_ptr == nullptr) ? 0 : last_chunk_ptr->next_start_child_chunk_index;
    parents_out.hashes.clear();

    CHECK_AND_ASSERT_THROW_MES(!child_scalars.empty(), "empty child scalars");

    const std::size_t offset = child_start_idx % chunk_width;

    // See how many children we need to fill up the existing last chunk
    std::size_t chunk_size = std::min(child_scalars.size(), chunk_width - offset);

    MDEBUG("Starting chunk_size: " << chunk_size << " , num child scalars: " << child_scalars.size()
        << " , offset: " << offset);

    // Hash chunks of child scalars to create the parent hashes
    std::size_t chunk_start_idx = 0;
    while (chunk_start_idx < child_scalars.size())
    {
        const auto chunk_start = child_scalars.data() + chunk_start_idx;
        const typename C::Chunk chunk{chunk_start, chunk_size};

        for (std::size_t i = 0; i < chunk_size; ++i)
            MDEBUG("Hashing child " << curve.to_string(chunk_start[i]));

        // Hash the chunk of children
        typename C::Point chunk_hash = chunk_start_idx == 0
            ? get_first_parent<C>(curve,
                chunk,
                chunk_width,
                last_chunk_ptr,
                offset)
            : get_new_parent<C>(curve, chunk);

        MDEBUG("Child chunk_start_idx " << chunk_start_idx << " result: " << curve.to_string(chunk_hash)
            << " , chunk_size: " << chunk_size);

        // We've got our hash
        parents_out.hashes.emplace_back(std::move(chunk_hash));

        // Advance to the next chunk
        chunk_start_idx += chunk_size;

        // Prepare for next loop if there should be one
        if (chunk_start_idx == child_scalars.size())
            break;

        // Fill a complete chunk, or add the remaining new children to the last chunk
        CHECK_AND_ASSERT_THROW_MES(chunk_start_idx < child_scalars.size(), "unexpected chunk start idx");
        chunk_size = std::min(chunk_width, child_scalars.size() - chunk_start_idx);
    }
};
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
typename CurveTrees<C1, C2>::TreeExtension CurveTrees<C1, C2>::get_tree_extension(
    const LastChunks &existing_last_chunks,
    const std::vector<LeafTuple> &new_leaf_tuples) const
{
    TreeExtension tree_extension;

    if (new_leaf_tuples.empty())
        return tree_extension;

    const auto &c1_last_chunks = existing_last_chunks.c1_last_chunks;
    const auto &c2_last_chunks = existing_last_chunks.c2_last_chunks;

    tree_extension.leaves.start_idx = existing_last_chunks.next_start_leaf_index;

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

    auto &c1_layer_extensions_out = tree_extension.c1_layer_extensions;
    auto &c2_layer_extensions_out = tree_extension.c2_layer_extensions;

    const std::vector<typename C2::Scalar> flattened_leaves = this->flatten_leaves(new_leaf_tuples);

    // Hash the leaf layer
    LayerExtension<C2> leaf_parents;
    hash_layer(m_c2,
        c2_last_chunks.empty() ? nullptr : &c2_last_chunks[0],
        flattened_leaves,
        tree_extension.leaves.start_idx,
        m_leaf_layer_chunk_width,
        leaf_parents);

    c2_layer_extensions_out.emplace_back(std::move(leaf_parents));

    // Check if we just added the root
    if (c2_layer_extensions_out.back().hashes.size() == 1 && c2_layer_extensions_out.back().start_idx == 0)
        return tree_extension;

    const std::size_t next_root_layer_idx = c1_last_chunks.size() + c2_last_chunks.size();

    // Alternate between hashing c2 children, c1 children, c2, c1, ...
    bool parent_is_c1 = true;

    std::size_t c1_last_idx = 0;
    std::size_t c2_last_idx = 0;
    // TODO: calculate max number of layers it should take to add all leaves (existing leaves + new leaves)
    while (true)
    {
        const std::size_t updating_layer_idx = 1 + c1_last_idx + c2_last_idx;
        const std::size_t updating_root_layer = updating_layer_idx == next_root_layer_idx;

        const auto *c1_last_chunk_ptr = (c1_last_idx >= c1_last_chunks.size())
            ? nullptr
            : &c1_last_chunks[c1_last_idx];

        const auto *c2_last_chunk_ptr = (c2_last_idx >= c2_last_chunks.size())
            ? nullptr
            : &c2_last_chunks[c2_last_idx];

        // TODO: templated function
        if (parent_is_c1)
        {
            CHECK_AND_ASSERT_THROW_MES(c2_last_idx < c2_layer_extensions_out.size(), "missing c2 layer");

            const auto &c2_child_extension = c2_layer_extensions_out[c2_last_idx];

            std::vector<typename C1::Scalar> c1_child_scalars;
            const std::size_t next_child_start_idx = next_child_scalars_from_children<C2, C1>(m_c2,
                updating_root_layer,
                c2_last_chunk_ptr,
                c2_child_extension,
                c1_child_scalars);

            LayerExtension<C1> c1_layer_extension;
            hash_layer<C1>(m_c1,
                c1_last_chunk_ptr,
                c1_child_scalars,
                next_child_start_idx,
                m_c1_width,
                c1_layer_extension);

            c1_layer_extensions_out.emplace_back(std::move(c1_layer_extension));

            // Check if we just added the root
            if (c1_layer_extensions_out.back().hashes.size() == 1 && c1_layer_extensions_out.back().start_idx == 0)
                return tree_extension;

            ++c2_last_idx;
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(c1_last_idx < c1_layer_extensions_out.size(), "missing c1 layer");

            const auto &c1_child_extension = c1_layer_extensions_out[c1_last_idx];

            std::vector<typename C2::Scalar> c2_child_scalars;
            const std::size_t next_child_start_idx = next_child_scalars_from_children<C1, C2>(m_c1,
                updating_root_layer,
                c1_last_chunk_ptr,
                c1_child_extension,
                c2_child_scalars);

            LayerExtension<C2> c2_layer_extension;
            hash_layer<C2>(m_c2,
                c2_last_chunk_ptr,
                c2_child_scalars,
                next_child_start_idx,
                m_c2_width,
                c2_layer_extension);

            c2_layer_extensions_out.emplace_back(std::move(c2_layer_extension));

            // Check if we just added the root
            if (c2_layer_extensions_out.back().hashes.size() == 1 && c2_layer_extensions_out.back().start_idx == 0)
                return tree_extension;

            ++c1_last_idx;
        }

        parent_is_c1 = !parent_is_c1;
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CurveTrees private member functions
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
//----------------------------------------------------------------------------------------------------------------------
} //namespace curve_trees
} //namespace fcmp
