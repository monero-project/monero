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

#include "unit_tests_utils.h"

#include "fcmp_pp/curve_trees.h"
#include "fcmp_pp/proof_len.h"
#include "serialization/binary_utils.h"
#include "serialization/containers.h"
#include "serialization/crypto.h"
#include "serialization/serialization.h"

#include <filesystem>
#include <iosfwd>

static std::string get_fcmp_pp_filename(const std::size_t n_inputs)
{
    return unit_test::data_dir.string() + "/fcmp_pp_verify_inputs_" + std::to_string(n_inputs) + "in.bin";
}

// Useful struct for serializing all the components necessary to verify a FCMP++ proof
struct SerializableFcmpPpVerify final
{
  crypto::hash signable_tx_hash;
  uint8_t n_layers;
  fcmp_pp::FcmpPpProof fcmp_pp_proof;
  crypto::ec_point tree_root;
  std::vector<crypto::ec_point> pseudo_outs;
  std::vector<crypto::key_image> key_images;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(signable_tx_hash)
    FIELD(n_layers)
    FIELD(fcmp_pp_proof)
    FIELD(tree_root)
    FIELD(pseudo_outs)
    FIELD(key_images)
  END_SERIALIZE()
};

namespace unit_test
{
  bool write_fcmp_pp_verify_input_to_file(
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

    // Serialize the data into a string buffer
    SerializableFcmpPpVerify serializable_fcmp_pp_verify{
        .signable_tx_hash = signable_tx_hash,
        .n_layers = n_layers,
        .fcmp_pp_proof = fcmp_pp_proof,
        .tree_root = tree_root_bytes,
        .pseudo_outs = pseudo_outs,
        .key_images = key_images
      };

    std::string blob;
    const bool r = ::serialization::dump_binary(serializable_fcmp_pp_verify, blob);
    CHECK_AND_ASSERT_MES(r, false, "Failed to serialize proof data");

    // Write to file
    const std::string filename = get_fcmp_pp_filename(n_inputs);
    std::ofstream file(filename, std::ios::binary);
    CHECK_AND_ASSERT_MES(file, false, "Failed to open file");
    file << blob;

    LOG_PRINT_L0("Wrote FCMP++ proof to " << filename);

    return true;
  }

  bool read_fcmp_pp_verify_input_from_file(
    const std::size_t n_inputs,
    crypto::hash &signable_tx_hash,
    fcmp_pp::FcmpPpProof &fcmp_pp_proof,
    uint8_t &n_layers,
    fcmp_pp::TreeRootShared &tree_root,
    std::vector<crypto::ec_point> &pseudo_outs,
    std::vector<crypto::key_image> &key_images)
  {
    const std::string filename = get_fcmp_pp_filename(n_inputs);
    std::ifstream file(filename, std::ios::binary);
    CHECK_AND_ASSERT_MES(file, false, "Failed to open file");

    // Read the file into a string buffer
    const auto blob_size = std::filesystem::file_size(filename);
    std::string blob(blob_size, '\0');
    file.read(blob.data(), blob_size);

    // Parse the data and read into the return-by-refs
    SerializableFcmpPpVerify serializable_fcmp_pp_verify;
    const bool r = ::serialization::parse_binary(blob, serializable_fcmp_pp_verify);
    CHECK_AND_ASSERT_MES(r, false, "Failed to de-serialize proof data");

    signable_tx_hash = serializable_fcmp_pp_verify.signable_tx_hash;
    n_layers = serializable_fcmp_pp_verify.n_layers;

    const std::size_t proof_len = fcmp_pp::fcmp_pp_proof_len(n_inputs, n_layers);
    fcmp_pp_proof = std::move(serializable_fcmp_pp_verify.fcmp_pp_proof);
    CHECK_AND_ASSERT_MES(fcmp_pp_proof.size() == proof_len, false, "Unexpected FCMP++ proof size");

    const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1();
    const crypto::ec_point tree_root_bytes = serializable_fcmp_pp_verify.tree_root;
    tree_root = n_layers % 2 == 0
        ? fcmp_pp::helios_tree_root(curve_trees->m_c2->from_bytes(tree_root_bytes))
        : fcmp_pp::selene_tree_root(curve_trees->m_c1->from_bytes(tree_root_bytes));

    pseudo_outs = std::move(serializable_fcmp_pp_verify.pseudo_outs);
    key_images = std::move(serializable_fcmp_pp_verify.key_images);

    MDEBUG("signable_tx_hash: " << signable_tx_hash <<
        ", proof_size: "              << proof_len <<
        ", n_layers: "                << (std::size_t)n_layers <<
        ", tree_root_bytes: "         << tree_root_bytes);

    return true;
  }
}
