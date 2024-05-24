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
#include "fcmp_rust/fcmp++.h"

#include <string>

namespace fcmp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Rust types
//----------------------------------------------------------------------------------------------------------------------
using RustEd25519Point = std::array<uint8_t, 32UL>;

// Need to forward declare Scalar types for point_to_cycle_scalar below
using SeleneScalar = fcmp_rust::SeleneScalar;
using HeliosScalar = fcmp_rust::HeliosScalar;
//----------------------------------------------------------------------------------------------------------------------
struct HeliosT final
{
    using Generators  = fcmp_rust::HeliosGenerators;
    using Scalar      = HeliosScalar;
    using Point       = fcmp_rust::HeliosPoint;
    using Chunk       = fcmp_rust::HeliosScalarSlice;
    using CycleScalar = SeleneScalar;
};
//----------------------------------------------------------------------------------------------------------------------
struct SeleneT final
{
    using Generators  = fcmp_rust::SeleneGenerators;
    using Scalar      = SeleneScalar;
    using Point       = fcmp_rust::SelenePoint;
    using Chunk       = fcmp_rust::SeleneScalarSlice;
    using CycleScalar = HeliosScalar;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Abstract parent curve class that curves in a cycle must implement
template<typename C>
class Curve
{
//constructor
public:
    Curve(const typename C::Generators &generators, const typename C::Point &hash_init_point):
        m_generators{generators},
        m_hash_init_point{hash_init_point}
    {};

//member functions
public:
    // Read the x-coordinate from this curve's point to get this curve's cycle scalar
    virtual typename C::CycleScalar point_to_cycle_scalar(const typename C::Point &point) const = 0;

    virtual typename C::Point hash_grow(
        const typename C::Point &existing_hash,
        const std::size_t offset,
        const typename C::Chunk &prior_children,
        const typename C::Chunk &new_children) const = 0;

    virtual typename C::Scalar zero_scalar() const = 0;

    virtual std::array<uint8_t, 32UL> to_bytes(const typename C::Scalar &scalar) const = 0;
    virtual std::array<uint8_t, 32UL> to_bytes(const typename C::Point &point) const = 0;

    virtual std::string to_string(const typename C::Scalar &scalar) const = 0;
    virtual std::string to_string(const typename C::Point &point) const = 0;

//member variables
public:
    // TODO: make these static constants
    const typename C::Generators &m_generators;
    const typename C::Point &m_hash_init_point;
};
//----------------------------------------------------------------------------------------------------------------------
class Helios final : public Curve<HeliosT>
{
//typedefs
public:
    using Generators  = HeliosT::Generators;
    using Scalar      = HeliosT::Scalar;
    using Point       = HeliosT::Point;
    using Chunk       = HeliosT::Chunk;
    using CycleScalar = HeliosT::CycleScalar;

//constructor
public:
    Helios(const Generators &generators, const Point &hash_init_point)
    : Curve<HeliosT>(generators, hash_init_point)
    {};

//member functions
public:
    CycleScalar point_to_cycle_scalar(const Point &point) const override;

    Point hash_grow(
        const Point &existing_hash,
        const std::size_t offset,
        const Chunk &prior_children,
        const Chunk &new_children) const override;

    Scalar zero_scalar() const override;

    std::array<uint8_t, 32UL> to_bytes(const Scalar &scalar) const override;
    std::array<uint8_t, 32UL> to_bytes(const Point &point) const override;

    std::string to_string(const Scalar &scalar) const override;
    std::string to_string(const Point &point) const override;
};
//----------------------------------------------------------------------------------------------------------------------
class Selene final : public Curve<SeleneT>
{
//typedefs
public:
    using Generators  = SeleneT::Generators;
    using Scalar      = SeleneT::Scalar;
    using Point       = SeleneT::Point;
    using Chunk       = SeleneT::Chunk;
    using CycleScalar = SeleneT::CycleScalar;

//constructor
public:
    Selene(const Generators &generators, const Point &hash_init_point)
    : Curve<SeleneT>(generators, hash_init_point)
    {};

//member functions
public:
    CycleScalar point_to_cycle_scalar(const Point &point) const override;

    Point hash_grow(
        const Point &existing_hash,
        const std::size_t offset,
        const Chunk &prior_children,
        const Chunk &new_children) const override;

    Scalar zero_scalar() const override;

    std::array<uint8_t, 32UL> to_bytes(const Scalar &scalar) const override;
    std::array<uint8_t, 32UL> to_bytes(const Point &point) const override;

    std::string to_string(const Scalar &scalar) const override;
    std::string to_string(const Point &point) const override;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Ed25519 point x-coordinates are Selene scalars
SeleneScalar ed_25519_point_to_scalar(const crypto::ec_point &point);
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
void extend_zeroes(const C &curve,
    const std::size_t num_zeroes,
    std::vector<typename C::Scalar> &zeroes_inout);
//----------------------------------------------------------------------------------------------------------------------
template<typename C_POINTS, typename C_SCALARS>
void extend_scalars_from_cycle_points(const C_POINTS &curve,
    const std::vector<typename C_POINTS::Point> &points,
    std::vector<typename C_SCALARS::Scalar> &scalars_out);
//----------------------------------------------------------------------------------------------------------------------
// TODO: use static constants and get rid of the below functions (WARNING: number of generators must be >= curve's
// width, and also need to account for selene leaf layer 3x)
Helios::Generators random_helios_generators(std::size_t n);
Selene::Generators random_selene_generators(std::size_t n);
Helios::Point random_helios_hash_init_point();
Selene::Point random_selene_hash_init_point();
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace tower_cycle
}//namespace fcmp
