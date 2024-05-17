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

#include "tx_utils.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool set_fcmp_pp_proof_input(const curve_trees::OutputPair &output_pair,
    const curve_trees::TreeCacheV1 &tree_cache,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const std::shared_ptr<curve_trees::CurveTreesV1> &curve_trees,
    ProofInput &proof_input_out)
{
    // Get path ready to be used in the FCMP++ proof
    fcmp_pp::curve_trees::CurveTreesV1::Path path;
    bool r = tree_cache.get_output_path(output_pair, path);
    CHECK_AND_ASSERT_MES(r, false, "failed to get output path");
    CHECK_AND_ASSERT_MES(!path.empty(), false, "output path is empty");

    const auto output_tuple = fcmp_pp::curve_trees::output_to_tuple(output_pair);
    const auto path_for_proof = curve_trees->path_for_proof(path, output_tuple);

    const auto helios_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::HeliosT>(
        path_for_proof.c2_scalar_chunks);
    const auto selene_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::SeleneT>(
        path_for_proof.c1_scalar_chunks);

    // Set the path
    proof_input_out.path = fcmp_pp::path_new({path_for_proof.leaves.data(), path_for_proof.leaves.size()},
        path_for_proof.output_idx,
        {helios_scalar_chunks.data(), helios_scalar_chunks.size()},
        {selene_scalar_chunks.data(), selene_scalar_chunks.size()});

    // Collect blinds for rerandomized output
    // TODO: read blinds from a cache
    {
        const auto o_blind = fcmp_pp::o_blind(rerandomized_output);
        const auto i_blind = fcmp_pp::i_blind(rerandomized_output);
        const auto i_blind_blind = fcmp_pp::i_blind_blind(rerandomized_output);
        const auto c_blind = fcmp_pp::c_blind(rerandomized_output);

        const auto blinded_o_blind = fcmp_pp::blind_o_blind(o_blind);
        const auto blinded_i_blind = fcmp_pp::blind_i_blind(i_blind);
        const auto blinded_i_blind_blind = fcmp_pp::blind_i_blind_blind(i_blind_blind);
        const auto blinded_c_blind = fcmp_pp::blind_c_blind(c_blind);

        proof_input_out.output_blinds = fcmp_pp::output_blinds_new(blinded_o_blind,
            blinded_i_blind,
            blinded_i_blind_blind,
            blinded_c_blind);
    }

    // Collect branch blinds
    // TODO: read blinds from a cache
    {
        const std::size_t n_selene_layers = path.c1_layers.size();
        const std::size_t n_helios_layers = path.c2_layers.size();

        const bool is_selene_root = n_selene_layers > n_helios_layers;

        const std::size_t n_selene_layers_excl_root = n_selene_layers - (is_selene_root ? 1 : 0);
        const std::size_t n_helios_layers_excl_root = n_helios_layers - (is_selene_root ? 0 : 1);

        for (std::size_t i = 0; i < n_selene_layers_excl_root; ++i)
            proof_input_out.selene_branch_blinds.emplace_back(fcmp_pp::selene_branch_blind());
        for (std::size_t i = 0; i < n_helios_layers_excl_root; ++i)
            proof_input_out.helios_branch_blinds.emplace_back(fcmp_pp::helios_branch_blind());
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
