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

#include "string_tools.h"
#include "tower_cycle.h"

namespace fcmp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
Helios::CycleScalar Helios::point_to_cycle_scalar(const Helios::Point &point) const
{
    return fcmp_rust::helios_point_to_selene_scalar(point);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::CycleScalar Selene::point_to_cycle_scalar(const Selene::Point &point) const
{
    return fcmp_rust::selene_point_to_helios_scalar(point);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::hash_grow(
    const Helios::Point &existing_hash,
    const std::size_t offset,
    const Helios::Chunk &prior_children,
    const Helios::Chunk &new_children) const
{
    return fcmp_rust::hash_grow_helios(
        m_generators,
        existing_hash,
        offset,
        prior_children,
        new_children);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::hash_grow(
    const Selene::Point &existing_hash,
    const std::size_t offset,
    const Selene::Chunk &prior_children,
    const Selene::Chunk &new_children) const
{
    return fcmp_rust::hash_grow_selene(
        m_generators,
        existing_hash,
        offset,
        prior_children,
        new_children);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Scalar Helios::clone(const Helios::Scalar &scalar) const
{
    return fcmp_rust::clone_helios_scalar(scalar);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Scalar Selene::clone(const Selene::Scalar &scalar) const
{
    return fcmp_rust::clone_selene_scalar(scalar);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::clone(const Helios::Point &point) const
{
    return fcmp_rust::clone_helios_point(point);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::clone(const Selene::Point &point) const
{
    return fcmp_rust::clone_selene_point(point);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Scalar Helios::zero_scalar() const
{
    return fcmp_rust::helios_zero_scalar();
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Scalar Selene::zero_scalar() const
{
    return fcmp_rust::selene_zero_scalar();
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Helios::to_bytes(const Helios::Scalar &scalar) const
{
    return fcmp_rust::helios_scalar_to_bytes(scalar);
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Selene::to_bytes(const Selene::Scalar &scalar) const
{
    return fcmp_rust::selene_scalar_to_bytes(scalar);
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Helios::to_bytes(const Helios::Point &point) const
{
    return fcmp_rust::helios_point_to_bytes(point);
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Selene::to_bytes(const Selene::Point &point) const
{
    return fcmp_rust::selene_point_to_bytes(point);
}
//----------------------------------------------------------------------------------------------------------------------
std::string Helios::to_string(const typename Helios::Scalar &scalar) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(scalar));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Selene::to_string(const typename Selene::Scalar &scalar) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(scalar));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Helios::to_string(const typename Helios::Point &point) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(point));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Selene::to_string(const typename Selene::Point &point) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(point));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar ed_25519_point_to_scalar(const crypto::ec_point &point)
{
    static_assert(sizeof(RustEd25519Point) == sizeof(crypto::ec_point),
        "expected same size ed25519 point to rust representation");

    // TODO: implement reading just the x coordinate of ed25519 point in C/C++
    fcmp::tower_cycle::RustEd25519Point rust_point;
    memcpy(&rust_point, &point, sizeof(fcmp::tower_cycle::RustEd25519Point));
    return fcmp_rust::ed25519_point_to_selene_scalar(rust_point);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Generators random_helios_generators(std::size_t n)
{
    return fcmp_rust::random_helios_generators(n);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Generators random_selene_generators(std::size_t n)
{
    return fcmp_rust::random_selene_generators(n);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point random_helios_hash_init_point()
{
    return fcmp_rust::random_helios_hash_init_point();
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point random_selene_hash_init_point()
{
    return fcmp_rust::random_selene_hash_init_point();
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace tower_cycle
} //namespace fcmp
