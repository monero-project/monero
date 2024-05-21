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
#include "string_tools.h"

#include <string>

namespace fcmp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
using RustEd25519Point = std::array<uint8_t, 32UL>;

// Need to forward declare Scalar types for point_to_cycle_scalar below
using SeleneScalar = rust::Box<fcmp_rust::SeleneScalar>;
using HeliosScalar = rust::Box<fcmp_rust::HeliosScalar>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace helios
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// TODO: Curve classes that inherit from a parent
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

    // Helios point x-coordinates are Selene scalars
    SeleneScalar point_to_cycle_scalar(const Point &point) const;

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
}//namespace helios
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace selene
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
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

    // Ed25519 point x-coordinates are Selene scalars
    SeleneScalar ed_25519_point_to_scalar(const crypto::ec_point &point) const;

    // Selene point x-coordinates are Helios scalars
    HeliosScalar point_to_cycle_scalar(const Point &point) const;

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
}// namespace selene
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curves
}//namespace fcmp
