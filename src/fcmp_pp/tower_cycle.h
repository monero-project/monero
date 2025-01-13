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
#include "ringct/rctTypes.h"

#include <string>

namespace fcmp_pp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Rust types
//----------------------------------------------------------------------------------------------------------------------
using OutputBytes = fcmp_pp_rust::OutputBytes;
using OutputChunk = fcmp_pp_rust::OutputSlice;
using RerandomizedOutput = fcmp_pp_rust::RerandomizedOutput;
using Blind = fcmp_pp_rust::Blind;
using BlindedPoint = fcmp_pp_rust::BlindedPoint;
using OutputBlinds = fcmp_pp_rust::OutputBlinds;
using BranchBlind = fcmp_pp_rust::BranchBlind;
using BranchBlinds = fcmp_pp_rust::BranchBlindSlice;
using Ed25519Scalar = fcmp_pp_rust::Ed25519ScalarBytes;
using TreeRoot = fcmp_pp_rust::TreeRoot;
//----------------------------------------------------------------------------------------------------------------------
// Need to forward declare Scalar types for point_to_cycle_scalar below
using SeleneScalar = fcmp_pp_rust::SeleneScalar;
using HeliosScalar = fcmp_pp_rust::HeliosScalar;
//----------------------------------------------------------------------------------------------------------------------
struct HeliosT final
{
    using Scalar       = HeliosScalar;
    using Point        = fcmp_pp_rust::HeliosPoint;
    using Chunk        = fcmp_pp_rust::HeliosScalarSlice;
    using CycleScalar  = SeleneScalar;
    using ScalarChunks = fcmp_pp_rust::HeliosScalarChunks;
};
//----------------------------------------------------------------------------------------------------------------------
struct SeleneT final
{
    using Scalar       = SeleneScalar;
    using Point        = fcmp_pp_rust::SelenePoint;
    using Chunk        = fcmp_pp_rust::SeleneScalarSlice;
    using CycleScalar  = HeliosScalar;
    using ScalarChunks = fcmp_pp_rust::SeleneScalarChunks;
};
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

    virtual typename C::Point hash_trim(
        const typename C::Point &existing_hash,
        const std::size_t offset,
        const typename C::Chunk &children,
        const typename C::Scalar &child_to_grow_back) const = 0;

    virtual typename C::Scalar zero_scalar() const = 0;

    virtual crypto::ec_scalar to_bytes(const typename C::Scalar &scalar) const = 0;
    virtual crypto::ec_point to_bytes(const typename C::Point &point) const = 0;

    virtual typename C::Point from_bytes(const crypto::ec_point &bytes) const = 0;

    virtual std::string to_string(const typename C::Scalar &scalar) const = 0;
    virtual std::string to_string(const typename C::Point &point) const = 0;
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

    Point hash_trim(
        const Point &existing_hash,
        const std::size_t offset,
        const Chunk &children,
        const Scalar &child_to_grow_back) const override;

    Scalar zero_scalar() const override;

    crypto::ec_scalar to_bytes(const Scalar &scalar) const override;
    crypto::ec_point to_bytes(const Point &point) const override;

    Point from_bytes(const crypto::ec_point &bytes) const override;

    std::string to_string(const Scalar &scalar) const override;
    std::string to_string(const Point &point) const override;
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

    Point hash_trim(
        const Point &existing_hash,
        const std::size_t offset,
        const Chunk &children,
        const Scalar &child_to_grow_back) const override;

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
TreeRoot selene_tree_root(const Selene::Point &point);
TreeRoot helios_tree_root(const Helios::Point &point);
//----------------------------------------------------------------------------------------------------------------------
// TODO: consider putting this section somewhere better than tower_cycle
RerandomizedOutput rerandomize_output(const OutputBytes output);

Blind o_blind(const RerandomizedOutput rerandomized_output);
Blind i_blind(const RerandomizedOutput rerandomized_output);
Blind i_blind_blind(const RerandomizedOutput rerandomized_output);
Blind c_blind(const RerandomizedOutput rerandomized_output);

BlindedPoint blind_o_blind(const Blind o_blind);
BlindedPoint blind_i_blind(const Blind i_blind);
BlindedPoint blind_i_blind_blind(const Blind i_blind_blind);
BlindedPoint blind_c_blind(const Blind c_blind);

OutputBlinds output_blinds_new(const Blind blinded_o_blind,
    const Blind blinded_i_blind,
    const Blind blinded_i_blind_blind,
    const Blind blinded_c_blind);

BranchBlind helios_branch_blind();
BranchBlind selene_branch_blind();

void prove(const Ed25519Scalar x,
    const Ed25519Scalar y,
    std::size_t output_idx,
    const OutputChunk &leaves,
    const Helios::ScalarChunks &c1_layer_chunks,
    const Selene::ScalarChunks &c2_layer_chunks,
    const RerandomizedOutput rerandomized_output,
    const OutputBlinds output_blinds,
    const BranchBlinds &c1_branch_blinds,
    const BranchBlinds &c2_branch_blinds,
    const TreeRoot tree_root);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace tower_cycle
}//namespace fcmp_pp
