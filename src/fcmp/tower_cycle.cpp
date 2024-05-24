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
    auto res = fcmp_rust::hash_grow_helios(
        existing_hash,
        offset,
        prior_children,
        new_children);
    if (res.err != 0) {
      throw std::runtime_error("failed to hash grow");
    }
    return res.value;
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::hash_grow(
    const Selene::Point &existing_hash,
    const std::size_t offset,
    const Selene::Chunk &prior_children,
    const Selene::Chunk &new_children) const
{
    auto res = fcmp_rust::hash_grow_selene(
        existing_hash,
        offset,
        prior_children,
        new_children);
    if (res.err != 0) {
      throw std::runtime_error("failed to hash grow");
    }
    return res.value;
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
    auto bytes = fcmp_rust::helios_scalar_to_bytes(scalar);
    std::array<uint8_t, 32UL> res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Selene::to_bytes(const Selene::Scalar &scalar) const
{
    auto bytes = fcmp_rust::selene_scalar_to_bytes(scalar);
    std::array<uint8_t, 32UL> res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Helios::to_bytes(const Helios::Point &point) const
{
    auto bytes = fcmp_rust::helios_point_to_bytes(point);
    std::array<uint8_t, 32UL> res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
std::array<uint8_t, 32UL> Selene::to_bytes(const Selene::Point &point) const
{
    auto bytes = fcmp_rust::selene_point_to_bytes(point);
    std::array<uint8_t, 32UL> res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
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
// Exposed helper functions
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar ed_25519_point_to_scalar(const crypto::ec_point &point)
{
    return fcmp_rust::ed25519_point_to_selene_scalar((uint8_t*) &point.data);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
void extend_zeroes(const C &curve,
    const std::size_t num_zeroes,
    std::vector<typename C::Scalar> &zeroes_inout)
{
    zeroes_inout.reserve(zeroes_inout.size() + num_zeroes);

    for (std::size_t i = 0; i < num_zeroes; ++i)
        zeroes_inout.emplace_back(curve.zero_scalar());
}

// Explicit instantiations
template void extend_zeroes<Helios>(const Helios &curve,
    const std::size_t num_zeroes,
    std::vector<Helios::Scalar> &zeroes_inout);

template void extend_zeroes<Selene>(const Selene &curve,
    const std::size_t num_zeroes,
    std::vector<Selene::Scalar> &zeroes_inout);
//----------------------------------------------------------------------------------------------------------------------
template<typename C_POINTS, typename C_SCALARS>
void extend_scalars_from_cycle_points(const C_POINTS &curve,
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

// Explicit instantiations
template void extend_scalars_from_cycle_points<Helios, Selene>(const Helios &curve,
    const std::vector<Helios::Point> &points,
    std::vector<Selene::Scalar> &scalars_out);

template void extend_scalars_from_cycle_points<Selene, Helios>(const Selene &curve,
    const std::vector<Selene::Point> &points,
    std::vector<Helios::Scalar> &scalars_out);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace tower_cycle
} //namespace fcmp
