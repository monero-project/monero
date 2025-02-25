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

namespace fcmp_pp
{
namespace tower_cycle
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::hash_init_point() const
{
    return fcmp_pp_rust::selene_hash_init_point();
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::hash_init_point() const
{
    return fcmp_pp_rust::helios_hash_init_point();
}
//----------------------------------------------------------------------------------------------------------------------
Selene::CycleScalar Selene::point_to_cycle_scalar(const Selene::Point &point) const
{
    return fcmp_pp_rust::selene_point_to_helios_scalar(point);
}
//----------------------------------------------------------------------------------------------------------------------
Helios::CycleScalar Helios::point_to_cycle_scalar(const Helios::Point &point) const
{
    return fcmp_pp_rust::helios_point_to_selene_scalar(point);
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::hash_grow(
    const Selene::Point &existing_hash,
    const std::size_t offset,
    const Selene::Scalar &existing_child_at_offset,
    const Selene::Chunk &new_children) const
{
    auto result = fcmp_pp_rust::hash_grow_selene(
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children);

    if (result.err != nullptr)
    {
        free(result.err);
        throw std::runtime_error("failed to hash grow");
    }

    typename Selene::Point res;
    memcpy(&res, result.value, sizeof(typename Selene::Point));
    free(result.value);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::hash_trim(
    const Selene::Point &existing_hash,
    const std::size_t offset,
    const Selene::Chunk &children,
    const Selene::Scalar &child_to_grow_back) const
{
    auto result = fcmp_pp_rust::hash_trim_selene(
        existing_hash,
        offset,
        children,
        child_to_grow_back);

    if (result.err != nullptr)
    {
        free(result.err);
        throw std::runtime_error("failed to hash trim");
    }

    typename Selene::Point res;
    memcpy(&res, result.value, sizeof(typename Selene::Point));
    free(result.value);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::hash_grow(
    const Helios::Point &existing_hash,
    const std::size_t offset,
    const Helios::Scalar &existing_child_at_offset,
    const Helios::Chunk &new_children) const
{
    auto result = fcmp_pp_rust::hash_grow_helios(
        existing_hash,
        offset,
        existing_child_at_offset,
        new_children);

    if (result.err != nullptr)
    {
        free(result.err);
        throw std::runtime_error("failed to hash grow");
    }

    typename Helios::Point res;
    memcpy(&res, result.value, sizeof(typename Helios::Point));
    free(result.value);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::hash_trim(
    const Helios::Point &existing_hash,
    const std::size_t offset,
    const Helios::Chunk &children,
    const Helios::Scalar &child_to_grow_back) const
{
    auto result = fcmp_pp_rust::hash_trim_helios(
        existing_hash,
        offset,
        children,
        child_to_grow_back);

    if (result.err != nullptr)
    {
        free(result.err);
        throw std::runtime_error("failed to hash trim");
    }

    typename Helios::Point res;
    memcpy(&res, result.value, sizeof(typename Helios::Point));
    free(result.value);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Scalar Selene::zero_scalar() const
{
    return fcmp_pp_rust::selene_zero_scalar();
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Scalar Helios::zero_scalar() const
{
    return fcmp_pp_rust::helios_zero_scalar();
}
//----------------------------------------------------------------------------------------------------------------------
crypto::ec_scalar Selene::to_bytes(const Selene::Scalar &scalar) const
{
    auto bytes = fcmp_pp_rust::selene_scalar_to_bytes(scalar);
    crypto::ec_scalar res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
crypto::ec_scalar Helios::to_bytes(const Helios::Scalar &scalar) const
{
    auto bytes = fcmp_pp_rust::helios_scalar_to_bytes(scalar);
    crypto::ec_scalar res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
crypto::ec_point Selene::to_bytes(const Selene::Point &point) const
{
    auto bytes = fcmp_pp_rust::selene_point_to_bytes(point);
    crypto::ec_point res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
crypto::ec_point Helios::to_bytes(const Helios::Point &point) const
{
    auto bytes = fcmp_pp_rust::helios_point_to_bytes(point);
    crypto::ec_point res;
    memcpy(&res, bytes, 32);
    free(bytes);
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
Selene::Point Selene::from_bytes(const crypto::ec_point &bytes) const
{
    return fcmp_pp_rust::selene_point_from_bytes(reinterpret_cast<const uint8_t*>(&bytes));
}
//----------------------------------------------------------------------------------------------------------------------
Helios::Point Helios::from_bytes(const crypto::ec_point &bytes) const
{
    return fcmp_pp_rust::helios_point_from_bytes(reinterpret_cast<const uint8_t*>(&bytes));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Selene::to_string(const typename Selene::Scalar &scalar) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(scalar));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Helios::to_string(const typename Helios::Scalar &scalar) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(scalar));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Selene::to_string(const typename Selene::Point &point) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(point));
}
//----------------------------------------------------------------------------------------------------------------------
std::string Helios::to_string(const typename Helios::Point &point) const
{
    return epee::string_tools::pod_to_hex(this->to_bytes(point));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Exposed helper functions
//----------------------------------------------------------------------------------------------------------------------
SeleneScalar selene_scalar_from_bytes(const rct::key &scalar)
{
    return fcmp_pp_rust::selene_scalar_from_bytes(scalar.bytes);
}
//----------------------------------------------------------------------------------------------------------------------
template<typename C>
void extend_zeroes(const std::unique_ptr<C> &curve,
    const std::size_t num_zeroes,
    std::vector<typename C::Scalar> &zeroes_inout)
{
    zeroes_inout.reserve(zeroes_inout.size() + num_zeroes);

    for (std::size_t i = 0; i < num_zeroes; ++i)
        zeroes_inout.emplace_back(curve->zero_scalar());
}

// Explicit instantiations
template void extend_zeroes<Helios>(const std::unique_ptr<Helios> &curve,
    const std::size_t num_zeroes,
    std::vector<Helios::Scalar> &zeroes_inout);

template void extend_zeroes<Selene>(const std::unique_ptr<Selene> &curve,
    const std::size_t num_zeroes,
    std::vector<Selene::Scalar> &zeroes_inout);
//----------------------------------------------------------------------------------------------------------------------
template<typename C_POINTS, typename C_SCALARS>
void extend_scalars_from_cycle_points(const std::unique_ptr<C_POINTS> &curve,
    const std::vector<typename C_POINTS::Point> &points,
    std::vector<typename C_SCALARS::Scalar> &scalars_out)
{
    scalars_out.reserve(scalars_out.size() + points.size());

    for (const auto &point : points)
    {
        typename C_SCALARS::Scalar scalar = curve->point_to_cycle_scalar(point);
        scalars_out.push_back(std::move(scalar));
    }
}

// Explicit instantiations
template void extend_scalars_from_cycle_points<Helios, Selene>(const std::unique_ptr<Helios> &curve,
    const std::vector<Helios::Point> &points,
    std::vector<Selene::Scalar> &scalars_out);

template void extend_scalars_from_cycle_points<Selene, Helios>(const std::unique_ptr<Selene> &curve,
    const std::vector<Selene::Point> &points,
    std::vector<Helios::Scalar> &scalars_out);
//----------------------------------------------------------------------------------------------------------------------
uint8_t *selene_tree_root(const Selene::Point &point) { return fcmp_pp_rust::selene_tree_root(point); }
uint8_t *helios_tree_root(const Helios::Point &point) { return fcmp_pp_rust::helios_tree_root(point); }
//----------------------------------------------------------------------------------------------------------------------
static uint8_t *handle_res_ptr(const std::string func, const fcmp_pp_rust::CResult &res)
{
    if (res.err != nullptr)
    {
        free(res.err);
        throw std::runtime_error("failed to " + func);
    }
    return (uint8_t *) res.value;
}

uint8_t *rerandomize_output(const OutputBytes output)
{
    auto res = fcmp_pp_rust::rerandomize_output(output);
    return handle_res_ptr(__func__, res);
}

rct::key pseudo_out(const uint8_t *rerandomized_output)
{
    uint8_t * res_ptr = fcmp_pp_rust::pseudo_out(rerandomized_output);
    rct::key res;
    static_assert(sizeof(rct::key) == 32, "unexpected size of rct::key");
    memcpy(&res, res_ptr, sizeof(rct::key));
    free(res_ptr);
    return res;
}

uint8_t *o_blind(const uint8_t *rerandomized_output)
{
    auto result = fcmp_pp_rust::o_blind(rerandomized_output);
    return handle_res_ptr(__func__, result);
}

uint8_t *i_blind(const uint8_t *rerandomized_output)
{
    auto result = fcmp_pp_rust::i_blind(rerandomized_output);
    return handle_res_ptr(__func__, result);
}

uint8_t *i_blind_blind(const uint8_t *rerandomized_output)
{
    auto result = fcmp_pp_rust::i_blind_blind(rerandomized_output);
    return handle_res_ptr(__func__, result);
}

uint8_t *c_blind(const uint8_t *rerandomized_output)
{
    auto result = fcmp_pp_rust::c_blind(rerandomized_output);
    return handle_res_ptr(__func__, result);
}

uint8_t *blind_o_blind(const uint8_t *o_blind)
{
    auto res = fcmp_pp_rust::blind_o_blind(o_blind);
    return handle_res_ptr(__func__, res);
}

uint8_t *blind_i_blind(const uint8_t *i_blind)
{
    auto res = fcmp_pp_rust::blind_i_blind(i_blind);
    return handle_res_ptr(__func__, res);
}

uint8_t *blind_i_blind_blind(const uint8_t *i_blind_blind)
{
    auto res = fcmp_pp_rust::blind_i_blind_blind(i_blind_blind);
    return handle_res_ptr(__func__, res);
}

uint8_t *blind_c_blind(const uint8_t *c_blind)
{
    auto res = fcmp_pp_rust::blind_c_blind(c_blind);
    return handle_res_ptr(__func__, res);
}

uint8_t *path_new(const OutputChunk &leaves,
    std::size_t output_idx,
    const Helios::ScalarChunks &helios_layer_chunks,
    const Selene::ScalarChunks &selene_layer_chunks)
{
    auto res = fcmp_pp_rust::path_new(leaves, output_idx, helios_layer_chunks, selene_layer_chunks);
    return handle_res_ptr(__func__, res);
}

uint8_t *output_blinds_new(const uint8_t *blinded_o_blind,
    const uint8_t *blinded_i_blind,
    const uint8_t *blinded_i_blind_blind,
    const uint8_t *blinded_c_blind)
{
    auto res = fcmp_pp_rust::output_blinds_new(blinded_o_blind, blinded_i_blind, blinded_i_blind_blind, blinded_c_blind);
    return handle_res_ptr(__func__, res);
}

uint8_t *selene_branch_blind()
{
    const auto res = fcmp_pp_rust::selene_branch_blind();
    return handle_res_ptr(__func__, res);
}

uint8_t *helios_branch_blind()
{
    const auto res = fcmp_pp_rust::helios_branch_blind();
    return handle_res_ptr(__func__, res);
}

uint8_t *fcmp_prove_input_new(const uint8_t *x,
    const uint8_t *y,
    const uint8_t *rerandomized_output,
    const uint8_t *path,
    const uint8_t *output_blinds,
    const std::vector<const uint8_t *> &selene_branch_blinds,
    const std::vector<const uint8_t *> &helios_branch_blinds)
{
    auto res = fcmp_pp_rust::fcmp_prove_input_new(x,
        y,
        rerandomized_output,
        path,
        output_blinds,
        {selene_branch_blinds.data(), selene_branch_blinds.size()},
        {helios_branch_blinds.data(), helios_branch_blinds.size()});

    return handle_res_ptr(__func__, res);
}

FcmpPpProof prove(const crypto::hash &signable_tx_hash,
    const std::vector<const uint8_t *> &fcmp_prove_inputs,
    const std::size_t n_tree_layers)
{
    auto res = fcmp_pp_rust::prove(reinterpret_cast<const uint8_t*>(&signable_tx_hash),
        {fcmp_prove_inputs.data(), fcmp_prove_inputs.size()},
        n_tree_layers);

    if (res.err != nullptr)
    {
        free(res.err);
        throw std::runtime_error("failed to construct FCMP++ proof");
    }

    const std::size_t proof_size = fcmp_pp_rust::fcmp_pp_proof_size(fcmp_prove_inputs.size(), n_tree_layers);

    // res.value is a void * pointing to a uint8_t *, so cast as a double pointer
    uint8_t **buf = (uint8_t**) res.value;

    static_assert(sizeof(std::size_t) >= sizeof(uint8_t), "unexpected size of size_t");
    FcmpPpProof proof{
            .n_tree_layers = (uint8_t) n_tree_layers,
            .buf           = {*buf, *buf + proof_size}
        };

    // Free both pointers
    free(*buf);
    free(res.value);

    return proof;
}

bool verify(const crypto::hash &signable_tx_hash,
    const FcmpPpProof &fcmp_pp_proof,
    const uint8_t *tree_root,
    const std::vector<rct::key> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images)
{
    std::vector<const uint8_t *> pseudo_outs_ptrs;
    pseudo_outs_ptrs.reserve(pseudo_outs.size());
    for (const auto &po : pseudo_outs)
        pseudo_outs_ptrs.emplace_back((const uint8_t *)&po.bytes);

    std::vector<const uint8_t *> key_images_ptrs;
    key_images_ptrs.reserve(key_images.size());
    for (const auto &ki : key_images)
        key_images_ptrs.emplace_back((const uint8_t *)&ki.data);

    return fcmp_pp_rust::verify(
            reinterpret_cast<const uint8_t*>(&signable_tx_hash),
            {fcmp_pp_proof.buf.data(), fcmp_pp_proof.buf.size()},
            fcmp_pp_proof.n_tree_layers,
            tree_root,
            {pseudo_outs_ptrs.data(), pseudo_outs_ptrs.size()},
            {key_images_ptrs.data(), key_images_ptrs.size()}
        );
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace tower_cycle
} //namespace fcmp_pp
