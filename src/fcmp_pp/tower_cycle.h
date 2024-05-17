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
#include "fcmp_pp_rust/fcmp++.h"
#include "fcmp_pp_types.h"
#include "ringct/rctTypes.h"

#include <string>

namespace fcmp_pp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Abstract parent curve class that curves in a cycle must implement
template<typename C>
class Curve
{
//member functions
public:
    virtual typename C::Point hash_init_point() const = 0;

    // Read the x-coordinate from this curve's point to get this curve's cycle scalar
    virtual typename C::CycleScalar point_to_cycle_scalar(const typename C::Point &point) const = 0;

    virtual typename C::Point hash_grow(
        const typename C::Point &existing_hash,
        const std::size_t offset,
        const typename C::Scalar &existing_child_at_offset,
        const typename C::Chunk &new_children) const = 0;

    virtual typename C::Scalar zero_scalar() const = 0;

    virtual crypto::ec_scalar to_bytes(const typename C::Scalar &scalar) const = 0;
    virtual crypto::ec_point to_bytes(const typename C::Point &point) const = 0;

    virtual typename C::Point from_bytes(const crypto::ec_point &bytes) const = 0;

    virtual std::string to_string(const typename C::Scalar &scalar) const = 0;
    virtual std::string to_string(const typename C::Point &point) const = 0;
};
//----------------------------------------------------------------------------------------------------------------------
class Selene final : public Curve<SeleneT>
{
//typedefs
public:
    using Scalar       = SeleneT::Scalar;
    using Point        = SeleneT::Point;
    using Chunk        = SeleneT::Chunk;
    using CycleScalar  = SeleneT::CycleScalar;
    using ScalarChunks = SeleneT::ScalarChunks;

//member functions
public:
    Point hash_init_point() const override;

    CycleScalar point_to_cycle_scalar(const Point &point) const override;

    Point hash_grow(
        const Point &existing_hash,
        const std::size_t offset,
        const Scalar &existing_child_at_offset,
        const Chunk &new_children) const override;

    Scalar zero_scalar() const override;

    crypto::ec_scalar to_bytes(const Scalar &scalar) const override;
    crypto::ec_point to_bytes(const Point &point) const override;

    Point from_bytes(const crypto::ec_point &bytes) const override;

    std::string to_string(const Scalar &scalar) const override;
    std::string to_string(const Point &point) const override;
};
//----------------------------------------------------------------------------------------------------------------------
class Helios final : public Curve<HeliosT>
{
//typedefs
public:
    using Scalar       = HeliosT::Scalar;
    using Point        = HeliosT::Point;
    using Chunk        = HeliosT::Chunk;
    using CycleScalar  = HeliosT::CycleScalar;
    using ScalarChunks = HeliosT::ScalarChunks;

//member functions
public:
    Point hash_init_point() const override;

    CycleScalar point_to_cycle_scalar(const Point &point) const override;

    Point hash_grow(
        const Point &existing_hash,
        const std::size_t offset,
        const Scalar &existing_child_at_offset,
        const Chunk &new_children) const override;

    Scalar zero_scalar() const override;

    crypto::ec_scalar to_bytes(const Scalar &scalar) const override;
    crypto::ec_point to_bytes(const Point &point) const override;

    Point from_bytes(const crypto::ec_point &bytes) const override;

    std::string to_string(const Scalar &scalar) const override;
    std::string to_string(const Point &point) const override;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar selene_scalar_from_bytes(const rct::key &scalar);
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
void extend_zeroes(const std::unique_ptr<C> &curve,
    const std::size_t num_zeroes,
    std::vector<typename C::Scalar> &zeroes_inout);
//----------------------------------------------------------------------------------------------------------------------
template<typename C_POINTS, typename C_SCALARS>
void extend_scalars_from_cycle_points(const std::unique_ptr<C_POINTS> &curve,
    const std::vector<typename C_POINTS::Point> &points,
    std::vector<typename C_SCALARS::Scalar> &scalars_out);
//----------------------------------------------------------------------------------------------------------------------
uint8_t *selene_tree_root(const Selene::Point &point);
uint8_t *helios_tree_root(const Helios::Point &point);
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
std::vector<typename C::Chunk> scalar_chunks_to_chunk_vector(
    const std::vector<std::vector<typename C::Scalar>> &scalar_chunks);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace tower_cycle
}//namespace fcmp_pp
