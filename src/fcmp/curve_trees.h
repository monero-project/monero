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

#pragma once

#include "crypto/crypto.h"
#include "misc_log_ex.h"
#include "tower_cycle.h"

#include <vector>

// forward declarations
class CurveTreesUnitTest;

namespace fcmp
{

// TODO: longer descriptions
template<typename C1, typename C2>
class CurveTrees
{
    friend class ::CurveTreesUnitTest;
public:
    CurveTrees(const C1 &c1, const C2 &c2, const std::size_t c1_width, const std::size_t c2_width):
        m_c1{c1},
        m_c2{c2},
        m_c1_width{c1_width},
        m_c2_width{c2_width},
        m_leaf_layer_chunk_width{LEAF_TUPLE_SIZE * c2_width}
    {
        assert(c1_width > 0);
        assert(c2_width > 0);
    };

//member structs
public:
    // Tuple that composes a single leaf in the tree
    struct LeafTuple final
    {
        // Output ed25519 point x-coordinate
        typename C2::Scalar O_x;
        // Key image generator x-coordinate
        typename C2::Scalar I_x;
        // Commitment x-coordinate
        typename C2::Scalar C_x;
    };
    static const std::size_t LEAF_TUPLE_SIZE = 3;
    static_assert(sizeof(LeafTuple) == (sizeof(typename C2::Scalar) * LEAF_TUPLE_SIZE), "unexpected LeafTuple size");

    // Leaves in the tree
    struct Leaves final
    {
        // Starting index in the leaf layer
        std::size_t            start_idx;
        // Contiguous leaves in a tree that start at the start_idx
        std::vector<LeafTuple> tuples;
    };

    // A layer of contiguous hashes starting from a specific start_idx in the tree
    template<typename C>
    struct LayerExtension final
    {
        std::size_t                    start_idx;
        std::vector<typename C::Point> hashes;
    };

    // A struct useful to extend an existing tree
    // - layers alternate between C1 and C2
    // - c2_layer_extensions[0] is first layer after leaves, then c1_layer_extensions[0], c2_layer_extensions[1], etc
    struct TreeExtension final
    {
        Leaves                          leaves;
        std::vector<LayerExtension<C1>> c1_layer_extensions;
        std::vector<LayerExtension<C2>> c2_layer_extensions;
    };

    // Useful data from the last chunk in a layer
    template<typename C>
    struct LastChunkData final
    {
        // The total number of children % child layer chunk width
        /*TODO: const*/ std::size_t                     child_offset;
        // The last child in the chunk (and therefore the last child in the child layer)
        /*TODO: const*/ typename C::Scalar              last_child;
        // The hash of the last chunk of child scalars
        /*TODO: const*/ typename C::Point               last_parent;
        // Total number of children in the child layer
        /*TODO: const*/ std::size_t                     child_layer_size;
        // Total number of hashes in the parent layer
        /*TODO: const*/ std::size_t                     parent_layer_size;
    };

    // Last chunk data from each layer in the tree
    // - layers alternate between C1 and C2
    // - c2_last_chunks[0] is first layer after leaves, then c1_last_chunks[0], then c2_last_chunks[1], etc
    struct LastChunks final
    {
        std::vector<LastChunkData<C1>> c1_last_chunks;
        std::vector<LastChunkData<C2>> c2_last_chunks;
    };

//member functions
public:
    // TODO: move impl into cpp
    LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C) const
    {
        crypto::ec_point I;
        crypto::derive_key_image_generator(O, I);

        return LeafTuple{
            .O_x = fcmp::tower_cycle::ed_25519_point_to_scalar(O),
            .I_x = fcmp::tower_cycle::ed_25519_point_to_scalar(I),
            .C_x = fcmp::tower_cycle::ed_25519_point_to_scalar(C)
        };
    };

    // TODO: move impl into cpp
    std::vector<typename C2::Scalar> flatten_leaves(const std::vector<LeafTuple> &leaves) const
    {
        std::vector<typename C2::Scalar> flattened_leaves;
        flattened_leaves.reserve(leaves.size() * LEAF_TUPLE_SIZE);

        for (const auto &l : leaves)
        {
            // TODO: implement without cloning
            flattened_leaves.emplace_back(m_c2.clone(l.O_x));
            flattened_leaves.emplace_back(m_c2.clone(l.I_x));
            flattened_leaves.emplace_back(m_c2.clone(l.C_x));
        }

        return flattened_leaves;
    };

    // TODO: move impl into cpp
    TreeExtension get_tree_extension(const LastChunks &existing_last_chunks, const Leaves &new_leaves)
    {
        TreeExtension tree_extension;

        if (new_leaves.tuples.empty())
            return tree_extension;

        const auto &c1_last_chunks = existing_last_chunks.c1_last_chunks;
        const auto &c2_last_chunks = existing_last_chunks.c2_last_chunks;

        // Set the leaf start idx
        tree_extension.leaves.start_idx = c2_last_chunks.empty()
            ? 0
            : c2_last_chunks[0].child_layer_size;

        // Copy the leaves
        // TODO: don't copy here
        tree_extension.leaves.tuples.reserve(new_leaves.tuples.size());
        for (const auto &leaf : new_leaves.tuples)
        {
            tree_extension.leaves.tuples.emplace_back(LeafTuple{
                .O_x = m_c2.clone(leaf.O_x),
                .I_x = m_c2.clone(leaf.I_x),
                .C_x = m_c2.clone(leaf.C_x)
            });
        }

        auto &c1_layer_extensions_out = tree_extension.c1_layer_extensions;
        auto &c2_layer_extensions_out = tree_extension.c2_layer_extensions;

        // Hash the leaf layer
        LayerExtension<C2> parents;
        this->hash_leaf_layer(c2_last_chunks.empty() ? nullptr : &c2_last_chunks[0],
            new_leaves,
            parents);

        c2_layer_extensions_out.emplace_back(std::move(parents));

        // Check if we just added the root
        if (c2_layer_extensions_out.back().hashes.size() == 1 && c2_layer_extensions_out.back().start_idx == 0)
            return tree_extension;

        // Alternate between hashing c2 children, c1 children, c2, c1, ...
        bool parent_is_c1 = true;

        std::size_t c1_last_idx = 0;
        std::size_t c2_last_idx = 0;
        // TODO: calculate max number of layers it should take to add all leaves (existing leaves + new leaves)
        while (true)
        {
            if (parent_is_c1)
            {
                CHECK_AND_ASSERT_THROW_MES(c2_layer_extensions_out.size() > c2_last_idx, "missing c2 layer");

                LayerExtension<C1> c1_layer_extension;
                this->hash_layer<C2, C1>(m_c2,
                    m_c1,
                    (c2_last_chunks.size() <= c2_last_idx) ? nullptr : &c2_last_chunks[c2_last_idx],
                    (c1_last_chunks.size() <= c1_last_idx) ? nullptr : &c1_last_chunks[c1_last_idx],
                    c2_layer_extensions_out[c2_last_idx],
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
                CHECK_AND_ASSERT_THROW_MES(c1_layer_extensions_out.size() > c1_last_idx, "missing c1 layer");

                LayerExtension<C2> c2_layer_extension;
                this->hash_layer<C1, C2>(m_c1,
                    m_c2,
                    (c1_last_chunks.size() <= c1_last_idx) ? nullptr : &c1_last_chunks[c1_last_idx],
                    (c2_last_chunks.size() <= c2_last_idx) ? nullptr : &c2_last_chunks[c2_last_idx],
                    c1_layer_extensions_out[c1_last_idx],
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
    }

private:
    // TODO: move impl into cpp
    template <typename C>
    typename C::Point get_new_parent(const C &curve,
        const typename C::Chunk &new_children) const
    {
        // New parent means no prior children, fill priors with 0
        std::vector<typename C::Scalar> prior_children;
        fcmp::tower_cycle::extend_zeroes(curve, new_children.size(), prior_children);

        return curve.hash_grow(
                curve.m_hash_init_point,
                0,/*offset*/
                typename C::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    // TODO: move impl into cpp
    typename C2::Point get_first_leaf_parent(const typename C2::Chunk &new_children,
        const LastChunkData<C2> *last_chunk_ptr) const
    {
        // If no last chunk exists, or if the last chunk is already full, then we can get a new parent
        if (last_chunk_ptr == nullptr || last_chunk_ptr->child_offset == 0)
            return get_new_parent<C2>(m_c2, new_children);

        // There won't be any existing children when growing the leaf layer, fill priors with 0
        std::vector<typename C2::Scalar> prior_children;
        fcmp::tower_cycle::extend_zeroes(m_c2, new_children.size(), prior_children);

        return m_c2.hash_grow(
                last_chunk_ptr->last_parent,
                last_chunk_ptr->child_offset,
                typename C2::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    // TODO: move impl into cpp
    template <typename C>
    typename C::Point get_first_non_leaf_parent(const C &curve,
        const typename C::Chunk &new_children,
        const std::size_t chunk_width,
        const bool child_layer_last_hash_updated,
        const LastChunkData<C> *last_chunk_ptr) const
    {
        // If no last chunk exists, we can get a new parent
        if (last_chunk_ptr == nullptr)
            return get_new_parent<C>(curve, new_children);

        std::vector<typename C::Scalar> prior_children;
        std::size_t offset = last_chunk_ptr->child_offset;

        if (child_layer_last_hash_updated)
        {
            // If the last chunk has updated children in it, then we need to get the delta to the old children, and
            // subtract the offset by 1 since we're updating the prior last hash
            prior_children.emplace_back(curve.clone(last_chunk_ptr->last_child));
            offset = offset > 0 ? (offset - 1) : (chunk_width - 1);

            // Extend prior children by zeroes for any additional new children, since they must be new
            if (new_children.size() > 1)
                fcmp::tower_cycle::extend_zeroes(curve, new_children.size() - 1, prior_children);
        }
        else if (offset > 0)
        {
            // If we're updating the parent hash and no children were updated, then we're just adding new children
            // to the existing last chunk and can fill priors with 0
            fcmp::tower_cycle::extend_zeroes(curve, new_children.size(), prior_children);
        }
        else
        {
            // If the last chunk is already full and isn't updated in any way, then we just get a new parent
            return get_new_parent<C>(curve, new_children);
        }

        return curve.hash_grow(
                last_chunk_ptr->last_parent,
                offset,
                typename C::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    // TODO: look into consolidating hash_layer and hash_leaf_layer into 1 function
    // TODO: move impl into cpp
    template<typename C_CHILD, typename C_PARENT>
    void hash_layer(const C_CHILD &c_child,
        const C_PARENT &c_parent,
        const LastChunkData<C_CHILD> *last_child_chunk_ptr,
        const LastChunkData<C_PARENT> *last_parent_chunk_ptr,
        const LayerExtension<C_CHILD> &children,
        const std::size_t chunk_width,
        LayerExtension<C_PARENT> &parents_out)
    {
        parents_out.start_idx = (last_parent_chunk_ptr == nullptr) ? 0 : last_parent_chunk_ptr->parent_layer_size;
        parents_out.hashes.clear();

        CHECK_AND_ASSERT_THROW_MES(!children.hashes.empty(), "empty children hashes");

        std::size_t offset = (last_parent_chunk_ptr == nullptr) ? 0 : last_parent_chunk_ptr->child_offset;

        // TODO: try to simplify the approach to avoid edge cases
        // If we're adding new children to an existing last chunk, then we need to pull the parent start idx back 1
        // since we'll be updating the existing parent hash of the last chunk
        if (offset > 0)
        {
            CHECK_AND_ASSERT_THROW_MES(parents_out.start_idx > 0, "parent start idx should be > 0");
            --parents_out.start_idx;
        }

        // If the child layer had its existing last hash updated, then we'll need to use the last hash's prior
        // version in order to update the existing last parent hash in this layer
        bool child_layer_last_hash_updated = (last_parent_chunk_ptr == nullptr)
            ? false
            : last_parent_chunk_ptr->child_layer_size == (children.start_idx + 1);

        if (offset == 0 && child_layer_last_hash_updated)
        {
            CHECK_AND_ASSERT_THROW_MES(parents_out.start_idx > 0, "parent start idx should be > 0");
            --parents_out.start_idx;
        }

        // TODO: clean this up so I don't have to do it twice here and in get_first_non_leaf_parent
        // The offset needs to be brought back because we're going to start with the prior hash, and so the chunk
        // will start from there and may need 1 more to fill
        CHECK_AND_ASSERT_THROW_MES(chunk_width > offset, "unexpected offset");
        if (child_layer_last_hash_updated)
            offset = offset > 0 ? (offset - 1) : (chunk_width - 1);

        // If we're creating a *new* root at the existing root layer, we may need to include the *existing* root when
        // hashing the *existing* root layer
        std::vector<typename C_PARENT::Scalar> child_scalars;
        if (last_child_chunk_ptr != nullptr && last_child_chunk_ptr->parent_layer_size == 1)
        {
            // We should be updating the existing root, there shouldn't be a last parent chunk
            CHECK_AND_ASSERT_THROW_MES(last_parent_chunk_ptr == nullptr, "last parent chunk exists at root");

            // If the children don't already include the existing root at start_idx 0 (they would if the existing
            // root was updated in the child layer), then we need to add it to the first chunk to be hashed
            if (children.start_idx > 0)
                child_scalars.emplace_back(c_child.point_to_cycle_scalar(last_child_chunk_ptr->last_parent));
        }

        // Convert child points to scalars
        tower_cycle::extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child, children.hashes, child_scalars);

        // See how many children we need to fill up the existing last chunk
        std::size_t chunk_size = std::min(child_scalars.size(), chunk_width - offset);
        MDEBUG("Starting chunk_size: " << chunk_size << " , num child scalars: " << child_scalars.size()
            << " , offset: " << offset);

        // Hash chunks of child scalars to create the parent hashes
        std::size_t chunk_start_idx = 0;
        while (chunk_start_idx < child_scalars.size())
        {
            const auto chunk_start = child_scalars.data() + chunk_start_idx;
            const typename C_PARENT::Chunk chunk{chunk_start, chunk_size};

            for (const auto &c : chunk)
                MDEBUG("Hashing " << c_parent.to_string(c));

            // Hash the chunk of children
            typename C_PARENT::Point chunk_hash = chunk_start_idx == 0
                ? get_first_non_leaf_parent<C_PARENT>(c_parent, chunk, chunk_width, child_layer_last_hash_updated, last_parent_chunk_ptr)
                : get_new_parent<C_PARENT>(c_parent, chunk);

            MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " result: " << c_parent.to_string(chunk_hash)
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
    }

    // TODO: move impl into cpp
    void hash_leaf_layer(const LastChunkData<C2> *last_chunk_ptr,
        const Leaves &leaves,
        LayerExtension<C2> &parents_out)
    {
        parents_out.start_idx = (last_chunk_ptr == nullptr) ? 0 : last_chunk_ptr->parent_layer_size;
        parents_out.hashes.clear();

        if (leaves.tuples.empty())
            return;

        // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [scalar, scalar, scalar, scalar, scalar, scalar,...]
        const std::vector<typename C2::Scalar> children = flatten_leaves(leaves.tuples);

        const std::size_t max_chunk_size = m_leaf_layer_chunk_width;
        const std::size_t offset         = (last_chunk_ptr == nullptr) ? 0 : last_chunk_ptr->child_offset;

        // If we're adding new children to an existing last chunk, then we need to pull the parent start idx back 1
        // since we'll be updating the existing parent hash of the last chunk
        if (offset > 0)
        {
            CHECK_AND_ASSERT_THROW_MES(parents_out.start_idx > 0, "parent start idx should be > 0");
            --parents_out.start_idx;
        }

        // See how many new children are needed to fill up the existing last chunk
        CHECK_AND_ASSERT_THROW_MES(max_chunk_size > offset, "unexpected offset");
        std::size_t chunk_size = std::min(children.size(), max_chunk_size - offset);

        std::size_t chunk_start_idx = 0;
        while (chunk_start_idx < children.size())
        {
            const auto chunk_start = children.data() + chunk_start_idx;
            const typename C2::Chunk chunk{chunk_start, chunk_size};

            for (const auto &c : chunk)
                MDEBUG("Hashing " << m_c2.to_string(c));

            // Hash the chunk of children
            typename C2::Point chunk_hash = chunk_start_idx == 0
                ? get_first_leaf_parent(chunk, last_chunk_ptr)
                : get_new_parent<C2>(m_c2, chunk);

            MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " result: " << m_c2.to_string(chunk_hash)
                << " , chunk_size: " << chunk_size);

            // We've got our hash
            parents_out.hashes.emplace_back(std::move(chunk_hash));

            // Advance to the next chunk
            chunk_start_idx += chunk_size;

            // Prepare for next loop if there should be one
            if (chunk_start_idx == children.size())
                break;

            // Fill a complete chunk, or add the remaining new children to the last chunk
            CHECK_AND_ASSERT_THROW_MES(chunk_start_idx < children.size(), "unexpected chunk start idx");
            chunk_size = std::min(max_chunk_size, children.size() - chunk_start_idx);
        }
    }

//member variables
private:
    const C1 &m_c1;
    const C2 &m_c2;

    const std::size_t m_c1_width;
    const std::size_t m_c2_width;

    const std::size_t m_leaf_layer_chunk_width;
};

} //namespace fcmp
