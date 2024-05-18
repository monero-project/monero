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
#include "fcmp_rust/cxx.h"
#include "fcmp_rust/fcmp_rust.h"
#include "misc_log_ex.h"
#include "string_tools.h"

#include <boost/variant.hpp>

#include <string>

namespace fcmp
{
    using RustEd25519Point = std::array<uint8_t, 32UL>;

    // Need to forward declare Scalar types for point_to_cycle_scalar below
    using SeleneScalar = rust::Box<fcmp_rust::SeleneScalar>;
    using HeliosScalar = rust::Box<fcmp_rust::HeliosScalar>;

    static struct Helios final
    {
        using Generators = rust::Box<fcmp_rust::HeliosGenerators>;
        using Scalar     = HeliosScalar;
        using Point      = rust::Box<fcmp_rust::HeliosPoint>;
        using Chunk      = rust::Slice<const Scalar>;

        // TODO: static constants
        const Generators GENERATORS = fcmp_rust::random_helios_generators();
        const Point HASH_INIT_POINT = fcmp_rust::random_helios_hash_init_point();

        // TODO: use correct value
        static const std::size_t WIDTH = 5;

        Point hash_grow(
            const Generators &generators,
            const Point &existing_hash,
            const std::size_t offset,
            const Chunk &prior_children,
            const Chunk &new_children) const
        {
            return fcmp_rust::hash_grow_helios(
                generators,
                existing_hash,
                offset,
                prior_children,
                new_children);
        }

        SeleneScalar point_to_cycle_scalar(const Point &point) const;

        Scalar clone(const Scalar &scalar) const { return fcmp_rust::clone_helios_scalar(scalar); }
        Point clone(const Point &point) const { return fcmp_rust::clone_helios_point(point); }

        Scalar zero_scalar() const { return fcmp_rust::helios_zero_scalar(); }

        std::array<uint8_t, 32UL> to_bytes(const Scalar &scalar) const
            { return fcmp_rust::helios_scalar_to_bytes(scalar); }
        std::array<uint8_t, 32UL> to_bytes(const Point &point) const
            { return fcmp_rust::helios_point_to_bytes(point); }

        std::string to_string(const Scalar &scalar) const
            { return epee::string_tools::pod_to_hex(to_bytes(scalar)); }
        std::string to_string(const Point &point) const
            { return epee::string_tools::pod_to_hex(to_bytes(point)); }
    } HELIOS;

    static struct Selene final
    {
        using Generators = rust::Box<fcmp_rust::SeleneGenerators>;
        using Scalar     = SeleneScalar;
        using Point      = rust::Box<fcmp_rust::SelenePoint>;
        using Chunk      = rust::Slice<const Scalar>;

        // TODO: static constants
        const Generators GENERATORS = fcmp_rust::random_selene_generators();
        const Point HASH_INIT_POINT = fcmp_rust::random_selene_hash_init_point();

        // TODO: use correct value
        static const std::size_t WIDTH = 5;

        Point hash_grow(
            const Generators &generators,
            const Point &existing_hash,
            const std::size_t offset,
            const Chunk &prior_children,
            const Chunk &new_children) const
        {
            return fcmp_rust::hash_grow_selene(
                generators,
                existing_hash,
                offset,
                prior_children,
                new_children);
        };

        HeliosScalar point_to_cycle_scalar(const Point &point) const;

        Scalar clone(const Scalar &scalar) const { return fcmp_rust::clone_selene_scalar(scalar); }
        Point clone(const Point &point) const { return fcmp_rust::clone_selene_point(point); }

        Scalar zero_scalar() const { return fcmp_rust::selene_zero_scalar(); }

        std::array<uint8_t, 32UL> to_bytes(const Scalar &scalar) const
            { return fcmp_rust::selene_scalar_to_bytes(scalar); }
        std::array<uint8_t, 32UL> to_bytes(const Point &point) const
            { return fcmp_rust::selene_point_to_bytes(point); }

        std::string to_string(const Scalar &scalar) const
            { return epee::string_tools::pod_to_hex(to_bytes(scalar)); }
        std::string to_string(const Point &point) const
            { return epee::string_tools::pod_to_hex(to_bytes(point)); }
    } SELENE;

    // TODO: cleanly separate everything below into another file. This current file should strictly be for the rust interface

    // TODO: template all the curve things

    // TODO: Curve class
    // TODO: CurveTree class instantiated with the curves and widths

    // TODO: template
    struct LeafTuple final
    {
        Selene::Scalar O_x;
        Selene::Scalar I_x;
        Selene::Scalar C_x;
    };
    static const std::size_t LEAF_TUPLE_SIZE       = 3;
    static const std::size_t LEAF_LAYER_CHUNK_SIZE = LEAF_TUPLE_SIZE * SELENE.WIDTH;

    // Tree structure
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

    // A struct useful to extend an existing tree, layers alternate between C1 and C2
    template<typename C1, typename C2>
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
        // The total number of children % child layer chunk size
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

    template<typename C1, typename C2>
    struct LastChunks final
    {
        std::vector<LastChunkData<C1>> c1_last_chunks;
        std::vector<LastChunkData<C2>> c2_last_chunks;
    };

    template<typename C>
    using Layer = std::vector<typename C::Point>;

    // A complete tree, useful for testing (can't fit the whole tree in memory otherwise)
    // TODO: move this to just the testing
    template<typename C1, typename C2>
    struct Tree final
    {
        std::vector<LeafTuple> leaves;
        std::vector<Layer<C1>> c1_layers;
        std::vector<Layer<C2>> c2_layers;
    };

    LeafTuple output_to_leaf_tuple(const crypto::public_key &O, const crypto::public_key &C);
    std::vector<Selene::Scalar> flatten_leaves(const std::vector<LeafTuple> &leaves);

    // TODO: move into its own fcmp_crypto file
    template <typename C_POINTS, typename C_SCALARS>
    static void extend_scalars_from_cycle_points(const C_POINTS &curve,
        const std::vector<typename C_POINTS::Point> &points,
        std::vector<typename C_SCALARS::Scalar> &scalars_out)
    {
        scalars_out.reserve(scalars_out.size() + points.size());

        for (const auto &point : points)
        {
            // TODO: implement reading just the x coordinate of points on curves in curve cycle in C/C++
            typename C_SCALARS::Scalar scalar = curve.point_to_cycle_scalar(point);
            scalars_out.push_back(std::move(scalar));
        }
    }

    template<typename C2>
    LastChunkData<C2> get_last_leaf_chunk(const C2 &c2,
        const std::vector<LeafTuple> &leaves,
        const std::vector<typename C2::Point> &parent_layer)
    {
        CHECK_AND_ASSERT_THROW_MES(!leaves.empty(), "empty leaf layer");
        CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "empty leaf parent layer");

        const std::size_t child_offset = (leaves.size() * LEAF_TUPLE_SIZE) % LEAF_LAYER_CHUNK_SIZE;

        const typename C2::Scalar &last_child = leaves.back().C_x;
        const typename C2::Point &last_parent = parent_layer.back();

        return LastChunkData<C2>{
            .child_offset      = child_offset,
            .last_child        = c2.clone(last_child),
            .last_parent       = c2.clone(last_parent),
            .child_layer_size  = leaves.size() * LEAF_TUPLE_SIZE,
            .parent_layer_size = parent_layer.size()
        };
    }

    template<typename C_CHILD, typename C_PARENT>
    LastChunkData<C_PARENT> get_last_child_layer_chunk(const C_CHILD &c_child,
        const C_PARENT &c_parent,
        const std::vector<typename C_CHILD::Point> &child_layer,
        const std::vector<typename C_PARENT::Point> &parent_layer)
    {
        CHECK_AND_ASSERT_THROW_MES(!child_layer.empty(), "empty child layer");
        CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "empty parent layer");

        const std::size_t child_offset = child_layer.size() % c_parent.WIDTH;

        const typename C_CHILD::Point &last_child_point = child_layer.back();
        const typename C_PARENT::Scalar &last_child = c_child.point_to_cycle_scalar(last_child_point);

        const typename C_PARENT::Point &last_parent = parent_layer.back();

        return LastChunkData<C_PARENT>{
            .child_offset      = child_offset,
            .last_child        = c_parent.clone(last_child),
            .last_parent       = c_parent.clone(last_parent),
            .child_layer_size  = child_layer.size(),
            .parent_layer_size = parent_layer.size()
        };
    }

    // TODO: implement in the db, never want the entire tree in memory
    template<typename C1, typename C2>
    LastChunks<C1, C2> get_last_chunks(const C1 &c1,
        const C2 &c2,
        const Tree<C1, C2> &tree)
    {
        // const bool valid = validate_tree<C1, C2>(tree, C1, C2);
        // CHECK_AND_ASSERT_THROW_MES(valid, "invalid tree");

        const auto &leaves    = tree.leaves;
        const auto &c1_layers = tree.c1_layers;
        const auto &c2_layers = tree.c2_layers;

        // We started with c2 and then alternated, so c2 is the same size or 1 higher than c1
        CHECK_AND_ASSERT_THROW_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
            "unexpected number of curve layers");

        LastChunks<C1, C2> last_chunks;

        auto &c1_last_chunks_out = last_chunks.c1_last_chunks;
        auto &c2_last_chunks_out = last_chunks.c2_last_chunks;

        c1_last_chunks_out.reserve(c1_layers.size());
        c2_last_chunks_out.reserve(c2_layers.size());

        // First push the last leaf chunk data into c2 chunks
        CHECK_AND_ASSERT_THROW_MES(!c2_layers.empty(), "empty curve 2 layers");
        auto last_leaf_chunk = get_last_leaf_chunk<C2>(c2,
            leaves,
            c2_layers[0]);
        c2_last_chunks_out.push_back(std::move(last_leaf_chunk));

        // Next parents will be c1
        bool parent_is_c1 = true;

        // If there are no c1 layers, we're done
        if (c1_layers.empty())
            return last_chunks;

        // Then get last chunks up until the root
        std::size_t c1_idx = 0;
        std::size_t c2_idx = 0;
        while (c1_last_chunks_out.size() < c1_layers.size() || c2_last_chunks_out.size() < c2_layers.size())
        {
            CHECK_AND_ASSERT_THROW_MES(c1_layers.size() > c1_idx, "missing c1 layer");
            CHECK_AND_ASSERT_THROW_MES(c2_layers.size() > c2_idx, "missing c2 layer");

            // TODO: template the below if statement into another function
            if (parent_is_c1)
            {
                const Layer<C2> &child_layer = c2_layers[c2_idx];
                CHECK_AND_ASSERT_THROW_MES(!child_layer.empty(), "child layer is empty");

                const Layer<C1> &parent_layer = c1_layers[c1_idx];
                CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "parent layer is empty");

                auto last_parent_chunk = get_last_child_layer_chunk<C2, C1>(c2,
                    c1,
                    child_layer,
                    parent_layer);

                c1_last_chunks_out.push_back(std::move(last_parent_chunk));

                ++c2_idx;
            }
            else
            {
                const Layer<C1> &child_layer = c1_layers[c1_idx];
                CHECK_AND_ASSERT_THROW_MES(!child_layer.empty(), "child layer is empty");

                const Layer<C2> &parent_layer = c2_layers[c2_idx];
                CHECK_AND_ASSERT_THROW_MES(!parent_layer.empty(), "parent layer is empty");

                auto last_parent_chunk = get_last_child_layer_chunk<C1, C2>(c1,
                    c2,
                    child_layer,
                    parent_layer);

                c2_last_chunks_out.push_back(std::move(last_parent_chunk));

                ++c1_idx;
            }

            // Alternate curves every iteration
            parent_is_c1 = !parent_is_c1;
        }

        CHECK_AND_ASSERT_THROW_MES(c1_last_chunks_out.size() == c1_layers.size(), "unexepcted c1 last chunks");
        CHECK_AND_ASSERT_THROW_MES(c2_last_chunks_out.size() == c2_layers.size(), "unexepcted c2 last chunks");

        return last_chunks;
    }

    template <typename C>
    static void extend_zeroes(const C &curve,
        const std::size_t num_zeroes,
        std::vector<typename C::Scalar> &zeroes_inout)
    {
        zeroes_inout.reserve(zeroes_inout.size() + num_zeroes);

        for (std::size_t i = 0; i < num_zeroes; ++i)
            zeroes_inout.emplace_back(curve.zero_scalar());
    }

    template <typename C>
    static typename C::Point get_new_parent(const C &curve,
        const typename C::Chunk &new_children)
    {
        // New parent means no prior children, fill priors with 0
        std::vector<typename C::Scalar> prior_children;
        extend_zeroes(curve, new_children.size(), prior_children);

        return curve.hash_grow(
                curve.GENERATORS,
                curve.HASH_INIT_POINT,
                0,/*offset*/
                typename C::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    template <typename C>
    static typename C::Point get_first_leaf_parent(const C &curve,
        const typename C::Chunk &new_children,
        const LastChunkData<C> *last_chunk_ptr)
    {
        // If no last chunk exists, or if the last chunk is already full, then we can get a new parent
        if (last_chunk_ptr == nullptr || last_chunk_ptr->child_offset == 0)
            return get_new_parent<C>(curve, new_children);

        // There won't be any existing children when growing the leaf layer, fill priors with 0
        std::vector<typename C::Scalar> prior_children;
        extend_zeroes(curve, new_children.size(), prior_children);

        return curve.hash_grow(
                curve.GENERATORS,
                last_chunk_ptr->last_parent,
                last_chunk_ptr->child_offset,
                typename C::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    template <typename C>
    static typename C::Point get_first_non_leaf_parent(const C &curve,
        const typename C::Chunk &new_children,
        const bool child_layer_last_hash_updated,
        const LastChunkData<C> *last_chunk_ptr)
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
            offset = offset > 0 ? (offset - 1) : (curve.WIDTH - 1);

            // Extend prior children by zeroes for any additional new children, since they must be new
            if (new_children.size() > 1)
                extend_zeroes(curve, new_children.size() - 1, prior_children);
        }
        else if (offset > 0)
        {
            // If we're updating the parent hash and no children were updated, then we're just adding new children
            // to the existing last chunk and can fill priors with 0
            extend_zeroes(curve, new_children.size(), prior_children);
        }
        else
        {
            // If the last chunk is already full and isn't updated in any way, then we just get a new parent
            return get_new_parent<C>(curve, new_children);
        }

        return curve.hash_grow(
                curve.GENERATORS,
                last_chunk_ptr->last_parent,
                offset,
                typename C::Chunk{prior_children.data(), prior_children.size()},
                new_children
            );
    }

    template<typename C_CHILD, typename C_PARENT>
    void hash_layer(const C_CHILD &c_child,
        const C_PARENT &c_parent,
        const LastChunkData<C_CHILD> *last_child_chunk_ptr,
        const LastChunkData<C_PARENT> *last_parent_chunk_ptr,
        const LayerExtension<C_CHILD> &children,
        LayerExtension<C_PARENT> &parents_out)
    {
        parents_out.start_idx = (last_parent_chunk_ptr == nullptr) ? 0 : last_parent_chunk_ptr->parent_layer_size;
        parents_out.hashes.clear();

        CHECK_AND_ASSERT_THROW_MES(!children.hashes.empty(), "empty children hashes");

        const std::size_t max_chunk_size = c_parent.WIDTH;
        std::size_t offset = (last_parent_chunk_ptr == nullptr) ? 0 : last_parent_chunk_ptr->child_offset;

        // TODO: work through all edge cases, then try to simplify the approach to avoid them
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
        CHECK_AND_ASSERT_THROW_MES(max_chunk_size > offset, "unexpected offset");
        if (child_layer_last_hash_updated)
            offset = offset > 0 ? (offset - 1) : (max_chunk_size - 1);

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
        extend_scalars_from_cycle_points<C_CHILD, C_PARENT>(c_child, children.hashes, child_scalars);

        // See how many children we need to fill up the existing last chunk
        std::size_t chunk_size = std::min(child_scalars.size(), max_chunk_size - offset);
        MDEBUG("Starting chunk_size: " << chunk_size << " , child_scalars.size(): " << child_scalars.size() << " , offset: " << offset);

        // Hash chunks of child scalars to create the parent hashes
        std::size_t chunk_start_idx = 0;
        while (chunk_start_idx < child_scalars.size())
        {
            const auto chunk_start = child_scalars.data() + chunk_start_idx;
            const typename C_PARENT::Chunk chunk{chunk_start, chunk_size};

            for (const auto &c : chunk)
                MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " : " << c_parent.to_string(c));

            // Hash the chunk of children
            typename C_PARENT::Point chunk_hash = chunk_start_idx == 0
                ? get_first_non_leaf_parent<C_PARENT>(c_parent, chunk, child_layer_last_hash_updated, last_parent_chunk_ptr)
                : get_new_parent<C_PARENT>(c_parent, chunk);

            MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " result: " << c_parent.to_string(chunk_hash));

            // We've got our hash
            parents_out.hashes.emplace_back(std::move(chunk_hash));

            // Advance to the next chunk
            chunk_start_idx += chunk_size;

            // Prepare for next loop if there should be one
            if (chunk_start_idx == child_scalars.size())
                break;

            // Fill a complete chunk, or add the remaining new children to the last chunk
            CHECK_AND_ASSERT_THROW_MES(chunk_start_idx < child_scalars.size(), "unexpected chunk start idx");
            chunk_size = std::min(max_chunk_size, child_scalars.size() - chunk_start_idx);
        }
    }

    template<typename C2>
    void hash_leaf_layer(const C2 &c2,
        const LastChunkData<C2> *last_chunk_ptr,
        const Leaves &leaves,
        LayerExtension<C2> &parents_out)
    {
        parents_out.start_idx = (last_chunk_ptr == nullptr) ? 0 : last_chunk_ptr->parent_layer_size;
        parents_out.hashes.clear();

        if (leaves.tuples.empty())
            return;

        // Flatten leaves [(O.x, I.x, C.x),(O.x, I.x, C.x),...] -> [scalar, scalar, scalar, scalar, scalar, scalar,...]
        const std::vector<typename C2::Scalar> children = fcmp::flatten_leaves(leaves.tuples);

        const std::size_t max_chunk_size = LEAF_LAYER_CHUNK_SIZE;
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
                MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " : " << c2.to_string(c));

            // Hash the chunk of children
            typename C2::Point chunk_hash = chunk_start_idx == 0
                ? get_first_leaf_parent<C2>(c2, chunk, last_chunk_ptr)
                : get_new_parent<C2>(c2, chunk);

            MDEBUG("Hash chunk_start_idx " << chunk_start_idx << " result: " << c2.to_string(chunk_hash) << " , chunk_size: " << chunk_size);

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

    template<typename C1, typename C2>
    TreeExtension<C1, C2> get_tree_extension(const LastChunks<C1, C2> &existing_last_chunks,
        const Leaves &new_leaves,
        const C1 &c1,
        const C2 &c2)
    {
        TreeExtension<C1, C2> tree_extension;

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
                .O_x = SELENE.clone(leaf.O_x),
                .I_x = SELENE.clone(leaf.I_x),
                .C_x = SELENE.clone(leaf.C_x)
            });
        }

        auto &c1_layer_extensions_out = tree_extension.c1_layer_extensions;
        auto &c2_layer_extensions_out = tree_extension.c2_layer_extensions;

        // Hash the leaf layer
        LayerExtension<C2> parents;
        hash_leaf_layer<C2>(c2,
            c2_last_chunks.empty() ? nullptr : &c2_last_chunks[0],
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
                fcmp::hash_layer<C2, C1>(c2,
                    c1,
                    (c2_last_chunks.size() <= c2_last_idx) ? nullptr : &c2_last_chunks[c2_last_idx],
                    (c1_last_chunks.size() <= c1_last_idx) ? nullptr : &c1_last_chunks[c1_last_idx],
                    c2_layer_extensions_out[c2_last_idx],
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
                fcmp::hash_layer<C1, C2>(c1,
                    c2,
                    (c1_last_chunks.size() <= c1_last_idx) ? nullptr : &c1_last_chunks[c1_last_idx],
                    (c2_last_chunks.size() <= c2_last_idx) ? nullptr : &c2_last_chunks[c2_last_idx],
                    c1_layer_extensions_out[c1_last_idx],
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

    // TODO: this is only useful for testsing, can't fit entire tree in memory
    template<typename C1, typename C2>
    void extend_tree(const TreeExtension<C1, C2> &tree_extension,
        const C1 &c1,
        const C2 &c2,
        Tree<C1, C2> &tree_inout)
    {
        // Add the leaves
        CHECK_AND_ASSERT_THROW_MES((tree_inout.leaves.size() * LEAF_TUPLE_SIZE) == tree_extension.leaves.start_idx,
            "unexpected leaf start idx");

        tree_inout.leaves.reserve(tree_inout.leaves.size() + tree_extension.leaves.tuples.size());
        for (const auto &leaf : tree_extension.leaves.tuples)
        {
            tree_inout.leaves.emplace_back(LeafTuple{
                .O_x = c2.clone(leaf.O_x),
                .I_x = c2.clone(leaf.I_x),
                .C_x = c2.clone(leaf.C_x)
            });
        }

        // Add the layers
        const auto &c2_extensions = tree_extension.c2_layer_extensions;
        const auto &c1_extensions = tree_extension.c1_layer_extensions;
        CHECK_AND_ASSERT_THROW_MES(!c2_extensions.empty(), "empty c2 extensions");

        bool use_c2 = true;
        std::size_t c2_idx = 0;
        std::size_t c1_idx = 0;
        for (std::size_t i = 0; i < (c2_extensions.size() + c1_extensions.size()); ++i)
        {
            if (use_c2)
            {
                CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_extensions.size(), "unexpected c2 layer extension");
                const LayerExtension<C2> &c2_ext = c2_extensions[c2_idx];

                CHECK_AND_ASSERT_THROW_MES(!c2_ext.hashes.empty(), "empty c2 layer extension");

                CHECK_AND_ASSERT_THROW_MES(c2_idx <= tree_inout.c2_layers.size(), "missing c2 layer");
                if (tree_inout.c2_layers.size() == c2_idx)
                    tree_inout.c2_layers.emplace_back(Layer<C2>{});

                auto &c2_inout = tree_inout.c2_layers[c2_idx];

                const bool started_after_tip = (c2_inout.size() == c2_ext.start_idx);
                const bool started_at_tip    = (c2_inout.size() == (c2_ext.start_idx + 1));
                CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c2 layer start");

                // We updated the last hash
                if (started_at_tip)
                    c2_inout.back() = c2.clone(c2_ext.hashes.front());

                for (std::size_t i = started_at_tip ? 1 : 0; i < c2_ext.hashes.size(); ++i)
                    c2_inout.emplace_back(c2.clone(c2_ext.hashes[i]));

                ++c2_idx;
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_extensions.size(), "unexpected c1 layer extension");
                const fcmp::LayerExtension<C1> &c1_ext = c1_extensions[c1_idx];

                CHECK_AND_ASSERT_THROW_MES(!c1_ext.hashes.empty(), "empty c1 layer extension");

                CHECK_AND_ASSERT_THROW_MES(c1_idx <= tree_inout.c1_layers.size(), "missing c1 layer");
                if (tree_inout.c1_layers.size() == c1_idx)
                    tree_inout.c1_layers.emplace_back(Layer<C1>{});

                auto &c1_inout = tree_inout.c1_layers[c1_idx];

                const bool started_after_tip = (c1_inout.size() == c1_ext.start_idx);
                const bool started_at_tip    = (c1_inout.size() == (c1_ext.start_idx + 1));
                CHECK_AND_ASSERT_THROW_MES(started_after_tip || started_at_tip, "unexpected c1 layer start");

                // We updated the last hash
                if (started_at_tip)
                    c1_inout.back() = c1.clone(c1_ext.hashes.front());

                for (std::size_t i = started_at_tip ? 1 : 0; i < c1_ext.hashes.size(); ++i)
                    c1_inout.emplace_back(c1.clone(c1_ext.hashes[i]));

                ++c1_idx;
            }

            use_c2 = !use_c2;
        }

        // existing tree should be valid
        // assert(validate_tree<C1, C2>(existing_tree_inout, c1, c2));
    }

    template<typename C_PARENT>
    bool validate_layer(const C_PARENT &c_parent,
        const Layer<C_PARENT> &parents,
        const std::vector<typename C_PARENT::Scalar> &child_scalars,
        const std::size_t max_chunk_size)
    {
        // Hash chunk of children scalars, then see if the hash matches up to respective parent
        std::size_t chunk_start_idx = 0;
        for (std::size_t i = 0; i < parents.size(); ++i)
        {
            CHECK_AND_ASSERT_MES(child_scalars.size() > chunk_start_idx, false, "chunk start too high");
            const std::size_t chunk_size = std::min(child_scalars.size() - chunk_start_idx, max_chunk_size);
            CHECK_AND_ASSERT_MES(child_scalars.size() >= (chunk_start_idx + chunk_size), false, "chunk size too large");

            const typename C_PARENT::Point &parent = parents[i];

            const auto chunk_start = child_scalars.data() + chunk_start_idx;
            const typename C_PARENT::Chunk chunk{chunk_start, chunk_size};

            const typename C_PARENT::Point chunk_hash = get_new_parent(c_parent, chunk);

            const auto actual_bytes = c_parent.to_bytes(parent);
            const auto expected_bytes = c_parent.to_bytes(chunk_hash);
            CHECK_AND_ASSERT_MES(actual_bytes == expected_bytes, false, "unexpected hash");

            chunk_start_idx += chunk_size;
        }

        CHECK_AND_ASSERT_THROW_MES(chunk_start_idx == child_scalars.size(), "unexpected ending chunk start idx");

        return true;
    }

    template<typename C1, typename C2>
    bool validate_tree(const Tree<C1, C2> &tree, const C1 &c1, const C2 &c2)
    {
        const auto &leaves = tree.leaves;
        const auto &c1_layers = tree.c1_layers;
        const auto &c2_layers = tree.c2_layers;

        CHECK_AND_ASSERT_MES(!leaves.empty(), false, "must have at least 1 leaf in tree");
        CHECK_AND_ASSERT_MES(!c2_layers.empty(), false, "must have at least 1 c2 layer in tree");
        CHECK_AND_ASSERT_MES(c2_layers.size() == c1_layers.size() || c2_layers.size() == (c1_layers.size() + 1),
            false, "unexpected mismatch of c2 and c1 layers");

        // Verify root has 1 member in it
        const bool c2_is_root = c2_layers.size() > c1_layers.size();
        CHECK_AND_ASSERT_MES(c2_is_root ? c2_layers.back().size() == 1 : c1_layers.back().size() == 1, false,
            "root must have 1 member in it");

        // Iterate from root down to layer above leaves, and check hashes match up correctly
        bool parent_is_c2 = c2_is_root;
        std::size_t c2_idx = c2_layers.size() - 1;
        std::size_t c1_idx = c1_layers.empty() ? 0 : (c1_layers.size() - 1);
        for (std::size_t i = 1; i < (c2_layers.size() + c1_layers.size()); ++i)
        {
            if (parent_is_c2)
            {
                CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");
                CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");

                const Layer<C2> &parents  = c2_layers[c2_idx];
                const Layer<C1> &children = c1_layers[c1_idx];

                CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c2_idx " + std::to_string(c2_idx));
                CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c1_idx " + std::to_string(c1_idx));

                std::vector<typename C2::Scalar> child_scalars;
                extend_scalars_from_cycle_points<C1, C2>(c1, children, child_scalars);

                const bool valid = validate_layer<C2>(c2, parents, child_scalars, c2.WIDTH);

                CHECK_AND_ASSERT_MES(valid, false, "failed to validate c2_idx " + std::to_string(c2_idx));

                --c2_idx;
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(c1_idx < c1_layers.size(), "unexpected c1_idx");
                CHECK_AND_ASSERT_THROW_MES(c2_idx < c2_layers.size(), "unexpected c2_idx");

                const Layer<C1> &parents  = c1_layers[c1_idx];
                const Layer<C2> &children = c2_layers[c2_idx];

                CHECK_AND_ASSERT_MES(!parents.empty(), false, "no parents at c1_idx " + std::to_string(c1_idx));
                CHECK_AND_ASSERT_MES(!children.empty(), false, "no children at c2_idx " + std::to_string(c2_idx));

                std::vector<typename C1::Scalar> child_scalars;
                extend_scalars_from_cycle_points<C2, C1>(c2, children, child_scalars);

                const bool valid = validate_layer<C1>(c1, parents, child_scalars, c1.WIDTH);

                CHECK_AND_ASSERT_MES(valid, false, "failed to validate c1_idx " + std::to_string(c1_idx));

                --c1_idx;
            }

            parent_is_c2 = !parent_is_c2;
        }

        // Now validate leaves
        return validate_layer<C2>(c2, c2_layers[0], flatten_leaves(leaves), LEAF_LAYER_CHUNK_SIZE);
    }
}
