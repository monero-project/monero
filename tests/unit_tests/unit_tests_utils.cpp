// Copyright (c) 2025, The Monero Project
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

#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/proof_len.h"

#include <iosfwd>

namespace unit_test
{
  bool write_fcmp_pp_verify_input_to_file(const std::string &filename,
    const std::size_t n_inputs,
    const crypto::hash &signable_tx_hash,
    const fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    const uint8_t n_layers,
    const crypto::ec_point &tree_root_bytes,
    const std::vector<crypto::ec_point> &pseudo_outs,
    const std::vector<crypto::key_image> &key_images)
  {
    CHECK_AND_ASSERT_MES(pseudo_outs.size() == n_inputs, false, "Unexpected size pseudo outs");
    CHECK_AND_ASSERT_MES(key_images.size() == n_inputs, false, "Unexpected size key_images");
    CHECK_AND_ASSERT_MES(fcmp_pp_proof.size() == fcmp_pp::fcmp_pp_proof_len(n_inputs, n_layers), false,
      "Bad FCMP++ proof size");

    LOG_PRINT_L1("Writing FCMP++ proof to " << filename);
    std::ofstream file(filename, std::ios::binary);
    CHECK_AND_ASSERT_MES(file, false, "Failed to open file");

    file.write(reinterpret_cast<const char*>(&signable_tx_hash), sizeof(signable_tx_hash));

    file.write(reinterpret_cast<const char*>(&n_layers), sizeof(uint8_t));
    file.write(reinterpret_cast<const char*>(fcmp_pp_proof.data()), fcmp_pp_proof.size());

    file.write(reinterpret_cast<const char*>(&tree_root_bytes), sizeof(tree_root_bytes));

    for (const auto &po : pseudo_outs)
        file.write(reinterpret_cast<const char*>(&po), sizeof(po));

    for (const auto &ki : key_images)
        file.write(reinterpret_cast<const char*>(&ki), sizeof(ki));

    file.close();

    return true;
  }

  bool read_fcmp_pp_verify_input_from_file(const std::string &filename,
    const std::size_t n_inputs,
    crypto::hash &signable_tx_hash,
    fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    uint8_t &n_layers,
    fcmp_pp::TreeRootShared &tree_root,
    std::vector<crypto::ec_point> &pseudo_outs,
    std::vector<crypto::key_image> &key_images)
  {
    std::ifstream file(filename, std::ios::binary);
    CHECK_AND_ASSERT_MES(file, false, "Failed to open file");

    file.read(reinterpret_cast<char*>(&signable_tx_hash), sizeof(signable_tx_hash));

    file.read(reinterpret_cast<char*>(&n_layers), sizeof(n_layers));
    const std::size_t proof_len = fcmp_pp::fcmp_pp_proof_len(n_inputs, n_layers);
    fcmp_pp_proof.resize(proof_len);
    file.read(reinterpret_cast<char*>(fcmp_pp_proof.data()), proof_len);

    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    crypto::ec_point tree_root_bytes;
    file.read(reinterpret_cast<char*>(&tree_root_bytes), sizeof(tree_root_bytes));
    tree_root = n_layers % 2 == 0
        ? fcmp_pp::helios_tree_root(curve_trees->m_c2->from_bytes(tree_root_bytes))
        : fcmp_pp::selene_tree_root(curve_trees->m_c1->from_bytes(tree_root_bytes));

    pseudo_outs = std::vector<crypto::ec_point>(n_inputs);
    for (auto &po : pseudo_outs)
        file.read(reinterpret_cast<char*>(&po), sizeof(po));

    key_images = std::vector<crypto::key_image>(n_inputs);
    for (auto &ki : key_images)
        file.read(reinterpret_cast<char*>(&ki), sizeof(ki));

    file.close();

    MDEBUG("signable_tx_hash: " << signable_tx_hash <<
        ", proof_size: "              << proof_len <<
        ", n_layers: "                << (std::size_t)n_layers <<
        ", tree_root_bytes: "         << tree_root_bytes);

    return true;
  }
}
